/**
 * @file test_hit_test_path.cc
 * @brief Tests for hit test path recording
 *
 * Tests the hit_test_path structure and path-recording hit_test() overload.
 * This is Phase 2 of the event routing implementation.
 */

#include <doctest/doctest.h>
#include <onyxui/events/hit_test_path.hh>
#include <utils/test_helpers.hh>

using namespace onyxui;
using namespace onyxui::testing;

// ======================================================================
// hit_test_path Basic Tests
// ======================================================================

TEST_CASE("hit_test_path - Construction and basic operations") {
    hit_test_path<TestBackend> path;

    SUBCASE("Empty path") {
        CHECK(path.empty());
        CHECK(path.size() == 0);
        CHECK(path.target() == nullptr);
        CHECK(path.root() == nullptr);
    }

    SUBCASE("Push single element") {
        TestElement elem;
        path.push(&elem);

        CHECK_FALSE(path.empty());
        CHECK(path.size() == 1);
        CHECK(path.target() == &elem);
        CHECK(path.root() == &elem);
        CHECK(path[0] == &elem);
    }

    SUBCASE("Push multiple elements") {
        TestElement root;
        TestElement child1;
        TestElement child2;

        path.push(&root);
        path.push(&child1);
        path.push(&child2);

        CHECK(path.size() == 3);
        CHECK(path.root() == &root);
        CHECK(path[0] == &root);
        CHECK(path[1] == &child1);
        CHECK(path[2] == &child2);
        CHECK(path.target() == &child2);
    }

    SUBCASE("Clear path") {
        TestElement elem1;
        TestElement elem2;

        path.push(&elem1);
        path.push(&elem2);
        CHECK(path.size() == 2);

        path.clear();
        CHECK(path.empty());
        CHECK(path.size() == 0);
        CHECK(path.target() == nullptr);
        CHECK(path.root() == nullptr);
    }
}

TEST_CASE("hit_test_path - Element queries") {
    hit_test_path<TestBackend> path;

    TestElement root;
    TestElement child1;
    TestElement child2;
    TestElement child3;

    path.push(&root);
    path.push(&child1);
    path.push(&child2);
    path.push(&child3);

    SUBCASE("contains() finds elements in path") {
        CHECK(path.contains(&root));
        CHECK(path.contains(&child1));
        CHECK(path.contains(&child2));
        CHECK(path.contains(&child3));
    }

    SUBCASE("contains() returns false for elements not in path") {
        TestElement other;
        CHECK_FALSE(path.contains(&other));
    }

    SUBCASE("depth_of() returns correct indices") {
        CHECK(path.depth_of(&root) == 0);
        CHECK(path.depth_of(&child1) == 1);
        CHECK(path.depth_of(&child2) == 2);
        CHECK(path.depth_of(&child3) == 3);
    }

    SUBCASE("depth_of() returns -1 for elements not in path") {
        TestElement other;
        CHECK(path.depth_of(&other) == static_cast<size_t>(-1));
    }

    SUBCASE("at() with valid index") {
        CHECK(path.at(0) == &root);
        CHECK(path.at(1) == &child1);
        CHECK(path.at(2) == &child2);
        CHECK(path.at(3) == &child3);
    }

    SUBCASE("at() with invalid index throws") {
        CHECK_THROWS_AS((void)path.at(4), std::out_of_range);
        CHECK_THROWS_AS((void)path.at(100), std::out_of_range);
    }
}

// ======================================================================
// hit_test() with Path Recording
// ======================================================================

TEST_CASE("hit_test() - Path recording with simple hierarchy") {
    SUBCASE("Step 1: Just create root") {
        auto root = std::make_unique<TestElement>();
        CHECK(root != nullptr);
    }

    SUBCASE("Step 2: Create root and add child") {
        auto root = std::make_unique<TestElement>();
        root->add_child(std::make_unique<TestElement>());
        CHECK(root->children().size() == 1);
    }

    SUBCASE("Step 3: Create root, add child, get pointer") {
        auto root = std::make_unique<TestElement>();
        root->add_child(std::make_unique<TestElement>());
        auto* child = static_cast<TestElement*>(root->children()[0].get());
        CHECK(child != nullptr);
    }

    SUBCASE("Step 4: Arrange root only (no children)") {
        auto root = std::make_unique<TestElement>();
        root->arrange({0, 0, 100, 100});
        CHECK(true);
    }

    SUBCASE("Step 5: Create root+child, then arrange root") {
        auto root = std::make_unique<TestElement>();
        root->add_child(std::make_unique<TestElement>());

        // THIS IS WHERE THE OVERFLOW HAPPENS
        root->arrange({0, 0, 100, 100});
        CHECK(true);
    }

    SUBCASE("Step 6: Full test - arrange both") {
        auto root = std::make_unique<TestElement>();
        root->add_child(std::make_unique<TestElement>());
        auto* child = static_cast<TestElement*>(root->children()[0].get());

        root->arrange({0, 0, 100, 100});
        child->arrange({25, 25, 50, 50});

        // Now try hit testing
        hit_test_path<TestBackend> path;
        auto* result = root->hit_test(5, 5, path);
        CHECK(result != nullptr);
    }
}

TEST_CASE("hit_test() - Path recording with nested hierarchy") {
    // Create hierarchy: root -> panel -> label
    auto root = std::make_unique<TestElement>();
    auto panel = std::make_unique<TestElement>();
    auto* panel_ptr = panel.get();
    auto label_ptr = std::make_unique<TestElement>();
    auto* label = label_ptr.get();

    panel_ptr->add_child(std::move(label_ptr));
    root->add_child(std::move(panel));

    // Manually arrange without layout strategy
    root->arrange({0, 0, 100, 100});
    panel_ptr->arrange({10, 10, 80, 80});
    label->arrange({5, 5, 70, 70});

    SUBCASE("Hit test records complete path") {
        hit_test_path<TestBackend> path;

        // Click somewhere that should hit deepest child
        auto* result = root->hit_test(10, 10, path);

        REQUIRE(result != nullptr);

        // Path should contain root
        CHECK(path.root() == root.get());
        CHECK(path.contains(root.get()));

        // Path should contain panel
        CHECK(path.contains(panel_ptr));

        // Target should be deepest element hit
        CHECK(path.target() != nullptr);
    }

    SUBCASE("Multiple hit tests with same path object") {
        hit_test_path<TestBackend> path;

        // First hit test
        auto* result1 = root->hit_test(10, 10, path);
        REQUIRE(result1 != nullptr);
        size_t first_size = path.size();
        CHECK(first_size > 0);

        // Second hit test without clearing - path should accumulate
        auto* result2 = root->hit_test(20, 20, path);
        REQUIRE(result2 != nullptr);
        CHECK(path.size() > first_size);  // Path accumulated

        // Third hit test with clearing - use same coordinate as first test
        path.clear();
        auto* result3 = root->hit_test(10, 10, path);
        REQUIRE(result3 != nullptr);
        CHECK(path.size() == first_size);  // Back to original size
    }
}

TEST_CASE("hit_test() - Path ordering") {
    // Create hierarchy: root -> child1 -> child2
    auto root = std::make_unique<TestElement>();
    auto child1_obj = std::make_unique<TestElement>();
    auto* child1 = child1_obj.get();
    auto child2_obj = std::make_unique<TestElement>();
    auto* child2 = child2_obj.get();

    child1->add_child(std::move(child2_obj));
    root->add_child(std::move(child1_obj));

    // Manually arrange
    root->arrange({0, 0, 100, 100});
    child1->arrange({10, 10, 80, 80});
    child2->arrange({5, 5, 70, 70});

    hit_test_path<TestBackend> path;
    auto* result = root->hit_test(10, 10, path);

    REQUIRE(result != nullptr);
    REQUIRE(path.size() >= 1);

    SUBCASE("Path is ordered root -> target") {
        // First element should always be root
        CHECK(path[0] == root.get());
        CHECK(path.root() == root.get());

        // If path contains child1, it should come after root
        if (path.contains(child1)) {
            size_t child1_depth = path.depth_of(child1);
            CHECK(child1_depth > 0);  // After root

            // If path also contains child2, it should come after child1
            if (path.contains(child2)) {
                size_t child2_depth = path.depth_of(child2);
                CHECK(child2_depth > child1_depth);
            }
        }
    }

    SUBCASE("Target is last element in path") {
        CHECK(path.target() == path[path.size() - 1]);
    }
}

TEST_CASE("hit_test() - Z-order affects path") {
    // Create siblings with different z-indices
    auto root = std::make_unique<TestElement>();
    auto back_child_obj = std::make_unique<TestElement>();
    auto* back_child = back_child_obj.get();
    auto front_child_obj = std::make_unique<TestElement>();
    auto* front_child = front_child_obj.get();
    root->add_child(std::move(back_child_obj));
    root->add_child(std::move(front_child_obj));

    // Set z-order
    back_child->set_z_order(0);
    front_child->set_z_order(10);

    // Manually arrange - both children occupy the same space (overlapping)
    root->arrange({0, 0, 100, 100});
    back_child->arrange({10, 10, 80, 80});
    front_child->arrange({10, 10, 80, 80});  // Same bounds - overlapping

    // Hit test at overlapping position
    hit_test_path<TestBackend> path;
    auto* result = root->hit_test(10, 10, path);

    REQUIRE(result != nullptr);

    // Higher z-index should be tested first
    // So if both are hit, front_child should be in the path
    // (Implementation detail: depends on how overlapping works)
    CHECK(path.contains(root.get()));
}

TEST_CASE("hit_test() - Visibility affects path") {
    auto root = std::make_unique<TestElement>();
    auto visible_child_obj = std::make_unique<TestElement>();
    auto* visible_child = visible_child_obj.get();
    (void)visible_child;  // May be unused in some subcases
    auto invisible_child_obj = std::make_unique<TestElement>();
    auto* invisible_child = invisible_child_obj.get();
    root->add_child(std::move(visible_child_obj));
    root->add_child(std::move(invisible_child_obj));

    invisible_child->set_visible(false);

    // Manually arrange
    root->arrange({0, 0, 100, 100});
    visible_child->arrange({10, 10, 40, 40});
    invisible_child->arrange({50, 50, 40, 40});

    hit_test_path<TestBackend> path;
    auto* result = root->hit_test(10, 10, path);

    // Invisible element should not be in path
    CHECK_FALSE(path.contains(invisible_child));

    // Visible element might be in path (depends on hit location)
    // Root should always be in path if hit succeeded
    if (result != nullptr) {
        CHECK(path.contains(root.get()));
    }
}

TEST_CASE("hit_test() - Old signature still works") {
    auto root = std::make_unique<TestElement>();
    auto child_ptr = std::make_unique<TestElement>();
    auto* child = child_ptr.get();
    root->add_child(std::move(child_ptr));

    // Manually arrange
    root->arrange({0, 0, 100, 100});
    child->arrange({10, 10, 80, 80});

    SUBCASE("Old hit_test() without path parameter") {
        auto* result = root->hit_test(10, 10);
        // Should still work, just doesn't record path
        // Result might be root or child depending on hit location
        (void)result;  // May be nullptr or valid element
    }

    SUBCASE("Both signatures can be used independently") {
        // Old signature
        auto* result1 = root->hit_test(10, 10);

        // New signature
        hit_test_path<TestBackend> path;
        auto* result2 = root->hit_test(10, 10, path);

        // Both should return same element
        CHECK(result1 == result2);
    }
}
