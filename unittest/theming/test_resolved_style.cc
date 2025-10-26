//
// test_resolved_style.cc - Unit tests for resolved_style structure
//
// Created: 2025-10-23 (Theme System Refactoring v2.0 - Phase 4)
//

#include <doctest/doctest.h>
#include "../utils/test_backend.hh"
#include <onyxui/resolved_style.hh>

using namespace onyxui;

TEST_CASE("resolved_style - Default construction") {
    SUBCASE("Default-constructed style has zero-initialized values") {
        resolved_style<test_backend> style;

        // Default color values (depends on backend defaults)
        CHECK(style.background_color.r == 0);
        CHECK(style.background_color.g == 0);
        CHECK(style.background_color.b == 0);
        CHECK(style.foreground_color.r == 0);
        CHECK(style.foreground_color.g == 0);
        CHECK(style.foreground_color.b == 0);
        CHECK(style.border_color.r == 0);
        CHECK(style.border_color.g == 0);
        CHECK(style.border_color.b == 0);

        // Default opacity (1.0 = fully opaque)
        CHECK(style.opacity == 1.0F);

        // Default box_style (no border)
        CHECK(style.box_style.draw_border == false);
    }
}

TEST_CASE("resolved_style - Parameterized construction") {
    SUBCASE("Construct with explicit colors") {
        test_backend::color bg{255, 0, 0, 255};    // Red
        test_backend::color fg{0, 255, 0, 255};    // Green

        resolved_style<test_backend> style(bg, fg);

        CHECK(style.background_color.r == 255);
        CHECK(style.background_color.g == 0);
        CHECK(style.background_color.b == 0);

        CHECK(style.foreground_color.r == 0);
        CHECK(style.foreground_color.g == 255);
        CHECK(style.foreground_color.b == 0);

        // Default values for other fields
        CHECK(style.opacity == 1.0F);  // Default opacity
        CHECK(style.box_style.draw_border == false);
    }

    SUBCASE("Construct with all parameters") {
        test_backend::color bg{100, 100, 100, 255};
        test_backend::color fg{200, 200, 200, 255};
        test_backend::color border{50, 50, 50, 255};
        test_backend::renderer::box_style box{true};  // With border
        test_backend::renderer::font font;
        float opacity = 0.75F;
        test_backend::renderer::icon_style icon;

        resolved_style<test_backend> style(bg, fg, border, box, font, opacity, icon);

        CHECK(style.background_color.r == 100);
        CHECK(style.foreground_color.r == 200);
        CHECK(style.border_color.r == 50);
        CHECK(style.box_style.draw_border == true);
        CHECK(style.opacity == 0.75F);
    }
}

TEST_CASE("resolved_style - Copy semantics") {
    SUBCASE("Copy constructor preserves all values") {
        test_backend::color bg{10, 20, 30, 255};
        test_backend::color fg{40, 50, 60, 255};
        resolved_style<test_backend> original(bg, fg);

        resolved_style<test_backend> copy = original;

        CHECK(copy.background_color.r == original.background_color.r);
        CHECK(copy.background_color.g == original.background_color.g);
        CHECK(copy.background_color.b == original.background_color.b);
        CHECK(copy.foreground_color.r == original.foreground_color.r);
        CHECK(copy.foreground_color.g == original.foreground_color.g);
        CHECK(copy.foreground_color.b == original.foreground_color.b);
        CHECK(copy.opacity == original.opacity);
    }

    SUBCASE("Copy assignment works correctly") {
        test_backend::color bg1{100, 100, 100, 255};
        test_backend::color fg1{200, 200, 200, 255};
        test_backend::color bg2{50, 50, 50, 255};
        test_backend::color fg2{150, 150, 150, 255};

        resolved_style<test_backend> style1(bg1, fg1);
        resolved_style<test_backend> style2(bg2, fg2);

        style2 = style1;  // Copy assignment

        CHECK(style2.background_color.r == 100);
        CHECK(style2.foreground_color.r == 200);
    }
}

TEST_CASE("resolved_style - Move semantics") {
    SUBCASE("Move constructor transfers values") {
        test_backend::color bg{111, 222, 123, 255};
        test_backend::color fg{44, 55, 66, 255};
        resolved_style<test_backend> original(bg, fg);

        resolved_style<test_backend> moved = std::move(original);

        CHECK(moved.background_color.r == 111);
        CHECK(moved.background_color.g == 222);
        CHECK(moved.background_color.b == 123);
        CHECK(moved.foreground_color.r == 44);
        CHECK(moved.foreground_color.g == 55);
        CHECK(moved.foreground_color.b == 66);
    }

    SUBCASE("Move assignment works correctly") {
        test_backend::color bg1{10, 20, 30, 255};
        test_backend::color fg1{40, 50, 60, 255};
        test_backend::color bg2{70, 80, 90, 255};
        test_backend::color fg2{100, 110, 120, 255};

        resolved_style<test_backend> style1(bg1, fg1);
        resolved_style<test_backend> style2(bg2, fg2);

        style2 = std::move(style1);  // Move assignment

        CHECK(style2.background_color.r == 10);
        CHECK(style2.background_color.g == 20);
        CHECK(style2.background_color.b == 30);
    }
}

TEST_CASE("resolved_style - Equality operators") {
    SUBCASE("Identical styles are equal") {
        test_backend::color bg{100, 100, 100, 255};
        test_backend::color fg{200, 200, 200, 255};

        resolved_style<test_backend> style1(bg, fg);
        resolved_style<test_backend> style2(bg, fg);

        CHECK(style1 == style2);
    }

    SUBCASE("Different background colors make styles unequal") {
        test_backend::color bg1{100, 100, 100, 255};
        test_backend::color bg2{101, 100, 100, 255};  // Slightly different
        test_backend::color fg{200, 200, 200, 255};

        resolved_style<test_backend> style1(bg1, fg);
        resolved_style<test_backend> style2(bg2, fg);

        CHECK_FALSE(style1 == style2);
        CHECK(style1 != style2);
    }

    SUBCASE("Different foreground colors make styles unequal") {
        test_backend::color bg{100, 100, 100, 255};
        test_backend::color fg1{200, 200, 200, 255};
        test_backend::color fg2{200, 201, 200, 255};  // Slightly different

        resolved_style<test_backend> style1(bg, fg1);
        resolved_style<test_backend> style2(bg, fg2);

        CHECK_FALSE(style1 == style2);
    }

    SUBCASE("Different opacity makes styles unequal") {
        test_backend::color bg{100, 100, 100, 255};
        test_backend::color fg{200, 200, 200, 255};
        test_backend::color border{50, 50, 50, 255};
        test_backend::renderer::box_style box{false};
        test_backend::renderer::font font;
        test_backend::renderer::icon_style icon;

        resolved_style<test_backend> style1(bg, fg, border, box, font, 0.5F, icon);
        resolved_style<test_backend> style2(bg, fg, border, box, font, 0.75F, icon);

        CHECK_FALSE(style1 == style2);
    }

    SUBCASE("Different box_style makes styles unequal") {
        test_backend::color bg{100, 100, 100, 255};
        test_backend::color fg{200, 200, 200, 255};
        test_backend::color border{50, 50, 50, 255};
        test_backend::renderer::box_style box1{false};  // No border
        test_backend::renderer::box_style box2{true};   // With border
        test_backend::renderer::font font;
        test_backend::renderer::icon_style icon;

        resolved_style<test_backend> style1(bg, fg, border, box1, font, 1.0F, icon);
        resolved_style<test_backend> style2(bg, fg, border, box2, font, 1.0F, icon);

        CHECK_FALSE(style1 == style2);
    }
}

TEST_CASE("resolved_style - with_opacity utility method") {
    SUBCASE("Create new style with different opacity") {
        test_backend::color bg{100, 100, 100, 255};
        test_backend::color fg{200, 200, 200, 255};

        resolved_style<test_backend> original(bg, fg);
        resolved_style<test_backend> modified = original.with_opacity(0.5F);

        // Opacity changed
        CHECK(modified.opacity == 0.5F);
        CHECK(original.opacity == 1.0F);  // Original unchanged

        // Other properties unchanged
        CHECK(modified.background_color.r == original.background_color.r);
        CHECK(modified.foreground_color.r == original.foreground_color.r);
    }

    SUBCASE("Chain multiple with_opacity calls") {
        test_backend::color bg{100, 100, 100, 255};
        test_backend::color fg{200, 200, 200, 255};

        resolved_style<test_backend> original(bg, fg);
        auto modified = original.with_opacity(0.5F).with_opacity(0.25F);

        CHECK(modified.opacity == 0.25F);
    }

    SUBCASE("with_opacity handles edge values") {
        test_backend::color bg{100, 100, 100, 255};
        test_backend::color fg{200, 200, 200, 255};
        resolved_style<test_backend> original(bg, fg);

        // Zero opacity
        auto transparent = original.with_opacity(0.0F);
        CHECK(transparent.opacity == 0.0F);

        // Full opacity
        auto opaque = original.with_opacity(1.0F);
        CHECK(opaque.opacity == 1.0F);

        // Over 1.0 (allowed but unusual)
        auto over = original.with_opacity(1.5F);
        CHECK(over.opacity == 1.5F);
    }
}

TEST_CASE("resolved_style - with_colors utility method") {
    SUBCASE("Create new style with different colors") {
        test_backend::color bg1{100, 100, 100, 255};
        test_backend::color fg1{200, 200, 200, 255};
        test_backend::color bg2{50, 50, 50, 255};
        test_backend::color fg2{250, 250, 250, 255};

        resolved_style<test_backend> original(bg1, fg1);
        resolved_style<test_backend> modified = original.with_colors(bg2, fg2);

        // Colors changed
        CHECK(modified.background_color.r == 50);
        CHECK(modified.foreground_color.r == 250);

        // Original unchanged
        CHECK(original.background_color.r == 100);
        CHECK(original.foreground_color.r == 200);

        // Other properties unchanged
        CHECK(modified.opacity == original.opacity);
    }

    SUBCASE("Chain with_colors and with_opacity") {
        test_backend::color bg1{100, 100, 100, 255};
        test_backend::color fg1{200, 200, 200, 255};
        test_backend::color bg2{50, 50, 50, 255};
        test_backend::color fg2{250, 250, 250, 255};

        resolved_style<test_backend> original(bg1, fg1);
        auto modified = original.with_colors(bg2, fg2).with_opacity(0.5F);

        CHECK(modified.background_color.r == 50);
        CHECK(modified.foreground_color.r == 250);
        CHECK(modified.opacity == 0.5F);
    }
}

TEST_CASE("resolved_style - with_font utility method") {
    SUBCASE("Create new style with different font") {
        test_backend::color bg{100, 100, 100, 255};
        test_backend::color fg{200, 200, 200, 255};
        test_backend::renderer::font font2;

        resolved_style<test_backend> original(bg, fg);
        resolved_style<test_backend> modified = original.with_font(font2);

        // Font changed (both are default-constructed, so structurally equal)
        // This tests the mechanism, not the value difference

        // Other properties unchanged
        CHECK(modified.background_color.r == original.background_color.r);
        CHECK(modified.foreground_color.r == original.foreground_color.r);
        CHECK(modified.opacity == original.opacity);
    }

    SUBCASE("Chain all utility methods") {
        test_backend::color bg1{100, 100, 100, 255};
        test_backend::color fg1{200, 200, 200, 255};
        test_backend::color bg2{50, 50, 50, 255};
        test_backend::color fg2{250, 250, 250, 255};
        test_backend::renderer::font font2;

        resolved_style<test_backend> original(bg1, fg1);
        auto modified = original
            .with_colors(bg2, fg2)
            .with_opacity(0.75F)
            .with_font(font2);

        CHECK(modified.background_color.r == 50);
        CHECK(modified.foreground_color.r == 250);
        CHECK(modified.opacity == 0.75F);
    }
}

TEST_CASE("resolved_style - Edge cases") {
    SUBCASE("Extreme color values") {
        test_backend::color bg{255, 255, 255, 255};  // White
        test_backend::color fg{0, 0, 0, 255};        // Black

        resolved_style<test_backend> style(bg, fg);

        CHECK(style.background_color.r == 255);
        CHECK(style.background_color.g == 255);
        CHECK(style.background_color.b == 255);
        CHECK(style.foreground_color.r == 0);
        CHECK(style.foreground_color.g == 0);
        CHECK(style.foreground_color.b == 0);
    }

    SUBCASE("Alpha channel values") {
        test_backend::color bg{100, 100, 100, 0};    // Fully transparent
        test_backend::color fg{200, 200, 200, 128};  // Half transparent

        resolved_style<test_backend> style(bg, fg);

        CHECK(style.background_color.a == 0);
        CHECK(style.foreground_color.a == 128);
    }

    SUBCASE("Opacity bounds") {
        test_backend::color bg{100, 100, 100, 255};
        test_backend::color fg{200, 200, 200, 255};
        test_backend::color border{50, 50, 50, 255};
        test_backend::renderer::box_style box{false};
        test_backend::renderer::font font;
        test_backend::renderer::icon_style icon;

        // Zero opacity
        resolved_style<test_backend> transparent(bg, fg, border, box, font, 0.0F, icon);
        CHECK(transparent.opacity == 0.0F);

        // Full opacity
        resolved_style<test_backend> opaque(bg, fg, border, box, font, 1.0F, icon);
        CHECK(opaque.opacity == 1.0F);

        // Negative opacity (unusual but allowed)
        resolved_style<test_backend> negative(bg, fg, border, box, font, -0.5F, icon);
        CHECK(negative.opacity == -0.5F);

        // Over 1.0 opacity (unusual but allowed)
        resolved_style<test_backend> over(bg, fg, border, box, font, 2.0F, icon);
        CHECK(over.opacity == 2.0F);
    }
}

TEST_CASE("resolved_style - Immutability guarantee") {
    SUBCASE("Utility methods return new instance, don't modify original") {
        test_backend::color bg{100, 100, 100, 255};
        test_backend::color fg{200, 200, 200, 255};

        resolved_style<test_backend> const original(bg, fg);

        // Create modified versions
        auto with_new_opacity = original.with_opacity(0.5F);
        auto with_new_colors = original.with_colors(
            test_backend::color{50, 50, 50, 255},
            test_backend::color{250, 250, 250, 255}
        );

        // Original unchanged
        CHECK(original.opacity == 1.0F);
        CHECK(original.background_color.r == 100);
        CHECK(original.foreground_color.r == 200);

        // Modified versions have changes
        CHECK(with_new_opacity.opacity == 0.5F);
        CHECK(with_new_colors.background_color.r == 50);
    }
}

TEST_CASE("resolved_style - Performance characteristics") {
    SUBCASE("Structure is lightweight (POD-like)") {
        // This test documents that resolved_style is a simple structure
        // with no virtual functions, heap allocations, or complex operations

        test_backend::color bg{100, 100, 100, 255};
        test_backend::color fg{200, 200, 200, 255};

        resolved_style<test_backend> style1(bg, fg);
        resolved_style<test_backend> style2 = style1;  // Fast copy

        CHECK(style1 == style2);

        // Size check - should be small (exact size depends on backend types)
        // This is a compile-time size assertion - if it grows unexpectedly, we know
        static_assert(sizeof(resolved_style<test_backend>) < 200,
                     "resolved_style should be lightweight");
    }
}

// ============================================================================
// Theme Extraction Tests
// ============================================================================

#include <onyxui/theme.hh>

namespace {
    // Helper to create a test theme
    ui_theme<test_backend> create_extraction_test_theme() {
        ui_theme<test_backend> theme;
        theme.name = "Extraction Test";

        // Button states
        theme.button.normal.foreground = {255, 255, 255, 255};
        theme.button.normal.background = {0, 0, 170, 255};
        theme.button.hover.foreground = {255, 255, 0, 255};
        theme.button.hover.background = {0, 170, 170, 255};
        theme.button.pressed.foreground = {0, 0, 0, 255};
        theme.button.pressed.background = {170, 170, 170, 255};
        theme.button.disabled.foreground = {128, 128, 128, 255};
        theme.button.disabled.background = {64, 64, 64, 255};

        // Label styling
        theme.label.text = {255, 255, 0, 255};
        theme.label.background = {0, 0, 0, 255};

        // Panel styling
        theme.panel.background = {50, 50, 50, 255};
        theme.panel.border_color = {200, 200, 200, 255};

        return theme;
    }
}

TEST_CASE("resolved_style - From theme (button normal state)") {
    auto theme = create_extraction_test_theme();

    // Extract style for button normal state
    resolved_style<test_backend> style;
    style.foreground_color = theme.button.normal.foreground;
    style.background_color = theme.button.normal.background;

    CHECK(style.foreground_color.r == 255);
    CHECK(style.foreground_color.g == 255);
    CHECK(style.foreground_color.b == 255);
    CHECK(style.background_color.r == 0);
    CHECK(style.background_color.g == 0);
    CHECK(style.background_color.b == 170);
}

TEST_CASE("resolved_style - From theme (button hover state)") {
    auto theme = create_extraction_test_theme();

    resolved_style<test_backend> style;
    style.foreground_color = theme.button.hover.foreground;
    style.background_color = theme.button.hover.background;

    CHECK(style.foreground_color.r == 255);
    CHECK(style.foreground_color.g == 255);
    CHECK(style.foreground_color.b == 0);
    CHECK(style.background_color.g == 170);
}

TEST_CASE("resolved_style - From theme (button pressed state)") {
    auto theme = create_extraction_test_theme();

    resolved_style<test_backend> style;
    style.foreground_color = theme.button.pressed.foreground;
    style.background_color = theme.button.pressed.background;

    CHECK(style.foreground_color.r == 0);
    CHECK(style.foreground_color.g == 0);
    CHECK(style.foreground_color.b == 0);
    CHECK(style.background_color.r == 170);
}

TEST_CASE("resolved_style - From theme (button disabled state)") {
    auto theme = create_extraction_test_theme();

    resolved_style<test_backend> style;
    style.foreground_color = theme.button.disabled.foreground;
    style.background_color = theme.button.disabled.background;

    CHECK(style.foreground_color.r == 128);
    CHECK(style.background_color.r == 64);
}

TEST_CASE("resolved_style - From theme (label)") {
    auto theme = create_extraction_test_theme();

    resolved_style<test_backend> style;
    style.foreground_color = theme.label.text;
    style.background_color = theme.label.background;

    CHECK(style.foreground_color.r == 255);
    CHECK(style.foreground_color.g == 255);
    CHECK(style.foreground_color.b == 0);
    CHECK(style.background_color.r == 0);
    CHECK(style.background_color.g == 0);
    CHECK(style.background_color.b == 0);
}

TEST_CASE("resolved_style - From theme (panel)") {
    auto theme = create_extraction_test_theme();

    resolved_style<test_backend> style;
    style.background_color = theme.panel.background;
    style.border_color = theme.panel.border_color;

    CHECK(style.background_color.r == 50);
    CHECK(style.border_color.r == 200);
}

TEST_CASE("resolved_style - State-based extraction preserves other properties") {
    auto theme = create_extraction_test_theme();

    // Extract normal state
    resolved_style<test_backend> normal_style;
    normal_style.foreground_color = theme.button.normal.foreground;
    normal_style.background_color = theme.button.normal.background;
    normal_style.opacity = 1.0F;

    // Extract hover state
    resolved_style<test_backend> hover_style;
    hover_style.foreground_color = theme.button.hover.foreground;
    hover_style.background_color = theme.button.hover.background;
    hover_style.opacity = 1.0F;

    // Both should have same opacity (not affected by state)
    CHECK(normal_style.opacity == hover_style.opacity);

    // But different colors
    CHECK(normal_style.foreground_color.b != hover_style.foreground_color.b);
}

TEST_CASE("resolved_style - Widget type affects extraction") {
    auto theme = create_extraction_test_theme();

    // Button gets button colors
    resolved_style<test_backend> button_style;
    button_style.foreground_color = theme.button.normal.foreground;
    button_style.background_color = theme.button.normal.background;

    // Label gets label colors
    resolved_style<test_backend> label_style;
    label_style.foreground_color = theme.label.text;
    label_style.background_color = theme.label.background;

    // Different widget types get different colors from same theme
    CHECK(button_style.background_color.b != label_style.background_color.b);
}
