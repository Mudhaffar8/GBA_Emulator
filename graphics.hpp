#pragma once

#include "io_register.hpp"
#include "memory.hpp"
#include "memory_regions.hpp"
#include "graphics_settings.hpp"
#include "utils.hpp"

class Graphics
{
public:
    explicit Graphics(Memory& memory);

    enum Dispcnt 
    {
        BgMode = 0b111, // (0-5= Video Mode 0-5, 6-7=Prohibited)
        CgbMode = (1 << 3), // (0=GBA, 1=CGB; can be set only by BIOS opcodes)
        DispFrameSelect = (1 << 4), // (0-1=Frame 0-1) (for BG Modes 4,5 only)
        HBlankIntervalFree = (1 << 5), // (1=Allow access to OAM during H-Blank)
        ObjCharVRAMMapping = (1 << 6), // (0=Two dimensional, 1=One dimensional)
        ForcedBlank = (1 << 7), // (1=Allow FAST access to VRAM,Palette,OAM)
        ScreenEnableBG0 = (1 << 8), // (0=Off, 1=On)
        ScreenEnableBG1 = (1 << 9),
        ScreenEnableBG2 = (1 << 10),
        ScreenEnableBG3 = (1 << 11),
        WindowEnableW0 = (1 << 12),
        WindowEnableW1 = (1 << 13),
        WindowEnableObj = (1 << 14),
    };

    enum DispStat 
    {
        VBlankFlag = (1 << 0),
        HBlankFlag = (1 << 1),
        VCounterFlag = (1 << 2),
        VBlankIRQEnable = (1 << 3),
        HBlankIRQEnable = (1 << 4),
        VCounterIRQEnable = (1 << 5),
        VCountSetting = 0xFF00, // Basically LYC (0..227)
    };

    void render_scanline();

    const std::array<uint32_t, GBARes::Resolution>& get_frame_buffer() const { return frame_buffer; }

private:
    Memory& memory;

    /* IO registers*/
    // LCD Control & Status
    Io16<GBAIO::DISPCNT> dispcnt;
    Io16<GBAIO::DISPSTAT> dispstat;

    // Scanline Y
    Io16<GBAIO::VCOUNT> scanline_y;

    // BG0-3 Toggle
    Io16<GBAIO::BG0CNT> bg0_control;
    Io16<GBAIO::BG1CNT> bg1_control;
    Io16<GBAIO::BG2CNT> bg2_control;
    Io16<GBAIO::BG3CNT> bg3_control;

    // BG0-3 X/Y Offsets
    Io16<GBAIO::BG0HOFS> bg0_x_offset;
    Io16<GBAIO::BG0VOFS> bg0_y_offset;
    Io16<GBAIO::BG1HOFS> bg1_x_offset;
    Io16<GBAIO::BG1VOFS> bg1_y_offset;
    Io16<GBAIO::BG2HOFS> bg2_x_offset;
    Io16<GBAIO::BG2VOFS> bg2_y_offset;
    Io16<GBAIO::BG3HOFS> bg3_x_offset;
    Io16<GBAIO::BG3VOFS> bg3_y_offset;

    // BG2 & BG3 Rotation/Scaling Parameters
    Io16<GBAIO::BG2PA> bg2_pa;
    Io16<GBAIO::BG2PB> bg2_pb;
    Io16<GBAIO::BG2PC> bg2_pc;
    Io16<GBAIO::BG2PD> bg2_pd;
    Io16<GBAIO::BG3PA> bg3_pa;
    Io16<GBAIO::BG3PB> bg3_pb;
    Io16<GBAIO::BG3PC> bg3_pc;
    Io16<GBAIO::BG3PD> bg3_pd;

    // BG2 & BG3 Reference Point X/Y Coordinates
    Io32<GBAIO::BG2X> bg2_x;
    Io32<GBAIO::BG2Y> bg2_y;
    Io32<GBAIO::BG3X> bg3_x;
    Io32<GBAIO::BG3Y> bg3_y;

    // Window Registers
    Io16<GBAIO::WIN0H> win0_horizontal;
    Io16<GBAIO::WIN0V> win0_vertical;
    Io16<GBAIO::WIN1H> win1_horizontal;
    Io16<GBAIO::WIN1V> win1_vertical;
    Io16<GBAIO::WININ> window_inside;
    Io16<GBAIO::WINOUT> window_outside;

    // Special Effects
    Io16<GBAIO::BLDCNT> color_effects_select;
    Io16<GBAIO::BLDALPHA> alpha_blend_coefficients;
    Io16<GBAIO::BLDY> brightness_coefficient;

    std::array<uint32_t, GBARes::LCD_H * GBARes::LCD_W> frame_buffer;

    void render_scanline_mode3(int screen_y);
    void render_scanline_mode4(int screen_y);
    // void render_scanline_mode5(int screen_y);

    uint32_t convert_bgr555_to_rgba32(uint16_t bgr);
};