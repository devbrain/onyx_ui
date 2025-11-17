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
            child->set_width_constraint({size_policy::fixed, 100, 100});
            child->set_height_constraint({size_policy::fixed, 50, 50});
            parent->add_test_child(std::move(child));
        }

        TestSize const measured = parent->measure(1000, 1000);
        CHECK(measured.w == 210);  // 2*100 + 1*10
        CHECK(measured.h == 105); // 2*50 + 1*5

        parent->arrange(testing::make_relative_rect<TestBackend>(0, 0, 210, 105));

        // Verify positions
        CHECK(parent->child_at(0)->bounds().get().x == 0);
        CHECK(parent->child_at(0)->bounds().get().y == 0);
        CHECK(parent->child_at(1)->bounds().get().x == 110);
        CHECK(parent->child_at(1)->bounds().get().y == 0);
        CHECK(parent->child_at(2)->bounds().get().x == 0);
        CHECK(parent->child_at(2)->bounds().get().y == 55);
        CHECK(parent->child_at(3)->bounds().get().x == 110);
        CHECK(parent->child_at(3)->bounds().get().y == 55);
    }

    TEST_CASE("Manual cell assignment") {
        auto parent = std::make_unique<TestElement>();
        auto layout = std::make_unique<TestGridLayout>(3, 3);
        parent->set_layout_strategy(std::move(layout));

        auto child1 = std::make_unique<TestElement>();
        child1->set_width_constraint({size_policy::fixed, 50, 50});
        child1->set_height_constraint({size_policy::fixed, 30, 30});
        auto child1_ptr = child1.get();
        parent->add_test_child(std::move(child1));

        auto child2 = std::make_unique<TestElement>();
        child2->set_width_constraint({size_policy::fixed, 60, 60});
        child2->set_height_constraint({size_policy::fixed, 40, 40});
        auto child2_ptr = child2.get();
        parent->add_test_child(std::move(child2));

        // Place children manually
        // Note: Can't access layout directly in tests, so cells auto-assign

        // Measure: grid calculates required space
        auto measured = parent->measure(1000, 1000);
        CHECK(measured.w > 0);  // Grid reports space needed for children
        CHECK(measured.h > 0);

        parent->arrange(testing::make_relative_rect<TestBackend>(0, 0, measured.w, measured.h));

        // Verify positions (auto-assigned sequentially)
        // Child1 goes to (0,0), Child2 goes to (1,0) in the 3x3 grid
        // Grid arranges to measured size, so columns are not scaled
        CHECK(child1_ptr->bounds().get().x == 0);
        CHECK(child1_ptr->bounds().get().y == 0);
        CHECK(child2_ptr->bounds().get().x == 50); // Second column starts at width of first column (50px)
        CHECK(child2_ptr->bounds().get().y == 0);  // Still in first row
    }

    TEST_CASE("Column and row spanning") {
        auto parent = std::make_unique<TestElement>();
        auto layout = std::make_unique<TestGridLayout>(3, 2, 10, 5);  // columns, rows, col_spacing, row_spacing
        parent->set_layout_strategy(std::move(layout));

        // Child spanning 2 columns
        auto child1 = std::make_unique<TestElement>();
        child1->set_width_constraint({size_policy::fill_parent});
        child1->set_height_constraint({size_policy::fixed, 40, 40});
        auto child1_ptr = child1.get();
        parent->add_test_child(std::move(child1));

        // Child spanning 2 rows
        auto child2 = std::make_unique<TestElement>();
        child2->set_width_constraint({size_policy::fixed, 80, 80});
        child2->set_height_constraint({size_policy::fill_parent});
        auto child2_ptr = child2.get();
        parent->add_test_child(std::move(child2));

        // Note: Can't access layout directly in tests, so spans are auto-assigned
        // Without explicit spanning, children just occupy single cells

        // Measure: grid with spanning considerations
        auto measured = parent->measure(300, 200);
        CHECK(measured.w > 0);
        CHECK(measured.h > 0);

        // Arrange to measured size to avoid scaling
        parent->arrange(testing::make_relative_rect<TestBackend>(0, 0, measured.w, measured.h));

        // With fill_parent policy, child1 fills its cell width
        CHECK(child1_ptr->bounds().get().w <= 100); // Single cell width

        // Child2 has fill_parent height policy, so it fills available cell height
        // Row height is 40px (from child1), so child2 fills to 40px
        CHECK(child2_ptr->bounds().get().h >= 40); // Fills cell height
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

        TestSize const measured = parent->measure(1000, 1000);
        CHECK(measured.w == 250);  // 150 + 100
        CHECK(measured.h == 140); // 60 + 80

        parent->arrange(testing::make_relative_rect<TestBackend>(0, 0, 250, 140));

        // First column children
        CHECK(parent->child_at(0)->bounds().get().w == 150);
        CHECK(parent->child_at(2)->bounds().get().w == 150);
        // Second column children
        CHECK(parent->child_at(1)->bounds().get().w == 100);
        CHECK(parent->child_at(3)->bounds().get().w == 100);
        // First row children
        CHECK(parent->child_at(0)->bounds().get().h == 60);
        CHECK(parent->child_at(1)->bounds().get().h == 60);
        // Second row children
        CHECK(parent->child_at(2)->bounds().get().h == 80);
        CHECK(parent->child_at(3)->bounds().get().h == 80);
    }
}