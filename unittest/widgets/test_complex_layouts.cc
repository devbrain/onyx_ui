//
// test_complex_layouts.cc - Complex layout scenario tests
//
// Tests verify real-world layout patterns:
// - Deeply nested containers
// - Mixed layout strategies
// - Complex spacing combinations
// - Multi-level border/padding interactions
//

#include <cstddef>
#include <doctest/doctest.h>
#include <onyxui/widgets/panel.hh>
#include <onyxui/widgets/group_box.hh>
#include <onyxui/widgets/label.hh>
#include <onyxui/widgets/vbox.hh>
#include <onyxui/widgets/hbox.hh>
#include <onyxui/ui_context.hh>
#include "../utils/test_canvas_backend.hh"
#include "../utils/test_helpers.hh"
#include "../utils/layout_assertions.hh"
#include "onyxui/element.hh"
#include "onyxui/concepts/rect_like.hh"

using namespace onyxui;
using namespace onyxui::testing;

TEST_SUITE("Layout - Complex Scenarios") {
    using Backend = test_canvas_backend;

    struct test_fixture {
        scoped_ui_context<Backend> ctx;

        test_fixture() {
            setup_theme();
        }

        void setup_theme() {
            // Ensure theme exists and is set as current
            auto* theme = ctx.themes().get_theme("Canvas Test Theme");
            if (!theme) {
                // Backend auto-registration didn't happen - register manually
                onyxui::ui_theme<Backend> test_theme;
                test_theme.name = "Canvas Test Theme";
                test_theme.panel.box_style.draw_border = true;
                test_theme.panel.box_style.corner = '+';
                test_theme.panel.box_style.horizontal = '-';
                test_theme.panel.box_style.vertical = '|';
                ctx.themes().register_theme(test_theme);
            }
            ctx.themes().set_current_theme("Canvas Test Theme");
        }

        template<typename Widget>
        void apply_default_theme(Widget& w) {
            // No longer needed - widgets automatically use current theme
            (void)w;
        }
    };

    TEST_CASE_FIXTURE(test_fixture, "Three-level nesting - panel -> group_box -> vbox") {
        // Ensure theme is set (workaround for fixture constructor timing issue)
        setup_theme();

        panel<Backend> outer;
        apply_default_theme(outer);
        outer.set_has_border(true);      // +1 per side
        outer.set_padding(thickness::all(2));  // +2 per side
        outer.set_vbox_layout(0);

        auto* gb = outer.emplace_child<group_box>();
        gb->set_title("Options");
        gb->set_padding(thickness::all(1));  // +1 per side (group_box has border already)

        auto* inner_vbox = gb->emplace_child<vbox>(1);  // 1px spacing
        inner_vbox->emplace_child<label>("Item 1");
        inner_vbox->emplace_child<label>("Item 2");

        (void)outer.measure(100, 100);
        outer.arrange({0, 0, 100, 100});

        // Panel: border(1) + padding(2) = 3 inset
        auto gb_bounds = gb->bounds();
        CHECK(rect_utils::get_x(gb_bounds) == 3);
        CHECK(rect_utils::get_y(gb_bounds) == 3);

        // Group box relative position should account for its border + padding
        auto vbox_bounds = inner_vbox->bounds();
        int const vbox_x_rel = rect_utils::get_x(vbox_bounds) - rect_utils::get_x(gb_bounds);
        int const vbox_y_rel = rect_utils::get_y(vbox_bounds) - rect_utils::get_y(gb_bounds);

        // Group box border(1) + padding(1) = 2
        CHECK(vbox_x_rel == 2);
        CHECK(vbox_y_rel == 2);

        // Visual verification - three levels of nesting
        auto canvas = render_to_canvas(outer, 35, 12);
        INFO("Three-level nesting:\n", debug_canvas(*canvas));

        // Outer panel border
        assert_border_at_rect(*canvas, 0, 0, 35, 12, "Outer panel border");

        // Items should be visible at correct positions
        // Panel border(1) + padding(2) + GB border(1) + GB padding(1) = 5
        assert_text_at(*canvas, "Item 1", 5, 5, "First item in nested layout");
        // Item 2 is at Item1_y(5) + Item1_height(1) + spacing(1) = 7
        assert_text_at(*canvas, "Item 2", 5, 7, "Second item with 1px spacing");
    }

    TEST_CASE_FIXTURE(test_fixture, "Side-by-side panels with borders - HBox layout") {
        hbox<Backend> container(5);  // 5px spacing between panels
        apply_default_theme(container);

        auto* panel1 = container.emplace_child<panel>();
        apply_default_theme(*panel1);
        panel1->set_has_border(true);
        panel1->set_vbox_layout(0);
        panel1->emplace_child<label>("Left");

        auto* panel2 = container.emplace_child<panel>();
        apply_default_theme(*panel2);
        panel2->set_has_border(true);
        panel2->set_vbox_layout(0);
        panel2->emplace_child<label>("Right");

        (void)container.measure(200, 100);
        container.arrange({0, 0, 200, 100});

        // Panels should be positioned side by side with spacing
        auto p1_bounds = panel1->bounds();
        auto p2_bounds = panel2->bounds();

        int const p1_right = rect_utils::get_x(p1_bounds) + rect_utils::get_width(p1_bounds);
        int const p2_left = rect_utils::get_x(p2_bounds);

        // Spacing between panels should be 5px
        CHECK(p2_left - p1_right == 5);

        // Visual verification - side-by-side panels
        auto canvas = render_to_canvas(container, 40, 5);
        INFO("Side-by-side panels:\n", debug_canvas(*canvas));

        // Both panels should have borders
        // Note: Can't easily assert both borders independently with current helpers
        // but we can verify the text positions
        assert_text_at(*canvas, "Left", 1, 1, "Left panel text");
        // Right panel: panel1_width(6) + spacing(5) + panel2_border_left(1) = 12
        assert_text_at(*canvas, "Right", 12, 1, "Right panel text");
    }

    TEST_CASE_FIXTURE(test_fixture, "Mixed layout - VBox with nested HBox") {
        vbox<Backend> outer(2);  // 2px vertical spacing
        apply_default_theme(outer);

        // Top section: simple label
        outer.emplace_child<label>("Header");

        // Middle section: horizontal layout
        auto* h = outer.emplace_child<hbox>(3);  // 3px horizontal spacing
        h->emplace_child<label>("Left");
        h->emplace_child<label>("Center");
        h->emplace_child<label>("Right");

        // Bottom section: another label
        outer.emplace_child<label>("Footer");

        (void)outer.measure(200, 100);
        outer.arrange({0, 0, 200, 100});

        // Verify vertical spacing
        const auto& children = outer.children();
        REQUIRE(children.size() == 3);

        auto header_bounds = children[0]->bounds();
        auto hbox_bounds = children[1]->bounds();

        int const header_bottom = rect_utils::get_y(header_bounds) + rect_utils::get_height(header_bounds);
        int const hbox_top = rect_utils::get_y(hbox_bounds);

        CHECK(hbox_top - header_bottom == 2);  // Vertical spacing

        // HBox should have been laid out
        CHECK(rect_utils::get_width(hbox_bounds) > 0);
        CHECK(rect_utils::get_height(hbox_bounds) > 0);
    }

    TEST_CASE_FIXTURE(test_fixture, "Asymmetric padding cascade - parent and child") {
        panel<Backend> outer;
        apply_default_theme(outer);
        outer.set_has_border(true);
        outer.set_padding({10, 5, 10, 5});  // L, T, R, B
        outer.set_vbox_layout(0);

        auto* inner = outer.emplace_child<panel>();
        apply_default_theme(*inner);
        inner->set_has_border(true);
        inner->set_padding({3, 7, 3, 7});  // Different asymmetry
        inner->set_vbox_layout(0);

        auto* text_label = inner->emplace_child<label>("Text");

        (void)outer.measure(200, 150);
        outer.arrange({0, 0, 200, 150});

        // Outer panel: border(1) + left_padding(10) = 11
        auto inner_bounds = inner->bounds();
        CHECK(rect_utils::get_x(inner_bounds) == 11);
        CHECK(rect_utils::get_y(inner_bounds) == 6);  // border(1) + top_padding(5)

        // Inner panel relative to its own bounds
        auto label_bounds = text_label->bounds();
        int const label_x_rel = rect_utils::get_x(label_bounds) - rect_utils::get_x(inner_bounds);
        int const label_y_rel = rect_utils::get_y(label_bounds) - rect_utils::get_y(inner_bounds);

        CHECK(label_x_rel == 4);  // inner_border(1) + inner_left_padding(3)
        CHECK(label_y_rel == 8);  // inner_border(1) + inner_top_padding(7)
    }

    TEST_CASE_FIXTURE(test_fixture, "Group boxes in grid-like layout using HBox/VBox") {
        vbox<Backend> main_container(5);
        apply_default_theme(main_container);

        // Top row
        auto* top_row = main_container.emplace_child<hbox>(5);
        auto* gb1 = top_row->emplace_child<group_box>();
        gb1->set_title("Box 1");
        gb1->set_vbox_layout(0);
        gb1->emplace_child<label>("Content 1");

        auto* gb2 = top_row->emplace_child<group_box>();
        gb2->set_title("Box 2");
        gb2->set_vbox_layout(0);
        gb2->emplace_child<label>("Content 2");

        // Bottom row
        auto* bottom_row = main_container.emplace_child<hbox>(5);
        auto* gb3 = bottom_row->emplace_child<group_box>();
        gb3->set_title("Box 3");
        gb3->set_vbox_layout(0);
        gb3->emplace_child<label>("Content 3");

        auto* gb4 = bottom_row->emplace_child<group_box>();
        gb4->set_title("Box 4");
        gb4->set_vbox_layout(0);
        gb4->emplace_child<label>("Content 4");

        (void)main_container.measure(300, 200);
        main_container.arrange({0, 0, 300, 200});

        // Verify all group boxes were laid out
        auto top_row_bounds = top_row->bounds();
        auto bottom_row_bounds = bottom_row->bounds();

        CHECK(rect_utils::get_y(top_row_bounds) == 0);
        CHECK(rect_utils::get_y(bottom_row_bounds) > rect_utils::get_y(top_row_bounds));

        // Verify spacing between rows
        int const top_row_bottom = rect_utils::get_y(top_row_bounds) + rect_utils::get_height(top_row_bounds);
        int const bottom_row_top = rect_utils::get_y(bottom_row_bounds);
        CHECK(bottom_row_top - top_row_bottom == 5);

        // Visual verification - 2x2 grid of group boxes
        // Need enough height: top_row(3) + spacing(5) + bottom_row(3) = 11 rows
        auto canvas = render_to_canvas(main_container, 60, 12);
        INFO("Grid-like layout with group boxes:\n", debug_canvas(*canvas));

        // All four content labels should be visible in their respective boxes
        // Top row: Content 1 at (1,1), Content 2 at (17,1) after box1(11) + spacing(5) + border(1)
        // Box1 width = content(9) + borders(2) = 11
        assert_text_at(*canvas, "Content 1", 1, 1, "Box 1 content");
        assert_text_at(*canvas, "Content 2", 17, 1, "Box 2 content");

        // Bottom row: top_row is 3 rows (box height), spacing is 5 rows, bottom starts at row 8
        // Content is at row 9 (border at row 8, content at row 9)
        // Box height = top_border(1) + content(1) + bottom_border(1) = 3
        assert_text_at(*canvas, "Content 3", 1, 9, "Box 3 content");
        assert_text_at(*canvas, "Content 4", 17, 9, "Box 4 content");
    }

    TEST_CASE_FIXTURE(test_fixture, "Deep nesting - 5 levels with mixed borders") {
        // Ensure theme is set (workaround for fixture constructor timing issue)
        setup_theme();

        panel<Backend> l1;
        apply_default_theme(l1);
        l1.set_has_border(true);
        l1.set_vbox_layout(0);

        auto* l2 = l1.emplace_child<panel>();
        apply_default_theme(*l2);
        l2->set_has_border(false);  // No border
        l2->set_padding(thickness::all(1));
        l2->set_vbox_layout(0);

        auto* l3 = l2->emplace_child<group_box>();
        l3->set_title("Level 3");
        l3->set_vbox_layout(0);

        auto* l4 = l3->emplace_child<panel>();
        apply_default_theme(*l4);
        l4->set_has_border(true);
        l4->set_vbox_layout(0);

        auto* l5_label = l4->emplace_child<label>("Deep");

        (void)l1.measure(200, 200);
        l1.arrange({0, 0, 200, 200});

        // Track cumulative offsets
        // L1: border(1) = 1
        // L2: padding(1) = 1 (no border)
        // L3: border(1) = 1 (group_box)
        // L4: border(1) = 1
        // Total: 4 pixels from L1 origin to L5 content

        auto l5_bounds = l5_label->bounds();

        // Should be at (4, 4) absolute
        CHECK(rect_utils::get_x(l5_bounds) >= 4);
        CHECK(rect_utils::get_y(l5_bounds) >= 4);

        // Visual verification - deep nesting with multiple borders
        auto canvas = render_to_canvas(l1, 25, 10);
        INFO("Deep nesting (5 levels):\n", debug_canvas(*canvas));

        // Outermost border (L1)
        assert_border_at_rect(*canvas, 0, 0, 25, 10, "Level 1 panel border");

        // Text should be deeply inset at position (4, 4)
        assert_text_at(*canvas, "Deep", 4, 4, "Deeply nested label");
    }

    TEST_CASE_FIXTURE(test_fixture, "Border toggle propagates through tree") {
        panel<Backend> outer;
        apply_default_theme(outer);
        outer.set_has_border(true);
        outer.set_vbox_layout(0);

        auto* inner = outer.emplace_child<panel>();
        apply_default_theme(*inner);
        inner->set_has_border(true);
        inner->set_vbox_layout(0);

        auto* text_label = inner->emplace_child<label>("Text");

        (void)outer.measure(100, 100);
        outer.arrange({0, 0, 100, 100});

        auto initial_label_bounds = text_label->bounds();
        int initial_x = rect_utils::get_x(initial_label_bounds);
        int initial_y = rect_utils::get_y(initial_label_bounds);

        // Remove inner border
        inner->set_has_border(false);
        (void)outer.measure(100, 100);
        outer.arrange({0, 0, 100, 100});

        auto new_label_bounds = text_label->bounds();
        int const new_x = rect_utils::get_x(new_label_bounds);
        int const new_y = rect_utils::get_y(new_label_bounds);

        // Label should move closer to origin (border removed = -1 per side)
        CHECK(new_x < initial_x);
        CHECK(new_y < initial_y);
    }

    TEST_CASE_FIXTURE(test_fixture, "VBox with many children - spacing accumulation") {
        vbox<Backend> container(2);  // 2px spacing
        apply_default_theme(container);

        // Add 10 labels
        for (int i = 0; i < 10; ++i) {
            container.emplace_child<label>("Item");
        }

        (void)container.measure(100, 500);
        container.arrange({0, 0, 100, 500});

        // Verify each child is spaced 2px from previous
        const auto& children = container.children();
        for (size_t i = 1; i < children.size(); ++i) {
            auto prev_bounds = children[i - 1]->bounds();
            auto curr_bounds = children[i]->bounds();

            int const prev_bottom = rect_utils::get_y(prev_bounds) + rect_utils::get_height(prev_bounds);
            int const curr_top = rect_utils::get_y(curr_bounds);

            CHECK(curr_top - prev_bottom == 2);
        }
    }

    TEST_CASE_FIXTURE(test_fixture, "Complex padding - all four sides different at each level") {
        panel<Backend> l1;
        apply_default_theme(l1);
        l1.set_padding({10, 20, 30, 40});  // L, T, R, B
        l1.set_vbox_layout(0);

        auto* l2 = l1.emplace_child<panel>();
        apply_default_theme(*l2);
        l2->set_padding({5, 15, 25, 35});  // Different on all sides
        l2->set_vbox_layout(0);

        auto* text_label = l2->emplace_child<label>("Text");

        (void)l1.measure(300, 300);
        l1.arrange({0, 0, 300, 300});

        // L1 positions L2 at (10, 20)
        auto l2_bounds = l2->bounds();
        CHECK(rect_utils::get_x(l2_bounds) == 10);
        CHECK(rect_utils::get_y(l2_bounds) == 20);

        // L2 positions label at relative (5, 15)
        auto label_bounds = text_label->bounds();
        int const label_x_rel = rect_utils::get_x(label_bounds) - rect_utils::get_x(l2_bounds);
        int const label_y_rel = rect_utils::get_y(label_bounds) - rect_utils::get_y(l2_bounds);

        CHECK(label_x_rel == 5);
        CHECK(label_y_rel == 15);
    }

    TEST_CASE_FIXTURE(test_fixture, "Mixed spacing - VBox spacing + child padding") {
        vbox<Backend> container(5);  // 5px between children
        apply_default_theme(container);
        container.set_padding(thickness::all(3));  // Container padding

        auto* p1 = container.emplace_child<panel>();
        apply_default_theme(*p1);
        p1->set_padding(thickness::all(2));
        p1->set_vbox_layout(0);
        p1->emplace_child<label>("First");

        auto* p2 = container.emplace_child<panel>();
        apply_default_theme(*p2);
        p2->set_padding(thickness::all(4));
        p2->set_vbox_layout(0);
        p2->emplace_child<label>("Second");

        (void)container.measure(200, 200);
        container.arrange({0, 0, 200, 200});

        // First panel at container_padding(3)
        auto p1_bounds = p1->bounds();
        CHECK(rect_utils::get_x(p1_bounds) == 3);
        CHECK(rect_utils::get_y(p1_bounds) == 3);

        // Second panel: container_padding(3) + p1_height + spacing(5)
        auto p2_bounds = p2->bounds();
        CHECK(rect_utils::get_x(p2_bounds) == 3);

        int expected_y2 = rect_utils::get_y(p1_bounds) +
                          rect_utils::get_height(p1_bounds) + 5;
        CHECK(rect_utils::get_y(p2_bounds) == expected_y2);
    }

    TEST_CASE_FIXTURE(test_fixture, "Group box with multiple nested VBoxes") {
        group_box<Backend> gb;
        apply_default_theme(gb);
        gb.set_title("Container");
        gb.set_padding(thickness::all(2));
        gb.set_vbox_layout(3);  // 3px spacing

        // Each child is a VBox with its own spacing
        auto* vb1 = gb.emplace_child<vbox>(1);
        vb1->emplace_child<label>("1A");
        vb1->emplace_child<label>("1B");

        auto* vb2 = gb.emplace_child<vbox>(2);
        vb2->emplace_child<label>("2A");
        vb2->emplace_child<label>("2B");

        (void)gb.measure(200, 200);
        gb.arrange({0, 0, 200, 200});

        // Group box border(1) + padding(2) = 3
        auto vb1_bounds = vb1->bounds();
        CHECK(rect_utils::get_x(vb1_bounds) == 3);
        CHECK(rect_utils::get_y(vb1_bounds) == 3);

        // vb2 should be vb1 + height + spacing(3)
        auto vb2_bounds = vb2->bounds();
        int expected_vb2_y = rect_utils::get_y(vb1_bounds) +
                             rect_utils::get_height(vb1_bounds) + 3;
        CHECK(rect_utils::get_y(vb2_bounds) == expected_vb2_y);
    }

    TEST_CASE_FIXTURE(test_fixture, "Empty containers in complex hierarchy") {
        vbox<Backend> outer(0);
        apply_default_theme(outer);

        // Empty panel
        auto* empty1 = outer.emplace_child<panel>();
        apply_default_theme(*empty1);
        empty1->set_vbox_layout(0);

        // Panel with label
        auto* filled = outer.emplace_child<panel>();
        apply_default_theme(*filled);
        filled->set_vbox_layout(0);
        filled->emplace_child<label>("Content");

        // Another empty panel
        auto* empty2 = outer.emplace_child<panel>();
        apply_default_theme(*empty2);
        empty2->set_vbox_layout(0);

        (void)outer.measure(100, 100);
        outer.arrange({0, 0, 100, 100});

        // Should not crash, all panels positioned
        CHECK(rect_utils::get_y(empty1->bounds()) >= 0);
        CHECK(rect_utils::get_y(filled->bounds()) >= 0);
        CHECK(rect_utils::get_y(empty2->bounds()) >= 0);
    }
}
