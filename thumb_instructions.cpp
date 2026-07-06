#include "arm7.hpp"

#include <cassert>
#include <iostream>

void Arm7TDMI::thumb_add_offset_sp(uint16_t opcode)
{
    std::cout << "THUMB Add Offset SP\n";
    /// @note The condition codes are not set by this instruction.
    assert(Utils::get_bits(opcode, 8, 16) == 0b1011'0000);

    std::cout << "SP Before: " << *registers[13] << '\n';
    uint32_t immediate9 = Utils::get_bits(opcode, 0, 7) << 2;
    bool is_sub = Utils::is_bit_set(opcode, 7);
    std::cout << "Immediate: " << immediate9 << '\n';
    std::cout << "Is Sub: " << is_sub << '\n';

    get_sp() = (is_sub) ? 
        get_sp() - immediate9 : 
        get_sp() + immediate9;
}

// Passes All Tests
void Arm7TDMI::thumb_add_subtract(uint16_t opcode)
{    
    std::cout << "THUMB Add Subtract\n";

    assert(Utils::get_bits(opcode, 11, 16) == 0b00011);

    auto [dest_register, src_register] = thumb_get_dst_src(opcode);

    int rn_or_offset3 = Utils::get_bits(opcode, 6, 9);
    bool is_sub = Utils::is_bit_set(opcode, 9);
    bool is_immediate = Utils::is_bit_set(opcode, 10);

    uint32_t operand2 = (is_immediate) ? rn_or_offset3 : *registers[rn_or_offset3];

    dest_register = (is_sub) ? 
        alu_sub_cmp(src_register, operand2, true) : 
        alu_add_cmn(src_register, operand2, true);
}

// Passes All Tests
void Arm7TDMI::thumb_alu_operations(uint16_t opcode)
{
    std::cout << "THUMB ALU Operations\n";

    assert(Utils::get_bits(opcode, 10, 16) == 0b010000);

    auto [dest_register, src_register] = thumb_get_dst_src(opcode);

    int operation = Utils::get_bits(opcode, 6, 10);
    bool set_condition_codes = Utils::is_bit_set(opcode, 20);

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
        throw std::runtime_error("ERROR (THUMB HI REG Operation): " + operation);
        break;
    }
}

void Arm7TDMI::thumb_conditional_branch(uint16_t opcode)
{
    std::cout << "THUMB Conditional Branch\n";

    assert(Utils::get_bits(opcode, 12, 16) == 0b1101);

    uint32_t cond = Utils::get_bits(opcode, 8, 12);
    int32_t signed_offset9 = Utils::sign_extend32(opcode, 0, 7) << 1;

    if (check_condition_code(cond))
        pc += signed_offset9 + 2; // This may also cause bugs.
}

void Arm7TDMI::thumb_hi_reg_op_branch_exchange(uint16_t opcode)
{
    std::cout << "THUMB HI Reg Operations/Branch Exchange\n";

    assert(Utils::get_bits(opcode, 10, 16) == 0b010'001);

    // The action of H1= 0, H2 = 0 for Op = 00 (ADD), Op =01 (CMP) and Op = 10 (MOV) is
    // undefined, and should not be used.
    bool hi_flag_2 = Utils::is_bit_set(opcode, 6);
    bool hi_flag_1 = Utils::is_bit_set(opcode, 7);
    std::cout << "Hi Flag 1: " << hi_flag_1 << '\n';
    std::cout << "Hi Flag 2: " << hi_flag_2 << '\n';

    int operation = Utils::get_bits(opcode, 8, 10);
    std::cout << "Operation: " << operation << '\n';

    int src_reg_index = Utils::get_bits(opcode, 3, 6);
    int dst_reg_index = Utils::get_bits(opcode, 0, 3);

    uint32_t& dest_register = *registers[dst_reg_index + (8 * hi_flag_1)];
    uint32_t src_register = *registers[src_reg_index + (8 * hi_flag_2)];
    std::cout << "Dst Reg Index: " << dst_reg_index + (8 * hi_flag_1) << '\n';
    std::cout << "Src Reg Index: " << hi_flag_2 << '\n';

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
    std::cout << "THUMB Load Address\n";

    assert(Utils::get_bits(opcode, 12, 16) == 0b1010);

    // The CPSR condition codes are unaffected by these instructions.
    auto& dest_register = thumb_get_dst(opcode);
    std::cout << "Destination Reg: " << dest_register << '\n';
    uint32_t immediate = Utils::get_bits(opcode, 0, 8) << 2;
    std::cout << "Immediate: " << immediate << '\n';
    bool is_stack_pointer = Utils::is_bit_set(opcode, 11);
    std::cout << "Is Stack Pointer: " << is_stack_pointer << '\n';

    std::cout << "SP: " << *registers[13] << '\n';
    std::cout << "PC: " << pc << '\n';
    if (is_stack_pointer) 
        dest_register = (get_sp() + immediate);
    else
    {
        // Where the PC is used as the source register (SP = 0), bit 1 of the PC is always read
        // as 0. The value of the PC will be 4 bytes greater than the address of the instruction
        // before bit 1 is forced to 0.
        dest_register = (pc + immediate) - 2;
    }
}

void Arm7TDMI::thumb_load_store_halfword(uint16_t opcode)
{ 
    std::cout << "THUMB Load Store Halfword\n";

    assert(Utils::get_bits(opcode, 12, 16) == 0b1000);

    auto [dst_src_register, base_register] = thumb_get_dst_src(opcode);
    
    uint32_t offset6 = Utils::get_bits(opcode, 6, 11) << 1;
    std::cout << "Offset: " << offset6 << '\n';

    bool is_load = Utils::is_bit_set(opcode, 11);
    std::cout << "Is Load: " << is_load << '\n';

    uint32_t total_offset = base_register + offset6;

    if (is_load)
    {
        dst_src_register = memory.read16(total_offset);
        dst_src_register &= 0xFFFF;
    }
    else 
        memory.write16(dst_src_register & 0xFFFF, total_offset);
}

void Arm7TDMI::thumb_load_store_immediate(uint16_t opcode)
{
    std::cout << "THUMB Load Store Immediate\n";

    assert(Utils::get_bits(opcode, 13, 16) == 0b011);

    auto [dst_src_register, base_register] = thumb_get_dst_src(opcode);

    uint32_t offset5 = Utils::get_bits(opcode, 6, 11);
    bool is_load = Utils::is_bit_set(opcode, 11);
    bool is_byte = Utils::is_bit_set(opcode, 12);

    if (is_byte) 
    {
        base_register += offset5;
        if (is_load)
            dst_src_register = memory.read8(base_register);
        else 
            memory.write8(dst_src_register, base_register);
    }   
    else 
    {
        offset5 <<= 2;     
        base_register += offset5;
        if (is_load)
            dst_src_register = memory.read32(base_register);
        else 
            memory.write32(dst_src_register, base_register);
    }
}

void Arm7TDMI::thumb_load_store_sign_extend_halfword(uint16_t opcode)
{
    std::cout << "THUMB Load Store Sign-Extended Halfword/Byte\n";
}

void Arm7TDMI::thumb_load_store_w_reg_offset(uint16_t opcode)
{
    std::cout << "THUMB Load Store w/ Register Offset\n";

    assert(Utils::get_bits(opcode, 12, 16) == 0b0101);
    assert(!Utils::is_bit_set(opcode, 9));

    auto [dst_register, base_register] = thumb_get_dst_src(opcode);

    uint32_t& offset_register = *registers[Utils::get_bits(opcode, 6, 9)];
    uint32_t word8 = Utils::get_bits(opcode, 0, 8);
    bool is_byte = Utils::is_bit_set(opcode, 10);
    bool is_load = Utils::is_bit_set(opcode, 11);

    uint32_t final_addr = base_register + offset_register;

    if (is_byte) 
    {
        if (is_load)
            dst_register = memory.read8(final_addr);
        else 
            memory.write8(dst_register, final_addr);
    }   
    else 
    {
        if (is_load)
            dst_register = memory.read32(final_addr);
        else 
            memory.write32(dst_register, final_addr);
    }
}

void Arm7TDMI::thumb_long_branch_w_link(uint16_t opcode)
{
    std::cout << "THUMB Long Branch w/ Link\n";

    assert(Utils::get_bits(opcode, 12, 16) == 0b1111);

    uint32_t offset = Utils::get_bits(offset, 0, 11);
    bool is_offset_low = Utils::is_bit_set(opcode, 11);

    if (is_offset_low)
    {
        uint32_t temp = pc - 2;
        pc = *registers[13] + (offset << 1);
        *registers[13] = temp | 1;
    }
    else
        *registers[13] = pc + (offset << 12);
}

// Passes All Tests
void Arm7TDMI::thumb_move_cmp_add_sub_immediate(uint16_t opcode)
{
    std::cout << "THUMB MOV/CMP/ADD/SUB Immediate\n";

    assert(Utils::get_bits(opcode, 13, 16) == 0b001);

    auto& dest_register = thumb_get_dst(opcode);

    int operation = Utils::get_bits(opcode, 11, 13);
    int offset = Utils::get_bits(opcode, 0, 8);

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
    std::cout << "THUMB Move Shifted Register\n";

    assert(Utils::get_bits(opcode, 13, 16) == 0b000);

    auto [dest_register, src_register] = thumb_get_dst_src(opcode);

    int operation = Utils::get_bits(opcode, 11, 13);
    uint32_t offset5 = Utils::get_bits(opcode, 6, 11);

    assert(operation != 0b11);

    dest_register = decode_shift_operation(src_register, offset5, operation);
}

void Arm7TDMI::thumb_multiple_load_store(uint16_t opcode)
{
    std::cout << "THUMB Multiple Load Store\n";

    assert(Utils::get_bits(opcode, 12, 16) == 0b1100);

    uint32_t& base_register = thumb_get_dst(opcode);

    uint32_t r_list = Utils::get_bits(opcode, 0, 8);
    bool is_load = Utils::is_bit_set(opcode, 11);

    for (int i = 0; i < 8; ++i)
    {
        int reg_index = (r_list >> i) & 1;
        if (!reg_index) continue;

        if (is_load)
            *registers[i] = memory.read32(base_register);
        else 
            memory.write32(*registers[i], base_register);

        base_register += 4;
    }
}

void Arm7TDMI::thumb_pc_relative_load(uint16_t opcode)
{
    std::cout << "THUMB PC Relative Load\n";

    assert(Utils::get_bits(opcode, 11, 16) == 0b01001);
    // Add unsigned offset (255 words,
    // 1020 bytes) in Imm to the current
    // value of the PC. Load the word
    // from the resulting address into Rd
    auto& dest_register = thumb_get_dst(opcode);

    uint32_t immediate10 = Utils::get_bits(opcode, 0, 8) << 2;

    pc += immediate10;
    dest_register = memory.read32(pc);
}

void Arm7TDMI::thumb_push_pop_registers(uint16_t opcode)
{
    std::cout << "THUMB PUSH/POP Registers\n";

    assert(Utils::get_bits(opcode, 12, 16) == 0b1011);
    assert(Utils::get_bits(opcode, 9, 11) == 0b10);

    int r_list = Utils::get_bits(opcode, 0, 8);
    std::cout << std::bitset<8>(r_list) << '\n';
    bool pc_lr_bit = Utils::is_bit_set(opcode, 8);
    std::cout << "PC/LR: " << pc_lr_bit << '\n';
    bool is_pop = Utils::is_bit_set(opcode, 11);
    std::cout << "Is POP: " << is_pop << '\n';

    std::cout << "Init SP value: " << get_sp() << '\n';

        
    if (r_list == 0 && !pc_lr_bit)
    {
        if (is_pop)
        {
            // Why does this not get word-aligned?
            pc = thumb_stack_pop() + 4;
            get_sp() += 60;
        }
        else
        {
            get_sp() -= 60;
            thumb_stack_push(pc);
        }
        
        return;
    }
    
    if (pc_lr_bit && !is_pop)
        thumb_stack_push(get_link());

    for (int i = 0; i < 8; ++i)
    {
        if (is_pop)
        {
            bool reg_index = Utils::is_bit_set(r_list, i);
            if (!reg_index) continue;
            *registers[i] = thumb_stack_pop();
        }
        else
        {
            bool reg_index = Utils::is_bit_set(r_list, 7 - i);
            if (!reg_index) continue;
            thumb_stack_push(*registers[7 - i]);
        }
    }

    if (pc_lr_bit && is_pop)
        pc = (thumb_stack_pop() & ~1) + 4;
}

/*
Final:
    4219587359, // Stays the same
    523393412, // changes
    1698507741, // stays same
    2080068945,
    731542932,
    3881155646,
    2132602419,
    3244240801,
    2341217335,
    2857604858,
    1205345846,
    105923297,
    2502113562,
    2349263592,
    3653915678,
    3895845124
*/

void Arm7TDMI::thumb_software_interrupt(uint16_t opcode)
{
    std::cout << "THUMB Software Interrupt\n";

    assert(Utils::get_bits(opcode, 8, 16) == 0b1101'1111);

    /// @note Value8 is used solely by the SWI handler: it is ignored by the processor
    handle_state_switch(CpuState::Arm);
    handle_mode_switch(CpuMode::Supervisor);
    *registers[14] = pc - 2;
    pc = Arm7VectorAddr::SWI;
}

void Arm7TDMI::thumb_sp_relative_load_store(uint16_t opcode)
{
    std::cout << "THUMB SP Relative Load/Store\n";

    assert(Utils::get_bits(opcode, 12, 16) == 0b1001);

    uint32_t& dest_register = thumb_get_dst(opcode);
    std::cout << "Destination: " << dest_register << '\n';

    uint32_t unsigned_offset10 = Utils::get_bits(opcode, 0, 8) << 2;
    std::cout << "Offset: " << unsigned_offset10 << '\n';

    bool is_load = Utils::is_bit_set(opcode, 11);
    std::cout << "Is Load: " << is_load << '\n';

    *registers[13] += unsigned_offset10;
    std::cout << "SP + Offset: " << *registers[13] << '\n';
    
    if (is_load)
        dest_register = memory.read32(*registers[13]);
    else 
        memory.write32(dest_register, *registers[13]);
}

// Passes All Tests BUT may still be buggy
void Arm7TDMI::thumb_unconditional_branch(uint16_t opcode)
{
    std::cout << "THUMB Unconditional Branch\n";

    assert(Utils::get_bits(opcode, 11, 16) == 0b11100);
    
    int32_t signed_extend12 = Utils::sign_extend32(opcode, 0, 10) << 1;

    pc += signed_extend12;
    pc += 2; // This part may cause some problems
}

void Arm7TDMI::thumb_undefined(uint16_t opcode)
{
    std::cout << "THUMB Undefined\n";

    handle_state_switch(CpuState::Arm);
    handle_mode_switch(CpuMode::Supervisor);
    *registers[14] = pc - 2;
    pc = Arm7VectorAddr::UNDEFINED;
}