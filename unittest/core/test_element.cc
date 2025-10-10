/**
 * @file test_element.cc
 * @brief Unit tests for ui_element core functionality
 */

#include "utils/test_helpers.hh"
#include <onyxui/layout/linear_layout.hh>

TEST_SUITE("ui_element") {
    TEST_CASE("Basic construction and hierarchy") {
        auto root = std::make_unique<TestElement>();
        CHECK(root->is_visible());
        CHECK(root->is_enabled());

        auto child1 = std::make_unique<TestElement>();
        root->add_test_child(std::move(child1));

        auto child2 = std::make_unique<TestElement>();
        root->add_test_child(std::move(child2));

        CHECK(root->child_count() == 2);
    }

    TEST_CASE("Margin and padding") {
        auto element = std::make_unique<TestElement>();

        element->set_margin({10, 15, 10, 15});
        element->set_padding({5, 8, 5, 8});

        CHECK(element->margin().left == 10);
        CHECK(element->margin().top == 15);
        CHECK(element->margin().horizontal() == 20);
        CHECK(element->margin().vertical() == 30);

        CHECK(element->padding().left == 5);
        CHECK(element->padding().top == 8);
        CHECK(element->padding().horizontal() == 10);
        CHECK(element->padding().vertical() == 16);

        // With fixed size policy, the preferred_size is the final size (margin not added)
        element->set_width_constraint({size_policy::fixed, 100, 100});
        element->set_height_constraint({size_policy::fixed, 50, 50});

        TestSize measured = element->measure(1000, 1000);
        CHECK(measured.w == 100);  // Fixed size is exact
        CHECK(measured.h == 50);  // Fixed size is exact
    }

    TEST_CASE("Size constraints") {
        auto element = std::make_unique<TestElement>();

        // Test minimum constraint
        // With content policy and no content, size is 0, then clamped to min
        element->set_width_constraint({size_policy::content, 100, 50, 200});
        element->set_height_constraint({size_policy::content, 80, 40, 150});

        TestSize measured = element->measure(30, 20);
        CHECK(measured.w == 50);   // Clamped to min
        CHECK(measured.h == 40);  // Clamped to min

        // Test maximum constraint - content size is 0, so it gets clamped to min
        measured = element->measure(300, 250);
        CHECK(measured.w == 50);  // Content is 0, clamped to min
        CHECK(measured.h == 40); // Content is 0, clamped to min
    }

    TEST_CASE("Visibility changes") {
        auto element = std::make_unique<TestElement>();

        CHECK(element->is_visible());
        element->set_visible(false);
        CHECK(!element->is_visible());
        element->set_visible(true);
        CHECK(element->is_visible());
    }

    TEST_CASE("Z-order sorting") {
        auto parent = std::make_unique<TestElement>();

        auto child1 = std::make_unique<TestElement>();
        child1->set_z_order(2);
        parent->add_test_child(std::move(child1));

        auto child2 = std::make_unique<TestElement>();
        child2->set_z_order(1);
        parent->add_test_child(std::move(child2));

        auto child3 = std::make_unique<TestElement>();
        child3->set_z_order(3);
        parent->add_test_child(std::move(child3));

        parent->sort_children_by_z_index();

        // Should be sorted by z_index
        CHECK(parent->child_at(0)->z_order() == 1);
        CHECK(parent->child_at(1)->z_order() == 2);
        CHECK(parent->child_at(2)->z_order() == 3);
    }

    TEST_CASE("Hit testing") {
        auto parent = std::make_unique<TestElement>();
        parent->arrange({0, 0, 200, 150});

        auto child1 = std::make_unique<TestElement>();
        child1->arrange({10, 10, 50, 30});
        auto child1_ptr = child1.get();
        parent->add_test_child(std::move(child1));

        auto child2 = std::make_unique<TestElement>();
        child2->arrange({100, 50, 60, 40});
        auto child2_ptr = child2.get();
        parent->add_test_child(std::move(child2));

        // Test hits
        CHECK(parent->hit_test(5, 5) == parent.get());      // Parent only
        CHECK(parent->hit_test(20, 20) == child1_ptr);      // Child 1
        CHECK(parent->hit_test(120, 70) == child2_ptr);     // Child 2
        CHECK(parent->hit_test(250, 200) == nullptr);       // Outside

        // Test with visibility
        child1_ptr->set_visible(false);
        CHECK(parent->hit_test(20, 20) == parent.get());    // Hidden child ignored
    }

    TEST_CASE("Alignment settings") {
        auto element = std::make_unique<TestElement>();

        CHECK(element->horizontal_align() == horizontal_alignment::stretch);
        CHECK(element->vertical_align() == vertical_alignment::stretch);

        element->set_horizontal_align(horizontal_alignment::center);
        element->set_vertical_align(vertical_alignment::top);

        CHECK(element->horizontal_align() == horizontal_alignment::center);
        CHECK(element->vertical_align() == vertical_alignment::top);
    }

    TEST_CASE("Custom content size") {
        auto element = std::make_unique<ContentElement>(120, 80);

        TestSize measured = element->measure(1000, 1000);
        CHECK(measured.w == 120);
        CHECK(measured.h == 80);
    }

    TEST_CASE("Thickness operators") {
        thickness t1 = {10, 20, 30, 40};
        thickness t2 = {10, 20, 30, 40};
        thickness t3 = {10, 20, 30, 41};

        CHECK(t1 == t2);
        CHECK(t1 != t3);
        CHECK(!(t1 == t3));
        CHECK(!(t1 != t2));
    }
}