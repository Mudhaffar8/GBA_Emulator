#include <iostream>
#include <bitset>

#include "arm7.hpp"
#include "memory.hpp"
#include "tests.hpp"

int main()
{
    FakeMemory memory;
    Arm7TDMI cpu(memory);

    // Passes thumb_mov_cmp_add_sub.json, thumb_add_sub.json, thumb_b.json, and thumb_bcc.json
    GBATests::run_test(cpu, memory, "thumb_ldr_str_sp_rel.json");
    return 0;
}