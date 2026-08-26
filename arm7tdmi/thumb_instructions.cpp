#include "arm7.hpp"

#include <cassert>
#include <iostream>

// 1S cycles
// Equivalent to ADD SP, imm8
Arm7TDMI::NextPCFetch Arm7TDMI::thumb_add_offset_sp(uint16_t opcode)
{
    Utils::print("THUMB Add Offset SP\n");

    /// @note The condition codes are not set by this instruction.
    assert(Utils::get_bits(opcode, 8, 16) == 0b1011'0000);

    Utils::log("SP Before", get_sp());
    
    uint32_t immediate9 = Utils::get_bits(opcode, 0, 7) << 2;
    bool is_sub = Utils::is_bit_set(opcode, 7);

    Utils::log("Immediate", immediate9);
    Utils::log("Is Sub", is_sub);

    get_sp() = (is_sub) ? 
        get_sp() - immediate9 : 
        get_sp() + immediate9;

    return NextPCFetch::Sequential;
}

// 1S cycle
// Equivalent to ADD Rd, Rs, Rn or SUB RD, Rs, Rn
Arm7TDMI::NextPCFetch Arm7TDMI::thumb_add_subtract(uint16_t opcode)
{    
    Utils::print("THUMB Add Subtract\n");

    assert(Utils::get_bits(opcode, 11, 16) == 0b00011);

    auto [dest_register, src_register] = thumb_get_dst_src(opcode);

    int rn_or_offset3 = Utils::get_bits(opcode, 6, 9);
    bool is_sub = Utils::is_bit_set(opcode, 9);
    bool is_immediate = Utils::is_bit_set(opcode, 10);

    uint32_t operand2 = (is_immediate) ? rn_or_offset3 : *registers[rn_or_offset3];

    dest_register = (is_sub) ? 
        alu_sub_cmp(src_register, operand2, true) : 
        alu_add_cmn(src_register, operand2, true);
    
    return NextPCFetch::Sequential;
}

// MUL: 1S + ml (my assumption)
// Anything else: 1S
Arm7TDMI::NextPCFetch Arm7TDMI::thumb_alu_operations(uint16_t opcode)
{
    Utils::print("THUMB ALU Operations\n");

    assert(Utils::get_bits(opcode, 10, 16) == 0b010000);

    auto [dest_register, src_register] = thumb_get_dst_src(opcode);

    int operation = Utils::get_bits(opcode, 6, 10);    
    Utils::log("Operations", operation);

    switch (operation)
    {
    case 0b0000: // AND Rd, Rs
        dest_register = alu_and_tst(dest_register, src_register, true);
        break;
    case 0b0001: // EOR Rd, Rs
        dest_register = alu_eor_teq(dest_register, src_register, true);
        break;
    case 0b0010: // LSL Rd, Rs
        {
            uint32_t first_8_bits = Utils::get_bits(src_register, 0, 8);
            Utils::log("First 8 bits", first_8_bits);

            if (first_8_bits < 32)
                dest_register = alu_lsl(dest_register, first_8_bits, true);
            else if (first_8_bits == 32)
            {
                set_cpsr(ProgramStatusRegsiter::C, Utils::is_bit_set(dest_register, 0));
                dest_register = 0;
                set_negative_and_zero(dest_register);
            }
            else
            {
                dest_register = 0;

                set_negative_and_zero(dest_register);
                set_cpsr(ProgramStatusRegsiter::C, false);
            }
        }
        break;
    case 0b0011: // LSR Rd, Rs
        {
            uint32_t first_8_bits = Utils::get_bits(src_register, 0, 8);
            Utils::log("First 8 Bits", first_8_bits);

            bool msb_is_set = Utils::is_bit_set(dest_register, 31);

            if (first_8_bits == 0)
                set_negative_and_zero(dest_register);
            else if (first_8_bits < 32)
                dest_register = alu_lsr(dest_register, first_8_bits, true);
            else if (first_8_bits == 32)
            {
                dest_register = 0;
                set_negative_and_zero(dest_register);
                set_cpsr(ProgramStatusRegsiter::C, msb_is_set);
            }
            else
            {
                dest_register = 0;

                set_negative_and_zero(dest_register);
                set_cpsr(ProgramStatusRegsiter::C, false);
            }
        }
        break;
    case 0b0100: // ASR Rd, Rs
        {
            uint32_t first_8_bits = Utils::get_bits(src_register, 0, 8);

            if (first_8_bits == 0)
                set_negative_and_zero(dest_register);
            else if (first_8_bits < 32) 
                dest_register = alu_asr(dest_register, first_8_bits, true);
            else if (first_8_bits >= 32)
            {
                bool msb_is_set = Utils::is_bit_set(dest_register, 31);
                dest_register = (msb_is_set) ? 0xFFFFFFFF : 0;

                set_negative_and_zero(dest_register);
                set_cpsr(ProgramStatusRegsiter::C, msb_is_set);
            }
        }
        break;
    case 0b0101: // ADC Rd, Rs -> Rd + Rs + Carry
        dest_register = alu_adc(dest_register, src_register, true);
        break;
    case 0b0110: // SBC Rd, Rs -> Rd - Rs - Carry
        dest_register = alu_sbc(dest_register, src_register, true);
        break;
    case 0b0111: // ROR Rd, Rs
        {
            if (Utils::get_bits(src_register, 0, 8) == 0)
                set_negative_and_zero(dest_register);
            else if (Utils::get_bits(src_register, 0, 5) == 0)
            {
                set_negative_and_zero(dest_register);
                set_cpsr(ProgramStatusRegsiter::C, Utils::is_bit_set(dest_register, 31));
            }
            else  
                dest_register = alu_ror(dest_register, src_register & 0xFF, true);
        }
        break;
    case 0b1000: // TST Rd, Rs -> Set Condition codes on Rd AND Rs
        (void)alu_and_tst(dest_register, src_register, true);
        break;
    case 0b1001: // NEG Rd, Rs
        dest_register = alu_sub_cmp(0, src_register, true);
        break;
    case 0b1010: // CMP Rd, Rs -> Set condition codes on Rd - Rs
        (void)alu_sub_cmp(dest_register, src_register, true);
        break;
    case 0b1011: // CMN Rd, Rs -> Set condition codes on Rd + Rs
        (void)alu_add_cmn(dest_register, src_register, true);
        break;
    case 0b1100: // ORR Rd, Rs
        dest_register = alu_orr(dest_register, src_register, true);
        break;
    case 0b1101: // MUL Rd, Rs -> Rs * Rd
        memory.add_internal_cycles(get_mult_internal_cycles<MultType::MulMla>(src_register));
        dest_register = alu_mul(dest_register, src_register, true);
        break;
    case 0b1110: // BIC Rd, Rs -> Rd AND NOT RS
        dest_register = alu_bic(dest_register, src_register, true);
        break;
    case 0b1111: // MVN Rd, Rs -> NOT Rs
        dest_register = alu_mov(~src_register, true);
        break;
    default:
        throw std::runtime_error("ERROR (THUMB HI REG Operation): " + std::to_string(operation));
        break;
    }

    return NextPCFetch::Sequential;
}

// 2S + 1N cycles
Arm7TDMI::NextPCFetch Arm7TDMI::thumb_conditional_branch(uint16_t opcode)
{
    Utils::print("THUMB Conditional Branch\n");

    assert(Utils::get_bits(opcode, 12, 16) == 0b1101);

    uint32_t cond = Utils::get_bits(opcode, 8, 12);
    int32_t signed_offset9 = Utils::sign_extend32(opcode, 0, 7) << 1;

    if (check_condition_code(cond))
        reload_pipeline16(r15 + signed_offset9 + 4); 

    return NextPCFetch::Sequential;
}

// BX: 2S + 1N cycles
// Anything else: 1S
Arm7TDMI::NextPCFetch Arm7TDMI::thumb_hi_reg_op_branch_exchange(uint16_t opcode)
{
    Utils::print("THUMB HI Reg Operations/Branch Exchange\n");

    assert(Utils::get_bits(opcode, 10, 16) == 0b010'001);

    // The action of H1= 0, H2 = 0 for Op = 00 (ADD), Op =01 (CMP) and Op = 10 (MOV) is
    // undefined, and should not be used.
    bool hi_flag_2 = Utils::is_bit_set(opcode, 6);
    bool hi_flag_1 = Utils::is_bit_set(opcode, 7);

    Utils::log("Hi Flag 1", hi_flag_1);
    Utils::log("Hi Flag 2", hi_flag_2);

    int operation = Utils::get_bits(opcode, 8, 10);
    Utils::log("Operation", operation);

    int src_reg_index = Utils::get_bits(opcode, 3, 6) + (8 * hi_flag_2);
    int dst_reg_index = Utils::get_bits(opcode, 0, 3) + (8 * hi_flag_1);

    uint32_t& dest_register = *registers[dst_reg_index];
    uint32_t src_register = *registers[src_reg_index];

    Utils::log("Dst Reg Index", dst_reg_index);
    Utils::log("Src Reg Index", src_reg_index);
    Utils::log("Dst Reg", dest_register);
    Utils::log("Src Reg", src_register);
    
    // In this group only CMP (Op = 01) sets the CPSR condition codes.
    switch(operation)
    {
    case 0: // ADD RD, Hs
        dest_register = alu_add_cmn(dest_register, src_register, false);
        break;
    case 1: // CMP Rd, Rs
        (void)alu_sub_cmp(dest_register, src_register, true);
        break;
    case 2: // MOV Rd, Rs
        dest_register = alu_mov(src_register, false);
        break;
    case 3: // BX Rs
        branch_and_exchange(src_register);
        break;
    default:
        throw std::runtime_error("ERROR (THUMB HI REG Operation): " + std::to_string(operation));
        break;
    }

    if (dst_reg_index == 15 && (operation == 0 || operation == 2))
        reload_pipeline16((r15 & ~1) + 4);

    return NextPCFetch::Sequential;
}

// 1S + 1N + 1I cycles
Arm7TDMI::NextPCFetch Arm7TDMI::thumb_load_address(uint16_t opcode)
{
    Utils::print("THUMB Load Address\n");

    assert(Utils::get_bits(opcode, 12, 16) == 0b1010);

    // The CPSR condition codes are unaffected by these instructions.
    auto& dest_register = thumb_get_dst(opcode);

    uint32_t immediate = Utils::get_bits(opcode, 0, 8) << 2;

    bool is_stack_pointer = Utils::is_bit_set(opcode, 11);

    memory.add_internal_cycles();

    if (is_stack_pointer)
        dest_register = get_sp() + immediate;
    else
    {
        // Where the PC is used as the source register (SP = 0), bit 1 of the PC is always read
        // as 0. The value of the PC will be 4 bytes greater than the address of the instruction
        // before bit 1 is forced to 0.
        dest_register = (r15 & ~3) + immediate;
    }

    return NextPCFetch::Sequential;
}

// LDR: 1S + 1N + 1I
// STR: 2N
Arm7TDMI::NextPCFetch Arm7TDMI::thumb_load_store_halfword(uint16_t opcode)
{ 
    Utils::print("THUMB Load Store Halfword\n");

    assert(Utils::get_bits(opcode, 12, 16) == 0b1000);

    auto [dst_src_register, base_register] = thumb_get_dst_src(opcode);
    
    bool is_load = Utils::is_bit_set(opcode, 11);

    uint32_t offset6 = Utils::get_bits(opcode, 6, 11) << 1;
    uint32_t total_offset = base_register + offset6;

    if (is_load)
    {
        memory.add_internal_cycles();

        // LDRH Rd,[odd] -->  LDRH Rd,[odd-1] ROR 8  ;read to bit0-7 and bit24-31
        // Why doesn't NBA half-word align the address before loading it?
        uint16_t val = memory.read<uint16_t>(total_offset, AccessType::NonSequential);
        dst_src_register = (total_offset & 1) ? alu_ror(val, 8, false) : val;

        return NextPCFetch::Sequential;
    }
    else 
    {
        memory.write<uint16_t>(dst_src_register & 0xFFFF, total_offset, AccessType::NonSequential);
        return NextPCFetch::NonSequential;
    }
}

// LDR: 1S + 1N + 1I cycles
// STR: 2N cycles
Arm7TDMI::NextPCFetch Arm7TDMI::thumb_load_store_immediate(uint16_t opcode)
{
    Utils::print("THUMB Load Store Immediate\n");

    assert(Utils::get_bits(opcode, 13, 16) == 0b011);

    auto [dst_src_register, base_register] = thumb_get_dst_src(opcode);
    
    uint32_t offset5 = Utils::get_bits(opcode, 6, 11);

    bool is_load = Utils::is_bit_set(opcode, 11);
    bool is_byte = Utils::is_bit_set(opcode, 12);

    uint32_t final_offset = base_register;

    final_offset += (is_byte) ? offset5 : (offset5 << 2);
    
    if (is_load) 
    {
        memory.add_internal_cycles();

        if (is_byte)
            dst_src_register = memory.read<uint8_t>(final_offset, AccessType::NonSequential);
        else 
        {
            uint32_t val = memory.read<uint32_t>(final_offset, AccessType::NonSequential);
            dst_src_register = (final_offset & 3) ? alu_ror(val, (final_offset & 3) * 8, false) : val;
        }

        return NextPCFetch::Sequential;
    }   
    else 
    {
        if (is_byte)
            memory.write<uint8_t>(dst_src_register, final_offset, AccessType::NonSequential);
        else 
            memory.write<uint32_t>(dst_src_register, final_offset, AccessType::NonSequential);

        return NextPCFetch::NonSequential;
    }
}

// LDRSH: 1S + 1N + 1I cycles
// STRSH: 2N cycles
Arm7TDMI::NextPCFetch Arm7TDMI::thumb_load_store_sign_extend_halfword(uint16_t opcode)
{
    Utils::print("THUMB Load Store Sign-Extended Halfword/Byte\n");

    assert(Utils::get_bits(opcode, 12, 16) == 0b0101);
    assert(Utils::is_bit_set(opcode, 9));
    
    auto [dst_register, base_register] = thumb_get_dst_src(opcode);

    int offset_reg_index = Utils::get_bits(opcode, 6, 9);
    uint32_t offset_register = *registers[offset_reg_index];

    Utils::log("Offset Reg Index", offset_reg_index);
    Utils::log("Offset Register", offset_register);

    bool is_sign_extended = Utils::is_bit_set(opcode, 10);
    bool h_flag = Utils::is_bit_set(opcode, 11);

    Utils::log("Is Sign-Extended", is_sign_extended);
    Utils::log("H Flag", h_flag);

    uint32_t final_offset = base_register + offset_register;
    Utils::log("Final Offset", final_offset);

    if (is_sign_extended)
    {
        memory.add_internal_cycles();

        if (h_flag)
        {
            dst_register = (final_offset & 1) ? 
                Utils::sign_extend32(memory.read<uint8_t>(final_offset + 1, AccessType::NonSequential), 0, 7) : 
                Utils::sign_extend32(memory.read<uint16_t>(final_offset, AccessType::NonSequential), 0, 15);
        } 
        else
            dst_register = Utils::sign_extend32(memory.read<uint8_t>(final_offset, AccessType::NonSequential), 0, 7);

        return NextPCFetch::Sequential;
    }
    else 
    {
        if (h_flag)
        {
            memory.add_internal_cycles();

            uint16_t val = memory.read<uint16_t>(final_offset, AccessType::NonSequential);
            dst_register = (final_offset & 1) ? alu_ror(val, 8, false) : val;

            return NextPCFetch::Sequential;
        }
        else
        {
            memory.write<uint16_t>(dst_register, final_offset, AccessType::NonSequential);

            return NextPCFetch::NonSequential;
        }
    }
}

// LDR: 1N + 1S + 1I
// STR: 2N cycles
Arm7TDMI::NextPCFetch Arm7TDMI::thumb_load_store_w_reg_offset(uint16_t opcode)
{
    Utils::print("THUMB Load Store w/ Register Offset\n");

    assert(Utils::get_bits(opcode, 12, 16) == 0b0101);
    assert(!Utils::is_bit_set(opcode, 9));

    auto [dst_register, base_register] = thumb_get_dst_src(opcode);

    int offset_reg_index = Utils::get_bits(opcode, 6, 9);
    uint32_t offset_register = *registers[offset_reg_index];

    bool is_byte = Utils::is_bit_set(opcode, 10);
    bool is_load = Utils::is_bit_set(opcode, 11);

    uint32_t final_addr = base_register + offset_register;

    if (is_load) 
    {
        memory.add_internal_cycles();

        if (is_byte)
            dst_register = memory.read<uint8_t>(final_addr, AccessType::NonSequential);
        else 
        {
            uint32_t val = memory.read<uint32_t>(final_addr, AccessType::NonSequential);
            // Reads from forcibly aligned address “addr AND (NOT 3)”, and does then rotate the data as “ROR (addr AND 3)*8”
            dst_register = (final_addr & 3) ? alu_ror(val, (final_addr & 3) * 8, false) : val;
        }

        return NextPCFetch::Sequential;
    }   
    else 
    {
        if (is_byte)
            memory.write<uint8_t>(dst_register, final_addr, AccessType::NonSequential);
        else 
            memory.write<uint32_t>(dst_register, final_addr, AccessType::NonSequential);

        return NextPCFetch::NonSequential;
    }
}

// This instruction format does not have an equivalent ARM instruction
Arm7TDMI::NextPCFetch Arm7TDMI::thumb_long_branch_w_link(uint16_t opcode)
{
    Utils::print("THUMB Long Branch w/ Link\n");

    assert(Utils::get_bits(opcode, 12, 16) == 0b1111);

    uint32_t offset = Utils::get_bits(opcode, 0, 11);

    bool is_offset_low = Utils::is_bit_set(opcode, 11);

    if (is_offset_low)
    {
        uint32_t next_instr_addr = r15 - 2;
        uint32_t new_addr = (get_link() + (offset << 1) + 4) & ~1;

        get_link() = next_instr_addr | 1;
        reload_pipeline16(new_addr);
    }
    else
        get_link() = r15 + (Utils::sign_extend32(offset, 0, 10) << 12);

    return NextPCFetch::Sequential;
}

// 1S cycles
Arm7TDMI::NextPCFetch Arm7TDMI::thumb_move_cmp_add_sub_immediate(uint16_t opcode)
{
    Utils::print("THUMB MOV/CMP/ADD/SUB Immediate\n");

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
            (void)alu_sub_cmp(dest_register, offset, true);
            break;
        case 2: // ADD Rd, #Offset8
            dest_register = alu_add_cmn(dest_register, offset, true);
            break;
        case 3: // SUB Rd, #Offset8
            dest_register = alu_sub_cmp(dest_register, offset, true);
            break;
    }

    return NextPCFetch::Sequential;
}

// 1S + 1I cycles (my assumption)
// Equivalent to MOVS RD, RS, SHIFT #Offset5
Arm7TDMI::NextPCFetch Arm7TDMI::thumb_move_shifted_register(uint16_t opcode)
{
    Utils::print("THUMB Move Shifted Register\n");

    assert(Utils::get_bits(opcode, 13, 16) == 0b000);

    auto [dest_register, src_register] = thumb_get_dst_src(opcode);

    int operation = Utils::get_bits(opcode, 11, 13);
    uint32_t offset5 = Utils::get_bits(opcode, 6, 11);

    assert(operation != 0b11);

    memory.add_internal_cycles();

    dest_register = decode_shift_operation(src_register, offset5, operation);

    return NextPCFetch::Sequential;
}

// LDM: nS + 1N + 1I cycles
// STM: (n-1)S + 2N cycles
Arm7TDMI::NextPCFetch Arm7TDMI::thumb_multiple_load_store(uint16_t opcode)
{
    Utils::print("THUMB Multiple Load Store\n");

    assert(Utils::get_bits(opcode, 12, 16) == 0b1100);

    int base_reg_index = Utils::get_bits(opcode, 8, 11);
    // std::cout << "Base Register Index: " << base_reg_index << '\n';
    uint32_t& base_register = *registers[base_reg_index];
    uint32_t address = base_register;
    uint32_t address_to_write_base = address;

    uint32_t r_list = Utils::get_bits(opcode, 0, 8);

    bool is_load = Utils::is_bit_set(opcode, 11);

    bool first_register = true;
    bool base_is_first = true;

    if (r_list == 0)
    {
        if (is_load)
            r15 = (memory.read<uint32_t>(base_register, AccessType::NonSequential) + 2);
        else
            memory.write<uint32_t>(r15 + 2, base_register, AccessType::NonSequential);

        base_register += 64;

        return (is_load) ? NextPCFetch::Sequential : NextPCFetch::NonSequential;
    }

    /*
        If <Rn> is specified in <registers>:
        • If <Rn> is the lowest-numbered register specified in <registers>, 
        the original value of <Rn> is stored.
    */
    AccessType access_type = AccessType::NonSequential;
    for (int i = 0; i < 8; ++i)
    {
        if (is_load)
        {
            bool reg_index = Utils::is_bit_set(r_list, i);
            if (!reg_index) continue;
            *registers[i] = memory.read<uint32_t>(address, access_type);
            address += 4;

            access_type = AccessType::Sequential;
        }
        else
        {
            bool reg_index = Utils::is_bit_set(r_list, i);
            if (!reg_index) continue;
            if (i == base_reg_index)
            {
                address_to_write_base = address;
                base_is_first = first_register;
            }
            memory.write<uint32_t>(*registers[i], address, access_type);
            address += 4;

            first_register = false;

            access_type = AccessType::Sequential;
        }
    }

    bool reg_index = Utils::is_bit_set(r_list, base_reg_index);
    // Need to fix the cycle counting here
    if (!is_load && reg_index)
    {
        if (base_is_first)
            memory.write<uint32_t>(base_register, address_to_write_base, AccessType::None);
        else 
            memory.write<uint32_t>(address, address_to_write_base, AccessType::None);
    }

    if (!reg_index || !is_load)
        base_register = address; 

    // STMIA is scuffed af
    return (is_load) ? NextPCFetch::Sequential : NextPCFetch::NonSequential;
}

// 1S + 1I cycles (my assumption)
// Equivalent to LDR Rd, [PC, #imm]
Arm7TDMI::NextPCFetch Arm7TDMI::thumb_pc_relative_load(uint16_t opcode)
{
    Utils::print("THUMB PC Relative Load\n");

    assert(Utils::get_bits(opcode, 11, 16) == 0b01001);
    
    // Add unsigned offset (255 words,
    // 1020 bytes) in Imm to the current
    // value of the PC. Load the word
    // from the resulting address into Rd
    auto& dest_register = thumb_get_dst(opcode);

    uint32_t immediate10 = Utils::get_bits(opcode, 0, 8) << 2;
    uint32_t new_address = (r15 + immediate10) & ~3;

    memory.add_internal_cycles();

    dest_register = memory.read<uint32_t>(new_address, AccessType::NonSequential);

    return NextPCFetch::Sequential;
}

// LDMIA: nS + 1N + 1I cycles
// STMDB: (n-1)S + 2N cycles
Arm7TDMI::NextPCFetch Arm7TDMI::thumb_push_pop_registers(uint16_t opcode)
{
    Utils::print("THUMB PUSH/POP Registers\n");

    assert(Utils::get_bits(opcode, 12, 16) == 0b1011);
    assert(Utils::get_bits(opcode, 9, 11) == 0b10);

    int r_list = Utils::get_bits(opcode, 0, 8);

    bool pc_lr_bit = Utils::is_bit_set(opcode, 8);
    bool is_pop = Utils::is_bit_set(opcode, 11);
        
    if (r_list == 0 && !pc_lr_bit)
    {
        if (is_pop)
        {
            // Why does this not get word-aligned?
            uint32_t new_addr = thumb_stack_pop(AccessType::NonSequential);
            reload_pipeline16(new_addr + 4);
            get_sp() += 60;

            return NextPCFetch::Sequential;
        }
        else
        {
            get_sp() -= 60;
            thumb_stack_push(r15 + 2, AccessType::NonSequential);

            return NextPCFetch::NonSequential;
        }
    }
    
    AccessType access_type = AccessType::NonSequential;
    if (pc_lr_bit && !is_pop)
    {
        thumb_stack_push(get_link(), access_type);
        access_type = AccessType::Sequential;
    }

    for (int i = 0; i < 8; ++i)
    {
        if (is_pop)
        {
            bool reg_index = Utils::is_bit_set(r_list, i);
            if (!reg_index) continue;
            *registers[i] = thumb_stack_pop(access_type);

            access_type = AccessType::Sequential;
        }
        else
        {
            bool reg_index = Utils::is_bit_set(r_list, 7 - i);
            if (!reg_index) continue;
            thumb_stack_push(*registers[7 - i], access_type);

            access_type = AccessType::Sequential;
        }
    }

    if (pc_lr_bit && is_pop) 
    {
        uint32_t new_addr = (thumb_stack_pop(access_type) & ~1);
        reload_pipeline16(new_addr + 4);
    }

    return (is_pop) ? NextPCFetch::Sequential : NextPCFetch::NonSequential;
}

// 2S + 1N cycles
// This may have some problems idk
Arm7TDMI::NextPCFetch Arm7TDMI::thumb_software_interrupt(uint16_t opcode)
{
    Utils::print("THUMB Software Interrupt\n");

    assert(Utils::get_bits(opcode, 8, 16) == 0b1101'1111);

    /// @note Value8 is used solely by the SWI handler: it is ignored by the processor
    /// @note The bottom 24 bits of the instruction are ignored by the processor
    spsr_svc = cpsr;

    handle_state_switch(ArmState::Arm);
    handle_mode_switch(ArmMode::Supervisor);
    set_cpsr(ProgramStatusRegsiter::I, true);
     
    get_link() = r15 - 2;
    reload_pipeline32(Arm7VectorAddr::SWI + 8);

    return NextPCFetch::Sequential;
}

// LDR: 1S + 1N + 1I cycles
// STR: 2N cycles
Arm7TDMI::NextPCFetch Arm7TDMI::thumb_sp_relative_load_store(uint16_t opcode)
{
    Utils::print("THUMB SP Relative Load/Store\n");

    assert(Utils::get_bits(opcode, 12, 16) == 0b1001);

    uint32_t& dest_register = thumb_get_dst(opcode);

    bool is_load = Utils::is_bit_set(opcode, 11);

    uint32_t unsigned_offset10 = Utils::get_bits(opcode, 0, 8) << 2;
    uint32_t new_addr = get_sp() + unsigned_offset10;
    
    if (is_load)
    {
        memory.add_internal_cycles();
        
        // Reads from forcibly aligned address “addr AND (NOT 3)”, and does then rotate the data as “ROR (addr AND 3)*8”
        uint32_t val = memory.read<uint32_t>(new_addr, AccessType::NonSequential);
        dest_register = (new_addr & 3) ? alu_ror(val, (new_addr & 3) * 8, false) : val;
        
        return NextPCFetch::Sequential;
    }
    else 
    {
        memory.write<uint32_t>(dest_register, new_addr, AccessType::NonSequential);

        return NextPCFetch::NonSequential;
    }
}

// 2S + 1N cycles
Arm7TDMI::NextPCFetch Arm7TDMI::thumb_unconditional_branch(uint16_t opcode)
{
    Utils::print("THUMB Unconditional Branch\n");

    assert(Utils::get_bits(opcode, 11, 16) == 0b11100);
    
    int32_t signed_extend12 = Utils::sign_extend32(opcode, 0, 10) << 1;
    reload_pipeline16(r15 + signed_extend12 + 4);

    return NextPCFetch::Sequential;
}

// 2S + 1I + 1N cycles (my assumption)
Arm7TDMI::NextPCFetch Arm7TDMI::thumb_undefined(uint16_t opcode)
{
    Utils::print("THUMB Undefined\n");

    spsr_svc = cpsr;

    handle_state_switch(ArmState::Arm);
    handle_mode_switch(ArmMode::Supervisor);
    set_cpsr(ProgramStatusRegsiter::I, true);
     
    get_link() = r15 - 2;
    reload_pipeline32(Arm7VectorAddr::UNDEFINED + 8);

    return NextPCFetch::Sequential;
}