/**
 * @file test_coordinates.cc
 * @brief Unit tests for strong coordinate types
 * @author OnyxUI Architecture Team
 * @date 2025-11-11
 *
 * @details
 * Tests for the strong coordinate type system that provides compile-time
 * safety for absolute (screen-space) vs relative (widget-local) coordinates.
 */

#include <doctest/doctest.h>
#include <utils/test_backend.hh>
#include <utils/test_helpers.hh>
#include <onyxui/core/element.hh>
#include <onyxui/geometry/coordinates.hh>
#include <onyxui/widgets/containers/panel.hh>
#include <onyxui/widgets/label.hh>

using namespace onyxui;
using namespace onyxui::geometry;
using Backend = test_backend;

TEST_SUITE("geometry::coordinates") {

    // ====================
    // CONSTRUCTION TESTS
    // ====================

    TEST_CASE("strong_point - Construction") {
        Backend::point pt{10, 20};

        absolute_point<Backend> abs_pt{pt};
        CHECK(abs_pt.x() == 10);
        CHECK(abs_pt.y() == 20);

        relative_point<Backend> rel_pt{pt};
        CHECK(rel_pt.x() == 10);
        CHECK(rel_pt.y() == 20);
    }

    TEST_CASE("strong_rect - Construction") {
        Backend::rect r{10, 20, 100, 50};

        absolute_rect<Backend> abs_rect{r};
        CHECK(abs_rect.x() == 10);
        CHECK(abs_rect.y() == 20);
        CHECK(abs_rect.width() == 100);
        CHECK(abs_rect.height() == 50);

        relative_rect<Backend> rel_rect{r};
        CHECK(rel_rect.x() == 10);
        CHECK(rel_rect.y() == 20);
        CHECK(rel_rect.width() == 100);
        CHECK(rel_rect.height() == 50);
    }

    // ====================
    // TYPE SAFETY TESTS
    // ====================

    TEST_CASE("Type safety - Points cannot be implicitly converted") {
        Backend::point pt{10, 20};
        absolute_point<Backend> abs_pt{pt};
        relative_point<Backend> rel_pt{pt};

        // These should NOT compile (verify type safety)
        // absolute_point<Backend> abs2 = rel_pt;  // Compiler error!
        // relative_point<Backend> rel2 = abs_pt;  // Compiler error!

        // Explicit conversion via get() is allowed
        Backend::point extracted = abs_pt.get();
        CHECK(point_utils::get_x(extracted) == 10);
    }

    TEST_CASE("Type safety - Rects cannot be implicitly converted") {
        Backend::rect r{10, 20, 100, 50};
        absolute_rect<Backend> abs_rect{r};
        relative_rect<Backend> rel_rect{r};

        // These should NOT compile (verify type safety)
        // absolute_rect<Backend> abs2 = rel_rect;  // Compiler error!
        // relative_rect<Backend> rel2 = abs_rect;  // Compiler error!

        // Explicit conversion via get() is allowed
        Backend::rect extracted = abs_rect.get();
        CHECK(rect_utils::get_x(extracted) == 10);
    }

    // ====================
    // EQUALITY TESTS
    // ====================

    TEST_CASE("strong_point - Equality") {
        Backend::point pt1{10, 20};
        Backend::point pt2{10, 20};
        Backend::point pt3{15, 25};

        absolute_point<Backend> abs1{pt1};
        absolute_point<Backend> abs2{pt2};
        absolute_point<Backend> abs3{pt3};

        CHECK(abs1 == abs2);
        CHECK(abs1 != abs3);
    }

    TEST_CASE("strong_rect - Equality") {
        Backend::rect r1{10, 20, 100, 50};
        Backend::rect r2{10, 20, 100, 50};
        Backend::rect r3{15, 25, 110, 60};

        relative_rect<Backend> rel1{r1};
        relative_rect<Backend> rel2{r2};
        relative_rect<Backend> rel3{r3};

        CHECK(rel1 == rel2);
        CHECK(rel1 != rel3);
    }

    // ====================
    // HELPER METHOD TESTS
    // ====================

    TEST_CASE("strong_rect - contains() method") {
        Backend::rect r{10, 20, 100, 50};
        absolute_rect<Backend> abs_rect{r};

        Backend::point inside{50, 40};
        Backend::point outside{5, 5};
        Backend::point on_edge{10, 20};  // Top-left corner (inside)

        CHECK(abs_rect.contains(absolute_point<Backend>{inside}));
        CHECK_FALSE(abs_rect.contains(absolute_point<Backend>{outside}));
        CHECK(abs_rect.contains(absolute_point<Backend>{on_edge}));
    }

    TEST_CASE("strong_rect - Dimension accessors") {
        Backend::rect r{10, 20, 100, 50};
        relative_rect<Backend> rel_rect{r};

        CHECK(rel_rect.x() == 10);
        CHECK(rel_rect.y() == 20);
        CHECK(rel_rect.width() == 100);
        CHECK(rel_rect.height() == 50);
    }

    // ====================
    // COORDINATE CONVERSION TESTS
    // ====================

    TEST_CASE("Coordinate conversion - relative to absolute (single level)") {
        // Create parent at (100, 50)
        panel<Backend> parent;
        parent.arrange(logical_rect{100_lu, 50_lu, 200_lu, 100_lu});

        // Create child at (10, 5) relative to parent
        auto* child = parent.emplace_child<label>("Child");
        child->arrange(logical_rect{10_lu, 5_lu, 50_lu, 20_lu});

        // Convert child's relative bounds to absolute
        const auto child_logical = child->bounds();
        typename Backend::rect_type child_backend_rect;
        rect_utils::set_bounds(child_backend_rect,
            child_logical.x.to_int(),
            child_logical.y.to_int(),
            child_logical.width.to_int(),
            child_logical.height.to_int());
        relative_rect<Backend> child_rel{child_backend_rect};

        absolute_rect<Backend> child_abs = to_absolute(child_rel, child);

        // Child should be at (110, 55) in absolute coordinates
        CHECK(child_abs.x() == 110);
        CHECK(child_abs.y() == 55);
        CHECK(child_abs.width() == 50);
        CHECK(child_abs.height() == 20);
    }

    TEST_CASE("Coordinate conversion - relative to absolute (nested)") {
        // Create hierarchy: root -> parent -> child
        // Root at (0, 0)
        panel<Backend> root;
        root.arrange(logical_rect{0_lu, 0_lu, 400_lu, 300_lu});

        // Parent at (50, 30) relative to root
        auto* parent = root.emplace_child<panel>();
        parent->arrange(logical_rect{50_lu, 30_lu, 200_lu, 150_lu});

        // Child at (10, 5) relative to parent
        auto* child = parent->emplace_child<label>("Child");
        child->arrange(logical_rect{10_lu, 5_lu, 50_lu, 20_lu});

        // Convert child's relative bounds to absolute
        const auto child_logical = child->bounds();
        typename Backend::rect_type child_rel_backend;
        rect_utils::set_bounds(child_rel_backend,
            child_logical.x.to_int(),
            child_logical.y.to_int(),
            child_logical.width.to_int(),
            child_logical.height.to_int());
        relative_rect<Backend> child_rel{child_rel_backend};

        absolute_rect<Backend> child_abs = to_absolute(child_rel, child);

        // Child should be at (60, 35) in absolute coordinates
        // (root 0+parent 50+child 10, root 0+parent 30+child 5)
        CHECK(child_abs.x() == 60);
        CHECK(child_abs.y() == 35);
        CHECK(child_abs.width() == 50);
        CHECK(child_abs.height() == 20);
    }

    TEST_CASE("Coordinate conversion - absolute to relative") {
        // Create parent at (100, 50)
        panel<Backend> parent;
        parent.arrange(logical_rect{100_lu, 50_lu, 200_lu, 100_lu});

        // Absolute rect at (110, 60)
        Backend::rect const abs_backend{110, 60, 50, 30};
        absolute_rect<Backend> abs{abs_backend};

        // Convert to relative coordinates
        relative_rect<Backend> rel = to_relative(abs, &parent);

        // Should be (10, 10) relative to parent
        CHECK(rel.x() == 10);
        CHECK(rel.y() == 10);
        CHECK(rel.width() == 50);
        CHECK(rel.height() == 30);
    }

    TEST_CASE("Coordinate conversion - point relative to absolute") {
        // Create parent at (100, 50)
        panel<Backend> parent;
        parent.arrange(logical_rect{100_lu, 50_lu, 200_lu, 100_lu});

        // Create child at (10, 5) relative to parent
        auto* child = parent.emplace_child<label>("Child");
        child->arrange(logical_rect{10_lu, 5_lu, 50_lu, 20_lu});

        // Point at (5, 8) relative to child
        Backend::point pt_backend{5, 8};
        relative_point<Backend> rel_pt{pt_backend};

        absolute_point<Backend> abs_pt = to_absolute(rel_pt, child);

        // Should be at (115, 63) in absolute coordinates
        // (parent 100 + child 10 + point 5, parent 50 + child 5 + point 8)
        CHECK(abs_pt.x() == 115);
        CHECK(abs_pt.y() == 63);
    }

    TEST_CASE("Coordinate conversion - point absolute to relative") {
        // Create parent at (100, 50)
        panel<Backend> parent;
        parent.arrange(logical_rect{100_lu, 50_lu, 200_lu, 100_lu});

        // Absolute point at (115, 63)
        Backend::point pt_backend{115, 63};
        absolute_point<Backend> abs_pt{pt_backend};

        relative_point<Backend> rel_pt = to_relative(abs_pt, &parent);

        // Should be (15, 13) relative to parent
        CHECK(rel_pt.x() == 15);
        CHECK(rel_pt.y() == 13);
    }

    TEST_CASE("Coordinate conversion - root element (no parent)") {
        // Root element has no parent
        panel<Backend> root;
        root.arrange(logical_rect{10_lu, 20_lu, 200_lu, 100_lu});

        // For root element, relative == absolute
        const auto root_logical = root.bounds();
        typename Backend::rect_type root_rel_backend;
        rect_utils::set_bounds(root_rel_backend,
            root_logical.x.to_int(),
            root_logical.y.to_int(),
            root_logical.width.to_int(),
            root_logical.height.to_int());
        relative_rect<Backend> root_rel{root_rel_backend};

        absolute_rect<Backend> root_abs = to_absolute(root_rel, &root);

        // Same coordinates (no parent to add)
        CHECK(root_abs.x() == root_rel.x());
        CHECK(root_abs.y() == root_rel.y());
    }

    // ====================
    // EDGE CASE TESTS
    // ====================

    TEST_CASE("Edge case - Zero-sized rectangles") {
        Backend::rect r{10, 20, 0, 0};
        relative_rect<Backend> rel{r};

        CHECK(rel.x() == 10);
        CHECK(rel.y() == 20);
        CHECK(rel.width() == 0);
        CHECK(rel.height() == 0);
    }

    TEST_CASE("Edge case - Negative coordinates") {
        Backend::rect r{-10, -20, 100, 50};
        absolute_rect<Backend> abs{r};

        CHECK(abs.x() == -10);
        CHECK(abs.y() == -20);
        CHECK(abs.width() == 100);
        CHECK(abs.height() == 50);
    }

    TEST_CASE("Edge case - Large coordinates") {
        Backend::rect r{10000, 20000, 5000, 3000};
        relative_rect<Backend> rel{r};

        CHECK(rel.x() == 10000);
        CHECK(rel.y() == 20000);
        CHECK(rel.width() == 5000);
        CHECK(rel.height() == 3000);
    }

    // ====================
    // INTEGRATION TESTS
    // ====================

    TEST_CASE("Integration - Complex hierarchy") {
        // Create a 3-level deep hierarchy
        panel<Backend> root;
        root.arrange(logical_rect{0_lu, 0_lu, 800_lu, 600_lu});

        auto* level1 = root.emplace_child<panel>();
        level1->arrange(logical_rect{50_lu, 40_lu, 400_lu, 300_lu});

        auto* level2 = level1->emplace_child<panel>();
        level2->arrange(logical_rect{30_lu, 20_lu, 200_lu, 150_lu});

        auto* level3 = level2->emplace_child<label>("Nested");
        level3->arrange(logical_rect{10_lu, 5_lu, 100_lu, 75_lu});

        // Convert deepest child to absolute
        const auto level3_logical = level3->bounds();
        typename Backend::rect_type level3_rel_backend;
        rect_utils::set_bounds(level3_rel_backend,
            level3_logical.x.to_int(),
            level3_logical.y.to_int(),
            level3_logical.width.to_int(),
            level3_logical.height.to_int());
        relative_rect<Backend> level3_rel{level3_rel_backend};

        absolute_rect<Backend> level3_abs = to_absolute(level3_rel, level3);

        // Should be at (90, 65) = 50+30+10, 40+20+5
        CHECK(level3_abs.x() == 90);
        CHECK(level3_abs.y() == 65);
        CHECK(level3_abs.width() == 100);
        CHECK(level3_abs.height() == 75);

        // Convert back to relative
        relative_rect<Backend> back_to_rel = to_relative(level3_abs, level2);

        // Should match original relative coords
        CHECK(back_to_rel.x() == 10);
        CHECK(back_to_rel.y() == 5);
        CHECK(back_to_rel.width() == 100);
        CHECK(back_to_rel.height() == 75);
    }

    TEST_CASE("Integration - Sibling elements") {
        // Create parent with multiple children
        panel<Backend> parent;
        parent.arrange(logical_rect{100_lu, 50_lu, 300_lu, 200_lu});

        auto* child1 = parent.emplace_child<label>("Child1");
        child1->arrange(logical_rect{10_lu, 10_lu, 80_lu, 40_lu});

        auto* child2 = parent.emplace_child<label>("Child2");
        child2->arrange(logical_rect{10_lu, 60_lu, 80_lu, 40_lu});

        auto* child3 = parent.emplace_child<label>("Child3");
        child3->arrange(logical_rect{100_lu, 10_lu, 80_lu, 40_lu});

        // Convert all children to absolute
        auto c1_logical = child1->bounds();
        auto c2_logical = child2->bounds();
        auto c3_logical = child3->bounds();

        typename Backend::rect_type c1_rel, c2_rel, c3_rel;
        rect_utils::set_bounds(c1_rel, c1_logical.x.to_int(), c1_logical.y.to_int(),
                              c1_logical.width.to_int(), c1_logical.height.to_int());
        rect_utils::set_bounds(c2_rel, c2_logical.x.to_int(), c2_logical.y.to_int(),
                              c2_logical.width.to_int(), c2_logical.height.to_int());
        rect_utils::set_bounds(c3_rel, c3_logical.x.to_int(), c3_logical.y.to_int(),
                              c3_logical.width.to_int(), c3_logical.height.to_int());

        absolute_rect<Backend> c1_abs = to_absolute(relative_rect<Backend>{c1_rel}, child1);
        absolute_rect<Backend> c2_abs = to_absolute(relative_rect<Backend>{c2_rel}, child2);
        absolute_rect<Backend> c3_abs = to_absolute(relative_rect<Backend>{c3_rel}, child3);

        // child1 at (110, 60)
        CHECK(c1_abs.x() == 110);
        CHECK(c1_abs.y() == 60);

        // child2 at (110, 110) - below child1
        CHECK(c2_abs.x() == 110);
        CHECK(c2_abs.y() == 110);

        // child3 at (200, 60) - to the right
        CHECK(c3_abs.x() == 200);
        CHECK(c3_abs.y() == 60);
    }

} // TEST_SUITE("geometry::coordinates")
