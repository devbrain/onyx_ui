/**
 * @file test_absolute.cc
 * @brief Unit tests for absolute_layout
 */

#include "../../include/onyxui/layout/layout_strategy.hh"
#include "utils/test_helpers.hh"
#include <memory>
#include <onyxui/layout/absolute_layout.hh>
#include <utility>

using TestAbsoluteLayout = absolute_layout<TestBackend>;

TEST_SUITE("absolute_layout") {
    TEST_CASE("Basic positioning") {
        auto parent = std::make_unique<TestElement>();
        auto layout = std::make_unique<TestAbsoluteLayout>();
        auto layout_ptr = layout.get();
        parent->set_layout_strategy(std::move(layout));

        // Add children at specific positions
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

        // Set positions
        layout_ptr->set_position(child1_ptr, 10, 20);
        layout_ptr->set_position(child2_ptr, 100, 50);

        // Measure: absolute layout reports largest extent needed
        auto measured = parent->measure(200_lu, 150_lu);
        CHECK(measured.width <= 200_lu);
        CHECK(measured.height <= 150_lu);

        parent->arrange(logical_rect{0_lu, 0_lu, 200_lu, 150_lu});

        // Verify positions
        CHECK(child1_ptr->bounds().x.to_int() == 10);
        CHECK(child1_ptr->bounds().y.to_int() == 20);
        CHECK(child1_ptr->bounds().width.to_int() == 50);
        CHECK(child1_ptr->bounds().height.to_int() == 30);

        CHECK(child2_ptr->bounds().x.to_int() == 100);
        CHECK(child2_ptr->bounds().y.to_int() == 50);
        CHECK(child2_ptr->bounds().width.to_int() == 60);
        CHECK(child2_ptr->bounds().height.to_int() == 40);
    }

    TEST_CASE("Position with size override") {
        auto parent = std::make_unique<TestElement>();
        auto layout = std::make_unique<TestAbsoluteLayout>();
        auto layout_ptr = layout.get();
        parent->set_layout_strategy(std::move(layout));

        auto child = std::make_unique<TestElement>();
        child->set_width_constraint({size_policy::fixed, 100_lu, 100_lu});
        child->set_height_constraint({size_policy::fixed, 50_lu, 50_lu});
        auto child_ptr = child.get();
        parent->add_test_child(std::move(child));

        // Set position with size override
        layout_ptr->set_position(child_ptr, 25, 35, 150, 75);

        // Measure phase
        auto measured = parent->measure(300_lu, 200_lu);
        CHECK(measured.width <= 300_lu);
        CHECK(measured.height <= 200_lu);

        parent->arrange(logical_rect{0_lu, 0_lu, 300_lu, 200_lu});

        // Should use overridden size, not measured size
        CHECK(child_ptr->bounds().x.to_int() == 25);
        CHECK(child_ptr->bounds().y.to_int() == 35);
        CHECK(child_ptr->bounds().width.to_int() == 150);
        CHECK(child_ptr->bounds().height.to_int() == 75);
    }
}