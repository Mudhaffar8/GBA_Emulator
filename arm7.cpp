#include "arm7.hpp"

#include <stdexcept>

Arm7TDMI::Arm7TDMI(Memory& _memory) : 
    memory(_memory)
{}


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

void Arm7TDMI::handle_mode_switch(CpuMode new_mode)
{
    Utils::set_bits(cpsr, new_mode, true);

    switch(new_mode)
    {
    case CpuMode::User:
    case CpuMode::System:
        registers[8] = &r8;
        registers[9] = &r9;
        registers[10] = &r10;
        registers[11] = &r11;
        registers[12] = &r12;
        registers[13] = &r13;
        registers[14] = &r14;
        break;

    case CpuMode::FastInterrupt:
        registers[8] = &r8_fiq;
        registers[9] = &r9_fiq;
        registers[10] = &r10_fiq;
        registers[11] = &r11_fiq;
        registers[12] = &r12_fiq;
        registers[13] = &r13_fiq;
        registers[14] = &r14_fiq;
        spsr_fiq = cpsr;
        break;

    case CpuMode::InterruptRequest:
        registers[13] = &r13_fiq;
        registers[14] = &r14_irq;
        spsr_irq = cpsr;
        break;

    case CpuMode::Supervisor:
        registers[13] = &r13_svc;
        registers[14] = &r14_svc;
        spsr_svc = cpsr;
        break;

    case CpuMode::Abort:
        registers[13] = &r13_abt;
        registers[14] = &r14_abt;
        spsr_abt = cpsr;
        break;

    case CpuMode::Undefined:
        registers[13] = &r13_und;
        registers[14] = &r14_und;
        spsr_und = cpsr;
        break;

    default:
        std::runtime_error("Invalid Mode: " + +new_mode);
        break;
    }
}

void Arm7TDMI::handle_state_switch(CpuState new_state)
{
    state = new_state;
    cpsr &= ~new_state;
    cpsr |= new_state;
}

void Arm7TDMI::branch_and_exchange(uint32_t address)
{
    if (address & 1)
    {
        pc = address & ~1;
        handle_state_switch(CpuState::Thumb);
    }
    else
    {
        pc = address & ~3;
        handle_state_switch(CpuState::Arm);
    }
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
    return table;
}