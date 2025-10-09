/**
 * @file test_anchor.cc
 * @brief Unit tests for anchor_layout
 */

#include "utils/test_helpers.hh"
#include <onyxui/layout/anchor_layout.hh>

using TestAnchorLayout = anchor_layout<TestRect, TestSize>;

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
            child->set_width_constraint({size_policy::fixed, 20, 20});
            child->set_height_constraint({size_policy::fixed, 10, 10});
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

        parent->measure(200, 100);
        parent->arrange({0, 0, 200, 100});

        // Verify positions
        // Top row
        CHECK(children[0]->bounds().x == 0);
        CHECK(children[0]->bounds().y == 0);

        CHECK(children[1]->bounds().x == 90);  // (200-20)/2
        CHECK(children[1]->bounds().y == 0);

        CHECK(children[2]->bounds().x == 180); // 200-20
        CHECK(children[2]->bounds().y == 0);

        // Middle row
        CHECK(children[3]->bounds().x == 0);
        CHECK(children[3]->bounds().y == 45);  // (100-10)/2

        CHECK(children[4]->bounds().x == 90);  // (200-20)/2
        CHECK(children[4]->bounds().y == 45);  // (100-10)/2

        CHECK(children[5]->bounds().x == 180); // 200-20
        CHECK(children[5]->bounds().y == 45);  // (100-10)/2

        // Bottom row
        CHECK(children[6]->bounds().x == 0);
        CHECK(children[6]->bounds().y == 90);  // 100-10

        CHECK(children[7]->bounds().x == 90);  // (200-20)/2
        CHECK(children[7]->bounds().y == 90);  // 100-10

        CHECK(children[8]->bounds().x == 180); // 200-20
        CHECK(children[8]->bounds().y == 90);  // 100-10
    }

    TEST_CASE("Anchor with offsets") {
        auto parent = std::make_unique<TestElement>();
        auto layout = std::make_unique<TestAnchorLayout>();
        auto layout_ptr = layout.get();
        parent->set_layout_strategy(std::move(layout));

        auto child = std::make_unique<TestElement>();
        child->set_width_constraint({size_policy::fixed, 50, 50});
        child->set_height_constraint({size_policy::fixed, 30, 30});
        auto child_ptr = child.get();
        parent->add_test_child(std::move(child));

        // Anchor to top-right with offsets
        layout_ptr->set_anchor(child_ptr, anchor_point::top_right, -10, 15);

        parent->measure(300, 200);
        parent->arrange({0, 0, 300, 200});

        // Should be at top-right minus offsets
        CHECK(child_ptr->bounds().x == 240);  // 300-50-10
        CHECK(child_ptr->bounds().y == 15);   // 0+15
    }
}