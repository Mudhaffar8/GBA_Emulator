#pragma once

#include <array>
#include <cstdint>
#include <vector>

class Memory 
{
public:
    Memory();

    /* Will be using this method later */
    uint8_t read_byte(uint32_t address);
    void write_byte(uint8_t byte, uint32_t address);

    uint8_t read8(uint32_t address);
    uint16_t read16(uint32_t address);
    uint32_t read32(uint32_t address); 

    void write8(uint8_t byte, uint32_t address);
    void write16(uint16_t half_word, uint32_t address);
    void write32(uint32_t word, uint32_t address);

private: 
    std::vector<uint8_t> memory;

    std::array<uint8_t, 0x4000> system_rom{}; 
     
    std::array<uint8_t, 0x40000> external_ram{};
    std::array<uint8_t, 0x8000> internal_ram{};

    std::array<uint8_t, 0x400> palette_data{};
    std::array<uint8_t, 0x18000> vram{};
    
    std::array<uint8_t, 0x3FF> io_registers{};
    std::array<uint8_t, 0x400> oam_data{};

    std::array<uint8_t, 0x2000000> game_pak_rom1{};
    std::array<uint8_t, 0x2000000> game_pak_rom2{};
    std::array<uint8_t, 0x2000000> game_pak_rom3{};
    std::array<uint8_t, 0x10000> game_pak_sram{};
};