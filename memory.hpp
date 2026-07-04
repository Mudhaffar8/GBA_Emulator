#pragma once

#include <array>
#include <cstdint>
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