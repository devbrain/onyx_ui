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
#include <onyxui/widgets/panel.hh>
#include "../utils/test_helpers.hh"
#include <onyxui/widgets/hbox.hh>
#include "../utils/test_helpers.hh"
#include <onyxui/widgets/vbox.hh>
#include "../utils/test_helpers.hh"
#include <onyxui/widgets/spacer.hh>
#include "../utils/test_helpers.hh"
#include <onyxui/widgets/spring.hh>
#include "../utils/test_helpers.hh"
#include <onyxui/ui_context.hh>
#include "../utils/test_helpers.hh"
#include <utility>
#include "../utils/test_helpers.hh"
#include "../utils/test_backend.hh"
#include "../utils/test_canvas_backend.hh"
#include "../utils/test_helpers.hh"
#include "onyxui/concepts/size_like.hh"
#include "../utils/test_helpers.hh"
#include "onyxui/layout_strategy.hh"
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
        hbox<test_canvas_backend> box(0);  // No spacing

        // Add three buttons with fixed sizes
        auto btn1 = std::make_unique<button<test_canvas_backend>>("A");
        auto btn2 = std::make_unique<button<test_canvas_backend>>("B");
        auto btn3 = std::make_unique<button<test_canvas_backend>>("C");

        box.add_child(std::move(btn1));
        box.add_child(std::move(btn2));
        box.add_child(std::move(btn3));

        // Measure
        auto size = box.measure(1000, 100);

        // Width should be sum of children widths (no spacing)
        // Height should be max of children heights
        CHECK(size_utils::get_width(size) > 0);
        CHECK(size_utils::get_height(size) > 0);
    }

    SUBCASE("HBox - Measure with spacing") {
        hbox<test_canvas_backend> box(10);  // 10px spacing

        auto btn1 = std::make_unique<button<test_canvas_backend>>("A");
        auto btn2 = std::make_unique<button<test_canvas_backend>>("B");

        box.add_child(std::move(btn1));
        box.add_child(std::move(btn2));

        auto size = box.measure(1000, 100);
        int const width = size_utils::get_width(size);

        // Width should include spacing between buttons
        CHECK(width > 0);
    }

    SUBCASE("VBox - Measure stacks children vertically") {
        vbox<test_canvas_backend> box(0);  // No spacing

        auto lbl1 = std::make_unique<label<test_canvas_backend>>("Line 1");
        auto lbl2 = std::make_unique<label<test_canvas_backend>>("Line 2");

        box.add_child(std::move(lbl1));
        box.add_child(std::move(lbl2));

        auto size = box.measure(200, 1000);

        // Height should be sum of children heights
        // Width should be max of children widths
        CHECK(size_utils::get_width(size) > 0);
        CHECK(size_utils::get_height(size) > 0);
    }

    SUBCASE("Spacer - Fixed size measurement") {
        spacer<test_canvas_backend> s(50, 30);

        auto size = s.measure(1000, 1000);

        CHECK(size_utils::get_width(size) == 50);
        CHECK(size_utils::get_height(size) == 30);
    }

    SUBCASE("Spring - Minimal size measurement") {
        spring<test_canvas_backend> spr(1.0F, true);  // Horizontal spring
        spr.set_min_size(20);

        auto size = spr.measure(1000, 100);

        // Spring should return at least min_size
        CHECK(size_utils::get_width(size) >= 20);
    }
}

// ============================================================================
// ARRANGE PHASE CORRECTNESS
// ============================================================================

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "Layout Correctness - Arrange Phase") {
    SUBCASE("HBox - Horizontal positioning without spacing") {
        hbox<test_canvas_backend> box(0);  // No spacing

        // Create fixed-size panels for predictable sizing
        auto panel1 = std::make_unique<panel<test_canvas_backend>>();
        auto panel2 = std::make_unique<panel<test_canvas_backend>>();
        auto panel3 = std::make_unique<panel<test_canvas_backend>>();

        // Set fixed sizes
        size_constraint fixed_width;
        fixed_width.policy = size_policy::fixed;
        fixed_width.preferred_size = 100;
        fixed_width.min_size = 100;
        fixed_width.max_size = 100;

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
        (void)box.measure(500, 100);

        test_canvas_backend::rect_type bounds;
        rect_utils::set_bounds(bounds, 0, 0, 500, 100);
        box.arrange(bounds);

        // Verify horizontal positioning (no spacing)
        auto b1 = p1->bounds();
        auto b2 = p2->bounds();
        auto b3 = p3->bounds();

        CHECK(rect_utils::get_x(b1) == 0);
        CHECK(rect_utils::get_x(b2) == 100);  // After panel1
        CHECK(rect_utils::get_x(b3) == 200);  // After panel1 + panel2
    }

    SUBCASE("HBox - Horizontal positioning with spacing") {
        hbox<test_canvas_backend> box(20);  // 20px spacing

        auto panel1 = std::make_unique<panel<test_canvas_backend>>();
        auto panel2 = std::make_unique<panel<test_canvas_backend>>();

        // Set fixed widths
        size_constraint fixed_width;
        fixed_width.policy = size_policy::fixed;
        fixed_width.preferred_size = 100;
        fixed_width.min_size = 100;
        fixed_width.max_size = 100;

        panel1->set_width_constraint(fixed_width);
        panel2->set_width_constraint(fixed_width);

        auto* p1 = panel1.get();
        auto* p2 = panel2.get();

        box.add_child(std::move(panel1));
        box.add_child(std::move(panel2));

        (void)box.measure(500, 100);

        test_canvas_backend::rect_type bounds;
        rect_utils::set_bounds(bounds, 0, 0, 500, 100);
        box.arrange(bounds);

        auto b1 = p1->bounds();
        auto b2 = p2->bounds();

        CHECK(rect_utils::get_x(b1) == 0);
        CHECK(rect_utils::get_x(b2) == 120);  // 100 (panel1 width) + 20 (spacing)
    }

    SUBCASE("VBox - Vertical positioning without spacing") {
        vbox<test_canvas_backend> box(0);  // No spacing

        auto panel1 = std::make_unique<panel<test_canvas_backend>>();
        auto panel2 = std::make_unique<panel<test_canvas_backend>>();

        // Set fixed heights
        size_constraint fixed_height;
        fixed_height.policy = size_policy::fixed;
        fixed_height.preferred_size = 50;
        fixed_height.min_size = 50;
        fixed_height.max_size = 50;

        panel1->set_height_constraint(fixed_height);
        panel2->set_height_constraint(fixed_height);

        auto* p1 = panel1.get();
        auto* p2 = panel2.get();

        box.add_child(std::move(panel1));
        box.add_child(std::move(panel2));

        (void)box.measure(200, 500);

        test_canvas_backend::rect_type bounds;
        rect_utils::set_bounds(bounds, 0, 0, 200, 500);
        box.arrange(bounds);

        auto b1 = p1->bounds();
        auto b2 = p2->bounds();

        CHECK(rect_utils::get_y(b1) == 0);
        CHECK(rect_utils::get_y(b2) == 50);  // After panel1
    }

    SUBCASE("VBox - Vertical positioning with spacing") {
        vbox<test_canvas_backend> box(15);  // 15px spacing

        auto panel1 = std::make_unique<panel<test_canvas_backend>>();
        auto panel2 = std::make_unique<panel<test_canvas_backend>>();

        // Set fixed heights
        size_constraint fixed_height;
        fixed_height.policy = size_policy::fixed;
        fixed_height.preferred_size = 50;
        fixed_height.min_size = 50;
        fixed_height.max_size = 50;

        panel1->set_height_constraint(fixed_height);
        panel2->set_height_constraint(fixed_height);

        auto* p1 = panel1.get();
        auto* p2 = panel2.get();

        box.add_child(std::move(panel1));
        box.add_child(std::move(panel2));

        (void)box.measure(200, 500);

        test_canvas_backend::rect_type bounds;
        rect_utils::set_bounds(bounds, 0, 0, 200, 500);
        box.arrange(bounds);

        auto b1 = p1->bounds();
        auto b2 = p2->bounds();

        CHECK(rect_utils::get_y(b1) == 0);
        CHECK(rect_utils::get_y(b2) == 65);  // 50 (panel1 height) + 15 (spacing)
    }
}

// ============================================================================
// FIXED SPACER CORRECTNESS
// ============================================================================

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "Layout Correctness - Fixed Spacers") {
    SUBCASE("HBox - Spacer creates exact gap") {
        hbox<test_canvas_backend> box(0);

        auto panel1 = std::make_unique<panel<test_canvas_backend>>();
        auto gap = std::make_unique<spacer<test_canvas_backend>>(30, 0);  // 30px horizontal gap
        auto panel2 = std::make_unique<panel<test_canvas_backend>>();

        // Set fixed widths for panels
        size_constraint fixed_width;
        fixed_width.policy = size_policy::fixed;
        fixed_width.preferred_size = 100;
        fixed_width.min_size = 100;
        fixed_width.max_size = 100;

        panel1->set_width_constraint(fixed_width);
        panel2->set_width_constraint(fixed_width);

        auto* p1 = panel1.get();
        auto* p2 = panel2.get();

        box.add_child(std::move(panel1));
        box.add_child(std::move(gap));
        box.add_child(std::move(panel2));

        (void)box.measure(500, 100);

        test_canvas_backend::rect_type bounds;
        rect_utils::set_bounds(bounds, 0, 0, 500, 100);
        box.arrange(bounds);

        auto b1 = p1->bounds();
        auto b2 = p2->bounds();

        // panel1 should start at 0
        CHECK(rect_utils::get_x(b1) == 0);

        // panel2 should start at 130: 100 (panel1) + 30 (spacer)
        CHECK(rect_utils::get_x(b2) == 130);
    }

    SUBCASE("VBox - Spacer creates exact gap") {
        vbox<test_canvas_backend> box(0);

        auto panel1 = std::make_unique<panel<test_canvas_backend>>();
        auto gap = std::make_unique<spacer<test_canvas_backend>>(0, 25);  // 25px vertical gap
        auto panel2 = std::make_unique<panel<test_canvas_backend>>();

        // Set fixed heights
        size_constraint fixed_height;
        fixed_height.policy = size_policy::fixed;
        fixed_height.preferred_size = 50;
        fixed_height.min_size = 50;
        fixed_height.max_size = 50;

        panel1->set_height_constraint(fixed_height);
        panel2->set_height_constraint(fixed_height);

        auto* p1 = panel1.get();
        auto* p2 = panel2.get();

        box.add_child(std::move(panel1));
        box.add_child(std::move(gap));
        box.add_child(std::move(panel2));

        (void)box.measure(200, 500);

        test_canvas_backend::rect_type bounds;
        rect_utils::set_bounds(bounds, 0, 0, 200, 500);
        box.arrange(bounds);

        auto b1 = p1->bounds();
        auto b2 = p2->bounds();

        // panel1 should start at 0
        CHECK(rect_utils::get_y(b1) == 0);

        // panel2 should start at 75: 50 (panel1) + 25 (spacer)
        CHECK(rect_utils::get_y(b2) == 75);
    }
}

// ============================================================================
// SPRING (FLEXIBLE SPACING) CORRECTNESS
// ============================================================================

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "Layout Correctness - Flexible Springs") {
    SUBCASE("HBox - Spring expands to fill space") {
        hbox<test_canvas_backend> box(0);

        auto panel1 = std::make_unique<panel<test_canvas_backend>>();
        auto spr = std::make_unique<spring<test_canvas_backend>>(1.0F, true);  // Horizontal spring
        auto panel2 = std::make_unique<panel<test_canvas_backend>>();

        // Set fixed widths for panels
        size_constraint fixed_width;
        fixed_width.policy = size_policy::fixed;
        fixed_width.preferred_size = 50;
        fixed_width.min_size = 50;
        fixed_width.max_size = 50;

        panel1->set_width_constraint(fixed_width);
        panel2->set_width_constraint(fixed_width);

        auto* p1 = panel1.get();
        auto* spring_ptr = spr.get();
        auto* p2 = panel2.get();

        box.add_child(std::move(panel1));
        box.add_child(std::move(spr));
        box.add_child(std::move(panel2));

        (void)box.measure(400, 100);

        test_canvas_backend::rect_type bounds;
        rect_utils::set_bounds(bounds, 0, 0, 400, 100);
        box.arrange(bounds);

        auto b1 = p1->bounds();
        auto spring_bounds = spring_ptr->bounds();
        auto b2 = p2->bounds();

        // panel1: x=0, w=50
        // spring: x=50, w=300 (fills remaining space)
        // panel2: x=350, w=50
        CHECK(rect_utils::get_x(b1) == 0);
        CHECK(rect_utils::get_width(b1) == 50);

        CHECK(rect_utils::get_x(spring_bounds) == 50);
        CHECK(rect_utils::get_width(spring_bounds) == 300);

        CHECK(rect_utils::get_x(b2) == 350);
        CHECK(rect_utils::get_width(b2) == 50);
    }

    SUBCASE("HBox - Two equal springs split space equally") {
        hbox<test_canvas_backend> box(0);

        auto spr1 = std::make_unique<spring<test_canvas_backend>>(1.0F, true);
        auto center_panel = std::make_unique<panel<test_canvas_backend>>();
        auto spr2 = std::make_unique<spring<test_canvas_backend>>(1.0F, true);

        // Fixed width panel
        size_constraint fixed_width;
        fixed_width.policy = size_policy::fixed;
        fixed_width.preferred_size = 100;
        fixed_width.min_size = 100;
        fixed_width.max_size = 100;
        center_panel->set_width_constraint(fixed_width);

        auto* spring1_ptr = spr1.get();
        auto* panel_ptr = center_panel.get();
        auto* spring2_ptr = spr2.get();

        box.add_child(std::move(spr1));
        box.add_child(std::move(center_panel));
        box.add_child(std::move(spr2));

        (void)box.measure(400, 100);

        test_canvas_backend::rect_type bounds;
        rect_utils::set_bounds(bounds, 0, 0, 400, 100);
        box.arrange(bounds);

        auto s1_bounds = spring1_ptr->bounds();
        auto p_bounds = panel_ptr->bounds();
        auto s2_bounds = spring2_ptr->bounds();

        // Total width: 400
        // Panel: 100
        // Remaining: 300, split equally: 150 each
        CHECK(rect_utils::get_width(s1_bounds) == 150);
        CHECK(rect_utils::get_width(p_bounds) == 100);
        CHECK(rect_utils::get_width(s2_bounds) == 150);

        // Positions: spring1 at 0, panel at 150, spring2 at 250
        CHECK(rect_utils::get_x(s1_bounds) == 0);
        CHECK(rect_utils::get_x(p_bounds) == 150);
        CHECK(rect_utils::get_x(s2_bounds) == 250);
    }

    SUBCASE("HBox - Weighted springs (2:1 ratio)") {
        hbox<test_canvas_backend> box(0);

        auto spr1 = std::make_unique<spring<test_canvas_backend>>(2.0F, true);  // 2x weight
        auto middle_panel = std::make_unique<panel<test_canvas_backend>>();
        auto spr2 = std::make_unique<spring<test_canvas_backend>>(1.0F, true);  // 1x weight

        // Fixed width panel
        size_constraint fixed_width;
        fixed_width.policy = size_policy::fixed;
        fixed_width.preferred_size = 100;
        fixed_width.min_size = 100;
        fixed_width.max_size = 100;
        middle_panel->set_width_constraint(fixed_width);

        auto* spring1_ptr = spr1.get();
        auto* spring2_ptr = spr2.get();

        box.add_child(std::move(spr1));
        box.add_child(std::move(middle_panel));
        box.add_child(std::move(spr2));

        (void)box.measure(400, 100);

        test_canvas_backend::rect_type bounds;
        rect_utils::set_bounds(bounds, 0, 0, 400, 100);
        box.arrange(bounds);

        auto s1_bounds = spring1_ptr->bounds();
        auto s2_bounds = spring2_ptr->bounds();

        // Total: 400, Panel: 100, Remaining: 300
        // Weights: 2:1, so spring1 gets 200, spring2 gets 100
        CHECK(rect_utils::get_width(s1_bounds) == 200);
        CHECK(rect_utils::get_width(s2_bounds) == 100);
    }

    SUBCASE("VBox - Vertical spring expands") {
        vbox<test_canvas_backend> box(0);

        auto panel1 = std::make_unique<panel<test_canvas_backend>>();
        auto spr = std::make_unique<spring<test_canvas_backend>>(1.0F, false);  // Vertical spring
        auto panel2 = std::make_unique<panel<test_canvas_backend>>();

        // Set fixed heights
        size_constraint fixed_height;
        fixed_height.policy = size_policy::fixed;
        fixed_height.preferred_size = 50;
        fixed_height.min_size = 50;
        fixed_height.max_size = 50;

        panel1->set_height_constraint(fixed_height);
        panel2->set_height_constraint(fixed_height);

        auto* p1 = panel1.get();
        auto* spring_ptr = spr.get();
        auto* p2 = panel2.get();

        box.add_child(std::move(panel1));
        box.add_child(std::move(spr));
        box.add_child(std::move(panel2));

        (void)box.measure(200, 400);

        test_canvas_backend::rect_type bounds;
        rect_utils::set_bounds(bounds, 0, 0, 200, 400);
        box.arrange(bounds);

        auto b1 = p1->bounds();
        auto spring_bounds = spring_ptr->bounds();
        auto b2 = p2->bounds();

        // panel1: y=0, h=50
        // spring: y=50, h=300 (fills remaining space)
        // panel2: y=350, h=50
        CHECK(rect_utils::get_y(b1) == 0);
        CHECK(rect_utils::get_height(b1) == 50);

        CHECK(rect_utils::get_y(spring_bounds) == 50);
        CHECK(rect_utils::get_height(spring_bounds) == 300);

        CHECK(rect_utils::get_y(b2) == 350);
        CHECK(rect_utils::get_height(b2) == 50);
    }
}

// ============================================================================
// COMPOSITION - NESTED LAYOUTS
// ============================================================================

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "Layout Correctness - Nested Layouts") {
    SUBCASE("VBox containing HBox - Dialog button row") {
        // Create a dialog layout: title + button row
        vbox<test_canvas_backend> dialog(10);  // 10px spacing

        auto title = std::make_unique<label<test_canvas_backend>>("Confirm Action");
        auto button_row = std::make_unique<hbox<test_canvas_backend>>(5);

        // Add buttons to hbox
        auto ok_btn = std::make_unique<button<test_canvas_backend>>("OK");
        auto cancel_btn = std::make_unique<button<test_canvas_backend>>("Cancel");

        button_row->add_child(std::move(ok_btn));
        button_row->add_child(std::move(cancel_btn));

        auto* button_row_ptr = button_row.get();

        dialog.add_child(std::move(title));
        dialog.add_child(std::move(button_row));

        // Measure and arrange
        (void)dialog.measure(300, 200);

        test_canvas_backend::rect_type bounds;
        rect_utils::set_bounds(bounds, 0, 0, 300, 200);
        dialog.arrange(bounds);

        // Verify button row is positioned below title + spacing
        auto button_row_bounds = button_row_ptr->bounds();
        CHECK(rect_utils::get_y(button_row_bounds) > 0);  // Should be below title

        // Verify button row has children arranged
        CHECK(button_row_ptr->children().size() == 2);
    }

    SUBCASE("HBox containing VBox - Toolbar groups") {
        // Create toolbar with vertical button groups
        hbox<test_canvas_backend> toolbar(20);  // 20px spacing between groups

        auto group1 = std::make_unique<vbox<test_canvas_backend>>(5);
        auto group2 = std::make_unique<vbox<test_canvas_backend>>(5);

        group1->add_child(std::make_unique<button<test_canvas_backend>>("New"));
        group1->add_child(std::make_unique<button<test_canvas_backend>>("Open"));

        group2->add_child(std::make_unique<button<test_canvas_backend>>("Cut"));
        group2->add_child(std::make_unique<button<test_canvas_backend>>("Copy"));

        auto* g1_ptr = group1.get();
        auto* g2_ptr = group2.get();

        toolbar.add_child(std::move(group1));
        toolbar.add_child(std::move(group2));

        (void)toolbar.measure(400, 100);

        test_canvas_backend::rect_type bounds;
        rect_utils::set_bounds(bounds, 0, 0, 400, 100);
        toolbar.arrange(bounds);

        auto g1_bounds = g1_ptr->bounds();
        auto g2_bounds = g2_ptr->bounds();

        // group2 should be positioned after group1 + spacing
        CHECK(rect_utils::get_x(g2_bounds) > rect_utils::get_x(g1_bounds));
    }

    SUBCASE("Three-level nesting - Complex form layout") {
        // Main vertical layout
        vbox<test_canvas_backend> form(15);

        // Header
        auto header = std::make_unique<label<test_canvas_backend>>("User Registration");

        // Form fields (vbox of hboxes)
        auto fields = std::make_unique<vbox<test_canvas_backend>>(10);

        // Name field row
        auto name_row = std::make_unique<hbox<test_canvas_backend>>(10);
        name_row->add_child(std::make_unique<label<test_canvas_backend>>("Name:"));
        name_row->add_child(std::make_unique<panel<test_canvas_backend>>());  // Input field
        fields->add_child(std::move(name_row));

        // Email field row
        auto email_row = std::make_unique<hbox<test_canvas_backend>>(10);
        email_row->add_child(std::make_unique<label<test_canvas_backend>>("Email:"));
        email_row->add_child(std::make_unique<panel<test_canvas_backend>>());  // Input field
        fields->add_child(std::move(email_row));

        // Button row
        auto button_row = std::make_unique<hbox<test_canvas_backend>>(10);
        button_row->add_child(std::make_unique<button<test_canvas_backend>>("Submit"));
        button_row->add_child(std::make_unique<button<test_canvas_backend>>("Cancel"));

        form.add_child(std::move(header));
        form.add_child(std::move(fields));
        form.add_child(std::move(button_row));

        // Should not crash with complex nesting
        CHECK_NOTHROW((void)form.measure(400, 300));

        test_canvas_backend::rect_type bounds;
        rect_utils::set_bounds(bounds, 0, 0, 400, 300);
        CHECK_NOTHROW(form.arrange(bounds));

        CHECK(form.children().size() == 3);
    }

    SUBCASE("Toolbar with springs - Push to edges") {
        hbox<test_canvas_backend> toolbar(0);

        auto left_btn = std::make_unique<button<test_canvas_backend>>("Menu");
        auto spr = std::make_unique<spring<test_canvas_backend>>(1.0F, true);
        auto right_btn = std::make_unique<button<test_canvas_backend>>("Help");

        // Fixed size buttons
        size_constraint fixed_width;
        fixed_width.policy = size_policy::fixed;
        fixed_width.preferred_size = 80;
        fixed_width.min_size = 80;
        fixed_width.max_size = 80;

        left_btn->set_width_constraint(fixed_width);
        right_btn->set_width_constraint(fixed_width);

        auto* left_ptr = left_btn.get();
        auto* right_ptr = right_btn.get();

        toolbar.add_child(std::move(left_btn));
        toolbar.add_child(std::move(spr));
        toolbar.add_child(std::move(right_btn));

        (void)toolbar.measure(400, 50);

        test_canvas_backend::rect_type bounds;
        rect_utils::set_bounds(bounds, 0, 0, 400, 50);
        toolbar.arrange(bounds);

        auto left_bounds = left_ptr->bounds();
        auto right_bounds = right_ptr->bounds();

        // Left button at 0
        CHECK(rect_utils::get_x(left_bounds) == 0);

        // Right button pushed to right edge
        CHECK(rect_utils::get_x(right_bounds) == 320);  // 400 - 80
    }

    SUBCASE("Centered content with springs") {
        hbox<test_canvas_backend> container(0);

        auto left_spring = std::make_unique<spring<test_canvas_backend>>(1.0F, true);
        auto content = std::make_unique<panel<test_canvas_backend>>();
        auto right_spring = std::make_unique<spring<test_canvas_backend>>(1.0F, true);

        // Fixed size content
        size_constraint fixed_width;
        fixed_width.policy = size_policy::fixed;
        fixed_width.preferred_size = 200;
        fixed_width.min_size = 200;
        fixed_width.max_size = 200;
        content->set_width_constraint(fixed_width);

        auto* content_ptr = content.get();

        container.add_child(std::move(left_spring));
        container.add_child(std::move(content));
        container.add_child(std::move(right_spring));

        (void)container.measure(600, 100);

        test_canvas_backend::rect_type bounds;
        rect_utils::set_bounds(bounds, 0, 0, 600, 100);
        container.arrange(bounds);

        auto content_bounds = content_ptr->bounds();

        // Content should be centered: (600 - 200) / 2 = 200
        CHECK(rect_utils::get_x(content_bounds) == 200);
        CHECK(rect_utils::get_width(content_bounds) == 200);
    }
}

// ============================================================================
// REAL-WORLD LAYOUT SCENARIOS
// ============================================================================

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "Layout Correctness - Real-World Scenarios") {
    SUBCASE("Application window - Header/Content/Footer") {
        vbox<test_canvas_backend> window(0);

        // Fixed-size header
        auto header = std::make_unique<panel<test_canvas_backend>>();
        size_constraint header_height;
        header_height.policy = size_policy::fixed;
        header_height.preferred_size = 60;
        header_height.min_size = 60;
        header_height.max_size = 60;
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
        footer_height.preferred_size = 40;
        footer_height.min_size = 40;
        footer_height.max_size = 40;
        footer->set_height_constraint(footer_height);

        auto* header_ptr = header.get();
        auto* content_ptr = content.get();
        auto* footer_ptr = footer.get();

        window.add_child(std::move(header));
        window.add_child(std::move(content));
        window.add_child(std::move(footer));

        (void)window.measure(800, 600);

        test_canvas_backend::rect_type bounds;
        rect_utils::set_bounds(bounds, 0, 0, 800, 600);
        window.arrange(bounds);

        auto h_bounds = header_ptr->bounds();
        auto c_bounds = content_ptr->bounds();
        auto f_bounds = footer_ptr->bounds();

        // Verify positions
        CHECK(rect_utils::get_y(h_bounds) == 0);
        CHECK(rect_utils::get_height(h_bounds) == 60);

        CHECK(rect_utils::get_y(c_bounds) == 60);
        CHECK(rect_utils::get_height(c_bounds) == 500);  // 600 - 60 - 40

        CHECK(rect_utils::get_y(f_bounds) == 560);
        CHECK(rect_utils::get_height(f_bounds) == 40);
    }

    SUBCASE("Split view with weighted panels") {
        hbox<test_canvas_backend> split(0);

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

        (void)split.measure(900, 600);

        test_canvas_backend::rect_type bounds;
        rect_utils::set_bounds(bounds, 0, 0, 900, 600);
        split.arrange(bounds);

        auto l_bounds = left_ptr->bounds();
        auto r_bounds = right_ptr->bounds();

        // Left: 1/3 = 300, Right: 2/3 = 600
        CHECK(rect_utils::get_width(l_bounds) == 300);
        CHECK(rect_utils::get_width(r_bounds) == 600);
        CHECK(rect_utils::get_x(r_bounds) == 300);
    }

    SUBCASE("Dashboard with mixed spacers and springs") {
        vbox<test_canvas_backend> dashboard(10);

        // Title bar
        auto title = std::make_unique<label<test_canvas_backend>>("Dashboard");

        // Fixed spacer
        auto spacer1 = std::make_unique<spacer<test_canvas_backend>>(0, 20);

        // Content panel
        auto content = std::make_unique<panel<test_canvas_backend>>();

        // Flexible spring
        auto spring1 = std::make_unique<spring<test_canvas_backend>>(1.0F, false);

        // Bottom buttons
        auto buttons = std::make_unique<hbox<test_canvas_backend>>(10);
        buttons->add_child(std::make_unique<button<test_canvas_backend>>("Refresh"));
        buttons->add_child(std::make_unique<button<test_canvas_backend>>("Settings"));

        dashboard.add_child(std::move(title));
        dashboard.add_child(std::move(spacer1));
        dashboard.add_child(std::move(content));
        dashboard.add_child(std::move(spring1));
        dashboard.add_child(std::move(buttons));

        // Should handle mix of spacers and springs
        CHECK_NOTHROW((void)dashboard.measure(600, 800));

        test_canvas_backend::rect_type bounds;
        rect_utils::set_bounds(bounds, 0, 0, 600, 800);
        CHECK_NOTHROW(dashboard.arrange(bounds));
    }
}
