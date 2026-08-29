#include "arm7dissassembler.hpp"
#include "../arm7.hpp"

Arm7Dissassembler::Arm7Dissassembler(Memory& _memory) :
    memory(_memory)
{}

std::string Arm7Dissassembler::disassemble(uint32_t address, bool is_thumb)
{
    std::string instruction = "Address " + Utils::int_to_hex(address) + ": ";
    
    instruction_address = address;

    if (is_thumb)
    {
        uint16_t half_word = memory.read<uint16_t>(address);
        instruction += (this->*thumb_instr_table[half_word >> 8])(half_word);
    }
    else
    {
        uint32_t word = memory.read<uint32_t>(address);
        int index = (Utils::get_bits(word, 20, 28) << 4) | Utils::get_bits(word, 4, 8);
        instruction += (this->*arm_instr_table[index])(word);
    }
    
    return instruction;
}

std::string Arm7Dissassembler::get_condition_code(uint32_t opcode)
{
    int code = Utils::get_bits(opcode, 28, 32);
    switch(code)
    {
        case Arm7TDMI::ConditionCode::EQ: return "EQ";
        case Arm7TDMI::ConditionCode::NE: return "NE";
        case Arm7TDMI::ConditionCode::CS: return "CS";
        case Arm7TDMI::ConditionCode::CC: return "CC";
        case Arm7TDMI::ConditionCode::MI: return "MI";
        case Arm7TDMI::ConditionCode::PL: return "PL";
        case Arm7TDMI::ConditionCode::VS: return "VS";
        case Arm7TDMI::ConditionCode::VC: return "VC";
        case Arm7TDMI::ConditionCode::HI: return "HI";
        case Arm7TDMI::ConditionCode::LS: return "LS";
        case Arm7TDMI::ConditionCode::GE: return "GE";
        case Arm7TDMI::ConditionCode::LT: return "LT";
        case Arm7TDMI::ConditionCode::GT: return "GT";
        case Arm7TDMI::ConditionCode::LE: return "LE";
        case Arm7TDMI::ConditionCode::AL: return "";
        default: return "NV"; // Forbidden condition code
    }

    return "";
}

std::string Arm7Dissassembler::decode_bios_function(uint32_t value)
{
    switch (value)
    {
        // Basic Functions
        case 0x00: return "SoftReset";
        case 0x01: return "RegisterRamReset";
        case 0x02: return "Halt";
        case 0x03: return "Stop";
        case 0x04: return "IntrWait";
        case 0x05: return "VBlankIntrWait";
        case 0x06: return "Div";
        case 0x07: return "DivArm";
        case 0x08: return "Sqrt";
        case 0x09: return "ArcTan";
        case 0x0A: return "ArcTan2";
        case 0x0B: return "CpuSet";
        case 0x0C: return "CpuFastSet";
        case 0x0D: return "GetBiosChecksum";
        case 0x0E: return "BgAffineSet";
        case 0x0F: return "ObjAffineSet";

        // Decompression Functions
        case 0x10: return "BitUnpack";
        case 0x11: return "LZ77UnCompReadNormalWrite8bit";
        case 0x12: return "LZ77UnCompReadNormalWrite16bit";
        case 0x13: return "HuffUnCompReadNormal";
        case 0x14: return "RLUnCompReadNormalWrite8bit";
        case 0x15: return "RLUnCompReadNormalWrite16bit";
        case 0x16: return "Diff8bitUnFilterWrite8bit";
        case 0x17: return "Diff8bitUnFilterWrite16bit";
        case 0x18: return "Diff16bitUnFilter";

        // Sound Functions
        case 0x19: return "SoundBias";
        case 0x1A: return "SoundDriverInit";
        case 0x1B: return "SoundDriverMode";
        case 0x1C: return "SoundDriverMain";
        case 0x1D: return "SoundDriverVSync";
        case 0x1E: return "SoundChannelClear";
        case 0x1F: return "MidiKey2Freq";
        case 0x20: return "SoundWhatever0";
        case 0x21: return "SoundWhatever1";
        case 0x22: return "SoundWhatever2";
        case 0x23: return "SoundWhatever3";
        case 0x24: return "SoundWhatever4";
        case 0x25: return "MultiBoot";
        case 0x26: return "HardReset";
        case 0x27: return "CustomHalt";
        case 0x28: return "SoundDriverVSyncOff";
        case 0x29: return "SoundDriverVSyncOn";
        case 0x2A: return "SoundGetJumpList";

        default: return "Uknown (" + std::to_string(value) + ")";
    }
    return "";
}

std::array<Arm7Dissassembler::ArmFunc, 4096> Arm7Dissassembler::generate_arm_dissassembler_table()
{
    // Bits 20-27 and bits 4-7 should be enough to decode the instruction
    std::array<ArmFunc, 4096> table{};
    for (int i = 0; i < 4096; ++i)
    {
        int bits_25_to_27 = Utils::get_bits(i, 9, 12);
        int bits_20_to_24 = Utils::get_bits(i, 4, 9);
        int bits_4_to_7 = Utils::get_bits(i, 0, 4);

        switch(bits_25_to_27)
        {
        case 0b000:
            // Rd in data Processing cannot be 15
            // Rd == 15 can be an easy way to differentiate from 
            // Data Processing and MSR

            // Filters out multply and multiply long
            if (bits_20_to_24 & 0b10000)
            {
                if ((bits_20_to_24 & 0b11011) == 0b10000 && bits_4_to_7 == 0b1001)
                    table[i] = &arm_single_data_swap_dissassemble;
                else if (bits_20_to_24 == 0b10010 && bits_4_to_7 == 0b0001)
                    table[i] = &arm_branch_and_exchange_dissassemble;
                else if ((bits_4_to_7 & 0b1001) == 0b1001)
                    table[i] = &arm_halfword_data_transfer_dissassemble;
                else if ((bits_20_to_24 & 0b11001) == 0b10000)
                    table[i] = &arm_psr_transfer_dissassemble;
                else  
                    table[i] = &arm_data_processing_dissassemble;
            }
            else
            {
                if ((bits_20_to_24 & 0b11100) == 0 && bits_4_to_7 == 0b1001)
                    table[i] = &arm_multiply_dissassemble;
                else if ((bits_20_to_24 & 0b11000) == 0b01000 && bits_4_to_7 == 0b1001)
                    table[i] = &arm_multiply_long_dissassemble;
                else if ((bits_4_to_7 & 0b1001) == 0b1001)
                    table[i] = &arm_halfword_data_transfer_dissassemble;
                else 
                    table[i] = &arm_data_processing_dissassemble;
            }
            break;

        case 0b001:
            table[i] = ((bits_20_to_24 & 0b11001) == 0b10000) ? 
                &arm_psr_transfer_dissassemble : 
                &arm_data_processing_dissassemble;
            break;

        case 0b010:
            table[i] = &arm_single_data_transfer_dissassemble;
            break;

        case 0b011:
            table[i] = (bits_4_to_7 & 1) ? 
                &arm_undefined_dissassemble : 
                &arm_single_data_transfer_dissassemble;
            break;

        case 0b100:
            table[i] = &arm_block_data_transfer_dissassemble; 
            break;
        
        case 0b101:
            table[i] = &arm_branch_dissassemble;
            break;
        
        case 0b110:
            table[i] = &arm_coprocessor_data_transfer_dissassemble;
            break;

        case 0b111:
            if (bits_20_to_24 & 0b10000)
                table[i] = &arm_software_interrupt_dissassemble;
            else
            {
                table[i] = (bits_4_to_7 & 1) ?
                    &arm_coprocessor_register_transfer_dissassemble :
                    &arm_coprocessor_data_operation_dissassemble;
            }
            break;
        }
    }
    return table;
}

std::array<Arm7Dissassembler::ThumbFunc, 256> Arm7Dissassembler::generate_thumb_dissassembler_table()
{
    // It seems only bits 8-15 are needed to decode the instruction
    std::array<ThumbFunc, 256> table{};

    for (int i = 0; i < 256; ++i)
    {
        int last_three_bits = (i & 0xE0) >> 5;
        
        switch(last_three_bits)
        {
        case 0:
            table[i] = (Utils::get_bits(i, 3, 5) == 0b11) ? 
                &thumb_add_subtract_dissassemble : 
                &thumb_move_shifted_register_dissassemble;
            break;

        case 0b001:
            table[i] = &thumb_move_cmp_add_sub_immediate_dissassemble;
            break;
        
        case 0b010:
            if (Utils::get_bits(i, 2, 5) == 0)
                table[i] = &thumb_alu_operations_dissassemble;

            else if (Utils::get_bits(i, 2, 5) == 0b001)
                table[i] = &thumb_hi_reg_op_branch_exchange_dissassemble;

            else if (Utils::get_bits(i, 3, 5) == 0b01)
                table[i] = &thumb_pc_relative_load_dissassemble;

            else if (Utils::is_bit_set(i, 1) && Utils::is_bit_set(i, 4))
                table[i] = &thumb_load_store_sign_extend_halfword_dissassemble;

            else    
                table[i] = &thumb_load_store_w_reg_offset_dissassemble;
            break;

        case 0b011:
            table[i] = &thumb_load_store_immediate_dissassemble;
            break;

        case 0b100:
            table[i] = (Utils::is_bit_set(i, 4)) ? 
                &thumb_sp_relative_load_store_dissassemble : 
                &thumb_load_store_halfword_dissassemble;
            break;
    
        case 0b101:
            if (!Utils::is_bit_set(i, 4))
                table[i] = &thumb_load_address_dissassemble;
            else if (Utils::get_bits(i, 0, 4) == 0b0000)
                table[i] = &thumb_add_offset_sp_dissassemble;
            else if (Utils::get_bits(i, 1, 3) == 0b10)
                table[i] = &thumb_push_pop_registers_dissassemble;
            else 
                table[i] = &thumb_undefined_dissassemble;
            break;

        // 1011'0001

        case 0b110:
            if (!Utils::is_bit_set(i, 4))
                table[i] = &thumb_multiple_load_store_dissassemble;
            else if (i == 0b11011111)
                table[i] = &thumb_software_interrupt_dissassemble;
            else 
                table[i] = &thumb_conditional_branch_dissassemble;
            break;

        case 0b111:
            if (Utils::get_bits(i, 3, 5) == 0b00)
                table[i] = &thumb_unconditional_branch_dissassemble;

            else if (Utils::is_bit_set(i, 4))
                table[i] = &thumb_long_branch_w_link_dissassemble;
    
            else 
                table[i] = &thumb_undefined_dissassemble;
            break;
        }
    }
    return table;
}