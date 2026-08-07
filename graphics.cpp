#include "graphics.hpp"

#include "interrupts.hpp"

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

void Graphics::enter_hblank()
{

}

/*
    Okay the issue is certainly with VBlankIntrWait
    IF is set to 0 even when VBlank occurs so it's stuck
    also attempts to write to HALTCNT but the behaviour is weird
*/
void Graphics::enter_vblank()
{
    dispstat |= DispStat::VBlankFlag;
    if (dispstat & DispStat::VBlankIRQEnable)
    {
        std::cout << "IF: " << memory.read_io16(GBAIO::IF) << '\n';
        std::cout << "IE: " << memory.read_io16(GBAIO::IE) << '\n';
        std::cout << "IME: " << memory.read_io16(GBAIO::IME) << '\n';
        GBAInterrupts::request_interrupt(memory, Interrupts::VBlank);
    }
}

void Graphics::exit_vblank()
{
    dispstat &= ~DispStat::VBlankFlag;
}

void Graphics::render_scanline()
{
    int graphics_mode = dispcnt & Dispcnt::BgMode;
    
    switch(graphics_mode)
    {
        case 0: break;
        case 1: break;
        case 2: break;
        case 3: render_scanline_mode3(scanline_y); break;
        case 4: render_scanline_mode4(scanline_y); break;
        case 5: break;
        default: throw std::runtime_error("Invalid Graphics Mode: " + std::to_string(graphics_mode)); break;
    }

    scanline_y = (scanline_y >= 227) ? 0 : scanline_y + 1;
}

// Standard 16-bit bitmapped (non-paletted) 240x160 mode.
void Graphics::render_scanline_mode3(uint32_t screen_y)
{
    if (scanline_y >= GBARes::LCD_H) return;

    // No way Mode 3 is this easy?
    // There has to be some hidden complexity
    // or obscure edge case I need to implement

    int height = GBARes::LCD_W * screen_y;

    for (int x = 0; x < GBARes::LCD_W; ++x)
    {
        uint16_t color = memory.read_vram16((x + height) * 2);
        frame_buffer.at(x + height) = convert_bgr555_to_rgba32(color);
    }
}

void Graphics::render_scanline_mode4(uint32_t screen_y)
{
    if (scanline_y >= GBARes::LCD_H) return;

    uint32_t bitmap_start_addr = (dispcnt & Dispcnt::DispFrameSelect) ? 0xA000 : 0;
    uint32_t height = GBARes::LCD_W * screen_y;
    
    for (int x = 0; x < GBARes::LCD_W; x += 2) 
    {
        int coords = x + height;

        uint16_t palette_indices = memory.read_vram16(bitmap_start_addr + coords);
        uint8_t palette_index1 = palette_indices & 0xFF;
        uint8_t palette_index2 = palette_indices >> 8;

        uint16_t color1 = memory.read_palette_data16(palette_index1 * 2);
        uint16_t color2 = memory.read_palette_data16(palette_index2 * 2);

        frame_buffer.at(coords) = convert_bgr555_to_rgba32(color1);
        frame_buffer.at(coords + 1) = convert_bgr555_to_rgba32(color2);
    }
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