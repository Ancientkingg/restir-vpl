//
// Created by rgardishyan on 5/6/25.
//

#ifndef VIEWER_H

#include <SDL2/SDL.h>
#include <glm/glm.hpp>
#include <vector>
#include <iostream>

// Call this once at program start:
bool init_sdl()
{
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cerr << "SDL_Init Error: " << SDL_GetError() << "\n";
        return false;
    }
    return true;
}

// Call this at program end:
void shutdown_sdl()
{
    SDL_Quit();
}

// Creates an SDL window + renderer + texture for live‐viewing.
// Returns true on success, and fills out the handles.
bool create_view_window(int width, int height,
                        SDL_Window*&  outWindow,
                        SDL_Renderer*& outRenderer,
                        SDL_Texture*& outTexture)
{
    outWindow = SDL_CreateWindow("Live View",
                                 SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                 width, height,
                                 SDL_WINDOW_SHOWN);
    if (!outWindow) {
        std::cerr << "SDL_CreateWindow Error: " << SDL_GetError() << "\n";
        return false;
    }

    outRenderer = SDL_CreateRenderer(outWindow, -1, SDL_RENDERER_ACCELERATED);
    if (!outRenderer) {
        std::cerr << "SDL_CreateRenderer Error: " << SDL_GetError() << "\n";
        return false;
    }

    // Note: SDL_PIXELFORMAT_RGB24 expects 3 bytes per pixel, R8G8B8
    outTexture = SDL_CreateTexture(outRenderer,
                                   SDL_PIXELFORMAT_RGB24,
                                   SDL_TEXTUREACCESS_STREAMING,
                                   width, height);
    if (!outTexture) {
        std::cerr << "SDL_CreateTexture Error: " << SDL_GetError() << "\n";
        return false;
    }

    return true;
}

// Call once per frame after you've filled `pixels`:
void update_and_present(SDL_Renderer* renderer,
                        SDL_Texture*  texture,
                        const std::vector<std::vector<glm::vec3>>& pixels)
{
    int height = int(pixels.size());
    int width  = int(pixels[0].size());

    // Lock texture to get raw pointer
    void*     texPixels;
    int       pitch; // bytes per row
    SDL_LockTexture(texture, nullptr, &texPixels, &pitch);

    // pitch should equal width * 3 for RGB24, but we use it in case
    unsigned char* dst = static_cast<unsigned char*>(texPixels);

    // Copy rows bottom-up if you want to flip vertically,
    // otherwise just iterate y from 0→height.
    for (int y = 0; y < height; ++y) {
        unsigned char* row = dst + y * pitch;
        for (int x = 0; x < width; ++x) {
            glm::vec3 c = glm::clamp(pixels[height - 1 - y][x], 0.0f, 1.0f);
            int idx = x * 3;
            row[idx + 0] = static_cast<unsigned char>(c.r * 255.0f);
            row[idx + 1] = static_cast<unsigned char>(c.g * 255.0f);
            row[idx + 2] = static_cast<unsigned char>(c.b * 255.0f);
        }
    }

    SDL_UnlockTexture(texture);

    // Render it
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, nullptr, nullptr);
    SDL_RenderPresent(renderer);
}

#endif