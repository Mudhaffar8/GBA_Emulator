#include <iostream>
#include <bitset>

#include "arm7.hpp"
#include "memory.hpp"
#include "tests.hpp"

int main()
{
    FakeMemory memory;
    Arm7TDMI cpu(memory);

    // Passes thumb_mov_cmp_add_sub.json, thumb_add_sub.json, thumb_b.json, 
    // thumb_bcc.json, thumb_add_sub_sp.json, thumb_push_pop.json, thumb_ldm_stm.json
    // thumb_ldr_pc_rel.json, thumb_lsl_lsr_asr.json
    GBATests::run_test(cpu, memory, "thumb_mov_cmp_add_sub.json");
    GBATests::run_test(cpu, memory, "thumb_add_sub.json");
    GBATests::run_test(cpu, memory, "thumb_b.json");
    GBATests::run_test(cpu, memory, "thumb_bcc.json");
    GBATests::run_test(cpu, memory, "thumb_add_sub_sp.json");
    GBATests::run_test(cpu, memory, "thumb_push_pop.json");
    GBATests::run_test(cpu, memory, "thumb_ldr_pc_rel.json");
    GBATests::run_test(cpu, memory, "thumb_lsl_lsr_asr.json");
    GBATests::run_test(cpu, memory, "thumb_ldm_stm.json");
    return 0;
}