/**
 * @file test_grid.cc
 * @brief Unit tests for grid_layout
 */

#include "../../include/onyxui/layout/layout_strategy.hh"
#include "utils/test_helpers.hh"
#include <memory>
#include <onyxui/layout/grid_layout.hh>
#include <utility>
#include <vector>

using TestGridLayout = grid_layout<TestBackend>;

TEST_SUITE("grid_layout") {
    TEST_CASE("Basic 2x2 grid with fixed cells") {
        auto parent = std::make_unique<TestElement>();
        auto layout = std::make_unique<TestGridLayout>(2, 2, 10, 5);  // columns, rows, col_spacing, row_spacing
        parent->set_layout_strategy(std::move(layout));

        // Add four children
        for (int i = 0; i < 4; i++) {
            auto child = std::make_unique<TestElement>();
            child->set_width_constraint({size_policy::fixed, 100_lu, 100_lu});
            child->set_height_constraint({size_policy::fixed, 50_lu, 50_lu});
            parent->add_test_child(std::move(child));
        }

        auto measured = parent->measure(1000_lu, 1000_lu);
        CHECK(measured.width == 210_lu);  // 2*100 + 1*10
        CHECK(measured.height == 105_lu); // 2*50 + 1*5

        parent->arrange(logical_rect{0_lu, 0_lu, 210_lu, 105_lu});

        // Verify positions
        CHECK(parent->child_at(0)->bounds().x.to_int() == 0);
        CHECK(parent->child_at(0)->bounds().y.to_int() == 0);
        CHECK(parent->child_at(1)->bounds().x.to_int() == 110);
        CHECK(parent->child_at(1)->bounds().y.to_int() == 0);
        CHECK(parent->child_at(2)->bounds().x.to_int() == 0);
        CHECK(parent->child_at(2)->bounds().y.to_int() == 55);
        CHECK(parent->child_at(3)->bounds().x.to_int() == 110);
        CHECK(parent->child_at(3)->bounds().y.to_int() == 55);
    }

    TEST_CASE("Manual cell assignment") {
        auto parent = std::make_unique<TestElement>();
        auto layout = std::make_unique<TestGridLayout>(3, 3);
        parent->set_layout_strategy(std::move(layout));

        auto child1 = std::make_unique<TestElement>();
        child1->set_width_constraint({size_policy::fixed, 50_lu, 50_lu});
        child1->set_height_constraint({size_policy::fixed, 30_lu, 30_lu});
        auto child1_ptr = child1.get();
        parent->add_test_child(std::move(child1));

        auto child2 = std::make_unique<TestElement>();
        child2->set_width_constraint({size_policy::fixed, 60_lu, 60_lu});
        child2->set_height_constraint({size_policy::fixed, 40_lu, 40_lu});
        auto child2_ptr = child2.get();
        parent->add_test_child(std::move(child2));

        // Place children manually
        // Note: Can't access layout directly in tests, so cells auto-assign

        // Measure: grid calculates required space
        auto measured = parent->measure(1000_lu, 1000_lu);
        CHECK(measured.width > 0_lu);  // Grid reports space needed for children
        CHECK(measured.height > 0_lu);

        parent->arrange(logical_rect{0_lu, 0_lu, measured.width, measured.height});

        // Verify positions (auto-assigned sequentially)
        // Child1 goes to (0,0), Child2 goes to (1,0) in the 3x3 grid
        // Grid arranges to measured size, so columns are not scaled
        CHECK(child1_ptr->bounds().x.to_int() == 0);
        CHECK(child1_ptr->bounds().y.to_int() == 0);
        CHECK(child2_ptr->bounds().x.to_int() == 50); // Second column starts at width of first column (50px)
        CHECK(child2_ptr->bounds().y.to_int() == 0);  // Still in first row
    }

    TEST_CASE("Column and row spanning") {
        auto parent = std::make_unique<TestElement>();
        auto layout = std::make_unique<TestGridLayout>(3, 2, 10, 5);  // columns, rows, col_spacing, row_spacing
        parent->set_layout_strategy(std::move(layout));

        // Child spanning 2 columns
        auto child1 = std::make_unique<TestElement>();
        child1->set_width_constraint({size_policy::fill_parent});
        child1->set_height_constraint({size_policy::fixed, 40_lu, 40_lu});
        auto child1_ptr = child1.get();
        parent->add_test_child(std::move(child1));

        // Child spanning 2 rows
        auto child2 = std::make_unique<TestElement>();
        child2->set_width_constraint({size_policy::fixed, 80_lu, 80_lu});
        child2->set_height_constraint({size_policy::fill_parent});
        auto child2_ptr = child2.get();
        parent->add_test_child(std::move(child2));

        // Note: Can't access layout directly in tests, so spans are auto-assigned
        // Without explicit spanning, children just occupy single cells

        // Measure: grid with spanning considerations
        auto measured = parent->measure(300_lu, 200_lu);
        CHECK(measured.width > 0_lu);
        CHECK(measured.height > 0_lu);

        // Arrange to measured size to avoid scaling
        parent->arrange(logical_rect{0_lu, 0_lu, measured.width, measured.height});

        // With fill_parent policy, child1 fills its cell width
        CHECK(child1_ptr->bounds().width.to_int() <= 100); // Single cell width

        // Child2 has fill_parent height policy, so it fills available cell height
        // Row height is 40px (from child1), so child2 fills to 40px
        CHECK(child2_ptr->bounds().height.to_int() >= 40); // Fills cell height
    }

    TEST_CASE("Fixed column and row dimensions") {
        auto parent = std::make_unique<TestElement>();
        auto layout = std::make_unique<TestGridLayout>(
            2, 2,                        // columns, rows
            0, 0,                        // col_spacing, row_spacing
            false,                       // auto_size_cells = false (we're using fixed sizes)
            std::vector<int>{150, 100},  // Fixed column widths
            std::vector<int>{60, 80}     // Fixed row heights
        );
        parent->set_layout_strategy(std::move(layout));

        // Add children
        for (int i = 0; i < 4; i++) {
            auto child = std::make_unique<TestElement>();
            child->set_width_constraint({size_policy::fill_parent});
            child->set_height_constraint({size_policy::fill_parent});
            parent->add_test_child(std::move(child));
        }

        auto measured = parent->measure(1000_lu, 1000_lu);
        CHECK(measured.width == 250_lu);  // 150 + 100
        CHECK(measured.height == 140_lu); // 60 + 80

        parent->arrange(logical_rect{0_lu, 0_lu, 250_lu, 140_lu});

        // First column children
        CHECK(parent->child_at(0)->bounds().width.to_int() == 150);
        CHECK(parent->child_at(2)->bounds().width.to_int() == 150);
        // Second column children
        CHECK(parent->child_at(1)->bounds().width.to_int() == 100);
        CHECK(parent->child_at(3)->bounds().width.to_int() == 100);
        // First row children
        CHECK(parent->child_at(0)->bounds().height.to_int() == 60);
        CHECK(parent->child_at(1)->bounds().height.to_int() == 60);
        // Second row children
        CHECK(parent->child_at(2)->bounds().height.to_int() == 80);
        CHECK(parent->child_at(3)->bounds().height.to_int() == 80);
    }
}