//
// Theme Builder Compatibility Tests
// Validates that theme_builder produces identical themes to manual construction
//

#include <doctest/doctest.h>
#include <onyxui/conio/conio_backend.hh>
#include <onyxui/theming/theme.hh>
#include "../src/conio_themes.hh"
#include "conio_themes_original.hh"

using namespace onyxui;
using namespace onyxui::conio;

// Helper macros for detailed comparisons
#define CHECK_COLOR_EQ(field, orig, builder) \
    CHECK_MESSAGE(orig.field.r == builder.field.r, "Field " #field " red mismatch: ", static_cast<int>(orig.field.r), " vs ", static_cast<int>(builder.field.r)); \
    CHECK_MESSAGE(orig.field.g == builder.field.g, "Field " #field " green mismatch: ", static_cast<int>(orig.field.g), " vs ", static_cast<int>(builder.field.g)); \
    CHECK_MESSAGE(orig.field.b == builder.field.b, "Field " #field " blue mismatch: ", static_cast<int>(orig.field.b), " vs ", static_cast<int>(builder.field.b))

#define CHECK_FONT_EQ(field, orig, builder) \
    CHECK_MESSAGE(orig.field.bold == builder.field.bold, "Field " #field " bold mismatch"); \
    CHECK_MESSAGE(orig.field.underline == builder.field.underline, "Field " #field " underline mismatch"); \
    CHECK_MESSAGE(orig.field.reverse == builder.field.reverse, "Field " #field " reverse mismatch")

#define CHECK_VISUAL_STATE_EQ(field, orig, builder) \
    CHECK_FONT_EQ(field.font, orig, builder); \
    CHECK_COLOR_EQ(field.foreground, orig, builder); \
    CHECK_COLOR_EQ(field.background, orig, builder)

#define CHECK_BOX_STYLE_EQ(field, orig, builder) \
    CHECK_MESSAGE(orig.field.style == builder.field.style, "Field " #field " style mismatch"); \
    CHECK_MESSAGE(orig.field.is_solid == builder.field.is_solid, "Field " #field " is_solid mismatch")

// ===========================================================================
// Norton Blue Theme Comparison
// ===========================================================================

TEST_CASE("theme_builder - Norton Blue identical to original") {
    auto original = conio_themes_original::create_norton_blue_original();
    auto builder_made = conio_themes::create_norton_blue();

    SUBCASE("Metadata") {
        CHECK(original.name == builder_made.name);
        CHECK(original.description == builder_made.description);
    }

    SUBCASE("Global colors") {
        CHECK_COLOR_EQ(window_bg, original, builder_made);
        CHECK_COLOR_EQ(text_fg, original, builder_made);
        CHECK_COLOR_EQ(border_color, original, builder_made);
    }

    SUBCASE("Button - visual states") {
        CHECK_VISUAL_STATE_EQ(button.normal, original, builder_made);
        CHECK_VISUAL_STATE_EQ(button.hover, original, builder_made);
        CHECK_VISUAL_STATE_EQ(button.pressed, original, builder_made);
        CHECK_VISUAL_STATE_EQ(button.disabled, original, builder_made);
    }

    SUBCASE("Button - mnemonic font") {
        CHECK_FONT_EQ(button.mnemonic_font, original, builder_made);
    }

    SUBCASE("Button - box style") {
        CHECK_BOX_STYLE_EQ(button.box_style, original, builder_made);
    }

    SUBCASE("Button - padding and alignment") {
        CHECK(original.button.padding_horizontal == builder_made.button.padding_horizontal);
        CHECK(original.button.padding_vertical == builder_made.button.padding_vertical);
        CHECK(original.button.text_align == builder_made.button.text_align);
    }

    SUBCASE("Label") {
        CHECK_COLOR_EQ(label.text, original, builder_made);
        CHECK_COLOR_EQ(label.background, original, builder_made);
    }

    SUBCASE("Panel") {
        CHECK_COLOR_EQ(panel.background, original, builder_made);
        CHECK_COLOR_EQ(panel.border_color, original, builder_made);
        CHECK_BOX_STYLE_EQ(panel.box_style, original, builder_made);
    }

    SUBCASE("Menu") {
        CHECK_COLOR_EQ(menu.background, original, builder_made);
        CHECK_COLOR_EQ(menu.border_color, original, builder_made);
        CHECK_BOX_STYLE_EQ(menu.box_style, original, builder_made);
    }

    SUBCASE("Menu bar") {
        CHECK(original.menu_bar.item_spacing == builder_made.menu_bar.item_spacing);
        CHECK(original.menu_bar.item_padding_horizontal == builder_made.menu_bar.item_padding_horizontal);
        CHECK(original.menu_bar.item_padding_vertical == builder_made.menu_bar.item_padding_vertical);
    }

    SUBCASE("Separator") {
        CHECK(original.separator.line_style.style == builder_made.separator.line_style.style);
    }

    SUBCASE("Menu item - visual states") {
        CHECK_VISUAL_STATE_EQ(menu_item.normal, original, builder_made);
        CHECK_VISUAL_STATE_EQ(menu_item.highlighted, original, builder_made);
        CHECK_VISUAL_STATE_EQ(menu_item.disabled, original, builder_made);
        CHECK_VISUAL_STATE_EQ(menu_item.shortcut, original, builder_made);
    }

    SUBCASE("Menu item - mnemonic font") {
        CHECK_FONT_EQ(menu_item.mnemonic_font, original, builder_made);
    }

    SUBCASE("Menu item - padding") {
        CHECK(original.menu_item.padding_horizontal == builder_made.menu_item.padding_horizontal);
        CHECK(original.menu_item.padding_vertical == builder_made.menu_item.padding_vertical);
    }

    SUBCASE("Menu bar item - visual states") {
        CHECK_VISUAL_STATE_EQ(menu_bar_item.normal, original, builder_made);
        CHECK_VISUAL_STATE_EQ(menu_bar_item.hover, original, builder_made);
        CHECK_VISUAL_STATE_EQ(menu_bar_item.open, original, builder_made);
    }

    SUBCASE("Menu bar item - mnemonic font") {
        CHECK_FONT_EQ(menu_bar_item.mnemonic_font, original, builder_made);
    }

    SUBCASE("Menu bar item - padding") {
        CHECK(original.menu_bar_item.padding_horizontal == builder_made.menu_bar_item.padding_horizontal);
        CHECK(original.menu_bar_item.padding_vertical == builder_made.menu_bar_item.padding_vertical);
    }
}

// ===========================================================================
// Borland Turbo Theme Comparison
// ===========================================================================

TEST_CASE("theme_builder - Borland Turbo identical to original") {
    auto original = conio_themes_original::create_borland_turbo_original();
    auto builder_made = conio_themes::create_borland_turbo();

    SUBCASE("Metadata") {
        CHECK(original.name == builder_made.name);
        CHECK(original.description == builder_made.description);
    }

    SUBCASE("Global colors") {
        CHECK_COLOR_EQ(window_bg, original, builder_made);
        CHECK_COLOR_EQ(text_fg, original, builder_made);
        CHECK_COLOR_EQ(border_color, original, builder_made);
    }

    SUBCASE("Button - box style (double_line)") {
        CHECK_BOX_STYLE_EQ(button.box_style, original, builder_made);
    }

    SUBCASE("Panel - box style (double_line)") {
        CHECK_BOX_STYLE_EQ(panel.box_style, original, builder_made);
    }

    SUBCASE("Menu - box style (double_line)") {
        CHECK_BOX_STYLE_EQ(menu.box_style, original, builder_made);
    }

    SUBCASE("Button - visual states") {
        CHECK_VISUAL_STATE_EQ(button.normal, original, builder_made);
        CHECK_VISUAL_STATE_EQ(button.hover, original, builder_made);
        CHECK_VISUAL_STATE_EQ(button.pressed, original, builder_made);
        CHECK_VISUAL_STATE_EQ(button.disabled, original, builder_made);
    }

    SUBCASE("Menu item - visual states") {
        CHECK_VISUAL_STATE_EQ(menu_item.normal, original, builder_made);
        CHECK_VISUAL_STATE_EQ(menu_item.highlighted, original, builder_made);
        CHECK_VISUAL_STATE_EQ(menu_item.disabled, original, builder_made);
    }

    SUBCASE("Menu bar item - visual states") {
        CHECK_VISUAL_STATE_EQ(menu_bar_item.normal, original, builder_made);
        CHECK_VISUAL_STATE_EQ(menu_bar_item.hover, original, builder_made);
        CHECK_VISUAL_STATE_EQ(menu_bar_item.open, original, builder_made);
    }
}

// ===========================================================================
// Midnight Commander Theme Comparison
// ===========================================================================

TEST_CASE("theme_builder - Midnight Commander identical to original") {
    auto original = conio_themes_original::create_midnight_commander_original();
    auto builder_made = conio_themes::create_midnight_commander();

    SUBCASE("Metadata") {
        CHECK(original.name == builder_made.name);
        CHECK(original.description == builder_made.description);
    }

    SUBCASE("Global colors") {
        CHECK_COLOR_EQ(window_bg, original, builder_made);
        CHECK_COLOR_EQ(text_fg, original, builder_made);
        CHECK_COLOR_EQ(border_color, original, builder_made);
    }

    SUBCASE("Button - visual states") {
        CHECK_VISUAL_STATE_EQ(button.normal, original, builder_made);
        CHECK_VISUAL_STATE_EQ(button.hover, original, builder_made);
        CHECK_VISUAL_STATE_EQ(button.pressed, original, builder_made);
        CHECK_VISUAL_STATE_EQ(button.disabled, original, builder_made);
    }

    SUBCASE("Menu item - visual states") {
        CHECK_VISUAL_STATE_EQ(menu_item.normal, original, builder_made);
        CHECK_VISUAL_STATE_EQ(menu_item.highlighted, original, builder_made);
        CHECK_VISUAL_STATE_EQ(menu_item.disabled, original, builder_made);
    }

    SUBCASE("Menu bar item - visual states") {
        CHECK_VISUAL_STATE_EQ(menu_bar_item.normal, original, builder_made);
        CHECK_VISUAL_STATE_EQ(menu_bar_item.hover, original, builder_made);
        CHECK_VISUAL_STATE_EQ(menu_bar_item.open, original, builder_made);
    }
}

// ===========================================================================
// DOS Edit Theme Comparison
// ===========================================================================

TEST_CASE("theme_builder - DOS Edit identical to original") {
    auto original = conio_themes_original::create_dos_edit_original();
    auto builder_made = conio_themes::create_dos_edit();

    SUBCASE("Metadata") {
        CHECK(original.name == builder_made.name);
        CHECK(original.description == builder_made.description);
    }

    SUBCASE("Global colors") {
        CHECK_COLOR_EQ(window_bg, original, builder_made);
        CHECK_COLOR_EQ(text_fg, original, builder_made);
        CHECK_COLOR_EQ(border_color, original, builder_made);
    }

    SUBCASE("Button - visual states") {
        CHECK_VISUAL_STATE_EQ(button.normal, original, builder_made);
        CHECK_VISUAL_STATE_EQ(button.hover, original, builder_made);
        CHECK_VISUAL_STATE_EQ(button.pressed, original, builder_made);
        CHECK_VISUAL_STATE_EQ(button.disabled, original, builder_made);
    }

    SUBCASE("Menu item - visual states") {
        CHECK_VISUAL_STATE_EQ(menu_item.normal, original, builder_made);
        CHECK_VISUAL_STATE_EQ(menu_item.highlighted, original, builder_made);
        CHECK_VISUAL_STATE_EQ(menu_item.disabled, original, builder_made);
    }

    SUBCASE("Menu bar item - visual states") {
        CHECK_VISUAL_STATE_EQ(menu_bar_item.normal, original, builder_made);
        CHECK_VISUAL_STATE_EQ(menu_bar_item.hover, original, builder_made);
        CHECK_VISUAL_STATE_EQ(menu_bar_item.open, original, builder_made);
    }
}
