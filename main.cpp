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
    TestMemory test_memory;
    Arm7TDMI cpu(test_memory);

    GBATests::run_all_tests(cpu, test_memory, false, true);

    return 0;
}