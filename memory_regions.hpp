#pragma once

#include <cstdint>

namespace GBAMem
{
    constexpr uint32_t SYSTEM_ROM_END = 0x0003FFF;

    constexpr uint32_t EWRAM_START = 0x2000000;
    constexpr uint32_t EWRAM_END = 0x203FFFF;

    constexpr uint32_t IWRAM_START = 0x3000000;
    constexpr uint32_t IWRAM_END = 0x3007FFF;

    constexpr uint32_t IO_REGISTERS_START = 0x4000000;
    constexpr uint32_t IO_REGISTERS_END = 0x40003FE;

    constexpr uint32_t BG_OBJ_PALETTE_DATA_START = 0x5000000;
    constexpr uint32_t BG_OBJ_PALETTE_DATA_END =   0x50003FF;
  
    constexpr uint32_t VRAM_START = 0x6000000;
    constexpr uint32_t VRAM_END = 0x6017FFF;  
    constexpr uint32_t SPRITE_TILES_START = 0x10000;

    constexpr uint32_t OAM_START = 0x7000000;
    constexpr uint32_t OAM_END = 0x70003FF;

    constexpr uint32_t GAME_PAK_ROM_START = 0x8000000;
    constexpr uint32_t GAME_PAK_ROM_END = 0xDFFFFFF;
}

namespace GBACart
{
    constexpr uint32_t ENTRY_POINT = 0x8000000;

    constexpr uint32_t NINTENDO_LOGO_START = 0x8000004;
    constexpr uint32_t NINTENDO_LOGO_END = 0x800009F;

    constexpr uint32_t GAME_TITLE_START = 0x80000A0;
    constexpr uint32_t GAME_TITLE_END = 0x80000AB;

    constexpr uint32_t GAME_CODE_START = 0x80000AC;
    constexpr uint32_t GAME_CODE_END = 0x80000AF;

    constexpr uint32_t MAKER_CODE_START = 0x80000B0;
    constexpr uint32_t MAKER_CODE_END = 0x80000B1;

    constexpr uint32_t FIXED_VALUE = 0x80000B2;
    constexpr uint32_t MAIN_UNIT_CODE = 0x80000B3;
    constexpr uint32_t DEVICE_TYPE = 0x80000B4;

    constexpr uint32_t RESERVED_START = 0x80000B5;
    constexpr uint32_t RESERVED_END = 0x80000BB;

    constexpr uint32_t SOFTWARE_VERSION = 0x80000BC;
    constexpr uint32_t COMPLEMENT_CHECK = 0x80000BD;

    /* Additional Multiboot Header Entries */
    constexpr uint32_t RAM_ENTRY_POINT_START = 0x80000C0;
    constexpr uint32_t RAM_ENTRY_POINT_END = 0x80000C3;

    constexpr uint32_t BOOT_MODE = 0x80000C4;
    constexpr uint32_t SLAVE_ID = 0x80000C5;

    constexpr uint32_t JOYBUS_ENTRY_START = 0x80000E0;
    constexpr uint32_t JOYBUS_ENTRY_END = 0x80000E3;
}

namespace GBAIO
{
    /* LCD Registers */
    constexpr uint32_t DISPCNT = 0x4000000; // LCD Control (R/W)
    constexpr uint32_t DISPSTAT = 0x4000004; // LCD Status (R/W)
    constexpr uint32_t VCOUNT = 0x4000006; // Vertical Counter (R)
    constexpr uint32_t BG0CNT = 0x4000008; // BG0 Control (R/W)
    constexpr uint32_t BG1CNT = 0x400000A; // BG1 Control (R/W)
    constexpr uint32_t BG2CNT = 0x400000C; // BG2 Control (R/W)
    constexpr uint32_t BG3CNT = 0x400000E; // BG3 Control (R/W)
    constexpr uint32_t BG0HOFS = 0x4000010; // BG0 X-Offset
    constexpr uint32_t BG0VOFS = 0x4000012; // BG0 Y-Offset
    constexpr uint32_t BG1HOFS = 0x4000014; // BG1 X-Offset
    constexpr uint32_t BG1VOFS = 0x4000016; // BG1 Y-Offset
    constexpr uint32_t BG2HOFS = 0x4000018; // BG2 X-Offset
    constexpr uint32_t BG2VOFS = 0x400001A; // BG2 Y-Offset
    constexpr uint32_t BG3HOFS = 0x400001C; // BG3 X-Offset
    constexpr uint32_t BG3VOFS = 0x400001E; // BG3 Y-Offset
    constexpr uint32_t BG2PA = 0x4000020; // BG2 Rotation/Scaling Parameter
    constexpr uint32_t BG2PB = 0x4000022; // BG2 Rotation/Scaling Parameter
    constexpr uint32_t BG2PC = 0x4000024; // BG2 Rotation/Scaling Parameter
    constexpr uint32_t BG2PD = 0x4000026; // BG2 Rotation/Scaling Parameter
    constexpr uint32_t BG2X = 0x4000028; // BG2 Reference Point X-Coordinate (32-bit)
    constexpr uint32_t BG2Y = 0x400002C; // BG2 Reference Point Y-Coordinate (32-bit)
    constexpr uint32_t BG3PA = 0x4000030; // BG3 Rotation/Scaling Parameter
    constexpr uint32_t BG3PB = 0x4000032; // BG3 Rotation/Scaling Parameter
    constexpr uint32_t BG3PC = 0x4000034; // BG3 Rotation/Scaling Parameter
    constexpr uint32_t BG3PD = 0x4000036; // BG3 Rotation/Scaling Parameter
    constexpr uint32_t BG3X = 0x4000038; // BG3 Reference Point X-Coordinate (32-bit)
    constexpr uint32_t BG3Y = 0x400003C; // BG3 Reference Point Y-Coordinate (32-bit)
    constexpr uint32_t WIN0H = 0x4000040; // Window 0 Horizontal Dimensions
    constexpr uint32_t WIN1H = 0x4000042; // Window 1 Horizontal Dimensions
    constexpr uint32_t WIN0V = 0x4000044; // Window 0 Horizontal Dimensions
    constexpr uint32_t WIN1V = 0x4000046; // Window 1 Horizontal Dimensions
    constexpr uint32_t WININ = 0x4000048; // Inside of Window 0 and 1
    constexpr uint32_t WINOUT = 0x400004A; // Inside of OBJ Window & Outside of Window
    constexpr uint32_t MOSAIC = 0x400004C; // Mosaic Size
    constexpr uint32_t BLDCNT = 0x4000050; // Color Special Effects Selection
    constexpr uint32_t BLDALPHA = 0x4000052; // Alpha Blending Coefficients
    constexpr uint32_t BLDY = 0x4000054; // Brightness (Fade-In/Out) Coefficient

    /* DMA Transfer Channels */
    constexpr uint32_t DMA0SAD = 0x40000B0; // DMA 0 Source Address (32-bit)
    constexpr uint32_t DMA0DAD = 0x40000B4; // DMA 0 Destination Address (32-bit)
    constexpr uint32_t DMA0CNT_L = 0x40000B8; // DMA 0 Word Count 
    constexpr uint32_t DMA0CNT_H = 0x40000BA; // DMA 0 Control
    constexpr uint32_t DMA1SAD = 0x40000BC; // DMA 1 Source Address (32-bit)
    constexpr uint32_t DMA1DAD = 0x40000C0; // DMA 1 Destination Address (32-bit)
    constexpr uint32_t DMA1CNT_L = 0x40000C4; // DMA 1 Word Count 
    constexpr uint32_t DMA1CNT_H = 0x40000C6; // DMA 1 Control 
    constexpr uint32_t DMA2SAD = 0x40000C8; // DMA 2 Source Address (32-bit)
    constexpr uint32_t DMA2DAD = 0x40000CC; // DMA 2 Destination Address (32-bit)
    constexpr uint32_t DMA2CNT_L = 0x40000D0; // DMA 2 Word Count 
    constexpr uint32_t DMA2CNT_H = 0x40000D2; // DMA 2 Control
    constexpr uint32_t DMA3SAD = 0x40000D4; // DMA 3 Source Address (32-bit)
    constexpr uint32_t DMA3DAD = 0x40000D8; // DMA 3 Destination Address (32-bit)
    constexpr uint32_t DMA3CNT_L = 0x40000DC; // DMA 3 Word Count 
    constexpr uint32_t DMA3CNT_H = 0x40000DE; // DMA 3 Control 

    /* Timer Registers */
    constexpr uint32_t TM0CNT_L = 0x4000100; // Timer 0 Counter/Reload
    constexpr uint32_t TM0CNT_H = 0x4000102; // Timer 0 Control
    constexpr uint32_t TM1CNT_L = 0x4000104; // Timer 1 Counter/Reload
    constexpr uint32_t TM1CNT_H = 0x4000106; // Timer 1 Control 
    constexpr uint32_t TM2CNT_L = 0x4000108; // Timer 2 Counter/Reload
    constexpr uint32_t TM2CNT_H = 0x400010A; // Timer 2 Control
    constexpr uint32_t TM3CNT_L = 0x400010C; // Timer 3 Counter/Reload
    constexpr uint32_t TM3CNT_H = 0x400010E; // Timer 3 Control

    /* Keypad Input */
    constexpr uint32_t KEYINPUT = 0x4000130; // Key Status (R)
    constexpr uint32_t KEYCNT = 0x4000132; // Key Interrupt Control (R/W)

    /* Interrupt, Waitstate, and Power-Down Control */
    constexpr uint32_t IE = 0x4000200; // Interrupt Enable (R/W)
    constexpr uint32_t IF = 0x4000202; // Interrupt Request Flag / IRQ Acknowledge (R/W)
    constexpr uint32_t WAITCNT = 0x4000204; // Game Pak Waitstate Control
    constexpr uint32_t IME = 0x4000208; // Interrupt Master Enable
    constexpr uint32_t POSTFLG = 0x4000300; // Post Boot Flag (8-bit)
    constexpr uint32_t HALTCNT = 0x4000301; // Power Down Control (8-bit)
};
