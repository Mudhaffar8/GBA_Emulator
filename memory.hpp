#pragma once

#include <array>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>
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

enum class CycleType 
{
    Sequential,
    NonSequential,
    Internal,
    Coprocessor
};  

class Memory 
{
public:
    Memory(Scheduler& scheduler);

    /// @note It would probably be easier/faster to change read and write into template methods.
    uint8_t read(uint32_t address);
    
    uint8_t read8(uint32_t address, AccessType access_type);
    uint16_t read16(uint32_t address, AccessType access_type);
    uint32_t read32(uint32_t address, AccessType access_type); 

    void write(uint8_t byte, uint32_t address);

    void write8(uint8_t byte, uint32_t address, AccessType access_type);
    void write16(uint16_t half_word, uint32_t address, AccessType access_type);
    void write32(uint32_t word, uint32_t address, AccessType access_type);

    void add_bus_transaction(CycleType cyle_type, uint32_t address = 0);

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

private: 
    Scheduler& scheduler;
    
    std::array<uint8_t, 0x4000> system_rom{}; 
     
    std::array<uint8_t, 0x40000> external_ram{};
    std::array<uint8_t, 0x8000> internal_ram{};

    std::array<uint8_t, 0x400> palette_data{};
    std::array<uint8_t, 0x18000> vram{};
    
    std::array<uint8_t, 0x400> io_registers{};
    std::array<uint8_t, 0x400> oam_data{};

    //  Turns out there are consequences for putting everything on the stack 
    std::vector<uint8_t> game_pak_rom;
    std::vector<uint8_t> game_pak_sram;
};

// ------------------------------------------
// For Testing Memory Below
// ------------------------------------------

class TestMemory 
{
public:
    TestMemory() {}

    void write(uint8_t byte, uint32_t address) 
    {
        memory.insert_or_assign(address, byte);
    }

    void write8(uint8_t byte, uint32_t address, AccessType access_type = AccessType::None) 
    {
        if (access_type != AccessType::None)
            accesses.insert_or_assign(address, std::make_pair("write8", access_type));
        
        write(byte, address);
    }

    void write16(uint16_t half_word, uint32_t address, AccessType access_type = AccessType::None)
    {
        if (access_type != AccessType::None)
            accesses.insert_or_assign(address, std::make_pair("write16", access_type));

        write(half_word & 0xFF, address);
        write((half_word >> 8) & 0xFF, address + 1);
    }

    void write32(uint32_t word, uint32_t address, AccessType access_type = AccessType::None)
    {
        if (access_type != AccessType::None)
            accesses.insert_or_assign(address, std::make_pair("write32", access_type));
        
        write(word & 0xFF, address);
        write((word >> 8) & 0xFF, address + 1);
        write((word >> 16) & 0xFF, address + 2);
        write((word >> 24) & 0xFF, address + 3);
    }

    uint8_t read(uint32_t address) 
    { 
        auto it = memory.find(address);
        
        if (it != memory.end())
            return it->second;
        
        return 0; 
    }
    
    uint8_t read8(uint32_t address, AccessType access_type = AccessType::None) 
    { 
        if (access_type != AccessType::None)
            accesses.insert_or_assign(address, std::make_pair("read8", access_type));

        return read(address);
    }

    uint16_t read16(uint32_t address, AccessType access_type = AccessType::None)
    {
        if (access_type != AccessType::None)
            accesses.insert_or_assign(address, std::make_pair("read16", access_type));

        return read(address) | (read(address + 1) << 8);
    }

    uint32_t read32(uint32_t address, AccessType access_type = AccessType::None)
    {  
        if (access_type != AccessType::None)
            accesses.insert_or_assign(address, std::make_pair("read32", access_type));

        return read(address) | 
            (read(address + 1) << 8) | 
            (read(address + 2) << 16) | 
            (read(address + 3) << 24); 
    }

    void add_bus_transaction(CycleType cyle_type, uint32_t address = 0) {}

    void clear_memory() { memory.clear(); }

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
};