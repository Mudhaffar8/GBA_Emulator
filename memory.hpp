#pragma once

#include <array>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <vector>

#include "memory_regions.hpp"
#include "scheduler.hpp"
#include "utils.hpp"

enum class AccessType
{
    Sequential, 
    NonSequential,
    None // Doesn't increment global counter
};

class Memory 
{
public:
    Memory(Scheduler& scheduler);

    template <typename T>
    T read(uint32_t address, AccessType access_type = AccessType::None);

    template <typename T>
    void write(T value, uint32_t address, AccessType access_type = AccessType::None);

    template <typename T>
    T read_io(uint32_t address);

    template <typename T>
    void write_io(T value, uint32_t address);

    /* Loading ROMs */
    bool load_bios(std::string file_name);
    bool load_rom(std::string file_name);

    /* Cycle Counting */
    void get_cycles(uint32_t address, AccessType access_type);
    void add_internal_cycles(uint64_t cycles_to_advance = 1);

    /* Read/Write Operations for I/O Registers */
    /// @note read_io16, read_io32, write_io16, write_io32 all assume little-endian
    /// Who's using big-endian in 2026? Are you running this on a NASA computer?
    uint16_t read_io16(uint32_t address)
    {
        uint16_t val{};
        std::memcpy(&val, &io_registers[address - 0x4000000], sizeof(uint16_t));
        return val;
    }

    uint32_t read_io32(uint32_t address)
    {
        uint32_t val{};
        std::memcpy(&val, &io_registers[address - 0x4000000], sizeof(uint32_t));
        return val;
    }

    void write_io16(uint16_t half_word, uint32_t address)
    {
        std::memcpy(&io_registers[address - 0x4000000], &half_word, sizeof(uint16_t));
    }

    void write_io32(uint32_t word, uint32_t address)
    {
        std::memcpy(&io_registers[address - 0x4000000], &word, sizeof(uint32_t));
    }

    uint16_t read_vram16(uint32_t address)
    {
        uint16_t val{};
        std::memcpy(&val, &vram[address], sizeof(uint16_t));
        return val;
    }

    uint16_t read_oam16(uint32_t address)
    {
        uint16_t val{};
        std::memcpy(&val, &oam_data[address], sizeof(uint16_t));
        return val;
    }

    uint16_t read_palette_data16(uint32_t address)
    {
        uint16_t val{};
        std::memcpy(&val, &palette_data[address], sizeof(uint16_t));
        return val;
    }

private: 
    Scheduler& scheduler;
    
    std::array<uint8_t, 0x4000> system_rom{}; 
     
    std::array<uint8_t, 0x40000> external_ram{};
    std::array<uint8_t, 0x8000> internal_ram{};

    std::array<uint8_t, 0x400> palette_data{};
    std::array<uint8_t, 0x18000> vram{};
    
    std::array<uint8_t, 0x400> io_registers{};
    std::array<uint8_t, 0x400> oam_data{};

    std::vector<uint8_t> game_pak_rom;
    std::vector<uint8_t> game_pak_sram;
};

template <typename T>
T Memory::read(uint32_t address, AccessType access_type)
{
    static_assert(std::is_same<T, uint32_t>::value || std::is_same<T, uint16_t>::value || std::is_same<T, uint8_t>::value);

    if (access_type != AccessType::None)
        get_cycles(address, access_type);

    T val{};

    // The memory map is so clean :o
    switch(address & 0xF000000)
    {
    case 0x0000000: 
        Utils::do_bounds_check(address, 0, GBAMem::SYSTEM_ROM_END, "READ BIOS");
        std::memcpy(&val, system_rom.data() + address, sizeof(T)); break;
    case 0x2000000: 
        std::memcpy(&val, external_ram.data() + (address & 0x3FFFF), sizeof(T)); break;
    case 0x3000000: 
        std::memcpy(&val, internal_ram.data() + (address & 0x7FFF), sizeof(T)); break;
    case 0x4000000: 
        Utils::do_bounds_check(address, GBAMem::IO_REGISTERS_START, GBAMem::IO_REGISTERS_END, "READ IO");
        std::memcpy(&val, io_registers.data() + (address - 0x4000000), sizeof(T)); break;
    case 0x5000000: 
        std::memcpy(&val, palette_data.data() + (address & 0x3FF), sizeof(T)); break;
    case 0x6000000: 
        std::memcpy(&val, vram.data() + (address & 0x1FFFF), sizeof(T)); break;
    case 0x7000000: 
        Utils::do_bounds_check(address, GBAMem::OAM_START, GBAMem::OAM_END, "READ OAM");
        std::memcpy(&val, oam_data.data() + (address - 0x7000000), sizeof(T)); break;
    
    case 0x8000000: 
    case 0x9000000:     
    case 0xA000000: 
    case 0xB000000: 
    case 0xC000000: 
    case 0xD000000: 
        std::memcpy(&val, game_pak_rom.data() + (address & 0x1FFFFFF), sizeof(T));
        break;
    
    case 0xE000000: 
        std::memcpy(&val, game_pak_sram.data() + (address - 0xE000000), sizeof(T));
        break;
    
    default: throw std::runtime_error("Invalid Read Address: " + std::to_string(address));
    }

    return val;
}

template <typename T>
void Memory::write(T val, uint32_t address, AccessType access_type)
{
    static_assert(std::is_same<T, uint32_t>::value || std::is_same<T, uint16_t>::value || std::is_same<T, uint8_t>::value);

    if (access_type != AccessType::None)
        get_cycles(address, access_type);

    switch(address & 0xF000000)
    {
    case 0x0000000: 
        std::cout << "You can't write to SYSTEM ROM!\n"; break;
    case 0x2000000: 
        std::memcpy(external_ram.data() + (address & 0x3FFFF), &val, sizeof(T)); break;
    case 0x3000000: 
        std::memcpy(internal_ram.data() + (address & 0x7FFF), &val, sizeof(T)); break;

    case 0x4000000: 
        Utils::do_bounds_check(address, GBAMem::IO_REGISTERS_START, GBAMem::IO_REGISTERS_END, "WRITE IO");
        std::memcpy(io_registers.data() + (address - 0x4000000), &val, sizeof(T)); break;

    case 0x5000000: 
        std::memcpy(palette_data.data() + (address & 0x3FF), &val, sizeof(T)); break;
    case 0x6000000: 
        std::memcpy(vram.data() + (address & 0x1FFFF), &val, sizeof(T)); break;
    case 0x7000000: 
        std::memcpy(oam_data.data() + (address - 0x7000000), &val, sizeof(T)); break;
    
    case 0x8000000: 
    case 0x9000000:     
    case 0xA000000: 
    case 0xB000000: 
    case 0xC000000: 
    case 0xD000000: 
        std::memcpy(game_pak_rom.data() + (address & 0x1FFFFFF), &val, sizeof(T));
        break;
    
    case 0xE000000: 
        std::memcpy(game_pak_sram.data() + (address - 0xE000000), &val, sizeof(T));
        break;
    
    default: throw std::runtime_error("Invalid Write Address: " + std::to_string(address));
    }
}

// When the interrupt signal is sent, the appropriate flag is set in REG_IF. 
// The program code unsets this flag (by writing a 1 to that bit) in order 
// to keep track of what interrupts have been handled.
// 
// Basically, writing a 1 to a bit in IF clears it.
/// @note Assumes word reads are word-aligned and halfword reads are halfword-aligned
template <typename T>
T Memory::read_io(uint32_t address)
{
    T val{};
    switch(address)
    {
    default: 
        std::memcpy(&val, io_registers.data() + (address - GBAMem::IO_REGISTERS_START), sizeof(T));
        break;
    }

    return val;
}

/// @note Assumes word writes are word-aligned and halfword writes are halfword-aligned
template <typename T>
void Memory::write_io(T val, uint32_t address)
{
    switch(address)
    {
    default: 
        std::memcpy(game_pak_sram.data() + (address - 0xE000000), &val, sizeof(T));
        break;
    }
}

// ------------------------------------------
// For Testing Memory Below
// ------------------------------------------

class TestMemory 
{
public:
    TestMemory() {}

    void inner_write(uint8_t byte, uint32_t address) 
    {
        memory.insert_or_assign(address, byte);
    }

    template<typename T>
    void write(T val, uint32_t address, AccessType access_type = AccessType::None) 
    {
        if (access_type != AccessType::None)
            accesses.insert_or_assign(address, std::make_pair("write8", access_type));
        
        inner_write(val, address);

        if constexpr(std::is_same<T, uint32_t>::value || std::is_same<T, uint16_t>::value) 
            inner_write((val >> 8) & 0xFF, address + 1);

        if constexpr(std::is_same<T, uint32_t>::value)
        {
            inner_write((val >> 16) & 0xFF, address + 2);
            inner_write((val >> 24) & 0xFF, address + 3);
        }
    }

    uint8_t inner_read(uint32_t address) 
    { 
        auto it = memory.find(address);
        
        if (it != memory.end())
            return it->second;
        
        return 0; 
    }
    
    template<typename T>
    T read(uint32_t address, AccessType access_type = AccessType::None) 
    { 
        if (access_type != AccessType::None)
            accesses.insert_or_assign(address, std::make_pair("read8", access_type));

        T val = inner_read(address);

        if constexpr(std::is_same<T, uint16_t>::value || std::is_same<T, uint32_t>::value) 
            val |= inner_read(address + 1) << 8;

        if constexpr(std::is_same<T, uint32_t>::value)
        {
            val |= inner_read(address + 2) << 16;
            val |= inner_read(address + 3) << 24; 
        }

        return val;
    }

    void add_internal_cycles(uint32_t cycles_to_advance = 1) { internal_cycles += cycles_to_advance; }

    void clear() 
    { 
        memory.clear(); 
        accesses.clear();
        internal_cycles = 0;
    }

    void print_memory()
    {
        for (auto& [key, value] : memory)
        {
            std::cout << "[" << key << "]: " << value;
        
            if (auto it = memory.find(key); it != memory.end())
                std::cout << " Access Type: " << (int)it->second << '\n';
        }
    }

private:
    std::unordered_map<uint32_t, uint8_t> memory{};
    std::unordered_map<uint32_t, std::pair<std::string, AccessType>> accesses{};
    uint32_t internal_cycles = 0;
};