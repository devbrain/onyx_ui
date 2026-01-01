//
// test_group_box_layout.cc - Group box layout integration tests
//
// Tests verify that group box borders and titles correctly affect child positioning.
// Group boxes are like panels but always have a border and can have a title.
//

#include <doctest/doctest.h>
#include <../../include/onyxui/widgets/containers/group_box.hh>
#include <onyxui/widgets/label.hh>
#include <../../include/onyxui/services/ui_context.hh>
#include "../utils/test_canvas_backend.hh"
#include "../utils/test_helpers.hh"
#include "../utils/layout_assertions.hh"
#include "onyxui/concepts/rect_like.hh"
#include "../../include/onyxui/core/element.hh"
#include "../../include/onyxui/widgets/containers/panel.hh"

using namespace onyxui;
using namespace onyxui::testing;

TEST_SUITE("Group Box - Layout Integration") {
    using Backend = test_canvas_backend;

    struct test_fixture {
        scoped_ui_context<Backend> ctx{make_terminal_metrics<Backend>()};

        template<typename Widget>
        void apply_default_theme([[maybe_unused]] Widget& w) {
            if (auto* theme = ctx.themes().get_theme("Canvas Test Theme")) {
//                 w.apply_theme(*theme);  // No longer needed - widgets use global theme
            }
        }
    };

    TEST_CASE_FIXTURE(test_fixture, "Group box - child positioned inside border") {
        // Set Canvas Test Theme as current (needed for render_to_canvas)
        ctx.themes().set_current_theme("Canvas Test Theme");

        group_box<Backend> gb;
        apply_default_theme(gb);
        gb.set_vbox_layout(spacing::none);

        auto* child = gb.emplace_child<label>("Test");

        (void)gb.measure(100_lu, 100_lu);
        gb.arrange(logical_rect{0_lu, 0_lu, 100_lu, 100_lu});

        // Group box always has border, child should be at (0,0) relative to parent's content area
        // (border offset is handled internally by group_box)
        auto child_bounds = child->bounds();
        CHECK(child_bounds.x.to_int() == 0);
        CHECK(child_bounds.y.to_int() == 0);
        CHECK(child_bounds.width.to_int() == 98);  // 100 - 2*1

        // Visual verification
        auto canvas = render_to_canvas(gb, 20, 5);
        INFO("Group box rendering:\n", debug_canvas(*canvas));

        // Border should be drawn (group box always has border)
        assert_border_at_rect(*canvas, 0, 0, 20, 5, "Group box border");

        // Text should be inside border at (1, 1)
        assert_text_at(*canvas, "Test", 1, 1, "Text inside group box border");
    }

    TEST_CASE_FIXTURE(test_fixture, "Group box with title - layout not affected by title") {
        // Set Canvas Test Theme as current (needed for render_to_canvas)
        ctx.themes().set_current_theme("Canvas Test Theme");

        group_box<Backend> gb;
        apply_default_theme(gb);
        gb.set_title("Settings");
        gb.set_vbox_layout(spacing::none);

        auto* child = gb.emplace_child<label>("Option 1");

        (void)gb.measure(100_lu, 100_lu);
        gb.arrange(logical_rect{0_lu, 0_lu, 100_lu, 100_lu});

        // Title appears in border, doesn't affect content area
        // Child is at (0,0) relative to parent's content area (border offset handled internally)
        auto child_bounds = child->bounds();
        CHECK(child_bounds.x.to_int() == 0);
        CHECK(child_bounds.y.to_int() == 0);

        // Visual verification - border should still be drawn
        auto canvas = render_to_canvas(gb, 30, 6);
        INFO("Group box with title:\n", debug_canvas(*canvas));

        // Border should be drawn
        assert_border_at_rect(*canvas, 0, 0, 30, 6, "Group box border with title");

        // Child text at correct position
        assert_text_at(*canvas, "Option 1", 1, 1, "Child inside titled group box");

        // TODO: When title rendering is implemented, verify title at top border
    }

    TEST_CASE_FIXTURE(test_fixture, "Group box with padding - child positioned inside border and padding") {
        // Set Canvas Test Theme as current (needed for render_to_canvas)
        ctx.themes().set_current_theme("Canvas Test Theme");

        group_box<Backend> gb;
        apply_default_theme(gb);
        gb.set_padding(logical_thickness(3_lu));
        gb.set_vbox_layout(spacing::none);

        auto* child = gb.emplace_child<label>("Test");

        (void)gb.measure(100_lu, 100_lu);
        gb.arrange(logical_rect{0_lu, 0_lu, 100_lu, 100_lu});

        // Child is at (0,0) relative to parent's content area
        // (border and padding offsets are handled internally by group_box)
        auto child_bounds = child->bounds();
        CHECK(child_bounds.x.to_int() == 0);
        CHECK(child_bounds.y.to_int() == 0);

        // Visual verification
        // Canvas needs to be tall enough for border(2px) + padding(6px) + content
        auto canvas = render_to_canvas(gb, 25, 12);
        INFO("Group box with padding:\n", debug_canvas(*canvas));

        // Border at edges
        assert_border_at_rect(*canvas, 0, 0, 25, 12, "Group box border");

        // Text at border(1) + padding(3) = 4
        assert_text_at(*canvas, "Test", 4, 4, "Text inside border + padding");
    }

    TEST_CASE_FIXTURE(test_fixture, "Group box with asymmetric padding") {
        // Set Canvas Test Theme as current
        ctx.themes().set_current_theme("Canvas Test Theme");

        group_box<Backend> gb;
        apply_default_theme(gb);
        gb.set_padding(logical_thickness{5_lu, 3_lu, 7_lu, 9_lu});  // L, T, R, B
        gb.set_vbox_layout(spacing::none);

        auto* child = gb.emplace_child<label>("Test");

        (void)gb.measure(100_lu, 100_lu);
        gb.arrange(logical_rect{0_lu, 0_lu, 100_lu, 100_lu});

        auto child_bounds = child->bounds();
        // Child is at (0,0) relative to parent's content area
        // (border and padding offsets are handled internally by group_box)
        CHECK(child_bounds.x.to_int() == 0);
        CHECK(child_bounds.y.to_int() == 0);

        // Width: 100 - border(2) - left_pad(5) - right_pad(7) = 86
        CHECK(child_bounds.width.to_int() == 86);
        // Note: height is 1 (label's natural height)
    }

    TEST_CASE_FIXTURE(test_fixture, "Group box - multiple children with spacing") {
        // Set Canvas Test Theme as current (needed for render_to_canvas)
        ctx.themes().set_current_theme("Canvas Test Theme");

        group_box<Backend> gb;
        apply_default_theme(gb);
        gb.set_title("Options");
        gb.set_padding(logical_thickness(2_lu));
        gb.set_vbox_layout(spacing::small);  // 1px spacing (small resolves to 1 in test theme)

        auto* child1 = gb.emplace_child<label>("Option 1");
        auto* child2 = gb.emplace_child<label>("Option 2");
        auto* child3 = gb.emplace_child<label>("Option 3");

        (void)gb.measure(100_lu, 100_lu);
        gb.arrange(logical_rect{0_lu, 0_lu, 100_lu, 100_lu});

        // All children should start at x = 0 (relative to parent's content area)
        // (border and padding offsets are handled internally by group_box)
        CHECK(child1->bounds().x.to_int() == 0);
        CHECK(child2->bounds().x.to_int() == 0);
        CHECK(child3->bounds().x.to_int() == 0);

        // Y positions should account for spacing
        int const y1 = child1->bounds().y.to_int();
        int const y2 = child2->bounds().y.to_int();
        int const y3 = child3->bounds().y.to_int();

        int const h1 = child1->bounds().height.to_int();
        int const h2 = child2->bounds().height.to_int();

        CHECK(y1 == 0);  // First child at (0,0) relative to content area
        CHECK(y2 == y1 + h1 + 1);  // child1 + spacing
        CHECK(y3 == y2 + h2 + 1);  // child2 + spacing

        // Visual verification
        // Need 11 rows: border(1) + padding(2) + label(1) + spacing(1) + label(1) + spacing(1) + label(1) + padding(2) + border(1)

        auto canvas = render_to_canvas(gb, 35, 12);
        INFO("Group box with multiple children:\n", debug_canvas(*canvas));

        // Border should be drawn
        assert_border_at_rect(*canvas, 0, 0, 35, 12, "Group box border");

        // Verify all three children are visible
        // Positions: border(1) + padding(2) = y starts at 3
        assert_text_at(*canvas, "Option 1", 3, 3, "First child");
        assert_text_at(*canvas, "Option 2", 3, 5, "Second child (y1+h1+spacing = 3+1+1)");
        assert_text_at(*canvas, "Option 3", 3, 7, "Third child (y2+h2+spacing = 5+1+1)");
    }

    TEST_CASE_FIXTURE(test_fixture, "Group box - empty title vs no title") {
        // Set Canvas Test Theme as current
        ctx.themes().set_current_theme("Canvas Test Theme");

        group_box<Backend> gb1;
        apply_default_theme(gb1);
        gb1.set_title("");  // Empty string
        gb1.set_vbox_layout(spacing::none);
        auto* child1 = gb1.emplace_child<label>("A");

        group_box<Backend> gb2;
        apply_default_theme(gb2);
        // No title set (default)
        gb2.set_vbox_layout(spacing::none);
        auto* child2 = gb2.emplace_child<label>("B");

        (void)gb1.measure(100_lu, 100_lu);
        gb1.arrange(logical_rect{0_lu, 0_lu, 100_lu, 100_lu});

        (void)gb2.measure(100_lu, 100_lu);
        gb2.arrange(logical_rect{0_lu, 0_lu, 100_lu, 100_lu});

        // Both should have same layout (empty title vs no title)
        CHECK(child1->bounds().x.to_int() == child2->bounds().x.to_int());
        CHECK(child1->bounds().y.to_int() == child2->bounds().y.to_int());
    }

    TEST_CASE_FIXTURE(test_fixture, "Group box - content area calculation") {
        // Set Canvas Test Theme as current
        ctx.themes().set_current_theme("Canvas Test Theme");

        group_box<Backend> gb;
        apply_default_theme(gb);
        gb.set_padding(logical_thickness{5_lu, 3_lu, 7_lu, 9_lu});
        gb.set_vbox_layout(spacing::none);

        auto* child = gb.emplace_child<label>("Test");

        (void)gb.measure(200_lu, 150_lu);
        gb.arrange(logical_rect{0_lu, 0_lu, 200_lu, 150_lu});

        auto child_bounds = child->bounds();

        // Child is at (0,0) relative to parent's content area
        // (border and padding offsets are handled internally by group_box)
        CHECK(child_bounds.x.to_int() == 0);
        CHECK(child_bounds.y.to_int() == 0);

        // Size: total - border(2) - padding(left+right or top+bottom)
        CHECK(child_bounds.width.to_int() == 186);   // 200 - 2 - 5 - 7
        // Note: height is 1 (label's natural height)
    }

    TEST_CASE_FIXTURE(test_fixture, "Group box - border styles don't affect layout") {
        // Set Canvas Test Theme as current
        ctx.themes().set_current_theme("Canvas Test Theme");

        group_box<Backend> gb1, gb2;
        apply_default_theme(gb1);
        apply_default_theme(gb2);

        gb1.set_border(group_box_border::single);
        gb2.set_border(group_box_border::double_);

        gb1.set_vbox_layout(spacing::none);
        gb2.set_vbox_layout(spacing::none);

        auto* child1 = gb1.emplace_child<label>("A");
        auto* child2 = gb2.emplace_child<label>("B");

        (void)gb1.measure(100_lu, 100_lu);
        gb1.arrange(logical_rect{0_lu, 0_lu, 100_lu, 100_lu});

        (void)gb2.measure(100_lu, 100_lu);
        gb2.arrange(logical_rect{0_lu, 0_lu, 100_lu, 100_lu});

        // Different border styles should have same layout
        CHECK(child1->bounds().x.to_int() == child2->bounds().x.to_int());
        CHECK(child1->bounds().y.to_int() == child2->bounds().y.to_int());
        CHECK(child1->bounds().width.to_int() == child2->bounds().width.to_int());
    }

    TEST_CASE_FIXTURE(test_fixture, "Group box - long title doesn't affect child position") {
        // Set Canvas Test Theme as current
        ctx.themes().set_current_theme("Canvas Test Theme");

        group_box<Backend> gb;
        apply_default_theme(gb);
        gb.set_title("This is a very long title that might wrap or truncate");
        gb.set_vbox_layout(spacing::none);

        auto* child = gb.emplace_child<label>("Child");

        (void)gb.measure(100_lu, 100_lu);
        gb.arrange(logical_rect{0_lu, 0_lu, 100_lu, 100_lu});

        // Child position should be same regardless of title length
        // Child is at (0,0) relative to parent's content area (border offset handled internally)
        CHECK(child->bounds().x.to_int() == 0);
        CHECK(child->bounds().y.to_int() == 0);
    }

    TEST_CASE_FIXTURE(test_fixture, "Group box - nested inside panel") {
        // Set Canvas Test Theme as current (needed for render_to_canvas)
        ctx.themes().set_current_theme("Canvas Test Theme");

        panel<Backend> outer;
        apply_default_theme(outer);
        outer.set_has_border(true);
        outer.set_padding(logical_thickness(2_lu));
        outer.set_vbox_layout(spacing::none);

        auto* gb = outer.emplace_child<group_box>();
        gb->set_title("Inner Group");
        gb->set_vbox_layout(spacing::none);

        auto* child = gb->emplace_child<label>("Nested");

        (void)outer.measure(100_lu, 100_lu);
        outer.arrange(logical_rect{0_lu, 0_lu, 100_lu, 100_lu});

        // Group box is at (0,0) relative to panel's content area
        // (panel's border and padding offsets are handled internally by panel)
        auto gb_bounds = gb->bounds();
        CHECK(gb_bounds.x.to_int() == 0);
        CHECK(gb_bounds.y.to_int() == 0);

        // Child is at (0,0) relative to group box's content area
        // (group box's border offset is handled internally by group_box)
        auto child_bounds = child->bounds();
        int const child_x_rel = child_bounds.x.to_int() - gb_bounds.x.to_int();
        int const child_y_rel = child_bounds.y.to_int() - gb_bounds.y.to_int();

        CHECK(child_x_rel == 0);  // Relative to group box's content area
        CHECK(child_y_rel == 0);

        // Visual verification - nested borders
        auto canvas = render_to_canvas(outer, 30, 10);
        INFO("Nested group box inside panel:\n", debug_canvas(*canvas));

        // Outer panel border
        assert_border_at_rect(*canvas, 0, 0, 30, 10, "Outer panel border");

        // Inner group box border (at position 3,3 with appropriate size)
        // Note: We can't easily assert inner border with current helpers
        // but we can verify the text position
        assert_text_at(*canvas, "Nested", 4, 4, "Text inside nested group box");
    }
}
