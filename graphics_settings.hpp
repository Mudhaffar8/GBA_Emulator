#pragma once

namespace GBARes
{
    constexpr int LCD_W = 240;
    constexpr int LCD_H = 160;

    constexpr int HBLANK = 68;

    constexpr int Resolution = LCD_W * LCD_H;
};

namespace GBATiming
{
    constexpr int PIXEL = 4;

    constexpr int HDRAW = GBARes::LCD_W * PIXEL;
    constexpr int HBLANK = GBARes::HBLANK * PIXEL;
    constexpr int SCANLINE = HDRAW + HBLANK;

    constexpr int VDRAW = GBARes::LCD_H * SCANLINE;
    constexpr int VBLANK = GBARes::HBLANK * SCANLINE;

    constexpr int REFRESH_RATE = VDRAW + VBLANK;
};

