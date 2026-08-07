#include "graphics.hpp"

#include "interrupts.hpp"

#include <cassert>
#include <algorithm>

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
    dispstat |= DispStat::HBlankFlag;
    if (dispstat & DispStat::HBlankIRQEnable)
        GBAInterrupts::request_interrupt(memory, Interrupts::HBlank);
}


void Graphics::enter_vblank()
{
    dispstat |= DispStat::VBlankFlag;
    if (dispstat & DispStat::VBlankIRQEnable)
    {
        // std::cout << "IF: " << memory.read_io16(GBAIO::IF) << '\n';
        // std::cout << "IE: " << memory.read_io16(GBAIO::IE) << '\n';
        // std::cout << "IME: " << memory.read_io16(GBAIO::IME) << '\n';
        GBAInterrupts::request_interrupt(memory, Interrupts::VBlank);
    }
}

void Graphics::exit_vblank()
{
    dispstat &= ~DispStat::VBlankFlag;
    // std::fill(frame_buffer.begin(), frame_buffer.end(), 0xFFFFFFFF);
}

void Graphics::exit_hblank()
{
    dispstat &= ~DispStat::HBlankFlag;
}

void Graphics::render_scanline()
{
    int graphics_mode = dispcnt & Dispcnt::BgMode;
    
    switch(graphics_mode)
    {
        case 0: render_scanline_mode0(scanline_y); break;
        case 1: break;
        case 2: break;
        case 3: render_scanline_mode3(scanline_y); break;
        case 4: render_scanline_mode4(scanline_y); break;
        case 5: render_scanline_mode5(scanline_y); break;
        default: throw std::runtime_error("Invalid Graphics Mode: " + std::to_string(graphics_mode)); break;
    }

    scanline_y = (scanline_y >= 227) ? 0 : scanline_y + 1;
}

void Graphics::render_scanline_mode0(uint16_t screen_y)
{
    if (screen_y >= GBARes::LCD_H) return;

    bool bg0_enable = (dispcnt & Dispcnt::ScreenEnableBG0) != 0;
    bool bg1_enable = (dispcnt & Dispcnt::ScreenEnableBG1) != 0;
    bool bg2_enable = (dispcnt & Dispcnt::ScreenEnableBG2) != 0;
    bool bg3_enable = (dispcnt & Dispcnt::ScreenEnableBG3) != 0;

    std::array<BGInfo, 4> mode0_bgs = {{
        {bg0_control.get(), bg0_x_offset.get(), bg0_y_offset.get(), bg0_enable},
        {bg1_control.get(), bg1_x_offset.get(), bg1_y_offset.get(), bg1_enable},
        {bg2_control.get(), bg2_x_offset.get(), bg2_y_offset.get(), bg2_enable},
        {bg3_control.get(), bg3_x_offset.get(), bg3_y_offset.get(), bg3_enable}
    }};

    std::stable_sort(mode0_bgs.begin(), mode0_bgs.end(), [](const BGInfo& bg1, const BGInfo& bg2)
    {
        return (bg1.bg_control & 3) >= (bg2.bg_control & 3);
    });

    for (BGInfo bg_info : mode0_bgs) 
    {
        if (bg_info.enable) 
            render_text_bg_scanline(screen_y, bg_info.bg_control, bg_info.x, bg_info.y);
    }
}

/* Bit Map Modes*/
// Standard 16-bit bitmapped (non-paletted) 240x160 mode.
void Graphics::render_scanline_mode3(uint16_t screen_y)
{
    if (screen_y >= GBARes::LCD_H) return;
    if (!(dispcnt & Dispcnt::ScreenEnableBG2)) return;

    // No way Mode 3 is this easy?
    // There has to be some hidden complexity
    // or obscure edge case I need to implement

    int height = GBARes::LCD_W * screen_y;

    for (int x = 0; x < GBARes::LCD_W; ++x)
    {
        uint16_t color = memory.read_vram16((x + height) * 2);
        frame_buffer.at(x + height) = Utils::convert_bgr555_to_rgba32(color);
    }
}

void Graphics::render_scanline_mode4(uint16_t screen_y)
{
    if (screen_y >= GBARes::LCD_H) return;
    if (!(dispcnt & Dispcnt::ScreenEnableBG2)) return;

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

        frame_buffer.at(coords) = Utils::convert_bgr555_to_rgba32(color1);
        frame_buffer.at(coords + 1) = Utils::convert_bgr555_to_rgba32(color2);
    }
}

void Graphics::render_scanline_mode5(uint16_t screen_y)
{
    static const int MODE_5_WIDTH = 160;
    static const int MODE_5_HEIGHT = 128;
    static const int LEFT = GBARes::LCD_W - MODE_5_WIDTH;

    (void)LEFT; // I might find a use for this
    
    if (screen_y >= GBARes::LCD_H) return;
    if (!(dispcnt & Dispcnt::ScreenEnableBG2)) return;

    uint32_t bitmap_start_addr = (dispcnt & Dispcnt::DispFrameSelect) ? 0xA000 : 0;
    uint32_t height = MODE_5_HEIGHT * screen_y;
    
    for (int x = 0; x < MODE_5_WIDTH; ++x) 
    {
        int coords = (x + height) * 2;

        uint16_t color = memory.read_vram16(bitmap_start_addr + coords);
        frame_buffer.at(x + height) = Utils::convert_bgr555_to_rgba32(color);
    }
}

void Graphics::render_text_bg_scanline(uint16_t screen_y, uint16_t bg_control, uint16_t bg_x, uint16_t bg_y)
{

}
