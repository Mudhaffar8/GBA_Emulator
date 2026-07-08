#include "arm7.hpp"

#include <bitset>
#include <iostream>
#include <stdexcept>

Arm7TDMI::Arm7TDMI(FakeMemory& _memory) : 
    memory(_memory)
{}

void Arm7TDMI::test()
{
    for (uint16_t op = 0; op < 256; ++op)
    {
        std::cout << "Opcode: " << std::bitset<16>(op << 8) << ", ";
        thumb_execute(op << 8);
    }
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
        default: std::runtime_error("Invalid Condition Code: " + +code);
    }

    return false;
}

void Arm7TDMI::handle_mode_switch(uint32_t new_mode)
{
    cpsr &= ~ProgramStatusRegsiter::Mode;
    cpsr |= new_mode;

    std::cout << "New CPSR: " << std::bitset<32>(cpsr) << '\n';

    switch(new_mode)
    {
    case CpuMode::User:
    case CpuMode::System:
        std::cout << "USER/SYSTEM\n";
        registers[8] = &r8;
        registers[9] = &r9;
        registers[10] = &r10;
        registers[11] = &r11;
        registers[12] = &r12;
        registers[13] = &r13;
        registers[14] = &r14;
        break;

    case CpuMode::FastInterrupt:
        std::cout << "FIQ\n";
        registers[8] = &r8_fiq;
        registers[9] = &r9_fiq;
        registers[10] = &r10_fiq;
        registers[11] = &r11_fiq;
        registers[12] = &r12_fiq;
        registers[13] = &r13_fiq;
        registers[14] = &r14_fiq;
        break;

    case CpuMode::InterruptRequest:
        std::cout << "IRQ\n";
        registers[8] = &r8;
        registers[9] = &r9;
        registers[10] = &r10;
        registers[11] = &r11;
        registers[12] = &r12;
        registers[13] = &r13_irq;
        registers[14] = &r14_irq;
        spsr_irq = old_cpsr; 
        break;

    case CpuMode::Supervisor:
        std::cout << "SVC\n";
        registers[8] = &r8;
        registers[9] = &r9;
        registers[10] = &r10;
        registers[11] = &r11;
        registers[12] = &r12;
        registers[13] = &r13_svc;
        registers[14] = &r14_svc;
        spsr_svc = old_cpsr; 
        break;

    case CpuMode::Abort:
        std::cout << "ABT\n";
        registers[8] = &r8;
        registers[9] = &r9;
        registers[10] = &r10;
        registers[11] = &r11;
        registers[12] = &r12;
        registers[13] = &r13_abt;
        registers[14] = &r14_abt;
        spsr_abt = old_cpsr; 
        break;

    case CpuMode::Undefined:
        std::cout << "UND\n";
        registers[8] = &r8;
        registers[9] = &r9;
        registers[10] = &r10;
        registers[11] = &r11;
        registers[12] = &r12;
        registers[13] = &r13_und;
        registers[14] = &r14_und;
        spsr_und = old_cpsr; 
        break;

    default:
        std::runtime_error("Invalid Mode: " + +new_mode);
        break;
    }
}

void Arm7TDMI::handle_state_switch(CpuState new_state)
{
    state = new_state;

    cpsr &= ~ProgramStatusRegsiter::T;
    cpsr |= new_state;
}

void Arm7TDMI::branch_and_exchange(uint32_t address)
{
    std::cout << "Old PC: " << pc << '\n';
    std::cout << "New Branch Address: " << address << '\n';
    std::cout << "Lower 2 bits: " << (address & 3) << '\n';

    uint32_t new_address = Utils::get_bits(address, 1, 32) << 1;
    if (address & 1)
    {
        pc = new_address + 4;
        handle_state_switch(CpuState::Thumb);
    }
    else
    {
        // 0b10 is unpredictable behaviour
        pc = new_address + 8;
        handle_state_switch(CpuState::Arm);
    }

    is_branched = true;
}

/* */
void Arm7TDMI::arm_execute(uint32_t opcode)
{}

void Arm7TDMI::thumb_execute(uint16_t opcode)
{
    is_branched = false;
    (this->*thumb_instr_table[opcode >> 8])(opcode);
    if (!is_branched)
        pc += 2;
}

/* Instruction Decoding */
std::array<Arm7TDMI::ArmFunc, 4096> Arm7TDMI::generate_arm_table()
{
    // Bits 20-27 and bits 4-7 should be enough to decode the instruction
    std::array<ArmFunc, 4096> table{};
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