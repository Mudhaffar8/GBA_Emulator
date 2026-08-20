#pragma once

#include <optional>

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

    /* Event Handling */
    void enter_hblank();
    void enter_vblank();
    void exit_hblank();
    void exit_vblank();

    void render_scanline();

    const std::array<uint32_t, GBARes::Resolution>& get_frame_buffer() const { return frame_buffer; }

private:
    template <typename T>
    struct Coords
    {
        T x{}, y{};

        Coords(T _x, T _y) : x(_x), y(_y) {}
    };

    using ScreenCoords = Coords<int>;
    using TileMapCoords = Coords<uint16_t>;
    using TileCoords = Coords<uint32_t>;
    using TexelCoords = Coords<int>;
    using Dimensions = Coords<uint32_t>;

    using TileRow = uint64_t;
    using Tile = std::array<TileRow, 8>;

    enum class TileType { BG, Sprite };

    enum class ObjMode { Normal, Affine, Disabled, AffineDouble };
    enum class GfxMode { Normal, AlphaBlending, Window, Forbidden };

    struct BGInfo 
    {
        uint16_t bg_control;
        uint16_t x, y;
        bool enable;
    };

    struct ScreenEntry
    {
        uint16_t tile_index;
        uint8_t palette_bank; // Only in 16-color mode
        bool h_flip;
        bool v_flip;
    };

    struct Sprite
    {
        uint16_t attributes0{};
        uint16_t attributes1{};
        uint16_t attributes2{};

        Sprite(uint64_t op2)
        {
            attributes0 = op2 & 0xFFFF;
            attributes1 = (op2 >> 16) & 0xFFFF;
            attributes2 = (op2 >> 32) & 0xFFFF;
        }

        // Attribute 0 Getters
        int y() { return attributes0 & 0xFF; }
        ObjMode obj_mode() { return static_cast<ObjMode>(Utils::get_bits(attributes0, 8, 10)); } 
        GfxMode gfx_mode() { return static_cast<GfxMode>(Utils::get_bits(attributes0, 10, 12)); } 
        bool mosaic_enabled() { return Utils::is_bit_set(attributes0, 12); } 
        bool is_8bpp() { return Utils::is_bit_set(attributes0, 13); } 
        int shape() { return Utils::get_bits(attributes0, 14, 16); } 

        // Attribute 1 Getters
        int x() { return attributes1 & 0x1FF; }
        int affine_index() { return Utils::get_bits(attributes1, 9, 14); }
        bool h_flip() { return Utils::is_bit_set(attributes1, 12); }
        bool v_flip() { return Utils::is_bit_set(attributes1, 13); }
        int size() { return Utils::get_bits(attributes1, 14, 16); }

        // Attribute 2 Getters
        uint16_t tile_id() { return attributes2 & 0x3FF; }
        uint8_t priority() { return Utils::get_bits(attributes2, 10, 12); }
        uint8_t palette_bank() { return Utils::get_bits(attributes2, 12, 16); }
    };

    static constexpr std::array<std::array<std::pair<int, int>, 4>, 3> shape_size
    {{
        {{{8, 8}, {16, 16}, {32, 32}, {64, 64}}},
        {{{16, 8}, {32, 8}, {32, 16}, {64, 32}}},
        {{{8, 16}, {8, 32}, {16, 32}, {32, 64}}}
    }};

    Memory& memory;

    /* IO registers */
    // LCD Control & Status
    Io16<GBAIO::DISPCNT> dispcnt;
    Io16<GBAIO::DISPSTAT> dispstat;

    // Scanline Y
    Io16<GBAIO::VCOUNT> scanline_y;

    // BG0-3 Control
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

    std::array<uint16_t, GBARes::LCD_W> scanline{};
    std::array<uint32_t, GBARes::Resolution> frame_buffer{};

    /* Scanline Rendering */
    void render_scanline_mode0(uint16_t screen_y);
    void render_scanline_mode3(uint16_t screen_y);
    void render_scanline_mode4(uint16_t screen_y);
    void render_scanline_mode5(uint16_t screen_y);

    /* BG Rendering Methods */
    void render_text_bg_scanline(TileMapCoords bg_coords, uint16_t screen_y, uint16_t bg_control);
    void render_affine_bg_scanline(uint16_t screen_y, uint16_t bg_control);
    ScreenEntry get_screen_entry(TileMapCoords coords, uint32_t screen_block_base, uint16_t pitch);
    
    TileRow fetch_tile_row(uint32_t base_addr, uint16_t tile_map_index, uint16_t tile_map_y, bool flip_y, bool is_8bpp);
    template <TileType T>
    void write_tile_row(ScreenCoords screen_coords, TileRow tile_row, uint16_t palette_index, bool flip_x, bool is_8bpp);

    /* Sprite Rendering Methods */
    void render_sprites_scanline(uint16_t screen_y);
    void render_normal_sprite_scanline(Sprite sprite, Dimensions dimensions, uint16_t screen_y);
    void render_affine_sprite_scanline(Sprite sprite, Dimensions dimensions, uint16_t screen_y);

    uint16_t get_tile_id_offset(Sprite sprite, Dimensions size_tiles, TileCoords tile_coords);

    /* Affine-related Methods */
    double convert_fixed_point_to_double(uint16_t fixed_point);
    std::optional<uint16_t> get_texel(TexelCoords tile, uint32_t base_addr);
};