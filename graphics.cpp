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
    scanline_y = (scanline_y >= 227) ? 0 : scanline_y + 1;
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
}

void Graphics::render_scanline_mode0(uint16_t screen_y)
{
    if (screen_y >= GBARes::LCD_H) return;

    std::fill(scanline.begin(), scanline.end(), 0);

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
        return (bg1.bg_control & 3) > (bg2.bg_control & 3);
    });

    for (BGInfo bg_info : mode0_bgs) 
    {
        if (bg_info.enable) 
            render_text_bg_scanline({bg_info.x, bg_info.y}, screen_y, bg_info.bg_control);
    }

    for (int i = 0; i < GBARes::LCD_W; ++i)
        frame_buffer.at(i + (screen_y * GBARes::LCD_W)) = Utils::convert_bgr555_to_rgba32(scanline[i]);
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

/// @note This implementation is buggy and incomplete.
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

void Graphics::render_text_bg_scanline(TileMapCoords bg_offset, uint16_t screen_y, uint16_t bg_control)
{
    if (screen_y >= GBARes::LCD_H) return;

    /*
        Value  Text Mode      Rotation/Scaling Mode
        0      256x256 (2K)   128x128   (256 bytes)
        1      512x256 (4K)   256x256   (1K)
        2      256x512 (4K)   512x512   (4K)
        3      512x512 (8K)   1024x1024 (16K)
    */

    int char_block_index = Utils::get_bits(bg_control, 2, 4); // 0-3 Charblocks (Tile Data)
    int screen_block_index = Utils::get_bits(bg_control, 8, 13); // 0-31 (Tile Maps)
    bool is_8bpp = Utils::is_bit_set(bg_control, 7);
    bool double_w_tilemap = Utils::is_bit_set(bg_control, 14);
    bool double_h_tilemap = Utils::is_bit_set(bg_control, 15);

    int scroll_max_x = (double_w_tilemap) ? 0x1FF : 0xFF;
    int scroll_max_y = (double_h_tilemap) ? 0x1FF : 0xFF;

    int tile_map_y = (screen_y + bg_offset.second) & scroll_max_y;

    // Get Tilemap Dimensions
    // Get Screen Coordinates
    // Get screen entry from screen block (need tilemap_dimensions, [screen_x, screen_y], screen_block_base)
    // Fetch tile row from char block base addr (need tile_number, char_block_base_addr, flip_y, is_8bpp)
    // Write pixels to scanline buffer (need tile_pixels, flip_x, screen_y, palette_number)

    uint16_t screenblock_base_addr = GBAVRam::SCREENBLOCK_SIZE * screen_block_index;
    uint16_t charblock_base_addr = GBAVRam::CHARBLOCK_SIZE * char_block_index;

    int tile_offset_x = bg_offset.first % 8;

    for (int tile_i = 0; tile_i < 31; ++tile_i)
    {
        int screen_x = tile_i * 8;
        int tile_map_x = (screen_x + bg_offset.first) & scroll_max_x;

        ScreenEntry se = get_screen_entry({tile_map_x, tile_map_y}, screenblock_base_addr, (double_w_tilemap ? 64 : 32));
        TileRow tile_row = fetch_bg_tile_row(se.tile_index, tile_map_y, charblock_base_addr, se.v_flip, is_8bpp);
        
        int last_tile_screen_x = screen_x - tile_offset_x;
        write_tile_row({last_tile_screen_x, screen_y}, tile_row, se.palette_bank, se.h_flip, is_8bpp);
    }
}

Graphics::ScreenEntry Graphics::get_screen_entry(TileMapCoords tile_map_coords, uint16_t base_addr, uint16_t pitch)
{
    int tile_x = tile_map_coords.first / 32;
    int tile_y = tile_map_coords.second / 32;
    int sbb = (tile_y / 32) * (pitch / 32) + (tile_x / 32);

    int screen_entry_index = (sbb * 1024) + (tile_y % 32) * 32 + (tile_x % 32);
    uint16_t entry = memory.read_vram16(base_addr + (screen_entry_index * 2));
    
    ScreenEntry screen_entry;
    screen_entry.tile_index = Utils::get_bits(entry, 0, 10);
    screen_entry.palette_bank = Utils::get_bits(entry, 12, 16);
    screen_entry.h_flip = Utils::is_bit_set(entry, 10);
    screen_entry.v_flip = Utils::is_bit_set(entry, 11);

    return screen_entry;
}

Graphics::TileRow Graphics::fetch_bg_tile_row(uint16_t tile_index, uint16_t tile_map_y, uint16_t base_addr, bool flip_y, bool is_8bpp)
{
    int tile_row_index = tile_map_y % 8;
    if (flip_y) tile_row_index = 7 - tile_row_index;

    int tile_row_len = (is_8bpp) ? 8 : 4; // in bytes
    int tile_row_addr = base_addr + (8 * tile_row_len * tile_index) + (tile_row_len * tile_row_index);
    TileRow tile_row = static_cast<TileRow>(memory.read_vram64(tile_row_addr));

    return tile_row;
}

void Graphics::write_tile_row(ScreenCoords screen_coords, TileRow tile_row, uint16_t palette_index, bool flip_x, bool is_8bpp)
{
    if (screen_coords.second >= GBARes::LCD_H) return;
    
    for (int i = 0; i < 8; ++i)
    {
        int pixel_screen_x = screen_coords.first + i;

        if (pixel_screen_x >= GBARes::LCD_W) break;
        else if (pixel_screen_x < 0) continue;

        int bit_index = (flip_x) ? 7 - i : i;
        int colour_index = (is_8bpp) ? (tile_row >> (bit_index * 8)) & 0xFF : (tile_row >> (bit_index * 4)) & 0xF;
        if (!is_8bpp) colour_index %= 16;

        if (colour_index == 0) continue;

        int colour_index_addr = (is_8bpp) ? (colour_index * 2) : (palette_index * 32) + (colour_index * 2);
        int palette_color = memory.read_palette_data16(colour_index_addr);
        
        scanline.at(pixel_screen_x) = palette_color;
    }
}