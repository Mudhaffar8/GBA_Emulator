#include "arm7.hpp"

#include <cmath>

// ADD Fully Tested
uint32_t Arm7TDMI::alu_add_cmn(uint32_t op1, uint32_t op2, bool set_cc)
{
    uint32_t result = op1 + op2;

    if (set_cc)
    {
        set_negative_and_zero(result);
        set_cpsr(ProgramStatusRegsiter::C, result < op1);

        bool op1_msb_set = Utils::is_bit_set(op1, 31);
        bool op2_msb_set = Utils::is_bit_set(op2, 31);
        bool result_msb_set = Utils::is_bit_set(result, 31);
        set_cpsr(ProgramStatusRegsiter::V, op1_msb_set == op2_msb_set && op1_msb_set != result_msb_set);
    }

    return result;
}

uint32_t Arm7TDMI::alu_adc(uint32_t op1, uint32_t op2, bool set_cc)
{
    uint32_t result = op1 + op2 + c_set();

    if (set_cc)
    {
        set_negative_and_zero(result);
        set_cpsr(ProgramStatusRegsiter::C, (op1 + op2 + c_set()) > std::numeric_limits<uint32_t>().max());
        set_cpsr(ProgramStatusRegsiter::V, (op1 + op2 + c_set()) > std::numeric_limits<int>().max());
    }

    return result;
}

uint32_t Arm7TDMI::alu_and_tst(uint32_t op1, uint32_t op2, bool set_cc)
{
    uint32_t result = op1 & op2;

    if (set_cc)
    {
        set_negative_and_zero(result);
        // C Flag based on shifter carry out
    }

    return result;
}

uint32_t Arm7TDMI::alu_bic(uint32_t op1, uint32_t op2, bool set_cc)
{
    uint32_t result = op1 & ~op2;
    
    if (set_cc)
        set_negative_and_zero(result);
        // C Flag based on shifter carry out

    return result;
}

// Exclusive OR
uint32_t Arm7TDMI::alu_eor_teq(uint32_t op1, uint32_t op2, bool set_cc) 
{ 
    uint32_t result = op1 ^ op2;
    if (set_cc)
        set_negative_and_zero(result);
        // C Flag based on shifter carry out

    return result;
} 

// There may be an edge case w this instruction when negative
uint32_t Arm7TDMI::alu_mov(uint32_t op2, bool set_cc) 
{ 
    if (set_cc)
        set_negative_and_zero(op2);

    return op2; 
}

uint32_t Arm7TDMI::alu_mvn(uint32_t op2, bool set_cc) 
{ 
    uint32_t result = ~op2;
    if (set_cc) 
        set_negative_and_zero(result);
        // C Flag based on shifter carry out

    return result;
}

uint32_t Arm7TDMI::alu_orr(uint32_t op1, uint32_t op2, bool set_cc) 
{ 
    uint32_t result = op1 | op2;
    if (set_cc)
        set_negative_and_zero(result);
        // C Flag based on shifter carry out

    return result;
}

uint32_t Arm7TDMI::alu_rsb(uint32_t op1, uint32_t op2, bool set_cc) 
{ 
    uint32_t result = op2 - op1;
    if (set_cc)
    {
        set_negative_and_zero(result);
        set_cpsr(ProgramStatusRegsiter::C, op2 > op1);
        set_cpsr(ProgramStatusRegsiter::V, (op2 - op1) < std::numeric_limits<int>().min());
    }
    return result;
}

uint32_t Arm7TDMI::alu_rsc(uint32_t op1, uint32_t op2, bool set_cc) 
{ 
    uint32_t result = op2 - op1 - !c_set();
    if (set_cc)
    {
        set_negative_and_zero(result);
        set_cpsr(ProgramStatusRegsiter::C, op2 - !c_set() > op1);
        set_cpsr(ProgramStatusRegsiter::V, (op1 - op2 - !c_set()) < std::numeric_limits<int>().min());
    }
    return result;
}

uint32_t Arm7TDMI::alu_sbc(uint32_t op1, uint32_t op2, bool set_cc) 
{ 
    uint32_t result = op1 - op2 - !c_set();
    if (set_cc)
    {
        set_negative_and_zero(result);
        set_cpsr(ProgramStatusRegsiter::C, op1 - !c_set() > op2);
        set_cpsr(ProgramStatusRegsiter::V, (op1 - op2 - !c_set()) < std::numeric_limits<int>().min());
    }
    return result;
}

// SUB fully Tested
uint32_t Arm7TDMI::alu_sub_cmp(uint32_t op1, uint32_t op2, bool set_cc) 
{ 
    uint32_t result = op1 - op2;
    if (set_cc)
    {
        set_negative_and_zero(result);
        set_cpsr(ProgramStatusRegsiter::C, op1 >= op2);

        bool op1_msb_set = Utils::is_bit_set(op1, 31);
        bool op2_msb_set = Utils::is_bit_set(op2, 31);
        bool result_msb_set = Utils::is_bit_set(result, 31);
        set_cpsr(ProgramStatusRegsiter::V, result_msb_set != op1_msb_set && op1_msb_set != op2_msb_set);
    }
    return result;
}

/* Shift Operations */
uint32_t Arm7TDMI::alu_lsl(uint32_t op1, uint32_t op2, bool set_cc) // Logical Shift Left
{
    // LSR#0 = no shift applied,
    // the C flag is NOT affected.
    uint32_t result = op1 << op2;
    if (set_cc)
    {
        set_negative_and_zero(result);
        if (op2 != 0) 
            set_cpsr(ProgramStatusRegsiter::C, op1 > op2);
    }
    return result;
}

uint32_t Arm7TDMI::alu_lsr(uint32_t op1, uint32_t op2, bool set_cc) // Logical Shift Right
{
    // LSR#0 is interpreted as LSR#32
    // Op2 becomes zero, C becomes Bit 31 of Rm.
    uint32_t result = op1 >> (op2 == 0 ? 32 : op2);
    if (set_cc)
    {
        set_negative_and_zero(result);
    }
    return result;
}

uint32_t Arm7TDMI::alu_asr(uint32_t op1, uint32_t op2, bool set_cc)
{
    // ASR#0 is interpreted as ASR#32
    // Op2 and C are filled by Bit 31 of Rm.
    op2 = (op2 == 0) ? 32 : op2;
    uint32_t result = op1 / (2 << op2);
    if (set_cc)
    {
        set_negative_and_zero(result);
    }
    return result;
}

uint32_t Arm7TDMI::alu_ror(uint32_t op1, uint32_t op2, bool set_cc)
{
    // ROR#0 interpreted as RRX#1 (RCR), 
    // like ROR#1, but Op1 Bit 31 set to old C.
    uint32_t result = op1;
    if (op2 == 0)
    {
        result >>= 1;
        result |= (c_set() << 31);
    }
    else
    {
        uint32_t bits_shifted_out = Utils::get_bits(result, 0, op2 + 1);
        result >>= op2;
        result |= (bits_shifted_out << (32 - op2)); // Check the math here
    }
    if (set_cc)
        set_negative_and_zero(result);
    
    return result;
}

uint32_t Arm7TDMI::decode_shift_operation(uint32_t op1, uint32_t op2, int shift_type)
{
    switch(shift_type)
    {
    case 0: return alu_lsl(op1, op2, true);
    case 1: return alu_lsr(op1, op2, true);
    case 2: return alu_asr(op1, op2, true);
    case 3: return alu_ror(op1, op2, true);
    default: std::runtime_error("Invalid Shift Opcode: " + shift_type);
    }
    
    return 0;
}

uint32_t Arm7TDMI::alu_mul(uint32_t op1, uint32_t op2, bool set_cc)
{
    uint32_t result = op1 * op2;
    if (set_cc)
        set_negative_and_zero(result);
    return result;
}