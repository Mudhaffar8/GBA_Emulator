#include "new_display.hpp"

#include "imgui/imgui_impl_sdl3.h"
#include "imgui/imgui_impl_sdlrenderer3.h"

#include <algorithm>
#include <stdexcept>

#ifdef __EMSCRIPTEN__
#include "../libs/emscripten/emscripten_mainloop_stub.h"
#endif

NewDisplay::NewDisplay(ImGuiIO& io) : io(io)
{
        // Setup SDL
    // [If using SDL_MAIN_USE_CALLBACKS: all code below until the main loop starts would likely be your SDL_AppInit() function]
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD))
    {
        printf("Error: SDL_Init(): %s\n", SDL_GetError());
        throw std::runtime_error("ERR!");
    }

    // Create window with SDL_Renderer graphics context
    float main_scale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());
    SDL_WindowFlags window_flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN | SDL_WINDOW_HIGH_PIXEL_DENSITY;
    SDL_Window* window = SDL_CreateWindow("Dear ImGui SDL3+SDL_Renderer example", (int)(1280 * main_scale), (int)(800 * main_scale), window_flags);
    if (window == nullptr)
    {
        printf("Error: SDL_CreateWindow(): %s\n", SDL_GetError());
        throw std::runtime_error("ERR!");
    }
    SDL_Renderer* renderer = SDL_CreateRenderer(window, nullptr);
    SDL_SetRenderVSync(renderer, 1);
    if (renderer == nullptr)
    {
        SDL_Log("Error: SDL_CreateRenderer(): %s\n", SDL_GetError());
        throw std::runtime_error("ERR!");
    }
    SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    SDL_ShowWindow(window);

    SDL_Texture* texture = SDL_CreateTexture(
        renderer, 
        SDL_PIXELFORMAT_RGBA8888, 
        SDL_TEXTUREACCESS_STREAMING, 
        240, 
        160
    );

    // Setup Dear ImGui context
    // IMGUI_CHECKVERSION();
    // ImGui::CreateContext();
    // io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Enable Docking

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    // Setup scaling
    ImGuiStyle& style = ImGui::GetStyle();
    style.ScaleAllSizes(main_scale);        // Bake a fixed style scale. (until we have a solution for dynamic style scaling, changing this requires resetting Style + calling this again)
    style.FontScaleDpi = main_scale;        // Set initial font scale. (in docking branch: using io.ConfigDpiScaleFonts=true automatically overrides this for every window depending on the current monitor)
    //io.ConfigDpiScaleFonts = true;        // [Experimental] Automatically overwrite style.FontScaleDpi in Begin() when Monitor DPI changes. This will scale fonts but _NOT_ scale sizes/padding for now.
    //io.ConfigDpiScaleViewports = true;    // [Experimental] Scale Dear ImGui and Platform Windows when Monitor DPI changes.

    // Setup Platform/Renderer backends
    ImGui_ImplSDL3_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer3_Init(renderer);
}

NewDisplay::~NewDisplay()
{
    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

void NewDisplay::handle_events()
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        ImGui_ImplSDL3_ProcessEvent(&event);

        switch (event.type)
        {
        case SDL_EVENT_QUIT:
            is_running = false;
            break;

        case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
            if (event.window.windowID == SDL_GetWindowID(window))
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

void NewDisplay::render()
{
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    ImGui_ImplSDLRenderer3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    {
        ImGui::Begin("CPU Registers");
            if (ImGui::BeginTable("GP Registers", 3))
            {
                for (int column = 0; column < 3; column++)
                {
                    ImGui::TableNextColumn();
                    for (int row = 0; row < 5; row++)
                    {
                        ImGui::TableSetColumnIndex(column);
                        ImGui::Text("R%d: 0x%X",  row + column * 5, 0xFF0332B8);
                    }
                }
                ImGui::EndTable();
            }
            ImGui::Text("R15: 0x%X", 0x0332B8);

            /* CPSR */
            ImGui::Text("CPSR: 0x%X", 0xC0000030);
            ImGui::SameLine();
            char n_set = (false ? 'N' : '-');
            char z_set = (true ? 'Z' : '-');
            char c_set = (false ? 'C' : '-');
            char v_set = (true ? 'V' : '-');
            char fiq_disable = (false ? 'F' : '-');
            char irq_disable = (true ? 'I' : '-');
            char thumb_mode = (false ? 'T' : '-');
            ImGui::Text("[%C%C%C%C%C%C%C]", n_set, z_set, c_set, v_set, fiq_disable, irq_disable, thumb_mode);
        ImGui::End();
    }

    {
        ImGui::Begin("Dissassembler");
            if (ImGui::Button("Run"));
            ImGui::SameLine();
            if (ImGui::Button("Step"));

            static ImGuiTableFlags flags = ImGuiTableFlags_RowBg 
                | ImGuiTableFlags_BordersInnerH
                | ImGuiTableFlags_BordersInnerV
                | ImGuiTableFlags_SizingFixedFit;
            if (ImGui::BeginTable("Disassembly", 3, flags))
            {
                ImGui::TableSetupColumn("Address");
                ImGui::TableSetupColumn("Opcode");
                ImGui::TableSetupColumn("Assembly");
                
                for (int row = 0; row < 15; row++)
                {
                    ImGui::TableNextColumn();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("%X", 0xC0032); // Address
                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("%X", 0xC00F0032); // Opcode
                    ImGui::TableSetColumnIndex(2);
                    ImGui::Text("%s", "ANDGT R0, R15, R2, LSR R0"); // Assembly Instruction
                    ImGui::TableNextRow();
                }
                ImGui::EndTable();
            }
        ImGui::End();
    }

    {
        ImGui::Begin("LCD");  
            ImVec2 available_size = ImGui::GetContentRegionAvail();
            ImGui::Image((ImTextureID)texture, available_size);
        ImGui::End();
    }

    // Rendering
    ImGui::Render();
    SDL_SetRenderScale(renderer, io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);
    SDL_SetRenderDrawColorFloat(renderer, clear_color.x, clear_color.y, clear_color.z, clear_color.w);
    SDL_RenderClear(renderer);
    ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);
    SDL_RenderPresent(renderer);
}

void NewDisplay::update_screen(const std::array<uint32_t, GBARes::Resolution>& frame_buffer)
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

void NewDisplay::render_registers(const std::array<uint32_t*, 16>& regs)
{

}

void NewDisplay::render_debugger(const std::vector<std::tuple<std::string, uint32_t, uint32_t>>& instructions)
{

}