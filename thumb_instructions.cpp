#include "arm7.hpp"

#include "utils.hpp"

void Arm7TDMI::thumb_add_offset_sp(uint16_t opcode)
{
    /// @note The condition codes are not set by this instruction.

    bool is_sub = Utils::is_bit_set(opcode, 7);
    uint32_t immediate9 = Utils::get_bits(opcode, 0, 7) << 2;

    *sp = (is_sub) ? *sp - immediate9 : *sp + immediate9;
}

void Arm7TDMI::thumb_add_subtract(uint16_t opcode)
{
    bool is_immediate = Utils::is_bit_set(opcode, 10);
    bool is_sub = Utils::is_bit_set(opcode, 9);

    int rn_or_offset3 = Utils::get_bits(opcode, 6, 9);
    int dst_reg_index = Utils::get_bits(opcode, 0, 3);
    int src_reg_index = Utils::get_bits(opcode, 3, 6);

    uint32_t& dest_register = *registers[dst_reg_index];
    uint32_t& src_register = *registers[src_reg_index];

    uint32_t operand2 = (is_immediate) ? rn_or_offset3 : *registers[rn_or_offset3];
    dest_register = (is_sub) ? operand2 - src_register : src_register + operand2;
}

void Arm7TDMI::thumb_alu_operations(uint16_t opcode)
{
    int operation = Utils::get_bits(opcode, 6, 10);
    int src_reg_index = Utils::get_bits(opcode, 3, 6);
    int dst_reg_index = Utils::get_bits(opcode, 0, 3);

    uint32_t& dest_register = *registers[dst_reg_index];
    uint32_t& src_register = *registers[src_reg_index];

    int result{};

    switch (operation)
    {
    case 0b0000: // AND Rd, Rs
        dest_register &= src_register;
        break;
    case 0b0001: // EOR Rd, Rs
        dest_register ^= src_register;
        break;
    case 0b0010: // LSL Rd, Rs
        dest_register <<= src_register;
        break;
    case 0b0011: // LSR Rd, Rs
        dest_register >>= src_register;
        break;
    case 0b0100: // ASR Rd, Rs
        // dest_register &= src_register;
        break;
    case 0b0101: // ADC Rd, Rs -> Rd + Rs + Carry
        dest_register += (src_register + static_cast<uint32_t>(c_set()));
        break;
    case 0b0110: // SBC Rd, Rs -> Rd - Rs - Carry
        dest_register -= (src_register - static_cast<uint32_t>(c_set()));
        break;
    case 0b0111: // ROR Rd, Rs
        break;
    case 0b1000: // TST Rd, Rs -> Set Condition codes on Rd AND Rs
        result = dest_register & src_register;
        break;
    case 0b1001: // NEG Rd, Rs
        dest_register = -src_register;
        break;
    case 0b1010: // CMP Rd, Rs -> Set condition codes on Rd - Rs
        result = dest_register - src_register;
        break;
    case 0b1011: // CMN Rd, Rs -> Set condition codes on Rd + Rs
        result = dest_register + src_register;
        break;
    case 0b1100: // ORR Rd, Rs
        dest_register |= src_register;
        break;
    case 0b1101: // MUL Rd, Rs -> Rs * Rd
        dest_register *= src_register;
        break;
    case 0b1110: // BIC Rd, Rs -> Rd AND NOT RS
        dest_register &= ~src_register;
        break;
    case 0b1111: // MVN Rd, Rs -> NOT Rs
        dest_register = ~src_register;
        break;
    default:
        break;
    }
}

void Arm7TDMI::thumb_conditional_branch(uint16_t opcode)
{
    uint32_t cond = Utils::get_bits(opcode, 8, 12);
    uint32_t signed_offset8 = Utils::sign_extend32(opcode, 7, 0);

    if (check_condition_code(cond))
        *pc += signed_offset8 - 2;
}

void thumb_hi_reg_op_branch_exchange(uint16_t opcode)
{
    
}


void Arm7TDMI::thumb_load_address(uint16_t opcode)
{
    // The CPSR condition codes are unaffected by these instructions.

    bool is_stack_pointer = Utils::is_bit_set(opcode, 11);

    int dest_reg_index = Utils::get_bits(opcode, 8, 11);
    uint32_t immediate = Utils::get_bits(opcode, 0, 8) << 2;

    if (is_stack_pointer) 
        *registers[dest_reg_index] = (*pc + immediate);
    else
    {
        // Where the PC is used as the source register (SP = 0), bit 1 of the PC is always read
        // as 0. The value of the PC will be 4 bytes greater than the address of the instruction
        // before bit 1 is forced to 0.
        *registers[dest_reg_index] = (*pc + immediate) & ~1; 
    }

}


void Arm7TDMI::thumb_move_cmp_add_sub_immediate(uint16_t opcode)
{
    int operation = Utils::get_bits(opcode, 11, 13);
    int offset = Utils::get_bits(opcode, 0, 8);

    int dest_reg_index = Utils::get_bits(opcode, 8, 11);
    uint32_t& dest_register = *registers[dest_reg_index];

    int result{};

    switch(operation)
    {
        case 0: // MOV Rd, #Offset8
            dest_register = offset; 
            break;
        case 1: // CMP Rd, #Offset8
            result = dest_register - offset;
            break;
        case 2: // ADD Rd, #Offset8
            dest_register += offset;
            break;
        case 3: // SUB Rd, #Offset8
            dest_register -= offset;
            break;
    }
}

void Arm7TDMI::thumb_move_shifted_register(uint16_t opcode)
{
    int dst_reg_index = Utils::get_bits(opcode, 0, 3);
    int src_reg_index = Utils::get_bits(opcode, 3, 6);
    int operation = Utils::get_bits(opcode, 11, 13);

    uint32_t offset5 = Utils::get_bits(opcode, 6, 11);

    /// @todo 
}

void Arm7TDMI::thumb_pc_relative_load(uint16_t opcode)
{
    // Add unsigned offset (255 words,
    // 1020 bytes) in Imm to the current
    // value of the PC. Load the word
    // from the resulting address into Rd
    uint32_t immediate10 = Utils::get_bits(opcode, 0, 8) << 2;

    int dst_reg_index = Utils::get_bits(opcode, 8, 11);

}

void Arm7TDMI::thumb_software_interrupt(uint16_t opcode)
{
    /// @note Value8 is used solely by the SWI handler: it is ignored by the processor
    handle_state_switch(CpuState::Arm);
    handle_mode_switch(CpuMode::Supervisor);
    *link = *pc - 2;
    *pc = Arm7VectorAddr::SWI;
}

void Arm7TDMI::thumb_unconditional_branch(uint16_t opcode)
{
    int32_t signed_extend11 = Utils::sign_extend32(opcode, 0, 11);
    *pc += signed_extend11 - 4;
}