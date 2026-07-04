#include <iostream>
#include <bitset>

#include "arm7.hpp"

int main()
{
    Memory memory;
    Arm7TDMI arm(memory);

    arm.test();

    return 0;
}