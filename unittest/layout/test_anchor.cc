/**
 * @file test_anchor.cc
 * @brief Unit tests for anchor_layout
 */

#include "../../include/onyxui/layout/layout_strategy.hh"
#include "utils/test_helpers.hh"
#include <memory>
#include <onyxui/layout/anchor_layout.hh>
#include <utility>
#include <vector>

using TestAnchorLayout = anchor_layout<TestBackend>;

TEST_SUITE("anchor_layout") {
    TEST_CASE("Basic anchor positions") {
        auto parent = std::make_unique<TestElement>();
        auto layout = std::make_unique<TestAnchorLayout>();
        auto layout_ptr = layout.get();
        parent->set_layout_strategy(std::move(layout));

        // Create children for all 9 anchor points
        std::vector<TestElement*> children;
        for (int i = 0; i < 9; i++) {
            auto child = std::make_unique<TestElement>();
            child->set_width_constraint({size_policy::fixed, 20_lu, 20_lu});
            child->set_height_constraint({size_policy::fixed, 10_lu, 10_lu});
            children.push_back(child.get());
            parent->add_test_child(std::move(child));
        }

        // Set anchor points
        layout_ptr->set_anchor(children[0], anchor_point::top_left);
        layout_ptr->set_anchor(children[1], anchor_point::top_center);
        layout_ptr->set_anchor(children[2], anchor_point::top_right);
        layout_ptr->set_anchor(children[3], anchor_point::center_left);
        layout_ptr->set_anchor(children[4], anchor_point::center);
        layout_ptr->set_anchor(children[5], anchor_point::center_right);
        layout_ptr->set_anchor(children[6], anchor_point::bottom_left);
        layout_ptr->set_anchor(children[7], anchor_point::bottom_center);
        layout_ptr->set_anchor(children[8], anchor_point::bottom_right);

        // Measure: anchor layout reports max extent needed
        auto measured = parent->measure(200_lu, 100_lu);
        CHECK(measured.width > 0_lu);
        CHECK(measured.height > 0_lu);

        parent->arrange(logical_rect{0_lu, 0_lu, 200_lu, 100_lu});

        // Verify positions
        // Top row
        CHECK(children[0]->bounds().x.to_int() == 0);
        CHECK(children[0]->bounds().y.to_int() == 0);

        CHECK(children[1]->bounds().x.to_int() == 90);  // (200-20)/2
        CHECK(children[1]->bounds().y.to_int() == 0);

        CHECK(children[2]->bounds().x.to_int() == 180); // 200-20
        CHECK(children[2]->bounds().y.to_int() == 0);

        // Middle row
        CHECK(children[3]->bounds().x.to_int() == 0);
        CHECK(children[3]->bounds().y.to_int() == 45);  // (100-10)/2

        CHECK(children[4]->bounds().x.to_int() == 90);  // (200-20)/2
        CHECK(children[4]->bounds().y.to_int() == 45);  // (100-10)/2

        CHECK(children[5]->bounds().x.to_int() == 180); // 200-20
        CHECK(children[5]->bounds().y.to_int() == 45);  // (100-10)/2

        // Bottom row
        CHECK(children[6]->bounds().x.to_int() == 0);
        CHECK(children[6]->bounds().y.to_int() == 90);  // 100-10

        CHECK(children[7]->bounds().x.to_int() == 90);  // (200-20)/2
        CHECK(children[7]->bounds().y.to_int() == 90);  // 100-10

        CHECK(children[8]->bounds().x.to_int() == 180); // 200-20
        CHECK(children[8]->bounds().y.to_int() == 90);  // 100-10
    }

    TEST_CASE("Anchor with offsets") {
        auto parent = std::make_unique<TestElement>();
        auto layout = std::make_unique<TestAnchorLayout>();
        auto layout_ptr = layout.get();
        parent->set_layout_strategy(std::move(layout));

        auto child = std::make_unique<TestElement>();
        child->set_width_constraint({size_policy::fixed, 50_lu, 50_lu});
        child->set_height_constraint({size_policy::fixed, 30_lu, 30_lu});
        auto child_ptr = child.get();
        parent->add_test_child(std::move(child));

        // Anchor to top-right with offsets
        layout_ptr->set_anchor(child_ptr, anchor_point::top_right, -10, 15);

        // Measure with offsets
        auto measured = parent->measure(300_lu, 200_lu);
        CHECK(measured.width <= 300_lu);
        CHECK(measured.height <= 200_lu);

        parent->arrange(logical_rect{0_lu, 0_lu, 300_lu, 200_lu});

        // Should be at top-right minus offsets
        CHECK(child_ptr->bounds().x.to_int() == 240);  // 300-50-10
        CHECK(child_ptr->bounds().y.to_int() == 15);   // 0+15
    }
}