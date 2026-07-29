#pragma once

#include <bitset>
#include <cstdint>
#include <exception>
#include <iostream>
#include <string_view>

// Quick and dirty
/// @todo Maybe find a more elegant way to do this without macros.
/// @note Could use std::is_same_v<T, Memory> and templates
// #define RUN_JSON_TESTS

namespace Utils
{
    constexpr uint32_t MSB32 = 0x80000000;
    constexpr bool DEBUG_MODE = false;

    template <typename T>
    inline void log(std::string_view name, T val)
    {
        if constexpr (DEBUG_MODE) 
            std::cout << name << ": " << val << '\n';
    }

    inline void print(std::string_view what)
    {
        if constexpr (DEBUG_MODE) 
            std::cout << what;
    }
    
    inline bool is_bit_set(uint32_t val, uint32_t bit_to_check)
    {   
        return (val >> bit_to_check) & 1;
    }

    inline uint32_t get_bits(uint32_t val, int bit_start, int bit_end)
    {
        return (val >> bit_start) & ((1 << (bit_end - bit_start)) - 1);
    }

    constexpr void set_bit(uint32_t& val, uint32_t bit_to_check, bool cond)
    {
        val = (cond) ? (val | bit_to_check) : (val & ~bit_to_check);
    }

    constexpr void set_bits(uint32_t& val, uint32_t bits_to_set, bool cond)
    {
        val &= ~bits_to_set;
        if (cond) 
            val |= bits_to_set;
    }

    /// @note Just make sure signed_bit_start < 32
    inline int32_t sign_extend32(uint32_t opcode, int start_offset, int signed_bit_start)
    {
        bool is_signed = Utils::is_bit_set(opcode, signed_bit_start);
        uint32_t unsigned_offset = Utils::get_bits(opcode, start_offset, signed_bit_start);
   
        uint32_t negative_bitmask = 0xFFFFFFFF << signed_bit_start;
        int32_t sign_extended_offset = (is_signed ? negative_bitmask : 0) | (unsigned_offset);
        
        return sign_extended_offset;
    }

    inline void do_bounds_check(uint32_t val, size_t start, size_t end)
    {
        if (val < start || val > end)
        {
            std::string s = "Out of Bounds! Value: " + std::to_string(val) + '\n';
            s += "Range Start: " + std::to_string(start);
            s += "Range End: " + std::to_string(end);
            throw std::runtime_error(s);
        }
    }
};