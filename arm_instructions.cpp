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

        uint32_t shift_amount{}; 
        if (is_register_shift) 
        {
            // Only the least significant byte of the contents of Rs is 
            // used to determine the shift amount.
            shift_amount = arm_get_rs(opcode) & 0xFF;

            //  If a register is used to specify the shift amount the PC will be 12 bytes ahead.
            pc += 4;
            is_branched = true;
        }
        else 
            shift_amount = Utils::get_bits(opcode, 7, 12);
        std::cout << "Shift Amount: " << +shift_amount << '\n';
        std::cout << "Shift: " << shift_amount % 32 << '\n';

        bool is_arithmetic = operation == AluOps::Sbc || operation == AluOps::Rsc || operation == AluOps::Adc;
        bool update_carry_flag = set_condition_codes && !is_arithmetic;
        if (shift_amount != 0 || !is_register_shift)
        {
            switch(shift_type)
            {
            case 0: 
                if (shift_amount == 32)
                {
                    op2 = 0;
                    if (update_carry_flag)
                        set_cpsr(ProgramStatusRegsiter::C, Utils::is_bit_set(op2_register, 0));
                }
                else if (shift_amount > 32)
                {
                    op2 = 0;
                    if (update_carry_flag)
                            set_cpsr(ProgramStatusRegsiter::C, false);
                }
                else 
                    op2 = alu_lsl(op2_register, shift_amount, set_condition_codes, !is_arithmetic);
                break;
            case 1: 
                if (shift_amount == 32)
                {
                    op2 = 0;
                    if (update_carry_flag)
                        set_cpsr(ProgramStatusRegsiter::C, Utils::is_bit_set(op2_register, 31));
                }
                else if (shift_amount > 32)
                {
                    op2 = 0;
                    if (update_carry_flag)
                        set_cpsr(ProgramStatusRegsiter::C, false);
                }
                else
                    op2 = alu_lsr(op2_register, shift_amount, set_condition_codes, !is_arithmetic);
                break;
            case 2: 
                if (shift_amount >= 32)
                {
                    if (Utils::is_bit_set(op2_register, 31))
                    {
                        op2 = 0xFFFFFFFF;
                        if (update_carry_flag)
                            set_cpsr(ProgramStatusRegsiter::C, true);
                    }
                    else
                    {
                        op2 = 0;
                        if (update_carry_flag)
                            set_cpsr(ProgramStatusRegsiter::C, false);
                    }
                }
                else 
                    op2 = alu_asr(op2_register, shift_amount, set_condition_codes, !is_arithmetic);
                break;
            case 3: 
                // RRX
                if (shift_amount == 0)
                {
                    uint32_t result = op2_register;
                    
                    result >>= 1;
                    result |= (c_set() << 31);
                    if (update_carry_flag)
                        set_cpsr(ProgramStatusRegsiter::C, Utils::is_bit_set(op2_register, 0));

                    op2 = result;
                }
                else if (shift_amount > 32)
                {
                    uint32_t shift_value = (shift_amount % 32);
                    if (shift_value == 0)  
                        shift_value = 32;
                    op2 = alu_ror(op2_register, shift_value, set_condition_codes, !is_arithmetic);
                }
                else 
                    op2 = alu_ror(op2_register, shift_amount, set_condition_codes, !is_arithmetic);
                break;
                
                default: throw std::runtime_error("Invalid Shift Opcode: " + std::to_string(shift_type));
            }
        }
        else 
            op2 = op2_register;
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
            uint32_t curr_mode = get_curr_mode();
            std::cout << "SPSR: " << std::bitset<32>(get_mode_spsr(curr_mode)) << '\n';
            cpsr = get_mode_spsr(curr_mode);
        }
        if (operation < AluOps::Tst || operation > AluOps::Cmn)
        {
            pc += 8;
            is_branched = true;
        }
    }
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
            cpsr = get_mode_spsr(get_curr_mode());
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
            cpsr = get_mode_spsr(get_curr_mode());          
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
    int sh_flag = Utils::get_bits(opcode, 5, 7);

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
    bool is_signed = Utils::is_bit_set(opcode, 22);

    int dst_lo_index = Utils::get_bits(opcode, 12, 16);
    int dst_hi_index = Utils::get_bits(opcode, 16, 20);

    uint32_t& dst_register_hi = *registers[dst_hi_index];
    uint32_t& dst_register_lo = *registers[dst_lo_index];

    uint32_t& op1_register_mult = arm_get_rs(opcode);
    uint32_t& op2_register_mult = arm_get_rm(opcode);

    uint32_t result_lo{}, result_hi{};

    pc += 4;
    is_branched = true;

    if (is_signed)
    {
        int32_t op1_signed = static_cast<int32_t>(op1_register_mult);
        int32_t op2_signed = static_cast<int32_t>(op2_register_mult);

        int64_t result = static_cast<int64_t>(op1_signed) * static_cast<int64_t>(op2_signed);
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
        set_cpsr(ProgramStatusRegsiter::C, dst_register_hi < op1_register_mult); // Carry Flag is destroyed anyways
         
        skip_mult_instr = true;
    }
}

void Arm7TDMI::arm_psr_transfer(uint32_t opcode)
{
    std::cout << "ARM PSR Transfer\n";

    assert(Utils::get_bits(opcode, 26, 28) == 0b00);
    assert(Utils::get_bits(opcode, 23, 25) == 0b10);

    bool set_to_spsr = Utils::is_bit_set(opcode, 22);
    std::cout << "Set to SPSR: " << set_to_spsr << '\n';
    
    uint32_t& psr = (set_to_spsr) ? get_mode_spsr(get_curr_mode()) : cpsr;
    std::cout << "SPSR Before: " << std::bitset<32>(psr) << "\n";

    if (Utils::get_bits(opcode, 16, 22) == 0b001111)
    {
        // MRS (transfer PSR contents to a register)
        int dst_reg_index = Utils::get_bits(opcode, 12, 16);
        uint32_t& dest_register = *registers[dst_reg_index];

        dest_register = psr;

        // Strangely no Rd = 15 edge case for these tests
        return;
    }
    else 
    {
        uint32_t byte_mask = Utils::is_bit_set(opcode, 16) ? 0xFF : 0;
        byte_mask |= Utils::is_bit_set(opcode, 17) ? 0x0000FF00 : 0;
        byte_mask |= Utils::is_bit_set(opcode, 18) ? 0x00FF0000 : 0;
        byte_mask |= Utils::is_bit_set(opcode, 19) ? 0xFF000000 : 0;
        std::cout << "Byte Mask: 0x" << std::hex << byte_mask << '\n';

        if (Utils::get_bits(opcode, 4, 8) == 0)
        {
            std::cout << "User Mode: " << !is_privileged_mode() << '\n';
            std::cout << "MSR (transfer register contents to PSR)\n";
            // MSR (transfer register contents to PSR)
            uint32_t src_register = arm_get_rm(opcode); 
            std::cout << "Src Register: " << std::bitset<32>(src_register) << '\n';
            
            if (src_register & 0x0FFFFF00) // UnallocMask
                std::cout << "UNPREDICTBLE\n";

            if (set_to_spsr && !mode_has_spsr())
                return;
            
            // In non-privileged mode (user mode): only condition 
            // code bits of CPSR can be changed, control bits can’t.
            uint32_t mask = (!is_privileged_mode()) ? byte_mask & 0xFF000000 : byte_mask;
            std::cout << "Mask: " << std::bitset<16>(mask) << '\n';
            std::cout << "Result: " << std::bitset<16>(src_register & mask) << '\n';

            psr &= ~mask;
            std::cout << "PSR after mask: " << std::bitset<32>(psr) << '\n';
            psr |= (src_register & mask);
            std::cout << "PSR after src register: " << std::bitset<32>(psr) << '\n';
            
            // Why is bit 4 not set here? huh?
            // 01010000000000000001001001100111 <-- Expected
            // 01010000000000000001001001110111 <-- What I got

            /// @note should probably look into the behaviour for when
            /// the mode bits are set to an invalid mode number
        }
        else if (Utils::get_bits(opcode, 12, 22) == 0)
        {
            std::cout << "MSR (transfer register contents or imm val to PSR flag bits)\n";
            // MSR (transfer register contents or imm val to PSR flag bits)
            bool is_immediate = Utils::is_bit_set(opcode, 25);

            uint32_t operand{};
            if (is_immediate)
            {
                uint32_t imm8 = Utils::get_bits(opcode, 0, 8);
                int rotate = Utils::get_bits(opcode, 8, 12);
                
                operand = alu_ror(imm8, rotate * 2, false);
            }
            else
                operand = arm_get_rm(opcode);

            psr = (psr & ~byte_mask) | (operand & byte_mask);
        }

        if (!set_to_spsr && (byte_mask & 0xFF))
        {
            cpsr |= 0x10;
            handle_mode_switch(cpsr & ProgramStatusRegsiter::Mode);
        }

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
    std::cout << "Is Load: " << is_load << '\n';
    bool writeback_to_base = Utils::is_bit_set(opcode, 21); // W
    std::cout << "Writeback to Base: " << writeback_to_base << '\n';
    bool is_byte = Utils::is_bit_set(opcode, 22); // B
    std::cout << "Is Byte: " << is_byte << '\n';
    bool add_to_base = Utils::is_bit_set(opcode, 23); // U
    std::cout << "Add to Base: " << add_to_base << '\n';
    bool add_before_transfer = Utils::is_bit_set(opcode, 24); // P
    std::cout << "Add Before Transfer: " << add_before_transfer << '\n';
    bool is_register_offset = Utils::is_bit_set(opcode, 25); // I
    std::cout << "Is Register Offset: " << is_register_offset << '\n';

    int src_dst_index = Utils::get_bits(opcode, 12, 16);
    int base_index = Utils::get_bits(opcode, 16, 20);

    std::cout << "Src/Dst Index: " << src_dst_index << '\n';
    std::cout << "Base Index: " << base_index << '\n';

    uint32_t& dst_src_register = *registers[src_dst_index];
    uint32_t& base_register = *registers[base_index];

    std::cout << "Src/Dst Register: " << dst_src_register << '\n';
    std::cout << "Base Register: " << base_register << '\n';


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

    int offset_amount = (add_to_base) ? offset : -offset;
    std::cout << "Offset Amount: " << offset_amount << '\n';

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
            // When R15 is the source register (Rd) of a register store (STR) instruction, 
            // the sto  red value will be address of the instruction plus 12.
            pc += 4;
            is_branched = true;

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
                uint32_t val = memory.read32(base_address);
                dst_src_register = (base_address & 3) ? 
                    alu_ror(val, (base_address & 3) * 8, false) : 
                    val;
            }
            else
            {
                uint32_t val = memory.read32(base_address);
                dst_src_register = (base_address & 3) ? 
                    alu_ror(val, (base_address & 3) * 8, false) : 
                    val;
                base_address += offset_amount;
            }
        }
        else 
        {
            // When R15 is the source register (Rd) of a register store (STR) instruction, 
            // the stored value will be address of the instruction plus 12.
            pc += 4;
            is_branched = true;

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

    // Must handle src/dst index == base Index
    if (src_dst_index == 15)
    {
        pc += 8;
        is_branched = true;
    }

    if (writeback_to_base || !add_before_transfer)
    {   
        if (src_dst_index != base_index)
            base_register = base_address;

        if (base_index == 15)
        {
            pc += 8;
            is_branched = true;
        }
    }
}

// 2S + 1N Cycles
void Arm7TDMI::arm_software_interrupt(uint32_t opcode)
{
    std::cout << "ARM Software Interrupt\n";

    assert(Utils::get_bits(opcode, 24, 28) == 0b1111);

    /// @note The bottom 24 bits of the instruction are ignored by the processor
    handle_state_switch(ArmState::Arm);

    spsr_svc = cpsr;
    handle_mode_switch(ArmMode::Supervisor); 
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

    /// @note The bottom 24 bits of the instruction are ignored by the processor
    handle_state_switch(ArmState::Arm);

    spsr_svc = cpsr;
    handle_mode_switch(ArmMode::Supervisor);
    set_cpsr(ProgramStatusRegsiter::I, true);
     
    get_link() = pc - 4;
    pc = Arm7VectorAddr::SWI + 8;
    is_branched = true;
}