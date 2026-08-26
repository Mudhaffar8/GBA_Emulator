#include "arm7dissassembler.hpp"

std::string Arm7Dissassembler::thumb_add_subtract_dissassemble(uint16_t opcode) 
{
    auto [dst_reg, src_reg] = thumb_get_dst_src(opcode);

    int rn_or_offset3 = Utils::get_bits(opcode, 6, 9);
    bool is_sub = Utils::is_bit_set(opcode, 9);
    bool is_immediate = Utils::is_bit_set(opcode, 10);

    std::string instruction = (is_sub) ? "SUB " : "ADD ";
    instruction += dst_reg + ", ";
    instruction += src_reg + ", ";

    instruction += (is_immediate) ? Utils::int_to_hex(rn_or_offset3) : "R" + std::to_string(rn_or_offset3);

    return instruction;
}

std::string Arm7Dissassembler::thumb_add_offset_sp_dissassemble(uint16_t opcode)
{
    uint32_t immediate9 = Utils::get_bits(opcode, 0, 7) << 2;
    bool is_sub = Utils::is_bit_set(opcode, 7);

    std::string instruction{};
    instruction += (is_sub) ? "SUB SP, " : "ADD SP, ";
    instruction += Utils::int_to_hex(immediate9);

    return instruction;
}

std::string Arm7Dissassembler::thumb_alu_operations_dissassemble(uint16_t opcode)
{
    auto [dst_reg, src_reg] = thumb_get_dst_src(opcode);
    int operation = Utils::get_bits(opcode, 6, 10);    

    std::string instruction{};

    switch (operation)
    {
        case 0b0000: instruction += "AND "; break;
        case 0b0001: instruction += "EOR "; break;
        case 0b0010: instruction += "LSL "; break;
        case 0b0011: instruction += "LSR "; break;
        case 0b0100: instruction += "ASR "; break;
        case 0b0101: instruction += "ADC "; break;
        case 0b0110: instruction += "SBC "; break;
        case 0b0111: instruction += "ROR "; break;
        case 0b1000: instruction += "TST "; break;
        case 0b1001: instruction += "NEG "; break;
        case 0b1010: instruction += "CMP "; break;
        case 0b1011: instruction += "CMN "; break;
        case 0b1100: instruction += "ORR "; break;
        case 0b1101: instruction += "MUL "; break;
        case 0b1110: instruction += "BIC "; break;
        case 0b1111: instruction += "MVN "; break;
        default: break;
    }

    instruction += dst_reg + ", " + src_reg;
    return instruction;
}

std::string Arm7Dissassembler::thumb_conditional_branch_dissassemble(uint16_t opcode)
{
    int32_t signed_offset9 = Utils::sign_extend32(opcode, 0, 7) << 1;
    uint32_t condition_code = Utils::get_bits(opcode, 8, 11) << 28;

    uint32_t final_addr = (instruction_address + signed_offset9) + 4;

    std::string instruction = "B" + get_condition_code(condition_code);
    instruction += " " + Utils::int_to_hex(final_addr);

    return instruction;
}

std::string Arm7Dissassembler::thumb_hi_reg_op_branch_exchange_dissassemble(uint16_t opcode)
{
    int dst_reg_index = Utils::get_bits(opcode, 0, 3);
    int src_reg_index = Utils::get_bits(opcode, 3, 6);
    int operation = Utils::get_bits(opcode, 8, 10);
    bool hi_flag_2 = Utils::is_bit_set(opcode, 6);
    bool hi_flag_1 = Utils::is_bit_set(opcode, 7);

    std::string dst_reg = "R" + std::to_string(dst_reg_index + (8 * hi_flag_1));
    std::string src_reg = "R" + std::to_string(src_reg_index + (8 * hi_flag_2));

    std::string instruction{};

    switch(operation)
    {
        case 0: instruction += "ADD " + dst_reg + ", " + src_reg; break;
        case 1: instruction += "CMP " + dst_reg + ", " + src_reg; break;
        case 2: instruction += "MOV " + dst_reg + ", " + src_reg; break;
        case 3: instruction += "BX " + src_reg; break;
    }

    return instruction;
}

std::string Arm7Dissassembler::thumb_load_address_dissassemble(uint16_t opcode)
{
    std::string dst_reg = thumb_get_dst(opcode);

    uint32_t immediate = Utils::get_bits(opcode, 0, 8) << 2;
    bool is_stack_pointer = Utils::is_bit_set(opcode, 11);

    std::string instruction = "ADD " + dst_reg + ", ";
    instruction += (is_stack_pointer) ? "SP, " : "PC, ";
    instruction += Utils::int_to_hex(immediate);

    return instruction;
}

std::string Arm7Dissassembler::thumb_load_store_halfword_dissassemble(uint16_t opcode)
{
    auto [dst_src_reg, base_reg] = thumb_get_dst_src(opcode);
    
    uint32_t offset6 = Utils::get_bits(opcode, 6, 11) << 1;

    bool is_load = Utils::is_bit_set(opcode, 11);

    std::string instruction = (is_load) ? "LDRH " : "STRH ";
    instruction += dst_src_reg + ", [";
    instruction += base_reg + ", " + Utils::int_to_hex(offset6) + "]";

    return instruction;
}

std::string Arm7Dissassembler::thumb_load_store_immediate_dissassemble(uint16_t opcode)
{
    auto [dst_src_register, base_register] = thumb_get_dst_src(opcode);

    uint32_t offset5 = Utils::get_bits(opcode, 6, 11);

    bool is_load = Utils::is_bit_set(opcode, 11);
    bool is_byte = Utils::is_bit_set(opcode, 12);

    std::string instruction = (is_load) ? (is_byte ? "LDRB " : "LDR ") : (is_byte ? "STRB " : "STR ");
    instruction += dst_src_register + ", [";
    instruction += base_register + ", " + Utils::int_to_hex(offset5) + "]";

    return instruction;
}

std::string Arm7Dissassembler::thumb_load_store_sign_extend_halfword_dissassemble(uint16_t opcode)
{
    auto [dst_src_register, base_register] = thumb_get_dst_src(opcode);

    int offset_reg_index = Utils::get_bits(opcode, 6, 9);

    bool is_sign_extended = Utils::is_bit_set(opcode, 10);
    bool h_flag = Utils::is_bit_set(opcode, 11);

    std::string instruction = (is_sign_extended) ? (h_flag ? "LDSH " : "LDSB ") : (h_flag ? "STRH " : "LDRH ");
    instruction += dst_src_register + ", [" + base_register + ", ";
    instruction += "R" + std::to_string(offset_reg_index) + "]";

    return instruction;
}

std::string Arm7Dissassembler::thumb_load_store_w_reg_offset_dissassemble(uint16_t opcode)
{
    auto [dst_src_register, base_register] = thumb_get_dst_src(opcode);

    int offset_reg_index = Utils::get_bits(opcode, 6, 9);

    bool is_byte = Utils::is_bit_set(opcode, 10);
    bool is_load = Utils::is_bit_set(opcode, 11);

    std::string instruction = (is_load) ? (is_byte ? "LDRB " : "LDR ") : (is_byte ? "STRB " : "STR ");
    instruction += dst_src_register + ", [" + base_register + ", ";
    instruction += "R" + std::to_string(offset_reg_index) + "]";

    return instruction;
}

std::string Arm7Dissassembler::thumb_long_branch_w_link_dissassemble(uint16_t opcode)
{
    uint32_t offset = Utils::get_bits(opcode, 0, 11);
    bool is_offset_low = Utils::is_bit_set(opcode, 11);

    std::string instruction = (is_offset_low) ? 
        "ADD LR, PC, " + Utils::int_to_hex(offset << 12) : "BL LR + " + Utils::int_to_hex(offset << 1);

    return instruction;
}

std::string Arm7Dissassembler::thumb_move_cmp_add_sub_immediate_dissassemble(uint16_t opcode)
{
    std::string dst_reg = thumb_get_dst(opcode);

    int operation = Utils::get_bits(opcode, 11, 13);
    int offset = Utils::get_bits(opcode, 0, 8);

    std::string instruction{};

    switch(operation)
    {
        case 0: instruction = "MOV "; break;
        case 1: instruction = "CMP "; break;
        case 2: instruction = "ADD "; break;
        case 3: instruction = "SUB "; break;
    }

    instruction += dst_reg + ", " + Utils::int_to_hex(offset);

    return instruction;
}

std::string Arm7Dissassembler::thumb_move_shifted_register_dissassemble(uint16_t opcode)
{
    auto [dst_reg, src_reg] = thumb_get_dst_src(opcode);

    int operation = Utils::get_bits(opcode, 11, 13);
    uint32_t offset5 = Utils::get_bits(opcode, 6, 11);
    
    std::string instruction = encode_shift_operation(operation); // Already comes w/ space
    instruction += dst_reg + ", " + src_reg + ", " + Utils::int_to_hex(offset5);

    return instruction;
}

std::string Arm7Dissassembler::thumb_multiple_load_store_dissassemble(uint16_t opcode)
{
    int r_list = Utils::get_bits(opcode, 0, 8);
    int base_reg_index = Utils::get_bits(opcode, 8, 11);
    bool is_load = Utils::is_bit_set(opcode, 11);

    std::string instruction = (is_load) ? "LDMIA " : "STMIA ";
    instruction += "R" + std::to_string(base_reg_index) + "!, { ";

    bool first_entry = false;
    for (int i = 0; i < 8; ++i)
    {
        if (!Utils::is_bit_set(r_list, i)) continue;
        
        if (first_entry) instruction += ", ";
        instruction += "R" + std::to_string(i);

        first_entry = true;
    }

    instruction += " }";

    return instruction;
}

std::string Arm7Dissassembler::thumb_pc_relative_load_dissassemble(uint16_t opcode)
{
    std::string dst_reg = thumb_get_dst(opcode);
    uint32_t immediate10 = Utils::get_bits(opcode, 0, 8) << 2;

    std::string instruction = "LDR " + dst_reg + ", [PC, ";
    instruction += Utils::int_to_hex(immediate10);
    instruction += "]";
    return instruction;
}

std::string Arm7Dissassembler::thumb_push_pop_registers_dissassemble(uint16_t opcode)
{
    int r_list = Utils::get_bits(opcode, 0, 8);
    bool pc_lr_bit = Utils::is_bit_set(opcode, 8);
    bool is_pop = Utils::is_bit_set(opcode, 11);

    std::string instruction = (is_pop) ? "POP { " : "PUSH { ";

    bool first_entry = false;
    for (int i = 0; i < 8; ++i)
    {
        if (!Utils::is_bit_set(r_list, i)) continue;
        
        if (first_entry) instruction += ", ";
        instruction += "R" + std::to_string(i);

        first_entry = true;
    }

    if (pc_lr_bit) 
    {
        if (first_entry) instruction += ", ";
        instruction += (is_pop) ? "PC" : "LR";
    }
    instruction += " }";

    return instruction;
}

std::string Arm7Dissassembler::thumb_software_interrupt_dissassemble(uint16_t opcode)
{
    int value8 = Utils::get_bits(opcode, 0, 8);

    std::string instruction = "SWI " + std::to_string(value8);
    return instruction;
}

std::string Arm7Dissassembler::thumb_sp_relative_load_store_dissassemble(uint16_t opcode)
{
    std::string dst_reg = thumb_get_dst(opcode);

    uint32_t unsigned_offset10 = Utils::get_bits(opcode, 0, 8) << 2;
    bool is_load = Utils::is_bit_set(opcode, 11);

    std::string instruction = (is_load) ? "LDR " : "STR ";
    instruction += dst_reg + ", [SP, ";
    instruction += Utils::int_to_hex(unsigned_offset10);
    instruction += "]";

    return instruction;
}

std::string Arm7Dissassembler::thumb_unconditional_branch_dissassemble(uint16_t opcode)
{
    int32_t signed_extend12 = Utils::sign_extend32(opcode, 0, 10) << 1;

    uint32_t final_addr = (instruction_address + signed_extend12) + 4;

    std::string instruction = "B ";
    instruction += Utils::int_to_hex(final_addr);

    return instruction;
}

std::string Arm7Dissassembler::thumb_undefined_dissassemble(uint16_t opcode)
{
    return "UND";
}