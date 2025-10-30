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
            child->set_width_constraint({size_policy::fixed, 100, 100});
            child->set_height_constraint({size_policy::fixed, 50, 50});
            parent->add_test_child(std::move(child));
        }

        // Measure and arrange
        TestSize const measured = parent->measure(1000, 1000);
        CHECK(measured.w == 320); // 3*100 + 2*10
        CHECK(measured.h == 50);

        parent->arrange({0, 0, 320, 50});

        // Verify child positions
        CHECK(parent->child_at(0)->bounds().x == 0);
        CHECK(parent->child_at(0)->bounds().w == 100);
        CHECK(parent->child_at(1)->bounds().x == 110);
        CHECK(parent->child_at(1)->bounds().w == 100);
        CHECK(parent->child_at(2)->bounds().x == 220);
        CHECK(parent->child_at(2)->bounds().w == 100);
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
            child->set_height_constraint({size_policy::expand, 0, 0, std::numeric_limits<int>::max(), 1.0F});
            parent->add_test_child(std::move(child));
        }

        // Measure and arrange
        TestSize const measured = parent->measure(200, 300);
        // For vertical layout with no fixed-width children, width should be max of child widths
        // Since children have fill_parent policy, they should measure to 0 initially
        CHECK(measured.w == 0);    // Children with fill_parent don't contribute to measure
        CHECK(measured.h == 5);   // Just the spacing between 2 children
        parent->arrange({0, 0, 200, 300});

        // Verify children split space equally (300 pixels total, 5 pixels spacing)
        // Available space = 300 - 5 = 295, divided by 2 = 147.5 each
        // First child gets the extra pixel from rounding (standard behavior in CSS flexbox)
        CHECK(parent->child_at(0)->bounds().h == 148); // Gets the extra pixel from rounding
        CHECK(parent->child_at(0)->bounds().y == 0);
        CHECK(parent->child_at(1)->bounds().h == 147); // Floor of 295/2
        CHECK(parent->child_at(1)->bounds().y == 153);      // 148 + 5 spacing
    }

    TEST_CASE("Weighted distribution") {
        auto parent = std::make_unique<TestElement>();

        auto layout = std::make_unique<TestLinearLayout>(direction::horizontal, 0);
        parent->set_layout_strategy(std::move(layout));

        // Add children with weights 1:2:1
        auto child1 = std::make_unique<TestElement>();
        child1->set_width_constraint({size_policy::weighted, 0, 0, std::numeric_limits<int>::max(), 1.0F});
        parent->add_test_child(std::move(child1));

        auto child2 = std::make_unique<TestElement>();
        child2->set_width_constraint({size_policy::weighted, 0, 0, std::numeric_limits<int>::max(), 2.0F});
        parent->add_test_child(std::move(child2));

        auto child3 = std::make_unique<TestElement>();
        child3->set_width_constraint({size_policy::weighted, 0, 0, std::numeric_limits<int>::max(), 1.0F});
        parent->add_test_child(std::move(child3));

        // Measure: weighted children report minimal size during measure
        auto measured = parent->measure(400, 100);
        CHECK(measured.w == 0);  // Weighted children contribute 0 to measure
        CHECK(measured.h == 0);

        parent->arrange({0, 0, 400, 100});

        CHECK(parent->child_at(0)->bounds().w == 100); // 1/4 of 400
        CHECK(parent->child_at(1)->bounds().w == 200); // 2/4 of 400
        CHECK(parent->child_at(2)->bounds().w == 100); // 1/4 of 400
    }

    TEST_CASE("Alignment in cross axis") {
        auto parent = std::make_unique<TestElement>();

        auto layout = std::make_unique<TestLinearLayout>(direction::horizontal, 0);
        parent->set_layout_strategy(std::move(layout));

        // Add children with different vertical alignments
        auto child1 = std::make_unique<TestElement>();
        child1->set_width_constraint({size_policy::fixed, 50, 50});
        child1->set_height_constraint({size_policy::fixed, 30, 30});
        child1->set_vertical_align(vertical_alignment::top);
        parent->add_test_child(std::move(child1));

        auto child2 = std::make_unique<TestElement>();
        child2->set_width_constraint({size_policy::fixed, 50, 50});
        child2->set_height_constraint({size_policy::fixed, 30, 30});
        child2->set_vertical_align(vertical_alignment::center);
        parent->add_test_child(std::move(child2));

        auto child3 = std::make_unique<TestElement>();
        child3->set_width_constraint({size_policy::fixed, 50, 50});
        child3->set_height_constraint({size_policy::fixed, 30, 30});
        child3->set_vertical_align(vertical_alignment::bottom);
        parent->add_test_child(std::move(child3));

        // Measure: horizontal layout with fixed children
        auto measured = parent->measure(200, 100);
        CHECK(measured.w == 150);  // 3 * 50
        CHECK(measured.h == 30);   // max height

        parent->arrange({0, 0, 200, 100});

        CHECK(parent->child_at(0)->bounds().y == 0);      // top aligned
        CHECK(parent->child_at(1)->bounds().y == 35);     // centered (100-30)/2
        CHECK(parent->child_at(2)->bounds().y == 70);     // bottom aligned (100-30)
    }

    TEST_CASE("Min/max constraints") {
        auto parent = std::make_unique<TestElement>();

        auto layout = std::make_unique<TestLinearLayout>(direction::horizontal, 0);
        parent->set_layout_strategy(std::move(layout));

        auto child = std::make_unique<TestElement>();
        child->set_width_constraint({size_policy::expand, 100, 50, 150, 1.0F}); // min=50, max=150
        parent->add_test_child(std::move(child));

        // Test minimum constraint
        auto measured1 = parent->measure(30, 100);
        CHECK(measured1.w >= 50);  // Reports at least minimum
        parent->arrange({0, 0, 30, 100});
        CHECK(parent->child_at(0)->bounds().w == 50); // Clamped to min even with expand

        // Test maximum constraint
        auto measured2 = parent->measure(200, 100);
        CHECK(measured2.w <= 200);
        parent->arrange({0, 0, 200, 100});
        CHECK(parent->child_at(0)->bounds().w == 150); // Expands to max (clamped at 150)

        // Test within constraints
        auto measured3 = parent->measure(120, 100);
        CHECK(measured3.w <= 120);
        parent->arrange({0, 0, 120, 100});
        CHECK(parent->child_at(0)->bounds().w == 120); // Expands to available space
    }

    TEST_CASE("Visibility handling") {
        auto parent = std::make_unique<TestElement>();

        auto layout = std::make_unique<TestLinearLayout>(direction::horizontal, 10);
        parent->set_layout_strategy(std::move(layout));

        // Add three children
        for (int i = 0; i < 3; i++) {
            auto child = std::make_unique<TestElement>();
            child->set_width_constraint({size_policy::fixed, 100, 100});
            parent->add_test_child(std::move(child));
        }

        // Hide middle child
        parent->child_at(1)->set_visible(false);

        TestSize const measured = parent->measure(1000, 100);
        CHECK(measured.w == 210); // 2*100 + 1*10 (only one spacing)

        parent->arrange({0, 0, 210, 100});

        // Verify positions skip hidden child
        CHECK(parent->child_at(0)->bounds().x == 0);
        CHECK(parent->child_at(2)->bounds().x == 110); // Right after first child + spacing
    }

    TEST_CASE("Percentage sizing - horizontal") {
        auto parent = std::make_unique<TestElement>();
        auto layout = std::make_unique<TestLinearLayout>(direction::horizontal, 0);
        parent->set_layout_strategy(std::move(layout));

        // Add child with 50% width constraint
        auto child1 = std::make_unique<TestElement>();
        child1->set_width_constraint({size_policy::percentage, 0, 0, 1000, 1.0F, 0.5F});
        child1->set_height_constraint({size_policy::content});
        parent->add_test_child(std::move(child1));

        // Add child with 30% width constraint
        auto child2 = std::make_unique<TestElement>();
        child2->set_width_constraint({size_policy::percentage, 0, 0, 1000, 1.0F, 0.3F});
        child2->set_height_constraint({size_policy::content});
        parent->add_test_child(std::move(child2));

        // Parent width = 400, so children should be 200 (50%) and 120 (30%)
        parent->measure(1000, 1000);
        parent->arrange({0, 0, 400, 100});

        CHECK(parent->child_at(0)->bounds().w == 200);  // 50% of 400
        CHECK(parent->child_at(1)->bounds().w == 120);  // 30% of 400
    }

    TEST_CASE("Percentage sizing - vertical") {
        auto parent = std::make_unique<TestElement>();
        auto layout = std::make_unique<TestLinearLayout>(direction::vertical, 0);
        parent->set_layout_strategy(std::move(layout));

        // Add child with 40% height constraint
        auto child1 = std::make_unique<TestElement>();
        child1->set_width_constraint({size_policy::content});
        child1->set_height_constraint({size_policy::percentage, 0, 0, 1000, 1.0F, 0.4F});
        parent->add_test_child(std::move(child1));

        // Add child with 25% height constraint
        auto child2 = std::make_unique<TestElement>();
        child2->set_width_constraint({size_policy::content});
        child2->set_height_constraint({size_policy::percentage, 0, 0, 1000, 1.0F, 0.25F});
        parent->add_test_child(std::move(child2));

        // Parent height = 500, so children should be 200 (40%) and 125 (25%)
        parent->measure(1000, 1000);
        parent->arrange({0, 0, 100, 500});

        CHECK(parent->child_at(0)->bounds().h == 200);  // 40% of 500
        CHECK(parent->child_at(1)->bounds().h == 125);  // 25% of 500
    }

    TEST_CASE("Percentage sizing with min/max constraints") {
        auto parent = std::make_unique<TestElement>();
        auto layout = std::make_unique<TestLinearLayout>(direction::horizontal, 0);
        parent->set_layout_strategy(std::move(layout));

        // Child with 80% width but max constrained to 150
        auto child = std::make_unique<TestElement>();
        child->set_width_constraint({size_policy::percentage, 0, 0, 150, 1.0F, 0.8F});
        child->set_height_constraint({size_policy::content});
        parent->add_test_child(std::move(child));

        // Parent width = 400, 80% would be 320, but clamped to max 150
        parent->measure(1000, 1000);
        parent->arrange({0, 0, 400, 100});

        CHECK(parent->child_at(0)->bounds().w == 150);  // Clamped to max
    }

    TEST_CASE("Content sizing") {
        auto parent = std::make_unique<TestElement>();

        auto layout = std::make_unique<TestLinearLayout>(direction::vertical, 5);
        parent->set_layout_strategy(std::move(layout));

        // Add elements with specific content sizes
        parent->add_test_child(std::make_unique<ContentElement>(150, 20));
        parent->add_test_child(std::make_unique<ContentElement>(100, 30));

        TestSize const measured = parent->measure(1000, 1000);
        CHECK(measured.w == 150);  // Max of child widths
        CHECK(measured.h == 55);  // 20 + 30 + 5
    }
}