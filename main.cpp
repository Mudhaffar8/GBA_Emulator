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

#include <deque>
#include <iostream>

// Things to look into: passing armwrestler.gba, memory.gba

int main(int argc, char** argv)
{
    #ifdef RUN_JSON_TESTS
    
    TestMemory test_memory;
    Arm7TDMI cpu(test_memory);
    
    GBATests::run_test(cpu, test_memory, "thumb_push_pop.json");
    #else

    constexpr int MAX_LOG_TRACE = 50'000;

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

    Arm7Dissassembler disassembler(memory);
    std::deque<std::pair<uint32_t, bool>> debug_trace{};
    std::deque<std::array<uint32_t, 16>> register_states{};
    std::deque<uint32_t> cpsr_trace{};

    cpsr_trace.resize(MAX_LOG_TRACE);
    register_states.resize(MAX_LOG_TRACE);
    debug_trace.resize(MAX_LOG_TRACE);

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
                    if (debug_trace.size() == MAX_LOG_TRACE)
                        debug_trace.pop_front();
                    
                    debug_trace.emplace_back(address, cpu.is_thumb());
                } 
                cpu.tick();
                if (debug_mode)
                {
                    if (register_states.size() == MAX_LOG_TRACE)
                        register_states.pop_front();
                    
                    if (cpsr_trace.size() == MAX_LOG_TRACE)
                        cpsr_trace.pop_front();
                    
                    std::array<uint32_t, 16> registers{};
                    for (int i = 0; i < 16; ++i)
                        registers[i] = *cpu.get_registers()[i];

                    cpsr_trace.emplace_back(cpu.get_cpsr());
                    register_states.emplace_back(registers);              
                } 

                if (address == Arm7VectorAddr::RESET) // || address == 0x8000aac
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

                uint32_t time = (diff < 15) ? (15 - diff) : 0;
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

        case Scheduler::EventType::DMA0:
        case Scheduler::EventType::DMA1:
        case Scheduler::EventType::DMA2:
        case Scheduler::EventType::DMA3:
            break;
        
        default:
            break;
        }
    }

    if (debug_mode)
    {
        std::ofstream debug_file("./trace_logs/debug_trace.txt");
        
        if (debug_file.is_open())
        {
            const std::string dash = "======================\n";

            for (int i = 0; i < MAX_LOG_TRACE; ++i)
            {
                auto [addr, is_thumb] = debug_trace[i];
                std::string debug = disassembler.disassemble(addr, is_thumb) + '\n';
                debug_file.write(debug.c_str(), debug.size());

                for (int j = 0; j < 16; ++j)
                {
                    std::string reg_str = "R" + std::to_string(j) + ": ";
                    reg_str += Utils::int_to_hex(register_states[i][j]) + '\n';

                    debug_file.write(reg_str.c_str(), reg_str.size());
                }

                uint32_t cpsr = cpsr_trace[i];
                
                std::string cpsr_str{};
                cpsr_str.push_back((cpsr & Arm7TDMI::ProgramStatusRegsiter::N) ? 'N' : '-');
                cpsr_str.push_back((cpsr & Arm7TDMI::ProgramStatusRegsiter::Z) ? 'Z' : '-');
                cpsr_str.push_back((cpsr & Arm7TDMI::ProgramStatusRegsiter::C) ? 'C' : '-');
                cpsr_str.push_back((cpsr & Arm7TDMI::ProgramStatusRegsiter::V) ? 'V' : '-');
                cpsr_str.push_back((cpsr & Arm7TDMI::ProgramStatusRegsiter::F) ? 'F' : '-');
                cpsr_str.push_back((cpsr & Arm7TDMI::ProgramStatusRegsiter::I) ? 'I' : '-');
                cpsr_str.push_back((cpsr & Arm7TDMI::ProgramStatusRegsiter::T) ? 'T' : '-');
                cpsr_str += ", Mode: ";

                int cpsr_mode = cpsr & Arm7TDMI::ProgramStatusRegsiter::Mode;
                switch(cpsr_mode)
                {
                    case Arm7TDMI::ArmMode::User: cpsr_str += "User\n"; break;
                    case Arm7TDMI::ArmMode::FastInterrupt: cpsr_str += "FIQ\n"; break;
                    case Arm7TDMI::ArmMode::InterruptRequest: cpsr_str += "IRQ\n"; break;
                    case Arm7TDMI::ArmMode::Supervisor: cpsr_str += "Supervisor\n"; break;
                    case Arm7TDMI::ArmMode::Abort: cpsr_str += "Abort\n"; break;
                    case Arm7TDMI::ArmMode::System: cpsr_str += "System\n"; break;
                    default: cpsr_str += "Uknown Mode (" + std::to_string(cpsr_mode) + ")\n"; break;
                }
                debug_file.write(cpsr_str.c_str(), cpsr_str.size());
                debug_file.write(dash.c_str(), dash.size());
            }
        }

        debug_file.close();
    }

    #endif

    return 0;
}