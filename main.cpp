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
    // thumb_ldr_pc_rel.json, thumb_lsl_lsr_asr.json, thumb_swi.json, thumb_bx.json
    // thumb_ldrh_strh_imm_offset.json, thumb_bx.json
    // thumb_undefined_bcc (I guess), and thumb_data_proc.json (not cpsr flags for mult)
    // thumb_ldr_str_sp_rel.json, thumb_bl_suffix.json, thumb_bl_blx_prefix.json
    // GBATests::run_test(cpu, memory, "thumb_mov_cmp_add_sub.json");
    // GBATests::run_test(cpu, memory, "thumb_add_sub.json");
    // GBATests::run_test(cpu, memory, "thumb_b.json");
    // GBATests::run_test(cpu, memory, "thumb_bcc.json");
    // GBATests::run_test(cpu, memory, "thumb_add_sub_sp.json");
    // GBATests::run_test(cpu, memory, "thumb_push_pop.json");
    // GBATests::run_test(cpu, memory, "thumb_ldr_pc_rel.json");
    // GBATests::run_test(cpu, memory, "thumb_lsl_lsr_asr.json");
    // GBATests::run_test(cpu, memory, "thumb_ldm_stm.json");
    // GBATests::run_test(cpu, memory, "thumb_swi.json");
    // GBATests::run_test(cpu, memory, "thumb_undefined_bcc.json");
    // GBATests::run_test(cpu, memory, "thumb_data_proc.json");
    // GBATests::run_test(cpu, memory, "thumb_ldrh_strh_imm_offset.json");
    // GBATests::run_test(cpu, memory, "thumb_bx.json");
    // GBATests::run_test(cpu, memory, "thumb_ldr_str_sp_rel.json"); 
    // GBATests::run_test(cpu, memory, "thumb_bl_suffix.json"); 
    // GBATests::run_test(cpu, memory, "thumb_bl_blx_prefix.json"); 
    return 0;
}