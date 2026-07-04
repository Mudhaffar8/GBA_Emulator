#include "arm7.hpp"

#include <cassert>

void Arm7TDMI::thumb_add_offset_sp(uint16_t opcode)
{
    /// @note The condition codes are not set by this instruction.

    bool is_sub = Utils::is_bit_set(opcode, 7);
    uint32_t immediate9 = Utils::get_bits(opcode, 0, 7) << 2;

    sp = (is_sub) ? sp - immediate9 : sp + immediate9;
}

void Arm7TDMI::thumb_add_subtract(uint16_t opcode)
{
    bool is_immediate = Utils::is_bit_set(opcode, 10);
    bool is_sub = Utils::is_bit_set(opcode, 9);
    int rn_or_offset3 = Utils::get_bits(opcode, 6, 9);

    auto& [dest_register, src_register] = thumb_get_dst_src(opcode);

    uint32_t operand2 = (is_immediate) ? rn_or_offset3 : *registers[rn_or_offset3];

    dest_register = (is_sub) ? 
        alu_sub_cmp(src_register, operand2, true) : 
        alu_add_cmn(src_register, operand2, true);
}

void Arm7TDMI::thumb_alu_operations(uint16_t opcode)
{
    bool set_condition_codes = Utils::is_bit_set(opcode, 20);
    int operation = Utils::get_bits(opcode, 6, 10);

    auto& [dest_register, src_register] = thumb_get_dst_src(opcode);

    switch (operation)
    {
    case 0b0000: // AND Rd, Rs
        dest_register = alu_and_tst(dest_register, src_register, true);
        break;
    case 0b0001: // EOR Rd, Rs
        dest_register = alu_eor_teq(dest_register, src_register, true);
        break;
    case 0b0010: // LSL Rd, Rs
        dest_register = alu_lsl(dest_register, src_register, true);
        break;
    case 0b0011: // LSR Rd, Rs
        dest_register = alu_lsr(dest_register, src_register, true);
        break;
    case 0b0100: // ASR Rd, Rs
        dest_register = alu_asr(dest_register, src_register, true);
        break;
    case 0b0101: // ADC Rd, Rs -> Rd + Rs + Carry
        dest_register = alu_adc(dest_register, src_register, true);
        break;
    case 0b0110: // SBC Rd, Rs -> Rd - Rs - Carry
        dest_register = alu_sbc(dest_register, src_register, true);
        break;
    case 0b0111: // ROR Rd, Rs
        dest_register = alu_ror(dest_register, src_register, true);
        break;
    case 0b1000: // TST Rd, Rs -> Set Condition codes on Rd AND Rs
        alu_and_tst(dest_register, src_register, true);
        break;
    case 0b1001: // NEG Rd, Rs
        dest_register = alu_mov(-src_register, true);
        break;
    case 0b1010: // CMP Rd, Rs -> Set condition codes on Rd - Rs
        alu_sub_cmp(dest_register, src_register, true);
        break;
    case 0b1011: // CMN Rd, Rs -> Set condition codes on Rd + Rs
        alu_add_cmn(dest_register, src_register, true);
        break;
    case 0b1100: // ORR Rd, Rs
        dest_register = alu_orr(dest_register, src_register, true);
        break;
    case 0b1101: // MUL Rd, Rs -> Rs * Rd
        dest_register = alu_mul(dest_register, src_register, true);
        break;
    case 0b1110: // BIC Rd, Rs -> Rd AND NOT RS
        dest_register = alu_bic(dest_register, src_register, true);
        break;
    case 0b1111: // MVN Rd, Rs -> NOT Rs
        dest_register = alu_mvn(dest_register, true);
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
        pc += signed_offset8 - 4;
}

void Arm7TDMI::thumb_hi_reg_op_branch_exchange(uint16_t opcode)
{
    // The action of H1= 0, H2 = 0 for Op = 00 (ADD), Op =01 (CMP) and Op = 10 (MOV) is
    // undefined, and should not be used.
    bool hi_flag_2 = Utils::is_bit_set(opcode, 6);
    bool hi_flag_1 = Utils::is_bit_set(opcode, 7);
    int operation = Utils::get_bits(opcode, 8, 10);

    int src_reg_index = Utils::get_bits(opcode, 3, 6);
    int dst_reg_index = Utils::get_bits(opcode, 0, 3);

    uint32_t& dest_register = *registers[dst_reg_index + (8 * hi_flag_1)];
    uint32_t src_register = *registers[src_reg_index + (8 * hi_flag_2)];

    // In this group only CMP (Op = 01) sets the CPSR condition codes.
    switch(operation)
    {
    case 0: // ADD RD, Hs
        dest_register = alu_add_cmn(dest_register, src_register, false);
        break;
    case 1: // CMP Rd, Rs
        alu_sub_cmp(dest_register, src_register, true);
        break;
    case 2: // MOV Rd, Rs
        dest_register = alu_mov(src_register, false);
        break;
    case 3: // BX Rs
        branch_and_exchange(src_register);
        break;
    default:
        throw std::runtime_error("ERROR (THUMB HI REG Operation): " + operation);
        break;
    }
}


void Arm7TDMI::thumb_load_address(uint16_t opcode)
{
    // The CPSR condition codes are unaffected by these instructions.

    bool is_stack_pointer = Utils::is_bit_set(opcode, 11);
    uint32_t immediate = Utils::get_bits(opcode, 0, 8) << 2;

    auto& dest_register = thumb_get_dst(opcode);

    if (is_stack_pointer) 
        dest_register = (sp + immediate);
    else
    {
        // Where the PC is used as the source register (SP = 0), bit 1 of the PC is always read
        // as 0. The value of the PC will be 4 bytes greater than the address of the instruction
        // before bit 1 is forced to 0.
        dest_register = (pc + immediate) & ~1; 
    }
}

void Arm7TDMI::thumb_load_store_halfword(uint16_t opcode)
{
    auto& [dest_register, base_register] = thumb_get_dst_src(opcode);
}

void Arm7TDMI::thumb_load_store_immediate(uint16_t opcode)
{}

void Arm7TDMI::thumb_long_branch_w_link(uint16_t opcode)
{

}

void Arm7TDMI::thumb_move_cmp_add_sub_immediate(uint16_t opcode)
{
    int operation = Utils::get_bits(opcode, 11, 13);
    int offset = Utils::get_bits(opcode, 0, 8);

    auto& dest_register = thumb_get_dst(opcode);

    switch(operation)
    {
        case 0: // MOV Rd, #Offset8
            dest_register = alu_mov(offset, true);
            break;
        case 1: // CMP Rd, #Offset8
            alu_sub_cmp(dest_register, offset, true);
            break;
        case 2: // ADD Rd, #Offset8
            dest_register = alu_add_cmn(dest_register, offset, true);
            break;
        case 3: // SUB Rd, #Offset8
            dest_register = alu_sub_cmp(dest_register, offset, true);
            break;
    }
}

void Arm7TDMI::thumb_move_shifted_register(uint16_t opcode)
{
    int operation = Utils::get_bits(opcode, 11, 13);
    uint32_t offset5 = Utils::get_bits(opcode, 6, 11);

    auto [dest_register, src_register] = thumb_get_dst_src(opcode);

    dest_register = decode_shift_operation(src_register, offset5, operation);
}

void Arm7TDMI::thumb_multiple_load_store(uint16_t opcode)
{
    bool is_load = Utils::is_bit_set(opcode, 11);

    uint32_t r_list = Utils::get_bits(opcode, 0, 8);

    int base_reg_index = Utils::get_bits(opcode, 8, 11);
    uint32_t& base_register = *registers[base_reg_index];

    for (int i = 0; i < 8; ++i)
    {
        int reg_index = (r_list >> i) & 1;
        if (!reg_index) continue;

        if (is_load)
            memory.write32(*registers[i], base_register);
        else 
            *registers[i] = memory.read32(base_register);

        base_register += 4;
    }
}

void Arm7TDMI::thumb_pc_relative_load(uint16_t opcode)
{
    // Add unsigned offset (255 words,
    // 1020 bytes) in Imm to the current
    // value of the PC. Load the word
    // from the resulting address into Rd
    uint32_t immediate10 = Utils::get_bits(opcode, 0, 8) << 2;

    auto& dest_register = thumb_get_dst(opcode);

    pc += immediate10;
    dest_register = memory.read32(pc);
}

void Arm7TDMI::thumb_software_interrupt(uint16_t opcode)
{
    /// @note Value8 is used solely by the SWI handler: it is ignored by the processor
    handle_state_switch(CpuState::Arm);
    handle_mode_switch(CpuMode::Supervisor);
    link = pc - 2;
    pc = Arm7VectorAddr::SWI;
}

void Arm7TDMI::thumb_sp_relative_load_store(uint16_t opcode)
{
    bool is_load = Utils::is_bit_set(opcode, 11);

    uint32_t unsigned_offset10 = Utils::get_bits(opcode, 0, 8) << 2;

    auto& dest_register = thumb_get_dst(opcode);

    sp += unsigned_offset10;

    if (is_load)
        memory.write32(dest_register, sp);
    else 
        dest_register = memory.read32(sp);
}

void Arm7TDMI::thumb_unconditional_branch(uint16_t opcode)
{
    int32_t signed_extend12 = Utils::sign_extend32(opcode, 0, 11) << 1;
    pc += signed_extend12;
    pc -= 4;
}