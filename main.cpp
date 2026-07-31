#include <iostream>

#include "arm7.hpp"
#include "display.hpp"
#include "graphics_settings.hpp"
#include "graphics.hpp"
#include "keypad.hpp"
#include "memory.hpp"
#include "memory_regions.hpp"
#include "scheduler.hpp"
#include "tests.hpp"
#include "utils.hpp"

int main(int argc, char** argv)
{
    #ifdef RUN_JSON_TESTS
    
    TestMemory test_memory;
    Arm7TDMI cpu(test_memory);

    GBATests::run_all_tests(cpu, test_memory);

    #else

    if (argc < 3)
    {
        std::cout << "Invalid # of Arguments\n";
        exit(1);
    }

    Scheduler scheduler;
    scheduler.add_event(Scheduler::EventType::HBlank, GBATiming::HDRAW);
    scheduler.add_event(Scheduler::EventType::VBlank, GBATiming::VDRAW);

    Memory memory(scheduler);
    
    // nvm I'm just dumb
    bool load_bios_success = memory.load_bios(std::string("./test_roms/") + argv[2]);
    bool load_rom_success = memory.load_rom(std::string("./test_roms/") + argv[1]);
    if (!load_rom_success || !load_bios_success)
    {
        std::cout << "File(s) Not Found!\n";
        return 1;
    }

    Arm7TDMI cpu(memory);
    Graphics ppu(memory);
    Keypad keypad(memory);
    Display display;

    while (display.is_program_running())
    {
        const bool* state = SDL_GetKeyboardState(NULL);
        keypad.handle_inputs(state);
    
        while (!scheduler.next_event_pending())
            cpu.tick();
        
        Scheduler::Event event = scheduler.get_next_event();
        scheduler.pop_event();

        int late_cycles = scheduler.get_global_cycles() - event.cycles;

        switch (event.event_type)
        {
        case Scheduler::EventType::VBlank:
            display.handle_events();
            display.update_screen(ppu.get_frame_buffer());
            scheduler.add_event(Scheduler::EventType::VBlank, GBATiming::REFRESH_RATE - late_cycles);
            break;

        case Scheduler::EventType::HBlank:
            // If NOT in VBlank
            ppu.render_scanline();
            scheduler.add_event(Scheduler::EventType::HBlank, GBATiming::SCANLINE - late_cycles);
            break;
        
        default:
            break;
        }
    }
    #endif

    return 0;
}