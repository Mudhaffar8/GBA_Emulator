#include "graphics.hpp"

#include <cassert>

Graphics::Graphics(Memory& _memory) : 
    memory(_memory),
    dispcnt(_memory),
    dispstat(_memory),
    scanline_y(_memory),
    bg0_control(_memory),
    bg1_control(_memory),
    bg2_control(_memory),
    bg3_control(_memory),
    bg0_x_offset(_memory),
    bg0_y_offset(_memory),
    bg1_x_offset(_memory),
    bg1_y_offset(_memory),
    bg2_x_offset(_memory),
    bg2_y_offset(_memory),
    bg3_x_offset(_memory),
    bg3_y_offset(_memory),
    bg2_pa(_memory),
    bg2_pb(_memory),
    bg2_pc(_memory),
    bg2_pd(_memory),
    bg3_pa(_memory),
    bg3_pb(_memory),
    bg3_pc(_memory),
    bg3_pd(_memory),
    bg2_x(_memory),
    bg2_y(_memory),
    bg3_x(_memory),
    bg3_y(_memory),
    win0_horizontal(_memory),
    win0_vertical(_memory),
    win1_horizontal(_memory),
    win1_vertical(_memory),
    window_inside(_memory),
    window_outside(_memory),
    color_effects_select(_memory),
    alpha_blend_coefficients(_memory),
    brightness_coefficient(_memory)
{}

void Graphics::render_scanline()
{
    render_scanline_mode3(static_cast<int>(scanline_y));
    scanline_y = (scanline_y >= GBARes::LCD_H-1) ? 0 : scanline_y + 1;
}

// Standard 16-bit bitmapped (non-paletted) 240x160 mode.
void Graphics::render_scanline_mode3(int screen_y)
{
    assert(screen_y < GBARes::LCD_H);

    // No way Mode 3 is this easy?
    // There has to be some hidden complexity
    // or obscure edge case I need to implement

    int height = GBARes::LCD_W * screen_y;
    
    for (int x = 0; x < GBARes::LCD_W; ++x)
    {
        uint16_t color = memory.read_vram16(GBAMem::VRAM_START + x*2 + height);
        frame_buffer.at(x + height) = convert_bgr555_to_rgba32(color);
    }
}

void Graphics::render_scanline_mode4(int screen_y)
{
    uint32_t bitmap_start_addr = Utils::is_bit_set(dispcnt, 4) ? 0x06000000 : 0x0600A000;

    int height = GBARes::LCD_W * screen_y;
    
    (void)bitmap_start_addr;
    (void)height;
    
    // for (int x = 0; x < GBARes::LCD_W; ++x) {}
}

uint32_t Graphics::convert_bgr555_to_rgba32(uint16_t bgr)
{
    uint8_t r5 = bgr & 0x1F;
    uint8_t g5 = (bgr >> 5) & 0x1F;
    uint8_t b5 = (bgr >> 10) & 0x1F;

    uint8_t r8 = (r5 << 3) | (r5 >> 2);
    uint8_t g8 = (g5 << 3) | (g5 >> 2);
    uint8_t b8 = (b5 << 3) | (b5 >> 2);

    return (r8 << 24) | (g8 << 16) | (b8 << 8) | 0xFF;
}