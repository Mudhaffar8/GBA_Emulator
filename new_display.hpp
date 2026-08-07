#include "graphics_settings.hpp"

#include "imgui/imgui.h"
#include <SDL3/SDL.h>

#include <array>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

class NewDisplay
{
public:
    NewDisplay(ImGuiIO& io);
    ~NewDisplay();

    void update_screen(const std::array<uint32_t, GBARes::Resolution>& frame_buffer);

    void handle_events();

    void render();
    void render_debugger(const std::vector<std::tuple<std::string, uint32_t, uint32_t>>& instructions);
    void render_registers(const std::array<uint32_t*, 16>& regs);

    bool is_program_running() { return is_running; }
    
private:
    ImGuiIO &io;

    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Texture* texture;

    bool is_running = true;
};