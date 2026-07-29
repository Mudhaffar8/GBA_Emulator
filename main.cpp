#include <iostream>
#include <bitset>
#include <limits>

#include "arm7.hpp"
#include "display.hpp"
#include "graphics_settings.hpp"
#include "graphics.hpp"
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
    Scheduler scheduler;
    scheduler.add_event(Scheduler::EventType::HBlank, GBATiming::HDRAW);
    scheduler.add_event(Scheduler::EventType::VBlank, GBATiming::VDRAW);

    Memory memory(scheduler);
    memory.write<uint32_t>(0xE1A00000, 0, AccessType::None); // MOV r0, r0 (nop)
    memory.write<uint32_t>(0xEAFFFFFE, 4, AccessType::None); // B 0

    // Some Blue-ish Colour
    for (int i = 0; i < GBARes::Resolution * 2; i += 2)
        memory.write<uint16_t>(0x7B36, GBAMem::VRAM_START + i, AccessType::None);

    Arm7TDMI cpu(memory);
    Graphics ppu(memory);
    Display display;

    while (display.is_program_running())
    {
        while (!scheduler.next_event_pending())
            cpu.tick();
        
        Scheduler::Event event = scheduler.get_next_event();
        scheduler.pop_event();

        switch (event.event_type)
        {
        case Scheduler::EventType::VBlank:
            display.handle_events();
            display.update_screen(ppu.get_frame_buffer());
            scheduler.add_event(Scheduler::EventType::VBlank, GBATiming::VDRAW);
            break;

        case Scheduler::EventType::HBlank:
            // If NOT in VBlank
            ppu.render_scanline();
            scheduler.add_event(Scheduler::EventType::HBlank, GBATiming::HDRAW);
            break;
        
        default:
            break;
        }
    }
    #endif

    return 0;
}