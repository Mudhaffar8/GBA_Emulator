#pragma once

#include <cstdint>
#include <bitset>

namespace Utils
{
    constexpr uint32_t MSB32 = 0x80000000;
    
    inline bool is_bit_set(uint32_t byte, uint32_t bit_to_check)
    {   
        return (byte >> bit_to_check) & 1;
    }

    inline uint32_t get_bits(uint32_t byte, int bit_start, int bit_end)
    {
        return (byte >> bit_start) & ((1 << (bit_end - bit_start)) - 1);
    }

    constexpr void set_bit(uint32_t& byte, uint32_t bit_to_check, bool cond)
    {
        if (cond) 
            (byte | bit_to_check);
        else 
            (byte & ~bit_to_check);
    }

    constexpr void set_bits(uint32_t& byte, uint32_t bits_to_set, bool cond)
    {
        byte &= ~bits_to_set;
        if (cond) byte |= bits_to_set;
    }

    inline int32_t sign_extend32(uint32_t opcode, int start_offset, int signed_bit_start)
    {
        bool is_signed = Utils::is_bit_set(opcode, signed_bit_start);
        uint32_t unsigned_offset = Utils::get_bits(opcode, start_offset, signed_bit_start);
   
        uint32_t negative_bitmask = 0xFFFFFFFF << signed_bit_start;
        int32_t sign_extended_offset = (is_signed ? negative_bitmask : 0) | (unsigned_offset);
        
        return sign_extended_offset;
    }
};