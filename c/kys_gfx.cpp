#include "kys_gfx.h"

#include <algorithm>
#include <cstddef>
#include <cmath>
#include <format>
#include <numbers>

namespace
{
constexpr int kColorBytes = 4;

struct SurfaceHolder
{
    SDL_Surface* surface = nullptr;

    SurfaceHolder() = default;
    explicit SurfaceHolder(SDL_Surface* value) : surface(value) {}

    SurfaceHolder(const SurfaceHolder&) = delete;
    SurfaceHolder& operator=(const SurfaceHolder&) = delete;

    SurfaceHolder(SurfaceHolder&& other) noexcept : surface(other.surface)
    {
        other.surface = nullptr;
    }

    SurfaceHolder& operator=(SurfaceHolder&& other) noexcept
    {
        if (this != &other)
        {
            if (surface != nullptr)
            {
                SDL_DestroySurface(surface);
            }
            surface = other.surface;
            other.surface = nullptr;
        }
        return *this;
    }

    ~SurfaceHolder()
    {
        if (surface != nullptr)
        {
            SDL_DestroySurface(surface);
        }
    }

    [[nodiscard]] SDL_Surface* release() noexcept
    {
        SDL_Surface* value = surface;
        surface = nullptr;
        return value;
    }
};

[[nodiscard]] bool MustLock(SDL_Surface* src)
{
    return src != nullptr && SDL_MUSTLOCK(src);
}

void ComputeRotozoomSize(int width, int height, double angle, double zoomx, double zoomy, int& dstWidth, int& dstHeight)
{
    const double radAngle = angle * (std::numbers::pi_v<double> / 180.0);
    const double sinAngle = std::sin(radAngle) * zoomx;
    const double cosAngle = std::cos(radAngle) * zoomy;

    const double x = static_cast<double>(width) / 2.0;
    const double y = static_cast<double>(height) / 2.0;

    const double cx = cosAngle * x;
    const double cy = cosAngle * y;
    const double sx = sinAngle * x;
    const double sy = sinAngle * y;

    const int dstWidthHalf = std::max(
        static_cast<int>(std::ceil(std::max({
            std::abs(cx + sy),
            std::abs(cx - sy),
            std::abs(-cx + sy),
            std::abs(-cx - sy),
        }))),
        1);

    const int dstHeightHalf = std::max(
        static_cast<int>(std::ceil(std::max({
            std::abs(sx + cy),
            std::abs(sx - cy),
            std::abs(-sx + cy),
            std::abs(-sx - cy),
        }))),
        1);

    dstWidth = dstWidthHalf * 2;
    dstHeight = dstHeightHalf * 2;
}

void ComputeZoomSize(int width, int height, double zoomx, double zoomy, int& dstWidth, int& dstHeight)
{
    dstWidth = std::max(static_cast<int>(std::lround(width * zoomx + 0.5)), 1);
    dstHeight = std::max(static_cast<int>(std::lround(height * zoomy + 0.5)), 1);
}

[[nodiscard]] SDL_Color ReadPixel(const SDL_Surface* surface, int x, int y)
{
    const auto* row = static_cast<const std::byte*>(surface->pixels) + static_cast<std::ptrdiff_t>(y) * surface->pitch;
    const auto* pixel = reinterpret_cast<const SDL_Color*>(row) + x;
    return *pixel;
}

void WritePixel(SDL_Surface* surface, int x, int y, const SDL_Color& color)
{
    auto* row = static_cast<std::byte*>(surface->pixels) + static_cast<std::ptrdiff_t>(y) * surface->pitch;
    auto* pixel = reinterpret_cast<SDL_Color*>(row) + x;
    *pixel = color;
}

[[nodiscard]] Uint8 LerpChannel(Uint8 c00, Uint8 c01, Uint8 c10, Uint8 c11, double fx, double fy)
{
    const double top = std::lerp(static_cast<double>(c00), static_cast<double>(c01), fx);
    const double bottom = std::lerp(static_cast<double>(c10), static_cast<double>(c11), fx);
    const double value = std::lerp(top, bottom, fy);
    return static_cast<Uint8>(std::clamp(std::lround(value), 0l, 255l));
}

[[nodiscard]] SDL_Color SampleNearest(const SDL_Surface* surface, double sourceX, double sourceY)
{
    const int pixelX = static_cast<int>(std::floor(sourceX));
    const int pixelY = static_cast<int>(std::floor(sourceY));
    if (pixelX < 0 || pixelY < 0 || pixelX >= surface->w || pixelY >= surface->h)
    {
        return SDL_Color{ 0, 0, 0, 0 };
    }

    return ReadPixel(surface, pixelX, pixelY);
}

[[nodiscard]] SDL_Color SampleLinear(const SDL_Surface* surface, double sourceX, double sourceY)
{
    if (sourceX < 0.0 || sourceY < 0.0 || sourceX >= static_cast<double>(surface->w) || sourceY >= static_cast<double>(surface->h))
    {
        return SDL_Color{ 0, 0, 0, 0 };
    }

    const int x0 = static_cast<int>(std::floor(sourceX));
    const int y0 = static_cast<int>(std::floor(sourceY));
    const int x1 = std::min(x0 + 1, surface->w - 1);
    const int y1 = std::min(y0 + 1, surface->h - 1);
    const double fx = sourceX - static_cast<double>(x0);
    const double fy = sourceY - static_cast<double>(y0);

    const SDL_Color c00 = ReadPixel(surface, x0, y0);
    const SDL_Color c01 = ReadPixel(surface, x1, y0);
    const SDL_Color c10 = ReadPixel(surface, x0, y1);
    const SDL_Color c11 = ReadPixel(surface, x1, y1);

    return SDL_Color{
        LerpChannel(c00.r, c01.r, c10.r, c11.r, fx, fy),
        LerpChannel(c00.g, c01.g, c10.g, c11.g, fx, fy),
        LerpChannel(c00.b, c01.b, c10.b, c11.b, fx, fy),
        LerpChannel(c00.a, c01.a, c10.a, c11.a, fx, fy),
    };
}

[[nodiscard]] SurfaceHolder ConvertToRgba32(SDL_Surface* source)
{
    SurfaceHolder converted(SDL_CreateSurface(source->w, source->h, SDL_PIXELFORMAT_RGBA32));
    if (converted.surface == nullptr)
    {
        return converted;
    }

    if (!SDL_BlitSurface(source, nullptr, converted.surface, nullptr))
    {
        return SurfaceHolder();
    }

    return converted;
}
}

SDL_Surface* rotozoomSurfaceXY(SDL_Surface* src, double angle, double zoomx, double zoomy, int smooth)
{
    if (src == nullptr)
    {
        return nullptr;
    }

    const bool flipX = zoomx < 0.0;
    const bool flipY = zoomy < 0.0;
    zoomx = std::max(std::abs(zoomx), KysGfxValueLimit);
    zoomy = std::max(std::abs(zoomy), KysGfxValueLimit);

    SurfaceHolder converted = ConvertToRgba32(src);
    if (converted.surface == nullptr)
    {
        return nullptr;
    }

    SDL_Surface* source = converted.surface;

    int dstWidth = 0;
    int dstHeight = 0;
    if (std::abs(angle) > KysGfxValueLimit)
    {
        ComputeRotozoomSize(source->w, source->h, angle, zoomx, zoomy, dstWidth, dstHeight);
    }
    else
    {
        ComputeZoomSize(source->w, source->h, zoomx, zoomy, dstWidth, dstHeight);
    }

    SurfaceHolder destination(SDL_CreateSurface(dstWidth, dstHeight, SDL_PIXELFORMAT_RGBA32));
    if (destination.surface == nullptr)
    {
        return nullptr;
    }

    const bool sourceLocked = MustLock(source) && SDL_LockSurface(source);
    const bool destinationLocked = MustLock(destination.surface) && SDL_LockSurface(destination.surface);
    if ((MustLock(source) && !sourceLocked) || (MustLock(destination.surface) && !destinationLocked))
    {
        if (destinationLocked)
        {
            SDL_UnlockSurface(destination.surface);
        }
        if (sourceLocked)
        {
            SDL_UnlockSurface(source);
        }
        return nullptr;
    }

    const double radians = angle * (std::numbers::pi_v<double> / 180.0);
    const double cosine = std::cos(radians);
    const double sine = std::sin(radians);
    const double sourceCenterX = (static_cast<double>(source->w) - 1.0) / 2.0;
    const double sourceCenterY = (static_cast<double>(source->h) - 1.0) / 2.0;
    const double destinationCenterX = (static_cast<double>(dstWidth) - 1.0) / 2.0;
    const double destinationCenterY = (static_cast<double>(dstHeight) - 1.0) / 2.0;

    for (int y = 0; y < dstHeight; ++y)
    {
        for (int x = 0; x < dstWidth; ++x)
        {
            double dx = static_cast<double>(x) - destinationCenterX;
            double dy = static_cast<double>(y) - destinationCenterY;

            if (flipX)
            {
                dx = -dx;
            }
            if (flipY)
            {
                dy = -dy;
            }

            const double rotatedX = cosine * dx + sine * dy;
            const double rotatedY = -sine * dx + cosine * dy;

            const double sourceX = rotatedX / zoomx + sourceCenterX;
            const double sourceY = rotatedY / zoomy + sourceCenterY;

            const SDL_Color color = smooth != 0
                ? SampleLinear(source, sourceX, sourceY)
                : SampleNearest(source, sourceX, sourceY);
            WritePixel(destination.surface, x, y, color);
        }
    }

    if (destinationLocked)
    {
        SDL_UnlockSurface(destination.surface);
    }
    if (sourceLocked)
    {
        SDL_UnlockSurface(source);
    }

    return destination.release();
}

std::string DescribeGraphicsModule()
{
    return std::format("{} -> {}", "kys_gfx.pas", "rotozoomSurfaceXY translated to SDL3 RGBA32 implementation");
}