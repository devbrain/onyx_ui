//
// test_panel_layout.cc - Critical layout integration tests for panel widget
//
// These tests verify that borders, padding, and margins correctly affect
// child positioning - the exact gap that allowed the y=65541 bug to slip through.
//

#include <doctest/doctest.h>
#include <../../include/onyxui/widgets/containers/panel.hh>
#include <onyxui/widgets/label.hh>
#include <../../include/onyxui/services/ui_context.hh>
#include "../utils/test_canvas_backend.hh"
#include "../utils/test_helpers.hh"
#include "../utils/layout_assertions.hh"
#include "onyxui/concepts/rect_like.hh"
#include "../../include/onyxui/core/element.hh"

using namespace onyxui;
using namespace onyxui::testing;

TEST_SUITE("Panel - Layout Integration (CRITICAL)") {
    using Backend = test_canvas_backend;

    // Create UI context to register themes
    struct test_fixture {
        scoped_ui_context<Backend> ctx;

        // Helper to apply default theme to a widget
        template<typename Widget>
        void apply_default_theme([[maybe_unused]] Widget& w) {
            // Use "Canvas Test Theme" which is guaranteed to have border settings
            if (auto* theme = ctx.themes().get_theme("Canvas Test Theme")) {
//                 w.apply_theme(*theme);  // No longer needed - widgets use global theme
            }
        }
    };

    TEST_CASE_FIXTURE(test_fixture, "Panel with border - child positioned inside border") {
        panel<Backend> p;
        apply_default_theme(p);
        p.set_has_border(true);
        p.set_vbox_layout(spacing::none);

        auto* child = p.emplace_child<label>("Test");

        (void)p.measure(100_lu, 100_lu);
        p.arrange(logical_rect{0_lu, 0_lu, 100_lu, 100_lu});

        // RELATIVE COORDINATES: child at (0,0) relative to parent's content area
        auto child_bounds = child->bounds();
        CHECK(child_bounds.x.to_int() == 0);
        CHECK(child_bounds.y.to_int() == 0);
        CHECK(child_bounds.width.to_int() == 98);  // 100 - 2*1
        // Note: height is 1 (label's natural height), not 98 (labels don't expand by default)
    }

    TEST_CASE_FIXTURE(test_fixture, "Panel with border - VISUAL verification") {
        // Set Canvas Test Theme as current (needed for render_to_canvas)
        ctx.themes().set_current_theme("Canvas Test Theme");

        panel<Backend> p;
        apply_default_theme(p);
        p.set_has_border(true);
        p.set_vbox_layout(spacing::none);

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
        p.set_padding(logical_thickness(3_lu));
        p.set_vbox_layout(spacing::none);

        auto* child = p.emplace_child<label>("Test");

        (void)p.measure(100_lu, 100_lu);
        p.arrange(logical_rect{0_lu, 0_lu, 100_lu, 100_lu});

        // RELATIVE COORDINATES: child at (0,0) relative to parent's content area
        // Padding offset handled internally by parent
        auto child_bounds = child->bounds();
        CHECK(child_bounds.x.to_int() == 0);
        CHECK(child_bounds.y.to_int() == 0);
        CHECK(child_bounds.width.to_int() == 94);  // 100 - 2*3 padding
    }

    TEST_CASE_FIXTURE(test_fixture, "Panel with border AND padding - compound spacing") {
        // Set Canvas Test Theme as current (needed for render_to_canvas)
        ctx.themes().set_current_theme("Canvas Test Theme");

        panel<Backend> p;
        apply_default_theme(p);
        p.set_has_border(true);           // +1 on each side
        p.set_padding(logical_thickness(2_lu)); // +2 on each side
        p.set_vbox_layout(spacing::none);

        auto* child = p.emplace_child<label>("Test");

        (void)p.measure(100_lu, 100_lu);
        p.arrange(logical_rect{0_lu, 0_lu, 100_lu, 100_lu});

        // RELATIVE COORDINATES: child at (0,0) relative to parent's content area
        // Border and padding offsets handled internally by parent
        auto child_bounds = child->bounds();
        CHECK(child_bounds.x.to_int() == 0);
        CHECK(child_bounds.y.to_int() == 0);
        CHECK(child_bounds.width.to_int() == 94);  // 100 - 2*(1+2)
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

        // Set Canvas Test Theme as current (needed for render_to_canvas)
        ctx.themes().set_current_theme("Canvas Test Theme");

        panel<Backend> main_panel;
        apply_default_theme(main_panel);
        main_panel.set_vbox_layout(spacing::none);
        main_panel.set_padding(logical_thickness(0_lu));

        // Create demo panel with border (the problematic one)
        auto* demo_panel = main_panel.emplace_child<panel>();
        demo_panel->set_has_border(true);
        demo_panel->set_padding(logical_thickness(1_lu));
        demo_panel->set_vbox_layout(spacing::tiny);

        // Add the "Panel with Border" label
        auto* label_ptr = demo_panel->emplace_child<label>("Panel with Border");

        // Measure and arrange
        (void)main_panel.measure(155_lu, 100_lu);
        main_panel.arrange(logical_rect{0_lu, 0_lu, 155_lu, 100_lu});

        // The bug: label_ptr->bounds() had y=65541 (overflow from negative)
        // With the fix, y should be positive and small
        auto label_bounds = label_ptr->bounds();
        int y = label_bounds.y.to_int();

        INFO("Label y-coordinate: ", y);
        INFO("Label bounds: (", label_bounds.x.to_int(), ",", y,
             ") ", label_bounds.width.to_int(), "x", label_bounds.height.to_int());

        // RELATIVE COORDINATES: label at (0,0) relative to demo_panel's content area
        // Border and padding offsets handled internally by parent
        CHECK(y == 0);
        CHECK(y < 100);  // Should be well within bounds

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
        p.set_vbox_layout(spacing::none);

        auto* child = p.emplace_child<label>("Test");

        (void)p.measure(100_lu, 100_lu);
        p.arrange(logical_rect{0_lu, 0_lu, 100_lu, 100_lu});

        // Child should start at 0,0 (no border offset)
        CHECK(child->bounds().x.to_int() == 0);
        CHECK(child->bounds().y.to_int() == 0);
    }
}
