#pragma once

#include "graphics_settings.hpp"

#include <array>
#include <cstdint>

#include <SDL3/SDL.h>

/// @brief SDL3 and IMGUI wrapper class for window and graphics.
class Display
{
public:
    /// @brief Initializes SDL resources
    /// @throws `std::runtime_error` If any SDL resources fail to initialize.
    Display();

    /// @brief Frees all SDL resources.
    ~Display();

    /// @brief Polls SDL events.
    void handle_events();

    /// @brief Updates screen using PPU's current frame buffer.
    void update_screen(const std::array<uint32_t, GBARes::Resolution>& frame_buffer);

    /* Getters & Setters */
    inline bool is_program_running() { return is_running; }

private:
    /* SDL resources */
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Texture* texture;
    SDL_Event event;

    bool is_running = true;
};
