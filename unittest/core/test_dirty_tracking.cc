/**
 * @file test_dirty_tracking.cc
 * @brief Comprehensive unit tests for dirty region tracking functionality
 * @author Assistant
 * @date 2024
 */

#include "../utils/test_helpers.hh"
#include "../../include/onyxui/core/element.hh"
#include "../../include/onyxui/core/types.hh"
#include "../../include/onyxui/core/geometry.hh"
#include <algorithm>
#include <cstddef>
#include <memory>
#include <utility>

using namespace onyxui;
using testing::make_relative_rect;

/**
 * @class DirtyTrackingElement
 * @brief Test element that exposes dirty tracking for verification
 */
class DirtyTrackingElement : public TestElement {
public:
    explicit DirtyTrackingElement(TestElement* parent = nullptr)
        : TestElement(parent) {}

    // Expose dirty region methods for testing
    using TestElement::mark_dirty;
    using TestElement::mark_dirty_region;
    using TestElement::get_and_clear_dirty_regions;

    // For testing, we'll just inspect and clear, not trying to restore
    size_t count_and_clear_dirty() {
        auto regions = get_and_clear_dirty_regions();
        return regions.size();
    }

    // Check if a specific region exists then clear all
    bool check_and_clear_dirty_region(logical_unit x, logical_unit y, logical_unit w, logical_unit h) {
        auto regions = get_and_clear_dirty_regions();
        // Dirty regions are in physical backend coordinates, so convert from logical
        TestRect target{x.to_int(), y.to_int(), w.to_int(), h.to_int()};

        return std::any_of(regions.begin(), regions.end(),
            [&](const TestRect& r) {
                return r.x == target.x && r.y == target.y &&
                       r.w == target.w && r.h == target.h;
            });
    }
};

TEST_SUITE("Dirty Region Tracking") {

    TEST_CASE("Basic dirty marking") {
        auto root = std::make_unique<DirtyTrackingElement>();

        SUBCASE("Initially no dirty regions") {
            auto regions = root->get_and_clear_dirty_regions();
            CHECK(regions.empty());
        }

        SUBCASE("Mark element dirty") {
            root->arrange(logical_rect{logical_unit(10.0), logical_unit(20.0), logical_unit(100.0), logical_unit(50.0)});

            // arrange() marks old bounds dirty (initial bounds are 0,0,0,0)
            // Clear that first
            (void)root->get_and_clear_dirty_regions();

            // Now mark current bounds as dirty
            root->mark_dirty();

            auto regions = root->get_and_clear_dirty_regions();
            CHECK(regions.size() == 1);
            CHECK(regions[0].x == 10);
            CHECK(regions[0].y == 20);
            CHECK(regions[0].w == 100);
            CHECK(regions[0].h == 50);
        }

        SUBCASE("Mark specific region dirty") {
            TestRect const custom_region{5, 15, 25, 35};
            root->mark_dirty_region(custom_region);

            auto regions = root->get_and_clear_dirty_regions();
            CHECK(regions.size() == 1);
            CHECK(regions[0].x == 5);
            CHECK(regions[0].y == 15);
            CHECK(regions[0].w == 25);
            CHECK(regions[0].h == 35);
        }
    }

    TEST_CASE("Dirty propagation to root") {
        auto root = std::make_unique<DirtyTrackingElement>();
        auto child1 = std::make_unique<DirtyTrackingElement>();
        auto child2 = std::make_unique<DirtyTrackingElement>();

        // Set up hierarchy
        child1->arrange(logical_rect{logical_unit(10.0), logical_unit(10.0), logical_unit(50.0), logical_unit(30.0)});
        child2->arrange(logical_rect{logical_unit(70.0), logical_unit(10.0), logical_unit(50.0), logical_unit(30.0)});

        auto* child1_ptr = child1.get();
        auto* child2_ptr = child2.get();

        root->add_test_child(std::move(child1));
        root->add_test_child(std::move(child2));

        SUBCASE("Child dirty marks propagate to root") {
            // Clear any initial dirty regions
            (void)root->get_and_clear_dirty_regions();

            child1_ptr->mark_dirty();

            // In the current implementation, both child and root store the region
            // This is fine as long as the root gets it for clearing
            auto child_regions = child1_ptr->get_and_clear_dirty_regions();
            // Child may or may not store it locally (implementation detail)

            // Root should have the child's bounds
            auto root_regions = root->get_and_clear_dirty_regions();
            CHECK(root_regions.size() >= 1);

            bool found = false;
            for (const auto& r : root_regions) {
                if (r.x == 10 && r.y == 10) {
                    found = true;
                    break;
                }
            }
            CHECK(found);
        }

        SUBCASE("Multiple children mark dirty") {
            // Clear any initial dirty regions
            (void)root->get_and_clear_dirty_regions();

            child1_ptr->mark_dirty();
            child2_ptr->mark_dirty();

            auto regions = root->get_and_clear_dirty_regions();
            CHECK(regions.size() == 2);

            // Check both regions are present (order may vary)
            bool found_child1 = false;
            bool found_child2 = false;
            for (const auto& r : regions) {
                if (r.x == 10 && r.y == 10) found_child1 = true;
                if (r.x == 70 && r.y == 10) found_child2 = true;
            }
            CHECK(found_child1);
            CHECK(found_child2);
        }

        SUBCASE("Deep hierarchy propagation") {
            auto grandchild = std::make_unique<DirtyTrackingElement>();
            grandchild->arrange(logical_rect{5_lu, 5_lu, 20_lu, 10_lu});
            auto* grandchild_ptr = grandchild.get();

            child1_ptr->add_test_child(std::move(grandchild));

            // Clear any initial dirty regions
            (void)root->get_and_clear_dirty_regions();

            // Mark grandchild dirty
            grandchild_ptr->mark_dirty();

            // Should propagate to root
            auto regions = root->get_and_clear_dirty_regions();
            CHECK(regions.size() == 1);
            // grandchild at (5, 5) relative to child1 at (10, 10) → absolute (15, 15)
            CHECK(regions[0].x == 15);
            CHECK(regions[0].y == 15);
        }
    }

    TEST_CASE("Visibility changes mark dirty") {
        auto root = std::make_unique<DirtyTrackingElement>();
        auto child = std::make_unique<DirtyTrackingElement>();

        child->arrange(logical_rect{20_lu, 30_lu, 60_lu, 40_lu});
        auto* child_ptr = child.get();
        root->add_test_child(std::move(child));

        SUBCASE("Hiding marks dirty") {
            (void)root->get_and_clear_dirty_regions(); // Clear initial

            child_ptr->set_visible(false);

            auto regions = root->get_and_clear_dirty_regions();
            CHECK(regions.size() == 1);
            CHECK(regions[0].x == 20);
            CHECK(regions[0].y == 30);
        }

        SUBCASE("Showing marks dirty") {
            // Start hidden
            child_ptr->set_visible(false);
            (void)root->get_and_clear_dirty_regions();

            // Show again
            child_ptr->set_visible(true);

            auto regions = root->get_and_clear_dirty_regions();
            CHECK(regions.size() == 1);
            CHECK(regions[0].x == 20);
            CHECK(regions[0].y == 30);
        }

        SUBCASE("No change doesn't mark dirty") {
            (void)root->get_and_clear_dirty_regions(); // Clear initial

            // Already visible, setting to visible again
            child_ptr->set_visible(true);

            auto regions = root->get_and_clear_dirty_regions();
            CHECK(regions.empty());
        }
    }

    TEST_CASE("Bounds changes mark dirty") {
        auto root = std::make_unique<DirtyTrackingElement>();
        auto child = std::make_unique<DirtyTrackingElement>();

        child->arrange(logical_rect{10_lu, 10_lu, 30_lu, 20_lu});
        auto* child_ptr = child.get();
        root->add_test_child(std::move(child));

        // Clear any initial dirty regions
        (void)root->get_and_clear_dirty_regions();

        SUBCASE("Moving element marks old bounds dirty") {
            // Move to new position
            child_ptr->arrange(logical_rect{50_lu, 60_lu, 30_lu, 20_lu});

            auto regions = root->get_and_clear_dirty_regions();
            CHECK(regions.size() == 1);
            // Should mark old bounds as dirty
            CHECK(regions[0].x == 10);
            CHECK(regions[0].y == 10);
        }

        SUBCASE("Resizing element marks old bounds dirty") {
            // Resize at same position
            child_ptr->arrange(logical_rect{10_lu, 10_lu, 50_lu, 40_lu});

            auto regions = root->get_and_clear_dirty_regions();
            CHECK(regions.size() == 1);
            // Should mark old bounds as dirty
            CHECK(regions[0].x == 10);
            CHECK(regions[0].y == 10);
            CHECK(regions[0].w == 30);
            CHECK(regions[0].h == 20);
        }

        SUBCASE("No bounds change doesn't mark dirty") {
            // Arrange with same bounds
            child_ptr->arrange(logical_rect{10_lu, 10_lu, 30_lu, 20_lu});

            auto regions = root->get_and_clear_dirty_regions();
            CHECK(regions.empty());
        }
    }

    TEST_CASE("Multiple dirty regions accumulate") {
        auto root = std::make_unique<DirtyTrackingElement>();

        SUBCASE("Different regions accumulate") {
            root->mark_dirty_region(TestRect{10,  10,  20,  20});
            root->mark_dirty_region(TestRect{50,  50,  30,  30});
            root->mark_dirty_region(TestRect{100,  10,  40,  25});

            auto regions = root->get_and_clear_dirty_regions();
            CHECK(regions.size() == 3);
        }

        SUBCASE("Same region can be marked multiple times") {
            root->mark_dirty_region(TestRect{10,  10,  20,  20});
            root->mark_dirty_region(TestRect{10,  10,  20,  20});

            auto regions = root->get_and_clear_dirty_regions();
            // Both are stored (no deduplication at this level)
            CHECK(regions.size() == 2);
        }

        SUBCASE("Clear removes all regions") {
            root->mark_dirty_region(TestRect{10,  10,  20,  20});
            root->mark_dirty_region(TestRect{50,  50,  30,  30});

            auto regions = root->get_and_clear_dirty_regions();
            CHECK(regions.size() == 2);

            // After clear, should be empty
            regions = root->get_and_clear_dirty_regions();
            CHECK(regions.empty());
        }
    }

    TEST_CASE("Complex hierarchy scenario") {
        // Create a complex hierarchy:
        //   root
        //   ├── panel1
        //   │   ├── button1
        //   │   └── button2
        //   └── panel2
        //       └── label1

        auto root = std::make_unique<DirtyTrackingElement>();
        root->arrange(logical_rect{0_lu, 0_lu, 800_lu, 600_lu});

        auto panel1 = std::make_unique<DirtyTrackingElement>();
        panel1->arrange(logical_rect{10_lu, 10_lu, 380_lu, 280_lu});
        auto* panel1_ptr = panel1.get();

        auto button1 = std::make_unique<DirtyTrackingElement>();
        button1->arrange(logical_rect{20_lu, 20_lu, 100_lu, 30_lu});
        auto* button1_ptr = button1.get();

        auto button2 = std::make_unique<DirtyTrackingElement>();
        button2->arrange(logical_rect{20_lu, 60_lu, 100_lu, 30_lu});
        auto* button2_ptr = button2.get();

        auto panel2 = std::make_unique<DirtyTrackingElement>();
        panel2->arrange(logical_rect{410_lu, 10_lu, 380_lu, 280_lu});
        auto* panel2_ptr = panel2.get();

        auto label1 = std::make_unique<DirtyTrackingElement>();
        label1->arrange(logical_rect{420_lu, 20_lu, 200_lu, 20_lu});

        // Build hierarchy
        panel1->add_test_child(std::move(button1));
        panel1->add_test_child(std::move(button2));
        panel2->add_test_child(std::move(label1));
        root->add_test_child(std::move(panel1));
        root->add_test_child(std::move(panel2));

        // Clear any setup dirty regions
        (void)root->get_and_clear_dirty_regions();

        SUBCASE("Multiple widgets change") {
            // Simulate hover on button1
            button1_ptr->mark_dirty();

            // Hide panel2
            panel2_ptr->set_visible(false);

            // Move button2
            button2_ptr->arrange(logical_rect{30_lu, 70_lu, 100_lu, 30_lu});

            // Check all dirty regions collected at root
            auto regions = root->get_and_clear_dirty_regions();
            CHECK(regions.size() == 3);

            // Verify the expected regions are present
            bool found_button1 = false;
            bool found_panel2 = false;
            bool found_button2_old = false;

            for (const auto& r : regions) {
                // button1 at (20, 20) relative to panel1 at (10, 10) → absolute (30, 30)
                if (r.x == 30 && r.y == 30) found_button1 = true;
                // panel2 is direct child of root at (410, 10) → absolute (410, 10)
                if (r.x == 410 && r.y == 10) found_panel2 = true;
                // button2 at (20, 60) relative to panel1 at (10, 10) → absolute (30, 70)
                if (r.x == 30 && r.y == 70) found_button2_old = true;
            }

            CHECK(found_button1);
            CHECK(found_panel2);
            CHECK(found_button2_old);
        }

        SUBCASE("Cascading visibility") {
            // Hide parent panel
            panel1_ptr->set_visible(false);

            auto regions = root->get_and_clear_dirty_regions();
            CHECK(regions.size() == 1);
            CHECK(regions[0].x == 10);
            CHECK(regions[0].y == 10);

            // Show again
            panel1_ptr->set_visible(true);

            regions = root->get_and_clear_dirty_regions();
            CHECK(regions.size() == 1);
            CHECK(regions[0].x == 10);
            CHECK(regions[0].y == 10);
        }
    }

    TEST_CASE("Hidden elements and bounds changes") {
        auto root = std::make_unique<DirtyTrackingElement>();
        auto child = std::make_unique<DirtyTrackingElement>();

        child->arrange(logical_rect{10_lu, 10_lu, 30_lu, 20_lu});
        child->set_visible(false);
        auto* child_ptr = child.get();
        root->add_test_child(std::move(child));

        // Clear setup dirty regions
        (void)root->get_and_clear_dirty_regions();

        SUBCASE("Hidden element bounds change doesn't mark dirty") {
            // Element is hidden
            CHECK(!child_ptr->is_visible());

            // Change bounds while hidden
            child_ptr->arrange(logical_rect{50_lu, 50_lu, 40_lu, 30_lu});

            // Should not mark dirty (element is hidden)
            auto regions = root->get_and_clear_dirty_regions();
            CHECK(regions.empty());
        }
    }

    TEST_CASE("Empty and edge cases") {
        auto root = std::make_unique<DirtyTrackingElement>();

        SUBCASE("Empty bounds") {
            root->arrange(logical_rect{0_lu, 0_lu, 0_lu, 0_lu});
            root->mark_dirty();

            auto regions = root->get_and_clear_dirty_regions();
            CHECK(regions.size() == 1);
            CHECK(regions[0].w == 0);
            CHECK(regions[0].h == 0);
        }

        SUBCASE("Negative coordinates") {
            TestRect const negative_region{-10,  -20,  30,  40};
            root->mark_dirty_region(negative_region);

            auto regions = root->get_and_clear_dirty_regions();
            CHECK(regions.size() == 1);
            CHECK(regions[0].x == -10);
            CHECK(regions[0].y == -20);
        }

        SUBCASE("Very large regions") {
            TestRect const large_region{0,  0,  10000,  10000};
            root->mark_dirty_region(large_region);

            auto regions = root->get_and_clear_dirty_regions();
            CHECK(regions.size() == 1);
            CHECK(regions[0].w == 10000);
            CHECK(regions[0].h == 10000);
        }
    }
}
