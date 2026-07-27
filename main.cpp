#include <iostream>
#include <bitset>
#include <limits>

#include "arm7.hpp"
#include "memory.hpp"
#include "scheduler.hpp"
#include "tests.hpp"
#include "utils.hpp"

int main(int argc, char** argv)
{
    Scheduler scheduler;
    scheduler.add_event(Scheduler::EventType::HBlank, 60);
    scheduler.add_event(Scheduler::EventType::VBlank, 200);
    scheduler.add_event(Scheduler::EventType::DMA, 40);
    scheduler.add_event(Scheduler::EventType::Timer, 100);

    scheduler.print_all();

    TestMemory test_memory;
    Arm7TDMI cpu(test_memory);

    GBATests::run_test(cpu, test_memory, "thumb_bcc.json");
    return 0;
}