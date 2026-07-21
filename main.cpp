#include <iostream>
#include <bitset>

#include "arm7.hpp"
#include "memory.hpp"
#include "tests.hpp"

int main()
{
    FakeMemory memory;
    Arm7TDMI cpu(memory);

    // Everything commented out is passed
    // GBATests::run_test(cpu, memory, "arm_b_bl.json");
    // GBATests::run_test(cpu, memory, "arm_bx.json");
    // GBATests::run_test(cpu, memory, "arm_swi.json");
    // GBATests::run_test(cpu, memory, "arm_mul_mla.json");
    // GBATests::run_test(cpu, memory, "arm_mull_mlal.json");
    // GBATests::run_test(cpu, memory, "arm_swp.json");
    // GBATests::run_test(cpu, memory, "arm_data_proc_immediate.json");
    // GBATests::run_test(cpu, memory, "arm_data_proc_immediate_shift.json");
    // GBATests::run_test(cpu, memory, "arm_data_proc_register_shift.json");
    // GBATests::run_test(cpu, memory, "arm_mrs.json");
    // GBATests::run_test(cpu, memory, "arm_msr_reg.json");  
    // GBATests::run_test(cpu, memory, "arm_msr_imm.json");
    // GBATests::run_test(cpu, memory, "arm_ldr_str_immediate_offset.json");    
    // GBATests::run_test(cpu, memory, "arm_ldr_str_register_offset.json");    
    // GBATests::run_test(cpu, memory, "arm_ldrh_strh.json");    

    // Passes all THUMB Tests
    // GBATests::run_test(cpu, memory, "thumb_mov_cmp_add_sub.json");
    // GBATests::run_test(cpu, memory, "thumb_add_sub.json");
    // GBATests::run_test(cpu, memory, "thumb_b.json"); 
    // GBATests::run_test(cpu, memory, "thumb_bcc.json");
    // GBATests::run_test(cpu, memory, "thumb_add_sub_sp.json");
    // GBATests::run_test(cpu, memory, "thumb_push_pop.json");
    // GBATests::run_test(cpu, memory, "thumb_ldr_pc_rel.json");
    // GBATests::run_test(cpu, memory, "thumb_lsl_lsr_asr.json");
    // GBATests::run_test(cpu, memory, "thumb_ldm_stm.json");
    // GBATests::run_test(cpu, memory, "thumb_undefined_bcc.json");
    // GBATests::run_test(cpu, memory, "thumb_data_proc.json");
    // GBATests::run_test(cpu, memory, "thumb_ldrh_strh_imm_offset.json");
    // GBATests::run_test(cpu, memory, "thumb_bx.json");
    // GBATests::run_test(cpu, memory, "thumb_ldr_str_sp_rel.json"); 
    // GBATests::run_test(cpu, memory, "thumb_bl_suffix.json"); 
    // GBATests::run_test(cpu, memory, "thumb_bl_blx_prefix.json"); 
    // GBATests::run_test(cpu, memory, "thumb_add_sp_or_pc.json"); 
    // GBATests::run_test(cpu, memory, "thumb_ldr_str_imm_offset.json");
    // GBATests::run_test(cpu, memory, "thumb_ldrb_strb_imm_offset.json");
    // GBATests::run_test(cpu, memory, "thumb_ldr_str_reg_offset.json");
    // GBATests::run_test(cpu, memory, "thumb_ldrh_strh_reg_offset.json");
    // GBATests::run_test(cpu, memory, "thumb_ldrsb_strb_reg_offset.json");
    // GBATests::run_test(cpu, memory, "thumb_ldrsh_ldrsb_reg_offset.json");

    return 0;
}