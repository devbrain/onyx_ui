//
// test_panel_layout.cc - Critical layout integration tests for panel widget
//
// These tests verify that borders, padding, and margins correctly affect
// child positioning - the exact gap that allowed the y=65541 bug to slip through.
//

#include <doctest/doctest.h>
#include <onyxui/widgets/panel.hh>
#include <onyxui/widgets/label.hh>
#include <onyxui/ui_context.hh>
#include "../utils/test_canvas_backend.hh"
#include "../utils/test_helpers.hh"
#include "../utils/layout_assertions.hh"
#include "onyxui/concepts/rect_like.hh"
#include "onyxui/element.hh"

using namespace onyxui;
using namespace onyxui::testing;

TEST_SUITE("Panel - Layout Integration (CRITICAL)") {
    using Backend = test_canvas_backend;

    // Create UI context to register themes
    struct test_fixture {
        scoped_ui_context<Backend> ctx;

        // Helper to apply default theme to a widget
        template<typename Widget>
        void apply_default_theme(Widget& w) {
            // Use "Canvas Test Theme" which is guaranteed to have border settings
            if (auto* theme = ctx.themes().get_theme("Canvas Test Theme")) {
                w.apply_theme(*theme);
            }
        }
    };

    TEST_CASE_FIXTURE(test_fixture, "Panel with border - child positioned inside border") {
        panel<Backend> p;
        apply_default_theme(p);
        p.set_has_border(true);
        p.set_vbox_layout(0);

        auto* child = p.emplace_child<label>("Test");

        (void)p.measure(100, 100);
        p.arrange({0, 0, 100, 100});

        // Child should be inset by 1 pixel for border
        auto child_bounds = child->bounds();
        CHECK(rect_utils::get_x(child_bounds) == 1);
        CHECK(rect_utils::get_y(child_bounds) == 1);
        CHECK(rect_utils::get_width(child_bounds) == 98);  // 100 - 2*1
        // Note: height is 1 (label's natural height), not 98 (labels don't expand by default)
    }

    TEST_CASE_FIXTURE(test_fixture, "Panel with border - VISUAL verification") {
        panel<Backend> p;
        apply_default_theme(p);
        p.set_has_border(true);
        p.set_vbox_layout(0);

        p.emplace_child<label>("Hello");

        // Render to canvas
        auto canvas = render_to_canvas(p, 20, 5);

        INFO("Rendered panel:\n", debug_canvas(*canvas));

        // Verify border drawn at edges
        assert_border_at_rect(*canvas, 0, 0, 20, 5, "Panel border");

        // Verify text inside border (positioned correctly at 1,1)
        assert_text_at(*canvas, "Hello", 1, 1, "Text inside border");
    }

    TEST_CASE_FIXTURE(test_fixture, "Panel with padding - child positioned inside padding") {
        panel<Backend> p;
        apply_default_theme(p);
        p.set_padding(thickness::all(3));
        p.set_vbox_layout(0);

        auto* child = p.emplace_child<label>("Test");

        (void)p.measure(100, 100);
        p.arrange({0, 0, 100, 100});

        // Child should be inset by 3 pixels for padding
        assert_child_inset(p, *child, 3, 3, "Padding should create 3px inset");
    }

    TEST_CASE_FIXTURE(test_fixture, "Panel with border AND padding - compound spacing") {
        panel<Backend> p;
        apply_default_theme(p);
        p.set_has_border(true);           // +1 on each side
        p.set_padding(thickness::all(2)); // +2 on each side
        p.set_vbox_layout(0);

        auto* child = p.emplace_child<label>("Test");

        (void)p.measure(100, 100);
        p.arrange({0, 0, 100, 100});

        // Child should be inset by border(1) + padding(2) = 3
        assert_child_inset(p, *child, 3, 3, "Border + padding = 3px inset");

        // Verify size
        auto child_bounds = child->bounds();
        CHECK(rect_utils::get_width(child_bounds) == 94);  // 100 - 2*3
        // Note: height is 1 (label's natural height), not 94

        // Visual verification
        auto canvas = render_to_canvas(p, 25, 8);
        INFO("Panel with border and padding:\n", debug_canvas(*canvas));

        // Border should be drawn
        assert_border_at_rect(*canvas, 0, 0, 25, 8, "Panel border");

        // Text should be at (3, 3) due to border(1) + padding(2)
        assert_text_at(*canvas, "Test", 3, 3, "Text inside border + padding");
    }

    TEST_CASE_FIXTURE(test_fixture, "REGRESSION: Border bug that caused y=65541 overflow") {
        // This test recreates the exact scenario from dos_theme_showcase
        // that caused the VRAM out-of-range error

        panel<Backend> main_panel;
        apply_default_theme(main_panel);
        main_panel.set_vbox_layout(0);
        main_panel.set_padding(thickness::all(0));

        // Create demo panel with border (the problematic one)
        auto* demo_panel = main_panel.emplace_child<panel>();
        demo_panel->set_has_border(true);
        demo_panel->set_padding(thickness::all(1));
        demo_panel->set_vbox_layout(1);

        // Add the "Panel with Border" label
        auto* label_ptr = demo_panel->emplace_child<label>("Panel with Border");

        // Measure and arrange
        (void)main_panel.measure(155, 100);
        main_panel.arrange({0, 0, 155, 100});

        // The bug: label_ptr->bounds() had y=65541 (overflow from negative)
        // With the fix, y should be positive and small
        auto label_bounds = label_ptr->bounds();
        int y = rect_utils::get_y(label_bounds);

        INFO("Label y-coordinate: ", y);
        INFO("Label bounds: (", rect_utils::get_x(label_bounds), ",", y,
             ") ", rect_utils::get_width(label_bounds), "x", rect_utils::get_height(label_bounds));

        // Should be inside demo_panel's border and padding
        CHECK(y >= 0);
        CHECK(y < 100);  // Should be well within bounds
        CHECK(y == 2);   // Should be exactly: border(1) + padding(1)

        // Visual verification - ensure the panel renders correctly
        auto canvas = render_to_canvas(main_panel, 30, 10);
        INFO("Regression test visual output:\n", debug_canvas(*canvas));

        // The demo panel should have a border
        // Panel sizes to content: border(2) + padding(2) + label(1) = 5 rows
        // Note: demo_panel is at (0,0) in main_panel since main has no border/padding
        assert_border_at_rect(*canvas, 0, 0, 30, 5, "Demo panel border");

        // The text should be visible inside the border + padding
        assert_text_at(*canvas, "Panel with Border", 2, 2, "Label inside bordered panel");
    }

    TEST_CASE_FIXTURE(test_fixture, "Panel without border - no offset") {
        panel<Backend> p;
        apply_default_theme(p);
        p.set_has_border(false);  // Explicitly no border
        p.set_vbox_layout(0);

        auto* child = p.emplace_child<label>("Test");

        (void)p.measure(100, 100);
        p.arrange({0, 0, 100, 100});

        // Child should start at 0,0 (no border offset)
        CHECK(rect_utils::get_x(child->bounds()) == 0);
        CHECK(rect_utils::get_y(child->bounds()) == 0);
    }
}
