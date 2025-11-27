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
#include <../../include/onyxui/widgets/containers/panel.hh>
#include <../../include/onyxui/widgets/containers/group_box.hh>
#include <onyxui/widgets/label.hh>
#include <../../include/onyxui/widgets/containers/vbox.hh>
#include <../../include/onyxui/services/ui_context.hh>
#include <limits>
#include "../utils/test_canvas_backend.hh"
#include "onyxui/concepts/rect_like.hh"
#include "../../include/onyxui/core/element.hh"

using namespace onyxui;
using namespace onyxui::testing;

TEST_SUITE("Layout - Edge Cases & Robustness") {
    using Backend = test_canvas_backend;

    struct test_fixture {
        scoped_ui_context<Backend> ctx;

        template<typename Widget>
        void apply_default_theme([[maybe_unused]] Widget& w) {
            if (auto* theme = ctx.themes().get_theme("Test Theme")) {
//                 w.apply_theme(*theme);  // No longer needed - widgets use global theme
            }
        }
    };

    TEST_CASE_FIXTURE(test_fixture, "Panel - zero width available") {
        panel<Backend> p;
        apply_default_theme(p);
        p.set_vbox_layout(spacing::none);

        auto* child = p.emplace_child<label>("Test");

        // Measure with zero width
        (void)p.measure(0_lu, 100_lu);
        p.arrange(logical_rect{0_lu, 0_lu, 0_lu, 100_lu});

        // Should not crash, child should have zero or minimal width
        auto child_bounds = child->bounds();
        CHECK(child_bounds.width.to_int() >= 0);
    }

    TEST_CASE_FIXTURE(test_fixture, "Panel - zero height available") {
        panel<Backend> p;
        apply_default_theme(p);
        p.set_vbox_layout(spacing::none);

        auto* child = p.emplace_child<label>("Test");

        // Measure with zero height
        (void)p.measure(100_lu, 0_lu);
        p.arrange(logical_rect{0_lu, 0_lu, 100_lu, 0_lu});

        // Should not crash, child should have zero or minimal height
        auto child_bounds = child->bounds();
        CHECK(child_bounds.height.to_int() >= 0);
    }

    TEST_CASE_FIXTURE(test_fixture, "Panel - both dimensions zero") {
        panel<Backend> p;
        apply_default_theme(p);
        p.set_vbox_layout(spacing::none);

        auto* child = p.emplace_child<label>("Test");

        // Measure with zero size
        (void)p.measure(0_lu, 0_lu);
        p.arrange(logical_rect{0_lu, 0_lu, 0_lu, 0_lu});

        // Should not crash
        auto child_bounds = child->bounds();
        CHECK(child_bounds.width.to_int() >= 0);
        CHECK(child_bounds.height.to_int() >= 0);
    }

    TEST_CASE_FIXTURE(test_fixture, "Panel with border - zero size collapses to border only") {
        panel<Backend> p;
        apply_default_theme(p);
        p.set_has_border(true);
        p.set_vbox_layout(spacing::none);

        auto* child = p.emplace_child<label>("Test");

        // Measure with size smaller than border (border = 2px total)
        (void)p.measure(1_lu, 1_lu);
        p.arrange(logical_rect{0_lu, 0_lu, 1_lu, 1_lu});

        // Content area should be clamped to zero
        auto child_bounds = child->bounds();
        CHECK(child_bounds.width.to_int() >= 0);
        CHECK(child_bounds.height.to_int() >= 0);
    }

    TEST_CASE_FIXTURE(test_fixture, "Panel - excessive padding clamped to zero content") {
        panel<Backend> p;
        apply_default_theme(p);
        p.set_padding(logical_thickness(100_lu));  // Huge padding
        p.set_vbox_layout(spacing::none);

        auto* child = p.emplace_child<label>("Test");

        // Small available space
        (void)p.measure(50_lu, 50_lu);
        p.arrange(logical_rect{0_lu, 0_lu, 50_lu, 50_lu});

        // Content area should be clamped to non-negative
        auto child_bounds = child->bounds();
        CHECK(child_bounds.width.to_int() >= 0);
        CHECK(child_bounds.height.to_int() >= 0);
    }

    TEST_CASE_FIXTURE(test_fixture, "Group box - excessive padding with border") {
        group_box<Backend> gb;
        apply_default_theme(gb);
        gb.set_padding(logical_thickness{50_lu, 50_lu, 50_lu, 50_lu});  // Large padding
        gb.set_vbox_layout(spacing::none);

        auto* child = gb.emplace_child<label>("Test");

        // Available space: 100x100
        // Border: 2px total
        // Padding: 100px per side
        // Should clamp to zero
        (void)gb.measure(100_lu, 100_lu);
        gb.arrange(logical_rect{0_lu, 0_lu, 100_lu, 100_lu});

        auto child_bounds = child->bounds();
        CHECK(child_bounds.width.to_int() >= 0);
        CHECK(child_bounds.height.to_int() >= 0);
    }

    TEST_CASE_FIXTURE(test_fixture, "Panel - very large dimensions") {
        panel<Backend> p;
        apply_default_theme(p);
        p.set_vbox_layout(spacing::none);

        auto* child = p.emplace_child<label>("Test");

        // Very large but not overflow-inducing
        const int large_size = 100000;
        (void)p.measure(logical_unit(static_cast<double>(large_size)), logical_unit(static_cast<double>(large_size)));
        p.arrange(logical_rect{0_lu, 0_lu, logical_unit(static_cast<double>(large_size)), logical_unit(static_cast<double>(large_size))});

        // Should handle large sizes
        auto child_bounds = child->bounds();
        CHECK(child_bounds.width.to_int() > 0);
        CHECK(child_bounds.height.to_int() > 0);
    }

    TEST_CASE_FIXTURE(test_fixture, "Panel - maximum int padding does not overflow") {
        panel<Backend> p;
        apply_default_theme(p);

        // Use very large padding that could cause overflow
        const int large_padding = std::numeric_limits<int>::max() / 2;
        p.set_padding(logical_thickness(logical_unit(static_cast<double>(large_padding))));
        p.set_vbox_layout(spacing::none);

        auto* child = p.emplace_child<label>("Test");

        // Should not crash from overflow
        (void)p.measure(1000_lu, 1000_lu);
        p.arrange(logical_rect{0_lu, 0_lu, 1000_lu, 1000_lu});

        // Content area should be safely clamped
        auto child_bounds = child->bounds();
        CHECK(child_bounds.width.to_int() >= 0);
        CHECK(child_bounds.height.to_int() >= 0);
    }

    TEST_CASE_FIXTURE(test_fixture, "VBox - empty container with zero size") {
        vbox<Backend> vb(spacing::none);
        apply_default_theme(vb);

        // No children
        (void)vb.measure(0_lu, 0_lu);
        vb.arrange(logical_rect{0_lu, 0_lu, 0_lu, 0_lu});

        // Should not crash
        auto bounds = vb.bounds();
        CHECK(bounds.width.to_int() == 0);
        CHECK(bounds.height.to_int() == 0);
    }

    TEST_CASE_FIXTURE(test_fixture, "VBox - children with zero spacing and zero size") {
        vbox<Backend> vb(spacing::none);
        apply_default_theme(vb);

        vb.emplace_child<label>("A");
        vb.emplace_child<label>("B");
        vb.emplace_child<label>("C");

        (void)vb.measure(0_lu, 0_lu);
        vb.arrange(logical_rect{0_lu, 0_lu, 0_lu, 0_lu});

        // Should not crash, children positioned at zero
        for (const auto& child : vb.children()) {
            auto child_bounds = child->bounds();
            CHECK(child_bounds.x.to_int() >= 0);
            CHECK(child_bounds.y.to_int() >= 0);
        }
    }

    TEST_CASE_FIXTURE(test_fixture, "Panel - asymmetric extreme padding") {
        panel<Backend> p;
        apply_default_theme(p);

        // Extreme asymmetric padding
        p.set_padding(logical_thickness{10000_lu, 1_lu, 1_lu, 1_lu});  // Huge left padding, minimal others
        p.set_vbox_layout(spacing::none);

        auto* child = p.emplace_child<label>("Test");

        (void)p.measure(100_lu, 100_lu);
        p.arrange(logical_rect{0_lu, 0_lu, 100_lu, 100_lu});

        // Width should be clamped to zero
        auto child_bounds = child->bounds();
        CHECK(child_bounds.width.to_int() >= 0);

        // Height should be available (100 - 1 - 1 = 98)
        // Note: label height is 1 (natural height)
        CHECK(child_bounds.height.to_int() >= 0);
    }

    TEST_CASE_FIXTURE(test_fixture, "Group box - border + extreme padding interaction") {
        group_box<Backend> gb;
        apply_default_theme(gb);

        // Border is always 1px per side
        // Add extreme padding on top
        gb.set_padding(logical_thickness{5000_lu, 3_lu, 5000_lu, 3_lu});
        gb.set_vbox_layout(spacing::none);

        auto* child = gb.emplace_child<label>("Test");

        (void)gb.measure(100_lu, 100_lu);
        gb.arrange(logical_rect{0_lu, 0_lu, 100_lu, 100_lu});

        // Content area calculation:
        // Width: 100 - border(2) - padding(10000) should clamp to 0
        // Height: 100 - border(2) - padding(6) = 92
        auto child_bounds = child->bounds();
        CHECK(child_bounds.width.to_int() >= 0);
        CHECK(child_bounds.height.to_int() >= 0);
    }

    TEST_CASE_FIXTURE(test_fixture, "Panel - border with minimal space (1x1)") {
        panel<Backend> p;
        apply_default_theme(p);
        p.set_has_border(true);
        p.set_vbox_layout(spacing::none);

        auto* child = p.emplace_child<label>("Test");

        // Border takes 2px total, content area = 0
        (void)p.measure(2_lu, 2_lu);
        p.arrange(logical_rect{0_lu, 0_lu, 2_lu, 2_lu});

        auto child_bounds = child->bounds();
        // Content area should be zero
        CHECK(child_bounds.width.to_int() >= 0);
        CHECK(child_bounds.height.to_int() >= 0);
    }

    TEST_CASE_FIXTURE(test_fixture, "VBox - many children with zero available space") {
        vbox<Backend> vb(spacing::tiny);
        apply_default_theme(vb);

        // Add many children
        for (int i = 0; i < 50; ++i) {
            vb.emplace_child<label>("Item");
        }

        // Zero space available
        (void)vb.measure(0_lu, 0_lu);
        vb.arrange(logical_rect{0_lu, 0_lu, 0_lu, 0_lu});

        // Should not crash
        for (const auto& child : vb.children()) {
            auto child_bounds = child->bounds();
            CHECK(child_bounds.width.to_int() >= 0);
            CHECK(child_bounds.y.to_int() >= 0);
        }
    }

    TEST_CASE_FIXTURE(test_fixture, "Safe math - content area with large padding") {
        panel<Backend> p;
        apply_default_theme(p);

        // Padding that would overflow if added directly
        const int large = std::numeric_limits<int>::max() / 3;
        p.set_padding(logical_thickness{logical_unit(static_cast<double>(large)), logical_unit(static_cast<double>(large)),
                                        logical_unit(static_cast<double>(large)), logical_unit(static_cast<double>(large))});
        p.set_vbox_layout(spacing::none);

        auto* child = p.emplace_child<label>("Test");

        // Measure and arrange should use safe math
        (void)p.measure(1000_lu, 1000_lu);
        p.arrange(logical_rect{0_lu, 0_lu, 1000_lu, 1000_lu});

        // Should not crash or produce negative values
        auto child_bounds = child->bounds();
        CHECK(child_bounds.width.to_int() >= 0);
        CHECK(child_bounds.height.to_int() >= 0);

        // Position should be valid
        CHECK(child_bounds.x.to_int() >= 0);
        CHECK(child_bounds.y.to_int() >= 0);
    }
}
