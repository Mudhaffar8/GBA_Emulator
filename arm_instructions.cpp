#include "arm7.hpp"

// 2S + 1N Incremental Cycles
void Arm7TDMI::arm_branch(uint32_t opcode)
{
    // Documentation says shift than sign extend but I don't think it makes a difference
    int32_t sign_extended_offset = Utils::sign_extend32(opcode, 0, 23) << 2;
    
    if (Utils::is_bit_set(opcode, 24)) // Branch with Link
        *registers[13] = pc - 4;

    // The branch offset must take account of the prefetch operation, 
    // which causes the PC to be 2 words (8 bytes) ahead of the current instruction.
    pc += sign_extended_offset;
}

// 2S + 1N Cycles
void Arm7TDMI::arm_branch_and_exchange(uint32_t opcode)
{
    // If R15 is used as an operand, the behaviour is undefined.
    uint32_t rn = Utils::get_bits(opcode, 0, 4);
    uint32_t address = *registers[rn];

    branch_and_exchange(address);
}

void Arm7TDMI::arm_multiply(uint32_t opcode)
{
    bool set_condition_codes = Utils::is_bit_set(opcode, 20);
    bool multiply_and_accumulate = Utils::is_bit_set(opcode, 21);
    bool is_signed = Utils::is_bit_set(opcode, 22);

    int src_reg_index = Utils::get_bits(opcode, 16, 20);
    int op2_reg_index = Utils::get_bits(opcode, 16, 20);
    int dst_reg_index = Utils::get_bits(opcode, 16, 20);

    /// @todo
}

void Arm7TDMI::arm_multiply_long(uint32_t opcode)
{
    bool set_condition_codes = Utils::is_bit_set(opcode, 20);
    bool multiply_and_accumulate = Utils::is_bit_set(opcode, 21);

    int src_reg_index = Utils::get_bits(opcode, 8, 12);
    int dst_reg_lo_index = Utils::get_bits(opcode, 12, 16);
    int dst_reg_hi_index = Utils::get_bits(opcode, 16, 20);

    /// @todo
}

// 2S + 1N Cycles
void Arm7TDMI::arm_software_interrupt(uint32_t opcode)
{
    /// @note The bottom 24 bits of the instruction are ignored by the processor
    handle_state_switch(CpuState::Arm);
    handle_mode_switch(CpuMode::Supervisor);
    *registers[14] = pc - 4;
    pc = Arm7VectorAddr::SWI;
}

// 2S + 1I + 1N cycles
void Arm7TDMI::arm_undefined(uint32_t opcode)
{
    handle_state_switch(CpuState::Arm);
    handle_mode_switch(CpuMode::Supervisor);
    *registers[14] = pc - 4;
    pc = Arm7VectorAddr::UNDEFINED;
}