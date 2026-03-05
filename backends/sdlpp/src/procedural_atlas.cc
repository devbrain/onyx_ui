/**
 * @file procedural_atlas.cc
 * @brief Implementation of procedural texture atlas generator
 */

#include <onyxui/tile/procedural_atlas.hh>
#include <SDL3/SDL.h>
#include <cstring>

namespace onyxui::tile {

// Simple 5x7 bitmap font data for printable ASCII (32-127)
// Each character is encoded as 5 bytes (5 columns x 7 rows, LSB = top)
// This creates readable 5-pixel wide glyphs that fit in 8x8 tiles
static constexpr uint8_t FONT_DATA[96][5] = {
    {0x00,0x00,0x00,0x00,0x00}, // 32 ' '
    {0x00,0x00,0x5F,0x00,0x00}, // 33 '!'
    {0x00,0x07,0x00,0x07,0x00}, // 34 '"'
    {0x14,0x7F,0x14,0x7F,0x14}, // 35 '#'
    {0x24,0x2A,0x7F,0x2A,0x12}, // 36 '$'
    {0x23,0x13,0x08,0x64,0x62}, // 37 '%'
    {0x36,0x49,0x55,0x22,0x50}, // 38 '&'
    {0x00,0x05,0x03,0x00,0x00}, // 39 '''
    {0x00,0x1C,0x22,0x41,0x00}, // 40 '('
    {0x00,0x41,0x22,0x1C,0x00}, // 41 ')'
    {0x08,0x2A,0x1C,0x2A,0x08}, // 42 '*'
    {0x08,0x08,0x3E,0x08,0x08}, // 43 '+'
    {0x00,0x50,0x30,0x00,0x00}, // 44 ','
    {0x08,0x08,0x08,0x08,0x08}, // 45 '-'
    {0x00,0x60,0x60,0x00,0x00}, // 46 '.'
    {0x20,0x10,0x08,0x04,0x02}, // 47 '/'
    {0x3E,0x51,0x49,0x45,0x3E}, // 48 '0'
    {0x00,0x42,0x7F,0x40,0x00}, // 49 '1'
    {0x42,0x61,0x51,0x49,0x46}, // 50 '2'
    {0x21,0x41,0x45,0x4B,0x31}, // 51 '3'
    {0x18,0x14,0x12,0x7F,0x10}, // 52 '4'
    {0x27,0x45,0x45,0x45,0x39}, // 53 '5'
    {0x3C,0x4A,0x49,0x49,0x30}, // 54 '6'
    {0x01,0x71,0x09,0x05,0x03}, // 55 '7'
    {0x36,0x49,0x49,0x49,0x36}, // 56 '8'
    {0x06,0x49,0x49,0x29,0x1E}, // 57 '9'
    {0x00,0x36,0x36,0x00,0x00}, // 58 ':'
    {0x00,0x56,0x36,0x00,0x00}, // 59 ';'
    {0x00,0x08,0x14,0x22,0x41}, // 60 '<'
    {0x14,0x14,0x14,0x14,0x14}, // 61 '='
    {0x41,0x22,0x14,0x08,0x00}, // 62 '>'
    {0x02,0x01,0x51,0x09,0x06}, // 63 '?'
    {0x32,0x49,0x79,0x41,0x3E}, // 64 '@'
    {0x7E,0x11,0x11,0x11,0x7E}, // 65 'A'
    {0x7F,0x49,0x49,0x49,0x36}, // 66 'B'
    {0x3E,0x41,0x41,0x41,0x22}, // 67 'C'
    {0x7F,0x41,0x41,0x22,0x1C}, // 68 'D'
    {0x7F,0x49,0x49,0x49,0x41}, // 69 'E'
    {0x7F,0x09,0x09,0x01,0x01}, // 70 'F'
    {0x3E,0x41,0x41,0x51,0x32}, // 71 'G'
    {0x7F,0x08,0x08,0x08,0x7F}, // 72 'H'
    {0x00,0x41,0x7F,0x41,0x00}, // 73 'I'
    {0x20,0x40,0x41,0x3F,0x01}, // 74 'J'
    {0x7F,0x08,0x14,0x22,0x41}, // 75 'K'
    {0x7F,0x40,0x40,0x40,0x40}, // 76 'L'
    {0x7F,0x02,0x04,0x02,0x7F}, // 77 'M'
    {0x7F,0x04,0x08,0x10,0x7F}, // 78 'N'
    {0x3E,0x41,0x41,0x41,0x3E}, // 79 'O'
    {0x7F,0x09,0x09,0x09,0x06}, // 80 'P'
    {0x3E,0x41,0x51,0x21,0x5E}, // 81 'Q'
    {0x7F,0x09,0x19,0x29,0x46}, // 82 'R'
    {0x46,0x49,0x49,0x49,0x31}, // 83 'S'
    {0x01,0x01,0x7F,0x01,0x01}, // 84 'T'
    {0x3F,0x40,0x40,0x40,0x3F}, // 85 'U'
    {0x1F,0x20,0x40,0x20,0x1F}, // 86 'V'
    {0x7F,0x20,0x18,0x20,0x7F}, // 87 'W'
    {0x63,0x14,0x08,0x14,0x63}, // 88 'X'
    {0x03,0x04,0x78,0x04,0x03}, // 89 'Y'
    {0x61,0x51,0x49,0x45,0x43}, // 90 'Z'
    {0x00,0x00,0x7F,0x41,0x41}, // 91 '['
    {0x02,0x04,0x08,0x10,0x20}, // 92 '\'
    {0x41,0x41,0x7F,0x00,0x00}, // 93 ']'
    {0x04,0x02,0x01,0x02,0x04}, // 94 '^'
    {0x40,0x40,0x40,0x40,0x40}, // 95 '_'
    {0x00,0x01,0x02,0x04,0x00}, // 96 '`'
    {0x20,0x54,0x54,0x54,0x78}, // 97 'a'
    {0x7F,0x48,0x44,0x44,0x38}, // 98 'b'
    {0x38,0x44,0x44,0x44,0x20}, // 99 'c'
    {0x38,0x44,0x44,0x48,0x7F}, // 100 'd'
    {0x38,0x54,0x54,0x54,0x18}, // 101 'e'
    {0x08,0x7E,0x09,0x01,0x02}, // 102 'f'
    {0x08,0x14,0x54,0x54,0x3C}, // 103 'g'
    {0x7F,0x08,0x04,0x04,0x78}, // 104 'h'
    {0x00,0x44,0x7D,0x40,0x00}, // 105 'i'
    {0x20,0x40,0x44,0x3D,0x00}, // 106 'j'
    {0x00,0x7F,0x10,0x28,0x44}, // 107 'k'
    {0x00,0x41,0x7F,0x40,0x00}, // 108 'l'
    {0x7C,0x04,0x18,0x04,0x78}, // 109 'm'
    {0x7C,0x08,0x04,0x04,0x78}, // 110 'n'
    {0x38,0x44,0x44,0x44,0x38}, // 111 'o'
    {0x7C,0x14,0x14,0x14,0x08}, // 112 'p'
    {0x08,0x14,0x14,0x18,0x7C}, // 113 'q'
    {0x7C,0x08,0x04,0x04,0x08}, // 114 'r'
    {0x48,0x54,0x54,0x54,0x20}, // 115 's'
    {0x04,0x3F,0x44,0x40,0x20}, // 116 't'
    {0x3C,0x40,0x40,0x20,0x7C}, // 117 'u'
    {0x1C,0x20,0x40,0x20,0x1C}, // 118 'v'
    {0x3C,0x40,0x30,0x40,0x3C}, // 119 'w'
    {0x44,0x28,0x10,0x28,0x44}, // 120 'x'
    {0x0C,0x50,0x50,0x50,0x3C}, // 121 'y'
    {0x44,0x64,0x54,0x4C,0x44}, // 122 'z'
    {0x00,0x08,0x36,0x41,0x00}, // 123 '{'
    {0x00,0x00,0x7F,0x00,0x00}, // 124 '|'
    {0x00,0x41,0x36,0x08,0x00}, // 125 '}'
    {0x08,0x08,0x2A,0x1C,0x08}, // 126 '~'
    {0x08,0x1C,0x2A,0x08,0x08}, // 127 DEL (arrow)
};

uint32_t procedural_atlas::make_color(const std::array<uint8_t, 4>& rgba) {
    // RGBA8888 format
    return (static_cast<uint32_t>(rgba[0]) << 24) |
           (static_cast<uint32_t>(rgba[1]) << 16) |
           (static_cast<uint32_t>(rgba[2]) << 8) |
           static_cast<uint32_t>(rgba[3]);
}

void procedural_atlas::set_pixel(uint32_t* pixels, int pitch, int x, int y,
                                 const std::array<uint8_t, 4>& color) {
    if (x >= 0 && x < ATLAS_WIDTH && y >= 0 && y < ATLAS_HEIGHT) {
        pixels[y * (pitch / 4) + x] = make_color(color);
    }
}

void procedural_atlas::draw_nine_slice_tiles(
    uint32_t* pixels, int pitch,
    int start_tile,
    const std::array<uint8_t, 4>& bg,
    const std::array<uint8_t, 4>& border,
    const std::array<uint8_t, 4>& highlight,
    const std::array<uint8_t, 4>& shadow)
{
    // Calculate base position for this set of tiles
    int base_row = start_tile / ATLAS_COLUMNS;
    int base_col = start_tile % ATLAS_COLUMNS;

    // Tile layout: TL, T, TR, L, C, R, BL, B, BR
    // Positions relative to start_tile
    struct TilePos { int dx; int dy; } positions[9] = {
        {0, 0}, {1, 0}, {2, 0},  // TL, T, TR
        {3, 0}, {4, 0}, {5, 0},  // L, C, R
        {6, 0}, {7, 0}, {8, 0}   // BL, B, BR
    };

    for (int tile = 0; tile < 9; ++tile) {
        int tile_x = (base_col + positions[tile].dx) * TILE_SIZE;
        int tile_y = (base_row + positions[tile].dy) * TILE_SIZE;

        // Fill background
        for (int y = 0; y < TILE_SIZE; ++y) {
            for (int x = 0; x < TILE_SIZE; ++x) {
                set_pixel(pixels, pitch, tile_x + x, tile_y + y, bg);
            }
        }

        // Draw borders based on tile type
        bool top_border = (tile == 0 || tile == 1 || tile == 2);
        bool bottom_border = (tile == 6 || tile == 7 || tile == 8);
        bool left_border = (tile == 0 || tile == 3 || tile == 6);
        bool right_border = (tile == 2 || tile == 5 || tile == 8);

        // Top edge (highlight)
        if (top_border) {
            for (int x = 0; x < TILE_SIZE; ++x) {
                set_pixel(pixels, pitch, tile_x + x, tile_y, highlight);
                set_pixel(pixels, pitch, tile_x + x, tile_y + 1, border);
            }
        }

        // Left edge (highlight)
        if (left_border) {
            for (int y = 0; y < TILE_SIZE; ++y) {
                set_pixel(pixels, pitch, tile_x, tile_y + y, highlight);
                set_pixel(pixels, pitch, tile_x + 1, tile_y + y, border);
            }
        }

        // Bottom edge (shadow)
        if (bottom_border) {
            for (int x = 0; x < TILE_SIZE; ++x) {
                set_pixel(pixels, pitch, tile_x + x, tile_y + TILE_SIZE - 1, shadow);
                set_pixel(pixels, pitch, tile_x + x, tile_y + TILE_SIZE - 2, border);
            }
        }

        // Right edge (shadow)
        if (right_border) {
            for (int y = 0; y < TILE_SIZE; ++y) {
                set_pixel(pixels, pitch, tile_x + TILE_SIZE - 1, tile_y + y, shadow);
                set_pixel(pixels, pitch, tile_x + TILE_SIZE - 2, tile_y + y, border);
            }
        }
    }
}

void procedural_atlas::draw_font_glyphs(
    uint32_t* pixels, int pitch,
    const std::array<uint8_t, 4>& color)
{
    const std::array<uint8_t, 4> transparent = {0, 0, 0, 0};

    for (int ch = 0; ch < FONT_CHAR_COUNT; ++ch) {
        int tile_id = FONT_START + ch;
        int tile_col = tile_id % ATLAS_COLUMNS;
        int tile_row = tile_id / ATLAS_COLUMNS;
        int tile_x = tile_col * TILE_SIZE;
        int tile_y = tile_row * TILE_SIZE;

        // Clear tile to transparent
        for (int y = 0; y < TILE_SIZE; ++y) {
            for (int x = 0; x < TILE_SIZE; ++x) {
                set_pixel(pixels, pitch, tile_x + x, tile_y + y, transparent);
            }
        }

        // Draw glyph (5x7, centered in 8x8 with 1px padding)
        const uint8_t* glyph = FONT_DATA[ch];
        for (int col = 0; col < 5; ++col) {
            uint8_t column_data = glyph[col];
            for (int row = 0; row < 7; ++row) {
                if (column_data & (1 << row)) {
                    // Offset by 1 pixel to center the 5x7 glyph in 8x8
                    set_pixel(pixels, pitch,
                             tile_x + col + 1,
                             tile_y + row + 1,
                             color);
                }
            }
        }
    }
}

std::optional<sdlpp::texture> procedural_atlas::generate(
    sdlpp::renderer& renderer,
    const atlas_palette& palette)
{
    // Create surface
    SDL_Surface* surface = SDL_CreateSurface(
        ATLAS_WIDTH, ATLAS_HEIGHT, SDL_PIXELFORMAT_RGBA8888);

    if (!surface) {
        return std::nullopt;
    }

    // Lock surface for pixel manipulation
    if (!SDL_LockSurface(surface)) {
        SDL_DestroySurface(surface);
        return std::nullopt;
    }

    auto* pixels = static_cast<uint32_t*>(surface->pixels);
    int pitch = surface->pitch;

    // Clear to transparent
    std::memset(pixels, 0, static_cast<size_t>(pitch) * ATLAS_HEIGHT);

    // Draw light panel nine-slice (row 0)
    draw_nine_slice_tiles(pixels, pitch, PANEL_LIGHT_START,
                         palette.panel_bg, palette.panel_border,
                         palette.panel_highlight, palette.panel_shadow);

    // Draw dark panel nine-slice (row 1)
    draw_nine_slice_tiles(pixels, pitch, PANEL_DARK_START,
                         palette.dark_bg, palette.dark_border,
                         palette.panel_highlight, palette.panel_shadow);

    // Draw button normal nine-slice (row 2)
    draw_nine_slice_tiles(pixels, pitch, BUTTON_NORMAL_START,
                         palette.button_bg, palette.panel_border,
                         palette.panel_highlight, palette.panel_shadow);

    // Draw button hover nine-slice (row 3)
    draw_nine_slice_tiles(pixels, pitch, BUTTON_HOVER_START,
                         palette.button_hover, palette.panel_border,
                         palette.panel_highlight, palette.panel_shadow);

    // Draw button pressed nine-slice (row 4)
    draw_nine_slice_tiles(pixels, pitch, BUTTON_PRESSED_START,
                         palette.button_pressed, palette.dark_border,
                         palette.panel_shadow, palette.panel_highlight);

    // Draw font glyphs
    draw_font_glyphs(pixels, pitch, palette.text_color);

    SDL_UnlockSurface(surface);

    // Create texture from surface
    SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer.get(), surface);
    SDL_DestroySurface(surface);

    if (!tex) {
        return std::nullopt;
    }

    // Wrap in sdlpp::texture
    return sdlpp::texture(tex);
}

tile_theme procedural_atlas::create_theme(tile_atlas* atlas) {
    auto font = create_font(atlas);

    return tile_theme{
        .atlas = atlas,
        .button = {
            .normal = button_normal_slice(),
            .hover = button_hover_slice(),
            .pressed = button_pressed_slice(),
            .disabled = button_normal_slice(),  // Use normal for disabled
            .focused = button_hover_slice()      // Use hover for focused
        },
        .panel = {.background = light_panel_slice()},
        .label = {},
        .progress_bar = {},
        .scrollbar = {},
        .slider = {},
        .checkbox = {},
        .radio = {},
        .tab = {},
        .text_input = {
            .normal = light_panel_slice(),
            .focused = button_hover_slice(),
            .disabled = light_panel_slice()
        },
        .combo = {},
        .list = {
            .background = light_panel_slice(),
            .item_hover = button_hover_slice(),
            .item_selected = button_pressed_slice()
        },
        .window = {},
        .menu = {},
        .tooltip = {.background = light_panel_slice()},
        .group_box = {.frame = light_panel_slice()},
        .font_normal = font,
        .font_disabled = font,  // Could create separate gray font
        .font_highlight = font,
        .font_title = font,
        .font_small = font,
        .animations = {}
    };
}

} // namespace onyxui::tile
