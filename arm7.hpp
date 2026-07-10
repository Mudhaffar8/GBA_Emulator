#pragma once

#include <array>
#include <cstdint>
#include <cassert>
#include <iostream>
#include <limits>
#include <stdexcept>

#include "memory.hpp"
#include "utils.hpp"

namespace Arm7VectorAddr
{
    const uint32_t RESET = 0x0;
    const uint32_t UNDEFINED = 0x4;
    const uint32_t SWI = 0x8;
    const uint32_t PREFETCH_ABORT = 0xC;
    const uint32_t DATA_ABORT = 0X10;
    const uint32_t RESERVED = 0x14;
    const uint32_t IRQ = 0x18;
    const uint32_t FIQ = 0x1C;

}

class Arm7TDMI 
{
public:
    Arm7TDMI(FakeMemory& memory);

    void tick();
    void test();

private:
    enum ConditionCode
    {
        EQ = 0b0000, // Equal, Z = 1
        NE = 0b0001, // Not Equal, Z = 0
        CS = 0b0010, // Unsigned higher or same, C = 1
        CC = 0b0011, // Unsigned lower, C = 0
        MI = 0b0100, // Negative, N = 1
        PL = 0b0101, // Positive or Zero, N = 0
        VS = 0b0110, // Overflow, V = 1
        VC = 0b0111, // No Overflow, V = 0
        HI = 0b1000, // Unsigned Higher, C = 1 and Z = 0
        LS = 0b1001, // Unsigned Lower or Same, C = 0 or Z = 1
        GE = 0b1010, // Unsigned Higher, N = V
        LT = 0b1011, // Less than, N <> V
        GT = 0b1100, // Greater than, Z = 0 and (N = V)
        LE = 0b1101, // Less than or Equal, Z = 1 or (N <> V)
        AL = 0b1110, // Always executes
        // 0b1111 is reserved and must not be used
    };

    enum CpuMode 
    {
        User = 0b10000,
        FastInterrupt = 0b10001,
        InterruptRequest = 0b10010,
        Supervisor = 0b10011,
        Abort = 0b10111,
        Undefined = 0b11011,
        System = 0b11111
    };

    enum CpuState
    {
        Arm = 0x00,
        Thumb = 0x20
    };

    enum ProgramStatusRegsiter
    {
        /* Condition Code Flags */
        N = (1 << 31), // Negative or less than
        Z = (1 << 30), // Zero
        C = (1 << 29), // Carry or borrow or extend, unsigned
        V = (1 << 28), // Overflow, signed
        
        /* Interrupts */
        I = (1 << 7), // IRQ Disable
        F = (1 << 6), // FIQ Disable

        T = (1 << 5), // State bit
        Mode = 0x1F  // Mode bit
    };

private:
    using ArmFunc = void (Arm7TDMI::*)(uint32_t opcode);
    using ThumbFunc = void (Arm7TDMI::*)(uint16_t opcode);

    FakeMemory& memory;

    std::array<ArmFunc, 4096> arm_instr_table = generate_arm_table();
    std::array<ThumbFunc, 256> thumb_instr_table = generate_thumb_table();

    /* Registers */
    // General Purpose Registers
    uint32_t r0{}, r1{}, r2{}, r3{}, r4{}, r5{}, r6{}, r7{}; 
    uint32_t r8{}, r9{}, r10{}, r11{}, r12{}, r13{}, r14{}, r15{}; // System and User share the same registers
    uint32_t r8_fiq{}, r9_fiq{}, r10_fiq{}, r11_fiq{}, r12_fiq{}, r13_fiq{}, r14_fiq{}; // Fast Interrupt
    uint32_t r13_svc{}, r14_svc{}; // Supervisor
    uint32_t r13_abt{}, r14_abt{}; // Abort
    uint32_t r13_irq{}, r14_irq{}; // IRQ
    uint32_t r13_und{}, r14_und{}; // Undefined

    std::array<uint32_t*, 16> registers 
    {{
        &r0, &r1, &r2, &r3, &r4, &r5, &r6, &r7,
        &r8, &r9, &r10, &r11, &r12, &r13, &r14, &r15
    }};

    uint32_t& pc = *registers[15];

    // Program Status Registers
    uint32_t cpsr{}, old_cpsr{};
    uint32_t spsr_fiq{}, spsr_svc{}, spsr_abt{}, spsr_irq{}, spsr_und{};

    CpuMode mode = CpuMode::User;
    CpuState state = CpuState::Arm;

    bool is_branched = false;
    bool skip_mult_instr = false; // Skipping mult cpsr flag on SST
private:
    inline uint32_t& get_sp() { return *registers[13]; }
    inline uint32_t& get_link() { return *registers[14]; }

    /* Instruction Table Dispatch */
    std::array<ArmFunc, 4096> generate_arm_table();
    std::array<ThumbFunc, 256> generate_thumb_table();

    void arm_execute(uint32_t opcode);
    void thumb_execute(uint16_t opcode);

    void handle_mode_switch(uint32_t new_mode);
    void handle_state_switch(CpuState new_state);

    uint32_t get_mode_spsr(CpuMode mode);

    bool check_condition_code(uint32_t code);
    constexpr void set_cpsr(ProgramStatusRegsiter bit, bool cond) { cpsr = (cond) ? (cpsr | bit) : (cpsr & ~bit); }
    inline void set_negative_and_zero(uint32_t num) 
    { 
        set_cpsr(ProgramStatusRegsiter::N, num & Utils::MSB32);
        set_cpsr(ProgramStatusRegsiter::Z, num == 0);
    }
    inline bool n_set() { return cpsr & ProgramStatusRegsiter::N; }
    inline bool z_set() { return cpsr & ProgramStatusRegsiter::Z; }
    inline bool c_set() { return cpsr & ProgramStatusRegsiter::C; }
    inline bool v_set() { return cpsr & ProgramStatusRegsiter::V; }

    /* Math & Logical Operations */
    uint32_t alu_add_cmn(uint32_t op1, uint32_t op2, bool set_cc);
    uint32_t alu_adc(uint32_t op1, uint32_t op2, bool set_cc); // add with carry
    uint32_t alu_and_tst(uint32_t op1, uint32_t op2, bool set_cc); // AND
    uint32_t alu_bic(uint32_t op1, uint32_t op2, bool set_cc); // BIC, Rd = Op1 AND NOT Op2
    uint32_t alu_eor_teq(uint32_t op1, uint32_t op2, bool set_cc); // Exclusive OR
    uint32_t alu_mov(uint32_t op2, bool set_cc); // RD:= op2
    uint32_t alu_mvn(uint32_t op2, bool set_cc);
    uint32_t alu_orr(uint32_t op1, uint32_t op2, bool set_cc);
    uint32_t alu_sbc(uint32_t op1, uint32_t op2, bool set_cc);
    uint32_t alu_sub_cmp(uint32_t op1, uint32_t op2, bool set_cc);
    uint32_t alu_lsl(uint32_t op1, uint32_t op2, bool set_cc); // Logical Shift Left
    uint32_t alu_lsr(uint32_t op1,uint32_t op2, bool set_cc); // Logical Shift Right
    uint32_t alu_asr(uint32_t op1, uint32_t op2, bool set_cc);
    uint32_t alu_ror(uint32_t op1, uint32_t op2, bool set_cc);
    uint32_t alu_mul(uint32_t op1, uint32_t op2, bool set_cc);
    uint32_t decode_shift_operation(uint32_t op1, uint32_t op2, int shift_type);

    /* Branching */
    void branch_and_exchange(uint32_t address);

    /* ARM Instructions */
    void arm_branch(uint32_t opcode); // Branch, Branch and Link
    void arm_branch_and_exchange(uint32_t opcode);
    void arm_block_data_transfer(uint32_t opcode);
    void arm_coprocessor_data_operation(uint32_t opcode);
    void arm_coprocessor_data_transfer(uint32_t opcode);
    void arm_coprocessor_register_transfer(uint32_t opcode);
    void arm_data_processing(uint32_t opcode);
    void arm_halfword_data_transfer(uint32_t opcode);
    void arm_multiply(uint32_t opcode);
    void arm_multiply_long(uint32_t opcode);
    void arm_software_interrupt(uint32_t opcode);
    void arm_single_data_swap(uint32_t opcode);
    void arm_single_data_transfer(uint32_t opcode);
    void arm_undefined(uint32_t opcode);

    /* THUMB Instructions */
    // I gotta find shorter method names
    void thumb_add_subtract(uint16_t opcode);
    void thumb_add_offset_sp(uint16_t opcode); // stack pointer
    void thumb_alu_operations(uint16_t opcode);
    void thumb_conditional_branch(uint16_t opcode);
    void thumb_hi_reg_op_branch_exchange(uint16_t opcode);
    void thumb_load_address(uint16_t opcode);
    void thumb_load_store_halfword(uint16_t opcode);
    void thumb_load_store_immediate(uint16_t opcode);
    void thumb_load_store_sign_extend_halfword(uint16_t opcode);
    void thumb_load_store_w_reg_offset(uint16_t opcode);
    void thumb_long_branch_w_link(uint16_t opcode);
    void thumb_move_cmp_add_sub_immediate(uint16_t opcode);
    void thumb_move_shifted_register(uint16_t opcode);
    void thumb_multiple_load_store(uint16_t opcode);
    void thumb_pc_relative_load(uint16_t opcode);
    void thumb_push_pop_registers(uint16_t opcode);
    void thumb_software_interrupt(uint16_t opcode);
    void thumb_sp_relative_load_store(uint16_t opcode);
    void thumb_unconditional_branch(uint16_t opcode);
    void thumb_undefined(uint16_t opcode);

    /* ARM Helper Methods */
    // Bits 12-15 for Dst, Bits 16-19 for Src
    // Normally in order Rd, Rn
    // Src for than dst usually
    inline std::pair<uint32_t&, uint32_t&> arm_get_rn_rd(uint32_t opcode) const
    {
        int dst_reg_index = Utils::get_bits(opcode, 12, 16);
        int src_reg_index = Utils::get_bits(opcode, 16, 20);

        // std::cout << "Destination Idx: " << dst_reg_index << '\n';
        // std::cout << "Source Idx: " << src_reg_index << '\n';

        uint32_t& dest_register = *registers[dst_reg_index];
        uint32_t& src_register = *registers[src_reg_index];

        // std::cout << "Dest Register Value: " << dest_register << '\n';
        // std::cout << "Src Register Value: " << src_register << '\n';

        return {src_register, dest_register};
    }
    
    // Bits 0-3
    inline uint32_t& arm_get_rm(uint32_t opcode)
    {
        int rm_index = Utils::get_bits(opcode, 0, 4);
        return *registers[rm_index];
    }

    // Bits 8-11
    inline uint32_t& arm_get_rs(uint32_t opcode)
    {
        int rm_index = Utils::get_bits(opcode, 8, 12);
        return *registers[rm_index];
    }

    /* Thumb Helper Methods */
    inline std::pair<uint32_t&, uint32_t&> thumb_get_dst_src(uint16_t opcode) const
    {
        int dst_reg_index = Utils::get_bits(opcode, 0, 3);
        int src_reg_index = Utils::get_bits(opcode, 3, 6);

        // std::cout << "Destination Idx: " << dst_reg_index << '\n';
        // std::cout << "Source Idx: " << src_reg_index << '\n';

        uint32_t& dest_register = *registers[dst_reg_index];
        uint32_t& src_register = *registers[src_reg_index];

        // std::cout << "Dest Register Value: " << dest_register << '\n';
        // std::cout << "Src Register Value: " << src_register << '\n';

        return {dest_register, src_register};
    }

    inline uint32_t& thumb_get_dst(uint16_t opcode) const
    {
        int dst_reg_index = Utils::get_bits(opcode, 8, 11);
        // std::cout << "Destination Idx: " << dst_reg_index << '\n';
        uint32_t& dest_register = *registers[dst_reg_index];
        return dest_register;
    }

    /* Stack Operations */
    inline void thumb_stack_push(uint32_t val)
    {
        get_sp() -= 4;
        memory.write32(val, get_sp());
    }

    inline uint32_t thumb_stack_pop()
    {
        uint32_t popped_value = memory.read32(get_sp());
        get_sp() += 4;
        return popped_value;
    }

    friend class GBATests;
};