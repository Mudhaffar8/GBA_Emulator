#pragma once

#include "memory.hpp"

enum Interrupts
{
    Vblank = (1 << 0),
    Hblank = (1 << 1),
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

namespace GBAInterrupts
{
    inline void request_interrupt(Memory& mem, Interrupts interrupt) {}
    inline void unset_interrupt(Memory& mem, Interrupts interrupt) {}

    inline void enable_interrupt(Memory& mem, Interrupts interrupt) {}
    inline void disable_interrupt(Memory& mem, Interrupts interrupt) {}

    /* Checking Interrupts */
    // // Check if interrupt is request and enabled
    inline bool is_interrupt_queued(Memory& mem, Interrupts interrupt) { return false; } 
    inline bool is_interrupt_requested(Memory& mem, Interrupts interrupt) { return false; }
    inline bool is_interrupt_enabled(Memory& mem, Interrupts interrupt) { return false; }
}