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
        set_cpsr(ProgramStatusRegsiter::C, result < op1 || (result == op1 && c_set()));

        bool op1_msb_set = Utils::is_bit_set(op1, 31);
        bool op2_msb_set = Utils::is_bit_set(op2, 31);
        bool result_msb_set = Utils::is_bit_set(result, 31);
        set_cpsr(ProgramStatusRegsiter::V, op1_msb_set == op2_msb_set && op1_msb_set != result_msb_set);
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

uint32_t Arm7TDMI::alu_sbc(uint32_t op1, uint32_t op2, bool set_cc) 
{ 
    std::cout << "Carry: " << c_set() << '\n';
    uint32_t result = op1 - op2 - !c_set();
    if (set_cc)
    {
        set_negative_and_zero(result);
        set_cpsr(ProgramStatusRegsiter::C, (result < op1) || (result == op1 && c_set()));

        bool op1_msb_set = Utils::is_bit_set(op1, 31);
        bool op2_msb_set = Utils::is_bit_set(op2, 31);
        bool result_msb_set = Utils::is_bit_set(result, 31);
        set_cpsr(ProgramStatusRegsiter::V, result_msb_set != op1_msb_set && op1_msb_set != op2_msb_set);
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
        set_cpsr(ProgramStatusRegsiter::C, result <= op1);

        bool op1_msb_set = Utils::is_bit_set(op1, 31);
        bool op2_msb_set = Utils::is_bit_set(op2, 31);
        bool result_msb_set = Utils::is_bit_set(result, 31);
        set_cpsr(ProgramStatusRegsiter::V, result_msb_set != op1_msb_set && op1_msb_set != op2_msb_set);
    }
    return result;
}

/* Shift Operations */
/// @note For all shift operations, cases where op2 >= 32 are handled at the call site.
uint32_t Arm7TDMI::alu_lsl(uint32_t op1, uint32_t op2, bool set_cc, bool set_carry) // Logical Shift Left
{
    // LSL#0 = no shift applied,
    // the C flag is NOT affected.
    uint32_t result = op1 << op2;
    if (set_cc)
    {
        set_negative_and_zero(result);
        if (op2 != 0 && set_carry) 
            set_cpsr(ProgramStatusRegsiter::C, Utils::is_bit_set(op1, 32 - op2));
    }
    return result;
}

uint32_t Arm7TDMI::alu_lsr(uint32_t op1, uint32_t op2, bool set_cc, bool set_carry) // Logical Shift Right
{
    // LSR#0 is interpreted as LSR#32
    // Op2 becomes zero, C becomes Bit 31 of Rm.
    uint32_t result = (op2 != 0) ? op1 >> op2 : 0;

    if (set_cc)
    {
        set_negative_and_zero(result);

        if (set_carry)
        {
            if (op2 != 0) 
                set_cpsr(ProgramStatusRegsiter::C, Utils::is_bit_set(op1, op2 - 1));
            else   
                set_cpsr(ProgramStatusRegsiter::C, Utils::is_bit_set(op1, 31)); 
        }
    }
    return result;
}

uint32_t Arm7TDMI::alu_asr(uint32_t op1, uint32_t op2, bool set_cc, bool set_carry)
{
    // ASR#0 is interpreted as ASR#32
    // Op2 and C are filled by Bit 31 of Rm.
    bool msb_is_set = Utils::is_bit_set(op1, 31);
    uint32_t result = (op2 != 0) ? (op1 >> op2) : 0;
    result = (msb_is_set) ? 
        (0xFFFFFFFF << (32 - op2)) | result : 
        result;
    if (set_cc)
    {
        set_negative_and_zero(result);
        if (set_carry)
            set_cpsr(ProgramStatusRegsiter::C, Utils::is_bit_set(op1, op2 - 1));
    }
    return result;
}

uint32_t Arm7TDMI::alu_ror(uint32_t op1, uint32_t op2, bool set_cc, bool set_carry)
{
    /* 
        if Rs[7:0] == 0 then
            C Flag = unaffected
            Rd = unaffected
        else if Rs[4:0] == 0 then
            C Flag = Rd[31]
            Rd = unaffected
        else // Rs[4:0] > 0
            C Flag = Rd[Rs[4:0] - 1]
            Rd = Rd Rotate_Right Rs[4:0]
        N Flag = Rd[31]
        Z Flag = if Rd == 0 then 1 else 0
        V Flag = unaffected
    */
    uint32_t result = op1;
    if (Utils::get_bits(op2, 0, 5) != 0 || Utils::get_bits(op2, 0, 8) != 0)
    {
        // std::cout << "Old Val: " << std::bitset<32>(op1) << '\n';

        uint8_t rotate_amount = Utils::get_bits(op2, 0, 5);
        // std::cout << "Rotate Amount: " << +rotate_amount << '\n';
        
        if (set_cc && set_carry)
            set_cpsr(ProgramStatusRegsiter::C, Utils::is_bit_set(op1, rotate_amount - 1));

        uint32_t bits_shifted_out = Utils::get_bits(op1, 0, rotate_amount);
        // std::cout << "Bits Shifted Out: " << std::bitset<32>(bits_shifted_out) << '\n';

        result >>= rotate_amount;
        // std::cout << "Bit Shifted Val: " << result << '\n';

        result |= (bits_shifted_out << (32 - rotate_amount)); 

        // std::cout << "New Val: " << std::bitset<32>(result) << '\n';
    }

    if (set_cc)
        set_negative_and_zero(result);
    
    return result;
}

uint32_t Arm7TDMI::decode_shift_operation(uint32_t op1, uint32_t op2, int shift_type, bool set_cc, bool set_carry)
{
    switch(shift_type)
    {
        case 0: return alu_lsl(op1, op2, set_cc, set_carry);
        case 1: return alu_lsr(op1, op2, set_cc, set_carry);
        case 2: return alu_asr(op1, op2, set_cc, set_carry);
        case 3: return alu_ror(op1, op2, set_cc, set_carry);
        default: throw std::runtime_error("Invalid Shift Opcode: " + std::to_string(shift_type));
    }
    
    return 0;
}

uint32_t Arm7TDMI::alu_mul(uint32_t op1, uint32_t op2, bool set_cc)
{
    /*
        The MUL instruction is defined to leave the C flag unchanged in ARMv5 and above. 
        In earlier versions of the architecture, the value of the C flag was UNPREDICTABLE 
        after a MUL instruction.
        
        I have no clue what NBA does with the carry flag
    */
    skip_mult_instr = true;
    uint32_t result = op1 * op2;
    std::cout << "Mult Result: " << result << '\n';
    if (set_cc)
    {
        set_negative_and_zero(result);
        set_cpsr(ProgramStatusRegsiter::C, result < op1);
    }
    return result;
}