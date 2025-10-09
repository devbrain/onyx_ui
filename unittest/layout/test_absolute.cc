/**
 * @file test_absolute.cc
 * @brief Unit tests for absolute_layout
 */

#include "utils/test_helpers.hh"
#include <onyxui/layout/absolute_layout.hh>

using TestAbsoluteLayout = absolute_layout<TestRect, TestSize>;

TEST_SUITE("absolute_layout") {
    TEST_CASE("Basic positioning") {
        auto parent = std::make_unique<TestElement>();
        auto layout = std::make_unique<TestAbsoluteLayout>();
        auto layout_ptr = layout.get();
        parent->set_layout_strategy(std::move(layout));

        // Add children at specific positions
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

        // Set positions
        layout_ptr->set_position(child1_ptr, 10, 20);
        layout_ptr->set_position(child2_ptr, 100, 50);

        parent->measure(200, 150);
        parent->arrange({0, 0, 200, 150});

        // Verify positions
        CHECK(child1_ptr->bounds().x == 10);
        CHECK(child1_ptr->bounds().y == 20);
        CHECK(child1_ptr->bounds().width == 50);
        CHECK(child1_ptr->bounds().height == 30);

        CHECK(child2_ptr->bounds().x == 100);
        CHECK(child2_ptr->bounds().y == 50);
        CHECK(child2_ptr->bounds().width == 60);
        CHECK(child2_ptr->bounds().height == 40);
    }

    TEST_CASE("Position with size override") {
        auto parent = std::make_unique<TestElement>();
        auto layout = std::make_unique<TestAbsoluteLayout>();
        auto layout_ptr = layout.get();
        parent->set_layout_strategy(std::move(layout));

        auto child = std::make_unique<TestElement>();
        child->set_width_constraint({size_policy::fixed, 100, 100});
        child->set_height_constraint({size_policy::fixed, 50, 50});
        auto child_ptr = child.get();
        parent->add_test_child(std::move(child));

        // Set position with size override
        layout_ptr->set_position(child_ptr, 25, 35, 150, 75);

        parent->measure(300, 200);
        parent->arrange({0, 0, 300, 200});

        // Should use overridden size, not measured size
        CHECK(child_ptr->bounds().x == 25);
        CHECK(child_ptr->bounds().y == 35);
        CHECK(child_ptr->bounds().width == 150);
        CHECK(child_ptr->bounds().height == 75);
    }
}