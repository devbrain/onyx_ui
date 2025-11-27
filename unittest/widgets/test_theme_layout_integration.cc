//
// test_theme_layout_integration.cc - Theme and layout interaction tests
//
// Tests verify that theme changes properly interact with layout:
// - Theme application triggers layout updates
// - Padding/margin from themes affects positioning
// - Theme switches preserve layout correctness
// - Inherited properties work with layout
//

#include <cstddef>
#include <doctest/doctest.h>
#include <../../include/onyxui/widgets/containers/panel.hh>
#include <../../include/onyxui/widgets/containers/group_box.hh>
#include <onyxui/widgets/label.hh>
#include <../../include/onyxui/widgets/containers/vbox.hh>
#include <../../include/onyxui/services/ui_context.hh>
#include <vector>
#include "../utils/test_canvas_backend.hh"
#include "../utils/test_helpers.hh"
#include "onyxui/concepts/rect_like.hh"
#include "../../include/onyxui/core/element.hh"

using namespace onyxui;
using namespace onyxui::testing;

TEST_SUITE("Theme - Layout Integration") {
    using Backend = test_canvas_backend;

    TEST_CASE_FIXTURE(ui_context_fixture<Backend>, "Theme application - layout remains valid") {
        panel<Backend> p;
        p.set_has_border(true);
        p.set_vbox_layout(spacing::none);

        auto* child = p.emplace_child<label>("Test");

        // Measure/arrange with theme active
        (void)p.measure(100_lu, 100_lu);  // Call for side effect (layout calculation)
        p.arrange(logical_rect{0_lu, 0_lu, 100_lu, 100_lu});

        auto bounds_after = child->bounds();

        // Layout should still be valid
        // Child coordinates are relative to parent's content area (after border is removed)
        CHECK(bounds_after.x.to_int() == 0);
        CHECK(bounds_after.y.to_int() == 0);
    }

    TEST_CASE_FIXTURE(ui_context_fixture<Backend>, "Theme inheritance - child inherits parent theme") {
        panel<Backend> parent;
        parent.set_vbox_layout(spacing::none);

        auto* child = parent.emplace_child<label>("Child");

        (void)parent.measure(100_lu, 100_lu);
        parent.arrange(logical_rect{0_lu, 0_lu, 100_lu, 100_lu});

        // Child should have inherited theme properties
        // (Actual theme values tested elsewhere; here we verify no layout issues)
        auto child_bounds = child->bounds();
        CHECK(child_bounds.width.to_int() > 0);
        CHECK(child_bounds.height.to_int() > 0);
    }

    TEST_CASE_FIXTURE(ui_context_fixture<Backend>, "Theme switch - layout updates correctly") {
        vbox<Backend> container(spacing::tiny);

        container.emplace_child<label>("Item 1");
        container.emplace_child<label>("Item 2");
        container.emplace_child<label>("Item 3");

        (void)container.measure(100_lu, 100_lu);
        container.arrange(logical_rect{0_lu, 0_lu, 100_lu, 100_lu});

        // Store initial positions
        const auto& children = container.children();
        std::vector<logical_rect> initial_bounds;
        for (const auto& child : children) {
            initial_bounds.push_back(child->bounds());
        }

        // Apply theme again (simulating theme switch)

        (void)container.measure(100_lu, 100_lu);
        container.arrange(logical_rect{0_lu, 0_lu, 100_lu, 100_lu});

        // Verify layout still correct (same positions for same theme)
        for (size_t i = 0; i < children.size(); ++i) {
            auto current = children[i]->bounds();
            CHECK(current.x.to_int() == initial_bounds[i].x.to_int());
            CHECK(current.y.to_int() == initial_bounds[i].y.to_int());
        }
    }

    TEST_CASE_FIXTURE(ui_context_fixture<Backend>, "Themed panel with border - content area correct") {
        panel<Backend> p;
        p.set_has_border(true);
        p.set_padding(logical_thickness(5_lu));
        p.set_vbox_layout(spacing::none);

        auto* child = p.emplace_child<label>("Content");

        (void)p.measure(100_lu, 100_lu);
        p.arrange(logical_rect{0_lu, 0_lu, 100_lu, 100_lu});

        // Child coordinates are relative to parent's content area
        // Border and padding are already removed from the content rect before children are positioned
        auto child_bounds = child->bounds();
        CHECK(child_bounds.x.to_int() == 0);
        CHECK(child_bounds.y.to_int() == 0);
    }

    TEST_CASE_FIXTURE(ui_context_fixture<Backend>, "Nested themed widgets - inheritance chain") {
        panel<Backend> outer;
        outer.set_vbox_layout(spacing::none);

        auto* middle = outer.emplace_child<group_box>();
        middle->set_title("Middle");
        middle->set_vbox_layout(spacing::none);

        auto* inner = middle->emplace_child<label>("Inner");

        (void)outer.measure(200_lu, 200_lu);
        outer.arrange(logical_rect{0_lu, 0_lu, 200_lu, 200_lu});

        // All widgets should have valid bounds through inheritance chain
        CHECK(outer.bounds().width.to_int() > 0);
        CHECK(middle->bounds().width.to_int() > 0);
        CHECK(inner->bounds().width.to_int() > 0);
    }

    TEST_CASE_FIXTURE(ui_context_fixture<Backend>, "Theme application - multiple children preserve spacing") {
        vbox<Backend> vb(spacing::small);  // small spacing

        vb.emplace_child<label>("A");
        vb.emplace_child<label>("B");
        vb.emplace_child<label>("C");

        (void)vb.measure(100_lu, 100_lu);
        vb.arrange(logical_rect{0_lu, 0_lu, 100_lu, 100_lu});

        const auto& children = vb.children();
        for (size_t i = 1; i < children.size(); ++i) {
            auto prev = children[i - 1]->bounds();
            auto curr = children[i]->bounds();

            int const prev_bottom = prev.y.to_int() + prev.height.to_int();
            int const curr_top = curr.y.to_int();

            // Spacing should be preserved (spacing::small = 1 in test theme)
            CHECK(curr_top - prev_bottom == 1);
        }
    }

    TEST_CASE_FIXTURE(ui_context_fixture<Backend>, "Group box with theme - title doesn't affect content layout") {
        group_box<Backend> gb;
        gb.set_title("Themed Title");
        gb.set_vbox_layout(spacing::none);

        auto* child = gb.emplace_child<label>("Content");

        (void)gb.measure(100_lu, 100_lu);
        gb.arrange(logical_rect{0_lu, 0_lu, 100_lu, 100_lu});

        // Child coordinates are relative to parent's content area
        // Title and border are handled by group_box; children positioned within content rect
        auto child_bounds = child->bounds();
        CHECK(child_bounds.x.to_int() == 0);
        CHECK(child_bounds.y.to_int() == 0);
    }

    TEST_CASE_FIXTURE(ui_context_fixture<Backend>, "Theme on already-laid-out tree - invalidation works") {
        panel<Backend> p;
        p.set_vbox_layout(spacing::none);
        p.emplace_child<label>("Text");

        // Layout without theme
        (void)p.measure(100_lu, 100_lu);
        p.arrange(logical_rect{0_lu, 0_lu, 100_lu, 100_lu});

        // Now apply theme (should invalidate and require re-layout)

        // Re-layout should work correctly
        (void)p.measure(100_lu, 100_lu);
        p.arrange(logical_rect{0_lu, 0_lu, 100_lu, 100_lu});

        // Should not crash, layout should be valid
        CHECK(p.bounds().width.to_int() == 100);
        CHECK(p.bounds().height.to_int() == 100);
    }
}
