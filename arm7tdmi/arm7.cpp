#include "arm7.hpp"

#include <bitset>
#include <iostream>
#include <stdexcept>

#include "../memory_regions.hpp"

#ifndef RUN_JSON_TESTS
Arm7TDMI::Arm7TDMI(Memory& _memory) : 
    memory(_memory),
    interrupt_enable(_memory),
    interrupt_flag(_memory),
    ime(_memory)
{
    // In normal GBAs, the FIQ signal is shortcut to VDD35, ie. the signal is always high, 
    // and there is no way to generate a FIQ by hardware.
    r13 = 0x03007F00;
    r13_irq = 0x03007FA0;
    r13_svc = 0x03007FE0;
    r15 = GBACart::ENTRY_POINT + 8;
    cpsr |= ProgramStatusRegsiter::F | ArmMode::User;
}
#else
Arm7TDMI::Arm7TDMI(TestMemory& _memory) : 
    memory(_memory)
{
    // In normal GBAs, the FIQ signal is shortcut to VDD35, ie. the signal is always high, 
    // and there is no way to generate a FIQ by hardware.
    pc = GBACart::ENTRY_POINT + 8;
    cpsr |= ProgramStatusRegsiter::F | ArmMode::System;
}
#endif

void Arm7TDMI::tick()
{
    #ifndef RUN_JSON_TESTS
    if (is_irq_enabled() && (interrupt_enable & interrupt_flag & 0x3FFF) != 0 && (ime & 1))
    {
        handle_irq();
        return;
    }
    #endif
    
    if (is_thumb_mode())
    {
        uint16_t half_word = memory.read<uint16_t>(r15 - 4);
        thumb_execute(half_word);
    }
    else
    {
        uint32_t word = memory.read<uint32_t>(r15 - 8);
        arm_execute(word);
    }
}

void Arm7TDMI::arm_execute(uint32_t opcode)
{
    is_branched = false;

    uint32_t condition_code = Utils::get_bits(opcode, 28, 32);
    if (check_condition_code(condition_code))
    {
        int index = (Utils::get_bits(opcode, 20, 28) << 4) | Utils::get_bits(opcode, 4, 8);

        auto next_pc_fetch = (this->*arm_instr_table[index])(opcode);
        (void)memory.read<uint32_t>(r15, next_pc_fetch); // Really contemplating modeling the pipeline
    }

    if (!is_branched) r15 += 4;
}

void Arm7TDMI::thumb_execute(uint16_t opcode)
{
    is_branched = false;

    auto next_pc_fetch = (this->*thumb_instr_table[opcode >> 8])(opcode);
    (void)memory.read<uint16_t>(r15, next_pc_fetch); // At this point I might as well model the pipeline

    if (!is_branched) r15 += 2;
}

bool Arm7TDMI::check_condition_code(uint32_t code)
{
    switch(code)
    {
        case ConditionCode::EQ: return z_set();
        case ConditionCode::NE: return !z_set();
        case ConditionCode::CS: return c_set();
        case ConditionCode::CC: return !c_set();
        case ConditionCode::MI: return n_set();
        case ConditionCode::PL: return !n_set();
        case ConditionCode::VS: return v_set();
        case ConditionCode::VC: return !v_set();
        case ConditionCode::HI: return c_set() && !z_set();
        case ConditionCode::LS: return !c_set() || z_set();
        case ConditionCode::GE: return !(n_set() ^ v_set());
        case ConditionCode::LT: return n_set() ^ v_set();
        case ConditionCode::GT: return !z_set() && !(n_set() ^ v_set());
        case ConditionCode::LE: return z_set() || (n_set() ^ v_set());
        case ConditionCode::AL: return true;
        default: throw std::runtime_error("Invalid Condition Code: " + std::to_string(code));
    }

    return false;
}

/// @note Any method that changes the cpsr flags should call this function.
void Arm7TDMI::handle_mode_switch(uint32_t new_mode)
{
    // If this was C++20, I'd mark this as likely
    if (new_mode != ArmMode::FastInterrupt) 
    {
        registers[8] = &r8;
        registers[9] = &r9;
        registers[10] = &r10;
        registers[11] = &r11;
        registers[12] = &r12;
    }

    cpsr &= ~ProgramStatusRegsiter::Mode;
    cpsr |= new_mode;

    Utils::log("New CPSR", std::bitset<32>(cpsr));
    Utils::print("Mode Switch: ");

    switch(new_mode)
    {
    case ArmMode::User:
    case ArmMode::System:
        Utils::print("USER/SYSTEM\n");
        registers[13] = &r13;
        registers[14] = &r14;
        break;

    case ArmMode::FastInterrupt:
        Utils::print("FIQ\n");
        registers[8] = &r8_fiq;
        registers[9] = &r9_fiq;
        registers[10] = &r10_fiq;
        registers[11] = &r11_fiq;
        registers[12] = &r12_fiq;
        registers[13] = &r13_fiq;
        registers[14] = &r14_fiq;
        break;

    case ArmMode::InterruptRequest:
        Utils::print("IRQ\n");
        registers[13] = &r13_irq;
        registers[14] = &r14_irq;
        break;

    case ArmMode::Supervisor:
        Utils::print("SVC\n");
        registers[13] = &r13_svc;
        registers[14] = &r14_svc;
        break;

    case ArmMode::Abort:
        Utils::print("ABT\n");
        registers[13] = &r13_abt;
        registers[14] = &r14_abt;
        break;

    case ArmMode::Undefined:
        Utils::print("UND\n");
        registers[13] = &r13_und;
        registers[14] = &r14_und;
        break;

    default:
        // MSR Transfer Tests sets the PSR to illegal modes so leave this commented for now
        // throw std::runtime_error("Invalid Mode: " + std::to_string(new_mode));
        break;
    }
}

uint32_t& Arm7TDMI::get_mode_spsr(uint32_t mode)
{
    switch(mode)
    {
        case ArmMode::User:
        case ArmMode::System:
            return cpsr;
        
        case ArmMode::Abort: return spsr_abt;
        case ArmMode::Supervisor: return spsr_svc;
        case ArmMode::FastInterrupt: return spsr_fiq;
        case ArmMode::InterruptRequest: return spsr_irq;
        case ArmMode::Undefined: return spsr_und;
    }

    assert(false && "Invalid SPSR");
    return cpsr;
}

void Arm7TDMI::handle_state_switch(uint32_t new_state)
{
    cpsr &= ~ProgramStatusRegsiter::T;
    cpsr |= new_state;
}

void Arm7TDMI::branch_and_exchange(uint32_t address)
{
    uint32_t new_address = Utils::get_bits(address, 1, 32) << 1;
    if (address & 1)
    {
        handle_state_switch(ArmState::Thumb); 
        reload_pipeline16(new_address + 4);
    }
    else
    {
        // 0b10 is unpredictable behaviour
        handle_state_switch(ArmState::Arm);
        reload_pipeline32(new_address + 8);
    }
}

/// @note This might be 1S cycle off.
void Arm7TDMI::handle_irq()
{
    #ifndef RUN_JSON_TESTS
    std::cout << "IRQ Triggered! " << std::bitset<16>(interrupt_flag) << '\n';
    #endif
    
    spsr_irq = cpsr;

    handle_state_switch(ArmState::Arm);
    handle_mode_switch(ArmMode::InterruptRequest);
    set_cpsr(ProgramStatusRegsiter::I, true);
     
    r14_irq = r15 - (is_thumb_mode() ? 2 : 4);
    reload_pipeline32(Arm7VectorAddr::IRQ + 8);
}

/* Instruction Decoding */
std::array<Arm7TDMI::ArmFunc, 4096> Arm7TDMI::generate_arm_table()
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
                    table[i] = &arm_single_data_swap;
                else if (bits_20_to_24 == 0b10010 && bits_4_to_7 == 0b0001)
                    table[i] = &arm_branch_and_exchange;
                else if ((bits_4_to_7 & 0b1001) == 0b1001)
                    table[i] = &arm_halfword_data_transfer;
                else if ((bits_20_to_24 & 0b11001) == 0b10000)
                    table[i] = &arm_psr_transfer;
                else  
                    table[i] = &arm_data_processing;
            }
            else
            {
                if ((bits_20_to_24 & 0b11100) == 0 && bits_4_to_7 == 0b1001)
                    table[i] = &arm_multiply;
                else if ((bits_20_to_24 & 0b11000) == 0b01000 && bits_4_to_7 == 0b1001)
                    table[i] = &arm_multiply_long;
                else if ((bits_4_to_7 & 0b1001) == 0b1001)
                    table[i] = &arm_halfword_data_transfer;
                // When bits[27:24] == 0b000, then register w/ shift
                // Either bit 4 is set and bit 7 is unset or bit 4 is unset for data processing
                else 
                    table[i] = &arm_data_processing;
            }
            break;

        case 0b001:
            table[i] = ((bits_20_to_24 & 0b11001) == 0b10000) ? 
                &arm_psr_transfer : 
                &arm_data_processing;
            break;

        case 0b010:
            table[i] = &arm_single_data_transfer;
            break;

        case 0b011:
            table[i] = (bits_4_to_7 & 1) ? 
                &arm_undefined : 
                &arm_single_data_transfer;
            break;

        case 0b100:
            table[i] = &arm_block_data_transfer; 
            break;
        
        case 0b101:
            table[i] = &arm_branch;
            break;
        
        case 0b110:
            table[i] = &arm_coprocessor_data_transfer;
            break;

        case 0b111:
            if (bits_20_to_24 & 0b10000)
                table[i] = &arm_software_interrupt;
            else
            {
                table[i] = (bits_4_to_7 & 1) ?
                    &arm_coprocessor_register_transfer :
                    &arm_coprocessor_data_operation;
            }
            break;
        }
    }
    return table;
}

std::array<Arm7TDMI::ThumbFunc, 256> Arm7TDMI::generate_thumb_table()
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
                &thumb_add_subtract : 
                &thumb_move_shifted_register;
            break;

        case 0b001:
            table[i] = &thumb_move_cmp_add_sub_immediate;
            break;
        
        case 0b010:
            if (Utils::get_bits(i, 2, 5) == 0)
                table[i] = &thumb_alu_operations;

            else if (Utils::get_bits(i, 2, 5) == 0b001)
                table[i] = &thumb_hi_reg_op_branch_exchange;

            else if (Utils::get_bits(i, 3, 5) == 0b01)
                table[i] = &thumb_pc_relative_load;

            else if (Utils::is_bit_set(i, 1) && Utils::is_bit_set(i, 4))
                table[i] = &thumb_load_store_sign_extend_halfword;

            else    
                table[i] = &thumb_load_store_w_reg_offset;
            break;

        case 0b011:
            table[i] = &thumb_load_store_immediate;
            break;

        case 0b100:
            table[i] = (Utils::is_bit_set(i, 4)) ? 
                &thumb_sp_relative_load_store : 
                &thumb_load_store_halfword;
            break;
    
        case 0b101:
            if (!Utils::is_bit_set(i, 4))
                table[i] = &thumb_load_address;
            else if (Utils::get_bits(i, 0, 4) == 0b0000)
                table[i] = &thumb_add_offset_sp;
            else if (Utils::get_bits(i, 1, 3) == 0b10)
                table[i] = &thumb_push_pop_registers;
            else 
                table[i] = &thumb_undefined;
            break;

        // 1011'0001

        case 0b110:
            if (!Utils::is_bit_set(i, 4))
                table[i] = &thumb_multiple_load_store;
            else if (i == 0b11011111)
                table[i] = &thumb_software_interrupt;
            else 
                table[i] = &thumb_conditional_branch;
            break;

        case 0b111:
            if (Utils::get_bits(i, 3, 5) == 0b00)
                table[i] = &thumb_unconditional_branch;

            else if (Utils::is_bit_set(i, 4))
                table[i] = &thumb_long_branch_w_link;
    
            else 
                table[i] = &thumb_undefined;
            break;
        }
    }
    return table;
}