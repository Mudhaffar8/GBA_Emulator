#include <iostream>

#include "arm7tdmi/arm7.hpp"
#include "arm7tdmi/disassembler/arm7dissassembler.hpp"
#include "display.hpp"
#include "graphics_settings.hpp"
#include "graphics.hpp"
#include "keypad.hpp"
#include "memory.hpp"
#include "memory_regions.hpp"
#include "scheduler.hpp"
#include "tests.hpp"
#include "utils.hpp"

using DebugTrace = std::vector<std::pair<uint32_t, bool>>;
using RegisterStates = std::vector<std::array<uint32_t, 16>>;

// Things to look into: passing armwrestler.gba, memory.gba
// 

int main(int argc, char** argv)
{
    #ifdef RUN_JSON_TESTS
    
    TestMemory test_memory;
    Arm7TDMI cpu(test_memory);
    
    GBATests::run_test(cpu, test_memory, "thumb_push_pop.json");
    #else

    constexpr int MAX_LOG_TRACE = 100;

    bool debug_mode = std::string(argv[3]) == "t";

    Scheduler scheduler;
    scheduler.add_event(Scheduler::EventType::HBlankEnter, GBATiming::HDRAW);
    scheduler.add_event(Scheduler::EventType::HBlankExit, GBATiming::SCANLINE);
    scheduler.add_event(Scheduler::EventType::VBlankEnter, GBATiming::VDRAW);
    scheduler.add_event(Scheduler::EventType::VBlankExit, GBATiming::REFRESH_RATE);

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

    Arm7Dissassembler debugger(memory);
    DebugTrace debug_trace{};
    RegisterStates register_states{};

    register_states.reserve(MAX_LOG_TRACE);
    debug_trace.reserve(MAX_LOG_TRACE);

    while (display.get_running_status())
    {   
        auto start = std::chrono::steady_clock::now();

        if (!memory.cpu_is_halted)
        {
            while (!scheduler.next_event_pending() && !memory.cpu_is_halted) 
            {
                uint32_t address = cpu.get_pc() - (cpu.is_thumb() ? 4 : 8);                
                // Next will be porting this to IMGUI
                if (debug_mode)
                {
                    if (debug_trace.size() == debug_trace.capacity())
                        debug_trace.erase(debug_trace.begin());
                    
                    debug_trace.emplace_back(address, cpu.is_thumb());
                } 
                cpu.tick();
                if (debug_mode)
                {
                    if (register_states.size() == register_states.capacity())
                        register_states.erase(register_states.begin());
                    
                    std::array<uint32_t, 16> registers{};
                    for (int i = 0; i < 16; ++i)
                        registers[i] = *cpu.get_registers()[i];

                    register_states.emplace_back(registers);              
                }

                if (address == Arm7VectorAddr::RESET)
                {
                    display.get_running_status() = false;
                    break;
                }
            }
        }
        else
        {
            if (memory.interrupt_pending())
                memory.cpu_is_halted = false;
        }
        
        Scheduler::Event event = scheduler.get_next_event();
        scheduler.pop_event();

        if (memory.cpu_is_halted) 
            scheduler.global_cycles = event.cycles;

        int late_cycles = scheduler.global_cycles - event.cycles;

        switch (event.event_type)
        {
        case Scheduler::EventType::VBlankEnter:
            display.handle_events();
            display.update_screen(ppu.get_frame_buffer());
            ppu.enter_vblank();
            scheduler.add_event(Scheduler::EventType::VBlankEnter, GBATiming::REFRESH_RATE - late_cycles);
            break;
    
        case Scheduler::EventType::VBlankExit:
            {
                const bool* state = SDL_GetKeyboardState(NULL);
                keypad.handle_inputs(state);

                auto end = std::chrono::steady_clock::now();
                double diff = std::chrono::duration<double, std::milli>(end - start).count();

                uint32_t time = (diff < 16.6) ? (16.6 - diff) : 0;
                SDL_Delay(time);
            }
            ppu.exit_vblank();
            scheduler.add_event(Scheduler::EventType::VBlankExit, GBATiming::REFRESH_RATE - late_cycles);
            break;

        case Scheduler::EventType::HBlankEnter:
            // Only render_scanline if NOT in VBlank
            // Should handle at call-site but handled in render_scanline for now
            ppu.render_scanline();
            ppu.enter_hblank();
            scheduler.add_event(Scheduler::EventType::HBlankEnter, GBATiming::SCANLINE - late_cycles);
            break;
        
        case Scheduler::EventType::HBlankExit:
            ppu.exit_hblank();
            scheduler.add_event(Scheduler::EventType::HBlankExit, GBATiming::SCANLINE - late_cycles);
            break;
        
        default:
            break;
        }
    }

    if (debug_mode)
    {
        for (int i = 0; i < MAX_LOG_TRACE; ++i)
        {
            auto [addr, is_thumb] = debug_trace[i];
            std::cout << debugger.disassemble(addr, is_thumb) << '\n';

            for (int j = 0; j < 16; ++j)
                std::cout << "R" << std::dec << j << ": " << Utils::int_to_hex(register_states[i][j]) << '\n';

            std::cout << "-----------------------\n";
        }
    }

    #endif

    return 0;
}