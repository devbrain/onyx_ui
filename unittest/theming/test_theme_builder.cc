//
// Theme Builder Tests
// Validates the fluent builder API for creating themes
//

#include <doctest/doctest.h>
#include <onyxui/theming/theme_builder.hh>
#include <onyxui/theming/theme_registry.hh>
#include <onyxui/services/ui_context.hh>
#include "../utils/test_canvas_backend.hh"

using namespace onyxui;
using test_backend = onyxui::testing::test_canvas_backend;

// ===========================================================================
// Creation Methods Tests
// ===========================================================================

TEST_CASE("theme_builder - create with name and description") {
    auto theme = theme_builder<test_backend>::create("Test Theme", "Test description")
        .with_palette(0x0000AA, 0xFFFFFF, 0xFFFF00)
        .build();

    CHECK(theme.name == "Test Theme");
    CHECK(theme.description == "Test description");
}

TEST_CASE("theme_builder - from existing theme") {
    auto base = theme_builder<test_backend>::create("Base", "Base theme")
        .with_palette(0x0000AA, 0xFFFFFF, 0xFFFF00)
        .build();

    auto derived = theme_builder<test_backend>::from(base)
        .build();

    CHECK(derived.name == "Base");
    CHECK(derived.window_bg == base.window_bg);
}

TEST_CASE("theme_builder - extend from registry") {
    scoped_ui_context<test_backend> ctx;

    auto base = theme_builder<test_backend>::create("Base", "Base theme")
        .with_palette(0x0000AA, 0xFFFFFF, 0xFFFF00)
        .build();
    ctx.themes().register_theme(base);

    auto derived = theme_builder<test_backend>::extend("Base", ctx.themes())
        .rename("Derived", "Derived theme")
        .build();

    CHECK(derived.name == "Derived");
    CHECK(derived.window_bg == base.window_bg);
}

// ===========================================================================
// Palette Management Tests
// ===========================================================================

TEST_CASE("theme_builder - with_palette reserved names") {
    auto theme = theme_builder<test_backend>::create("Test", "Test")
        .with_palette({
            {"window_bg", 0xFF0000},
            {"text_fg", 0x00FF00},
            {"border_color", 0x0000FF}
        })
        .build();

    CHECK(theme.window_bg.r == 255);
    CHECK(theme.text_fg.g == 255);
    CHECK(theme.border_color.b == 255);
}

TEST_CASE("theme_builder - override_color") {
    auto theme = theme_builder<test_backend>::create("Test", "Test")
        .with_palette({{"window_bg", 0xFF0000}})
        .override_color("window_bg", 0x00FF00)
        .build();

    CHECK(theme.window_bg.g == 255);
}

TEST_CASE("theme_builder - resolve_color by name") {
    auto builder = theme_builder<test_backend>::create("Test", "Test")
        .with_palette({{"red", 0xFF0000}});

    auto red = builder.resolve_color("red");
    CHECK(red.r == 255);
}

// ===========================================================================
// Widget Builder Tests
// ===========================================================================

TEST_CASE("theme_builder - with_button padding") {
    auto builder = theme_builder<test_backend>::create("Test", "Test")
        .with_palette(0x0000AA, 0xFFFFFF, 0xFFFF00);

    builder.with_button().padding(5, 2);
    auto theme = builder.build();

    CHECK(theme.button.padding_horizontal == 5);
    CHECK(theme.button.padding_vertical == 2);
}

TEST_CASE("theme_builder - with_button colors") {
    auto builder = theme_builder<test_backend>::create("Test", "Test")
        .with_palette({{"fg", 0xFFFFFF}, {"bg", 0x0000AA}});

    builder.with_button().colors("fg", "bg");
    auto theme = builder.build();

    CHECK(theme.button.normal.foreground.r == 255);
    CHECK(theme.button.normal.background.b == 170);
}

TEST_CASE("theme_builder - with_label") {
    auto builder = theme_builder<test_backend>::create("Test", "Test")
        .with_palette({{"text", 0xFFFFFF}, {"bg", 0x0000AA}});

    builder.with_label()
        .text_color("text")
        .background_color("bg");
    auto theme = builder.build();

    CHECK(theme.label.text.r == 255);
    CHECK(theme.label.background.b == 170);
}

TEST_CASE("theme_builder - with_line_edit") {
    auto builder = theme_builder<test_backend>::create("Test", "Test")
        .with_palette({
            {"text", 0xFFFFFF},
            {"bg", 0x0000AA},
            {"border", 0xFFFF00},
            {"placeholder", 0xAAAAAA},
            {"cursor", 0xFF0000}
        });

    builder.with_line_edit()
        .text_color("text")
        .background_color("bg")
        .border_color("border")
        .placeholder_color("placeholder")
        .cursor_color("cursor")
        .padding(1, 0)
        .cursor_insert_icon(test_backend::renderer_type::icon_style::check)
        .cursor_overwrite_icon(test_backend::renderer_type::icon_style::cross);
    auto theme = builder.build();

    CHECK(theme.line_edit.text.r == 255);           // White text
    CHECK(theme.line_edit.background.b == 170);     // Blue background
    CHECK(theme.line_edit.border_color.r == 255);   // Yellow border
    CHECK(theme.line_edit.border_color.g == 255);
    CHECK(theme.line_edit.placeholder_text.r == 170); // Gray placeholder
    CHECK(theme.line_edit.cursor.r == 255);         // Red cursor
    CHECK(theme.line_edit.padding_horizontal == 1);
    CHECK(theme.line_edit.padding_vertical == 0);
    CHECK(theme.line_edit.cursor_insert_icon == test_backend::renderer_type::icon_style::check);
    CHECK(theme.line_edit.cursor_overwrite_icon == test_backend::renderer_type::icon_style::cross);
}

TEST_CASE("theme_builder - with_panel") {
    auto builder = theme_builder<test_backend>::create("Test", "Test")
        .with_palette({{"bg", 0x0000AA}, {"border", 0xFFFF00}});

    builder.with_panel()
        .background_color("bg")
        .border_color("border");
    auto theme = builder.build();

    CHECK(theme.panel.background.b == 170);
    CHECK(theme.panel.border_color.r == 255);
}

TEST_CASE("theme_builder - with_menu_bar item_spacing") {
    auto builder = theme_builder<test_backend>::create("Test", "Test")
        .with_palette(0x0000AA, 0xFFFFFF, 0xFFFF00);

    builder.with_menu_bar()
        .item_spacing(0);  // Continuous menu bar (no gaps)
    auto theme = builder.build();

    CHECK(theme.menu_bar.item_spacing == 0);
}

TEST_CASE("theme_builder - with_menu_bar item_padding") {
    auto builder = theme_builder<test_backend>::create("Test", "Test")
        .with_palette(0x0000AA, 0xFFFFFF, 0xFFFF00);

    builder.with_menu_bar()
        .item_padding(4, 1);
    auto theme = builder.build();

    CHECK(theme.menu_bar.item_padding_horizontal == 4);
    CHECK(theme.menu_bar.item_padding_vertical == 1);
}

TEST_CASE("theme_builder - with_separator foreground") {
    auto builder = theme_builder<test_backend>::create("Test", "Test")
        .with_palette({{"black", 0x000000}, {"white", 0xFFFFFF}});

    builder.with_separator()
        .foreground("black");

    auto theme = builder.build();

    CHECK(theme.separator.foreground.r == 0);
    CHECK(theme.separator.foreground.g == 0);
    CHECK(theme.separator.foreground.b == 0);
}

TEST_CASE("theme_builder - with_separator foreground hex") {
    auto builder = theme_builder<test_backend>::create("Test", "Test")
        .with_palette(0x0000AA, 0xFFFFFF, 0xFFFF00);

    builder.with_separator()
        .foreground(0xFF0000);  // Red

    auto theme = builder.build();

    CHECK(theme.separator.foreground.r == 255);
    CHECK(theme.separator.foreground.g == 0);
    CHECK(theme.separator.foreground.b == 0);
}

TEST_CASE("theme_builder - menu shadow configuration") {
    auto builder = theme_builder<test_backend>::create("Test", "Test")
        .with_palette({{"white", 0xFFFFFF}, {"black", 0x000000}});

    builder.with_menu()
        .background_color("white")
        .border_color("black")
        .shadow(true, 2, 1);  // Enable shadow with custom offsets

    auto theme = builder.build();

    CHECK(theme.menu.shadow.enabled == true);
    CHECK(theme.menu.shadow.offset_x == 2);
    CHECK(theme.menu.shadow.offset_y == 1);
}

TEST_CASE("theme_builder - menu shadow disabled by default") {
    auto builder = theme_builder<test_backend>::create("Test", "Test")
        .with_palette({{"white", 0xFFFFFF}, {"black", 0x000000}});

    builder.with_menu()
        .background_color("white")
        .border_color("black");

    auto theme = builder.build();

    // Shadow should be disabled by default
    CHECK(theme.menu.shadow.enabled == false);
}

//  ===========================================================================
// State Builder Tests
// ===========================================================================

TEST_CASE("theme_builder - button state builder") {
    auto builder = theme_builder<test_backend>::create("Test", "Test")
        .with_palette({{"accent", 0xFFFF00}, {"bg", 0x0000AA}});

    builder.with_button()
        .normal()
            .foreground("accent")
            .background("bg")
            .font(true, false)  // bold, no underline
            .end<theme_builder<test_backend>::button_builder>();

    auto theme = builder.build();

    CHECK(theme.button.normal.foreground.r == 255);
    CHECK(theme.button.normal.foreground.g == 255);
    CHECK(theme.button.normal.background.b == 170);
    CHECK(theme.button.normal.font.bold == true);
}

TEST_CASE("theme_builder - button state builder hex colors") {
    auto builder = theme_builder<test_backend>::create("Test", "Test")
        .with_palette(0x0000AA, 0xFFFFFF, 0xFFFF00);

    builder.with_button()
        .hover()
            .foreground(0xFF8000)  // Direct hex
            .background(0x0000FF)
            .end<theme_builder<test_backend>::button_builder>();

    auto theme = builder.build();

    CHECK(theme.button.hover.foreground.r == 255);
    CHECK(theme.button.hover.foreground.g == 128);
    CHECK(theme.button.hover.background.b == 255);
}

// ===========================================================================
// Chaining Tests
// ===========================================================================

TEST_CASE("theme_builder - implicit conversion chaining") {
    auto builder = theme_builder<test_backend>::create("Test", "Test")
        .with_palette({{"bg", 0x0000AA}, {"fg", 0xFFFFFF}});

    builder.with_button().padding(2, 0);
    builder.with_label().text_color("fg");
    builder.with_panel().background_color("bg");

    auto theme = builder.build();

    CHECK(theme.button.padding_horizontal == 2);
    CHECK(theme.label.text.r == 255);
    CHECK(theme.panel.background.b == 170);
}

// ===========================================================================
// Smart Defaults Integration Tests
// ===========================================================================

TEST_CASE("theme_builder - smart defaults applied") {
    auto theme = theme_builder<test_backend>::create("Test", "Test")
        .with_palette({
            {"window_bg", 0x0000AA},
            {"text_fg", 0xFFFFFF},
            {"border_color", 0xFFFF00}
        })
        .build();

    // Smart defaults should auto-generate button states
    CHECK(theme.button.normal.background == theme.window_bg);
    CHECK(theme.button.normal.foreground == theme.text_fg);

    // Hover should have bold font (auto-generated)
    CHECK(theme.button.hover.font.bold == true);

    // Label should copy from global
    CHECK(theme.label.text == theme.text_fg);
    CHECK(theme.label.background == theme.window_bg);

    // Separator should default to border_color
    CHECK(theme.separator.foreground == theme.border_color);
}

TEST_CASE("theme_builder - smart defaults don't override explicit settings") {
    auto builder = theme_builder<test_backend>::create("Test", "Test")
        .with_palette({{"window_bg", 0x0000AA}, {"text_fg", 0xFFFFFF}, {"border_color", 0xFFFF00}});

    builder.with_button()
        .hover()
            .foreground(0xFF0000)  // Explicit red
            .font(false, false)     // Explicit NOT bold
            .end<theme_builder<test_backend>::button_builder>();

    auto theme = builder.build();

    // Should keep explicit settings
    CHECK(theme.button.hover.foreground.r == 255);
    CHECK(theme.button.hover.font.bold == false);  // NOT overridden
}

// ===========================================================================
// Inheritance Tests
// ===========================================================================

TEST_CASE("theme_builder - palette inheritance from base") {
    auto base = theme_builder<test_backend>::create("Base", "Base")
        .with_palette({
            {"window_bg", 0x0000AA},
            {"text_fg", 0xFFFFFF},
            {"border_color", 0xFFFF00}
        })
        .build();

    auto derived = theme_builder<test_backend>::from(base)
        .override_color("window_bg", 0x000055)  // Darker blue
        .build();

    // Should have darker blue
    CHECK(derived.window_bg.b == 85);
    // Other colors inherited
    CHECK(derived.text_fg.r == 255);
}

TEST_CASE("theme_builder - build sets defaults") {
    auto theme = theme_builder<test_backend>::create("Test", "Test")
        .with_palette(0x0000AA, 0xFFFFFF, 0xFFFF00)
        .build();

    // Non-color defaults should be set
    CHECK(theme.button.text_align == horizontal_alignment::center);
    CHECK(theme.menu_bar.item_spacing == 2);
}

TEST_CASE("theme_builder - build sets mnemonic fonts") {
    auto theme = theme_builder<test_backend>::create("Test", "Test")
        .with_palette(0x0000AA, 0xFFFFFF, 0xFFFF00)
        .build();

    // Mnemonic fonts should be underlined
    CHECK(theme.button.mnemonic_font.underline == true);
    CHECK(theme.label.mnemonic_font.underline == true);
}
