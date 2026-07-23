#include <iostream>
#include <bitset>

#include "arm7.hpp"
#include "memory.hpp"
#include "tests.hpp"

int main(int argc, char** argv)
{
    TestMemory memory;
    Arm7TDMI cpu(memory);

    GBATests::run_all_tests(cpu, memory);

    return 0;
}