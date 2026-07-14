#include "arm7.hpp"

enum AluOps 
{
    And = 0b0000,
    Eor = 0b0001,
    Sub = 0b0010,
    Rsb = 0b0011,
    Add = 0b0100,
    Adc = 0b0101,
    Sbc = 0b0110,
    Rsc = 0b0111,
    Tst = 0b1000,
    Teq = 0b1001,
    Cmp = 0b1010,
    Cmn = 0b1011,
    Orr = 0b1100,
    Mov = 0b1101,
    Bic = 0b1110,
    Mvn = 0b1111
};


// 2S + 1N Incremental Cycles
void Arm7TDMI::arm_branch(uint32_t opcode)
{
    std::cout << "ARM Branch & Branch w/ Link \n";
    assert(Utils::get_bits(opcode, 25, 28) == 0b101);

    // Documentation says shift than sign extend but I don't think it makes a difference
    int32_t sign_extended_offset = Utils::sign_extend32(opcode, 0, 23) << 2;

    if (Utils::is_bit_set(opcode, 24)) // Branch with Link
        get_link() = pc - 4;
    
    // The branch offset must take account of the prefetch operation, 
    // which causes the PC to be 2 words (8 bytes) ahead of the current instruction.
    pc += sign_extended_offset + 8;
    is_branched = true;
}

// 2S + 1N Cycles
void Arm7TDMI::arm_branch_and_exchange(uint32_t opcode)
{
    std::cout << "ARM Branch And Exchange\n";

    assert(Utils::get_bits(opcode, 4, 28) == 0b0001'0010'1111'1111'1111'0001);

    // If R15 is used as an operand, the behaviour is undefined.
    uint32_t reg_index = Utils::get_bits(opcode, 0, 4);
    uint32_t address = *registers[reg_index];

    branch_and_exchange(address);
}

// Unused as GBA has no coprocessors
void Arm7TDMI::arm_coprocessor_data_operation(uint32_t opcode) 
{
    std::cout << "ARM Coprocessor Data Operation\n";

    assert(Utils::get_bits(opcode, 24, 28) == 0b1110);
    assert(!Utils::is_bit_set(opcode, 4));
}

void Arm7TDMI::arm_coprocessor_data_transfer(uint32_t opcode) 
{
    std::cout << "ARM Coprocessor Data Transfer\n";
    
    assert(Utils::get_bits(opcode, 25, 28) == 0b110);
}

void Arm7TDMI::arm_coprocessor_register_transfer(uint32_t opcode) 
{
    std::cout << "ARM Coprocessor Register Transfer\n";

    assert(Utils::get_bits(opcode, 24, 28) == 0b1110);
    assert(Utils::is_bit_set(opcode, 4));
}

void Arm7TDMI::arm_data_processing(uint32_t opcode)
{
    std::cout << "ARM Data Processing\n";

    assert(Utils::get_bits(opcode, 26, 28) == 0b00);

    int dst_reg_index = Utils::get_bits(opcode, 12, 16);
    int src_reg_index = Utils::get_bits(opcode, 16, 20);

    std::cout << "Destination Idx: " << dst_reg_index << '\n';
    std::cout << "Source Idx: " << src_reg_index << '\n';

    uint32_t& dst_register = *registers[dst_reg_index];
    uint32_t& op1_register = *registers[src_reg_index];

    std::cout << "Dest Register Value: " << dst_register << '\n';
    std::cout << "Src Register Value: " << op1_register << '\n';

    int operation = Utils::get_bits(opcode, 21, 25);
    std::cout << "Operation: " << operation << '\n';

    bool set_condition_codes = Utils::is_bit_set(opcode, 20);
    std::cout << "Set CC: " << set_condition_codes << '\n';
    bool is_immediate = Utils::is_bit_set(opcode, 25);
    std::cout << "Is Immediate: " << is_immediate << '\n';

    /*
        If R15 (the PC) is used as an operand in a data processing 
        instruction the register is used directly. 

        The PC value will be the address of the instruction, 
        plus 8 or 12 bytes due to instruction prefetching. If the shift 
        amount is specified in the instruction, the PC will be 8 bytes 
        ahead. If a register is used to specify the shift amount the 
        PC will be 12 bytes ahead

        For once Rd = PC is well-defined.
    */
    uint32_t op2{};
    if (is_immediate)
    {
        uint32_t imm8 = Utils::get_bits(opcode, 0, 8);
        std::cout << "imm8: " << imm8 << '\n';
        int shift = Utils::get_bits(opcode, 8, 12);
        std::cout << "Shift: " << shift << '\n';

        // Either the CC set after alu_ror are wrong (carry should not be set)
        // Or, the CC should not be touched for certain instructions?
        bool is_arithmetic = operation == AluOps::Sbc || operation == AluOps::Rsc || operation == AluOps::Adc;
        op2 = alu_ror(imm8, shift * 2, set_condition_codes, !is_arithmetic);
    }
    else 
    {
        uint32_t& op2_register = arm_get_rm(opcode);

        int shift_type = Utils::get_bits(opcode, 5, 7);
        std::cout << "Shift Type: " << shift_type << '\n';

        bool is_register_shift = Utils::is_bit_set(opcode, 4);
        std::cout << "Register Shift: " << is_register_shift << '\n';

        uint32_t shift_amount = (is_register_shift) ? 
            arm_get_rs(opcode) : 
            Utils::get_bits(opcode, 7, 12);
        std::cout << "Shift Amount: " << +shift_amount << '\n';

        bool is_arithmetic = operation == AluOps::Sbc || operation == AluOps::Rsc || operation == AluOps::Adc;
        switch(shift_type)
        {
            case 0: 
                op2 = alu_lsl(op2_register, shift_amount, set_condition_codes, !is_arithmetic);
                break;
            case 1: 
                op2 = alu_lsr(op2_register, shift_amount, set_condition_codes, !is_arithmetic);
                break;
            case 2: 
                op2 = alu_asr(op2_register, shift_amount, set_condition_codes, !is_arithmetic);
                break;
            case 3: 
                // RRX
                if (!shift_amount)
                {
                    uint32_t result = op2_register;
                    
                    result >>= 1;
                    result |= (c_set() << 31);
                    if (set_condition_codes)
                    {
                        set_negative_and_zero(result);
                        if (!is_arithmetic)
                            set_cpsr(ProgramStatusRegsiter::C, Utils::is_bit_set(op2_register, 0));
                    }

                    op2 = result;
                }
                else 
                    op2 = alu_ror(op2_register, shift_amount, set_condition_codes, !is_arithmetic);
                break;
            default: std::runtime_error("Invalid Shift Opcode: " + shift_type);
        }
    }

    std::cout << "Operand 2: " << op2 << '\n';

    switch(operation)
    {
    case AluOps::And: // AND
        dst_register = alu_and_tst(op1_register, op2, set_condition_codes);
        break;
    case AluOps::Eor: // EOR
        dst_register = alu_eor_teq(op1_register, op2, set_condition_codes);
        break;
    case AluOps::Sub: // SUB
        dst_register = alu_sub_cmp(op1_register, op2, set_condition_codes);
        break;
    case AluOps::Rsb: // RSB
        dst_register = alu_sub_cmp(op2, op1_register, set_condition_codes);
        break;
    case AluOps::Add: // ADD
        dst_register = alu_add_cmn(op1_register, op2, set_condition_codes);
        break;
    case AluOps::Adc: // ADC
        dst_register = alu_adc(op1_register, op2, set_condition_codes);
        break;
    case AluOps::Sbc: // SBC
        dst_register = alu_sbc(op1_register, op2, set_condition_codes);
        break;
    case AluOps::Rsc: // RSC
        dst_register = alu_sbc(op2, op1_register, set_condition_codes);
        break;
    case AluOps::Tst: // TST
        alu_and_tst(op1_register, op2, true);
        break;
    case AluOps::Teq: // TEQ
        alu_eor_teq(op1_register, op2, true);
        break;
    case AluOps::Cmp: // CMP
        alu_sub_cmp(op1_register, op2, true);
        break;
    case AluOps::Cmn: // CMN
        alu_add_cmn(op1_register, op2, true);
        break;
    case AluOps::Orr: // ORR
        dst_register = alu_orr(op1_register, op2, set_condition_codes);
        break;
    case AluOps::Mov: // MOV
        dst_register = alu_mov(op2, set_condition_codes);
        break;
    case AluOps::Bic: // BIC
        dst_register = alu_bic(op1_register, op2, set_condition_codes);
        break;
    case AluOps::Mvn: // MVN
        dst_register = alu_mov(~op2, set_condition_codes);
        break;
    default:
        std::cout << "Invalid Opcode: " << opcode << '\n';
        break;
    }

    std::cout << "Dst Register: " << dst_register << '\n';
    if (dst_reg_index == 15)
    {
        if (set_condition_codes)
        {
            std::cout << "Curr Mode: " << mode << '\n';
            std::cout << "SPSR: " << std::bitset<32>(get_mode_spsr(mode)) << '\n';
            cpsr = get_mode_spsr(mode);
        }
        if (operation < AluOps::Tst || operation > AluOps::Cmn)
        {
            pc += 8;
            is_branched = true;
        }
    }
    /*
      "CPSR": 3489661137,
      "SPSR": [
        3489661137,
        91,
        1073741840,
        268435600,
        2415919195
      ], 
    */
}

void Arm7TDMI::arm_block_data_transfer(uint32_t opcode)
{
    std::cout << "ARM Block Data Transfer\n";

    // This instruction is going to break me :sob:
    // It'd be a good idea to break this into seperate instructions
    assert(Utils::get_bits(opcode, 25, 28) == 0b100);

    int base_register_index = Utils::get_bits(opcode, 16, 20);
    uint32_t& base_register = *registers[base_register_index];

    int register_list = Utils::get_bits(opcode, 0, 16);

    bool add_offset_before_transfer = Utils::is_bit_set(opcode, 24); // P
    bool add_offset_to_base = Utils::is_bit_set(opcode, 23); // U
    bool load_psr_or_force_usr_mode = Utils::is_bit_set(opcode, 22); // S
    bool writeback_to_base = Utils::is_bit_set(opcode, 21); // W
    bool load_from_memory = Utils::is_bit_set(opcode, 20); // L

    int offset_amount = (add_offset_to_base) ? 4 : -4;
    uint32_t inital_address = base_register;

    if (load_from_memory)
    {
        for (int i = 0; i < 15; ++i)
        {
            if (!Utils::is_bit_set(register_list, i)) continue;

            if (add_offset_before_transfer) 
            { 
                inital_address += offset_amount;
                *registers[i] = memory.read32(inital_address);
            }
            else
            {
                *registers[i] = memory.read32(inital_address);
                inital_address += offset_amount;
            }
        }  

        if (Utils::is_bit_set(register_list, 15))
        {
            if (add_offset_before_transfer) 
            { 
                inital_address += offset_amount;
                pc = memory.read32(inital_address);
            }
            else
            {
                pc = memory.read32(inital_address);
                inital_address += offset_amount;
            }
        }

        if (load_psr_or_force_usr_mode)
            cpsr = get_mode_spsr(mode);
    }
    else
    {
        for (int i = 0; i < 15; ++i)
        {
            if (!Utils::is_bit_set(register_list, i)) continue;

            if (add_offset_before_transfer) 
            { 
                inital_address += offset_amount;
                memory.write32(*registers[i], inital_address);
            }
            else
            {
                memory.write32(*registers[i], inital_address);
                inital_address += offset_amount;
            }
        }  

        if (Utils::is_bit_set(register_list, 15))
        {
            if (add_offset_before_transfer) 
            { 
                inital_address += offset_amount;
                memory.write32(pc, inital_address);
            }
            else
            {
                memory.write32(pc, inital_address);
                inital_address += offset_amount;
            }
        }

        if (load_psr_or_force_usr_mode)
            cpsr = get_mode_spsr(mode);       
        
    }

    if (writeback_to_base)
        base_register = inital_address;
}

void Arm7TDMI::arm_halfword_data_transfer(uint32_t opcode)
{
    std::cout << "ARM Halfword Data Transfer\n";

    assert(Utils::get_bits(opcode, 25, 28) == 0);
    assert(Utils::get_bits(opcode, 7, 12) == 1);
    assert(Utils::is_bit_set(opcode, 4));

    bool add_before_transfer = Utils::is_bit_set(opcode, 24);
    bool add_to_offset = Utils::is_bit_set(opcode, 23);
    bool writeback_to_base = Utils::is_bit_set(opcode, 22);
    bool is_load = Utils::is_bit_set(opcode, 21);
    bool sh_flag = Utils::get_bits(opcode, 5, 7);

    assert(sh_flag != 0); // 0b00 is the SWAP instruction

    auto [base_register, dst_src_register] = arm_get_rn_rd(opcode);
    uint32_t offset_register = arm_get_rm(opcode);

    uint32_t offset_hi = Utils::get_bits(opcode, 8, 12);

    uint32_t total_offset = (offset_hi == 0) ? 
        (offset_hi << 4) | Utils::get_bits(opcode, 0, 4) : 
        arm_get_rm(opcode);
        
    int offset_amount = (add_to_offset) ? total_offset : -total_offset;
    uint32_t base_address = base_register;
    if (sh_flag == 0b01) // Unsigned Halfwords
    {
        if (is_load)
        {
            if (add_before_transfer) 
            { 
                base_address += offset_amount;
                dst_src_register = memory.read16(base_address);
            }
            else
            {
                dst_src_register = memory.read16(base_address);
                base_register += offset_amount;
            }
        }
        else
        {
            if (add_before_transfer) 
            { 
                base_address += offset_amount;
                memory.write16(dst_src_register, base_address);
            }
            else
            {
                memory.write16(dst_src_register, base_address);
                base_address += offset_amount;
            }
        }
    }
    else if (sh_flag == 0b10) // Signed Byte
    {
        if (is_load)
        {
            if (add_before_transfer) 
            { 
                base_address += offset_amount;
                dst_src_register = Utils::sign_extend32(memory.read8(base_address), 0, 15);
            }
            else
            {
                dst_src_register = Utils::sign_extend32(memory.read8(base_address), 0, 15);
                base_register += offset_amount;
            }
        }
        else
        {
            if (add_before_transfer) 
            { 
                base_address += offset_amount;
                memory.write8(dst_src_register, base_address);
            }
            else
            {
                memory.write8(dst_src_register, base_address);
                base_address += offset_amount;
            }
        }
    }
    else // Signed Halfwords
    {
        if (is_load)
        {
            if (add_before_transfer) 
            { 
                base_address += offset_amount;
                dst_src_register = Utils::sign_extend32(memory.read16(base_address), 0, 15);
            }
            else
            {
                dst_src_register = Utils::sign_extend32(memory.read16(base_address), 0, 15);
                base_register += offset_amount;
            }
        }
        else
        {
            if (add_before_transfer) 
            { 
                base_address += offset_amount;
                memory.write16(dst_src_register, base_address);
            }
            else
            {
                memory.write16(dst_src_register, base_address);
                base_address += offset_amount;
            }
        }
    }

    if (writeback_to_base)
        base_register = base_address;
}


void Arm7TDMI::arm_multiply(uint32_t opcode)
{
    std::cout << "ARM Multiply\n";

    assert(Utils::get_bits(opcode, 22, 28) == 0);
    assert(Utils::get_bits(opcode, 4, 8) == 0b1001);

    bool set_condition_codes = Utils::is_bit_set(opcode, 20);
    bool accumulate = Utils::is_bit_set(opcode, 21);
    std::cout << "Is Accumulate: " << accumulate << '\n';

    // Restrictions: Rd may not be same as Rm. Rd,Rn,Rs,Rm may not be R15.
    int op3_add_index = Utils::get_bits(opcode, 12, 16);
    int dst_reg_index = Utils::get_bits(opcode, 16, 20);

    std::cout << "Destination Idx: " << dst_reg_index << '\n';
    std::cout << "Op3 Idx: " << op3_add_index << '\n';

    uint32_t& dst_register = *registers[dst_reg_index];
    uint32_t& op3_register_add = *registers[op3_add_index];

    std::cout << "Dest Register Value: " << dst_register << '\n';
    std::cout << "Op3 Add Register Value: " << op3_register_add << '\n';

    uint32_t& op1_register_mult = arm_get_rs(opcode);
    uint32_t& op2_register_mult = arm_get_rm(opcode);

    std::cout << "Dst Register Old: " << dst_register << '\n';

    pc += 4;
    is_branched = true;

    uint32_t result = (op1_register_mult * op2_register_mult) + (accumulate * op3_register_add);
    if (dst_reg_index == 15)
        pc = result + 8;
    else 
        dst_register = result;

    
    if (set_condition_codes)
    {
        set_negative_and_zero(dst_register);
        set_cpsr(ProgramStatusRegsiter::C, dst_register < op1_register_mult);
        
        skip_mult_instr = true;
    }
}

void Arm7TDMI::arm_multiply_long(uint32_t opcode)
{
    std::cout << "ARM Multiply Long\n";

    assert(Utils::get_bits(opcode, 23, 28) == 1);
    assert(Utils::get_bits(opcode, 4, 8) == 0b1001);

    bool set_condition_codes = Utils::is_bit_set(opcode, 20);
    bool accumulate = Utils::is_bit_set(opcode, 21);
    std::cout << "Accumulate: " << accumulate << '\n';
    bool is_signed = Utils::is_bit_set(opcode, 22);
    std::cout << "Is Signed: " << is_signed << '\n';

    int dst_lo_index = Utils::get_bits(opcode, 12, 16);
    int dst_hi_index = Utils::get_bits(opcode, 16, 20);

    std::cout << "Dst Hi Idx: " << dst_hi_index << '\n';
    std::cout << "Dst Lo Idx: " << dst_lo_index << '\n';

    uint32_t& dst_register_hi = *registers[dst_hi_index];
    uint32_t& dst_register_lo = *registers[dst_lo_index];

    std::cout << "Dest Register Hi: " << dst_register_hi << '\n';
    std::cout << "Dest Register Lo: " << dst_register_lo << '\n';

    uint32_t& op1_register_mult = arm_get_rs(opcode);
    uint32_t& op2_register_mult = arm_get_rm(opcode);

    uint32_t result_lo{}, result_hi{};

    pc += 4;
    is_branched = true;

    if (is_signed)
    {
        int32_t op1_signed = static_cast<int32_t>(op1_register_mult);
        int32_t op2_signed = static_cast<int32_t>(op2_register_mult);
        std::cout << "Op1 signed: " << op1_signed << '\n';
        std::cout << "Op2 signed: " << op2_signed << '\n';

        int64_t result = static_cast<int64_t>(op1_signed) * static_cast<int64_t>(op2_signed);
        std::cout << "Result:      " << std::bitset<64>(result) << '\n';
        result_lo = (result & 0xFFFFFFFF) + (accumulate * dst_register_lo);

        bool carry = result_lo < (result & 0xFFFFFFFF);

        result_hi = (result >> 32) + (accumulate * dst_register_hi) + carry;
    }
    else
    {
        uint64_t op1_unsigned = static_cast<uint64_t>(op1_register_mult);
        uint64_t op2_unsigned = static_cast<uint64_t>(op2_register_mult);

        uint64_t result = op1_unsigned * op2_unsigned;
        result_lo = (result & 0xFFFFFFFF) + (accumulate * dst_register_lo);

        bool carry = result_lo < (result & 0xFFFFFFFF);
        std::cout << "Carry: " << carry << '\n';

        result_hi = (result >> 32) + (accumulate * dst_register_hi) + carry;
    }

    dst_register_lo = result_lo;
    dst_register_hi = result_hi;

    if (dst_hi_index == 15 || dst_lo_index == 15)
        pc += 8;

    if (set_condition_codes)
    {
        set_cpsr(ProgramStatusRegsiter::N, dst_register_hi & Utils::MSB32);
        set_cpsr(ProgramStatusRegsiter::Z, dst_register_hi == 0 && dst_register_lo == 0);
        set_cpsr(ProgramStatusRegsiter::C, dst_register_hi < op1_register_mult);
         
        skip_mult_instr = true;
    }
}

void Arm7TDMI::arm_psr_transfer(uint32_t opcode)
{
    std::cout << "ARM PSR Transfer\n";

    assert(Utils::get_bits(opcode, 26, 28) == 0b00);
    assert(Utils::get_bits(opcode, 23, 25) == 0b10);
    assert(!Utils::get_bits(opcode, 19, 21) == 0b01);
    //  1010001111
    //  001111xxxx

    bool set_to_spsr = Utils::is_bit_set(opcode, 22);
    uint32_t& psr = (set_to_spsr) ? get_mode_spsr(mode) : cpsr;

    if (Utils::get_bits(opcode, 16, 22) == 0b001111)
    {
        // MRS (transfer PSR contents to a register)
        int dst_reg_index = Utils::get_bits(opcode, 12, 16);
        uint32_t& dest_register = *registers[dst_reg_index];

        dest_register = psr;
        
        return;
    }
    else if (Utils::get_bits(opcode, 12, 22) == 0b1010011111)
    {
        // MSR (transfer register contents to PSR)
        uint32_t src_register = arm_get_rm(opcode); 
        psr = src_register;

        return;
    }
    else if (Utils::get_bits(opcode, 12, 22) == 0b1010001111)
    {
        // MSR (transfer register contents or imm val to PSR flag bits)
        bool is_immediate = Utils::is_bit_set(opcode, 25);

        uint32_t operand{};
        if (is_immediate)
        {
            uint32_t imm8 = Utils::get_bits(opcode, 0, 8);
            int rotate = Utils::get_bits(opcode, 8, 12);
            
            operand = alu_ror(imm8, rotate, false);
        }
        else
            operand = arm_get_rm(opcode);

        psr = operand << 24;

        return;
    }

    std::cout << "Invalid Opcode: " << std::bitset<32>(opcode) << '\n';
    assert(false);
}

// 2S + 1N + 1I
void Arm7TDMI::arm_single_data_swap(uint32_t opcode)
{
    std::cout << "ARM Single Data Swap\n";

    assert(Utils::get_bits(opcode, 23, 28) == 0b00010);
    assert(Utils::get_bits(opcode, 20, 22) == 0b00);
    assert(Utils::get_bits(opcode, 8, 12) == 0b0000);
    assert(Utils::get_bits(opcode, 4, 8) == 0b1001);

    bool swap_byte = Utils::is_bit_set(opcode, 22);
    std::cout << "Byte Swap: " << swap_byte << '\n';

    int dst_reg_index = Utils::get_bits(opcode, 12, 16);
    int swap_reg_index = Utils::get_bits(opcode, 16, 20);

    std::cout << "Destination Idx: " << dst_reg_index << '\n';
    std::cout << "Source Idx: " << swap_reg_index << '\n';

    uint32_t& dst_register = *registers[dst_reg_index];
    uint32_t& swap_address = *registers[swap_reg_index];
    uint32_t& src_register = arm_get_rm(opcode);

    std::cout << "Dest Register Value: " << dst_register << '\n';
    std::cout << "Swap Address Value: " << swap_address << '\n';
    std::cout << "Src Register Value: " << src_register << '\n';

    pc += 4;
    is_branched = true;

    uint32_t value{};

    if (swap_byte)
    {
        uint8_t swap_address_value = memory.read8(swap_address);    
        memory.write8(src_register, swap_address);
        value = swap_address_value;
    }
    else
    {
        uint32_t swap_address_value = memory.read32(swap_address);
        memory.write32(src_register, swap_address);
        value = (swap_address & 3) ? alu_ror(swap_address_value, (swap_address & 3) * 8, false) : swap_address_value;
    }

    dst_register = value;
    if (dst_reg_index == 15)
        pc += 8;

    std::cout << "Final Dst Value: " << dst_register << '\n';
}

void Arm7TDMI::arm_single_data_transfer(uint32_t opcode)
{
    std::cout << "ARM Single Data Transfer\n";

    assert(Utils::get_bits(opcode, 26, 28) == 0b01);

    bool is_load = Utils::is_bit_set(opcode, 20); // L
    bool writeback_to_base = Utils::is_bit_set(opcode, 21); // W
    bool is_byte = Utils::is_bit_set(opcode, 22); // B
    bool add_to_base = Utils::is_bit_set(opcode, 23); // U
    bool add_before_transfer = Utils::is_bit_set(opcode, 24); // P
    bool is_register_offset = Utils::is_bit_set(opcode, 25); // I

    auto [base_register, dst_src_register] = arm_get_rn_rd(opcode);

    uint32_t offset{};
    if (!is_register_offset)
        offset = Utils::get_bits(opcode, 0, 12);
    else
    {
        uint32_t op2_register = arm_get_rm(opcode);
        uint8_t shift_amount{};

        int shift_type = Utils::get_bits(opcode, 5, 7);
        bool is_register_shift = Utils::is_bit_set(opcode, 4);

        shift_amount = (is_register_offset) ? 
            arm_get_rs(opcode) : 
            Utils::get_bits(opcode, 7, 12);

        offset = decode_shift_operation(op2_register, shift_amount, shift_type);
    }

    int offset_amount = (add_to_base) ? -offset : offset;
    uint32_t base_address = base_register;

    if (is_byte)
    {
        if (is_load)
        {
            if (add_before_transfer) 
            { 
                base_address += offset_amount;
                dst_src_register = memory.read8(base_address);
            }
            else
            {
                dst_src_register = memory.read8(base_address);
                base_address += offset_amount;
            }
        }
        else 
        {
            if (add_before_transfer) 
            { 
                base_address += offset_amount;
                memory.write8(dst_src_register, base_address);
            }
            else
            {
                memory.write8(dst_src_register, base_address);
                base_address += offset_amount;
            }
        }
    }
    else
    {
        if (is_load)
        {
            if (add_before_transfer) 
            { 
                base_address += offset_amount;
                dst_src_register = memory.read32(base_address);
            }
            else
            {
                dst_src_register = memory.read32(base_address);
                base_address += offset_amount;
            }
        }
        else 
        {
            if (add_before_transfer) 
            { 
                base_address += offset_amount;
                memory.write32(dst_src_register, base_address);
            }
            else
            {
                memory.write32(dst_src_register, base_address);
                base_address += offset_amount;
            }
        }
    }

    if (writeback_to_base)
        base_register = base_address;
}

// 2S + 1N Cycles
void Arm7TDMI::arm_software_interrupt(uint32_t opcode)
{
    std::cout << "ARM Software Interrupt\n";

    assert(Utils::get_bits(opcode, 24, 28) == 0b1111);

    /// @note The bottom 24 bits of the instruction are ignored by the processor
    old_cpsr = cpsr;
    handle_state_switch(CpuState::Arm);
    handle_mode_switch(CpuMode::Supervisor);
    set_cpsr(ProgramStatusRegsiter::I, true);
    get_link() = pc - 4;
    pc = Arm7VectorAddr::SWI + 8;
    is_branched = true;
}

// 2S + 1I + 1N cycles
void Arm7TDMI::arm_undefined(uint32_t opcode)
{
    std::cout << "ARM Undefined\n";

    assert(Utils::get_bits(opcode, 25, 28) == 0b011);
    assert(Utils::is_bit_set(opcode, 4));

    old_cpsr = cpsr;
    handle_state_switch(CpuState::Arm);
    handle_mode_switch(CpuMode::Supervisor);
    set_cpsr(ProgramStatusRegsiter::I, true);
    get_link() = pc - 4;
    pc = Arm7VectorAddr::UNDEFINED + 8;
    is_branched = true;
}