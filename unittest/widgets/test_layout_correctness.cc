//
// Created by claude on 16/10/2025.
//
// Comprehensive tests for layout algorithmic correctness and composition

#include <doctest/doctest.h>
#include "../utils/test_helpers.hh"

#include <memory>
#include "../utils/test_helpers.hh"
#include <onyxui/widgets/button.hh>
#include "../utils/test_helpers.hh"
#include <onyxui/widgets/label.hh>
#include "../utils/test_helpers.hh"
#include <../../include/onyxui/widgets/containers/panel.hh>
#include "../utils/test_helpers.hh"
#include <../../include/onyxui/widgets/containers/hbox.hh>
#include "../utils/test_helpers.hh"
#include <../../include/onyxui/widgets/containers/vbox.hh>
#include "../utils/test_helpers.hh"
#include <../../include/onyxui/widgets/layout/spacer.hh>
#include "../utils/test_helpers.hh"
#include <../../include/onyxui/widgets/layout/spring.hh>
#include "../utils/test_helpers.hh"
#include <../../include/onyxui/services/ui_context.hh>
#include "../utils/test_helpers.hh"
#include <utility>
#include "../utils/test_helpers.hh"
#include "../utils/test_backend.hh"
#include "../utils/test_canvas_backend.hh"
#include "../utils/test_helpers.hh"
#include "onyxui/concepts/size_like.hh"
#include "../utils/test_helpers.hh"
#include "../../include/onyxui/layout/layout_strategy.hh"
#include "../utils/test_helpers.hh"
#include "onyxui/concepts/rect_like.hh"
#include "../utils/test_helpers.hh"

using namespace onyxui;
using namespace onyxui::testing;

// ============================================================================
// MEASURE PHASE CORRECTNESS
// ============================================================================

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "Layout Correctness - Measure Phase") {
    SUBCASE("HBox - Measure with fixed-size children") {
        hbox<test_canvas_backend> box(spacing::none);

        // Add three buttons with fixed sizes
        auto btn1 = std::make_unique<button<test_canvas_backend>>("A");
        auto btn2 = std::make_unique<button<test_canvas_backend>>("B");
        auto btn3 = std::make_unique<button<test_canvas_backend>>("C");

        box.add_child(std::move(btn1));
        box.add_child(std::move(btn2));
        box.add_child(std::move(btn3));

        // Measure
        auto size = box.measure(1000_lu, 100_lu);

        // Width should be sum of children widths (no spacing)
        // Height should be max of children heights
        CHECK(size.width.to_int() > 0);
        CHECK(size.height.to_int() > 0);
    }

    SUBCASE("HBox - Measure with spacing") {
        hbox<test_canvas_backend> box(spacing::medium);

        auto btn1 = std::make_unique<button<test_canvas_backend>>("A");
        auto btn2 = std::make_unique<button<test_canvas_backend>>("B");

        box.add_child(std::move(btn1));
        box.add_child(std::move(btn2));

        auto size = box.measure(1000_lu, 100_lu);
        int const width = size.width.to_int();

        // Width should include spacing between buttons
        CHECK(width > 0);
    }

    SUBCASE("VBox - Measure stacks children vertically") {
        vbox<test_canvas_backend> box(spacing::none);

        auto lbl1 = std::make_unique<label<test_canvas_backend>>("Line 1");
        auto lbl2 = std::make_unique<label<test_canvas_backend>>("Line 2");

        box.add_child(std::move(lbl1));
        box.add_child(std::move(lbl2));

        auto size = box.measure(200_lu, 1000_lu);

        // Height should be sum of children heights
        // Width should be max of children widths
        CHECK(size.width.to_int() > 0);
        CHECK(size.height.to_int() > 0);
    }

    SUBCASE("Spacer - Fixed size measurement") {
        spacer<test_canvas_backend> s(50, 30);

        auto size = s.measure(1000_lu, 1000_lu);

        CHECK(size.width.to_int() == 50);
        CHECK(size.height.to_int() == 30);
    }

    SUBCASE("Spring - Minimal size measurement") {
        spring<test_canvas_backend> spr(1.0F, true);  // Horizontal spring
        spr.set_min_size(20);

        auto size = spr.measure(1000_lu, 100_lu);

        // Spring should return at least min_size
        CHECK(size.width.to_int() >= 20);
    }
}

// ============================================================================
// ARRANGE PHASE CORRECTNESS
// ============================================================================

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "Layout Correctness - Arrange Phase") {
    SUBCASE("HBox - Horizontal positioning without spacing") {
        hbox<test_canvas_backend> box(spacing::none);

        // Create fixed-size panels for predictable sizing
        auto panel1 = std::make_unique<panel<test_canvas_backend>>();
        auto panel2 = std::make_unique<panel<test_canvas_backend>>();
        auto panel3 = std::make_unique<panel<test_canvas_backend>>();

        // Set fixed sizes
        size_constraint fixed_width;
        fixed_width.policy = size_policy::fixed;
        fixed_width.preferred_size = 100_lu;
        fixed_width.min_size = 100_lu;
        fixed_width.max_size = 100_lu;

        panel1->set_width_constraint(fixed_width);
        panel2->set_width_constraint(fixed_width);
        panel3->set_width_constraint(fixed_width);

        auto* p1 = panel1.get();
        auto* p2 = panel2.get();
        auto* p3 = panel3.get();

        box.add_child(std::move(panel1));
        box.add_child(std::move(panel2));
        box.add_child(std::move(panel3));

        // Measure and arrange
        (void)box.measure(500_lu, 100_lu);

        box.arrange(logical_rect{0_lu, 0_lu, 500_lu, 100_lu});

        // Verify horizontal positioning (no spacing)
        auto b1 = p1->bounds();
        auto b2 = p2->bounds();
        auto b3 = p3->bounds();

        CHECK(b1.x == 0_lu);
        CHECK(b2.x == 100_lu);  // After panel1
        CHECK(b3.x == 200_lu);  // After panel1 + panel2
    }

    SUBCASE("HBox - Horizontal positioning with spacing") {
        hbox<test_canvas_backend> box(spacing::large);

        auto panel1 = std::make_unique<panel<test_canvas_backend>>();
        auto panel2 = std::make_unique<panel<test_canvas_backend>>();

        // Set fixed widths
        size_constraint fixed_width;
        fixed_width.policy = size_policy::fixed;
        fixed_width.preferred_size = 100_lu;
        fixed_width.min_size = 100_lu;
        fixed_width.max_size = 100_lu;

        panel1->set_width_constraint(fixed_width);
        panel2->set_width_constraint(fixed_width);

        auto* p1 = panel1.get();
        auto* p2 = panel2.get();

        box.add_child(std::move(panel1));
        box.add_child(std::move(panel2));

        (void)box.measure(500_lu, 100_lu);

                box.arrange(logical_rect{0_lu, 0_lu, 500_lu, 100_lu});

        auto b1 = p1->bounds();
        auto b2 = p2->bounds();

        CHECK(b1.x == 0_lu);
        CHECK(b2.x == 102_lu);  // 100 (panel1 width) + 2 (spacing::large in test theme)
    }

    SUBCASE("VBox - Vertical positioning without spacing") {
        vbox<test_canvas_backend> box(spacing::none);

        auto panel1 = std::make_unique<panel<test_canvas_backend>>();
        auto panel2 = std::make_unique<panel<test_canvas_backend>>();

        // Set fixed heights
        size_constraint fixed_height;
        fixed_height.policy = size_policy::fixed;
        fixed_height.preferred_size = 50_lu;
        fixed_height.min_size = 50_lu;
        fixed_height.max_size = 50_lu;

        panel1->set_height_constraint(fixed_height);
        panel2->set_height_constraint(fixed_height);

        auto* p1 = panel1.get();
        auto* p2 = panel2.get();

        box.add_child(std::move(panel1));
        box.add_child(std::move(panel2));

        (void)box.measure(200_lu, 500_lu);

                box.arrange(logical_rect{0_lu, 0_lu, 200_lu, 500_lu});

        auto b1 = p1->bounds();
        auto b2 = p2->bounds();

        CHECK(b1.y == 0_lu);
        CHECK(b2.y == 50_lu);  // After panel1
    }

    SUBCASE("VBox - Vertical positioning with spacing") {
        vbox<test_canvas_backend> box(spacing::large);

        auto panel1 = std::make_unique<panel<test_canvas_backend>>();
        auto panel2 = std::make_unique<panel<test_canvas_backend>>();

        // Set fixed heights
        size_constraint fixed_height;
        fixed_height.policy = size_policy::fixed;
        fixed_height.preferred_size = 50_lu;
        fixed_height.min_size = 50_lu;
        fixed_height.max_size = 50_lu;

        panel1->set_height_constraint(fixed_height);
        panel2->set_height_constraint(fixed_height);

        auto* p1 = panel1.get();
        auto* p2 = panel2.get();

        box.add_child(std::move(panel1));
        box.add_child(std::move(panel2));

        (void)box.measure(200_lu, 500_lu);

                box.arrange(logical_rect{0_lu, 0_lu, 200_lu, 500_lu});

        auto b1 = p1->bounds();
        auto b2 = p2->bounds();

        CHECK(b1.y == 0_lu);
        CHECK(b2.y == 52_lu);  // 50 (panel1 height) + 2 (spacing::large in test theme)
    }
}

// ============================================================================
// FIXED SPACER CORRECTNESS
// ============================================================================

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "Layout Correctness - Fixed Spacers") {
    SUBCASE("HBox - Spacer creates exact gap") {
        hbox<test_canvas_backend> box(spacing::none);

        auto panel1 = std::make_unique<panel<test_canvas_backend>>();
        auto gap = std::make_unique<spacer<test_canvas_backend>>(30, 0);  // 30px horizontal gap
        auto panel2 = std::make_unique<panel<test_canvas_backend>>();

        // Set fixed widths for panels
        size_constraint fixed_width;
        fixed_width.policy = size_policy::fixed;
        fixed_width.preferred_size = 100_lu;
        fixed_width.min_size = 100_lu;
        fixed_width.max_size = 100_lu;

        panel1->set_width_constraint(fixed_width);
        panel2->set_width_constraint(fixed_width);

        auto* p1 = panel1.get();
        auto* p2 = panel2.get();

        box.add_child(std::move(panel1));
        box.add_child(std::move(gap));
        box.add_child(std::move(panel2));

        (void)box.measure(500_lu, 100_lu);

                box.arrange(logical_rect{0_lu, 0_lu, 500_lu, 100_lu});

        auto b1 = p1->bounds();
        auto b2 = p2->bounds();

        // panel1 should start at 0
        CHECK(b1.x == 0_lu);

        // panel2 should start at 130: 100 (panel1) + 30 (spacer)
        CHECK(b2.x == 130_lu);
    }

    SUBCASE("VBox - Spacer creates exact gap") {
        vbox<test_canvas_backend> box(spacing::none);

        auto panel1 = std::make_unique<panel<test_canvas_backend>>();
        auto gap = std::make_unique<spacer<test_canvas_backend>>(0, 25);  // 25px vertical gap
        auto panel2 = std::make_unique<panel<test_canvas_backend>>();

        // Set fixed heights
        size_constraint fixed_height;
        fixed_height.policy = size_policy::fixed;
        fixed_height.preferred_size = 50_lu;
        fixed_height.min_size = 50_lu;
        fixed_height.max_size = 50_lu;

        panel1->set_height_constraint(fixed_height);
        panel2->set_height_constraint(fixed_height);

        auto* p1 = panel1.get();
        auto* p2 = panel2.get();

        box.add_child(std::move(panel1));
        box.add_child(std::move(gap));
        box.add_child(std::move(panel2));

        (void)box.measure(200_lu, 500_lu);

                box.arrange(logical_rect{0_lu, 0_lu, 200_lu, 500_lu});

        auto b1 = p1->bounds();
        auto b2 = p2->bounds();

        // panel1 should start at 0
        CHECK(b1.y == 0_lu);

        // panel2 should start at 75: 50 (panel1) + 25 (spacer)
        CHECK(b2.y == 75_lu);
    }
}

// ============================================================================
// SPRING (FLEXIBLE SPACING) CORRECTNESS
// ============================================================================

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "Layout Correctness - Flexible Springs") {
    SUBCASE("HBox - Spring expands to fill space") {
        hbox<test_canvas_backend> box(spacing::none);

        auto panel1 = std::make_unique<panel<test_canvas_backend>>();
        auto spr = std::make_unique<spring<test_canvas_backend>>(1.0F, true);  // Horizontal spring
        auto panel2 = std::make_unique<panel<test_canvas_backend>>();

        // Set fixed widths for panels
        size_constraint fixed_width;
        fixed_width.policy = size_policy::fixed;
        fixed_width.preferred_size = 50_lu;
        fixed_width.min_size = 50_lu;
        fixed_width.max_size = 50_lu;

        panel1->set_width_constraint(fixed_width);
        panel2->set_width_constraint(fixed_width);

        auto* p1 = panel1.get();
        auto* spring_ptr = spr.get();
        auto* p2 = panel2.get();

        box.add_child(std::move(panel1));
        box.add_child(std::move(spr));
        box.add_child(std::move(panel2));

        (void)box.measure(400_lu, 100_lu);

                box.arrange(logical_rect{0_lu, 0_lu, 400_lu, 100_lu});

        auto b1 = p1->bounds();
        auto spring_bounds = spring_ptr->bounds();
        auto b2 = p2->bounds();

        // panel1: x=0, w=50
        // spring: x=50, w=300 (fills remaining space)
        // panel2: x=350, w=50
        CHECK(b1.x == 0_lu);
        CHECK(b1.width == 50_lu);

        CHECK(spring_bounds.x == 50_lu);
        CHECK(spring_bounds.width == 300_lu);

        CHECK(b2.x == 350_lu);
        CHECK(b2.width == 50_lu);
    }

    SUBCASE("HBox - Two equal springs split space equally") {
        hbox<test_canvas_backend> box(spacing::none);

        auto spr1 = std::make_unique<spring<test_canvas_backend>>(1.0F, true);
        auto center_panel = std::make_unique<panel<test_canvas_backend>>();
        auto spr2 = std::make_unique<spring<test_canvas_backend>>(1.0F, true);

        // Fixed width panel
        size_constraint fixed_width;
        fixed_width.policy = size_policy::fixed;
        fixed_width.preferred_size = 100_lu;
        fixed_width.min_size = 100_lu;
        fixed_width.max_size = 100_lu;
        center_panel->set_width_constraint(fixed_width);

        auto* spring1_ptr = spr1.get();
        auto* panel_ptr = center_panel.get();
        auto* spring2_ptr = spr2.get();

        box.add_child(std::move(spr1));
        box.add_child(std::move(center_panel));
        box.add_child(std::move(spr2));

        (void)box.measure(400_lu, 100_lu);

                box.arrange(logical_rect{0_lu, 0_lu, 400_lu, 100_lu});

        auto s1_bounds = spring1_ptr->bounds();
        auto p_bounds = panel_ptr->bounds();
        auto s2_bounds = spring2_ptr->bounds();

        // Total width: 400
        // Panel: 100
        // Remaining: 300, split equally: 150 each
        CHECK(s1_bounds.width == 150_lu);
        CHECK(p_bounds.width == 100_lu);
        CHECK(s2_bounds.width == 150_lu);

        // Positions: spring1 at 0, panel at 150, spring2 at 250
        CHECK(s1_bounds.x == 0_lu);
        CHECK(p_bounds.x == 150_lu);
        CHECK(s2_bounds.x == 250_lu);
    }

    SUBCASE("HBox - Weighted springs (2:1 ratio)") {
        hbox<test_canvas_backend> box(spacing::none);

        auto spr1 = std::make_unique<spring<test_canvas_backend>>(2.0F, true);  // 2x weight
        auto middle_panel = std::make_unique<panel<test_canvas_backend>>();
        auto spr2 = std::make_unique<spring<test_canvas_backend>>(1.0F, true);  // 1x weight

        // Fixed width panel
        size_constraint fixed_width;
        fixed_width.policy = size_policy::fixed;
        fixed_width.preferred_size = 100_lu;
        fixed_width.min_size = 100_lu;
        fixed_width.max_size = 100_lu;
        middle_panel->set_width_constraint(fixed_width);

        auto* spring1_ptr = spr1.get();
        auto* spring2_ptr = spr2.get();

        box.add_child(std::move(spr1));
        box.add_child(std::move(middle_panel));
        box.add_child(std::move(spr2));

        (void)box.measure(400_lu, 100_lu);

                box.arrange(logical_rect{0_lu, 0_lu, 400_lu, 100_lu});

        auto s1_bounds = spring1_ptr->bounds();
        auto s2_bounds = spring2_ptr->bounds();

        // Total: 400, Panel: 100, Remaining: 300
        // Weights: 2:1, so spring1 gets 200, spring2 gets 100
        INFO("s1_bounds.width.value = " << s1_bounds.width.value);
        INFO("s2_bounds.width.value = " << s2_bounds.width.value);
        INFO("200_lu.value = " << logical_unit(200.0).value);
        INFO("Comparison: " << (s1_bounds.width == logical_unit(200.0)));
        CHECK(s1_bounds.width.to_int() == 200);
        CHECK(s2_bounds.width.to_int() == 100);
    }

    SUBCASE("VBox - Vertical spring expands") {
        vbox<test_canvas_backend> box(spacing::none);

        auto panel1 = std::make_unique<panel<test_canvas_backend>>();
        auto spr = std::make_unique<spring<test_canvas_backend>>(1.0F, false);  // Vertical spring
        auto panel2 = std::make_unique<panel<test_canvas_backend>>();

        // Set fixed heights
        size_constraint fixed_height;
        fixed_height.policy = size_policy::fixed;
        fixed_height.preferred_size = 50_lu;
        fixed_height.min_size = 50_lu;
        fixed_height.max_size = 50_lu;

        panel1->set_height_constraint(fixed_height);
        panel2->set_height_constraint(fixed_height);

        auto* p1 = panel1.get();
        auto* spring_ptr = spr.get();
        auto* p2 = panel2.get();

        box.add_child(std::move(panel1));
        box.add_child(std::move(spr));
        box.add_child(std::move(panel2));

        (void)box.measure(200_lu, 400_lu);

                box.arrange(logical_rect{0_lu, 0_lu, 200_lu, 400_lu});

        auto b1 = p1->bounds();
        auto spring_bounds = spring_ptr->bounds();
        auto b2 = p2->bounds();

        // panel1: y=0, h=50
        // spring: y=50, h=300 (fills remaining space)
        // panel2: y=350, h=50
        CHECK(b1.y == 0_lu);
        CHECK(b1.height == 50_lu);

        CHECK(spring_bounds.y == 50_lu);
        CHECK(spring_bounds.height == 300_lu);

        CHECK(b2.y == 350_lu);
        CHECK(b2.height == 50_lu);
    }
}

// ============================================================================
// COMPOSITION - NESTED LAYOUTS
// ============================================================================

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "Layout Correctness - Nested Layouts") {
    SUBCASE("VBox containing HBox - Dialog button row") {
        // Create a dialog layout: title + button row
        vbox<test_canvas_backend> dialog(spacing::medium);

        auto title = std::make_unique<label<test_canvas_backend>>("Confirm Action");
        auto button_row = std::make_unique<hbox<test_canvas_backend>>(spacing::small);

        // Add buttons to hbox
        auto ok_btn = std::make_unique<button<test_canvas_backend>>("OK");
        auto cancel_btn = std::make_unique<button<test_canvas_backend>>("Cancel");

        button_row->add_child(std::move(ok_btn));
        button_row->add_child(std::move(cancel_btn));

        auto* button_row_ptr = button_row.get();

        dialog.add_child(std::move(title));
        dialog.add_child(std::move(button_row));

        // Measure and arrange
        (void)dialog.measure(300_lu, 200_lu);

                dialog.arrange(logical_rect{0_lu, 0_lu, 300_lu, 200_lu});

        // Verify button row is positioned below title + spacing
        auto button_row_bounds = button_row_ptr->bounds();
        CHECK(button_row_bounds.y > 0_lu);  // Should be below title

        // Verify button row has children arranged
        CHECK(button_row_ptr->children().size() == 2);
    }

    SUBCASE("HBox containing VBox - Toolbar groups") {
        // Create toolbar with vertical button groups
        hbox<test_canvas_backend> toolbar(spacing::large);

        auto group1 = std::make_unique<vbox<test_canvas_backend>>(spacing::small);
        auto group2 = std::make_unique<vbox<test_canvas_backend>>(spacing::small);

        group1->add_child(std::make_unique<button<test_canvas_backend>>("New"));
        group1->add_child(std::make_unique<button<test_canvas_backend>>("Open"));

        group2->add_child(std::make_unique<button<test_canvas_backend>>("Cut"));
        group2->add_child(std::make_unique<button<test_canvas_backend>>("Copy"));

        auto* g1_ptr = group1.get();
        auto* g2_ptr = group2.get();

        toolbar.add_child(std::move(group1));
        toolbar.add_child(std::move(group2));

        (void)toolbar.measure(400_lu, 100_lu);

                toolbar.arrange(logical_rect{0_lu, 0_lu, 400_lu, 100_lu});

        auto g1_bounds = g1_ptr->bounds();
        auto g2_bounds = g2_ptr->bounds();

        // group2 should be positioned after group1 + spacing
        CHECK(g2_bounds.x > g1_bounds.x);
    }

    SUBCASE("Three-level nesting - Complex form layout") {
        // Main vertical layout
        vbox<test_canvas_backend> form(spacing::large);

        // Header
        auto header = std::make_unique<label<test_canvas_backend>>("User Registration");

        // Form fields (vbox of hboxes)
        auto fields = std::make_unique<vbox<test_canvas_backend>>(spacing::medium);

        // Name field row
        auto name_row = std::make_unique<hbox<test_canvas_backend>>(spacing::medium);
        name_row->add_child(std::make_unique<label<test_canvas_backend>>("Name:"));
        name_row->add_child(std::make_unique<panel<test_canvas_backend>>());  // Input field
        fields->add_child(std::move(name_row));

        // Email field row
        auto email_row = std::make_unique<hbox<test_canvas_backend>>(spacing::medium);
        email_row->add_child(std::make_unique<label<test_canvas_backend>>("Email:"));
        email_row->add_child(std::make_unique<panel<test_canvas_backend>>());  // Input field
        fields->add_child(std::move(email_row));

        // Button row
        auto button_row = std::make_unique<hbox<test_canvas_backend>>(spacing::medium);
        button_row->add_child(std::make_unique<button<test_canvas_backend>>("Submit"));
        button_row->add_child(std::make_unique<button<test_canvas_backend>>("Cancel"));

        form.add_child(std::move(header));
        form.add_child(std::move(fields));
        form.add_child(std::move(button_row));

        // Should not crash with complex nesting
        CHECK_NOTHROW((void)form.measure(400_lu, 300_lu));

                CHECK_NOTHROW(form.arrange(logical_rect{0_lu, 0_lu, 400_lu, 300_lu}));

        CHECK(form.children().size() == 3);
    }

    SUBCASE("Toolbar with springs - Push to edges") {
        hbox<test_canvas_backend> toolbar(spacing::none);

        auto left_btn = std::make_unique<button<test_canvas_backend>>("Menu");
        auto spr = std::make_unique<spring<test_canvas_backend>>(1.0F, true);
        auto right_btn = std::make_unique<button<test_canvas_backend>>("Help");

        // Fixed size buttons
        size_constraint fixed_width;
        fixed_width.policy = size_policy::fixed;
        fixed_width.preferred_size = 80_lu;
        fixed_width.min_size = 80_lu;
        fixed_width.max_size = 80_lu;

        left_btn->set_width_constraint(fixed_width);
        right_btn->set_width_constraint(fixed_width);

        auto* left_ptr = left_btn.get();
        auto* right_ptr = right_btn.get();

        toolbar.add_child(std::move(left_btn));
        toolbar.add_child(std::move(spr));
        toolbar.add_child(std::move(right_btn));

        (void)toolbar.measure(400_lu, 50_lu);

                toolbar.arrange(logical_rect{0_lu, 0_lu, 400_lu, 50_lu});

        auto left_bounds = left_ptr->bounds();
        auto right_bounds = right_ptr->bounds();

        // Left button at 0
        CHECK(left_bounds.x == 0_lu);

        // Right button pushed to right edge
        CHECK(right_bounds.x == 320_lu);  // 400 - 80
    }

    SUBCASE("Centered content with springs") {
        hbox<test_canvas_backend> container(spacing::none);

        auto left_spring = std::make_unique<spring<test_canvas_backend>>(1.0F, true);
        auto content = std::make_unique<panel<test_canvas_backend>>();
        auto right_spring = std::make_unique<spring<test_canvas_backend>>(1.0F, true);

        // Fixed size content
        size_constraint fixed_width;
        fixed_width.policy = size_policy::fixed;
        fixed_width.preferred_size = 200_lu;
        fixed_width.min_size = 200_lu;
        fixed_width.max_size = 200_lu;
        content->set_width_constraint(fixed_width);

        auto* content_ptr = content.get();

        container.add_child(std::move(left_spring));
        container.add_child(std::move(content));
        container.add_child(std::move(right_spring));

        (void)container.measure(600_lu, 100_lu);

                container.arrange(logical_rect{0_lu, 0_lu, 600_lu, 100_lu});

        auto content_bounds = content_ptr->bounds();

        // Content should be centered: (600 - 200) / 2 = 200
        CHECK(content_bounds.x == 200_lu);
        CHECK(content_bounds.width == 200_lu);
    }
}

// ============================================================================
// REAL-WORLD LAYOUT SCENARIOS
// ============================================================================

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "Layout Correctness - Real-World Scenarios") {
    SUBCASE("Application window - Header/Content/Footer") {
        vbox<test_canvas_backend> window(spacing::none);

        // Fixed-size header
        auto header = std::make_unique<panel<test_canvas_backend>>();
        size_constraint header_height;
        header_height.policy = size_policy::fixed;
        header_height.preferred_size = 60_lu;
        header_height.min_size = 60_lu;
        header_height.max_size = 60_lu;
        header->set_height_constraint(header_height);

        // Expanding content area
        auto content = std::make_unique<panel<test_canvas_backend>>();
        size_constraint expand_height;
        expand_height.policy = size_policy::expand;
        content->set_height_constraint(expand_height);

        // Fixed-size footer
        auto footer = std::make_unique<panel<test_canvas_backend>>();
        size_constraint footer_height;
        footer_height.policy = size_policy::fixed;
        footer_height.preferred_size = 40_lu;
        footer_height.min_size = 40_lu;
        footer_height.max_size = 40_lu;
        footer->set_height_constraint(footer_height);

        auto* header_ptr = header.get();
        auto* content_ptr = content.get();
        auto* footer_ptr = footer.get();

        window.add_child(std::move(header));
        window.add_child(std::move(content));
        window.add_child(std::move(footer));

        (void)window.measure(800_lu, 600_lu);

                window.arrange(logical_rect{0_lu, 0_lu, 800_lu, 600_lu});

        auto h_bounds = header_ptr->bounds();
        auto c_bounds = content_ptr->bounds();
        auto f_bounds = footer_ptr->bounds();

        // Verify positions
        CHECK(h_bounds.y == 0_lu);
        CHECK(h_bounds.height == 60_lu);

        CHECK(c_bounds.y == 60_lu);
        CHECK(c_bounds.height == 500_lu);  // 600 - 60 - 40

        CHECK(f_bounds.y == 560_lu);
        CHECK(f_bounds.height == 40_lu);
    }

    SUBCASE("Split view with weighted panels") {
        hbox<test_canvas_backend> split(spacing::none);

        // Left panel: 1/3 of space
        auto left = std::make_unique<panel<test_canvas_backend>>();
        size_constraint left_width;
        left_width.policy = size_policy::weighted;
        left_width.weight = 1.0F;
        left->set_width_constraint(left_width);

        // Right panel: 2/3 of space
        auto right = std::make_unique<panel<test_canvas_backend>>();
        size_constraint right_width;
        right_width.policy = size_policy::weighted;
        right_width.weight = 2.0F;
        right->set_width_constraint(right_width);

        auto* left_ptr = left.get();
        auto* right_ptr = right.get();

        split.add_child(std::move(left));
        split.add_child(std::move(right));

        (void)split.measure(900_lu, 600_lu);

                split.arrange(logical_rect{0_lu, 0_lu, 900_lu, 600_lu});

        auto l_bounds = left_ptr->bounds();
        auto r_bounds = right_ptr->bounds();

        // Left: 1/3 = 300, Right: 2/3 = 600
        CHECK(l_bounds.width.to_int() == 300);
        CHECK(r_bounds.width.to_int() == 600);
        CHECK(r_bounds.x.to_int() == 300);
    }

    SUBCASE("Dashboard with mixed spacers and springs") {
        vbox<test_canvas_backend> dashboard(spacing::medium);

        // Title bar
        auto title = std::make_unique<label<test_canvas_backend>>("Dashboard");

        // Fixed spacer
        auto spacer1 = std::make_unique<spacer<test_canvas_backend>>(0, 20);

        // Content panel
        auto content = std::make_unique<panel<test_canvas_backend>>();

        // Flexible spring
        auto spring1 = std::make_unique<spring<test_canvas_backend>>(1.0F, false);

        // Bottom buttons
        auto buttons = std::make_unique<hbox<test_canvas_backend>>(spacing::medium);
        buttons->add_child(std::make_unique<button<test_canvas_backend>>("Refresh"));
        buttons->add_child(std::make_unique<button<test_canvas_backend>>("Settings"));

        dashboard.add_child(std::move(title));
        dashboard.add_child(std::move(spacer1));
        dashboard.add_child(std::move(content));
        dashboard.add_child(std::move(spring1));
        dashboard.add_child(std::move(buttons));

        // Should handle mix of spacers and springs
        CHECK_NOTHROW((void)dashboard.measure(600_lu, 800_lu));

                CHECK_NOTHROW(dashboard.arrange(logical_rect{0_lu, 0_lu, 600_lu, 800_lu}));
    }
}
