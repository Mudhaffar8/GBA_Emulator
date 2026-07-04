#include "memory.hpp"

#include <stdexcept>

#include "utils.hpp"

Memory::Memory() : 
    game_pak_rom1(0x2000000, 0),
    game_pak_rom2(0x2000000, 0),
    game_pak_rom3(0x2000000, 0)
{}

void Memory::write16(uint16_t half_word, uint32_t address)
{
    write8(half_word & 0xFF, address);
    write8(half_word >> 8, address + 1);
}

void Memory::write32(uint32_t word, uint32_t address)
{
    write8(word & 0xFF, address);
    write8((word >> 8) & 0xFF, address + 1);
    write8((word >> 16) & 0xFF, address + 2);
    write8((word >> 24) & 0xFF, address + 3);
}


uint16_t Memory::read16(uint32_t address)
{
    return read8(address) | (read8(address + 1) << 8);
}

uint32_t Memory::read32(uint32_t address)
{  
    return read8(address) | 
        (read8(address + 1) << 8) | 
        (read8(address + 2) << 16) | 
        (read8(address + 3) << 24); 
}

uint8_t Memory::read8(uint32_t address)
{
    // The memory map is so clean :o
    switch(address & Utils::MSB32)
    {
    case 0x0000000: return system_rom.at(address);
    case 0x2000000: return external_ram.at(address - 0x2000000);
    case 0x3000000: return internal_ram.at(address - 0x3000000);
    case 0x4000000: return io_registers.at(address - 0x4000000);
    case 0x5000000: return palette_data.at(address - 0x5000000);
    case 0x6000000: return vram.at(address - 0x6000000);
    case 0x7000000: return oam_data.at(address - 0x7000000);
    
    case 0x8000000: 
    case 0x9000000: 
        return game_pak_rom1.at(address - 0x8000000);
    
    case 0xA000000: 
    case 0xB000000: 
        return game_pak_rom2.at(address - 0xA000000);

    case 0xC000000: 
    case 0xD000000: 
        return game_pak_rom3.at(address - 0xC000000);
    
    case 0xE000000: 
        return game_pak_sram.at(address - 0xE000000);
    
    default: std::runtime_error("Invalid Read Address: " + +address);
    }

    return 0;
}

void Memory::write8(uint8_t byte, uint32_t address)
{
    switch(address & Utils::MSB32)
    {
    case 0x0000000: system_rom.at(address) = byte;
    case 0x2000000: external_ram.at(address - 0x2000000) = byte;
    case 0x3000000: internal_ram.at(address - 0x3000000) = byte;
    case 0x4000000: io_registers.at(address - 0x4000000) = byte;
    case 0x5000000: palette_data.at(address - 0x5000000) = byte;
    case 0x6000000: vram.at(address - 0x6000000) = byte;
    case 0x7000000: oam_data.at(address - 0x7000000) = byte;
    
    case 0x8000000: 
    case 0x9000000: 
        game_pak_rom1.at(address - 0x8000000) = byte;
    
    case 0xA000000: 
    case 0xB000000: 
        game_pak_rom2.at(address - 0xA000000) = byte;

    case 0xC000000: 
    case 0xD000000: 
        game_pak_rom3.at(address - 0xC000000) = byte;
    
    case 0xE000000: 
        game_pak_sram.at(address - 0xE000000) = byte;
    
    default: std::runtime_error("Invalid Write Address: " + +address);
    }
}