#pragma once

#include <SDL3/SDL.h>

#include <string>

constexpr double KysGfxValueLimit = 0.001;

SDL_Surface* rotozoomSurfaceXY(SDL_Surface* src, double angle, double zoomx, double zoomy, int smooth);
std::string DescribeGraphicsModule();