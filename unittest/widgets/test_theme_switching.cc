//
// Test theme switching behavior
//

#include <doctest/doctest.h>
#include "utils/test_backend.hh"
#include <memory>
#include <onyxui/widgets/button.hh>
#include <onyxui/widgets/label.hh>
#include <onyxui/widgets/panel.hh>
#include <onyxui/theme.hh>
#include <utility>

using namespace onyxui;
using Backend = test_backend;

namespace {
    auto create_blue_theme() {
        ui_theme<Backend> theme;
        theme.window_bg = {0, 0, 100};      // Dark blue
        theme.text_fg = {100, 100, 255};    // Light blue

        theme.button.normal.background = {0, 0, 255};     // Bright blue
        theme.button.normal.foreground = {255, 255, 255}; // White
        theme.button.hover.background = {0, 0, 200};
        theme.button.hover.foreground = {255, 255, 255};
        theme.button.pressed.background = {0, 0, 150};
        theme.button.pressed.foreground = {255, 255, 255};
        theme.button.disabled.background = {50, 50, 100};
        theme.button.disabled.foreground = {150, 150, 200};

        theme.label.text = {100, 100, 255};
        theme.label.background = {0, 0, 100};

        theme.panel.background = {0, 0, 100};
        theme.panel.border_color = {100, 100, 255};

        return theme;
    }

    auto create_red_theme() {
        ui_theme<Backend> theme;
        theme.window_bg = {100, 0, 0};      // Dark red
        theme.text_fg = {255, 100, 100};    // Light red

        theme.button.normal.background = {255, 0, 0};     // Bright red
        theme.button.normal.foreground = {255, 255, 0};   // Yellow (different from blue theme's white)
        theme.button.hover.background = {200, 0, 0};
        theme.button.hover.foreground = {255, 255, 0};
        theme.button.pressed.background = {150, 0, 0};
        theme.button.pressed.foreground = {255, 255, 0};
        theme.button.disabled.background = {100, 50, 50};
        theme.button.disabled.foreground = {200, 150, 150};

        theme.label.text = {255, 100, 100};
        theme.label.background = {100, 0, 0};

        theme.panel.background = {100, 0, 0};
        theme.panel.border_color = {255, 100, 100};

        return theme;
    }
}

TEST_SUITE("Theme Switching") {

    using color_type = Backend::color_type;

    // Helper to create two VERY distinct themes


    TEST_CASE("Theme Switching - Single button gets correct colors from theme") {
        auto blue_theme = create_blue_theme();
        auto red_theme = create_red_theme();

        auto btn = std::make_unique<button<Backend>>("Test");

        SUBCASE("Button with blue theme gets blue background") {
            btn->apply_theme(std::move(blue_theme));
            auto bg = btn->resolve_style().background_color;

            // Button should get background from theme (window_bg or panel bg, depending on inheritance)
            // The important thing is it should be from the blue theme
            CHECK((bg.r == 0 || bg.r < 10));  // Low red component
            CHECK((bg.b >= 100));              // High blue component
        }

        SUBCASE("Button with red theme gets red background") {
            btn->apply_theme(std::move(red_theme));
            auto bg = btn->resolve_style().background_color;

            // Should get colors from red theme
            CHECK((bg.r >= 100));  // High red component
            CHECK((bg.b == 0 || bg.b < 10));   // Low blue component
        }

        SUBCASE("Theme switch changes button colors") {
            // Start with blue theme
            btn->apply_theme(std::move(blue_theme));
            auto bg_blue = btn->resolve_style().background_color;

            // Switch to red theme
            btn->apply_theme(std::move(red_theme));
            auto bg_red = btn->resolve_style().background_color;

            // Colors MUST be different
            bool const colors_changed = (bg_blue.r != bg_red.r) ||
                                 (bg_blue.g != bg_red.g) ||
                                 (bg_blue.b != bg_red.b);

            CHECK(colors_changed);

            // Verify direction of change: blue should have low red, high blue
            CHECK(bg_blue.b > bg_blue.r);
            // Red should have high red, low blue
            CHECK(bg_red.r > bg_red.b);
        }
    }

    TEST_CASE("Theme Switching - Child widgets get parent theme colors") {
        auto blue_theme = create_blue_theme();
        auto red_theme = create_red_theme();

        auto root = std::make_unique<panel<Backend>>();
        auto btn_ptr = std::make_unique<button<Backend>>("Button");
        auto* btn = btn_ptr.get();
        root->add_child(std::move(btn_ptr));

        SUBCASE("Child inherits blue theme from parent") {
            root->apply_theme(std::move(blue_theme));

            auto root_bg = root->resolve_style().background_color;
            auto btn_bg = btn->resolve_style().background_color;

            // Both should have blue theme colors
            CHECK(root_bg.b >= 100);  // High blue
            CHECK(btn_bg.b >= 100);   // High blue
        }

        SUBCASE("Theme switch on parent propagates to child") {
            // Apply blue theme
            root->apply_theme(std::move(blue_theme));
            auto btn_bg_blue = btn->resolve_style().background_color;

            // Switch to red theme
            root->apply_theme(std::move(red_theme));
            auto btn_bg_red = btn->resolve_style().background_color;

            // Child's colors MUST change
            bool const child_colors_changed = (btn_bg_blue.r != btn_bg_red.r) ||
                                       (btn_bg_blue.g != btn_bg_red.g) ||
                                       (btn_bg_blue.b != btn_bg_red.b);

            CHECK(child_colors_changed);

            // Verify the child got the right theme
            CHECK(btn_bg_blue.b > 50);  // Blue theme = high blue
            CHECK(btn_bg_red.r > 50);   // Red theme = high red
        }
    }

    TEST_CASE("Theme Switching - Multiple children all get updated") {
        auto blue_theme = create_blue_theme();
        auto red_theme = create_red_theme();

        auto root = std::make_unique<panel<Backend>>();

        auto btn1_ptr = std::make_unique<button<Backend>>("Button1");
        auto* btn1 = btn1_ptr.get();
        root->add_child(std::move(btn1_ptr));

        auto btn2_ptr = std::make_unique<button<Backend>>("Button2");
        auto* btn2 = btn2_ptr.get();
        root->add_child(std::move(btn2_ptr));

        auto lbl_ptr = std::make_unique<label<Backend>>("Label");
        auto* lbl = lbl_ptr.get();
        root->add_child(std::move(lbl_ptr));

        SUBCASE("All children get blue theme") {
            root->apply_theme(std::move(blue_theme));

            auto btn1_bg = btn1->resolve_style().background_color;
            auto btn2_bg = btn2->resolve_style().background_color;
            auto lbl_bg = lbl->resolve_style().background_color;

            // All should have high blue component
            CHECK(btn1_bg.b >= 50);
            CHECK(btn2_bg.b >= 50);
            CHECK(lbl_bg.b >= 50);
        }

        SUBCASE("Theme switch updates ALL children") {
            root->apply_theme(std::move(blue_theme));
            auto btn1_blue = btn1->resolve_style().background_color;
            auto btn2_blue = btn2->resolve_style().background_color;
            auto lbl_blue = lbl->resolve_style().background_color;

            root->apply_theme(std::move(red_theme));
            auto btn1_red = btn1->resolve_style().background_color;
            auto btn2_red = btn2->resolve_style().background_color;
            auto lbl_red = lbl->resolve_style().background_color;

            // Every child must have different colors
            CHECK((btn1_blue.r != btn1_red.r || btn1_blue.b != btn1_red.b));
            CHECK((btn2_blue.r != btn2_red.r || btn2_blue.b != btn2_red.b));
            CHECK((lbl_blue.r != lbl_red.r || lbl_blue.b != lbl_red.b));

            // All should now have red theme colors (high red, low blue)
            CHECK(btn1_red.r >= 50);
            CHECK(btn2_red.r >= 50);
            CHECK(lbl_red.r >= 50);
        }
    }

    TEST_CASE("Theme Switching - Nested containers propagate correctly") {
        auto blue_theme = create_blue_theme();
        auto red_theme = create_red_theme();

        auto root = std::make_unique<panel<Backend>>();

        auto container_ptr = std::make_unique<panel<Backend>>();
        auto* container = container_ptr.get();
        root->add_child(std::move(container_ptr));

        auto btn_ptr = std::make_unique<button<Backend>>("DeepButton");
        auto* btn = btn_ptr.get();
        container->add_child(std::move(btn_ptr));

        SUBCASE("Theme propagates through nesting") {
            root->apply_theme(std::move(blue_theme));

            auto root_bg = root->resolve_style().background_color;
            auto container_bg = container->resolve_style().background_color;
            auto btn_bg = btn->resolve_style().background_color;

            // All levels should have blue theme
            CHECK(root_bg.b >= 50);
            CHECK(container_bg.b >= 50);
            CHECK(btn_bg.b >= 50);
        }

        SUBCASE("Theme switch propagates through all nesting levels") {
            root->apply_theme(std::move(blue_theme));
            auto btn_blue = btn->resolve_style().background_color;

            root->apply_theme(std::move(red_theme));
            auto btn_red = btn->resolve_style().background_color;

            // Deep child must get updated
            CHECK((btn_blue.r != btn_red.r || btn_blue.b != btn_red.b));

            // Should now have red theme
            CHECK(btn_red.r >= 50);  // High red
        }
    }

    TEST_CASE("Theme Switching - Rapid theme changes work correctly") {
        auto blue_theme = create_blue_theme();
        auto red_theme = create_red_theme();

        auto btn = std::make_unique<button<Backend>>("Test");

        SUBCASE("Multiple rapid switches maintain consistency") {
            // Switch multiple times - create fresh themes for each switch
            btn->apply_theme(create_blue_theme());
            auto bg1 = btn->resolve_style().background_color;

            btn->apply_theme(create_red_theme());
            auto bg2 = btn->resolve_style().background_color;

            btn->apply_theme(create_blue_theme());
            auto bg3 = btn->resolve_style().background_color;

            btn->apply_theme(create_red_theme());
            auto bg4 = btn->resolve_style().background_color;

            // First and third should match (both blue)
            CHECK(bg1.r == bg3.r);
            CHECK(bg1.g == bg3.g);
            CHECK(bg1.b == bg3.b);

            // Second and fourth should match (both red)
            CHECK(bg2.r == bg4.r);
            CHECK(bg2.g == bg4.g);
            CHECK(bg2.b == bg4.b);

            // Blue and red should differ
            CHECK((bg1.r != bg2.r || bg1.b != bg2.b));
        }
    }

    // =========================================================================
    // CRITICAL: This test replicates the ACTUAL bug that was fixed
    // =========================================================================
    TEST_CASE("Theme Switching - Child added AFTER parent theme inherits via get_theme()") {
        auto blue_theme = create_blue_theme();
        auto red_theme = create_red_theme();

        auto root = std::make_unique<panel<Backend>>();

        SUBCASE("Child added after parent theme application gets theme") {
            // Apply theme to parent FIRST (like dos_theme_showcase initialization)
            root->apply_theme(std::move(blue_theme));

            // Add child AFTER parent has theme
            // This is how add_button(*this, "Normal") works in dos_theme_showcase
            // The child NEVER has apply_theme() called directly
            auto btn_ptr = std::make_unique<button<Backend>>("Test");
            auto* btn = btn_ptr.get();
            root->add_child(std::move(btn_ptr));

            // Child should inherit via get_theme() even without apply_theme() call
            CHECK(btn->has_theme());  // Should find theme via parent chain

            auto bg = btn->resolve_style().background_color;
            CHECK(bg.b >= 100);  // Should have blue theme colors
            CHECK(bg.r < 50);    // Low red component for blue theme
        }

        SUBCASE("ORIGINAL BUG: First theme switch updates child without apply_theme") {
            // This is the EXACT scenario that was broken before the fix:
            // 1. Parent has theme
            root->apply_theme(std::move(blue_theme));

            // 2. Child added after (never gets apply_theme called)
            auto btn_ptr = std::make_unique<button<Backend>>("Test");
            auto* btn = btn_ptr.get();
            root->add_child(std::move(btn_ptr));

            auto bg_blue = btn->resolve_style().background_color;
            CHECK(bg_blue.b >= 100);  // Should have blue

            // 3. Theme switch on parent - THIS WAS THE BUG
            // Before fix: btn->m_theme was nullptr, so do_render() would return early
            // After fix: btn->get_theme() walks parent chain, finds theme
            root->apply_theme(std::move(red_theme));

            auto bg_red = btn->resolve_style().background_color;

            // CRITICAL ASSERTION: First switch MUST work
            CHECK(bg_red.r >= 100);   // Should have red theme
            CHECK(bg_red.b < 50);     // Low blue component

            // Verify colors actually changed on FIRST switch
            CHECK((bg_blue.r != bg_red.r || bg_blue.b != bg_red.b));
        }

        SUBCASE("Multiple children added after theme all update on switch") {
            root->apply_theme(std::move(blue_theme));

            // Add multiple children after theme application
            auto btn1_ptr = std::make_unique<button<Backend>>("Btn1");
            auto* btn1 = btn1_ptr.get();
            root->add_child(std::move(btn1_ptr));

            auto btn2_ptr = std::make_unique<button<Backend>>("Btn2");
            auto* btn2 = btn2_ptr.get();
            root->add_child(std::move(btn2_ptr));

            auto lbl_ptr = std::make_unique<label<Backend>>("Label");
            auto* lbl = lbl_ptr.get();
            root->add_child(std::move(lbl_ptr));

            // All should have blue theme
            CHECK(btn1->resolve_style().background_color.b >= 100);
            CHECK(btn2->resolve_style().background_color.b >= 100);
            CHECK(lbl->resolve_style().background_color.b >= 100);

            // Switch theme
            root->apply_theme(std::move(red_theme));

            // ALL children must update on first switch
            CHECK(btn1->resolve_style().background_color.r >= 100);
            CHECK(btn2->resolve_style().background_color.r >= 100);
            CHECK(lbl->resolve_style().background_color.r >= 100);
        }
    }

    TEST_CASE("Theme Switching - Foreground colors update correctly") {
        auto blue_theme = create_blue_theme();
        auto red_theme = create_red_theme();

        auto btn = std::make_unique<button<Backend>>("Test");

        SUBCASE("Foreground color changes with theme") {
            btn->apply_theme(std::move(blue_theme));
            auto fg_blue = btn->resolve_style().foreground_color;

            btn->apply_theme(std::move(red_theme));
            auto fg_red = btn->resolve_style().foreground_color;

            // Foreground colors should change between themes
            bool const fg_changed = (fg_blue.r != fg_red.r) ||
                             (fg_blue.g != fg_red.g) ||
                             (fg_blue.b != fg_red.b);
            CHECK(fg_changed);
        }

        SUBCASE("Child inherits foreground color from parent") {
            auto root = std::make_unique<panel<Backend>>();
            auto btn_ptr = std::make_unique<button<Backend>>("Button");
            auto* btn_child = btn_ptr.get();
            root->add_child(std::move(btn_ptr));

            root->apply_theme(std::move(blue_theme));
            auto fg_blue = btn_child->resolve_style().foreground_color;

            root->apply_theme(std::move(red_theme));
            auto fg_red = btn_child->resolve_style().foreground_color;

            // Child's foreground should change when parent theme changes
            CHECK((fg_blue.r != fg_red.r || fg_blue.g != fg_red.g || fg_blue.b != fg_red.b));
        }
    }

    TEST_CASE("Theme Switching - has_theme() reports correctly") {
        auto blue_theme = create_blue_theme();

        SUBCASE("Widget without theme returns false") {
            auto btn = std::make_unique<button<Backend>>("Test");
            CHECK_FALSE(btn->has_theme());
        }

        SUBCASE("Widget with applied theme returns true") {
            auto btn = std::make_unique<button<Backend>>("Test");
            btn->apply_theme(std::move(blue_theme));
            CHECK(btn->has_theme());
        }

        SUBCASE("Child without direct theme but with themed parent returns true") {
            auto root = std::make_unique<panel<Backend>>();
            root->apply_theme(std::move(blue_theme));

            auto btn_ptr = std::make_unique<button<Backend>>("Button");
            auto* btn = btn_ptr.get();
            root->add_child(std::move(btn_ptr));

            // Child never had apply_theme() called, but parent has theme
            CHECK(btn->has_theme());  // Should find theme via CSS inheritance
        }

        SUBCASE("Orphaned widget has no theme") {
            auto btn = std::make_unique<button<Backend>>("Test");
            CHECK_FALSE(btn->has_theme());
        }
    }

    TEST_CASE("Theme Switching - Explicit color overrides respected") {
        auto blue_theme = create_blue_theme();

        SUBCASE("Explicit background override not changed by theme") {
            auto btn = std::make_unique<button<Backend>>("Test");

            // Set explicit override
            color_type const green{0, 255, 0};
            btn->set_background_color(green);

            // Apply theme - should NOT override explicit color
            btn->apply_theme(std::move(blue_theme));
            auto bg = btn->resolve_style().background_color;

            CHECK(bg.g == 255);  // Should still be green
            CHECK(bg.r == 0);
            CHECK(bg.b == 0);
        }

        SUBCASE("Clearing override allows theme to take effect") {
            auto btn = std::make_unique<button<Backend>>("Test");

            // Set explicit override
            color_type const green{0, 255, 0};
            btn->set_background_color(green);
            btn->apply_theme(std::move(blue_theme));

            auto bg_override = btn->resolve_style().background_color;
            CHECK(bg_override.g == 255);  // Green override

            // Clear override
            btn->clear_background_color();
            auto bg_theme = btn->resolve_style().background_color;

            // Now should get theme color (blue)
            CHECK(bg_theme.b >= 100);
            CHECK(bg_theme.g < 50);
        }
    }

    TEST_CASE("Theme Switching - Deep nesting propagates correctly") {
        auto blue_theme = create_blue_theme();
        auto red_theme = create_red_theme();

        // Create 3-level hierarchy: root -> container -> subpanel -> button
        auto root = std::make_unique<panel<Backend>>();

        auto container_ptr = std::make_unique<panel<Backend>>();
        auto* container = container_ptr.get();
        root->add_child(std::move(container_ptr));

        auto subpanel_ptr = std::make_unique<panel<Backend>>();
        auto* subpanel = subpanel_ptr.get();
        container->add_child(std::move(subpanel_ptr));

        auto btn_ptr = std::make_unique<button<Backend>>("DeepButton");
        auto* btn = btn_ptr.get();
        subpanel->add_child(std::move(btn_ptr));

        SUBCASE("Theme propagates through 3 levels") {
            root->apply_theme(std::move(blue_theme));

            // All levels should have theme
            CHECK(root->has_theme());
            CHECK(container->has_theme());
            CHECK(subpanel->has_theme());
            CHECK(btn->has_theme());

            // Deepest child should have blue theme
            auto bg = btn->resolve_style().background_color;
            CHECK(bg.b >= 50);
        }

        SUBCASE("Theme switch propagates to deepest child") {
            root->apply_theme(std::move(blue_theme));
            auto bg_blue = btn->resolve_style().background_color;

            root->apply_theme(std::move(red_theme));
            auto bg_red = btn->resolve_style().background_color;

            // Deepest child must update
            CHECK((bg_blue.r != bg_red.r || bg_blue.b != bg_red.b));
            CHECK(bg_red.r >= 50);  // Red theme
        }
    }

    TEST_CASE("Theme Switching - Theme application order independence") {
        auto blue_theme = create_blue_theme();

        SUBCASE("Child added before parent theme application") {
            auto root = std::make_unique<panel<Backend>>();

            auto btn_ptr = std::make_unique<button<Backend>>("Button");
            auto* btn = btn_ptr.get();
            root->add_child(std::move(btn_ptr));

            // Apply theme AFTER child added
            root->apply_theme(std::move(blue_theme));

            auto bg = btn->resolve_style().background_color;
            CHECK(bg.b >= 100);
        }

        SUBCASE("Child added after parent theme application") {
            auto root = std::make_unique<panel<Backend>>();

            // Apply theme BEFORE child added
            root->apply_theme(std::move(blue_theme));

            auto btn_ptr = std::make_unique<button<Backend>>("Button");
            auto* btn = btn_ptr.get();
            root->add_child(std::move(btn_ptr));

            auto bg = btn->resolve_style().background_color;
            CHECK(bg.b >= 100);
        }

        SUBCASE("Both orderings produce identical results") {
            // Scenario 1: child first, theme second
            auto root1 = std::make_unique<panel<Backend>>();
            auto btn1_ptr = std::make_unique<button<Backend>>("Btn");
            auto* btn1 = btn1_ptr.get();
            root1->add_child(std::move(btn1_ptr));
            root1->apply_theme(create_blue_theme());  // Fresh theme for comparison
            auto bg1 = btn1->resolve_style().background_color;

            // Scenario 2: theme first, child second (should produce identical results)
            auto root2 = std::make_unique<panel<Backend>>();
            root2->apply_theme(create_blue_theme());  // Same theme, fresh copy
            auto btn2_ptr = std::make_unique<button<Backend>>("Btn");
            auto* btn2 = btn2_ptr.get();
            root2->add_child(std::move(btn2_ptr));
            auto bg2 = btn2->resolve_style().background_color;

            // Results should be identical
            CHECK(bg1.r == bg2.r);
            CHECK(bg1.g == bg2.g);
            CHECK(bg1.b == bg2.b);
        }
    }
}
