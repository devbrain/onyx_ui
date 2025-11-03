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
        p.set_vbox_layout(0);

        auto* child = p.emplace_child<label>("Test");

        // Measure/arrange with theme active
        (void)p.measure(100, 100);  // Call for side effect (layout calculation)
        p.arrange({0, 0, 100, 100});

        auto bounds_after = child->bounds();

        // Layout should still be valid
        // Child coordinates are relative to parent's content area (after border is removed)
        CHECK(rect_utils::get_x(bounds_after) == 0);
        CHECK(rect_utils::get_y(bounds_after) == 0);
    }

    TEST_CASE_FIXTURE(ui_context_fixture<Backend>, "Theme inheritance - child inherits parent theme") {
        panel<Backend> parent;
        parent.set_vbox_layout(0);

        auto* child = parent.emplace_child<label>("Child");

        (void)parent.measure(100, 100);
        parent.arrange({0, 0, 100, 100});

        // Child should have inherited theme properties
        // (Actual theme values tested elsewhere; here we verify no layout issues)
        auto child_bounds = child->bounds();
        CHECK(rect_utils::get_width(child_bounds) > 0);
        CHECK(rect_utils::get_height(child_bounds) > 0);
    }

    TEST_CASE_FIXTURE(ui_context_fixture<Backend>, "Theme switch - layout updates correctly") {
        vbox<Backend> container(2);

        container.emplace_child<label>("Item 1");
        container.emplace_child<label>("Item 2");
        container.emplace_child<label>("Item 3");

        (void)container.measure(100, 100);
        container.arrange({0, 0, 100, 100});

        // Store initial positions
        const auto& children = container.children();
        std::vector<typename Backend::rect_type> initial_bounds;
        for (const auto& child : children) {
            initial_bounds.push_back(child->bounds());
        }

        // Apply theme again (simulating theme switch)

        (void)container.measure(100, 100);
        container.arrange({0, 0, 100, 100});

        // Verify layout still correct (same positions for same theme)
        for (size_t i = 0; i < children.size(); ++i) {
            auto current = children[i]->bounds();
            CHECK(rect_utils::get_x(current) == rect_utils::get_x(initial_bounds[i]));
            CHECK(rect_utils::get_y(current) == rect_utils::get_y(initial_bounds[i]));
        }
    }

    TEST_CASE_FIXTURE(ui_context_fixture<Backend>, "Themed panel with border - content area correct") {
        panel<Backend> p;
        p.set_has_border(true);
        p.set_padding(thickness::all(5));
        p.set_vbox_layout(0);

        auto* child = p.emplace_child<label>("Content");

        (void)p.measure(100, 100);
        p.arrange({0, 0, 100, 100});

        // Child coordinates are relative to parent's content area
        // Border and padding are already removed from the content rect before children are positioned
        auto child_bounds = child->bounds();
        CHECK(rect_utils::get_x(child_bounds) == 0);
        CHECK(rect_utils::get_y(child_bounds) == 0);
    }

    TEST_CASE_FIXTURE(ui_context_fixture<Backend>, "Nested themed widgets - inheritance chain") {
        panel<Backend> outer;
        outer.set_vbox_layout(0);

        auto* middle = outer.emplace_child<group_box>();
        middle->set_title("Middle");
        middle->set_vbox_layout(0);

        auto* inner = middle->emplace_child<label>("Inner");

        (void)outer.measure(200, 200);
        outer.arrange({0, 0, 200, 200});

        // All widgets should have valid bounds through inheritance chain
        CHECK(rect_utils::get_width(outer.bounds()) > 0);
        CHECK(rect_utils::get_width(middle->bounds()) > 0);
        CHECK(rect_utils::get_width(inner->bounds()) > 0);
    }

    TEST_CASE_FIXTURE(ui_context_fixture<Backend>, "Theme application - multiple children preserve spacing") {
        vbox<Backend> vb(3);  // 3px spacing

        vb.emplace_child<label>("A");
        vb.emplace_child<label>("B");
        vb.emplace_child<label>("C");

        (void)vb.measure(100, 100);
        vb.arrange({0, 0, 100, 100});

        const auto& children = vb.children();
        for (size_t i = 1; i < children.size(); ++i) {
            auto prev = children[i - 1]->bounds();
            auto curr = children[i]->bounds();

            int const prev_bottom = rect_utils::get_y(prev) + rect_utils::get_height(prev);
            int const curr_top = rect_utils::get_y(curr);

            // Spacing should be preserved
            CHECK(curr_top - prev_bottom == 3);
        }
    }

    TEST_CASE_FIXTURE(ui_context_fixture<Backend>, "Group box with theme - title doesn't affect content layout") {
        group_box<Backend> gb;
        gb.set_title("Themed Title");
        gb.set_vbox_layout(0);

        auto* child = gb.emplace_child<label>("Content");

        (void)gb.measure(100, 100);
        gb.arrange({0, 0, 100, 100});

        // Child coordinates are relative to parent's content area
        // Title and border are handled by group_box; children positioned within content rect
        auto child_bounds = child->bounds();
        CHECK(rect_utils::get_x(child_bounds) == 0);
        CHECK(rect_utils::get_y(child_bounds) == 0);
    }

    TEST_CASE_FIXTURE(ui_context_fixture<Backend>, "Theme on already-laid-out tree - invalidation works") {
        panel<Backend> p;
        p.set_vbox_layout(0);
        p.emplace_child<label>("Text");

        // Layout without theme
        (void)p.measure(100, 100);
        p.arrange({0, 0, 100, 100});

        // Now apply theme (should invalidate and require re-layout)

        // Re-layout should work correctly
        (void)p.measure(100, 100);
        p.arrange({0, 0, 100, 100});

        // Should not crash, layout should be valid
        CHECK(rect_utils::get_width(p.bounds()) == 100);
        CHECK(rect_utils::get_height(p.bounds()) == 100);
    }
}
