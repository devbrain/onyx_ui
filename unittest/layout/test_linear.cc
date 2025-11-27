/**
 * @file test_linear.cc
 * @brief Unit tests for linear_layout
 */

#include "../../include/onyxui/layout/layout_strategy.hh"
#include "utils/test_helpers.hh"
#include <memory>
#include <limits>
#include <onyxui/layout/linear_layout.hh>
#include <utility>

using TestLinearLayout = linear_layout<TestBackend>;

TEST_SUITE("linear_layout") {
    TEST_CASE("Horizontal layout with fixed size children") {
        auto parent = std::make_unique<TestElement>();

        // Create layout with horizontal direction and spacing
        auto layout = std::make_unique<TestLinearLayout>(
            direction::horizontal, 10);
        parent->set_layout_strategy(std::move(layout));

        // Add three children with fixed sizes
        for (int i = 0; i < 3; i++) {
            auto child = std::make_unique<TestElement>();
            child->set_width_constraint({size_policy::fixed, 100_lu, 100_lu});
            child->set_height_constraint({size_policy::fixed, 50_lu, 50_lu});
            parent->add_test_child(std::move(child));
        }

        // Measure and arrange
        auto measured = parent->measure(1000_lu, 1000_lu);
        CHECK(measured.width == 320_lu); // 3*100 + 2*10
        CHECK(measured.height == 50_lu);

        parent->arrange(logical_rect{0_lu, 0_lu, 320_lu, 50_lu});

        // Verify child positions
        CHECK(parent->child_at(0)->bounds().x.to_int() == 0);
        CHECK(parent->child_at(0)->bounds().width.to_int() == 100);
        CHECK(parent->child_at(1)->bounds().x.to_int() == 110);
        CHECK(parent->child_at(1)->bounds().width.to_int() == 100);
        CHECK(parent->child_at(2)->bounds().x.to_int() == 220);
        CHECK(parent->child_at(2)->bounds().width.to_int() == 100);
    }

    TEST_CASE("Vertical layout with expand children") {
        auto parent = std::make_unique<TestElement>();

        auto layout = std::make_unique<TestLinearLayout>(
            direction::vertical, 5);
        parent->set_layout_strategy(std::move(layout));

        // Add two expanding children
        for (int i = 0; i < 2; i++) {
            auto child = std::make_unique<TestElement>();
            child->set_width_constraint({size_policy::fill_parent});
            child->set_height_constraint({size_policy::expand, 0_lu, 0_lu, 10000_lu, 1.0F});
            parent->add_test_child(std::move(child));
        }

        // Measure and arrange
        auto measured = parent->measure(200_lu, 300_lu);
        // For vertical layout with no fixed-width children, width should be max of child widths
        // Since children have fill_parent policy, they should measure to 0 initially
        CHECK(measured.width == 0_lu);    // Children with fill_parent don't contribute to measure
        CHECK(measured.height == 5_lu);   // Just the spacing between 2 children
        parent->arrange(logical_rect{0_lu, 0_lu, 200_lu, 300_lu});

        // Verify children split space equally (300 pixels total, 5 pixels spacing)
        // Available space = 300 - 5 = 295, divided by 2 = 147.5 each
        // First child gets the extra pixel from rounding (standard behavior in CSS flexbox)
        CHECK(parent->child_at(0)->bounds().height.to_int() == 148); // Gets the extra pixel from rounding
        CHECK(parent->child_at(0)->bounds().y.to_int() == 0);
        CHECK(parent->child_at(1)->bounds().height.to_int() == 147); // Floor of 295/2
        CHECK(parent->child_at(1)->bounds().y.to_int() == 153);      // 148 + 5 spacing
    }

    TEST_CASE("Weighted distribution") {
        auto parent = std::make_unique<TestElement>();

        auto layout = std::make_unique<TestLinearLayout>(direction::horizontal, 0);
        parent->set_layout_strategy(std::move(layout));

        // Add children with weights 1:2:1
        auto child1 = std::make_unique<TestElement>();
        child1->set_width_constraint({size_policy::weighted, 0_lu, 0_lu, 10000_lu, 1.0F});
        parent->add_test_child(std::move(child1));

        auto child2 = std::make_unique<TestElement>();
        child2->set_width_constraint({size_policy::weighted, 0_lu, 0_lu, 10000_lu, 2.0F});
        parent->add_test_child(std::move(child2));

        auto child3 = std::make_unique<TestElement>();
        child3->set_width_constraint({size_policy::weighted, 0_lu, 0_lu, 10000_lu, 1.0F});
        parent->add_test_child(std::move(child3));

        // Measure: weighted children report minimal size during measure
        auto measured = parent->measure(400_lu, 100_lu);
        CHECK(measured.width == 0_lu);  // Weighted children contribute 0 to measure
        CHECK(measured.height == 0_lu);

        parent->arrange(logical_rect{0_lu, 0_lu, 400_lu, 100_lu});

        CHECK(parent->child_at(0)->bounds().width.to_int() == 100); // 1/4 of 400
        CHECK(parent->child_at(1)->bounds().width.to_int() == 200); // 2/4 of 400
        CHECK(parent->child_at(2)->bounds().width.to_int() == 100); // 1/4 of 400
    }

    TEST_CASE("Alignment in cross axis") {
        auto parent = std::make_unique<TestElement>();

        auto layout = std::make_unique<TestLinearLayout>(direction::horizontal, 0);
        parent->set_layout_strategy(std::move(layout));

        // Add children with different vertical alignments
        auto child1 = std::make_unique<TestElement>();
        child1->set_width_constraint({size_policy::fixed, 50_lu, 50_lu});
        child1->set_height_constraint({size_policy::fixed, 30_lu, 30_lu});
        child1->set_vertical_align(vertical_alignment::top);
        parent->add_test_child(std::move(child1));

        auto child2 = std::make_unique<TestElement>();
        child2->set_width_constraint({size_policy::fixed, 50_lu, 50_lu});
        child2->set_height_constraint({size_policy::fixed, 30_lu, 30_lu});
        child2->set_vertical_align(vertical_alignment::center);
        parent->add_test_child(std::move(child2));

        auto child3 = std::make_unique<TestElement>();
        child3->set_width_constraint({size_policy::fixed, 50_lu, 50_lu});
        child3->set_height_constraint({size_policy::fixed, 30_lu, 30_lu});
        child3->set_vertical_align(vertical_alignment::bottom);
        parent->add_test_child(std::move(child3));

        // Measure: horizontal layout with fixed children
        auto measured = parent->measure(200_lu, 100_lu);
        CHECK(measured.width == 150_lu);  // 3 * 50
        CHECK(measured.height == 30_lu);   // max height

        parent->arrange(logical_rect{0_lu, 0_lu, 200_lu, 100_lu});

        CHECK(parent->child_at(0)->bounds().y.to_int() == 0);      // top aligned
        CHECK(parent->child_at(1)->bounds().y.to_int() == 35);     // centered (100-30)/2
        CHECK(parent->child_at(2)->bounds().y.to_int() == 70);     // bottom aligned (100-30)
    }

    TEST_CASE("Min/max constraints") {
        auto parent = std::make_unique<TestElement>();

        auto layout = std::make_unique<TestLinearLayout>(direction::horizontal, 0);
        parent->set_layout_strategy(std::move(layout));

        auto child = std::make_unique<TestElement>();
        child->set_width_constraint({size_policy::expand, 100_lu, 50_lu, 150_lu, 1.0F}); // min=50, max=150
        parent->add_test_child(std::move(child));

        // Test minimum constraint
        auto measured1 = parent->measure(30_lu, 100_lu);
        CHECK(measured1.width >= 50_lu);  // Reports at least minimum
        parent->arrange(logical_rect{0_lu, 0_lu, 30_lu, 100_lu});
        CHECK(parent->child_at(0)->bounds().width.to_int() == 50); // Clamped to min even with expand

        // Test maximum constraint
        auto measured2 = parent->measure(200_lu, 100_lu);
        CHECK(measured2.width <= 200_lu);
        parent->arrange(logical_rect{0_lu, 0_lu, 200_lu, 100_lu});
        CHECK(parent->child_at(0)->bounds().width.to_int() == 150); // Expands to max (clamped at 150)

        // Test within constraints
        auto measured3 = parent->measure(120_lu, 100_lu);
        CHECK(measured3.width <= 120_lu);
        parent->arrange(logical_rect{0_lu, 0_lu, 120_lu, 100_lu});
        CHECK(parent->child_at(0)->bounds().width.to_int() == 120); // Expands to available space
    }

    TEST_CASE("Visibility handling") {
        auto parent = std::make_unique<TestElement>();

        auto layout = std::make_unique<TestLinearLayout>(direction::horizontal, 10);
        parent->set_layout_strategy(std::move(layout));

        // Add three children
        for (int i = 0; i < 3; i++) {
            auto child = std::make_unique<TestElement>();
            child->set_width_constraint({size_policy::fixed, 100_lu, 100_lu});
            parent->add_test_child(std::move(child));
        }

        // Hide middle child
        parent->child_at(1)->set_visible(false);

        auto measured = parent->measure(1000_lu, 100_lu);
        CHECK(measured.width == 210_lu); // 2*100 + 1*10 (only one spacing)

        parent->arrange(logical_rect{0_lu, 0_lu, 210_lu, 100_lu});

        // Verify positions skip hidden child
        CHECK(parent->child_at(0)->bounds().x.to_int() == 0);
        CHECK(parent->child_at(2)->bounds().x.to_int() == 110); // Right after first child + spacing
    }

    TEST_CASE("Percentage sizing - horizontal") {
        auto parent = std::make_unique<TestElement>();
        auto layout = std::make_unique<TestLinearLayout>(direction::horizontal, 0);
        parent->set_layout_strategy(std::move(layout));

        // Add child with 50% width constraint
        auto child1 = std::make_unique<TestElement>();
        child1->set_width_constraint({size_policy::percentage, 0_lu, 0_lu, 1000_lu, 1.0F, 0.5F});
        child1->set_height_constraint({size_policy::content});
        parent->add_test_child(std::move(child1));

        // Add child with 30% width constraint
        auto child2 = std::make_unique<TestElement>();
        child2->set_width_constraint({size_policy::percentage, 0_lu, 0_lu, 1000_lu, 1.0F, 0.3F});
        child2->set_height_constraint({size_policy::content});
        parent->add_test_child(std::move(child2));

        // Parent width = 400, so children should be 200 (50%) and 120 (30%)
        [[maybe_unused]] auto size = parent->measure(1000_lu, 1000_lu);
        parent->arrange(logical_rect{0_lu, 0_lu, 400_lu, 100_lu});

        CHECK(parent->child_at(0)->bounds().width.to_int() == 200);  // 50% of 400
        CHECK(parent->child_at(1)->bounds().width.to_int() == 120);  // 30% of 400
    }

    TEST_CASE("Percentage sizing - vertical") {
        auto parent = std::make_unique<TestElement>();
        auto layout = std::make_unique<TestLinearLayout>(direction::vertical, 0);
        parent->set_layout_strategy(std::move(layout));

        // Add child with 40% height constraint
        auto child1 = std::make_unique<TestElement>();
        child1->set_width_constraint({size_policy::content});
        child1->set_height_constraint({size_policy::percentage, 0_lu, 0_lu, 1000_lu, 1.0F, 0.4F});
        parent->add_test_child(std::move(child1));

        // Add child with 25% height constraint
        auto child2 = std::make_unique<TestElement>();
        child2->set_width_constraint({size_policy::content});
        child2->set_height_constraint({size_policy::percentage, 0_lu, 0_lu, 1000_lu, 1.0F, 0.25F});
        parent->add_test_child(std::move(child2));

        // Parent height = 500, so children should be 200 (40%) and 125 (25%)
        [[maybe_unused]] auto size2 = parent->measure(1000_lu, 1000_lu);
        parent->arrange(logical_rect{0_lu, 0_lu, 100_lu, 500_lu});

        CHECK(parent->child_at(0)->bounds().height.to_int() == 200);  // 40% of 500
        CHECK(parent->child_at(1)->bounds().height.to_int() == 125);  // 25% of 500
    }

    TEST_CASE("Percentage sizing with min/max constraints") {
        auto parent = std::make_unique<TestElement>();
        auto layout = std::make_unique<TestLinearLayout>(direction::horizontal, 0);
        parent->set_layout_strategy(std::move(layout));

        // Child with 80% width but max constrained to 150
        auto child = std::make_unique<TestElement>();
        child->set_width_constraint({size_policy::percentage, 0_lu, 0_lu, 150_lu, 1.0F, 0.8F});
        child->set_height_constraint({size_policy::content});
        parent->add_test_child(std::move(child));

        // Parent width = 400, 80% would be 320, but clamped to max 150
        [[maybe_unused]] auto size3 = parent->measure(1000_lu, 1000_lu);
        parent->arrange(logical_rect{0_lu, 0_lu, 400_lu, 100_lu});

        CHECK(parent->child_at(0)->bounds().width.to_int() == 150);  // Clamped to max
    }

    TEST_CASE("Content sizing") {
        auto parent = std::make_unique<TestElement>();

        auto layout = std::make_unique<TestLinearLayout>(direction::vertical, 5);
        parent->set_layout_strategy(std::move(layout));

        // Add elements with specific content sizes
        parent->add_test_child(std::make_unique<ContentElement>(150, 20));
        parent->add_test_child(std::make_unique<ContentElement>(100, 30));

        auto measured = parent->measure(1000_lu, 1000_lu);
        CHECK(measured.width == 150_lu);  // Max of child widths
        CHECK(measured.height == 55_lu);  // 20 + 30 + 5
    }
}