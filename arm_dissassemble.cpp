#include "arm7dissassembler.hpp"

std::string Arm7Dissassembler::arm_branch_dissassemble(uint32_t opcode) 
{ 
    int32_t sign_extended_offset = Utils::sign_extend32(opcode, 0, 23) << 1;
    bool with_link = Utils::is_bit_set(opcode, 24);

    std::string instruction = "B";
    if (with_link) instruction += "L";
    instruction += get_condition_code(opcode) + " #";
    instruction += Utils::int_to_hex(sign_extended_offset);

    return instruction; 
} 

std::string Arm7Dissassembler::arm_branch_and_exchange_dissassemble(uint32_t opcode) 
{ 
    std::string rn = arm_get_rm(opcode);
    std::string instruction = "BX" + get_condition_code(opcode) + " ";
    instruction += rn;

    return instruction; 
}

std::string Arm7Dissassembler::arm_block_data_transfer_dissassemble(uint32_t opcode) 
{ 
    auto [base_reg, none] = arm_get_rn_rd(opcode);
    int register_list = Utils::get_bits(opcode, 0, 16);
    bool is_load = Utils::is_bit_set(opcode, 20); // L
    bool writeback_to_base = Utils::is_bit_set(opcode, 21); // W
    bool psr_or_force_usr = Utils::is_bit_set(opcode, 22); // S
    bool add_to_base = Utils::is_bit_set(opcode, 23); // U
    bool add_before_transfer = Utils::is_bit_set(opcode, 24); // P

    (void)none;

    std::string instruction = (is_load) ? "LDM" : "STM";
    instruction += get_condition_code(opcode);
    instruction += (add_to_base) ? "I" : "D";
    instruction += (add_before_transfer) ? "B " : "A ";
    
    instruction += base_reg;
    if (writeback_to_base) instruction += "!";
    instruction += ", { ";

    bool first_entry_written = false;
    for (int i = 0; i < 16; ++i)
    {
        if (!Utils::is_bit_set(register_list, i)) continue;
        
        if (first_entry_written) instruction += ", ";
        instruction += "R" + std::to_string(i);

        first_entry_written = true;
    }

    instruction += " }";
    if (psr_or_force_usr) instruction += "^";

    return instruction; 
}

std::string Arm7Dissassembler::arm_coprocessor_data_operation_dissassemble(uint32_t opcode) { return "UND"; }
std::string Arm7Dissassembler::arm_coprocessor_data_transfer_dissassemble(uint32_t opcode) { return "UND"; }
std::string Arm7Dissassembler::arm_coprocessor_register_transfer_dissassemble(uint32_t opcode) { return "UND"; }

std::string Arm7Dissassembler::arm_data_processing_dissassemble(uint32_t opcode) 
{ 
    auto [src_reg, dst_reg] = arm_get_rn_rd(opcode);
    int operation = Utils::get_bits(opcode, 21, 25);
    bool is_immediate = Utils::is_bit_set(opcode, 25);

    std::string instruction{};
    switch (operation)
    {
        case 0b0000: instruction += "AND"; break;
        case 0b0001: instruction += "EOR"; break;
        case 0b0010: instruction += "SUB"; break;
        case 0b0011: instruction += "RSB"; break;
        case 0b0100: instruction += "ADD"; break;
        case 0b0101: instruction += "ADC"; break;
        case 0b0110: instruction += "SBC"; break;
        case 0b0111: instruction += "RSC"; break;
        case 0b1000: instruction += "TST"; break;
        case 0b1001: instruction += "TEQ"; break;
        case 0b1010: instruction += "CMP"; break;
        case 0b1011: instruction += "CMN"; break;
        case 0b1100: instruction += "ORR"; break;
        case 0b1101: instruction += "MOV"; break;
        case 0b1110: instruction += "BIC"; break;
        case 0b1111: instruction += "MVN"; break;
        default: break;
    }
    instruction += get_condition_code(opcode);

    // not TST, TEQ, CMN, or CMP
    instruction += (operation < 0b1000 || operation > 0b1011) ? arm_set_cc(opcode) + " " + dst_reg +  ", " : " ";

    if (operation != 0b1101) instruction += src_reg + ", ";
    
    if (is_immediate)
    {
        uint32_t imm8 = Utils::get_bits(opcode, 0, 8);
        int shift_amount = Utils::get_bits(opcode, 8, 12);

        instruction += "#";
        instruction += (shift_amount == 0) ? Utils::int_to_hex(imm8) : compute_ror(imm8, shift_amount * 2);
    }
    else
    {
        std::string op2_reg = arm_get_rm(opcode);
        int shift_type = Utils::get_bits(opcode, 5, 7);
        int shift_amount = Utils::get_bits(opcode, 7, 12);
        bool is_register_shift = Utils::is_bit_set(opcode, 4);

        instruction += op2_reg;
        if (shift_amount != 0 || is_register_shift)
        {
            instruction += + ", " + encode_shift_operation(shift_type);
            instruction += (is_register_shift) ? arm_get_rs(opcode) : "#" + Utils::int_to_hex(shift_amount);
        }
    }

    return instruction; 
}

std::string Arm7Dissassembler::arm_halfword_data_transfer_dissassemble(uint32_t opcode) 
{ 
    auto [base_reg, dst_src_reg] = arm_get_rn_rd(opcode);
    bool is_halfword = Utils::is_bit_set(opcode, 5);
    bool is_signed = Utils::is_bit_set(opcode, 6);
    bool is_load = Utils::is_bit_set(opcode, 20); // L
    bool writeback_to_base = Utils::is_bit_set(opcode, 21); // W
    bool is_immediate_offset = Utils::is_bit_set(opcode, 22);
    bool add_to_base = Utils::is_bit_set(opcode, 23); // U
    bool add_before_transfer = Utils::is_bit_set(opcode, 24); // P

    std::string instruction = (is_load) ? "LDR" : "STR";
    instruction += get_condition_code(opcode);
    if (is_signed) instruction += "S";
    instruction += (is_halfword) ? "H " : "B ";

    instruction += dst_src_reg + ", [" + base_reg;
    instruction += (add_before_transfer) ? ", " : "], ";

    if (is_immediate_offset)
    {
        int immediate8 = Utils::get_bits(opcode, 8, 12) << 4 | Utils::get_bits(opcode, 0, 4);
        instruction += "#";
        if (!add_to_base) instruction += "-";
        instruction += Utils::int_to_hex(immediate8);
    }
    else
    {
        if (!add_to_base) instruction += "-";
        instruction += arm_get_rm(opcode);
    }

    if (add_before_transfer) 
    {
        instruction += "]";
        if (writeback_to_base) instruction += "!";
    }

    return instruction; 
}

std::string Arm7Dissassembler::arm_multiply_dissassemble(uint32_t opcode) 
{ 
    auto [dst_reg, op3_reg_add] = arm_get_rn_rd(opcode);
    std::string op1_reg_mult = arm_get_rm(opcode);
    std::string op2_reg_mult = arm_get_rs(opcode);

    bool accumulate = Utils::is_bit_set(opcode, 21);

    std::string instruction = (accumulate) ? "MLA" : "MUL";
    instruction += get_condition_code(opcode);
    instruction += arm_set_cc(opcode) + " ";
    instruction += dst_reg + ", " + op2_reg_mult + ", " + op1_reg_mult;
    if (accumulate) instruction += ", " + op3_reg_add;

    return instruction; 
}

std::string Arm7Dissassembler::arm_multiply_long_dissassemble(uint32_t opcode) 
{ 
    auto [dst_reg_hi, dst_reg_lo] = arm_get_rn_rd(opcode);
    std::string op1_reg_mult = arm_get_rm(opcode);
    std::string op2_reg_mult = arm_get_rs(opcode);

    bool is_accumulate = Utils::is_bit_set(opcode, 21);
    bool is_signed = Utils::is_bit_set(opcode, 22);

    std::string instruction = (is_signed) ? "S" : "U";
    instruction += (is_accumulate) ? "MLAL" : "MULL";
    instruction += get_condition_code(opcode);
    instruction += arm_set_cc(opcode);
    instruction += dst_reg_lo + ", " + dst_reg_hi + ", " + op1_reg_mult + ", " + op2_reg_mult;
    
    return instruction; 
}

std::string Arm7Dissassembler::arm_psr_transfer_dissassemble(uint32_t opcode) 
{ 
    auto [none, dst_src_reg] = arm_get_rn_rd(opcode);
    bool is_mrs = Utils::get_bits(opcode, 16, 22) == 0b001111;
    bool set_to_spsr = Utils::is_bit_set(opcode, 22);    

    (void)none;

    std::string instruction ;
    if (is_mrs)
    {
        instruction = "MRS" + get_condition_code(opcode) + " ";
        instruction += dst_src_reg + " ";
        instruction += (set_to_spsr) ? "SPSR" : "CPSR";
    }
    else
    {
        instruction = "MSR" + get_condition_code(opcode) + " ";
        instruction += (set_to_spsr) ? "SPSR_" : "CPSR_";
        if (Utils::is_bit_set(opcode, 16)) instruction += "c";
        if (Utils::is_bit_set(opcode, 17)) instruction += "x";
        if (Utils::is_bit_set(opcode, 18)) instruction += "s";
        if (Utils::is_bit_set(opcode, 19)) instruction += "f";
        instruction += " ";

        bool is_immediate = Utils::is_bit_set(opcode, 25);
        if (is_immediate)
        {
            uint32_t imm8 = Utils::get_bits(opcode, 0, 8);
            int rotate = Utils::get_bits(opcode, 8, 12);

            instruction += compute_ror(imm8, rotate);
        }
        else
            instruction += arm_get_rm(opcode);
    }
    return instruction; 
}

std::string Arm7Dissassembler::arm_software_interrupt_dissassemble(uint32_t opcode) 
{ 
    int value24 = Utils::get_bits(opcode, 0, 24);

    return "SWI " + std::to_string(value24);
}

std::string Arm7Dissassembler::arm_single_data_swap_dissassemble(uint32_t opcode) 
{ 
    auto [base_reg, dst_reg] = arm_get_rn_rd(opcode);
    std::string src_reg = arm_get_rm(opcode);
    bool swap_byte = Utils::is_bit_set(opcode, 22);

    std::string instruction = "SWP" + get_condition_code(opcode);
    instruction += (swap_byte) ? "B " : " ";
    instruction += dst_reg + ", " + src_reg + ", [" + base_reg + "]";

    return instruction; 
}

std::string Arm7Dissassembler::arm_single_data_transfer_dissassemble(uint32_t opcode) 
{ 
    auto [base_reg, dst_reg] = arm_get_rn_rd(opcode);
    bool is_load = Utils::is_bit_set(opcode, 20); // L
    bool writeback_to_base = Utils::is_bit_set(opcode, 21); // W
    bool is_byte = Utils::is_bit_set(opcode, 22); // B
    bool add_to_base = Utils::is_bit_set(opcode, 23); // U
    bool add_before_transfer = Utils::is_bit_set(opcode, 24); // P
    bool is_register_offset = Utils::is_bit_set(opcode, 25); // I
    
    std::string instruction = (is_load) ? "LDR" : "STR";
    instruction += get_condition_code(opcode);
    instruction += (is_byte) ? "B " : " ";
    instruction += dst_reg + ", [" + base_reg;
    instruction += (add_before_transfer) ? ", " : "], ";

    if (!is_register_offset)
    {
        int offset = Utils::get_bits(opcode, 0, 12);
        instruction += "#";
        if (!add_to_base) instruction += "-";
        instruction += Utils::int_to_hex(offset);
    }
    else
    {
        std::string op2_reg = arm_get_rm(opcode);
        int shift_amount = Utils::get_bits(opcode, 7, 12);
        int shift_type = Utils::get_bits(opcode, 5, 7);

        instruction += op2_reg + ", " + encode_shift_operation(shift_type);
        instruction += "#" + Utils::int_to_hex(shift_amount);
    }

    if (add_before_transfer) 
    {
        instruction += "]";
        if (writeback_to_base) instruction += "!";
    }

    return instruction; 
}

std::string Arm7Dissassembler::arm_undefined_dissassemble(uint32_t opcode) 
{ 
    // There's no mnemonic for UNDEFINED so this is the best I could do
    return "UND"; 
}
