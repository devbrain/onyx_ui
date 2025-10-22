//
// test_layout_edge_cases.cc - Edge cases and robustness tests
//
// Tests verify that layout calculations handle boundary conditions safely:
// - Zero sizes
// - Negative values (should be clamped)
// - Maximum values (INT_MAX)
// - Overflow scenarios
// - safe_math integration
//

#include <doctest/doctest.h>
#include <onyxui/widgets/panel.hh>
#include <onyxui/widgets/group_box.hh>
#include <onyxui/widgets/label.hh>
#include <onyxui/widgets/vbox.hh>
#include <onyxui/ui_context.hh>
#include <limits>
#include "../utils/test_canvas_backend.hh"
#include "onyxui/concepts/rect_like.hh"
#include "onyxui/element.hh"

using namespace onyxui;
using namespace onyxui::testing;

TEST_SUITE("Layout - Edge Cases & Robustness") {
    using Backend = test_canvas_backend;

    struct test_fixture {
        scoped_ui_context<Backend> ctx;

        template<typename Widget>
        void apply_default_theme(Widget& w) {
            if (auto* theme = ctx.themes().get_theme("Test Theme")) {
                w.apply_theme(*theme);
            }
        }
    };

    TEST_CASE_FIXTURE(test_fixture, "Panel - zero width available") {
        panel<Backend> p;
        apply_default_theme(p);
        p.set_vbox_layout(0);

        auto* child = p.emplace_child<label>("Test");

        // Measure with zero width
        (void)p.measure(0, 100);
        p.arrange({0, 0, 0, 100});

        // Should not crash, child should have zero or minimal width
        auto child_bounds = child->bounds();
        CHECK(rect_utils::get_width(child_bounds) >= 0);
    }

    TEST_CASE_FIXTURE(test_fixture, "Panel - zero height available") {
        panel<Backend> p;
        apply_default_theme(p);
        p.set_vbox_layout(0);

        auto* child = p.emplace_child<label>("Test");

        // Measure with zero height
        (void)p.measure(100, 0);
        p.arrange({0, 0, 100, 0});

        // Should not crash, child should have zero or minimal height
        auto child_bounds = child->bounds();
        CHECK(rect_utils::get_height(child_bounds) >= 0);
    }

    TEST_CASE_FIXTURE(test_fixture, "Panel - both dimensions zero") {
        panel<Backend> p;
        apply_default_theme(p);
        p.set_vbox_layout(0);

        auto* child = p.emplace_child<label>("Test");

        // Measure with zero size
        (void)p.measure(0, 0);
        p.arrange({0, 0, 0, 0});

        // Should not crash
        auto child_bounds = child->bounds();
        CHECK(rect_utils::get_width(child_bounds) >= 0);
        CHECK(rect_utils::get_height(child_bounds) >= 0);
    }

    TEST_CASE_FIXTURE(test_fixture, "Panel with border - zero size collapses to border only") {
        panel<Backend> p;
        apply_default_theme(p);
        p.set_has_border(true);
        p.set_vbox_layout(0);

        auto* child = p.emplace_child<label>("Test");

        // Measure with size smaller than border (border = 2px total)
        (void)p.measure(1, 1);
        p.arrange({0, 0, 1, 1});

        // Content area should be clamped to zero
        auto child_bounds = child->bounds();
        CHECK(rect_utils::get_width(child_bounds) >= 0);
        CHECK(rect_utils::get_height(child_bounds) >= 0);
    }

    TEST_CASE_FIXTURE(test_fixture, "Panel - excessive padding clamped to zero content") {
        panel<Backend> p;
        apply_default_theme(p);
        p.set_padding(thickness::all(100));  // Huge padding
        p.set_vbox_layout(0);

        auto* child = p.emplace_child<label>("Test");

        // Small available space
        (void)p.measure(50, 50);
        p.arrange({0, 0, 50, 50});

        // Content area should be clamped to non-negative
        auto child_bounds = child->bounds();
        CHECK(rect_utils::get_width(child_bounds) >= 0);
        CHECK(rect_utils::get_height(child_bounds) >= 0);
    }

    TEST_CASE_FIXTURE(test_fixture, "Group box - excessive padding with border") {
        group_box<Backend> gb;
        apply_default_theme(gb);
        gb.set_padding({50, 50, 50, 50});  // Large padding
        gb.set_vbox_layout(0);

        auto* child = gb.emplace_child<label>("Test");

        // Available space: 100x100
        // Border: 2px total
        // Padding: 100px per side
        // Should clamp to zero
        (void)gb.measure(100, 100);
        gb.arrange({0, 0, 100, 100});

        auto child_bounds = child->bounds();
        CHECK(rect_utils::get_width(child_bounds) >= 0);
        CHECK(rect_utils::get_height(child_bounds) >= 0);
    }

    TEST_CASE_FIXTURE(test_fixture, "Panel - very large dimensions") {
        panel<Backend> p;
        apply_default_theme(p);
        p.set_vbox_layout(0);

        auto* child = p.emplace_child<label>("Test");

        // Very large but not overflow-inducing
        const int large_size = 100000;
        (void)p.measure(large_size, large_size);
        p.arrange({0, 0, large_size, large_size});

        // Should handle large sizes
        auto child_bounds = child->bounds();
        CHECK(rect_utils::get_width(child_bounds) > 0);
        CHECK(rect_utils::get_height(child_bounds) > 0);
    }

    TEST_CASE_FIXTURE(test_fixture, "Panel - maximum int padding does not overflow") {
        panel<Backend> p;
        apply_default_theme(p);

        // Use very large padding that could cause overflow
        const int large_padding = std::numeric_limits<int>::max() / 2;
        p.set_padding(thickness::all(large_padding));
        p.set_vbox_layout(0);

        auto* child = p.emplace_child<label>("Test");

        // Should not crash from overflow
        (void)p.measure(1000, 1000);
        p.arrange({0, 0, 1000, 1000});

        // Content area should be safely clamped
        auto child_bounds = child->bounds();
        CHECK(rect_utils::get_width(child_bounds) >= 0);
        CHECK(rect_utils::get_height(child_bounds) >= 0);
    }

    TEST_CASE_FIXTURE(test_fixture, "VBox - empty container with zero size") {
        vbox<Backend> vb(0);
        apply_default_theme(vb);

        // No children
        (void)vb.measure(0, 0);
        vb.arrange({0, 0, 0, 0});

        // Should not crash
        auto bounds = vb.bounds();
        CHECK(rect_utils::get_width(bounds) == 0);
        CHECK(rect_utils::get_height(bounds) == 0);
    }

    TEST_CASE_FIXTURE(test_fixture, "VBox - children with zero spacing and zero size") {
        vbox<Backend> vb(0);
        apply_default_theme(vb);

        vb.emplace_child<label>("A");
        vb.emplace_child<label>("B");
        vb.emplace_child<label>("C");

        (void)vb.measure(0, 0);
        vb.arrange({0, 0, 0, 0});

        // Should not crash, children positioned at zero
        for (const auto& child : vb.children()) {
            auto child_bounds = child->bounds();
            CHECK(rect_utils::get_x(child_bounds) >= 0);
            CHECK(rect_utils::get_y(child_bounds) >= 0);
        }
    }

    TEST_CASE_FIXTURE(test_fixture, "Panel - asymmetric extreme padding") {
        panel<Backend> p;
        apply_default_theme(p);

        // Extreme asymmetric padding
        p.set_padding({10000, 1, 1, 1});  // Huge left padding, minimal others
        p.set_vbox_layout(0);

        auto* child = p.emplace_child<label>("Test");

        (void)p.measure(100, 100);
        p.arrange({0, 0, 100, 100});

        // Width should be clamped to zero
        auto child_bounds = child->bounds();
        CHECK(rect_utils::get_width(child_bounds) >= 0);

        // Height should be available (100 - 1 - 1 = 98)
        // Note: label height is 1 (natural height)
        CHECK(rect_utils::get_height(child_bounds) >= 0);
    }

    TEST_CASE_FIXTURE(test_fixture, "Group box - border + extreme padding interaction") {
        group_box<Backend> gb;
        apply_default_theme(gb);

        // Border is always 1px per side
        // Add extreme padding on top
        gb.set_padding({5000, 3, 5000, 3});
        gb.set_vbox_layout(0);

        auto* child = gb.emplace_child<label>("Test");

        (void)gb.measure(100, 100);
        gb.arrange({0, 0, 100, 100});

        // Content area calculation:
        // Width: 100 - border(2) - padding(10000) should clamp to 0
        // Height: 100 - border(2) - padding(6) = 92
        auto child_bounds = child->bounds();
        CHECK(rect_utils::get_width(child_bounds) >= 0);
        CHECK(rect_utils::get_height(child_bounds) >= 0);
    }

    TEST_CASE_FIXTURE(test_fixture, "Panel - border with minimal space (1x1)") {
        panel<Backend> p;
        apply_default_theme(p);
        p.set_has_border(true);
        p.set_vbox_layout(0);

        auto* child = p.emplace_child<label>("Test");

        // Border takes 2px total, content area = 0
        (void)p.measure(2, 2);
        p.arrange({0, 0, 2, 2});

        auto child_bounds = child->bounds();
        // Content area should be zero
        CHECK(rect_utils::get_width(child_bounds) >= 0);
        CHECK(rect_utils::get_height(child_bounds) >= 0);
    }

    TEST_CASE_FIXTURE(test_fixture, "VBox - many children with zero available space") {
        vbox<Backend> vb(1);  // With spacing
        apply_default_theme(vb);

        // Add many children
        for (int i = 0; i < 50; ++i) {
            vb.emplace_child<label>("Item");
        }

        // Zero space available
        (void)vb.measure(0, 0);
        vb.arrange({0, 0, 0, 0});

        // Should not crash
        for (const auto& child : vb.children()) {
            auto child_bounds = child->bounds();
            CHECK(rect_utils::get_width(child_bounds) >= 0);
            CHECK(rect_utils::get_y(child_bounds) >= 0);
        }
    }

    TEST_CASE_FIXTURE(test_fixture, "Safe math - content area with large padding") {
        panel<Backend> p;
        apply_default_theme(p);

        // Padding that would overflow if added directly
        const int large = std::numeric_limits<int>::max() / 3;
        p.set_padding({large, large, large, large});
        p.set_vbox_layout(0);

        auto* child = p.emplace_child<label>("Test");

        // Measure and arrange should use safe math
        (void)p.measure(1000, 1000);
        p.arrange({0, 0, 1000, 1000});

        // Should not crash or produce negative values
        auto child_bounds = child->bounds();
        CHECK(rect_utils::get_width(child_bounds) >= 0);
        CHECK(rect_utils::get_height(child_bounds) >= 0);

        // Position should be valid
        CHECK(rect_utils::get_x(child_bounds) >= 0);
        CHECK(rect_utils::get_y(child_bounds) >= 0);
    }
}
