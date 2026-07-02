#pragma once

#include <array>
#include <cstdint>

class Memory 
{
public:
    Memory();

    // uint8_t read8(uint32_t address);
    // uint8_t read16(uint32_t address);
    // uint8_t read32(uint32_t address);

    // uint8_t write8(uint32_t address);
    // uint8_t write16(uint32_t address);
    // uint8_t write32(uint32_t address);

private: 
    // std::array<uint8_t, 0x4000> system_rom{}; 
    // std::array<uint8_t, 0x40000> external_ram{};
    // std::array<uint8_t, 0x8000> internal_ram{};

    // std::array<uint8_t, 0x400> palette_data{};
    // std::array<uint8_t, 0x18000> vram{};
    
    // std::array<uint8_t, 0x3FF> io_registers{};
    // std::array<uint8_t, 0x400> oam_data{};
};