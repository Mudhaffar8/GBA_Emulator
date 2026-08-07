#include "memory.hpp"

#include "utils.hpp"

#include <stdexcept>
#include <fstream>
#include <filesystem>

Memory::Memory(Scheduler& _scheduler) : 
    scheduler(_scheduler),
    game_pak_rom(0x2000000, 0), // 32MB
    game_pak_sram(0x2000000, 0)
{
    write_io16(0x3FF, GBAIO::KEYINPUT);
}

static const std::array<std::vector<int>, 6> WAITSTATE_CTRL_TABLE 
{{
    {4, 3, 2, 8}, {2, 1},
    {4, 3, 2, 8}, {4, 1},
    {4, 3, 2, 8}, {8, 1},
}};

void Memory::add_internal_cycles(uint64_t cycles_to_advance)
{
    scheduler.advance(cycles_to_advance);
}

bool Memory::load_bios(std::string path)
{
    std::ifstream file(path,  std::ios::in | std::ios::out | std::ios::binary | std::ios::ate);

    if (!file.is_open()) 
    {
        std::cerr << "File does not exist: " << path << std::endl;
        return false;
    }

    size_t file_size = static_cast<size_t>(std::filesystem::file_size(path));
    if (file_size > GBAMem::SYSTEM_ROM_END+1)
    {
        std::cerr << "File size is too large" << std::endl;
        return false;
    }

    file.seekg(0, std::ios::beg);
    file.read(reinterpret_cast<char*>(system_rom.data()), system_rom.size());
    file.close();

    return true;
}

/// @todo Perform Header checks
bool Memory::load_rom(std::string path)
{
    std::ifstream file(path, std::ios::in | std::ios::binary);

    if (!file.is_open()) 
    {
        std::cerr << "File does not exist: " << path << std::endl;
        return false;
    }

    size_t file_size = static_cast<size_t>(std::filesystem::file_size(path));
    if (file_size > (GBAMem::GAME_PAK_ROM_END - GBAMem::GAME_PAK_ROM_START +1))
    {
        std::cerr << "File size is too large" << std::endl;
        return false;
    }

    file.seekg(0, std::ios::beg);
    file.read(reinterpret_cast<char*>(game_pak_rom.data()), game_pak_rom.size());
    file.close();

    return true;
}

void Memory::get_cycles(uint32_t address, AccessType access_type)
{
    uint64_t cycles{};

    switch(address & 0xF000000)
    {
    case 0x0000000: cycles = 1; break; // BIOS
    case 0x2000000: cycles = 3; break; // EWRAM, 16-bit data bus
    case 0x3000000: cycles = 1; break; // IWRAM, 32-bit data bus
    case 0x4000000: cycles = 1; break; // IO Registers
    case 0x5000000: cycles = 1; break; // Palette 
    case 0x6000000: cycles = 1; break; // VRAM, 16-bit data bus
    case 0x7000000: cycles = 1; break; // OAM, unless PPU is accessing at same time then 1
    
    /// @note The GBA forcefully uses non-sequential timing at the beginning of each 128K-block of gamepak ROM, 
    /// eg. “LDMIA [801fff8h],r0-r7” will have non-sequential timing at 8020000h.
    case 0x8000000: 
    case 0x9000000:     
        {
            bool is_sequential = access_type == AccessType::Sequential;
            int index = is_sequential ?
                Utils::is_bit_set(read_io16(GBAIO::WAITCNT), 4) :
                Utils::get_bits(read_io16(GBAIO::WAITCNT), 2, 4);

            cycles = WAITSTATE_CTRL_TABLE[is_sequential][index];
        }
        break;
        
    case 0xA000000: 
    case 0xB000000: // Game Pak ROM, 16-bit data bus
        {
            bool is_sequential = access_type == AccessType::Sequential;
            int index = is_sequential ?
                Utils::is_bit_set(read_io16(GBAIO::WAITCNT), 7) :
                Utils::get_bits(read_io16(GBAIO::WAITCNT), 5, 7);

            cycles = WAITSTATE_CTRL_TABLE[is_sequential + 2][index];
        }
        break;

    case 0xC000000: 
    case 0xD000000: // Game Pak ROM, 16-bit data bus
        {
            bool is_sequential = access_type == AccessType::Sequential;
            int index = is_sequential ?
                Utils::is_bit_set(read_io16(GBAIO::WAITCNT), 10) :
                Utils::get_bits(read_io16(GBAIO::WAITCNT), 8, 10);

            cycles = WAITSTATE_CTRL_TABLE[is_sequential + 4][index];
        }
        break;
    
    case 0xE000000: // 8-bit data bus
        cycles = WAITSTATE_CTRL_TABLE[0][Utils::get_bits(read_io16(GBAIO::WAITCNT), 0, 2)];
        break;
    
    default:
        cycles = 1;
        break;
    // default: throw std::runtime_error("Invalid Address: " + std::to_string(address));
    }

    scheduler.advance(cycles);
}