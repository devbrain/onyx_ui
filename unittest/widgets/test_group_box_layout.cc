//
// test_group_box_layout.cc - Group box layout integration tests
//
// Tests verify that group box borders and titles correctly affect child positioning.
// Group boxes are like panels but always have a border and can have a title.
//

#include <doctest/doctest.h>
#include <onyxui/widgets/group_box.hh>
#include <onyxui/widgets/label.hh>
#include <onyxui/ui_context.hh>
#include "../utils/test_canvas_backend.hh"
#include "../utils/test_helpers.hh"
#include "../utils/layout_assertions.hh"

using namespace onyxui;
using namespace onyxui::testing;

TEST_SUITE("Group Box - Layout Integration") {
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

    TEST_CASE_FIXTURE(test_fixture, "Group box - child positioned inside border") {
        group_box<Backend> gb;
        apply_default_theme(gb);
        gb.set_vbox_layout(0);

        auto* child = gb.emplace_child<label>("Test");

        (void)gb.measure(100, 100);
        gb.arrange({0, 0, 100, 100});

        // Group box always has border, child should be inset by 1 pixel
        auto child_bounds = child->bounds();
        CHECK(rect_utils::get_x(child_bounds) == 1);
        CHECK(rect_utils::get_y(child_bounds) == 1);
        CHECK(rect_utils::get_width(child_bounds) == 98);  // 100 - 2*1

        // Visual verification
        auto canvas = render_to_canvas(gb, 20, 5);
        INFO("Group box rendering:\n", debug_canvas(*canvas));

        // Border should be drawn (group box always has border)
        assert_border_at_rect(*canvas, 0, 0, 20, 5, "Group box border");

        // Text should be inside border at (1, 1)
        assert_text_at(*canvas, "Test", 1, 1, "Text inside group box border");
    }

    TEST_CASE_FIXTURE(test_fixture, "Group box with title - layout not affected by title") {
        group_box<Backend> gb;
        apply_default_theme(gb);
        gb.set_title("Settings");
        gb.set_vbox_layout(0);

        auto* child = gb.emplace_child<label>("Option 1");

        (void)gb.measure(100, 100);
        gb.arrange({0, 0, 100, 100});

        // Title appears in border, doesn't affect content area
        auto child_bounds = child->bounds();
        CHECK(rect_utils::get_x(child_bounds) == 1);
        CHECK(rect_utils::get_y(child_bounds) == 1);

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
        group_box<Backend> gb;
        apply_default_theme(gb);
        gb.set_padding(thickness::all(3));
        gb.set_vbox_layout(0);

        auto* child = gb.emplace_child<label>("Test");

        (void)gb.measure(100, 100);
        gb.arrange({0, 0, 100, 100});

        // Child inset by border(1) + padding(3) = 4
        assert_child_inset(gb, *child, 4, 4, "Border + padding = 4px inset");

        // Visual verification
        auto canvas = render_to_canvas(gb, 25, 8);
        INFO("Group box with padding:\n", debug_canvas(*canvas));

        // Border at edges
        assert_border_at_rect(*canvas, 0, 0, 25, 8, "Group box border");

        // Text at border(1) + padding(3) = 4
        assert_text_at(*canvas, "Test", 4, 4, "Text inside border + padding");
    }

    TEST_CASE_FIXTURE(test_fixture, "Group box with asymmetric padding") {
        group_box<Backend> gb;
        apply_default_theme(gb);
        gb.set_padding({5, 3, 7, 9});  // L, T, R, B
        gb.set_vbox_layout(0);

        auto* child = gb.emplace_child<label>("Test");

        (void)gb.measure(100, 100);
        gb.arrange({0, 0, 100, 100});

        auto child_bounds = child->bounds();
        // Border(1) + left_padding(5) = 6
        // Border(1) + top_padding(3) = 4
        CHECK(rect_utils::get_x(child_bounds) == 6);
        CHECK(rect_utils::get_y(child_bounds) == 4);

        // Width: 100 - border(2) - left_pad(5) - right_pad(7) = 86
        CHECK(rect_utils::get_width(child_bounds) == 86);
        // Note: height is 1 (label's natural height)
    }

    TEST_CASE_FIXTURE(test_fixture, "Group box - multiple children with spacing") {
        group_box<Backend> gb;
        apply_default_theme(gb);
        gb.set_title("Options");
        gb.set_padding(thickness::all(2));
        gb.set_vbox_layout(1);  // 1px spacing

        auto* child1 = gb.emplace_child<label>("Option 1");
        auto* child2 = gb.emplace_child<label>("Option 2");
        auto* child3 = gb.emplace_child<label>("Option 3");

        (void)gb.measure(100, 100);
        gb.arrange({0, 0, 100, 100});

        // All children should start at x = border(1) + padding(2) = 3
        CHECK(rect_utils::get_x(child1->bounds()) == 3);
        CHECK(rect_utils::get_x(child2->bounds()) == 3);
        CHECK(rect_utils::get_x(child3->bounds()) == 3);

        // Y positions should account for spacing
        int y1 = rect_utils::get_y(child1->bounds());
        int y2 = rect_utils::get_y(child2->bounds());
        int y3 = rect_utils::get_y(child3->bounds());

        int h1 = rect_utils::get_height(child1->bounds());
        int h2 = rect_utils::get_height(child2->bounds());

        CHECK(y1 == 3);  // border + padding
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
        group_box<Backend> gb1;
        apply_default_theme(gb1);
        gb1.set_title("");  // Empty string
        gb1.set_vbox_layout(0);
        auto* child1 = gb1.emplace_child<label>("A");

        group_box<Backend> gb2;
        apply_default_theme(gb2);
        // No title set (default)
        gb2.set_vbox_layout(0);
        auto* child2 = gb2.emplace_child<label>("B");

        (void)gb1.measure(100, 100);
        gb1.arrange({0, 0, 100, 100});

        (void)gb2.measure(100, 100);
        gb2.arrange({0, 0, 100, 100});

        // Both should have same layout (empty title vs no title)
        CHECK(rect_utils::get_x(child1->bounds()) == rect_utils::get_x(child2->bounds()));
        CHECK(rect_utils::get_y(child1->bounds()) == rect_utils::get_y(child2->bounds()));
    }

    TEST_CASE_FIXTURE(test_fixture, "Group box - content area calculation") {
        group_box<Backend> gb;
        apply_default_theme(gb);
        gb.set_padding({5, 3, 7, 9});
        gb.set_vbox_layout(0);

        auto* child = gb.emplace_child<label>("Test");

        (void)gb.measure(200, 150);
        gb.arrange({0, 0, 200, 150});

        auto child_bounds = child->bounds();

        // Position: border(1) + padding
        CHECK(rect_utils::get_x(child_bounds) == 6);   // 1 + 5
        CHECK(rect_utils::get_y(child_bounds) == 4);   // 1 + 3

        // Size: total - border(2) - padding(left+right or top+bottom)
        CHECK(rect_utils::get_width(child_bounds) == 186);   // 200 - 2 - 5 - 7
        // Note: height is 1 (label's natural height)
    }

    TEST_CASE_FIXTURE(test_fixture, "Group box - border styles don't affect layout") {
        group_box<Backend> gb1, gb2;
        apply_default_theme(gb1);
        apply_default_theme(gb2);

        gb1.set_border(group_box_border::single);
        gb2.set_border(group_box_border::double_);

        gb1.set_vbox_layout(0);
        gb2.set_vbox_layout(0);

        auto* child1 = gb1.emplace_child<label>("A");
        auto* child2 = gb2.emplace_child<label>("B");

        (void)gb1.measure(100, 100);
        gb1.arrange({0, 0, 100, 100});

        (void)gb2.measure(100, 100);
        gb2.arrange({0, 0, 100, 100});

        // Different border styles should have same layout
        CHECK(rect_utils::get_x(child1->bounds()) == rect_utils::get_x(child2->bounds()));
        CHECK(rect_utils::get_y(child1->bounds()) == rect_utils::get_y(child2->bounds()));
        CHECK(rect_utils::get_width(child1->bounds()) == rect_utils::get_width(child2->bounds()));
    }

    TEST_CASE_FIXTURE(test_fixture, "Group box - long title doesn't affect child position") {
        group_box<Backend> gb;
        apply_default_theme(gb);
        gb.set_title("This is a very long title that might wrap or truncate");
        gb.set_vbox_layout(0);

        auto* child = gb.emplace_child<label>("Child");

        (void)gb.measure(100, 100);
        gb.arrange({0, 0, 100, 100});

        // Child position should be same regardless of title length
        CHECK(rect_utils::get_x(child->bounds()) == 1);
        CHECK(rect_utils::get_y(child->bounds()) == 1);
    }

    TEST_CASE_FIXTURE(test_fixture, "Group box - nested inside panel") {
        panel<Backend> outer;
        apply_default_theme(outer);
        outer.set_has_border(true);
        outer.set_padding(thickness::all(2));
        outer.set_vbox_layout(0);

        auto* gb = outer.emplace_child<group_box>();
        gb->set_title("Inner Group");
        gb->set_vbox_layout(0);

        auto* child = gb->emplace_child<label>("Nested");

        (void)outer.measure(100, 100);
        outer.arrange({0, 0, 100, 100});

        // Panel: border(1) + padding(2) = 3
        auto gb_bounds = gb->bounds();
        CHECK(rect_utils::get_x(gb_bounds) == 3);
        CHECK(rect_utils::get_y(gb_bounds) == 3);

        // Group box adds its own border(1)
        auto child_bounds = child->bounds();
        int child_x_rel = rect_utils::get_x(child_bounds) - rect_utils::get_x(gb_bounds);
        int child_y_rel = rect_utils::get_y(child_bounds) - rect_utils::get_y(gb_bounds);

        CHECK(child_x_rel == 1);  // Group box border
        CHECK(child_y_rel == 1);

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
