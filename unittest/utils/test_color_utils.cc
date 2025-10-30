//
// Unit tests for color_utils
//

#include <doctest/doctest.h>
#include <onyxui/utils/color_utils.hh>

using namespace onyxui::color_utils;

// ===========================================================================
// parse_hex_rgb() - RGB Colors (0xRRGGBB)
// ===========================================================================

TEST_CASE("color_utils::parse_hex_rgb - Basic colors") {
    // Test primary colors
    auto const red = parse_hex_rgb(0xFF0000);
    CHECK(red.r == 255);
    CHECK(red.g == 0);
    CHECK(red.b == 0);
    CHECK(red.a == 255); // RGB always has full opacity

    auto const green = parse_hex_rgb(0x00FF00);
    CHECK(green.r == 0);
    CHECK(green.g == 255);
    CHECK(green.b == 0);
    CHECK(green.a == 255);

    auto const blue = parse_hex_rgb(0x0000FF);
    CHECK(blue.r == 0);
    CHECK(blue.g == 0);
    CHECK(blue.b == 255);
    CHECK(blue.a == 255);
}

TEST_CASE("color_utils::parse_hex_rgb - Black and white") {
    auto const black = parse_hex_rgb(0x000000);
    CHECK(black.r == 0);
    CHECK(black.g == 0);
    CHECK(black.b == 0);
    CHECK(black.a == 255);

    auto const white = parse_hex_rgb(0xFFFFFF);
    CHECK(white.r == 255);
    CHECK(white.g == 255);
    CHECK(white.b == 255);
    CHECK(white.a == 255);
}

TEST_CASE("color_utils::parse_hex_rgb - Norton Blue theme color") {
    // Norton Commander blue: 0x0000AA (170 in decimal)
    auto const norton_blue = parse_hex_rgb(0x0000AA);
    CHECK(norton_blue.r == 0);
    CHECK(norton_blue.g == 0);
    CHECK(norton_blue.b == 170);
    CHECK(norton_blue.a == 255);
}

TEST_CASE("color_utils::parse_hex_rgb - Gray values") {
    auto const gray = parse_hex_rgb(0x808080);
    CHECK(gray.r == 128);
    CHECK(gray.g == 128);
    CHECK(gray.b == 128);
    CHECK(gray.a == 255);

    auto const dark_gray = parse_hex_rgb(0x555555);
    CHECK(dark_gray.r == 85);
    CHECK(dark_gray.g == 85);
    CHECK(dark_gray.b == 85);
    CHECK(dark_gray.a == 255);
}

TEST_CASE("color_utils::parse_hex_rgb - Arbitrary colors") {
    auto const orange = parse_hex_rgb(0xFF8800);
    CHECK(orange.r == 255);
    CHECK(orange.g == 136);
    CHECK(orange.b == 0);
    CHECK(orange.a == 255);

    auto const purple = parse_hex_rgb(0x8000FF);
    CHECK(purple.r == 128);
    CHECK(purple.g == 0);
    CHECK(purple.b == 255);
    CHECK(purple.a == 255);
}

// ===========================================================================
// parse_hex_rgba() - RGBA Colors (0xRRGGBBAA)
// ===========================================================================

TEST_CASE("color_utils::parse_hex_rgba - Basic colors with alpha") {
    // Red with full opacity
    auto const red_opaque = parse_hex_rgba(0xFF0000FF);
    CHECK(red_opaque.r == 255);
    CHECK(red_opaque.g == 0);
    CHECK(red_opaque.b == 0);
    CHECK(red_opaque.a == 255);

    // Green with half opacity
    auto const green_half = parse_hex_rgba(0x00FF0080);
    CHECK(green_half.r == 0);
    CHECK(green_half.g == 255);
    CHECK(green_half.b == 0);
    CHECK(green_half.a == 128);

    // Blue with zero opacity (fully transparent)
    auto const blue_transparent = parse_hex_rgba(0x0000FF00);
    CHECK(blue_transparent.r == 0);
    CHECK(blue_transparent.g == 0);
    CHECK(blue_transparent.b == 255);
    CHECK(blue_transparent.a == 0);
}

TEST_CASE("color_utils::parse_hex_rgba - White with various opacities") {
    auto const white_full = parse_hex_rgba(0xFFFFFFFF);
    CHECK(white_full.r == 255);
    CHECK(white_full.g == 255);
    CHECK(white_full.b == 255);
    CHECK(white_full.a == 255);

    auto const white_half = parse_hex_rgba(0xFFFFFF80);
    CHECK(white_half.r == 255);
    CHECK(white_half.g == 255);
    CHECK(white_half.b == 255);
    CHECK(white_half.a == 128);

    auto const white_transparent = parse_hex_rgba(0xFFFFFF00);
    CHECK(white_transparent.r == 255);
    CHECK(white_transparent.g == 255);
    CHECK(white_transparent.b == 255);
    CHECK(white_transparent.a == 0);
}

TEST_CASE("color_utils::parse_hex_rgba - Alpha edge cases") {
    // Minimum alpha (0x00)
    auto const min_alpha = parse_hex_rgba(0xFF000000);
    CHECK(min_alpha.r == 255);
    CHECK(min_alpha.g == 0);
    CHECK(min_alpha.b == 0);
    CHECK(min_alpha.a == 0);

    // Maximum alpha (0xFF)
    auto const max_alpha = parse_hex_rgba(0xFF0000FF);
    CHECK(max_alpha.r == 255);
    CHECK(max_alpha.g == 0);
    CHECK(max_alpha.b == 0);
    CHECK(max_alpha.a == 255);

    // Mid alpha (0x7F = 127)
    auto const mid_alpha = parse_hex_rgba(0xFF00007F);
    CHECK(mid_alpha.r == 255);
    CHECK(mid_alpha.g == 0);
    CHECK(mid_alpha.b == 0);
    CHECK(mid_alpha.a == 127);
}

// ===========================================================================
// parse_hex_string() - String to Integer Parsing
// ===========================================================================

TEST_CASE("color_utils::parse_hex_string - 0x prefix") {
    // RGB with 0x prefix
    CHECK(parse_hex_string("0xFF0000") == 0xFF0000);
    CHECK(parse_hex_string("0x00FF00") == 0x00FF00);
    CHECK(parse_hex_string("0x0000FF") == 0x0000FF);

    // RGBA with 0x prefix
    CHECK(parse_hex_string("0xFF0000FF") == 0xFF0000FF);
    CHECK(parse_hex_string("0x00FF0080") == 0x00FF0080);
    CHECK(parse_hex_string("0x0000FF00") == 0x0000FF00);
}

TEST_CASE("color_utils::parse_hex_string - 0X prefix (uppercase)") {
    CHECK(parse_hex_string("0XFF0000") == 0xFF0000);
    CHECK(parse_hex_string("0XFF0000FF") == 0xFF0000FF);
}

TEST_CASE("color_utils::parse_hex_string - No prefix") {
    CHECK(parse_hex_string("FF0000") == 0xFF0000);
    CHECK(parse_hex_string("00FF00") == 0x00FF00);
    CHECK(parse_hex_string("0000FF") == 0x0000FF);
    CHECK(parse_hex_string("FF0000FF") == 0xFF0000FF);
}

TEST_CASE("color_utils::parse_hex_string - Case insensitive") {
    // Lowercase
    CHECK(parse_hex_string("0xffffff") == 0xFFFFFF);
    CHECK(parse_hex_string("0xabcdef") == 0xABCDEF);
    CHECK(parse_hex_string("0xabcdef12") == 0xABCDEF12);

    // Uppercase
    CHECK(parse_hex_string("0xFFFFFF") == 0xFFFFFF);
    CHECK(parse_hex_string("0xABCDEF") == 0xABCDEF);
    CHECK(parse_hex_string("0xABCDEF12") == 0xABCDEF12);

    // Mixed case
    CHECK(parse_hex_string("0xAbCdEf") == 0xABCDEF);
    CHECK(parse_hex_string("0xAbCdEf12") == 0xABCDEF12);
}

TEST_CASE("color_utils::parse_hex_string - Leading zeros") {
    CHECK(parse_hex_string("0x000000") == 0x000000);
    CHECK(parse_hex_string("0x0000AA") == 0x0000AA);
    CHECK(parse_hex_string("0x00000000") == 0x00000000);
    CHECK(parse_hex_string("0x000000FF") == 0x000000FF);
}

TEST_CASE("color_utils::parse_hex_string - Invalid inputs") {
    // Empty string
    CHECK_FALSE(parse_hex_string("").has_value());

    // Wrong length (not 6 or 8 digits)
    CHECK_FALSE(parse_hex_string("0xFF").has_value());
    CHECK_FALSE(parse_hex_string("0xFFF").has_value());
    CHECK_FALSE(parse_hex_string("0xFFFF").has_value());
    CHECK_FALSE(parse_hex_string("0xFFFFF").has_value());
    CHECK_FALSE(parse_hex_string("0xFFFFFFF").has_value());  // 7 digits
    CHECK_FALSE(parse_hex_string("0xFFFFFFFFF").has_value()); // 9 digits

    // Invalid characters
    CHECK_FALSE(parse_hex_string("0xZZZZZZ").has_value());
    CHECK_FALSE(parse_hex_string("0xGGGGGG").has_value());
    CHECK_FALSE(parse_hex_string("0xFF00GG").has_value());
    CHECK_FALSE(parse_hex_string("0xFG0000").has_value());

    // Special characters
    CHECK_FALSE(parse_hex_string("0xFF-000").has_value());
    CHECK_FALSE(parse_hex_string("0xFF 000").has_value());
    CHECK_FALSE(parse_hex_string("0xFF:000").has_value());
}

TEST_CASE("color_utils::parse_hex_string - Just 0x prefix") {
    CHECK_FALSE(parse_hex_string("0x").has_value());
    CHECK_FALSE(parse_hex_string("0X").has_value());
}

// ===========================================================================
// is_hex_color() - Validation Function
// ===========================================================================

TEST_CASE("color_utils::is_hex_color - Valid colors") {
    CHECK(is_hex_color("0xFF0000"));
    CHECK(is_hex_color("0x00FF00"));
    CHECK(is_hex_color("0x0000FF"));
    CHECK(is_hex_color("0xFF0000FF"));
    CHECK(is_hex_color("FF0000"));
    CHECK(is_hex_color("00FF00"));
    CHECK(is_hex_color("FF0000FF"));
    CHECK(is_hex_color("0xffffff"));
    CHECK(is_hex_color("0xFFFFFF"));
}

TEST_CASE("color_utils::is_hex_color - Invalid colors") {
    CHECK_FALSE(is_hex_color(""));
    CHECK_FALSE(is_hex_color("0x"));
    CHECK_FALSE(is_hex_color("0xFF"));
    CHECK_FALSE(is_hex_color("0xFFFFF"));  // 5 digits
    CHECK_FALSE(is_hex_color("0xFFFFFFF")); // 7 digits
    CHECK_FALSE(is_hex_color("0xZZZZZZ"));
    CHECK_FALSE(is_hex_color("0xGGGGGG"));
    CHECK_FALSE(is_hex_color("not a color"));
}

// ===========================================================================
// Round-trip Tests - parse_hex_string + parse_hex_rgb/rgba
// ===========================================================================

TEST_CASE("color_utils - Round-trip RGB") {
    auto const hex_value = parse_hex_string("0xFF8800");
    REQUIRE(hex_value.has_value());

    auto const color = parse_hex_rgb(*hex_value);
    CHECK(color.r == 255);
    CHECK(color.g == 136);
    CHECK(color.b == 0);
    CHECK(color.a == 255);
}

TEST_CASE("color_utils - Round-trip RGBA") {
    auto const hex_value = parse_hex_string("0xFF880080");
    REQUIRE(hex_value.has_value());

    auto const color = parse_hex_rgba(*hex_value);
    CHECK(color.r == 255);
    CHECK(color.g == 136);
    CHECK(color.b == 0);
    CHECK(color.a == 128);
}

TEST_CASE("color_utils - Round-trip without prefix") {
    auto const hex_value = parse_hex_string("ABCDEF");
    REQUIRE(hex_value.has_value());

    auto const color = parse_hex_rgb(*hex_value);
    CHECK(color.r == 0xAB);
    CHECK(color.g == 0xCD);
    CHECK(color.b == 0xEF);
    CHECK(color.a == 255);
}

// ===========================================================================
// Constexpr Tests - Compile-time Evaluation
// ===========================================================================

TEST_CASE("color_utils - Constexpr parse_hex_rgb") {
    constexpr auto red = parse_hex_rgb(0xFF0000);
    static_assert(red.r == 255);
    static_assert(red.g == 0);
    static_assert(red.b == 0);
    static_assert(red.a == 255);

    constexpr auto white = parse_hex_rgb(0xFFFFFF);
    static_assert(white.r == 255);
    static_assert(white.g == 255);
    static_assert(white.b == 255);
}

TEST_CASE("color_utils - Constexpr parse_hex_rgba") {
    constexpr auto red_half = parse_hex_rgba(0xFF000080);
    static_assert(red_half.r == 255);
    static_assert(red_half.g == 0);
    static_assert(red_half.b == 0);
    static_assert(red_half.a == 128);
}

// ===========================================================================
// Equality Tests - rgb_components
// ===========================================================================

TEST_CASE("color_utils::rgb_components - Equality") {
    rgb_components const c1{255, 0, 0, 255};
    rgb_components const c2{255, 0, 0, 255};
    rgb_components const c3{0, 255, 0, 255};

    CHECK(c1 == c2);
    CHECK_FALSE(c1 == c3);
}

// ===========================================================================
// Phase 4: Color Manipulation Functions
// ===========================================================================

// ---------------------------------------------------------------------------
// lighten() Tests
// ---------------------------------------------------------------------------

TEST_CASE("color_utils::lighten - Basic lightening") {
    rgb_components const dark_blue{0, 0, 170, 255};

    // Lighten by 30%
    auto const light = lighten(dark_blue, 0.3f);

    // RGB should move toward 255
    CHECK(light.r > dark_blue.r);
    CHECK(light.g > dark_blue.g);
    CHECK(light.b > dark_blue.b);
    CHECK(light.a == 255);  // Alpha preserved
}

TEST_CASE("color_utils::lighten - Full white") {
    rgb_components const dark{50, 50, 50, 255};

    // Lighten to full white
    auto const white = lighten(dark, 1.0f);

    CHECK(white.r == 255);
    CHECK(white.g == 255);
    CHECK(white.b == 255);
    CHECK(white.a == 255);
}

TEST_CASE("color_utils::lighten - No change") {
    rgb_components const color{100, 100, 100, 255};

    // Factor 0.0 = no change
    auto const same = lighten(color, 0.0f);

    CHECK(same == color);
}

TEST_CASE("color_utils::lighten - Already white") {
    rgb_components const white{255, 255, 255, 255};

    // Lightening white stays white
    auto const still_white = lighten(white, 0.5f);

    CHECK(still_white == white);
}

// ---------------------------------------------------------------------------
// darken() Tests
// ---------------------------------------------------------------------------

TEST_CASE("color_utils::darken - Basic darkening") {
    rgb_components const bright_yellow{255, 255, 0, 255};

    // Darken by 50%
    auto const dark = darken(bright_yellow, 0.5f);

    // RGB should move toward 0
    CHECK(dark.r < bright_yellow.r);
    CHECK(dark.g < bright_yellow.g);
    CHECK(dark.b == 0);  // Already 0
    CHECK(dark.a == 255);  // Alpha preserved
}

TEST_CASE("color_utils::darken - Full black") {
    rgb_components const bright{200, 200, 200, 255};

    // Darken to full black
    auto const black = darken(bright, 1.0f);

    CHECK(black.r == 0);
    CHECK(black.g == 0);
    CHECK(black.b == 0);
    CHECK(black.a == 255);
}

TEST_CASE("color_utils::darken - No change") {
    rgb_components const color{100, 100, 100, 255};

    // Factor 0.0 = no change
    auto const same = darken(color, 0.0f);

    CHECK(same == color);
}

TEST_CASE("color_utils::darken - Already black") {
    rgb_components const black{0, 0, 0, 255};

    // Darkening black stays black
    auto const still_black = darken(black, 0.5f);

    CHECK(still_black == black);
}

// ---------------------------------------------------------------------------
// dim() Tests
// ---------------------------------------------------------------------------

TEST_CASE("color_utils::dim - Basic dimming") {
    rgb_components const solid_white{255, 255, 255, 255};

    // Dim by 50%
    auto const dimmed = dim(solid_white, 0.5f);

    // RGB preserved, alpha reduced
    CHECK(dimmed.r == 255);
    CHECK(dimmed.g == 255);
    CHECK(dimmed.b == 255);
    CHECK(dimmed.a == 127);  // 255 * 0.5 = 127.5 → 127
}

TEST_CASE("color_utils::dim - Full transparency") {
    rgb_components const solid{200, 100, 50, 255};

    // Dim to full transparency
    auto const transparent = dim(solid, 1.0f);

    CHECK(transparent.r == 200);
    CHECK(transparent.g == 100);
    CHECK(transparent.b == 50);
    CHECK(transparent.a == 0);
}

TEST_CASE("color_utils::dim - No change") {
    rgb_components const color{100, 100, 100, 255};

    // Factor 0.0 = no change
    auto const same = dim(color, 0.0f);

    CHECK(same == color);
}

// ---------------------------------------------------------------------------
// invert() Tests
// ---------------------------------------------------------------------------

TEST_CASE("color_utils::invert - Black to white") {
    rgb_components const black{0, 0, 0, 255};

    auto const white = invert(black);

    CHECK(white.r == 255);
    CHECK(white.g == 255);
    CHECK(white.b == 255);
    CHECK(white.a == 255);  // Alpha preserved
}

TEST_CASE("color_utils::invert - Primary colors") {
    rgb_components const red{255, 0, 0, 255};
    auto const cyan = invert(red);
    CHECK(cyan.r == 0);
    CHECK(cyan.g == 255);
    CHECK(cyan.b == 255);

    rgb_components const blue{0, 0, 255, 255};
    auto const yellow = invert(blue);
    CHECK(yellow.r == 255);
    CHECK(yellow.g == 255);
    CHECK(yellow.b == 0);
}

TEST_CASE("color_utils::invert - Double inversion") {
    rgb_components const original{123, 45, 67, 200};

    auto const inverted = invert(original);
    auto const restored = invert(inverted);

    CHECK(restored == original);
}

// ---------------------------------------------------------------------------
// luminance() Tests
// ---------------------------------------------------------------------------

TEST_CASE("color_utils::luminance - Black and white") {
    rgb_components const black{0, 0, 0, 255};
    CHECK(luminance(black) == 0.0f);

    rgb_components const white{255, 255, 255, 255};
    CHECK(luminance(white) == doctest::Approx(1.0f).epsilon(0.01));
}

TEST_CASE("color_utils::luminance - Gray scale") {
    rgb_components const dark_gray{64, 64, 64, 255};
    rgb_components const light_gray{192, 192, 192, 255};

    float const lum_dark = luminance(dark_gray);
    float const lum_light = luminance(light_gray);

    CHECK(lum_dark < lum_light);
    CHECK(lum_dark > 0.0f);
    CHECK(lum_light < 1.0f);
}

TEST_CASE("color_utils::luminance - Weighted channels") {
    // Green contributes most to luminance (0.7152)
    rgb_components const green{0, 255, 0, 255};
    rgb_components const red{255, 0, 0, 255};
    rgb_components const blue{0, 0, 255, 255};

    float const lum_green = luminance(green);
    float const lum_red = luminance(red);
    float const lum_blue = luminance(blue);

    // Green should have highest luminance
    CHECK(lum_green > lum_red);
    CHECK(lum_green > lum_blue);
}

// ---------------------------------------------------------------------------
// contrast() Tests
// ---------------------------------------------------------------------------

TEST_CASE("color_utils::contrast - Dark background") {
    rgb_components const dark_blue{0, 0, 170, 255};

    auto const text_color = contrast(dark_blue);

    // Dark background → white text
    CHECK(text_color.r == 255);
    CHECK(text_color.g == 255);
    CHECK(text_color.b == 255);
}

TEST_CASE("color_utils::contrast - Light background") {
    rgb_components const light_yellow{255, 255, 0, 255};

    auto const text_color = contrast(light_yellow);

    // Light background → black text
    CHECK(text_color.r == 0);
    CHECK(text_color.g == 0);
    CHECK(text_color.b == 0);
}

TEST_CASE("color_utils::contrast - Edge cases") {
    // Pure black → white text
    rgb_components const black{0, 0, 0, 255};
    CHECK(contrast(black) == rgb_components{255, 255, 255, 255});

    // Pure white → black text
    rgb_components const white{255, 255, 255, 255};
    CHECK(contrast(white) == rgb_components{0, 0, 0, 255});
}

// ---------------------------------------------------------------------------
// to_hex_string() Tests
// ---------------------------------------------------------------------------

TEST_CASE("color_utils::to_hex_string - RGB format") {
    rgb_components const red{255, 0, 0, 255};
    CHECK(to_hex_string(red) == "0xFF0000");

    rgb_components const green{0, 255, 0, 255};
    CHECK(to_hex_string(green) == "0x00FF00");

    rgb_components const blue{0, 0, 255, 255};
    CHECK(to_hex_string(blue) == "0x0000FF");
}

TEST_CASE("color_utils::to_hex_string - RGBA format") {
    rgb_components const red_opaque{255, 0, 0, 255};
    CHECK(to_hex_string(red_opaque, true) == "0xFF0000FF");

    rgb_components const red_half{255, 0, 0, 128};
    CHECK(to_hex_string(red_half, true) == "0xFF000080");

    rgb_components const transparent{255, 255, 255, 0};
    CHECK(to_hex_string(transparent, true) == "0xFFFFFF00");
}

TEST_CASE("color_utils::to_hex_string - Lowercase output") {
    // Uppercase hex letters
    rgb_components const color{170, 187, 204, 255};
    std::string const hex = to_hex_string(color);

    // Should contain uppercase hex digits
    CHECK(hex == "0xAABBCC");
}

// ---------------------------------------------------------------------------
// Integration Tests - Combining operations
// ---------------------------------------------------------------------------

TEST_CASE("color_utils::integration - Generate hover state") {
    // Typical use case: generate hover color from normal color
    rgb_components const normal_bg{0, 0, 170, 255};  // Dark blue

    // Hover = lighten background slightly
    auto const hover_bg = lighten(normal_bg, 0.2f);

    CHECK(hover_bg.b > normal_bg.b);
    CHECK(hover_bg.b < 255);
}

TEST_CASE("color_utils::integration - Generate disabled state") {
    // Typical use case: generate disabled color
    rgb_components const normal_fg{255, 255, 255, 255};  // White

    // Disabled = dim foreground
    auto const disabled_fg = dim(normal_fg, 0.6f);  // 40% visible

    CHECK(disabled_fg.r == 255);
    CHECK(disabled_fg.g == 255);
    CHECK(disabled_fg.b == 255);
    CHECK(disabled_fg.a == 101);  // 255 * 0.4 = 102 → 101
}

TEST_CASE("color_utils::integration - Readable text color") {
    // Typical use case: ensure text is readable on background
    rgb_components const bg{0, 0, 170, 255};  // Dark blue background

    auto const text_color = contrast(bg);

    // Should get white text for readability
    CHECK(text_color == rgb_components{255, 255, 255, 255});
}
