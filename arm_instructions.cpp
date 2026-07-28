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


// 2S + 1N Cycles
Arm7TDMI::NextPCFetch Arm7TDMI::arm_branch(uint32_t opcode)
{
    Utils::print("ARM Branch & Branch w/ Link \n");
    assert(Utils::get_bits(opcode, 25, 28) == 0b101);

    // Documentation says shift than sign extend but I don't think it makes a difference
    int32_t sign_extended_offset = Utils::sign_extend32(opcode, 0, 23) << 2;

    if (Utils::is_bit_set(opcode, 24)) // Branch with Link
        get_link() = pc - 4;
    
    // The branch offset must take account of the prefetch operation, 
    // which causes the PC to be 2 words (8 bytes) ahead of the current instruction.
    reload_pipeline32(pc + sign_extended_offset + 8);

    return NextPCFetch::Sequential;
}

// 2S + 1N Cycles
Arm7TDMI::NextPCFetch Arm7TDMI::arm_branch_and_exchange(uint32_t opcode)
{
    Utils::print("ARM Branch And Exchange\n");

    assert(Utils::get_bits(opcode, 4, 28) == 0b0001'0010'1111'1111'1111'0001);

    // If R15 is used as an operand, the behaviour is undefined.
    uint32_t address = arm_get_rm(opcode);
    branch_and_exchange(address); // 1S + 1N

    return NextPCFetch::Sequential;
}

// Unused as GBA has no coprocessors
Arm7TDMI::NextPCFetch Arm7TDMI::arm_coprocessor_data_operation(uint32_t opcode) 
{
    Utils::print("ARM Coprocessor Data Operation\n");

    assert(Utils::get_bits(opcode, 24, 28) == 0b1110);
    assert(!Utils::is_bit_set(opcode, 4));

    return arm_undefined(opcode);
}

Arm7TDMI::NextPCFetch Arm7TDMI::arm_coprocessor_data_transfer(uint32_t opcode) 
{
    Utils::print("ARM Coprocessor Data Transfer\n");
    
    assert(Utils::get_bits(opcode, 25, 28) == 0b110);

    return arm_undefined(opcode);
}

Arm7TDMI::NextPCFetch Arm7TDMI::arm_coprocessor_register_transfer(uint32_t opcode) 
{
    Utils::print("ARM Coprocessor Register Transfer\n");

    assert(Utils::get_bits(opcode, 24, 28) == 0b1110);
    assert(Utils::is_bit_set(opcode, 4));

    return arm_undefined(opcode);
}

// Normal Data Processing: 1S
// Data Processing w/ reg specified shift: 1S + 1I
// Data Processing w/ dst == r15: 2S + 1N (Not TEQ, TST, CMP, CMN)
// Data Processing w/ two of above: 2S + 1N + 1I
Arm7TDMI::NextPCFetch Arm7TDMI::arm_data_processing(uint32_t opcode)
{
    Utils::print("ARM Data Processing\n");

    assert(Utils::get_bits(opcode, 26, 28) == 0b00);

    int dst_reg_index = Utils::get_bits(opcode, 12, 16);
    int src_reg_index = Utils::get_bits(opcode, 16, 20);

    uint32_t& dst_register = *registers[dst_reg_index];
    uint32_t& op1_register = *registers[src_reg_index];

    Utils::log("Dst Reg Index", dst_reg_index);
    Utils::log("Op1 Reg Index", src_reg_index);

    Utils::log("Dst Register", dst_register);
    Utils::log("Op1 Register", op1_register);

    int operation = Utils::get_bits(opcode, 21, 25);
    Utils::log("Operation", operation);

    bool set_condition_codes = Utils::is_bit_set(opcode, 20);
    bool is_immediate = Utils::is_bit_set(opcode, 25);

    Utils::log("Set CC", set_condition_codes);
    Utils::log("Is Immediate", is_immediate);

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
        int shift_amount = Utils::get_bits(opcode, 8, 12);

        Utils::log("Imm8", imm8);
        Utils::log("Shift Amount", shift_amount);

        bool is_arithmetic = operation == AluOps::Sbc || operation == AluOps::Rsc || operation == AluOps::Adc;
        op2 = (shift_amount == 0) ? 
            imm8 : 
            alu_ror(imm8, shift_amount * 2, set_condition_codes, !is_arithmetic);
    }
    else 
    {
        uint32_t& op2_register = arm_get_rm(opcode);

        bool is_register_shift = Utils::is_bit_set(opcode, 4);
        int shift_type = Utils::get_bits(opcode, 5, 7);

        Utils::log("Shift Type", shift_type);
        Utils::log("Is Register Shift", is_register_shift);

        uint32_t shift_amount{}; 
        if (is_register_shift) 
        {
            memory.add_internal_cycles();
            
            // Only the least significant byte of the contents of Rs is 
            // used to determine the shift amount.
            shift_amount = arm_get_rs(opcode) & 0xFF;

            //  If a register is used to specify the shift amount the PC will be 12 bytes ahead.
            pc += 4;
            is_branched = true;
        }
        else 
            shift_amount = Utils::get_bits(opcode, 7, 12);

        Utils::log("Shift Amount", +shift_amount);
        Utils::log("Shift", shift_amount % 32);

        bool is_arithmetic = operation == AluOps::Sbc || operation == AluOps::Rsc || operation == AluOps::Adc;
        bool update_carry_flag = set_condition_codes && !is_arithmetic;

        op2 = (shift_amount != 0 || !is_register_shift) ?
            decode_shift_operation_arm(op2_register, shift_amount, shift_type, set_condition_codes, update_carry_flag) :  
            op2_register;
    }

    Utils::log("Operand 2", op2);

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
        (void)alu_and_tst(op1_register, op2, true);
        break;
    case AluOps::Teq: // TEQ
        (void)alu_eor_teq(op1_register, op2, true);
        break;
    case AluOps::Cmp: // CMP
        (void)alu_sub_cmp(op1_register, op2, true);
        break;
    case AluOps::Cmn: // CMN
        (void)alu_add_cmn(op1_register, op2, true);
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

    Utils::log("Dst Register", dst_register);
    if (dst_reg_index == 15)
    {
        if (set_condition_codes)
        {
            uint32_t curr_mode = get_curr_mode();
            Utils::log("SPSR", std::bitset<32>(get_mode_spsr(curr_mode)));
            cpsr = get_mode_spsr(curr_mode);
        }
        if (operation < AluOps::Tst || operation > AluOps::Cmn)
            reload_pipeline32(pc + 8);
    }

    return NextPCFetch::Sequential;
}

// LDM: nS + 1N + 1I cycles
// LDM w/ PC: (n+1)S + 2N + 1I cycles
// STM: (n-1)S + 2N cycles
// where n is # of words transferred
Arm7TDMI::NextPCFetch Arm7TDMI::arm_block_data_transfer(uint32_t opcode)
{
    Utils::print("ARM Block Data Transfer\n");

    // This is taking years off my life
    assert(Utils::get_bits(opcode, 25, 28) == 0b100);

    static const std::array<uint32_t*, 16> usr_registers = {
        &r0, &r1, &r2, &r3, &r4, &r5, &r6, &r7,
        &r8, &r9, &r10, &r11, &r12, &r13, &r14, &r15
    };

    int base_register_index = Utils::get_bits(opcode, 16, 20);
    uint32_t& base_register = *registers[base_register_index];

    Utils::log("Base Register Index", base_register_index);
    Utils::log("Base Register", base_register);

    int register_list = Utils::get_bits(opcode, 0, 16);
    Utils::log("Register List", std::bitset<16>(register_list));

    bool add_offset_before_transfer = Utils::is_bit_set(opcode, 24); // P
    bool add_offset_to_base = Utils::is_bit_set(opcode, 23); // U
    bool load_psr_or_force_usr_mode = Utils::is_bit_set(opcode, 22); // S
    bool writeback_to_base = Utils::is_bit_set(opcode, 21); // W
    bool is_load = Utils::is_bit_set(opcode, 20); // L

    Utils::log("Add Before Transfer", add_offset_before_transfer);
    Utils::log("Add to Offset", add_offset_to_base);
    Utils::log("Load PSR Or Force USR Mode", load_psr_or_force_usr_mode);
    Utils::log("Writeback to Base", writeback_to_base);
    Utils::log("Is Load", is_load);

    int offset_amount = (add_offset_to_base) ? 4 : -4;
    uint32_t address = base_register;

    Utils::log("Offset Amount", offset_amount);

    bool use_usr_bank = load_psr_or_force_usr_mode && (!Utils::is_bit_set(register_list, 15) || !is_load);
    auto& registers_to_transfer = (use_usr_bank) ? usr_registers : registers;

    if (register_list == 0)
    {
        // Empty Rlist: R15 loaded/stored (ARMv4 only), and Rb=Rb+/-40h (ARMv4-v5).
        *registers_to_transfer[base_register_index] += offset_amount * 16;

        if (is_load)
        {
            pc = (memory.read32(*registers_to_transfer[base_register_index], AccessType::Sequential) + 4);

            return NextPCFetch::Sequential;
        }
        else
        {
            memory.write32(pc + 4, *registers_to_transfer[base_register_index], AccessType::NonSequential);

            return NextPCFetch::NonSequential;
        }
    }

    uint32_t address_to_write_to_base = address;

    AccessType access_type = AccessType::NonSequential;
    if (is_load)
    {
        for (int i = 0; i < 16; ++i)
        {
            // The lowest Register in Rlist (R0 if its in the list) will be 
            // loaded/stored to/from the lowest memory address.
            int index = (add_offset_to_base) ? i : 15 - i;
            if (!Utils::is_bit_set(register_list, index)) 
                continue;
        
            if (add_offset_before_transfer) 
            { 
                address += offset_amount;
                *registers_to_transfer[index] = memory.read32(address, access_type);
            }
            else
            {
                *registers_to_transfer[index] = memory.read32(address, access_type);
                address += offset_amount;
            }

            if (index == 15)
                reload_pipeline32(pc + 8);

            access_type = AccessType::Sequential;
        }  
    }
    else
    {
        pc += 4;
        is_branched = true;

        for (int i = 0; i < 16; ++i)
        {
            int index = (add_offset_to_base) ? i : 15 - i;
            if (!Utils::is_bit_set(register_list, index)) 
                continue;

            if (index == base_register_index)
                address_to_write_to_base = address + (add_offset_before_transfer ? offset_amount : 0);

            if (add_offset_before_transfer) 
            { 
                address += offset_amount;
                memory.write32(*registers_to_transfer[index], address, access_type);
            }
            else
            {
                memory.write32(*registers_to_transfer[index], address, access_type);
                address += offset_amount;
            }

            access_type = AccessType::Sequential;
        }       
    }

    if (load_psr_or_force_usr_mode && Utils::is_bit_set(register_list, 15) && is_load)
    {
        // LDM Rn,…,PC on ARMv4 leaves CPSR.T unchanged.
        cpsr = get_mode_spsr(get_curr_mode());
        handle_state_switch(cpsr & ProgramStatusRegsiter::Mode);
    }

    /*
    When write-back is specified, the base is written back at the end of the second cycle of 
    the instruction. During a STM, the first register is written out at the start of the second 
    cycle. A STM which includes storing the base, with the base as the first register to be 
    stored, will therefore store the unchanged value, whereas with the base second or later 
    in the transfer order, will store the modified value.
    */
    Utils::log("Address to write to base", address_to_write_to_base);

    if (writeback_to_base) 
    {
        // A LDM will always overwrite the updated base if the base is in the list. 
        if (is_load)
        {
            if (!Utils::is_bit_set(register_list, base_register_index))
            {
                *registers_to_transfer[base_register_index] = address;
                
                // Pipeline Flush
                if (base_register_index == 15)
                    reload_pipeline32(pc + 8);
            }
        }
        else
        {
            *registers_to_transfer[base_register_index] = address;
            Utils::log("Final Base Register", *registers_to_transfer[base_register_index]);

            bool base_is_first = true;
            for (int i = 0; i < 16; ++i)
            {
                if (Utils::is_bit_set(register_list, i))
                {
                    base_is_first = i == base_register_index;
                    break;
                }
            }

            Utils::log("Local Base Is First", base_is_first);
            if (!base_is_first && Utils::is_bit_set(register_list, base_register_index))
                memory.write32(address, address_to_write_to_base, AccessType::None);

            // Pipeline Flush
            if (base_register_index == 15)
                reload_pipeline32(pc + 8);
        }
    }

    // STM is unbelievably scuffed
    return (is_load) ? NextPCFetch::Sequential :NextPCFetch::NonSequential;
}

// LDR(H,SH,SB): 1S + 1N + 1I cycles
// LDR(H, SH, SB) w/ PC: 2S + 2N + 1I cycles
// STRH: 2N cycles
Arm7TDMI::NextPCFetch Arm7TDMI::arm_halfword_data_transfer(uint32_t opcode)
{
    Utils::print("ARM Halfword Data Transfer\n");

    assert(Utils::get_bits(opcode, 25, 28) == 0);
    assert(Utils::is_bit_set(opcode, 7));
    assert(Utils::is_bit_set(opcode, 4));

    bool add_before_transfer = Utils::is_bit_set(opcode, 24);
    bool add_to_offset = Utils::is_bit_set(opcode, 23);
    bool is_immediate_offset = Utils::is_bit_set(opcode, 22);
    bool writeback_to_base = Utils::is_bit_set(opcode, 21);
    bool is_load = Utils::is_bit_set(opcode, 20);
    int sh_flag = Utils::get_bits(opcode, 5, 7);

    Utils::log("Add Before Transfer", add_before_transfer);
    Utils::log("Add to Offset", add_to_offset);
    Utils::log("Is Immediate", is_immediate_offset);
    Utils::log("Writeback to Base", writeback_to_base);
    Utils::log("Is Load", is_load);
    Utils::log("SH Flag", sh_flag);

    assert(sh_flag != 0); // 0b00 is the SWAP instruction

    int src_dst_index = Utils::get_bits(opcode, 12, 16);
    int base_index = Utils::get_bits(opcode, 16, 20);
    
    uint32_t& dst_src_register = *registers[src_dst_index];
    uint32_t& base_register = *registers[base_index];

    Utils::log("Src/Dst Index", src_dst_index);
    Utils::log("Base Index", base_index);

    Utils::log("Src/Dst Register", dst_src_register);
    Utils::log("Base Register", base_register);

    uint32_t offset_amount = is_immediate_offset ? 
        Utils::get_bits(opcode, 8, 12) << 4 | Utils::get_bits(opcode, 0, 4) : 
        arm_get_rm(opcode);
        
    int total_offset = (add_to_offset) ? offset_amount : -offset_amount;
    uint32_t base_address = base_register;

    Utils::log("Offset Amount", total_offset);
    Utils::log("Base Address", base_address);

    if (sh_flag == 0b01) // Unsigned Halfwords
    {
        if (is_load)
        {
            memory.add_internal_cycles();

            if (add_before_transfer) 
            { 
                base_address += total_offset;
                uint16_t val = memory.read16(base_address, AccessType::NonSequential);
                dst_src_register = (base_address & 1) ? alu_ror(val, 8, false) : val;
            }
            else
            {
                uint16_t val = memory.read16(base_address, AccessType::NonSequential);
                dst_src_register = (base_address & 1) ? alu_ror(val, 8, false) : val;
                base_address += total_offset;
            }
        }
        else
        {
            if (src_dst_index == 15)
            {
                pc += 4;
                is_branched = true;
            }

            if (add_before_transfer) 
            { 
                base_address += total_offset;
                memory.write16(dst_src_register & 0xFFFF, base_address, AccessType::NonSequential);
            }
            else
            {
                memory.write16(dst_src_register & 0xFFFF, base_address, AccessType::NonSequential);
                base_address += total_offset;
            }
        }
    }
    else if (sh_flag == 0b10) // Signed Byte
    {
        if (is_load)
        {
            memory.add_internal_cycles();

            if (add_before_transfer) 
            { 
                base_address += total_offset;
                dst_src_register = Utils::sign_extend32(memory.read8(base_address, AccessType::NonSequential), 0, 7);
            }
            else
            {
                dst_src_register = Utils::sign_extend32(memory.read8(base_address, AccessType::NonSequential), 0, 7);
                base_address += total_offset;
            }
        }
        else
        {
            if (src_dst_index == 15)
            {
                pc += 4;
                is_branched = true;
            }

            if (add_before_transfer) 
            { 
                base_address += total_offset;
                memory.write8(dst_src_register, base_address, AccessType::NonSequential);
            }
            else
            {
                memory.write8(dst_src_register, base_address, AccessType::NonSequential);
                base_address += total_offset;
            }
        }
    }
    else // Signed Halfwords
    {
        if (is_load)
        {
            memory.add_internal_cycles();

            if (add_before_transfer) 
            { 
                base_address += total_offset;
                dst_src_register = (base_address & 1) ? 
                    Utils::sign_extend32(memory.read8(base_address + 1, AccessType::NonSequential), 0, 7) : 
                    Utils::sign_extend32(memory.read16(base_address, AccessType::NonSequential), 0, 15);
            }
            else
            {
                dst_src_register = (base_address & 1) ? 
                    Utils::sign_extend32(memory.read8(base_address + 1, AccessType::NonSequential), 0, 7) : 
                    Utils::sign_extend32(memory.read16(base_address, AccessType::NonSequential), 0, 15);
                base_address += total_offset;
            }
        }
        else
        {
            if (src_dst_index == 15)
            {
                pc += 4;
                is_branched = true;
            }

            if (add_before_transfer) 
            { 
                base_address += total_offset;
                memory.write16(dst_src_register, base_address, AccessType::NonSequential);
            }
            else
            {
                memory.write16(dst_src_register, base_address, AccessType::NonSequential);
                base_address += total_offset;
            }
        }
    }


    if (src_dst_index == 15 && is_load)
        reload_pipeline32(pc + 8);

    if (writeback_to_base || !add_before_transfer)
    {   
        if (src_dst_index != base_index || !is_load)
            base_register = base_address;

        if (base_index == 15)
        {
            if (src_dst_index != base_index || !is_load)
                reload_pipeline32(pc + 12);
        }
    }

    return (is_load) ? NextPCFetch::Sequential : NextPCFetch::NonSequential;
}

// MUL: 1S + ml cycles
// MLA: 1S + (m+1)I cycles
// m = 1 if bits [32:8] of mult operand are all zero or all one
// m = 2 if bits [32:16] of mult operand are all zero or all one
// m = 3 if bits [32:24] of mult operand are all zero or all one
// m = 4 in all other cases
Arm7TDMI::NextPCFetch Arm7TDMI::arm_multiply(uint32_t opcode)
{
    Utils::print("ARM Multiply\n");

    assert(Utils::get_bits(opcode, 22, 28) == 0);
    assert(Utils::get_bits(opcode, 4, 8) == 0b1001);

    bool set_condition_codes = Utils::is_bit_set(opcode, 20);
    bool accumulate = Utils::is_bit_set(opcode, 21);

    Utils::log("Set CC", set_condition_codes);
    Utils::log("Is Accumulate", accumulate);

    // Restrictions: Rd may not be same as Rm. Rd,Rn,Rs,Rm may not be R15.
    int op3_add_index = Utils::get_bits(opcode, 12, 16);
    int dst_reg_index = Utils::get_bits(opcode, 16, 20);

    Utils::log("Destination Idx", dst_reg_index);
    Utils::log("Op3 Idx", op3_add_index);

    uint32_t& dst_register = *registers[dst_reg_index];
    uint32_t& op3_register_add = *registers[op3_add_index];

    Utils::log("Dest Register Value", dst_register);
    Utils::log("Op3 Add Register Value", op3_register_add);

    uint32_t& op1_register_mult = arm_get_rs(opcode);
    uint32_t& op2_register_mult = arm_get_rm(opcode);

    Utils::log("Dst Register Old", dst_register);

    pc += 4;
    is_branched = true;

    memory.add_internal_cycles(get_mult_internal_cycles<MultType::MulMla>(op2_register_mult) + accumulate);

    uint32_t result = (op1_register_mult * op2_register_mult) + (accumulate * op3_register_add);
    if (dst_reg_index == 15)
        reload_pipeline32(result + 8);
    else 
        dst_register = result;

    
    if (set_condition_codes)
    {
        set_negative_and_zero(dst_register);
        set_cpsr(ProgramStatusRegsiter::C, dst_register < op1_register_mult);
        
        skip_mult_instr = true;
    }

    return NextPCFetch::Sequential;
}

// MULL: 1S + (m+1)I cycles
// MLAL: 1S + (m+2)I cycles
// For SMULL and SMLAL:
// m = 1 if bits [32:8] of mult operand are all zero or all one
// m = 2 if bits [32:16] of mult operand are all zero or all one
// m = 3 if bits [32:24] of mult operand are all zero or all one
// m = 4 in all other cases
// For UMULL and UMLAL:
// m = 1 if bits [32:8] of mult operand are all zero
// m = 2 if bits [32:16] of mult operand are all zero
// m = 3 if bits [32:24] of mult operand are all zero
// m = 4 in all other cases
Arm7TDMI::NextPCFetch Arm7TDMI::arm_multiply_long(uint32_t opcode)
{
    Utils::print("ARM Multiply Long\n");

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
        memory.add_internal_cycles(get_mult_internal_cycles<MultType::SmullSmlal>(op2_register_mult) + accumulate);

        int32_t op1_signed = static_cast<int32_t>(op1_register_mult);
        int32_t op2_signed = static_cast<int32_t>(op2_register_mult);

        int64_t result = static_cast<int64_t>(op1_signed) * static_cast<int64_t>(op2_signed);
        result_lo = (result & 0xFFFFFFFF) + (accumulate * dst_register_lo);

        bool carry = result_lo < (result & 0xFFFFFFFF);

        result_hi = (result >> 32) + (accumulate * dst_register_hi) + carry;
    }
    else
    {
        memory.add_internal_cycles(get_mult_internal_cycles<MultType::UmullUmlal>(op2_register_mult) + accumulate);

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
        reload_pipeline32(pc + 8);

    if (set_condition_codes)
    {
        set_cpsr(ProgramStatusRegsiter::N, dst_register_hi & Utils::MSB32);
        set_cpsr(ProgramStatusRegsiter::Z, dst_register_hi == 0 && dst_register_lo == 0);
         
        skip_mult_instr = true;
    }

    return NextPCFetch::Sequential;
}

// 1S cycle
Arm7TDMI::NextPCFetch Arm7TDMI::arm_psr_transfer(uint32_t opcode)
{
    Utils::print("ARM PSR Transfer\n");

    assert(Utils::get_bits(opcode, 26, 28) == 0b00);
    assert(Utils::get_bits(opcode, 23, 25) == 0b10);

    bool set_to_spsr = Utils::is_bit_set(opcode, 22);    
    uint32_t& psr = (set_to_spsr) ? get_mode_spsr(get_curr_mode()) : cpsr;

    Utils::log("Set to SPSR", set_to_spsr);
    Utils::log("SPSR Before", std::bitset<32>(psr));

    if (Utils::get_bits(opcode, 16, 22) == 0b001111)
    {
        Utils::print("MRS Transfer\n");
        // MRS (transfer PSR contents to a register)
        int dst_reg_index = Utils::get_bits(opcode, 12, 16);
        uint32_t& dest_register = *registers[dst_reg_index];

        dest_register = psr;

        // Strangely no Rd = 15 edge case for these tests
    }
    else if (!set_to_spsr || mode_has_spsr())
    {
        bool is_immediate = Utils::is_bit_set(opcode, 25);
        Utils::log("Is Immediate", is_immediate);

        uint32_t byte_mask = Utils::is_bit_set(opcode, 16) ? 0xFF : 0;
        byte_mask |= Utils::is_bit_set(opcode, 17) ? 0x0000FF00 : 0;
        byte_mask |= Utils::is_bit_set(opcode, 18) ? 0x00FF0000 : 0;
        byte_mask |= Utils::is_bit_set(opcode, 19) ? 0xFF000000 : 0;
        Utils::log("Byte Mask", std::bitset<32>(byte_mask));

        uint32_t mask = (!is_privileged_mode()) ? byte_mask & 0xFF000000 : byte_mask;
        uint32_t operand{};

        if (is_immediate)
        {
            Utils::print("MSR (Immediate)\n");
            // MSR (transfer register contents or imm val to PSR flag bits)

            uint32_t imm8 = Utils::get_bits(opcode, 0, 8);
            int rotate = Utils::get_bits(opcode, 8, 12);
            
            operand = alu_ror(imm8, rotate * 2, false);
            Utils::log("Operand", std::bitset<32>(imm8));

            psr = (psr & ~mask) | (operand & mask);
        }
        else
        {
            Utils::print("MSR (Register Operands)\n");

            // MSR (transfer register contents to PSR)
            operand = arm_get_rm(opcode); 
            Utils::log("Src Register", std::bitset<32>(operand));
            
            if (operand & 0x0FFFFF00) // UnallocMask
                Utils::print("UNPREDICTBLE\n");
            
            // In non-privileged mode (user mode): only condition 
            // code bits of CPSR can be changed, control bits can’t.
            Utils::log("Mask", std::bitset<16>(mask));
            Utils::log("Result", std::bitset<16>(operand & mask));

            // Why is bit 4 not set here? huh?
            // For one of the SPSRs
            // 01010000000000000001001001100111 <-- Expected
            // 01010000000000000001001001110111 <-- What I got

            /// @note should probably look into the behaviour for when
            /// the mode bits are set to an invalid mode number
        }

        psr = (psr & ~mask) | (operand & mask);

        if (!set_to_spsr && (byte_mask & 0xFF))
        {
            cpsr |= 0x10;
            handle_mode_switch(cpsr & ProgramStatusRegsiter::Mode);
        }
    }

    return NextPCFetch::Sequential;
}

// 2S + 1N + 1I cycles
Arm7TDMI::NextPCFetch Arm7TDMI::arm_single_data_swap(uint32_t opcode)
{
    Utils::print("ARM Single Data Swap\n");

    assert(Utils::get_bits(opcode, 23, 28) == 0b00010);
    assert(Utils::get_bits(opcode, 20, 22) == 0b00);
    assert(Utils::get_bits(opcode, 8, 12) == 0b0000);
    assert(Utils::get_bits(opcode, 4, 8) == 0b1001);

    bool swap_byte = Utils::is_bit_set(opcode, 22);
    Utils::log("Byte Swap", swap_byte);

    int dst_reg_index = Utils::get_bits(opcode, 12, 16);
    int swap_reg_index = Utils::get_bits(opcode, 16, 20);

    uint32_t& dst_register = *registers[dst_reg_index];
    uint32_t& swap_address = *registers[swap_reg_index];
    uint32_t& src_register = arm_get_rm(opcode);

    Utils::log("Destination Index", dst_reg_index);
    Utils::log("Swap Register Index", swap_reg_index);

    Utils::log("Dest Register Value", dst_register);
    Utils::log("Swap Address Value", swap_address);
    Utils::log("Src Register Value", src_register);

    pc += 4;
    is_branched = true;

    uint32_t value{};

    if (swap_byte)
    {
        uint8_t swap_address_value = memory.read8(swap_address, AccessType::NonSequential);    
        memory.write8(src_register, swap_address, AccessType::Sequential);
        value = swap_address_value;
    }
    else
    {
        uint32_t swap_address_value = memory.read32(swap_address, AccessType::NonSequential);
        memory.write32(src_register, swap_address, AccessType::Sequential);
        value = (swap_address & 3) ? alu_ror(swap_address_value, (swap_address & 3) * 8, false) : swap_address_value;
    }

    memory.add_internal_cycles();

    dst_register = value;
    if (dst_reg_index == 15)
        reload_pipeline32(pc + 8);

    Utils::log("Final Dst Value", dst_register);

    return NextPCFetch::Sequential;
}

// LDR: 1S + 1N + 1I cycles
// LDR w/ PC: 2S + 2N + 1I cycles
// STR: 2N cycles
Arm7TDMI::NextPCFetch Arm7TDMI::arm_single_data_transfer(uint32_t opcode)
{
    Utils::print("ARM Single Data Transfer\n");

    assert(Utils::get_bits(opcode, 26, 28) == 0b01);

    bool is_load = Utils::is_bit_set(opcode, 20); // L
    bool writeback_to_base = Utils::is_bit_set(opcode, 21); // W
    bool is_byte = Utils::is_bit_set(opcode, 22); // B
    bool add_to_base = Utils::is_bit_set(opcode, 23); // U
    bool add_before_transfer = Utils::is_bit_set(opcode, 24); // P
    bool is_register_offset = Utils::is_bit_set(opcode, 25); // I

    Utils::log("Is Load", is_load);
    Utils::log("Writeback to Base", writeback_to_base);
    Utils::log("Is Byte", is_byte);
    Utils::log("Add to Base", add_to_base);
    Utils::log("Add Before Transfer", add_before_transfer);
    Utils::log("Is Register Offset", is_register_offset);

    int src_dst_index = Utils::get_bits(opcode, 12, 16);
    int base_index = Utils::get_bits(opcode, 16, 20);

    uint32_t& dst_src_register = *registers[src_dst_index];
    uint32_t& base_register = *registers[base_index];

    Utils::log("Src/Dst Index: ", src_dst_index);
    Utils::log("Base Index: ", base_index);
    Utils::log("Src/Dst Register: ", dst_src_register);
    Utils::log("Base Register: ", base_register);

    uint32_t offset{};
    if (!is_register_offset)
        offset = Utils::get_bits(opcode, 0, 12);
    else
    {
        // the register specified shift amounts are not available in this instruction class.
        uint32_t op2_register = arm_get_rm(opcode);
        int shift_amount = Utils::get_bits(opcode, 7, 12);
        int shift_type = Utils::get_bits(opcode, 5, 7);

        Utils::log("Shift Amount", shift_amount);
        Utils::log("Shift Type", shift_type);

        offset = decode_shift_operation_arm(op2_register, shift_amount, shift_type, false, false);
    }

    int offset_amount = (add_to_base) ? offset : -offset;
    Utils::log("Offset Amount", offset_amount);

    uint32_t base_address = base_register;

    if (is_byte)
    {
        if (is_load)
        {
            memory.add_internal_cycles();

            if (add_before_transfer) 
            { 
                base_address += offset_amount;
                dst_src_register = memory.read8(base_address, AccessType::NonSequential);
            }
            else
            {
                dst_src_register = memory.read8(base_address, AccessType::NonSequential);
                base_address += offset_amount;
            }
        }
        else 
        {
            // When R15 is the source register (Rd) of a register store (STR) instruction, 
            // the store red value will be address of the instruction plus 12.
            if (src_dst_index == 15)
            {
                pc += 4;
                is_branched = true;
            }

            if (add_before_transfer) 
            { 
                base_address += offset_amount;
                memory.write8(dst_src_register, base_address, AccessType::NonSequential);
            }
            else
            {
                memory.write8(dst_src_register, base_address, AccessType::NonSequential);
                base_address += offset_amount;
            }
        }
    }
    else
    {
        if (is_load)
        {
            memory.add_internal_cycles();

            if (add_before_transfer) 
            { 
                base_address += offset_amount;
                uint32_t val = memory.read32(base_address, AccessType::NonSequential);
                dst_src_register = (base_address & 3) ? alu_ror(val, (base_address & 3) * 8, false) : val;
            }
            else
            {
                uint32_t val = memory.read32(base_address, AccessType::NonSequential);
                dst_src_register = (base_address & 3) ? alu_ror(val, (base_address & 3) * 8, false) : val;
                base_address += offset_amount;
            }
        }
        else 
        {
            // When R15 is the source register (Rd) of a register store (STR) instruction, 
            // the stored value will be address of the instruction plus 12.
            if (src_dst_index == 15)
            {
                pc += 4;
                is_branched = true;
            }

            if (add_before_transfer) 
            { 
                base_address += offset_amount;
                memory.write32(dst_src_register, base_address, AccessType::NonSequential);
            }
            else
            {
                memory.write32(dst_src_register, base_address, AccessType::NonSequential);
                base_address += offset_amount;
            }
        }
    }

    if (src_dst_index == 15 && is_load)
        reload_pipeline32(pc + 8);

    if (writeback_to_base || !add_before_transfer)
    {   
        if (src_dst_index != base_index || !is_load)
            base_register = base_address;

        if (base_index == 15)
        {
            // Would this actually lead to 2 pipeline flushes?
            if (src_dst_index != base_index || !is_load)
                reload_pipeline32(pc + 12);

        }
    }

    return NextPCFetch::Sequential;
}

// 2S + 1N Cycles
Arm7TDMI::NextPCFetch Arm7TDMI::arm_software_interrupt(uint32_t opcode)
{
    Utils::print("ARM Software Interrupt\n");

    assert(Utils::get_bits(opcode, 24, 28) == 0b1111);

    /// @note The bottom 24 bits of the instruction are ignored by the processor
    spsr_svc = cpsr;

    handle_state_switch(ArmState::Arm);
    handle_mode_switch(ArmMode::Supervisor); 
    set_cpsr(ProgramStatusRegsiter::I, true);
     
    get_link() = pc - 4;
    reload_pipeline32(Arm7VectorAddr::SWI + 8); // 1S + 1N

    return NextPCFetch::Sequential;
}

// 2S + 1I + 1N cycles (Where does the internal cycle even come from?)
Arm7TDMI::NextPCFetch Arm7TDMI::arm_undefined(uint32_t opcode)
{
    Utils::print("ARM Undefined\n");

    // assert(Utils::get_bits(opcode, 25, 28) == 0b011);
    // assert(Utils::is_bit_set(opcode, 4));

    /// @note The bottom 24 bits of the instruction are ignored by the processor
    spsr_svc = cpsr;

    handle_state_switch(ArmState::Arm);
    handle_mode_switch(ArmMode::Supervisor);
    set_cpsr(ProgramStatusRegsiter::I, true);

    memory.add_internal_cycles();

    get_link() = pc - 4;
    reload_pipeline32(Arm7VectorAddr::UNDEFINED + 8); // 1S + 1N

    return NextPCFetch::Sequential;
}