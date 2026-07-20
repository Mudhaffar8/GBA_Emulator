#pragma once

#include <array>
#include <cstdint>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

class Memory 
{
public:
    Memory();

    uint8_t read8(uint32_t address);
    uint16_t read16(uint32_t address);
    uint32_t read32(uint32_t address); 

    void write8(uint8_t byte, uint32_t address);
    void write16(uint16_t half_word, uint32_t address);
    void write32(uint32_t word, uint32_t address);

private: 
    std::array<uint8_t, 0x4000> system_rom{}; 
     
    std::array<uint8_t, 0x40000> external_ram{};
    std::array<uint8_t, 0x8000> internal_ram{};

    std::array<uint8_t, 0x400> palette_data{};
    std::array<uint8_t, 0x18000> vram{};
    
    std::array<uint8_t, 0x3FF> io_registers{};
    std::array<uint8_t, 0x400> oam_data{};

    //  Turns out there are consequences for putting everything on the stack 
    std::vector<uint8_t> game_pak_rom1;
    std::vector<uint8_t> game_pak_rom2;
    std::vector<uint8_t> game_pak_rom3;
    std::vector<uint8_t> game_pak_sram;
};

// ------------------------------------------

class FakeMemory 
{
public:
    FakeMemory() {}
    
    uint8_t read8(uint32_t address) 
    { 
        auto it = memory.find(address);
        
        if (it != memory.end())
            return it->second;
        
        return 0; 
    }

    void write8(uint8_t byte, uint32_t address) 
    {
        std::cout << "Wrote byte " << std::dec << +byte << " @ " << address << '\n';
        memory.insert_or_assign(address, byte);
    }

    void write16(uint16_t half_word, uint32_t address)
    {
        std::cout << "Wrote half word " << half_word << " @ " << address << '\n';
        write8(half_word & 0xFF, address);
        write8((half_word >> 8) & 0xFF, address + 1);
    }

    void write32(uint32_t word, uint32_t address)
    {
        std::cout << "Wrote word " << word << " @ " << address << '\n';
        write8(word & 0xFF, address);
        write8((word >> 8) & 0xFF, address + 1);
        write8((word >> 16) & 0xFF, address + 2);
        write8((word >> 24) & 0xFF, address + 3);
    }

    uint16_t read16(uint32_t address)
    {
        return read8(address) | (read8(address + 1) << 8);
    }

    uint32_t read32(uint32_t address)
    {  
        return read8(address) | 
            (read8(address + 1) << 8) | 
            (read8(address + 2) << 16) | 
            (read8(address + 3) << 24); 
    }

    void clear_memory() { memory.clear(); }

    void print_memory()
    {
        for (auto& [key, value] : memory)
            std::cout << "[" << key << "]: " << +value << '\n';
    }

private:
    std::unordered_map<uint32_t, uint8_t> memory{};
};