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
#include <../../include/onyxui/widgets/containers/panel.hh>
#include <../../include/onyxui/widgets/containers/group_box.hh>
#include <onyxui/widgets/label.hh>
#include <../../include/onyxui/widgets/containers/vbox.hh>
#include <../../include/onyxui/widgets/containers/hbox.hh>
#include <../../include/onyxui/services/ui_context.hh>
#include "../utils/test_canvas_backend.hh"
#include "../utils/test_helpers.hh"
#include "../utils/layout_assertions.hh"
#include "../../include/onyxui/core/element.hh"
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
                // Add spacing values (TUI-style)
                test_theme.spacing.none = 0;
                test_theme.spacing.tiny = 0;
                test_theme.spacing.small = 1;
                test_theme.spacing.medium = 1;
                test_theme.spacing.large = 2;
                test_theme.spacing.xlarge = 3;
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
        outer.set_padding(logical_thickness(2_lu));  // +2 per side
        outer.set_vbox_layout(spacing::none);

        auto* gb = outer.emplace_child<group_box>();
        gb->set_title("Options");
        gb->set_padding(logical_thickness(1_lu));  // +1 per side (group_box has border already)

        auto* inner_vbox = gb->emplace_child<vbox>(spacing::tiny);  // tiny spacing
        inner_vbox->emplace_child<label>("Item 1");
        inner_vbox->emplace_child<label>("Item 2");

        (void)outer.measure(100_lu, 100_lu);
        outer.arrange(logical_rect{0_lu, 0_lu, 100_lu, 100_lu});

        // RELATIVE COORDINATES: child at (0,0) relative to parent's content area
        // Panel: border(1) + padding(2) = 3 inset (accounted for in parent arrange)
        auto gb_bounds = gb->bounds();
        CHECK(gb_bounds.x.to_int() == 0);
        CHECK(gb_bounds.y.to_int() == 0);

        // RELATIVE COORDINATES: child at (0,0) relative to parent's content area
        // Group box relative position should account for its border + padding
        auto vbox_bounds = inner_vbox->bounds();
        int const vbox_x_rel = vbox_bounds.x.to_int() - gb_bounds.x.to_int();
        int const vbox_y_rel = vbox_bounds.y.to_int() - gb_bounds.y.to_int();

        // Group box border(1) + padding(1) = 2 (accounted for in parent arrange)
        CHECK(vbox_x_rel == 0);
        CHECK(vbox_y_rel == 0);

        // Visual verification - three levels of nesting
        // Canvas needs: outer border(2) + outer padding(4) + gb border(2) + gb padding(2) + content(3) = 13px
        auto canvas = render_to_canvas(outer, 35, 13);
        INFO("Three-level nesting:\n", debug_canvas(*canvas));

        // Outer panel border
        assert_border_at_rect(*canvas, 0, 0, 35, 13, "Outer panel border");

        // Items should be visible at correct positions
        // Panel border(1) + padding(2) + GB border(1) + GB padding(1) = 5
        assert_text_at(*canvas, "Item 1", 5, 5, "First item in nested layout");
        // Item 2 is at Item1_y(5) + Item1_height(1) + spacing(0) = 6 (tiny spacing = 0)
        assert_text_at(*canvas, "Item 2", 5, 6, "Second item with 0px spacing");
    }

    TEST_CASE_FIXTURE(test_fixture, "Side-by-side panels with borders - HBox layout") {
        hbox<Backend> container(spacing::small);  // small spacing between panels
        apply_default_theme(container);

        auto* panel1 = container.emplace_child<panel>();
        apply_default_theme(*panel1);
        panel1->set_has_border(true);
        panel1->set_vbox_layout(spacing::none);
        panel1->emplace_child<label>("Left");

        auto* panel2 = container.emplace_child<panel>();
        apply_default_theme(*panel2);
        panel2->set_has_border(true);
        panel2->set_vbox_layout(spacing::none);
        panel2->emplace_child<label>("Right");

        (void)container.measure(200_lu, 100_lu);
        container.arrange(logical_rect{0_lu, 0_lu, 200_lu, 100_lu});

        // Panels should be positioned side by side with spacing
        auto p1_bounds = panel1->bounds();
        auto p2_bounds = panel2->bounds();

        int const p1_right = p1_bounds.x.to_int() + p1_bounds.width.to_int();
        int const p2_left = p2_bounds.x.to_int();

        // Spacing between panels should be small (1px in test theme)
        CHECK(p2_left - p1_right == 1);

        // Visual verification - side-by-side panels
        auto canvas = render_to_canvas(container, 40, 5);
        INFO("Side-by-side panels:\n", debug_canvas(*canvas));

        // Both panels should have borders
        // Note: Can't easily assert both borders independently with current helpers
        // but we can verify the text positions
        assert_text_at(*canvas, "Left", 1, 1, "Left panel text");
        // Right panel: panel1_width(6) + spacing(1) + panel2_border_left(1) = 8 (small spacing = 1)
        assert_text_at(*canvas, "Right", 8, 1, "Right panel text");
    }

    TEST_CASE_FIXTURE(test_fixture, "Mixed layout - VBox with nested HBox") {
        vbox<Backend> outer(spacing::tiny);  // tiny vertical spacing
        apply_default_theme(outer);

        // Top section: simple label
        outer.emplace_child<label>("Header");

        // Middle section: horizontal layout
        auto* h = outer.emplace_child<hbox>(spacing::small);  // small horizontal spacing
        h->emplace_child<label>("Left");
        h->emplace_child<label>("Center");
        h->emplace_child<label>("Right");

        // Bottom section: another label
        outer.emplace_child<label>("Footer");

        (void)outer.measure(200_lu, 100_lu);
        outer.arrange(logical_rect{0_lu, 0_lu, 200_lu, 100_lu});

        // Verify vertical spacing
        const auto& children = outer.children();
        REQUIRE(children.size() == 3);

        auto header_bounds = children[0]->bounds();
        auto hbox_bounds = children[1]->bounds();

        int const header_bottom = header_bounds.y.to_int() + header_bounds.height.to_int();
        int const hbox_top = hbox_bounds.y.to_int();

        CHECK(hbox_top - header_bottom == 0);  // Vertical spacing (tiny = 0px in test theme)

        // HBox should have been laid out
        CHECK(hbox_bounds.width.to_int() > 0);
        CHECK(hbox_bounds.height.to_int() > 0);
    }

    TEST_CASE_FIXTURE(test_fixture, "Asymmetric padding cascade - parent and child") {
        panel<Backend> outer;
        apply_default_theme(outer);
        outer.set_has_border(true);
        outer.set_padding(logical_thickness{10_lu, 5_lu, 10_lu, 5_lu});  // L, T, R, B
        outer.set_vbox_layout(spacing::none);

        auto* inner = outer.emplace_child<panel>();
        apply_default_theme(*inner);
        inner->set_has_border(true);
        inner->set_padding(logical_thickness{3_lu, 7_lu, 3_lu, 7_lu});  // Different asymmetry
        inner->set_vbox_layout(spacing::none);

        auto* text_label = inner->emplace_child<label>("Text");

        (void)outer.measure(200_lu, 150_lu);
        outer.arrange(logical_rect{0_lu, 0_lu, 200_lu, 150_lu});

        // RELATIVE COORDINATES: child at (0,0) relative to parent's content area
        // Outer panel: border(1) + left_padding(10) = 11 (accounted for in parent arrange)
        auto inner_bounds = inner->bounds();
        CHECK(inner_bounds.x.to_int() == 0);
        CHECK(inner_bounds.y.to_int() == 0);  // border(1) + top_padding(5) accounted for

        // RELATIVE COORDINATES: child at (0,0) relative to parent's content area
        // Inner panel relative to its own bounds
        auto label_bounds = text_label->bounds();
        int const label_x_rel = label_bounds.x.to_int() - inner_bounds.x.to_int();
        int const label_y_rel = label_bounds.y.to_int() - inner_bounds.y.to_int();

        CHECK(label_x_rel == 0);  // inner_border(1) + inner_left_padding(3) accounted for
        CHECK(label_y_rel == 0);  // inner_border(1) + inner_top_padding(7) accounted for
    }

    TEST_CASE_FIXTURE(test_fixture, "Group boxes in grid-like layout using HBox/VBox") {
        vbox<Backend> main_container(spacing::small);
        apply_default_theme(main_container);

        // Top row
        auto* top_row = main_container.emplace_child<hbox>(spacing::small);
        auto* gb1 = top_row->emplace_child<group_box>();
        gb1->set_title("Box 1");
        gb1->set_vbox_layout(spacing::none);
        gb1->emplace_child<label>("Content 1");

        auto* gb2 = top_row->emplace_child<group_box>();
        gb2->set_title("Box 2");
        gb2->set_vbox_layout(spacing::none);
        gb2->emplace_child<label>("Content 2");

        // Bottom row
        auto* bottom_row = main_container.emplace_child<hbox>(spacing::small);
        auto* gb3 = bottom_row->emplace_child<group_box>();
        gb3->set_title("Box 3");
        gb3->set_vbox_layout(spacing::none);
        gb3->emplace_child<label>("Content 3");

        auto* gb4 = bottom_row->emplace_child<group_box>();
        gb4->set_title("Box 4");
        gb4->set_vbox_layout(spacing::none);
        gb4->emplace_child<label>("Content 4");

        (void)main_container.measure(300_lu, 200_lu);
        main_container.arrange(logical_rect{0_lu, 0_lu, 300_lu, 200_lu});

        // Verify all group boxes were laid out
        auto top_row_bounds = top_row->bounds();
        auto bottom_row_bounds = bottom_row->bounds();

        CHECK(top_row_bounds.y.to_int() == 0);
        CHECK(bottom_row_bounds.y.to_int() > top_row_bounds.y.to_int());

        // Verify spacing between rows
        int const top_row_bottom = top_row_bounds.y.to_int() + top_row_bounds.height.to_int();
        int const bottom_row_top = bottom_row_bounds.y.to_int();
        CHECK(bottom_row_top - top_row_bottom == 1);  // small spacing (1px in test theme)

        // Visual verification - 2x2 grid of group boxes
        // Need enough height: top_row(3) + spacing(1) + bottom_row(3) = 7 rows
        auto canvas = render_to_canvas(main_container, 60, 10);
        INFO("Grid-like layout with group boxes:\n", debug_canvas(*canvas));

        // All four content labels should be visible in their respective boxes
        // Top row: Content 1 at (1,1), Content 2 at (13,1) after box1(11) + spacing(1) + border(1)
        // Box1 width = content(9) + borders(2) = 11
        assert_text_at(*canvas, "Content 1", 1, 1, "Box 1 content");
        assert_text_at(*canvas, "Content 2", 13, 1, "Box 2 content");

        // Bottom row: top_row is 3 rows (box height), spacing is 1 row, bottom starts at row 4
        // Content is at row 5 (border at row 4, content at row 5)
        // Box height = top_border(1) + content(1) + bottom_border(1) = 3
        assert_text_at(*canvas, "Content 3", 1, 5, "Box 3 content");
        assert_text_at(*canvas, "Content 4", 13, 5, "Box 4 content");
    }

    TEST_CASE_FIXTURE(test_fixture, "Deep nesting - 5 levels with mixed borders") {
        // Ensure theme is set (workaround for fixture constructor timing issue)
        setup_theme();

        panel<Backend> l1;
        apply_default_theme(l1);
        l1.set_has_border(true);
        l1.set_vbox_layout(spacing::none);

        auto* l2 = l1.emplace_child<panel>();
        apply_default_theme(*l2);
        l2->set_has_border(false);  // No border
        l2->set_padding(logical_thickness(1_lu));
        l2->set_vbox_layout(spacing::none);

        auto* l3 = l2->emplace_child<group_box>();
        l3->set_title("Level 3");
        l3->set_vbox_layout(spacing::none);

        auto* l4 = l3->emplace_child<panel>();
        apply_default_theme(*l4);
        l4->set_has_border(true);
        l4->set_vbox_layout(spacing::none);

        auto* l5_label = l4->emplace_child<label>("Deep");

        (void)l1.measure(200_lu, 200_lu);
        l1.arrange(logical_rect{0_lu, 0_lu, 200_lu, 200_lu});

        // Track cumulative offsets
        // L1: border(1) = 1
        // L2: padding(1) = 1 (no border)
        // L3: border(1) = 1 (group_box)
        // L4: border(1) = 1
        // Total: 4 pixels from L1 origin to L5 content

        auto l5_bounds = l5_label->bounds();

        // RELATIVE COORDINATES: child at (0,0) relative to parent's content area
        // Should be at (0, 0) relative to parent (4 levels of inset accounted for)
        CHECK(l5_bounds.x.to_int() == 0);
        CHECK(l5_bounds.y.to_int() == 0);

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
        outer.set_vbox_layout(spacing::none);

        auto* inner = outer.emplace_child<panel>();
        apply_default_theme(*inner);
        inner->set_has_border(true);
        inner->set_vbox_layout(spacing::none);

        auto* text_label = inner->emplace_child<label>("Text");

        (void)outer.measure(100_lu, 100_lu);
        outer.arrange(logical_rect{0_lu, 0_lu, 100_lu, 100_lu});

        auto initial_label_bounds = text_label->bounds();
        int initial_x = initial_label_bounds.x.to_int();
        int initial_y = initial_label_bounds.y.to_int();

        // Remove inner border
        inner->set_has_border(false);
        (void)outer.measure(100_lu, 100_lu);
        outer.arrange(logical_rect{0_lu, 0_lu, 100_lu, 100_lu});

        auto new_label_bounds = text_label->bounds();
        int const new_x = new_label_bounds.x.to_int();
        int const new_y = new_label_bounds.y.to_int();

        // RELATIVE COORDINATES: both initial and new positions are (0,0) relative to parent
        // Border removal doesn't change relative child coordinates
        CHECK(new_x == initial_x);
        CHECK(new_y == initial_y);
    }

    TEST_CASE_FIXTURE(test_fixture, "VBox with many children - spacing accumulation") {
        vbox<Backend> container(spacing::tiny);  // tiny spacing
        apply_default_theme(container);

        // Add 10 labels
        for (int i = 0; i < 10; ++i) {
            container.emplace_child<label>("Item");
        }

        (void)container.measure(100_lu, 500_lu);
        container.arrange(logical_rect{0_lu, 0_lu, 100_lu, 500_lu});

        // Verify each child is spaced with tiny spacing (0px in test theme) from previous
        const auto& children = container.children();
        for (size_t i = 1; i < children.size(); ++i) {
            auto prev_bounds = children[i - 1]->bounds();
            auto curr_bounds = children[i]->bounds();

            int const prev_bottom = prev_bounds.y.to_int() + prev_bounds.height.to_int();
            int const curr_top = curr_bounds.y.to_int();

            CHECK(curr_top - prev_bottom == 0);
        }
    }

    TEST_CASE_FIXTURE(test_fixture, "Complex padding - all four sides different at each level") {
        panel<Backend> l1;
        apply_default_theme(l1);
        l1.set_padding(logical_thickness{10_lu, 20_lu, 30_lu, 40_lu});  // L, T, R, B
        l1.set_vbox_layout(spacing::none);

        auto* l2 = l1.emplace_child<panel>();
        apply_default_theme(*l2);
        l2->set_padding(logical_thickness{5_lu, 15_lu, 25_lu, 35_lu});  // Different on all sides
        l2->set_vbox_layout(spacing::none);

        auto* text_label = l2->emplace_child<label>("Text");

        (void)l1.measure(300_lu, 300_lu);
        l1.arrange(logical_rect{0_lu, 0_lu, 300_lu, 300_lu});

        // RELATIVE COORDINATES: child at (0,0) relative to parent's content area
        // L1 positions L2 at (10, 20) via padding (accounted for in parent arrange)
        auto l2_bounds = l2->bounds();
        CHECK(l2_bounds.x.to_int() == 0);
        CHECK(l2_bounds.y.to_int() == 0);

        // RELATIVE COORDINATES: child at (0,0) relative to parent's content area
        // L2 positions label at relative (5, 15) via padding (accounted for in parent arrange)
        auto label_bounds = text_label->bounds();
        int const label_x_rel = label_bounds.x.to_int() - l2_bounds.x.to_int();
        int const label_y_rel = label_bounds.y.to_int() - l2_bounds.y.to_int();

        CHECK(label_x_rel == 0);
        CHECK(label_y_rel == 0);
    }

    TEST_CASE_FIXTURE(test_fixture, "Mixed spacing - VBox spacing + child padding") {
        vbox<Backend> container(spacing::small);  // small spacing between children
        apply_default_theme(container);
        container.set_padding(logical_thickness(3_lu));  // Container padding

        auto* p1 = container.emplace_child<panel>();
        apply_default_theme(*p1);
        p1->set_padding(logical_thickness(2_lu));
        p1->set_vbox_layout(spacing::none);
        p1->emplace_child<label>("First");

        auto* p2 = container.emplace_child<panel>();
        apply_default_theme(*p2);
        p2->set_padding(logical_thickness(4_lu));
        p2->set_vbox_layout(spacing::none);
        p2->emplace_child<label>("Second");

        (void)container.measure(200_lu, 200_lu);
        container.arrange(logical_rect{0_lu, 0_lu, 200_lu, 200_lu});

        // RELATIVE COORDINATES: child at (0,0) relative to parent's content area
        // First panel at container_padding(3) (accounted for in parent arrange)
        auto p1_bounds = p1->bounds();
        CHECK(p1_bounds.x.to_int() == 0);
        CHECK(p1_bounds.y.to_int() == 0);

        // Second panel: p1_y + p1_height + spacing::small (1 in test theme)
        auto p2_bounds = p2->bounds();
        CHECK(p2_bounds.x.to_int() == 0);

        int expected_y2 = p1_bounds.y.to_int() +
                          p1_bounds.height.to_int() + 1;  // spacing::small = 1
        CHECK(p2_bounds.y.to_int() == expected_y2);
    }

    TEST_CASE_FIXTURE(test_fixture, "Group box with multiple nested VBoxes") {
        group_box<Backend> gb;
        apply_default_theme(gb);
        gb.set_title("Container");
        gb.set_padding(logical_thickness(2_lu));
        gb.set_vbox_layout(spacing::small);  // small spacing

        // Each child is a VBox with its own spacing
        auto* vb1 = gb.emplace_child<vbox>(spacing::tiny);
        vb1->emplace_child<label>("1A");
        vb1->emplace_child<label>("1B");

        auto* vb2 = gb.emplace_child<vbox>(spacing::tiny);
        vb2->emplace_child<label>("2A");
        vb2->emplace_child<label>("2B");

        (void)gb.measure(200_lu, 200_lu);
        gb.arrange(logical_rect{0_lu, 0_lu, 200_lu, 200_lu});

        // RELATIVE COORDINATES: child at (0,0) relative to parent's content area
        // Group box border(1) + padding(2) = 3 (accounted for in parent arrange)
        auto vb1_bounds = vb1->bounds();
        CHECK(vb1_bounds.x.to_int() == 0);
        CHECK(vb1_bounds.y.to_int() == 0);

        // vb2 should be vb1 + height + spacing
        // Note: set_vbox_layout(spacing::small) resolves to 1 in test theme
        auto vb2_bounds = vb2->bounds();
        int expected_vb2_y = vb1_bounds.y.to_int() +
                             vb1_bounds.height.to_int() + 1;  // spacing::small resolves to 1
        CHECK(vb2_bounds.y.to_int() == expected_vb2_y);
    }

    TEST_CASE_FIXTURE(test_fixture, "Empty containers in complex hierarchy") {
        vbox<Backend> outer(spacing::none);
        apply_default_theme(outer);

        // Empty panel
        auto* empty1 = outer.emplace_child<panel>();
        apply_default_theme(*empty1);
        empty1->set_vbox_layout(spacing::none);

        // Panel with label
        auto* filled = outer.emplace_child<panel>();
        apply_default_theme(*filled);
        filled->set_vbox_layout(spacing::none);
        filled->emplace_child<label>("Content");

        // Another empty panel
        auto* empty2 = outer.emplace_child<panel>();
        apply_default_theme(*empty2);
        empty2->set_vbox_layout(spacing::none);

        (void)outer.measure(100_lu, 100_lu);
        outer.arrange(logical_rect{0_lu, 0_lu, 100_lu, 100_lu});

        // Should not crash, all panels positioned
        CHECK(empty1->bounds().y.to_int() >= 0);
        CHECK(filled->bounds().y.to_int() >= 0);
        CHECK(empty2->bounds().y.to_int() >= 0);
    }
}
