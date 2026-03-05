/**
 * @file procedural_atlas.hh
 * @brief Procedural texture atlas generator for tile demos
 *
 * Generates a simple tile atlas at runtime using SDL drawing functions.
 * Useful for demos and testing without requiring external image files.
 */

#pragma once

#include <sdlpp/video/renderer.hh>
#include <sdlpp/video/texture.hh>
#include <onyxui/tile/tile_types.hh>
#include <onyxui/tile/tile_theme.hh>

// Forward declarations to avoid namespace conflicts
namespace sdlpp {
    class renderer;
    class texture;
}

#include <cstdint>
#include <array>
#include <memory>
#include <optional>

namespace onyxui::tile {

/**
 * @brief Color palette for procedural atlas
 */
struct atlas_palette {
    std::array<uint8_t, 4> panel_bg{60, 60, 80, 255};       // Panel background
    std::array<uint8_t, 4> panel_border{100, 100, 140, 255}; // Panel border
    std::array<uint8_t, 4> panel_highlight{120, 120, 160, 255}; // Panel highlight
    std::array<uint8_t, 4> panel_shadow{40, 40, 60, 255};   // Panel shadow

    std::array<uint8_t, 4> button_bg{80, 80, 120, 255};     // Button background
    std::array<uint8_t, 4> button_hover{100, 100, 150, 255}; // Button hover
    std::array<uint8_t, 4> button_pressed{50, 50, 80, 255}; // Button pressed

    std::array<uint8_t, 4> dark_bg{30, 30, 45, 255};        // Dark panel background
    std::array<uint8_t, 4> dark_border{50, 50, 70, 255};    // Dark panel border

    std::array<uint8_t, 4> text_color{220, 220, 240, 255};  // Normal text
    std::array<uint8_t, 4> text_disabled{120, 120, 140, 255}; // Disabled text
    std::array<uint8_t, 4> text_highlight{255, 255, 150, 255}; // Highlighted text
};

/**
 * @brief Procedurally generated atlas with all necessary tiles
 *
 * Atlas layout (16x16 tiles, 8x8 pixels each = 128x128 texture):
 *
 * Row 0: Light panel nine-slice (tiles 0-8)
 *   0=TL, 1=T, 2=TR, 3=L, 4=C, 5=R, 6=BL, 7=B, 8=BR
 *
 * Row 1: Dark panel nine-slice (tiles 16-24)
 *   16=TL, 17=T, 18=TR, 19=L, 20=C, 21=R, 22=BL, 23=B, 24=BR
 *
 * Row 2: Button normal nine-slice (tiles 32-40)
 * Row 3: Button hover nine-slice (tiles 48-56)
 * Row 4: Button pressed nine-slice (tiles 64-72)
 *
 * Rows 6-11: ASCII font (glyphs 32-127, 6 rows x 16 columns)
 *   Row 6: Space to / (tiles 96-111)
 *   Row 7: 0-9, :;<=>?@ (tiles 112-127)
 *   Row 8: A-O (tiles 128-143)
 *   Row 9: P-Z, [\]^_` (tiles 144-159)
 *   Row 10: a-o (tiles 160-175)
 *   Row 11: p-z, {|}~ (tiles 176-191)
 */
class procedural_atlas {
public:
    static constexpr int TILE_SIZE = 8;
    static constexpr int ATLAS_COLUMNS = 16;
    static constexpr int ATLAS_ROWS = 16;
    static constexpr int ATLAS_WIDTH = TILE_SIZE * ATLAS_COLUMNS;   // 128
    static constexpr int ATLAS_HEIGHT = TILE_SIZE * ATLAS_ROWS;     // 128

    // Nine-slice tile indices
    static constexpr int PANEL_LIGHT_START = 0;
    static constexpr int PANEL_DARK_START = 16;
    static constexpr int BUTTON_NORMAL_START = 32;
    static constexpr int BUTTON_HOVER_START = 48;
    static constexpr int BUTTON_PRESSED_START = 64;

    // Font tile indices
    static constexpr int FONT_START = 96;  // Row 6, starting at tile 96
    static constexpr int FONT_FIRST_CHAR = 32;  // ASCII space
    static constexpr int FONT_CHAR_COUNT = 96;  // 32-127

    /**
     * @brief Generate the procedural atlas texture
     * @param renderer SDL renderer to create texture with
     * @param palette Color palette to use (optional, uses default if not provided)
     * @return Generated texture, or nullopt on failure
     */
    static std::optional<::sdlpp::texture> generate(
        ::sdlpp::renderer& renderer,
        const atlas_palette& palette = {});

    /**
     * @brief Create a tile_atlas struct configured for the procedural atlas
     * @param texture Pointer to the generated texture
     * @return Configured tile_atlas
     */
    static tile_atlas create_atlas(::sdlpp::texture* texture) {
        return tile_atlas{
            .texture = texture,
            .tex_type = texture_type::sdlpp,
            .tile_width = TILE_SIZE,
            .tile_height = TILE_SIZE,
            .columns = ATLAS_COLUMNS
        };
    }

    /**
     * @brief Create nine_slice for light panel
     */
    static constexpr nine_slice light_panel_slice() {
        return nine_slice{
            PANEL_LIGHT_START + 0,  // top_left
            PANEL_LIGHT_START + 1,  // top
            PANEL_LIGHT_START + 2,  // top_right
            PANEL_LIGHT_START + 3,  // left
            PANEL_LIGHT_START + 4,  // center
            PANEL_LIGHT_START + 5,  // right
            PANEL_LIGHT_START + 6,  // bottom_left
            PANEL_LIGHT_START + 7,  // bottom
            PANEL_LIGHT_START + 8,  // bottom_right
            TILE_SIZE / 2,          // margin_h
            TILE_SIZE / 2           // margin_v
        };
    }

    /**
     * @brief Create nine_slice for dark panel
     */
    static constexpr nine_slice dark_panel_slice() {
        return nine_slice{
            PANEL_DARK_START + 0,
            PANEL_DARK_START + 1,
            PANEL_DARK_START + 2,
            PANEL_DARK_START + 3,
            PANEL_DARK_START + 4,
            PANEL_DARK_START + 5,
            PANEL_DARK_START + 6,
            PANEL_DARK_START + 7,
            PANEL_DARK_START + 8,
            TILE_SIZE / 2,
            TILE_SIZE / 2
        };
    }

    /**
     * @brief Create nine_slice for normal button
     */
    static constexpr nine_slice button_normal_slice() {
        return nine_slice{
            BUTTON_NORMAL_START + 0,
            BUTTON_NORMAL_START + 1,
            BUTTON_NORMAL_START + 2,
            BUTTON_NORMAL_START + 3,
            BUTTON_NORMAL_START + 4,
            BUTTON_NORMAL_START + 5,
            BUTTON_NORMAL_START + 6,
            BUTTON_NORMAL_START + 7,
            BUTTON_NORMAL_START + 8,
            TILE_SIZE / 2,
            TILE_SIZE / 2
        };
    }

    /**
     * @brief Create nine_slice for hovered button
     */
    static constexpr nine_slice button_hover_slice() {
        return nine_slice{
            BUTTON_HOVER_START + 0,
            BUTTON_HOVER_START + 1,
            BUTTON_HOVER_START + 2,
            BUTTON_HOVER_START + 3,
            BUTTON_HOVER_START + 4,
            BUTTON_HOVER_START + 5,
            BUTTON_HOVER_START + 6,
            BUTTON_HOVER_START + 7,
            BUTTON_HOVER_START + 8,
            TILE_SIZE / 2,
            TILE_SIZE / 2
        };
    }

    /**
     * @brief Create nine_slice for pressed button
     */
    static constexpr nine_slice button_pressed_slice() {
        return nine_slice{
            BUTTON_PRESSED_START + 0,
            BUTTON_PRESSED_START + 1,
            BUTTON_PRESSED_START + 2,
            BUTTON_PRESSED_START + 3,
            BUTTON_PRESSED_START + 4,
            BUTTON_PRESSED_START + 5,
            BUTTON_PRESSED_START + 6,
            BUTTON_PRESSED_START + 7,
            BUTTON_PRESSED_START + 8,
            TILE_SIZE / 2,
            TILE_SIZE / 2
        };
    }

    /**
     * @brief Create bitmap_font configured for the procedural atlas
     * @param atlas Pointer to the tile_atlas
     * @return Configured bitmap_font
     */
    static bitmap_font create_font(tile_atlas* atlas) {
        return bitmap_font{
            .atlas = atlas,
            .glyph_width = TILE_SIZE,
            .glyph_height = TILE_SIZE,
            .first_char = FONT_FIRST_CHAR,
            .char_count = FONT_CHAR_COUNT,
            .first_tile_id = FONT_START
        };
    }

    /**
     * @brief Create a complete tile_theme using the procedural atlas
     * @param atlas Pointer to the tile_atlas (must remain valid)
     * @return Configured tile_theme
     */
    static tile_theme create_theme(tile_atlas* atlas);

private:
    // Drawing helpers
    static void draw_nine_slice_tiles(
        uint32_t* pixels, int pitch,
        int start_tile,
        const std::array<uint8_t, 4>& bg,
        const std::array<uint8_t, 4>& border,
        const std::array<uint8_t, 4>& highlight,
        const std::array<uint8_t, 4>& shadow);

    static void draw_font_glyphs(
        uint32_t* pixels, int pitch,
        const std::array<uint8_t, 4>& color);

    static void set_pixel(uint32_t* pixels, int pitch, int x, int y,
                         const std::array<uint8_t, 4>& color);

    static uint32_t make_color(const std::array<uint8_t, 4>& rgba);
};

} // namespace onyxui::tile
