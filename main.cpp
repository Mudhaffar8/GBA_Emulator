#include <iostream>

#include "arm7.hpp"
#include "arm7dissassembler.hpp"
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
    
    GBATests::run_test(cpu, test_memory, "arm_data_proc_immediate_shift.json");
    GBATests::run_test(cpu, test_memory, "arm_data_proc_immediate.json");
    GBATests::run_test(cpu, test_memory, "arm_data_proc_register_shift.json");
    GBATests::run_test(cpu, test_memory, "arm_ldm_stm.json");
    //GBATests::run_all_tests(cpu, test_memory);
    #else

    bool debug_mode = std::string(argv[3]) == "t";

    Scheduler scheduler;
    scheduler.add_event(Scheduler::EventType::HBlankEnter, GBATiming::HDRAW);
    //scheduler.add_event(Scheduler::EventType::HBlankExit, GBATiming::SCANLINE);
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
    Arm7Dissassembler debugger(memory);
    Graphics ppu(memory);
    Keypad keypad(memory);
    Display display;
    
    bool slowdown = false;

    while (display.is_program_running())
    {   
        auto start = std::chrono::steady_clock::now();

        // I think I've found a clue for what could be causing this issue
        // R6 should contain the constant #1 but is reset after LDMIA R13!, { R4, R5, R6, R7 } 
        // Lord of course it was with the stack pointers
        if (!memory.cpu_is_halted)
        {
            while (!scheduler.next_event_pending() && !memory.cpu_is_halted) 
            {
                // Next will be porting this to IMGUI
                if (debug_mode)
                {
                    uint32_t address = cpu.get_pc() - (cpu.is_thumb() ? 4 : 8);
                    std::string instr = debugger.disassemble(address, cpu.is_thumb());
                    if (address == 0x8) slowdown = true;
                    if (slowdown) std::cout << instr << '\n';
                    if (slowdown) SDL_Delay(100);
                }
                cpu.tick();
                if (debug_mode && slowdown) 
                {
                    std::cout << +memory.read<uint8_t>(0x04000000-8) << '\n'; 
                    for (int i = 0; i < 8; ++i)
                        std::cout << "R" << std::dec << i << ": " << std::hex << *cpu.get_registers()[i] << '\n';
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

                uint32_t time = (diff <= 15) ? (15 - diff) : 0;
                SDL_Delay(time);
            }
            ppu.exit_vblank();
            scheduler.add_event(Scheduler::EventType::VBlankExit, GBATiming::REFRESH_RATE - late_cycles);
            break;

        case Scheduler::EventType::HBlankEnter:
            // If NOT in VBlank
            // Handled in render_scanline for now
            ppu.render_scanline();
            ppu.enter_hblank();
            scheduler.add_event(Scheduler::EventType::HBlankEnter, GBATiming::SCANLINE - late_cycles);
            break;
        
        default:
            break;
        }
    }
    std::cout << "IF: " << memory.read_io16(GBAIO::IF) << '\n';
    std::cout << "IE: " << memory.read_io16(GBAIO::IE) << '\n';
    std::cout << "IME: " << memory.read_io16(GBAIO::IME) << '\n';
    #endif

    return 0;
}