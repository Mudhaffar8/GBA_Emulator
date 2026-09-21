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
        mem.write_io16(if_flag | static_cast<uint16_t>(interrupt), GBAIO::IF);
    }
    inline void unset_interrupt(Memory& mem, InterruptType interrupt) 
    {
        uint16_t if_flag = mem.get_if();
        mem.write_io16(if_flag & ~static_cast<uint16_t>(interrupt), GBAIO::IF);
    }

    inline void enable_interrupt(Memory& mem, InterruptType interrupt) 
    {
        uint16_t ie_flag = mem.get_ie();
        mem.write_io16(ie_flag | static_cast<uint16_t>(interrupt), GBAIO::IE);
    }
    inline void disable_interrupt(Memory& mem, InterruptType interrupt) 
    {
        uint16_t ie_flag = mem.get_ie();
        mem.write_io16(ie_flag & ~static_cast<uint16_t>(interrupt), GBAIO::IE);
    }

    /* Checking Interrupts */
    inline bool is_interrupt_requested(Memory& mem, InterruptType interrupt) 
    { 
        return mem.get_if() & static_cast<uint16_t>(interrupt); 
    }
    inline bool is_interrupt_enabled(Memory& mem, InterruptType interrupt)
    { 
        return mem.get_ie() & static_cast<uint16_t>(interrupt); 
    }
    // Check if interrupt is request and enabled
    inline bool is_interrupt_queued(Memory& mem, InterruptType interrupt) 
    { 
        return is_interrupt_requested(mem, interrupt) && is_interrupt_enabled(mem, interrupt); 
    } 
}