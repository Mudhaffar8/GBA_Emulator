#pragma once

#include <cstdint>

#include "memory.hpp"

template<uint32_t addr>
struct Io32
{
    static_assert(addr % 4 == 0); // Ensure address is word-aligned

    Memory& memory;

    explicit Io32(Memory& mem) : memory(mem) {}

    uint32_t get() const { return memory.read_io32(addr); }
    void set(uint32_t val) { memory.write_io32(val, addr); }

    operator uint32_t() const { return get(); }

    void operator=(uint32_t op2) { set(op2); }

    void operator+=(uint32_t op2) { set(get() + op2); }
    void operator-=(uint32_t op2) { set(get() - op2); }
    void operator|=(uint32_t op2) { set(get() | op2); }
    void operator&=(uint32_t op2) { set(get() & op2); }
};

template<uint32_t addr>
struct Io16
{
    static_assert(addr % 2 == 0); // Ensure address is half-word aligned

    Memory& memory;

    explicit Io16(Memory& mem) : memory(mem) {}

    uint16_t get() const { return memory.read_io16(addr); }
    void set(uint16_t val) { memory.write_io16(val, addr); }

    operator uint16_t() const { return get(); }

    void operator=(uint16_t op2) { set(op2); }

    void operator+=(uint16_t op2) { set(get() + op2); }
    void operator-=(uint16_t op2) { set(get() - op2); }
    void operator|=(uint16_t op2) { set(get() | op2); }
    void operator&=(uint16_t op2) { set(get() & op2); }
};