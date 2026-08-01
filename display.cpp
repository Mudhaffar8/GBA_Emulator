#include "display.hpp"

#include "graphics_settings.hpp"

#include <array>
#include <stdexcept>
#include <iostream>

Display::Display()
{
    if (!SDL_Init(SDL_INIT_VIDEO)) 
    {
        SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
        throw std::runtime_error("Failed Initializing SDL");
    }

    if (!SDL_CreateWindowAndRenderer(
        "Game Boy Advance Emulator", 
        GBARes::LCD_W, 
        GBARes::LCD_H, 
        SDL_WINDOW_RESIZABLE, 
        &window, &renderer
    )) 
    {
        SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
        throw std::runtime_error("Creating window/renderer failed");
    }

    texture = SDL_CreateTexture(
        renderer, 
        SDL_PIXELFORMAT_RGBA8888, 
        SDL_TEXTUREACCESS_STREAMING, 
        GBARes::LCD_W, 
        GBARes::LCD_H
    );

    if (!texture)
    {
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        throw std::runtime_error("Could not initialize texture");
    }
}

Display::~Display()
{
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    SDL_Quit();
}

void Display::handle_events()
{
    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
        case SDL_EVENT_QUIT:
            is_running = false;
            break;

        case SDL_EVENT_KEY_DOWN:
            switch(event.key.scancode)
            {
            case SDL_SCANCODE_ESCAPE:
                is_running = false;
                break;
            case SDL_SCANCODE_1:
                SDL_SetWindowSize(window, GBARes::LCD_W, GBARes::LCD_H);
                break;
            case SDL_SCANCODE_2:
                SDL_SetWindowSize(window, GBARes::LCD_W * 2, GBARes::LCD_H * 2);
                break;
            case SDL_SCANCODE_3:
                SDL_SetWindowSize(window, GBARes::LCD_W * 3, GBARes::LCD_H * 3);
                break;
            case SDL_SCANCODE_4:
                SDL_SetWindowSize(window, GBARes::LCD_W * 4, GBARes::LCD_H * 4);
                break;
            default:
                break;
            }

        default:
            break;
        }
    }
}

void Display::update_screen(const std::array<uint32_t, GBARes::Resolution>& frame_buffer)
{
    int pitch = 0;
    uint32_t* pixels = nullptr;

    SDL_LockTexture(texture, nullptr, (void**)(&pixels), &pitch);

    std::copy(frame_buffer.begin(), frame_buffer.end(), pixels);

    SDL_UnlockTexture(texture);

    SDL_RenderClear(renderer);
    SDL_RenderTexture(renderer, texture, nullptr, nullptr);
    SDL_RenderPresent(renderer);
}