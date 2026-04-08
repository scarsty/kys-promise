#include "kys_main.h"

#include "kys_battle.h"
#include "kys_engine.h"
#include "kys_event.h"
#include "kys_gfx.h"
#include "kys_littlegame.h"

#include <SDL3/SDL.h>

#include <array>
#include <format>
#include <iostream>
#include <string>

namespace
{
void RunGraphicsSmokeTest()
{
    SDL_Surface* source = SDL_CreateSurface(8, 8, SDL_PIXELFORMAT_RGBA32);
    if (source == nullptr)
    {
        std::cout << "gfx smoke test: source creation failed" << '\n';
        return;
    }

    SDL_Surface* rotated = rotozoomSurfaceXY(source, 30.0, 1.5, 0.75, 1);
    if (rotated == nullptr)
    {
        std::cout << "gfx smoke test: rotozoom failed" << '\n';
        SDL_DestroySurface(source);
        return;
    }

    std::cout << std::format("gfx smoke test: {}x{} -> {}x{}", source->w, source->h, rotated->w, rotated->h) << '\n';

    SDL_DestroySurface(rotated);
    SDL_DestroySurface(source);
}
}

int Run(int argc, char* argv[])
{
    const std::array<std::string, 5> moduleSummaries = {
        DescribeBattleModule(),
        DescribeEngineModule(),
        DescribeEventModule(),
        DescribeGraphicsModule(),
        DescribeLittleGameModule(),
    };

    std::cout << "kys-promise C++ scaffold" << '\n';
    std::cout << "source project: Pascal units mapped to one header/source pair each" << '\n';

    for (const std::string& summary : moduleSummaries)
    {
        std::cout << summary << '\n';
    }

    RunGraphicsSmokeTest();

    std::cout << std::format("argc = {}", argc) << '\n';
    for (int index = 0; index < argc; ++index)
    {
        std::cout << std::format("argv[{}] = {}", index, argv[index] == nullptr ? "" : argv[index]) << '\n';
    }

    return 0;
}