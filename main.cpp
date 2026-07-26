#include <iostream>
#include <bitset>

#include "arm7.hpp"
#include "memory.hpp"
#include "memory_regions.hpp"
#include "io_register.hpp"
#include "tests.hpp"

int main(int argc, char** argv)
{
    Memory memory;

    Io16<GBAIO::TM0CNT_H> tm0_count(memory);

    tm0_count = 0b1001;
    tm0_count |= ~0b1001;
    std::cout << std::bitset<4>(memory.read16(GBAIO::TM0CNT_H)) << '\n';

    Io32<GBAIO::BG2X> bg2_x(memory);
    Io32<GBAIO::BG2Y> bg2_y(memory);
    Io32<GBAIO::BG3X> bg3_x(memory);

    bg2_x = 123400000;
    bg2_y = 56789;
    bg3_x = bg2_x + bg2_y;

    std::cout << memory.read32(GBAIO::BG3X) << '\n';

    TestMemory test_memory;
    Arm7TDMI cpu(test_memory);
    GBATests::run_all_tests(cpu, test_memory);

    return 0;
}