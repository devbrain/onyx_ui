/**
 * @file test_tile_types.cc
 * @brief Unit tests for tile backend core types
 */

#include <doctest/doctest.h>
#include <onyxui/tile/tile_types.hh>

using namespace onyxui::tile;

// ============================================================================
// h_slice Tests
// ============================================================================

TEST_CASE("h_slice - default construction") {
    h_slice slice;
    CHECK(slice.left == -1);
    CHECK(slice.center == -1);
    CHECK(slice.right == -1);
    CHECK(slice.margin == 0);
    CHECK_FALSE(slice.is_valid());
}

TEST_CASE("h_slice - designated initializer") {
    h_slice slice{
        .left = 10,
        .center = 11,
        .right = 12,
        .margin = 8
    };
    CHECK(slice.left == 10);
    CHECK(slice.center == 11);
    CHECK(slice.right == 12);
    CHECK(slice.margin == 8);
    CHECK(slice.is_valid());
}

TEST_CASE("h_slice - validity requires center tile") {
    h_slice slice{.left = 0, .center = -1, .right = 2};
    CHECK_FALSE(slice.is_valid());

    slice.center = 1;
    CHECK(slice.is_valid());
}

// ============================================================================
// v_slice Tests
// ============================================================================

TEST_CASE("v_slice - default construction") {
    v_slice slice;
    CHECK(slice.top == -1);
    CHECK(slice.center == -1);
    CHECK(slice.bottom == -1);
    CHECK(slice.margin == 0);
    CHECK_FALSE(slice.is_valid());
}

TEST_CASE("v_slice - designated initializer") {
    v_slice slice{
        .top = 5,
        .center = 6,
        .bottom = 7,
        .margin = 4
    };
    CHECK(slice.top == 5);
    CHECK(slice.center == 6);
    CHECK(slice.bottom == 7);
    CHECK(slice.margin == 4);
    CHECK(slice.is_valid());
}

TEST_CASE("v_slice - validity requires center tile") {
    v_slice slice{.top = 0, .center = -1, .bottom = 2};
    CHECK_FALSE(slice.is_valid());

    slice.center = 1;
    CHECK(slice.is_valid());
}

// ============================================================================
// nine_slice Tests
// ============================================================================

TEST_CASE("nine_slice - default construction") {
    nine_slice slice;
    CHECK(slice.top_left == -1);
    CHECK(slice.top == -1);
    CHECK(slice.top_right == -1);
    CHECK(slice.left == -1);
    CHECK(slice.center == -1);
    CHECK(slice.right == -1);
    CHECK(slice.bottom_left == -1);
    CHECK(slice.bottom == -1);
    CHECK(slice.bottom_right == -1);
    CHECK(slice.margin_h == 0);
    CHECK(slice.margin_v == 0);
    CHECK_FALSE(slice.is_valid());
    CHECK_FALSE(slice.has_center());
}

TEST_CASE("nine_slice - designated initializer") {
    nine_slice slice{
        .top_left = 0, .top = 1, .top_right = 2,
        .left = 8, .center = 9, .right = 10,
        .bottom_left = 16, .bottom = 17, .bottom_right = 18,
        .margin_h = 8,
        .margin_v = 8
    };

    CHECK(slice.top_left == 0);
    CHECK(slice.top == 1);
    CHECK(slice.top_right == 2);
    CHECK(slice.left == 8);
    CHECK(slice.center == 9);
    CHECK(slice.right == 10);
    CHECK(slice.bottom_left == 16);
    CHECK(slice.bottom == 17);
    CHECK(slice.bottom_right == 18);
    CHECK(slice.margin_h == 8);
    CHECK(slice.margin_v == 8);
    CHECK(slice.is_valid());
    CHECK(slice.has_center());
}

TEST_CASE("nine_slice - validity requires corner tiles") {
    nine_slice slice{
        .top_left = 0, .top_right = 2,
        .bottom_left = 16, .bottom_right = 18
    };
    CHECK(slice.is_valid());

    slice.top_left = -1;
    CHECK_FALSE(slice.is_valid());
}

TEST_CASE("nine_slice - center is optional (transparent)") {
    nine_slice slice{
        .top_left = 0, .top = 1, .top_right = 2,
        .left = 8, .center = -1, .right = 10,  // No center
        .bottom_left = 16, .bottom = 17, .bottom_right = 18
    };
    CHECK(slice.is_valid());
    CHECK_FALSE(slice.has_center());
}

// ============================================================================
// tile_atlas Tests
// ============================================================================

TEST_CASE("tile_atlas - default construction") {
    tile_atlas atlas;
    CHECK(atlas.texture == nullptr);
    CHECK(atlas.tile_width == 16);
    CHECK(atlas.tile_height == 16);
    CHECK(atlas.columns == 16);
    // is_valid() only checks dimensions, not texture (texture checked at render time)
    CHECK(atlas.is_valid());
}

TEST_CASE("tile_atlas - designated initializer") {
    int dummy_texture = 42;
    tile_atlas atlas{
        .texture = &dummy_texture,
        .tile_width = 8,
        .tile_height = 8,
        .columns = 32
    };
    CHECK(atlas.texture == &dummy_texture);
    CHECK(atlas.tile_width == 8);
    CHECK(atlas.tile_height == 8);
    CHECK(atlas.columns == 32);
}

TEST_CASE("tile_atlas - source_x calculation") {
    tile_atlas atlas{
        .texture = nullptr,
        .tile_width = 16,
        .tile_height = 16,
        .columns = 8
    };

    // First row
    CHECK(atlas.source_x(0) == 0);
    CHECK(atlas.source_x(1) == 16);
    CHECK(atlas.source_x(7) == 112);  // 7 * 16

    // Second row wraps
    CHECK(atlas.source_x(8) == 0);    // Column 0
    CHECK(atlas.source_x(9) == 16);   // Column 1
    CHECK(atlas.source_x(10) == 32);  // Column 2
}

TEST_CASE("tile_atlas - source_y calculation") {
    tile_atlas atlas{
        .texture = nullptr,
        .tile_width = 16,
        .tile_height = 16,
        .columns = 8
    };

    // First row
    CHECK(atlas.source_y(0) == 0);
    CHECK(atlas.source_y(7) == 0);

    // Second row
    CHECK(atlas.source_y(8) == 16);
    CHECK(atlas.source_y(15) == 16);

    // Third row
    CHECK(atlas.source_y(16) == 32);
    CHECK(atlas.source_y(23) == 32);
}

TEST_CASE("tile_atlas - column and row helpers") {
    tile_atlas atlas{
        .texture = nullptr,
        .tile_width = 16,
        .tile_height = 16,
        .columns = 8
    };

    CHECK(atlas.column(0) == 0);
    CHECK(atlas.column(5) == 5);
    CHECK(atlas.column(8) == 0);   // Wraps to column 0
    CHECK(atlas.column(10) == 2);  // 10 % 8 = 2

    CHECK(atlas.row(0) == 0);
    CHECK(atlas.row(7) == 0);
    CHECK(atlas.row(8) == 1);
    CHECK(atlas.row(16) == 2);
    CHECK(atlas.row(25) == 3);  // 25 / 8 = 3
}

TEST_CASE("tile_atlas - combined source coordinates") {
    tile_atlas atlas{
        .texture = nullptr,
        .tile_width = 16,
        .tile_height = 16,
        .columns = 8
    };

    // Tile 10: row 1, column 2
    CHECK(atlas.source_x(10) == 32);   // 2 * 16
    CHECK(atlas.source_y(10) == 16);   // 1 * 16

    // Tile 27: row 3, column 3
    CHECK(atlas.source_x(27) == 48);   // 3 * 16
    CHECK(atlas.source_y(27) == 48);   // 3 * 16
}

TEST_CASE("tile_atlas - non-square tiles") {
    tile_atlas atlas{
        .texture = nullptr,
        .tile_width = 8,
        .tile_height = 12,
        .columns = 16
    };

    // Tile 20: row 1, column 4
    CHECK(atlas.source_x(20) == 32);   // 4 * 8
    CHECK(atlas.source_y(20) == 12);   // 1 * 12
}

TEST_CASE("tile_atlas - validity check") {
    tile_atlas atlas;

    // Invalid: zero dimensions
    atlas.tile_width = 0;
    CHECK_FALSE(atlas.is_valid());

    atlas.tile_width = 16;
    atlas.tile_height = 0;
    CHECK_FALSE(atlas.is_valid());

    atlas.tile_height = 16;
    atlas.columns = 0;
    CHECK_FALSE(atlas.is_valid());

    atlas.columns = 8;
    CHECK(atlas.is_valid());
}

// ============================================================================
// bitmap_font Tests
// ============================================================================

TEST_CASE("bitmap_font - default construction") {
    bitmap_font font;
    CHECK(font.atlas == nullptr);
    CHECK(font.glyph_width == 8);
    CHECK(font.glyph_height == 8);
    CHECK(font.first_char == 32);
    CHECK(font.char_count == 96);
    CHECK(font.first_tile_id == 0);
    CHECK_FALSE(font.is_valid());  // No atlas
}

TEST_CASE("bitmap_font - glyph_id calculation") {
    tile_atlas atlas{.texture = reinterpret_cast<void*>(1), .columns = 16};
    bitmap_font font{
        .atlas = &atlas,
        .glyph_width = 8,
        .glyph_height = 12,
        .first_char = 32,
        .char_count = 96,
        .first_tile_id = 256
    };

    // Space (ASCII 32) -> first tile
    CHECK(font.glyph_id(' ') == 256);

    // 'A' (ASCII 65) -> 65 - 32 = 33rd character
    CHECK(font.glyph_id('A') == 256 + 33);

    // '0' (ASCII 48) -> 48 - 32 = 16th character
    CHECK(font.glyph_id('0') == 256 + 16);

    // '~' (ASCII 126) -> 126 - 32 = 94th character (last in standard set)
    CHECK(font.glyph_id('~') == 256 + 94);
}

TEST_CASE("bitmap_font - glyph_id returns -1 for invalid characters") {
    tile_atlas atlas{.texture = reinterpret_cast<void*>(1), .columns = 16};
    bitmap_font font{
        .atlas = &atlas,
        .first_char = 32,
        .char_count = 96,  // Covers ASCII 32-127 (space to DEL)
        .first_tile_id = 0
    };

    // Characters before first_char
    CHECK(font.glyph_id('\0') == -1);
    CHECK(font.glyph_id('\n') == -1);
    CHECK(font.glyph_id('\t') == -1);
    CHECK(font.glyph_id(31) == -1);

    // DEL (127) is within range: 127 - 32 = 95, which is < 96
    CHECK(font.glyph_id(127) == 95);  // DEL is the last valid character

    // Characters after range (32 + 96 = 128)
    CHECK(font.glyph_id(static_cast<char>(128)) == -1);
    CHECK(font.glyph_id(static_cast<char>(200)) == -1);
}

TEST_CASE("bitmap_font - has_glyph") {
    tile_atlas atlas{.texture = reinterpret_cast<void*>(1), .columns = 16};
    bitmap_font font{
        .atlas = &atlas,
        .first_char = 32,
        .char_count = 96,  // Covers ASCII 32-127
        .first_tile_id = 0
    };

    CHECK(font.has_glyph(' '));
    CHECK(font.has_glyph('A'));
    CHECK(font.has_glyph('~'));
    CHECK(font.has_glyph(127));  // DEL is within range (32 + 95 = 127)
    CHECK_FALSE(font.has_glyph('\n'));
    CHECK_FALSE(font.has_glyph(static_cast<char>(128)));  // First out of range
}

TEST_CASE("bitmap_font - text_width") {
    tile_atlas atlas{.texture = reinterpret_cast<void*>(1), .columns = 16};
    bitmap_font font{
        .atlas = &atlas,
        .glyph_width = 8,
        .glyph_height = 12
    };

    CHECK(font.text_width("") == 0);
    CHECK(font.text_width("A") == 8);
    CHECK(font.text_width("Hello") == 40);
    CHECK(font.text_width("Hello, World!") == 104);  // 13 * 8
}

TEST_CASE("bitmap_font - custom first_char (numbers only)") {
    tile_atlas atlas{.texture = reinterpret_cast<void*>(1), .columns = 16};
    bitmap_font font{
        .atlas = &atlas,
        .first_char = '0',  // ASCII 48
        .char_count = 10,   // Only digits 0-9
        .first_tile_id = 100
    };

    CHECK(font.glyph_id('0') == 100);
    CHECK(font.glyph_id('5') == 105);
    CHECK(font.glyph_id('9') == 109);

    CHECK(font.has_glyph('0'));
    CHECK(font.has_glyph('9'));
    CHECK_FALSE(font.has_glyph('A'));
    CHECK_FALSE(font.has_glyph(' '));
}

TEST_CASE("bitmap_font - validity check") {
    tile_atlas valid_atlas{
        .texture = reinterpret_cast<void*>(1),
        .tile_width = 16,
        .tile_height = 16,
        .columns = 16
    };

    bitmap_font font{.atlas = &valid_atlas};

    CHECK(font.is_valid());

    // Invalid: no atlas
    font.atlas = nullptr;
    CHECK_FALSE(font.is_valid());

    // Invalid: zero glyph width
    font.atlas = &valid_atlas;
    font.glyph_width = 0;
    CHECK_FALSE(font.is_valid());

    // Invalid: zero glyph height
    font.glyph_width = 8;
    font.glyph_height = 0;
    CHECK_FALSE(font.is_valid());

    // Invalid: zero char count
    font.glyph_height = 8;
    font.char_count = 0;
    CHECK_FALSE(font.is_valid());

    font.char_count = 96;
    CHECK(font.is_valid());
}

// ============================================================================
// Constexpr Tests (compile-time verification)
// ============================================================================

TEST_CASE("tile_atlas - constexpr source calculations") {
    constexpr tile_atlas atlas{
        .texture = nullptr,
        .tile_width = 16,
        .tile_height = 16,
        .columns = 8
    };

    // These should be evaluated at compile time
    static_assert(atlas.source_x(10) == 32);
    static_assert(atlas.source_y(10) == 16);
    static_assert(atlas.column(10) == 2);
    static_assert(atlas.row(10) == 1);

    CHECK(true);  // If we got here, static_asserts passed
}

TEST_CASE("bitmap_font - constexpr glyph calculations") {
    // Note: can't use atlas pointer in constexpr context easily,
    // but glyph_id math itself is constexpr
    constexpr bitmap_font font{
        .atlas = nullptr,
        .first_char = 32,
        .char_count = 96,
        .first_tile_id = 256
    };

    static_assert(font.glyph_id(' ') == 256);
    static_assert(font.glyph_id('A') == 289);  // 256 + 33

    // text_width with string_view can't be constexpr with string literals
    // (string_view from const char* is not constexpr in all contexts)
    CHECK(font.text_width("Hello") == 40);

    CHECK(true);  // If we got here, static_asserts passed
}
