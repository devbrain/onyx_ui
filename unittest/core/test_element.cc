/**
 * @file test_element.cc
 * @brief Unit tests for ui_element core functionality
 */

#include "../../include/onyxui/layout/layout_strategy.hh"
#include "../../include/onyxui/core/types.hh"
#include "../../include/onyxui/core/geometry.hh"
#include "utils/test_backend.hh"
#include "utils/test_helpers.hh"
#include <memory>
#include <utility>

using namespace onyxui;
using testing::make_relative_rect;

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

        element->set_margin(logical_thickness{10_lu, 15_lu, 10_lu, 15_lu});
        element->set_padding(logical_thickness{5_lu, 8_lu, 5_lu, 8_lu});

        CHECK(element->margin().left == 10_lu);
        CHECK(element->margin().top == 15_lu);
        CHECK(element->margin().horizontal() == 20_lu);
        CHECK(element->margin().vertical() == 30_lu);

        CHECK(element->padding().left == 5_lu);
        CHECK(element->padding().top == 8_lu);
        CHECK(element->padding().horizontal() == 10_lu);
        CHECK(element->padding().vertical() == 16_lu);

        // With fixed size policy, the preferred_size is the final size (margin not added)
        element->set_width_constraint({size_policy::fixed, 100_lu, 100_lu});
        element->set_height_constraint({size_policy::fixed, 50_lu, 50_lu});

        logical_size const measured = element->measure(1000_lu, 1000_lu);
        CHECK(measured.width == 100_lu);  // Fixed size is exact
        CHECK(measured.height == 50_lu);  // Fixed size is exact
    }

    TEST_CASE("Size constraints") {
        auto element = std::make_unique<TestElement>();

        // Test minimum constraint
        // With content policy and no content, size is 0, then clamped to min
        element->set_width_constraint({size_policy::content, 100_lu, 50_lu, 200_lu});
        element->set_height_constraint({size_policy::content, 80_lu, 40_lu, 150_lu});

        logical_size measured = element->measure(30_lu, 20_lu);
        CHECK(measured.width == 50_lu);   // Clamped to min
        CHECK(measured.height == 40_lu);  // Clamped to min

        // Test maximum constraint - content size is 0, so it gets clamped to min
        measured = element->measure(300_lu, 250_lu);
        CHECK(measured.width == 50_lu);  // Content is 0, clamped to min
        CHECK(measured.height == 40_lu); // Content is 0, clamped to min
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
        parent->arrange(logical_rect{logical_unit(0.0), logical_unit(0.0), logical_unit(200.0), logical_unit(150.0)});

        auto child1 = std::make_unique<TestElement>();
        child1->arrange(logical_rect{logical_unit(10.0), logical_unit(10.0), logical_unit(50.0), logical_unit(30.0)});
        auto child1_ptr = child1.get();
        parent->add_test_child(std::move(child1));

        auto child2 = std::make_unique<TestElement>();
        child2->arrange(logical_rect{logical_unit(100.0), logical_unit(50.0), logical_unit(60.0), logical_unit(40.0)});
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

        logical_size const measured = element->measure(1000_lu, 1000_lu);
        CHECK(measured.width == 120_lu);
        CHECK(measured.height == 80_lu);
    }


    TEST_CASE("Deep hierarchy - recursion safety") {
        SUBCASE("Moderate depth (50 levels) - typical deep UI") {
            // Create a 50-level deep hierarchy
            auto root = std::make_unique<TestElement>();
            TestElement* current = root.get();

            for (int i = 0; i < 49; i++) {
                auto child = std::make_unique<TestElement>();
                TestElement* child_ptr = child.get();
                current->add_test_child(std::move(child));
                current = child_ptr;
            }

            // Test invalidate_measure (walks UP the tree)
            current->invalidate_measure();

            // Test measure/arrange (walks DOWN the tree)
            auto size = root->measure(1000_lu, 1000_lu);
            CHECK(size.width >= 0_lu);
            CHECK(size.height >= 0_lu);
            root->arrange(logical_rect{logical_unit(0.0), logical_unit(0.0), logical_unit(1000.0), logical_unit(1000.0)});
            CHECK(root->bounds().width.to_int() == 1000);

            // Test hit_test (walks DOWN the tree)
            auto* hit = root->hit_test(500, 500);
            CHECK(hit != nullptr);
        }

        SUBCASE("Reasonable depth (100 levels) - stress test") {
            // Create a 100-level deep hierarchy
            auto root = std::make_unique<TestElement>();
            TestElement* current = root.get();

            for (int i = 0; i < 99; i++) {
                auto child = std::make_unique<TestElement>();
                TestElement* child_ptr = child.get();
                current->add_test_child(std::move(child));
                current = child_ptr;
            }

            // All operations should work without stack overflow
            current->invalidate_measure();
            auto size = root->measure(1000_lu, 1000_lu);
            root->arrange(logical_rect{logical_unit(0.0), logical_unit(0.0), logical_unit(1000.0), logical_unit(1000.0)});
            auto* hit = root->hit_test(500, 500);

            CHECK(size.width >= 0_lu);
            CHECK(hit != nullptr);
        }

        SUBCASE("Wide tree (100 children at one level)") {
            // Test wide tree instead of deep
            auto root = std::make_unique<TestElement>();

            for (int i = 0; i < 100; i++) {
                auto child = std::make_unique<TestElement>();
                root->add_test_child(std::move(child));
            }

            CHECK(root->child_count() == 100);

            // Should handle invalidation efficiently
            root->invalidate_measure();
            auto size = root->measure(1000_lu, 1000_lu);
            root->arrange(logical_rect{logical_unit(0.0), logical_unit(0.0), logical_unit(1000.0), logical_unit(1000.0)});

            // Hit testing should work
            auto* hit = root->hit_test(500, 500);
            CHECK(size.width >= 0_lu);
            CHECK(hit != nullptr);
        }
    }
}
