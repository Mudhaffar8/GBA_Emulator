#include "arm7.hpp"

// 2S + 1N Incremental Cycles
void Arm7TDMI::arm_branch(uint32_t opcode)
{
    assert(Utils::get_bits(opcode, 25, 28) == 0b101);

    // Documentation says shift than sign extend but I don't think it makes a difference
    int32_t sign_extended_offset = Utils::sign_extend32(opcode, 0, 24) << 2;
    
    if (Utils::is_bit_set(opcode, 24)) // Branch with Link
        get_link() = pc - 2;

    // The branch offset must take account of the prefetch operation, 
    // which causes the PC to be 2 words (8 bytes) ahead of the current instruction.
    pc += (sign_extended_offset) + 4;
    is_branched = true;
}

// 2S + 1N Cycles
void Arm7TDMI::arm_branch_and_exchange(uint32_t opcode)
{
    assert(Utils::get_bits(opcode, 4, 28) == 0b0001'0010'1111'1111'1111'0001);

    // If R15 is used as an operand, the behaviour is undefined.
    uint32_t reg_index = Utils::get_bits(opcode, 0, 4);
    uint32_t address = *registers[reg_index];

    branch_and_exchange(address);
}

void Arm7TDMI::arm_data_processing(uint32_t opcode)
{
    assert(Utils::get_bits(opcode, 26, 28) == 0b00);

    auto [op1_register, dst_register] = arm_get_rn_rd(opcode);

    int operation = Utils::get_bits(opcode, 21, 25);

    bool set_condition_codes = Utils::is_bit_set(opcode, 20);
    bool is_immediate = Utils::is_bit_set(opcode, 25);

    uint32_t op2{};
    if (is_immediate)
    {
        uint32_t imm8 = Utils::get_bits(opcode, 0, 8);
        int shift = Utils::get_bits(opcode, 8, 12);

        op2 = alu_lsl(imm8, shift, set_condition_codes);
    }
    else 
    {
        uint32_t op2_register = arm_get_rm(opcode);
        uint8_t shift_amount{};

        int shift_type = Utils::get_bits(opcode, 5, 7);
        bool is_register_shift = Utils::is_bit_set(opcode, 4);
        
        if (is_register_shift)
        {
            int shift_register_index = Utils::get_bits(opcode, 8, 12);
            shift_amount = *registers[shift_register_index] & 0xFF;
        }
        else
            shift_amount = Utils::get_bits(opcode, 7, 12);

        op2_register = decode_shift_operation(op2_register, shift_amount, shift_type);
    }

    switch(operation)
    {
    case 0b0000: // AND
        dst_register = alu_and_tst(op1_register, op2, set_condition_codes);
        break;
    case 0b0001: // EOR
        dst_register = alu_eor_teq(op1_register, op2, set_condition_codes);
        break;
    case 0b0010: // SUB
        dst_register = alu_sub_cmp(op1_register, op2, set_condition_codes);
        break;
    case 0b0011: // RSB
        dst_register = alu_sub_cmp(op2, op1_register, set_condition_codes);
        break;
    case 0b0100: // ADD
        dst_register = alu_add_cmn(op1_register, op2, set_condition_codes);
        break;
    case 0b0101: // ADC
        dst_register = alu_adc(op1_register, op2, set_condition_codes);
        break;
    case 0b0110: // SBC
        dst_register = alu_sbc(op1_register, op2, set_condition_codes);
        break;
    case 0b0111: // RSC
        dst_register = alu_sbc(op2, op1_register, set_condition_codes);
        break;
    case 0b1000: // TST
        alu_and_tst(op1_register, op2, true);
        break;
    case 0b1001: // TEQ
        alu_eor_teq(op1_register, op2, true);
        break;
    case 0b1010: // CMP
        alu_sub_cmp(op1_register, op2, true);
        break;
    case 0b1011: // CMN
        alu_add_cmn(op1_register, op2, true);
        break;
    case 0b1100: // ORR
        dst_register = alu_orr(op1_register, op2, set_condition_codes);
        break;
    case 0b1101: // MOV
        dst_register = alu_mov(op2, set_condition_codes);
        break;
    case 0b1110: // BIC
        dst_register = alu_bic(op1_register, op2, set_condition_codes);
        break;
    case 0b1111: // MVN
        dst_register = alu_mov(~op2, set_condition_codes);
        break;
    default:
        std::cout << "Invalid Opcode: " << opcode << '\n';
        break;
    }
}

void Arm7TDMI::arm_multiply(uint32_t opcode)
{
    assert(Utils::get_bits(opcode, 22, 28) == 0);
    assert(Utils::get_bits(opcode, 4, 8) == 0b1001);

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
    assert(Utils::get_bits(opcode, 23, 28) == 1);
    
    bool set_condition_codes = Utils::is_bit_set(opcode, 20);
    bool multiply_and_accumulate = Utils::is_bit_set(opcode, 21);

    int src_reg_index = Utils::get_bits(opcode, 8, 12);
    int dst_reg_lo_index = Utils::get_bits(opcode, 12, 16);
    int dst_reg_hi_index = Utils::get_bits(opcode, 16, 20);

    /// @todo
}

// 2S + 1N + 1I
void Arm7TDMI::arm_single_data_swap(uint32_t opcode)
{
    assert(Utils::get_bits(opcode, 23, 28) == 0b00010);
    assert(Utils::get_bits(opcode, 20, 22) == 0b00);
    assert(Utils::get_bits(opcode, 8, 12) == 0b0000);
    assert(Utils::get_bits(opcode, 4, 8) == 0b1001);

    bool swap_byte = Utils::is_bit_set(opcode, 22);

    auto [swap_address, dst_register] = arm_get_rn_rd(opcode);
    uint32_t src_register = arm_get_rm(opcode);

    /*
        The swap address is determined by the contents of the base 
        register (Rn). The processor first reads the contents of the 
        swap address. Then it writes the contents of the source register 
        (Rm) to the swap address, and stores the old memory contents in 
        the destination register (Rd). The same register may be 
        specified as both the source and destination. 
    */
    if (swap_byte)
    {
        uint8_t swap_address_value = memory.read8(swap_address);    
        memory.write8(src_register, swap_address);
        dst_register = swap_address_value;
    }
    else
    {
        uint32_t swap_address_value = memory.read32(swap_address);    
        memory.write32(src_register, swap_address);
        dst_register = swap_address_value;
    }
}

// 2S + 1N Cycles
void Arm7TDMI::arm_software_interrupt(uint32_t opcode)
{
    assert(Utils::get_bits(opcode, 24, 28) == 0b1111);

    /// @note The bottom 24 bits of the instruction are ignored by the processor
    old_cpsr = cpsr;
    handle_state_switch(CpuState::Arm);
    handle_mode_switch(CpuMode::Supervisor);
    set_cpsr(ProgramStatusRegsiter::I, true);
    get_link() = pc - 2;
    pc = Arm7VectorAddr::SWI + 8;
    is_branched = true;
}

// 2S + 1I + 1N cycles
void Arm7TDMI::arm_undefined(uint32_t opcode)
{
    assert(Utils::get_bits(opcode, 25, 28) == 0b011);
    assert(Utils::is_bit_set(opcode, 4));

    old_cpsr = cpsr;
    handle_state_switch(CpuState::Arm);
    handle_mode_switch(CpuMode::Supervisor);
    set_cpsr(ProgramStatusRegsiter::I, true);
    get_link() = pc - 2;
    pc = Arm7VectorAddr::UNDEFINED + 8;
    is_branched = true;
}