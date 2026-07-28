#include <iostream>
#include <bitset>
#include <limits>

#include "arm7.hpp"
#include "memory.hpp"
#include "memory_regions.hpp"
#include "scheduler.hpp"
#include "tests.hpp"
#include "utils.hpp"

int main(int argc, char** argv)
{
    Scheduler scheduler;
    Memory memory(scheduler);

    memory.write<uint16_t>(1234, GBAMem::EWRAM_START, AccessType::None);
    memory.write<uint32_t>(123456789, GBAMem::IWRAM_START, AccessType::None);
    
    std::cout << memory.read<uint16_t>(GBAMem::EWRAM_START, AccessType::None) << '\n';
    std::cout << memory.read<uint32_t>(GBAMem::IWRAM_START, AccessType::None) << '\n';

    return 0;
}