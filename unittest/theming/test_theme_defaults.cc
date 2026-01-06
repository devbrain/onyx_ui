//
// Unit tests for theme_defaults - Smart default generation (Phase 4)
//

#include <doctest/doctest.h>

#ifdef ONYXUI_ENABLE_YAML_THEMES

#include <onyxui/theming/theme_defaults.hh>
#include "../utils/test_canvas_backend.hh"

using namespace onyxui;
using backend_type = onyxui::testing::test_canvas_backend;

// ===========================================================================
// Completeness Detection Tests
// ===========================================================================

TEST_CASE("theme_defaults::check_completeness - Empty theme") {
    ui_theme<backend_type> theme{};

    auto completeness = theme_defaults::check_completeness(theme);

    // All fields should be marked as missing
    CHECK_FALSE(completeness.has_window_bg);
    CHECK_FALSE(completeness.has_text_fg);
    CHECK_FALSE(completeness.has_border_color);
    CHECK_FALSE(completeness.has_button_normal);
    CHECK_FALSE(completeness.has_button_hover);
    CHECK_FALSE(completeness.has_button_pressed);
    CHECK_FALSE(completeness.has_button_disabled);
}

TEST_CASE("theme_defaults::check_completeness - Only global colors") {
    ui_theme<backend_type> theme{};
    theme.window_bg = backend_type::color_type{0, 0, 170};
    theme.text_fg = backend_type::color_type{255, 255, 255};
    theme.border_color = backend_type::color_type{255, 255, 0};

    auto completeness = theme_defaults::check_completeness(theme);

    // Global colors should be detected
    CHECK(completeness.has_window_bg);
    CHECK(completeness.has_text_fg);
    CHECK(completeness.has_border_color);

    // Widget styles still missing
    CHECK_FALSE(completeness.has_button_normal);
    CHECK_FALSE(completeness.has_label_text);
}

TEST_CASE("theme_defaults::check_completeness - Button with normal state only") {
    ui_theme<backend_type> theme{};
    theme.button.normal.foreground = backend_type::color_type{255, 255, 255};
    theme.button.normal.background = backend_type::color_type{0, 0, 170};

    auto completeness = theme_defaults::check_completeness(theme);

    // Button normal should be detected
    CHECK(completeness.has_button_normal);

    // Other button states should be missing
    CHECK_FALSE(completeness.has_button_hover);
    CHECK_FALSE(completeness.has_button_pressed);
    CHECK_FALSE(completeness.has_button_disabled);
}

TEST_CASE("theme_defaults::check_completeness - Complete button") {
    ui_theme<backend_type> theme{};

    // Fill in all button states
    theme.button.normal.foreground = backend_type::color_type{255, 255, 255};
    theme.button.normal.background = backend_type::color_type{0, 0, 170};

    theme.button.hover.foreground = backend_type::color_type{255, 255, 0};
    theme.button.hover.background = backend_type::color_type{0, 0, 200};

    theme.button.pressed.foreground = backend_type::color_type{0, 0, 255};
    theme.button.pressed.background = backend_type::color_type{255, 255, 255};

    theme.button.disabled.foreground = backend_type::color_type{128, 128, 128};
    theme.button.disabled.background = backend_type::color_type{0, 0, 170};

    auto completeness = theme_defaults::check_completeness(theme);

    // All button states should be detected
    CHECK(completeness.has_button_normal);
    CHECK(completeness.has_button_hover);
    CHECK(completeness.has_button_pressed);
    CHECK(completeness.has_button_disabled);
}

// ===========================================================================
// Visual State Generation Tests
// ===========================================================================

TEST_CASE("theme_defaults::generate_hover_state - Basic generation") {
    typename ui_theme<backend_type>::visual_state normal;
    normal.foreground = backend_type::color_type{255, 255, 255};  // White
    normal.background = backend_type::color_type{0, 0, 170};      // Dark blue
    normal.font.bold = false;

    backend_type::color_type accent{255, 255, 0};  // Yellow

    auto hover = theme_defaults::generate_hover_state<backend_type>(normal, accent);

    // Foreground should be accent color
    CHECK(hover.foreground.r == 255);
    CHECK(hover.foreground.g == 255);
    CHECK(hover.foreground.b == 0);

    // Background should be lightened (but still blue-ish)
    CHECK(hover.background.b > normal.background.b);
    CHECK(hover.background.b < 255);

    // Font should be bold
    CHECK(hover.font.bold == true);
}

TEST_CASE("theme_defaults::generate_pressed_state - Inverts colors") {
    typename ui_theme<backend_type>::visual_state normal;
    normal.foreground = backend_type::color_type{255, 255, 255};  // White
    normal.background = backend_type::color_type{0, 0, 170};      // Dark blue

    auto pressed = theme_defaults::generate_pressed_state<backend_type>(normal);

    // Foreground should be inverted (white → black)
    CHECK(pressed.foreground.r == 0);
    CHECK(pressed.foreground.g == 0);
    CHECK(pressed.foreground.b == 0);

    // Background should be inverted (dark blue → light yellow-green)
    CHECK(pressed.background.r == 255);
    CHECK(pressed.background.g == 255);
    CHECK(pressed.background.b == 85);  // 255 - 170 = 85
}

TEST_CASE("theme_defaults::generate_disabled_state - Darkens foreground") {
    typename ui_theme<backend_type>::visual_state normal;
    normal.foreground = backend_type::color_type{255, 255, 255};  // White
    normal.background = backend_type::color_type{0, 0, 170};      // Dark blue

    auto disabled = theme_defaults::generate_disabled_state<backend_type>(normal);

    // Foreground should be darkened (for RGB-only backends)
    CHECK(disabled.foreground.r < 255);
    CHECK(disabled.foreground.g < 255);
    CHECK(disabled.foreground.b < 255);

    // Background should stay the same
    CHECK(disabled.background.r == normal.background.r);
    CHECK(disabled.background.g == normal.background.g);
    CHECK(disabled.background.b == normal.background.b);
}

// ===========================================================================
// Default Application Tests
// ===========================================================================

TEST_CASE("theme_defaults::apply_defaults - Minimal theme with global colors") {
    ui_theme<backend_type> theme{};
    theme.name = "Minimal";
    theme.window_bg = backend_type::color_type{0, 0, 170};      // Dark blue
    theme.text_fg = backend_type::color_type{255, 255, 255};    // White
    theme.border_color = backend_type::color_type{255, 255, 0}; // Yellow

    theme_defaults::apply_defaults(theme);

    // Button normal should be generated from global colors
    CHECK(theme.button.normal.background.b == 170);  // Primary (dark blue)
    CHECK(theme.button.normal.foreground.r == 255);  // Foreground (white)
    CHECK(theme.button.normal.foreground.g == 255);
    CHECK(theme.button.normal.foreground.b == 255);

    // Button hover should be generated from normal
    CHECK(theme.button.hover.foreground.r == 255);   // Accent (yellow)
    CHECK(theme.button.hover.foreground.g == 255);
    CHECK(theme.button.hover.foreground.b == 0);
    CHECK(theme.button.hover.background.b > 170);    // Lightened
    CHECK(theme.button.hover.font.bold == true);     // Bold

    // Button pressed should be inverted
    CHECK(theme.button.pressed.foreground.r < 10);   // Inverted from white
    CHECK(theme.button.pressed.background.b < 100);  // Inverted from dark blue

    // Button disabled should be darkened (for RGB-only backends)
    CHECK(theme.button.disabled.foreground.r < 200); // Darkened
}

TEST_CASE("theme_defaults::apply_defaults - Label copies from button") {
    ui_theme<backend_type> theme{};
    theme.button.normal.foreground = backend_type::color_type{255, 255, 255};
    theme.button.normal.background = backend_type::color_type{0, 0, 170};

    theme_defaults::apply_defaults(theme);

    // Label should copy from button
    CHECK(theme.label.text.r == theme.button.normal.foreground.r);
    CHECK(theme.label.text.g == theme.button.normal.foreground.g);
    CHECK(theme.label.text.b == theme.button.normal.foreground.b);

    CHECK(theme.label.background.r == theme.button.normal.background.r);
    CHECK(theme.label.background.g == theme.button.normal.background.g);
    CHECK(theme.label.background.b == theme.button.normal.background.b);
}

TEST_CASE("theme_defaults::apply_defaults - Panel uses global colors") {
    ui_theme<backend_type> theme{};
    theme.window_bg = backend_type::color_type{0, 0, 170};
    theme.border_color = backend_type::color_type{255, 255, 0};

    theme_defaults::apply_defaults(theme);

    // Panel should use global colors
    CHECK(theme.panel.background.r == theme.window_bg.r);
    CHECK(theme.panel.background.g == theme.window_bg.g);
    CHECK(theme.panel.background.b == theme.window_bg.b);

    CHECK(theme.panel.border_color.r == theme.border_color.r);
    CHECK(theme.panel.border_color.g == theme.border_color.g);
    CHECK(theme.panel.border_color.b == theme.border_color.b);
}

TEST_CASE("theme_defaults::apply_defaults - Menu copies from panel") {
    ui_theme<backend_type> theme{};
    theme.panel.background = backend_type::color_type{0, 0, 170};
    theme.panel.border_color = backend_type::color_type{255, 255, 0};

    theme_defaults::apply_defaults(theme);

    // Menu should copy from panel
    CHECK(theme.menu.background.r == theme.panel.background.r);
    CHECK(theme.menu.background.g == theme.panel.background.g);
    CHECK(theme.menu.background.b == theme.panel.background.b);

    CHECK(theme.menu.border_color.r == theme.panel.border_color.r);
    CHECK(theme.menu.border_color.g == theme.panel.border_color.g);
    CHECK(theme.menu.border_color.b == theme.panel.border_color.b);
}

TEST_CASE("theme_defaults::apply_defaults - Menu items like buttons") {
    ui_theme<backend_type> theme{};
    theme.button.normal.foreground = backend_type::color_type{255, 255, 255};
    theme.button.normal.background = backend_type::color_type{0, 0, 170};
    theme.border_color = backend_type::color_type{255, 255, 0};

    theme_defaults::apply_defaults(theme);

    // Menu item normal should match button normal
    CHECK(theme.menu_item.normal.foreground.r == theme.button.normal.foreground.r);
    CHECK(theme.menu_item.normal.background.b == theme.button.normal.background.b);

    // Menu item highlighted should be like button hover
    CHECK(theme.menu_item.highlighted.foreground.r == 255);  // Accent
    CHECK(theme.menu_item.highlighted.foreground.g == 255);
    CHECK(theme.menu_item.highlighted.foreground.b == 0);
    CHECK(theme.menu_item.highlighted.font.bold == true);

    // Menu item disabled should be darkened
    CHECK(theme.menu_item.disabled.foreground.r < 200);
}

TEST_CASE("theme_defaults::apply_defaults - Scrollbar defaults") {
    ui_theme<backend_type> theme{};
    theme.window_bg = backend_type::color_type{0, 0, 170};
    theme.border_color = backend_type::color_type{255, 255, 0};
    theme.text_fg = backend_type::color_type{255, 255, 255};

    theme_defaults::apply_defaults(theme);

    // Track should be darkened background
    CHECK(theme.scrollbar.track_normal.background.b < theme.window_bg.b);

    // Thumb should use border color
    CHECK(theme.scrollbar.thumb_normal.background.r == theme.border_color.r);
    CHECK(theme.scrollbar.thumb_normal.background.g == theme.border_color.g);
    CHECK(theme.scrollbar.thumb_normal.background.b == theme.border_color.b);

    // Thumb hover should be lightened
    CHECK(theme.scrollbar.thumb_hover.background.b > theme.scrollbar.thumb_normal.background.b);
}

TEST_CASE("theme_defaults::apply_defaults - Safe to call multiple times") {
    ui_theme<backend_type> theme{};
    theme.window_bg = backend_type::color_type{0, 0, 170};
    theme.text_fg = backend_type::color_type{255, 255, 255};
    theme.border_color = backend_type::color_type{255, 255, 0};

    // Apply defaults twice
    theme_defaults::apply_defaults(theme);
    auto button_bg_after_first = theme.button.normal.background;

    theme_defaults::apply_defaults(theme);
    auto button_bg_after_second = theme.button.normal.background;

    // Should be idempotent
    CHECK(button_bg_after_first.r == button_bg_after_second.r);
    CHECK(button_bg_after_first.g == button_bg_after_second.g);
    CHECK(button_bg_after_first.b == button_bg_after_second.b);
}

TEST_CASE("theme_defaults::apply_defaults - Respects existing values") {
    ui_theme<backend_type> theme{};
    theme.window_bg = backend_type::color_type{0, 0, 170};
    theme.text_fg = backend_type::color_type{255, 255, 255};

    // Explicitly set button hover to custom value
    theme.button.hover.foreground = backend_type::color_type{0, 255, 0};  // Green
    theme.button.hover.background = backend_type::color_type{255, 0, 0};  // Red

    theme_defaults::apply_defaults(theme);

    // Custom hover values should be preserved
    CHECK(theme.button.hover.foreground.r == 0);
    CHECK(theme.button.hover.foreground.g == 255);
    CHECK(theme.button.hover.foreground.b == 0);

    CHECK(theme.button.hover.background.r == 255);
    CHECK(theme.button.hover.background.g == 0);
    CHECK(theme.button.hover.background.b == 0);
}

// ===========================================================================
// Integration Tests
// ===========================================================================

TEST_CASE("theme_defaults::integration - 3 colors → complete theme") {
    // Simulate a truly minimal theme with just 3 colors
    ui_theme<backend_type> theme{};
    theme.name = "Minimal Blue";
    theme.description = "3-color minimal theme";

    // Only 3 colors defined
    theme.window_bg = backend_type::color_type{0, 0, 170};      // Dark blue
    theme.text_fg = backend_type::color_type{255, 255, 255};    // White
    theme.border_color = backend_type::color_type{255, 255, 0}; // Yellow

    // Apply smart defaults
    theme_defaults::apply_defaults(theme);

    // Verify we now have a complete theme
    // (Check a sampling of important fields)

    // Button states
    CHECK(theme.button.normal.background.b == 170);
    CHECK(theme.button.hover.font.bold == true);
    CHECK(theme.button.pressed.foreground.r < 10);
    CHECK(theme.button.disabled.foreground.r < 200);

    // Label
    CHECK(theme.label.text.r == 255);
    CHECK(theme.label.background.b == 170);

    // Panel
    CHECK(theme.panel.background.b == 170);
    CHECK(theme.panel.border_color.b == 0);

    // Menu
    CHECK(theme.menu.background.b == 170);

    // Menu items
    CHECK(theme.menu_item.normal.foreground.r == 255);
    CHECK(theme.menu_item.highlighted.font.bold == true);
    CHECK(theme.menu_item.disabled.foreground.r < 200);

    // Scrollbar
    CHECK(theme.scrollbar.thumb_normal.background.b == 0);  // Yellow border color
    CHECK(theme.scrollbar.thumb_hover.background.b > 0);    // Lightened
}

#else // !ONYXUI_ENABLE_YAML_THEMES

TEST_CASE("theme_defaults - YAML themes disabled" * doctest::skip(true)) {
    // Skipped: YAML theme support not enabled in this build
}

#endif // ONYXUI_ENABLE_YAML_THEMES
