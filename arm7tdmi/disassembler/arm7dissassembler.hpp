#pragma once

#include "../../memory.hpp"
#include "../../utils.hpp"

#include <cstdint>
#include <string>

class Arm7Dissassembler
{
public:
    Arm7Dissassembler(Memory& memory);

    std::string disassemble(uint32_t address, bool is_thumb);

private:
    using ArmFunc = std::string (Arm7Dissassembler::*)(uint32_t opcode);
    using ThumbFunc = std::string (Arm7Dissassembler::*)(uint16_t opcode);

    std::string get_condition_code(uint32_t code);
    std::string decode_bios_function(uint32_t value);

    Memory& memory;

    std::array<ArmFunc, 4096> generate_arm_dissassembler_table();
    std::array<ThumbFunc, 256> generate_thumb_dissassembler_table();

    std::array<ArmFunc, 4096> arm_instr_table = generate_arm_dissassembler_table();
    std::array<ThumbFunc, 256> thumb_instr_table = generate_thumb_dissassembler_table();

    // For calculating branch target addresses
    uint32_t instruction_address = 0;

    std::string arm_branch_dissassemble(uint32_t opcode); // Branch, Branch and Link
    std::string arm_branch_and_exchange_dissassemble(uint32_t opcode);
    std::string arm_block_data_transfer_dissassemble(uint32_t opcode);
    std::string arm_coprocessor_data_operation_dissassemble(uint32_t opcode);
    std::string arm_coprocessor_data_transfer_dissassemble(uint32_t opcode);
    std::string arm_coprocessor_register_transfer_dissassemble(uint32_t opcode);
    std::string arm_data_processing_dissassemble(uint32_t opcode);
    std::string arm_halfword_data_transfer_dissassemble(uint32_t opcode);
    std::string arm_multiply_dissassemble(uint32_t opcode);
    std::string arm_multiply_long_dissassemble(uint32_t opcode);
    std::string arm_psr_transfer_dissassemble(uint32_t opcode);
    std::string arm_software_interrupt_dissassemble(uint32_t opcode);
    std::string arm_single_data_swap_dissassemble(uint32_t opcode);
    std::string arm_single_data_transfer_dissassemble(uint32_t opcode);
    std::string arm_undefined_dissassemble(uint32_t opcode);

    /* THUMB Instructions */
    // I gotta find shorter method names
    std::string thumb_add_subtract_dissassemble(uint16_t opcode);
    std::string thumb_add_offset_sp_dissassemble(uint16_t opcode);
    std::string thumb_alu_operations_dissassemble(uint16_t opcode);
    std::string thumb_conditional_branch_dissassemble(uint16_t opcode);
    std::string thumb_hi_reg_op_branch_exchange_dissassemble(uint16_t opcode);
    std::string thumb_load_address_dissassemble(uint16_t opcode);
    std::string thumb_load_store_halfword_dissassemble(uint16_t opcode);
    std::string thumb_load_store_immediate_dissassemble(uint16_t opcode);
    std::string thumb_load_store_sign_extend_halfword_dissassemble(uint16_t opcode);
    std::string thumb_load_store_w_reg_offset_dissassemble(uint16_t opcode);
    std::string thumb_long_branch_w_link_dissassemble(uint16_t opcode);
    std::string thumb_move_cmp_add_sub_immediate_dissassemble(uint16_t opcode);
    std::string thumb_move_shifted_register_dissassemble(uint16_t opcode);
    std::string thumb_multiple_load_store_dissassemble(uint16_t opcode);
    std::string thumb_pc_relative_load_dissassemble(uint16_t opcode);
    std::string thumb_push_pop_registers_dissassemble(uint16_t opcode);
    std::string thumb_software_interrupt_dissassemble(uint16_t opcode);
    std::string thumb_sp_relative_load_store_dissassemble(uint16_t opcode);
    std::string thumb_unconditional_branch_dissassemble(uint16_t opcode);
    std::string thumb_undefined_dissassemble(uint16_t opcode);

    inline std::pair<std::string, std::string> arm_get_rn_rd(uint32_t opcode)
    {
        int dst_reg_index = Utils::get_bits(opcode, 12, 16);
        int src_reg_index = Utils::get_bits(opcode, 16, 20);

        std::string dst_reg = "R" + std::to_string(dst_reg_index);
        std::string src_reg = "R" + std::to_string(src_reg_index);

        return {src_reg, dst_reg};
    }
    
    // Bits 0-3
    inline std::string arm_get_rm(uint32_t opcode)
    {
        int rm_index = Utils::get_bits(opcode, 0, 4);
        std::string rm = "R" + std::to_string(rm_index);
        return rm;
    }

    // Bits 8-11
    inline std::string arm_get_rs(uint32_t opcode)
    {
        int rs_index = Utils::get_bits(opcode, 8, 12);
        std::string rs = "R" + std::to_string(rs_index);
        return rs;
    }

    inline std::string arm_set_cc(uint32_t opcode)
    {
        bool set_condition_codes = Utils::is_bit_set(opcode, 20);
        return set_condition_codes ? "S" : "";
    }

    /* Thumb Helper Methods */
    inline std::pair<std::string, std::string> thumb_get_dst_src(uint16_t opcode) const
    {
        int dst_reg_index = Utils::get_bits(opcode, 0, 3);
        int src_reg_index = Utils::get_bits(opcode, 3, 6);

        std::string dst_reg = "R" + std::to_string(dst_reg_index);
        std::string src_reg = "R" + std::to_string(src_reg_index);

        return {dst_reg, src_reg};
    }

    inline std::string thumb_get_dst(uint16_t opcode) const
    {
        int dst_reg_index = Utils::get_bits(opcode, 8, 11);
        std::string dst_reg = "R" + std::to_string(dst_reg_index);
        return dst_reg;
    }

    inline std::string encode_shift_operation(int shift_type) const
    {
        switch(shift_type)
        {
            case 0: return "LSL ";
            case 1: return "LSR ";
            case 2: return "ASR ";
            case 3: return "ROR ";
            default: break;
        }

        return "";
    }

    inline std::string compute_ror(uint32_t op, int shift_amount)
    {
        uint32_t result = op;
        uint32_t rotate_amount = Utils::get_bits(shift_amount, 0, 5);
        uint32_t bits_shifted_out = Utils::get_bits(op, 0, rotate_amount);
        result >>= rotate_amount;
        result |= (bits_shifted_out << (32 - rotate_amount));

        return Utils::int_to_hex(result);
    }
};