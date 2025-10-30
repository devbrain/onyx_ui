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
