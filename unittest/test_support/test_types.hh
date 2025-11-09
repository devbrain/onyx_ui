#pragma once

/**
 * @file test_types.hh
 * @brief Backend-agnostic types for testing YAML serialization infrastructure
 *
 * These types are intentionally minimal and independent of any backend.
 * They test the YAML serialization/deserialization mechanism itself,
 * not backend-specific functionality.
 *
 * Purpose:
 * - Decouple YAML infrastructure tests from specific backends (conio, sdl2, etc.)
 * - Test YAML serialization without pulling in backend dependencies
 * - Keep test types simple and focused on infrastructure testing
 *
 * Integration tests that validate real backend types with YAML should use
 * actual backend types (e.g., ui_theme<conio::conio_backend>).
 */

#include <cstdint>
#include <string>
#include <stdexcept>
#include <string_view>

namespace test {

// ============================================================================
// Color Type
// ============================================================================

/**
 * @brief Minimal RGB color for YAML serialization testing
 *
 * Equivalent to backend color types (conio::color, sdl2::color, etc.)
 * but without any backend dependencies.
 */
struct test_color {
    uint8_t r;
    uint8_t g;
    uint8_t b;

    constexpr bool operator==(const test_color&) const = default;
};

// ============================================================================
// Enum Types
// ============================================================================

/**
 * @brief Box/border style for testing enum serialization
 *
 * Mirrors conio_renderer::box_style for testing purposes
 */
enum class test_box_style : uint8_t {
    none,
    single_line,
    double_line,
    rounded,
    heavy
};

/**
 * @brief Text alignment for testing enum serialization
 */
enum class test_alignment : uint8_t {
    left,
    center,
    right,
    stretch
};

/**
 * @brief Icon style for testing enum serialization
 *
 * Mirrors conio_renderer::icon_style for testing purposes
 */
enum class test_icon_style : uint8_t {
    none,

    // General purpose icons
    check,
    cross,
    bullet,
    folder,
    file,

    // Navigation arrows
    arrow_up,
    arrow_down,
    arrow_left,
    arrow_right,

    // Window management icons
    menu,
    minimize,
    maximize,
    restore,
    close_x
};

// ============================================================================
// Struct Types
// ============================================================================

/**
 * @brief Font flags for testing struct serialization
 *
 * Mirrors conio_renderer::font for testing purposes
 */
struct test_font {
    bool bold = false;
    bool underline = false;
    bool reverse = false; // Swap fg/bg colors

    constexpr bool operator==(const test_font&) const = default;
};

/**
 * @brief Simplified button style for testing nested struct serialization
 *
 * Used to test complex nested YAML structures with multiple color states
 */
struct test_button_style {
    test_color fg_normal;
    test_color bg_normal;
    test_color fg_hover;
    test_color bg_hover;
    test_color fg_pressed;
    test_color bg_pressed;
    test_color fg_disabled;
    test_color bg_disabled;

    test_box_style box_style;
    int padding_horizontal;
    int padding_vertical;
    test_alignment text_align;

    test_font font;
    test_font mnemonic_font;
};

/**
 * @brief Simplified label style
 */
struct test_label_style {
    test_color text;
    test_color background;
    test_font font;
    test_font mnemonic_font;
};

/**
 * @brief Simplified panel style
 */
struct test_panel_style {
    test_color background;
    test_color border_color;
    test_box_style box_style;
    bool has_border;
};

/**
 * @brief Complete theme structure for testing
 *
 * Mirrors ui_theme<Backend> structure but with test types.
 * Used for comprehensive YAML serialization tests.
 */
struct test_theme {
    std::string name;
    std::string description;

    // Global colors
    test_color window_bg;
    test_color text_fg;
    test_color border_color;

    // Widget styles
    test_button_style button;
    test_label_style label;
    test_panel_style panel;
};

// ============================================================================
// Enum String Conversion (for YAML serialization)
// ============================================================================

/**
 * @brief Convert test_box_style enum to string
 */
inline const char* to_string(test_box_style style) {
    switch(style) {
        case test_box_style::none: return "none";
        case test_box_style::single_line: return "single_line";
        case test_box_style::double_line: return "double_line";
        case test_box_style::rounded: return "rounded";
        case test_box_style::heavy: return "heavy";
    }
    throw std::runtime_error("Invalid test_box_style");
}

/**
 * @brief Convert string to test_box_style enum
 */
inline test_box_style box_style_from_string(std::string_view str) {
    if (str == "none") return test_box_style::none;
    if (str == "single_line") return test_box_style::single_line;
    if (str == "double_line") return test_box_style::double_line;
    if (str == "rounded") return test_box_style::rounded;
    if (str == "heavy") return test_box_style::heavy;
    throw std::runtime_error("Invalid box_style string: " + std::string(str));
}

/**
 * @brief Convert test_alignment enum to string
 */
inline const char* to_string(test_alignment align) {
    switch(align) {
        case test_alignment::left: return "left";
        case test_alignment::center: return "center";
        case test_alignment::right: return "right";
        case test_alignment::stretch: return "stretch";
    }
    throw std::runtime_error("Invalid test_alignment");
}

/**
 * @brief Convert string to test_alignment enum
 */
inline test_alignment alignment_from_string(std::string_view str) {
    if (str == "left") return test_alignment::left;
    if (str == "center") return test_alignment::center;
    if (str == "right") return test_alignment::right;
    if (str == "stretch") return test_alignment::stretch;
    throw std::runtime_error("Invalid alignment string: " + std::string(str));
}

/**
 * @brief Convert test_icon_style enum to string
 */
inline const char* to_string(test_icon_style style) {
    switch(style) {
        case test_icon_style::none: return "none";

        // General purpose icons
        case test_icon_style::check: return "check";
        case test_icon_style::cross: return "cross";
        case test_icon_style::bullet: return "bullet";
        case test_icon_style::folder: return "folder";
        case test_icon_style::file: return "file";

        // Navigation arrows
        case test_icon_style::arrow_up: return "arrow_up";
        case test_icon_style::arrow_down: return "arrow_down";
        case test_icon_style::arrow_left: return "arrow_left";
        case test_icon_style::arrow_right: return "arrow_right";

        // Window management icons
        case test_icon_style::menu: return "menu";
        case test_icon_style::minimize: return "minimize";
        case test_icon_style::maximize: return "maximize";
        case test_icon_style::restore: return "restore";
        case test_icon_style::close_x: return "close_x";
    }
    throw std::runtime_error("Invalid test_icon_style");
}

/**
 * @brief Convert string to test_icon_style enum
 */
inline test_icon_style icon_style_from_string(std::string_view str) {
    if (str == "none") return test_icon_style::none;

    // General purpose icons
    if (str == "check") return test_icon_style::check;
    if (str == "cross") return test_icon_style::cross;
    if (str == "bullet") return test_icon_style::bullet;
    if (str == "folder") return test_icon_style::folder;
    if (str == "file") return test_icon_style::file;

    // Navigation arrows
    if (str == "arrow_up") return test_icon_style::arrow_up;
    if (str == "arrow_down") return test_icon_style::arrow_down;
    if (str == "arrow_left") return test_icon_style::arrow_left;
    if (str == "arrow_right") return test_icon_style::arrow_right;

    // Window management icons
    if (str == "menu") return test_icon_style::menu;
    if (str == "minimize") return test_icon_style::minimize;
    if (str == "maximize") return test_icon_style::maximize;
    if (str == "restore") return test_icon_style::restore;
    if (str == "close_x") return test_icon_style::close_x;

    throw std::runtime_error("Invalid icon_style string: " + std::string(str));
}

} // namespace test
