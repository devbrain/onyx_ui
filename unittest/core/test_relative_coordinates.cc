/**
 * @file test_relative_coordinates.cc
 * @brief Comprehensive tests for relative coordinate system
 * @author Testing Team
 * @date 2025-11-03
 *
 * @details
 * Tests the relative coordinate system introduced in Phase 1-8 of the
 * relative coordinates refactoring. These tests ensure that:
 * - Children store relative bounds (0,0 origin)
 * - Hit testing works correctly with nested layouts
 * - Dirty regions are converted to absolute coordinates
 * - Rendering produces correct visual output
 */

#include <doctest/doctest.h>
#include "../utils/test_helpers.hh"
#include <onyxui/widgets/containers/panel.hh>
#include <onyxui/widgets/button.hh>
#include <onyxui/widgets/label.hh>
#include <onyxui/services/ui_context.hh>

using namespace onyxui;
using namespace onyxui::testing;

TEST_SUITE("Relative Coordinates - Core System") {
    using Backend = test_canvas_backend;
    using ui_context_fixture_type = ui_context_fixture<Backend>;

    TEST_CASE_FIXTURE(ui_context_fixture_type, "Children have relative bounds, not absolute") {
        panel<Backend> root;
        root.set_vbox_layout(0);

        auto* child = root.emplace_child<label>("Child");

        root.measure(200, 200);
        root.arrange({0, 0, 200, 200});

        // Child should be at (0,0) relative to root's content area
        auto child_bounds = child->bounds();
        CHECK(rect_utils::get_x(child_bounds) == 0);
        CHECK(rect_utils::get_y(child_bounds) == 0);
    }

    TEST_CASE_FIXTURE(ui_context_fixture_type, "Nested containers - children always at relative (0,0)") {
        // Create 3-level hierarchy: root -> middle_panel -> inner_label
        panel<Backend> root;
        root.set_vbox_layout(0);

        auto* middle_panel = root.emplace_child<panel>();
        middle_panel->set_vbox_layout(0);
        middle_panel->set_padding(thickness::all(10));  // Offset inner child

        auto* inner_label = middle_panel->emplace_child<label>("Inner");

        root.measure(200, 200);
        root.arrange({0, 0, 200, 200});

        // Middle panel at (0,0) relative to root
        auto middle_bounds = middle_panel->bounds();
        CHECK(rect_utils::get_x(middle_bounds) == 0);
        CHECK(rect_utils::get_y(middle_bounds) == 0);

        // Inner label at (0,0) relative to middle panel's content area
        auto inner_bounds = inner_label->bounds();
        CHECK(rect_utils::get_x(inner_bounds) == 0);
        CHECK(rect_utils::get_y(inner_bounds) == 0);
    }

    TEST_CASE_FIXTURE(ui_context_fixture_type, "Hit testing with nested layouts - absolute coordinates") {
        // Create nested layout with padding to offset child
        panel<Backend> root;
        root.set_vbox_layout(0);
        root.set_padding(thickness::all(2));  // Add padding to root so (1,1) hits root's padding area

        auto* parent_panel = root.emplace_child<panel>();
        parent_panel->set_vbox_layout(0);
        parent_panel->set_padding(thickness::all(10));  // Child at absolute (12,12) = root padding 2 + parent padding 10

        auto* child_button = parent_panel->emplace_child<button>("Click");

        root.measure(200, 200);
        root.arrange({0, 0, 200, 200});

        // Child button is at relative (0,0) but absolute (12,12) on screen
        auto child_bounds = child_button->bounds();
        INFO("Child bounds (relative): (",
             rect_utils::get_x(child_bounds), ",",
             rect_utils::get_y(child_bounds), ")");

        CHECK(rect_utils::get_x(child_bounds) == 0);  // Relative
        CHECK(rect_utils::get_y(child_bounds) == 0);

        // Hit test at absolute position (15, 15) should hit child (at absolute 12,12)
        auto* hit_at_child = root.hit_test(15, 15);
        CHECK(hit_at_child == child_button);

        // Hit test at absolute position (5, 5) should hit parent (starts at absolute 2,2)
        auto* hit_at_parent = root.hit_test(5, 5);
        CHECK(hit_at_parent == parent_panel);

        // Hit test at absolute position (1, 1) should hit root (in root's padding area)
        auto* hit_at_root = root.hit_test(1, 1);
        CHECK(hit_at_root == &root);

        // Hit test outside all bounds
        auto* hit_outside = root.hit_test(250, 250);
        CHECK(hit_outside == nullptr);
    }

    TEST_CASE_FIXTURE(ui_context_fixture_type, "Hit testing with multiple nested levels") {
        // Create deep hierarchy with offsets at each level
        panel<Backend> root;
        root.set_vbox_layout(0);
        root.set_padding(thickness{1, 1, 0, 0});  // Add padding to root for hit testing gaps

        auto* level1 = root.emplace_child<panel>();
        level1->set_vbox_layout(0);
        level1->set_padding(thickness{5, 5, 0, 0});  // Offset by (5,5) from level1's content area

        auto* level2 = level1->emplace_child<panel>();
        level2->set_vbox_layout(0);
        level2->set_padding(thickness{10, 10, 0, 0});  // Offset by (10,10) from level2's content area

        auto* level3_label = level2->emplace_child<label>("Deep");

        root.measure(200, 200);
        root.arrange({0, 0, 200, 200});

        // level3_label should be at absolute (17, 17) = root(1) + level1(5) + level1_content + level2(10) + level2_content
        // Actually: root padding 1, level1 at (1,1), level1 padding 5, level2 at (6,6), level2 padding 10, label at (16,16)
        // But stored as relative (0, 0)
        auto label_bounds = level3_label->bounds();
        CHECK(rect_utils::get_x(label_bounds) == 0);
        CHECK(rect_utils::get_y(label_bounds) == 0);

        // Hit test at absolute (17, 16) should hit the deep label (at absolute 16,16 from 1+5+10)
        // Note: Using y=16 because label height is only 1 pixel, so y=17 would be out of bounds
        auto* hit = root.hit_test(17, 16);
        CHECK(hit == level3_label);

        // Hit test at intermediate positions
        // level2 starts at absolute (6, 6) = 1 (root padding) + 5 (level1 padding)
        auto* hit_level2 = root.hit_test(7, 7);
        CHECK(hit_level2 == level2);

        // level1 starts at absolute (1, 1) = 1 (root padding)
        auto* hit_level1 = root.hit_test(2, 2);
        CHECK(hit_level1 == level1);
    }

    TEST_CASE_FIXTURE(ui_context_fixture_type, "Dirty regions use absolute coordinates") {
        // Create nested layout
        panel<Backend> root;
        root.set_vbox_layout(0);

        auto* parent_panel = root.emplace_child<panel>();
        parent_panel->set_vbox_layout(0);
        parent_panel->set_padding(thickness::all(10));  // Child at absolute (10,10)

        auto* child_label = parent_panel->emplace_child<label>("Child");

        root.measure(200, 200);
        root.arrange({0, 0, 200, 200});

        // Clear any initial dirty regions
        (void)root.get_and_clear_dirty_regions();

        // Mark child as dirty
        child_label->mark_dirty();

        // Get dirty regions from root
        auto dirty_regions = root.get_and_clear_dirty_regions();

        REQUIRE(dirty_regions.size() >= 1);

        // Find the region corresponding to child_label
        bool found_child_region = false;
        for (const auto& region : dirty_regions) {
            // Child is at absolute (10, 10), so dirty region should reflect that
            int region_x = rect_utils::get_x(region);
            int region_y = rect_utils::get_y(region);

            INFO("Dirty region: (", region_x, ",", region_y, ") ",
                 rect_utils::get_width(region), "x", rect_utils::get_height(region));

            // Check if this region is at the child's absolute position
            if (region_x == 10 && region_y == 10) {
                found_child_region = true;
                break;
            }
        }

        CHECK(found_child_region);
    }

    TEST_CASE_FIXTURE(ui_context_fixture_type, "Dirty regions with deeply nested elements") {
        // Create 3-level hierarchy with padding at each level
        panel<Backend> root;
        root.set_vbox_layout(0);

        auto* level1 = root.emplace_child<panel>();
        level1->set_vbox_layout(0);
        level1->set_padding(thickness{5, 7, 0, 0});  // Offset by (5,7)

        auto* level2 = level1->emplace_child<panel>();
        level2->set_vbox_layout(0);
        level2->set_padding(thickness{10, 15, 0, 0});  // Offset by (10,15)

        auto* deep_label = level2->emplace_child<label>("Deep");

        root.measure(200, 200);
        root.arrange({0, 0, 200, 200});

        // Clear initial dirty regions
        (void)root.get_and_clear_dirty_regions();

        // Mark deep label as dirty
        deep_label->mark_dirty();

        // Get dirty regions from root
        auto dirty_regions = root.get_and_clear_dirty_regions();

        REQUIRE(dirty_regions.size() >= 1);

        // Deep label should be at absolute (15, 22) = (5+10, 7+15)
        bool found_at_correct_position = false;
        for (const auto& region : dirty_regions) {
            int region_x = rect_utils::get_x(region);
            int region_y = rect_utils::get_y(region);

            INFO("Checking dirty region: (", region_x, ",", region_y, ")");

            if (region_x == 15 && region_y == 22) {
                found_at_correct_position = true;
                break;
            }
        }

        CHECK(found_at_correct_position);
    }

    TEST_CASE_FIXTURE(ui_context_fixture_type, "Rendering produces correct visual output") {
        // Verify that widgets render at correct screen positions despite relative bounds
        panel<Backend> root;
        root.set_vbox_layout(0);

        auto* top_label = root.emplace_child<label>("Top");
        auto* middle_panel = root.emplace_child<panel>();
        middle_panel->set_vbox_layout(0);
        middle_panel->set_padding(thickness{2, 0, 0, 0});
        auto* nested_label = middle_panel->emplace_child<label>("Nested");

        root.measure(40, 10);
        root.arrange({0, 0, 40, 10});

        // Verify relative bounds
        CHECK(rect_utils::get_y(top_label->bounds()) == 0);
        CHECK(rect_utils::get_y(middle_panel->bounds()) == 1);  // After top_label
        CHECK(rect_utils::get_y(nested_label->bounds()) == 0);   // Relative to middle_panel

        // Render and verify visual output
        auto canvas = render_to_canvas(root, 40, 10);

        // "Top" should be at row 0
        std::string row0 = canvas->get_row(0);
        CHECK(row0.find("Top") != std::string::npos);

        // "Nested" should be at row 1 (middle_panel.y=1, nested_label.y=0 relative)
        std::string row1 = canvas->get_row(1);
        CHECK(row1.find("Nested") != std::string::npos);
    }

    TEST_CASE_FIXTURE(ui_context_fixture_type, "Borders and padding offset children correctly") {
        panel<Backend> root;
        root.set_vbox_layout(0);
        root.set_padding(thickness::all(1));  // Add padding to root for hit testing gaps

        auto* bordered_panel = root.emplace_child<panel>();
        bordered_panel->set_has_border(true);           // +1 pixel border
        bordered_panel->set_padding(thickness::all(2)); // +2 pixel padding
        bordered_panel->set_vbox_layout(0);

        auto* child = bordered_panel->emplace_child<label>("Child");

        root.measure(100, 100);
        root.arrange({0, 0, 100, 100});

        // Child should be at relative (0,0)
        auto child_bounds = child->bounds();
        CHECK(rect_utils::get_x(child_bounds) == 0);
        CHECK(rect_utils::get_y(child_bounds) == 0);

        // Hit testing should account for: root padding (1) + border (1) + padding (2) = 4 pixel offset
        // Child is at absolute (4, 4), label height is 1 pixel, so y goes from 4 to 4
        auto* hit_at_child = root.hit_test(5, 4);
        CHECK(hit_at_child == child);

        // Hit at (2, 2) should hit bordered_panel (starts at absolute 1,1), not child
        auto* hit_at_panel = root.hit_test(2, 2);
        CHECK(hit_at_panel == bordered_panel);
    }

    TEST_CASE_FIXTURE(ui_context_fixture_type, "Multiple children in linear layout") {
        // Test that multiple children in a vbox are positioned correctly
        panel<Backend> root;
        root.set_vbox_layout(5);  // 5 pixel spacing

        auto* child1 = root.emplace_child<label>("Child1");
        auto* child2 = root.emplace_child<label>("Child2");
        auto* child3 = root.emplace_child<label>("Child3");

        root.measure(100, 100);
        root.arrange({0, 0, 100, 100});

        // All children should have x=0 (relative to root)
        CHECK(rect_utils::get_x(child1->bounds()) == 0);
        CHECK(rect_utils::get_x(child2->bounds()) == 0);
        CHECK(rect_utils::get_x(child3->bounds()) == 0);

        // Y positions should account for spacing (relative coordinates)
        int y1 = rect_utils::get_y(child1->bounds());
        int y2 = rect_utils::get_y(child2->bounds());
        int y3 = rect_utils::get_y(child3->bounds());

        CHECK(y1 == 0);                      // First at top
        CHECK(y2 > y1);                      // Second below first
        CHECK(y3 > y2);                      // Third below second
        CHECK(y2 - y1 >= 5);                 // Spacing included
        CHECK(y3 - y2 >= 5);                 // Spacing included

        // Hit testing should work at all positions
        CHECK(root.hit_test(5, y1) == child1);
        CHECK(root.hit_test(5, y2) == child2);
        CHECK(root.hit_test(5, y3) == child3);
    }
}
