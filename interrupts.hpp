#pragma once

#include "memory.hpp"

enum class InterruptType
{
    VBlank = (1 << 0),
    HBlank = (1 << 1),
    VCounterMatch = (1 << 2),
    Timer0Overflow = (1 << 3),
    Timer1Overflow = (1 << 4),
    Timer2Overflow = (1 << 5),
    Timer3Overflow = (1 << 6),
    SerialComm = (1 << 7),
    DMA0 = (1 << 8),
    DMA1 = (1 << 9),
    DMA2 = (1 << 10),
    DMA3 = (1 << 11),
    Keypad = (1 << 12),
    GamePak = (1 << 13),
};

namespace Interrupts
{
    inline void request_interrupt(Memory& mem, InterruptType interrupt) 
    {
        uint16_t if_flag = mem.get_if();
        // std::cout << "IF OLD: " << std::bitset<16>(if_flag) << '\n';
        mem.write_io16(if_flag | static_cast<uint16_t>(interrupt), GBAIO::IF);
        // std::cout << "IF NEW: " << std::bitset<16>(if_flag) << '\n';
    }
    inline void unset_interrupt(Memory& mem, InterruptType interrupt) {}

    inline void enable_interrupt(Memory& mem, InterruptType interrupt) {}
    inline void disable_interrupt(Memory& mem, InterruptType interrupt) {}

    /* Checking Interrupts */
    // Check if interrupt is request and enabled
    inline bool is_interrupt_queued(Memory& mem, InterruptType interrupt) { return false; } 
    inline bool is_interrupt_requested(Memory& mem, InterruptType interrupt) { return false; }
    inline bool is_interrupt_enabled(Memory& mem, InterruptType interrupt) { return false; }
}