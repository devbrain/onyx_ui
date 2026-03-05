/**
 * @file test_tile_renderer.cc
 * @brief Unit tests for tile_renderer
 *
 * Note: These tests verify the tile_renderer interface and logic.
 * Actual rendering tests require SDL and are done in integration tests.
 */

#include <doctest/doctest.h>
#include <onyxui/tile/tile_renderer.hh>

using namespace onyxui::tile;

// ============================================================================
// tile_renderer::tile_rect Tests
// ============================================================================

TEST_CASE("tile_renderer::tile_rect - default construction") {
    tile_renderer::tile_rect r;
    CHECK(r.x == 0);
    CHECK(r.y == 0);
    CHECK(r.width == 0);
    CHECK(r.height == 0);
}

TEST_CASE("tile_renderer::tile_rect - designated initializer") {
    tile_renderer::tile_rect r{.x = 10, .y = 20, .width = 100, .height = 50};
    CHECK(r.x == 10);
    CHECK(r.y == 20);
    CHECK(r.width == 100);
    CHECK(r.height == 50);
}

// ============================================================================
// tile_renderer::tile_point Tests
// ============================================================================

TEST_CASE("tile_renderer::tile_point - default construction") {
    tile_renderer::tile_point p;
    CHECK(p.x == 0);
    CHECK(p.y == 0);
}

TEST_CASE("tile_renderer::tile_point - designated initializer") {
    tile_renderer::tile_point p{.x = 42, .y = 84};
    CHECK(p.x == 42);
    CHECK(p.y == 84);
}

// ============================================================================
// tile_atlas Integration Tests
// ============================================================================

TEST_CASE("tile_atlas - coordinates for nine_slice layout") {
    // Simulate a typical 9-slice setup in a 16x16 atlas with 8 columns
    tile_atlas atlas{
        .texture = nullptr,
        .tile_width = 16,
        .tile_height = 16,
        .columns = 8
    };

    // Nine-slice tiles arranged as:
    // Row 0: 0,1,2 (top-left, top, top-right)
    // Row 1: 8,9,10 (left, center, right)
    // Row 2: 16,17,18 (bottom-left, bottom, bottom-right)

    // Verify top row positions
    CHECK(atlas.source_x(0) == 0);    // top-left
    CHECK(atlas.source_y(0) == 0);
    CHECK(atlas.source_x(1) == 16);   // top
    CHECK(atlas.source_y(1) == 0);
    CHECK(atlas.source_x(2) == 32);   // top-right
    CHECK(atlas.source_y(2) == 0);

    // Verify middle row positions
    CHECK(atlas.source_x(8) == 0);    // left
    CHECK(atlas.source_y(8) == 16);
    CHECK(atlas.source_x(9) == 16);   // center
    CHECK(atlas.source_y(9) == 16);
    CHECK(atlas.source_x(10) == 32);  // right
    CHECK(atlas.source_y(10) == 16);

    // Verify bottom row positions
    CHECK(atlas.source_x(16) == 0);   // bottom-left
    CHECK(atlas.source_y(16) == 32);
    CHECK(atlas.source_x(17) == 16);  // bottom
    CHECK(atlas.source_y(17) == 32);
    CHECK(atlas.source_x(18) == 32);  // bottom-right
    CHECK(atlas.source_y(18) == 32);
}

TEST_CASE("tile_atlas - font glyph positions") {
    // Simulate a font starting at tile 256 in the atlas
    tile_atlas atlas{
        .texture = nullptr,
        .tile_width = 8,
        .tile_height = 12,
        .columns = 16
    };

    bitmap_font font{
        .atlas = &atlas,
        .glyph_width = 8,
        .glyph_height = 12,
        .first_char = 32,
        .char_count = 96,
        .first_tile_id = 256
    };

    // Space (tile 256) should be at row 16, column 0
    CHECK(atlas.row(256) == 16);
    CHECK(atlas.column(256) == 0);

    // 'A' (tile 289) = 256 + 33
    int a_tile = font.glyph_id('A');
    CHECK(a_tile == 289);
    CHECK(atlas.row(a_tile) == 18);    // 289 / 16 = 18
    CHECK(atlas.column(a_tile) == 1);  // 289 % 16 = 1
}

// ============================================================================
// Nine-Slice Layout Calculation Tests
// ============================================================================

TEST_CASE("nine_slice - layout with margins") {
    nine_slice slice{
        .top_left = 0, .top = 1, .top_right = 2,
        .left = 8, .center = 9, .right = 10,
        .bottom_left = 16, .bottom = 17, .bottom_right = 18,
        .margin_h = 16,  // 16px margins horizontally
        .margin_v = 16   // 16px margins vertically
    };

    // For a 200x100 bounds:
    // - Corners: 16x16 each
    // - Center: (200 - 32) x (100 - 32) = 168 x 68
    tile_renderer::tile_rect bounds{0, 0, 200, 100};

    int center_x = bounds.x + slice.margin_h;
    int center_y = bounds.y + slice.margin_v;
    int center_w = bounds.width - 2 * slice.margin_h;
    int center_h = bounds.height - 2 * slice.margin_v;

    CHECK(center_x == 16);
    CHECK(center_y == 16);
    CHECK(center_w == 168);
    CHECK(center_h == 68);
}

TEST_CASE("h_slice - layout with margins") {
    h_slice slice{
        .left = 48,
        .center = 49,
        .right = 50,
        .margin = 8  // 8px caps on each end
    };

    tile_renderer::tile_rect bounds{0, 0, 100, 16};

    int left_x = bounds.x;
    int left_w = slice.margin;
    int center_x = bounds.x + slice.margin;
    int center_w = bounds.width - 2 * slice.margin;
    int right_x = bounds.x + bounds.width - slice.margin;
    int right_w = slice.margin;

    CHECK(left_x == 0);
    CHECK(left_w == 8);
    CHECK(center_x == 8);
    CHECK(center_w == 84);
    CHECK(right_x == 92);
    CHECK(right_w == 8);
}

TEST_CASE("v_slice - layout with margins") {
    v_slice slice{
        .top = 60,
        .center = 61,
        .bottom = 62,
        .margin = 4  // 4px caps on each end
    };

    tile_renderer::tile_rect bounds{0, 0, 16, 50};

    int top_y = bounds.y;
    int top_h = slice.margin;
    int center_y = bounds.y + slice.margin;
    int center_h = bounds.height - 2 * slice.margin;
    int bottom_y = bounds.y + bounds.height - slice.margin;
    int bottom_h = slice.margin;

    CHECK(top_y == 0);
    CHECK(top_h == 4);
    CHECK(center_y == 4);
    CHECK(center_h == 42);
    CHECK(bottom_y == 46);
    CHECK(bottom_h == 4);
}

// ============================================================================
// Text Layout Calculation Tests
// ============================================================================

TEST_CASE("bitmap_font - text centering calculation") {
    tile_atlas atlas{
        .texture = nullptr,
        .tile_width = 8,
        .tile_height = 12,
        .columns = 16
    };

    bitmap_font font{
        .atlas = &atlas,
        .glyph_width = 8,
        .glyph_height = 12
    };

    std::string_view text = "Hello";
    tile_renderer::tile_rect bounds{0, 0, 100, 50};

    int text_width = font.text_width(text);
    int text_height = font.glyph_height;

    int centered_x = bounds.x + (bounds.width - text_width) / 2;
    int centered_y = bounds.y + (bounds.height - text_height) / 2;

    CHECK(text_width == 40);  // 5 chars * 8px
    CHECK(text_height == 12);
    CHECK(centered_x == 30);  // (100 - 40) / 2
    CHECK(centered_y == 19);  // (50 - 12) / 2
}

// ============================================================================
// Progress Bar Fill Calculation Tests
// ============================================================================

TEST_CASE("progress bar - fill width calculation") {
    h_slice track{.left = 0, .center = 1, .right = 2, .margin = 4};
    h_slice fill{.left = 3, .center = 4, .right = 5, .margin = 4};

    tile_renderer::tile_rect bounds{0, 0, 100, 16};
    float progress = 0.75f;  // 75%

    // Fill should be 75% of the track width
    int fill_width = static_cast<int>(bounds.width * progress);
    CHECK(fill_width == 75);

    // Calculate fill parts
    int fill_left_w = fill.margin;
    int fill_center_w = fill_width - 2 * fill.margin;
    int fill_right_x = fill_width - fill.margin;

    CHECK(fill_left_w == 4);
    CHECK(fill_center_w == 67);
    CHECK(fill_right_x == 71);
}

// ============================================================================
// Edge Cases Tests
// ============================================================================

TEST_CASE("nine_slice - bounds too small for margins") {
    nine_slice slice{
        .top_left = 0, .top = 1, .top_right = 2,
        .left = 8, .center = 9, .right = 10,
        .bottom_left = 16, .bottom = 17, .bottom_right = 18,
        .margin_h = 16,
        .margin_v = 16
    };

    // Bounds smaller than margins (20x20, margins need 32x32)
    tile_renderer::tile_rect bounds{0, 0, 20, 20};

    int center_w = bounds.width - 2 * slice.margin_h;
    int center_h = bounds.height - 2 * slice.margin_v;

    CHECK(center_w < 0);  // Would be -12
    CHECK(center_h < 0);  // Would be -12
    // Renderer should skip drawing if center is negative
}

TEST_CASE("h_slice - single pixel center") {
    h_slice slice{.left = 0, .center = 1, .right = 2, .margin = 4};

    // Bounds exactly fit margins + 1 pixel center
    tile_renderer::tile_rect bounds{0, 0, 9, 8};

    int center_w = bounds.width - 2 * slice.margin;
    CHECK(center_w == 1);
}

TEST_CASE("bitmap_font - empty text") {
    tile_atlas atlas{
        .texture = nullptr,
        .tile_width = 8,
        .tile_height = 12,
        .columns = 16
    };

    bitmap_font font{
        .atlas = &atlas,
        .glyph_width = 8,
        .glyph_height = 12
    };

    CHECK(font.text_width("") == 0);
    CHECK(font.text_width(std::string_view{}) == 0);
}

// ============================================================================
// Font Atlas Texture Scenario Tests
// ============================================================================
// These tests verify the expected configurations for font rendering.
// The tile_renderer::draw_bitmap_text behavior depends on these scenarios:
//
// Scenario 1: Shared atlas (font.atlas == renderer atlas)
//   - Font uses same atlas pointer as UI tiles
//   - No texture swap needed
//   - Expected: Works correctly
//
// Scenario 2: Separate atlas WITH texture
//   - Font has its own atlas with atlas->texture set
//   - Texture swap required
//   - Expected: Works correctly
//
// Scenario 3: Separate atlas WITHOUT texture
//   - Font has its own atlas but atlas->texture is nullptr
//   - Cannot render correctly (wrong texture with wrong coordinates)
//   - Expected: draw_bitmap_text skips rendering

TEST_CASE("Font atlas scenario - shared atlas configuration") {
    // Scenario 1: Font shares the same atlas as UI tiles
    int dummy_texture = 42;  // Simulates a valid texture pointer

    tile_atlas shared_atlas{
        .texture = &dummy_texture,
        .tile_width = 16,
        .tile_height = 16,
        .columns = 16
    };

    // Font points to the SAME atlas
    bitmap_font font{
        .atlas = &shared_atlas,  // Same atlas pointer
        .glyph_width = 8,
        .glyph_height = 12,
        .first_char = 32,
        .char_count = 96,
        .first_tile_id = 256
    };

    // Verify configuration is correct for shared atlas scenario
    CHECK(font.atlas == &shared_atlas);
    CHECK(font.atlas->texture != nullptr);
    CHECK(font.is_valid());

    // This configuration should work: same atlas, same texture
    // No texture swap needed in draw_bitmap_text
}

TEST_CASE("Font atlas scenario - separate atlas with texture") {
    // Scenario 2: Font has its own atlas with its own texture
    int ui_texture = 42;
    int font_texture = 84;

    tile_atlas ui_atlas{
        .texture = &ui_texture,
        .tile_width = 16,
        .tile_height = 16,
        .columns = 16
    };

    tile_atlas font_atlas{
        .texture = &font_texture,  // Different texture
        .tile_width = 8,
        .tile_height = 12,
        .columns = 16
    };

    bitmap_font font{
        .atlas = &font_atlas,  // Different atlas pointer
        .glyph_width = 8,
        .glyph_height = 12,
        .first_char = 32,
        .char_count = 96,
        .first_tile_id = 0
    };

    // Verify separate atlas configuration
    CHECK(font.atlas != &ui_atlas);
    CHECK(font.atlas->texture != nullptr);
    CHECK(font.atlas->texture != ui_atlas.texture);
    CHECK(font.is_valid());

    // This configuration should work: draw_bitmap_text will swap
    // both atlas AND texture to render correctly
}

TEST_CASE("Font atlas scenario - separate atlas WITHOUT texture (error case)") {
    // Scenario 3: Font has separate atlas but NO texture
    // This is a configuration error that draw_bitmap_text should detect
    int ui_texture = 42;

    tile_atlas ui_atlas{
        .texture = &ui_texture,
        .tile_width = 16,
        .tile_height = 16,
        .columns = 16
    };

    tile_atlas font_atlas{
        .texture = nullptr,  // NO TEXTURE - error case
        .tile_width = 8,
        .tile_height = 12,
        .columns = 16
    };

    bitmap_font font{
        .atlas = &font_atlas,
        .glyph_width = 8,
        .glyph_height = 12,
        .first_char = 32,
        .char_count = 96,
        .first_tile_id = 0
    };

    // Verify error configuration
    CHECK(font.atlas != &ui_atlas);
    CHECK(font.atlas->texture == nullptr);  // Missing texture

    // is_valid() returns true because atlas dimensions are valid
    // But rendering would fail - draw_bitmap_text should skip this
    CHECK(font.is_valid());

    // User should use atlas->is_ready() to check for texture
    CHECK_FALSE(font.atlas->is_ready());

    // Documentation note: draw_bitmap_text will skip rendering for this
    // configuration to avoid garbage output (using wrong texture with
    // font atlas coordinates)
}

TEST_CASE("tile_atlas - is_ready vs is_valid") {
    // is_valid() checks dimensions only
    // is_ready() checks dimensions AND texture pointer

    tile_atlas valid_but_not_ready{
        .texture = nullptr,  // No texture
        .tile_width = 16,
        .tile_height = 16,
        .columns = 16
    };

    CHECK(valid_but_not_ready.is_valid());      // Dimensions OK
    CHECK_FALSE(valid_but_not_ready.is_ready()); // No texture

    int dummy = 1;
    tile_atlas fully_ready{
        .texture = &dummy,  // Has texture
        .tile_width = 16,
        .tile_height = 16,
        .columns = 16
    };

    CHECK(fully_ready.is_valid());  // Dimensions OK
    CHECK(fully_ready.is_ready());  // AND has texture
}

TEST_CASE("bitmap_font - atlas pointer equality check") {
    // This test verifies the pointer comparison used in draw_bitmap_text
    // to detect shared vs separate atlas

    int texture = 42;
    tile_atlas atlas{
        .texture = &texture,
        .tile_width = 16,
        .tile_height = 16,
        .columns = 16
    };

    bitmap_font font1{.atlas = &atlas};
    bitmap_font font2{.atlas = &atlas};

    // Both fonts share the same atlas
    CHECK(font1.atlas == font2.atlas);
    CHECK(font1.atlas == &atlas);

    // Create a copy of atlas (different pointer, same values)
    tile_atlas atlas_copy = atlas;
    bitmap_font font3{.atlas = &atlas_copy};

    // Different atlas pointers (even with same values)
    CHECK(font3.atlas != &atlas);
    CHECK(font3.atlas == &atlas_copy);
}

// ============================================================================
// Font Rendering Decision Tests
// ============================================================================
// These tests verify the logic that draw_bitmap_text uses to decide
// whether to render text (and avoid garbage output).

TEST_CASE("Font rendering decision - shared atlas with texture") {
    // When font uses the same atlas as the renderer AND that atlas has
    // a texture, rendering should proceed normally.

    int texture = 42;
    tile_atlas shared_atlas{
        .texture = &texture,
        .tile_width = 8,
        .tile_height = 12,
        .columns = 16
    };

    bitmap_font font{
        .atlas = &shared_atlas,
        .glyph_width = 8,
        .glyph_height = 12,
        .first_char = 32,
        .char_count = 96,
        .first_tile_id = 0
    };

    // Prerequisites for rendering:
    // 1. Font has valid atlas
    CHECK(font.atlas != nullptr);
    // 2. Atlas has texture
    CHECK(font.atlas->texture != nullptr);
    // 3. Font is valid
    CHECK(font.is_valid());
    // 4. Atlas is ready
    CHECK(font.atlas->is_ready());

    // All conditions met - rendering should proceed
}

TEST_CASE("Font rendering decision - separate atlas with texture") {
    // When font has its own atlas with its own texture,
    // draw_bitmap_text should swap both atlas AND texture.

    int ui_texture = 1;
    int font_texture = 2;

    tile_atlas ui_atlas{
        .texture = &ui_texture,
        .tile_width = 16,
        .tile_height = 16,
        .columns = 16
    };

    tile_atlas font_atlas{
        .texture = &font_texture,
        .tile_width = 8,
        .tile_height = 12,
        .columns = 16
    };

    bitmap_font font{
        .atlas = &font_atlas,
        .glyph_width = 8,
        .glyph_height = 12,
        .first_char = 32,
        .char_count = 96,
        .first_tile_id = 0
    };

    // Verify configuration
    CHECK(font.atlas != &ui_atlas);  // Different atlas
    CHECK(font.atlas->texture != nullptr);  // Font atlas has texture
    CHECK(font.atlas->texture != ui_atlas.texture);  // Different textures

    // draw_bitmap_text implementation should:
    // 1. Save current atlas/texture
    // 2. Set font's atlas and texture
    // 3. Render glyphs
    // 4. Restore original atlas/texture
}

TEST_CASE("Font rendering decision - separate atlas WITHOUT texture (skip)") {
    // When font has separate atlas but NO texture, rendering should skip
    // to avoid garbage output (using renderer's texture with wrong coords).

    int ui_texture = 1;

    tile_atlas ui_atlas{
        .texture = &ui_texture,
        .tile_width = 16,
        .tile_height = 16,
        .columns = 16
    };

    tile_atlas font_atlas{
        .texture = nullptr,  // NO TEXTURE
        .tile_width = 8,
        .tile_height = 12,
        .columns = 16
    };

    bitmap_font font{
        .atlas = &font_atlas,
        .glyph_width = 8,
        .glyph_height = 12,
        .first_char = 32,
        .char_count = 96,
        .first_tile_id = 0
    };

    // Verify error condition
    CHECK(font.atlas != nullptr);  // Has atlas
    CHECK(font.atlas->texture == nullptr);  // But no texture!

    // draw_bitmap_text should detect this and skip rendering
    // The implementation checks: if (!font.atlas->texture) return;
    CHECK_FALSE(font.atlas->is_ready());  // User can check this
}

TEST_CASE("Font rendering decision - null atlas (skip)") {
    // When font has no atlas at all, rendering should skip.

    bitmap_font font{
        .atlas = nullptr,  // NO ATLAS
        .glyph_width = 8,
        .glyph_height = 12,
        .first_char = 32,
        .char_count = 96,
        .first_tile_id = 0
    };

    // draw_bitmap_text should detect this and skip
    // The implementation checks: if (!font.atlas) return;
    CHECK(font.atlas == nullptr);
    CHECK_FALSE(font.is_valid());  // User can check this
}

TEST_CASE("Font rendering decision - empty text (skip)") {
    // Empty text should be handled gracefully.

    int texture = 42;
    tile_atlas atlas{
        .texture = &texture,
        .tile_width = 8,
        .tile_height = 12,
        .columns = 16
    };

    bitmap_font font{
        .atlas = &atlas,
        .glyph_width = 8,
        .glyph_height = 12
    };

    // Valid font but empty text
    CHECK(font.is_valid());
    CHECK(font.atlas->is_ready());

    // Text width for empty string should be 0
    CHECK(font.text_width("") == 0);
    CHECK(font.text_width(std::string_view{}) == 0);

    // draw_bitmap_text checks: if (text.empty()) return;
}

// ============================================================================
// Font Atlas Texture State Transitions
// ============================================================================

TEST_CASE("Font atlas - texture state transitions") {
    // Test that atlas can transition from not-ready to ready
    // This simulates async texture loading.

    tile_atlas atlas{
        .texture = nullptr,  // Initially no texture
        .tile_width = 8,
        .tile_height = 12,
        .columns = 16
    };

    // Before texture load
    CHECK(atlas.is_valid());  // Dimensions valid
    CHECK_FALSE(atlas.is_ready());  // Not ready (no texture)

    // Simulate texture load
    int loaded_texture = 123;
    atlas.texture = &loaded_texture;

    // After texture load
    CHECK(atlas.is_valid());  // Still valid
    CHECK(atlas.is_ready());  // Now ready

    // Font using this atlas should work now
    bitmap_font font{.atlas = &atlas};
    CHECK(font.atlas->is_ready());
}

TEST_CASE("Font atlas - texture invalidation") {
    // Test handling when texture becomes invalid.

    int texture = 42;
    tile_atlas atlas{
        .texture = &texture,
        .tile_width = 8,
        .tile_height = 12,
        .columns = 16
    };

    CHECK(atlas.is_ready());

    // Simulate texture destruction
    atlas.texture = nullptr;

    CHECK(atlas.is_valid());  // Dimensions still valid
    CHECK_FALSE(atlas.is_ready());  // But no longer ready
}

// ============================================================================
// Multiple Font Support
// ============================================================================

TEST_CASE("Multiple fonts - different atlases") {
    // Verify multiple fonts with different atlases can coexist.

    int normal_texture = 1;
    int bold_texture = 2;
    int icon_texture = 3;

    tile_atlas normal_atlas{
        .texture = &normal_texture,
        .tile_width = 8,
        .tile_height = 12,
        .columns = 16
    };

    tile_atlas bold_atlas{
        .texture = &bold_texture,
        .tile_width = 10,
        .tile_height = 14,
        .columns = 16
    };

    tile_atlas icon_atlas{
        .texture = &icon_texture,
        .tile_width = 16,
        .tile_height = 16,
        .columns = 16
    };

    bitmap_font normal_font{
        .atlas = &normal_atlas,
        .glyph_width = 8,
        .glyph_height = 12,
        .first_char = 32,
        .char_count = 96,
        .first_tile_id = 0
    };

    bitmap_font bold_font{
        .atlas = &bold_atlas,
        .glyph_width = 10,
        .glyph_height = 14,
        .first_char = 32,
        .char_count = 96,
        .first_tile_id = 0
    };

    bitmap_font icon_font{
        .atlas = &icon_atlas,
        .glyph_width = 16,
        .glyph_height = 16,
        .first_char = 0,
        .char_count = 128,
        .first_tile_id = 0
    };

    // All fonts should be valid and ready
    CHECK(normal_font.is_valid());
    CHECK(normal_font.atlas->is_ready());

    CHECK(bold_font.is_valid());
    CHECK(bold_font.atlas->is_ready());

    CHECK(icon_font.is_valid());
    CHECK(icon_font.atlas->is_ready());

    // Each font has different dimensions
    CHECK(normal_font.glyph_width == 8);
    CHECK(bold_font.glyph_width == 10);
    CHECK(icon_font.glyph_width == 16);

    // Each font has different atlas
    CHECK(normal_font.atlas != bold_font.atlas);
    CHECK(bold_font.atlas != icon_font.atlas);
}

TEST_CASE("Multiple fonts - shared atlas") {
    // Verify multiple fonts can share the same atlas (different tile ranges).

    int shared_texture = 42;
    tile_atlas shared_atlas{
        .texture = &shared_texture,
        .tile_width = 8,
        .tile_height = 12,
        .columns = 16
    };

    // Normal ASCII font starting at tile 0
    bitmap_font ascii_font{
        .atlas = &shared_atlas,
        .glyph_width = 8,
        .glyph_height = 12,
        .first_char = 32,
        .char_count = 96,
        .first_tile_id = 0
    };

    // Extended characters starting at tile 96
    bitmap_font extended_font{
        .atlas = &shared_atlas,
        .glyph_width = 8,
        .glyph_height = 12,
        .first_char = 128,
        .char_count = 128,
        .first_tile_id = 96
    };

    // Both fonts share same atlas
    CHECK(ascii_font.atlas == extended_font.atlas);

    // But have different tile ranges
    CHECK(ascii_font.first_tile_id == 0);
    CHECK(extended_font.first_tile_id == 96);

    // Glyph IDs should be in different ranges
    CHECK(ascii_font.glyph_id('A') == 33);   // 'A' - 32 + 0 = 33
    CHECK(extended_font.glyph_id(static_cast<char>(128)) == 96);  // 128 - 128 + 96 = 96
}
