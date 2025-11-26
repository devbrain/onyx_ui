/**
 * @file test_logical_geometry.cc
 * @brief Comprehensive tests for logical geometry types (size, point, rect, thickness)
 * @author OnyxUI Framework
 * @date 2025-11-26
 */

#include <doctest/doctest.h>
#include <../../include/onyxui/core/geometry.hh>

using namespace onyxui;

// ============================================================================
// logical_size Tests
// ============================================================================

TEST_CASE("logical_size - Construction") {
    SUBCASE("Default construction") {
        logical_size size;
        CHECK(size.width.value == 0.0);
        CHECK(size.height.value == 0.0);
    }

    SUBCASE("Parameterized construction") {
        logical_size size(10.5_lu, 20.3_lu);
        CHECK(size.width.value == 10.5);
        CHECK(size.height.value == 20.3);
    }
}

TEST_CASE("logical_size - Comparison") {
    logical_size a(10.0_lu, 20.0_lu);
    logical_size b(10.0_lu, 20.0_lu);
    logical_size c(15.0_lu, 25.0_lu);

    SUBCASE("Equality") {
        CHECK(a == b);
        CHECK_FALSE(a == c);
    }

    SUBCASE("Inequality") {
        CHECK(a != c);
        CHECK_FALSE(a != b);
    }
}

TEST_CASE("logical_size - Arithmetic") {
    logical_size a(10.0_lu, 20.0_lu);
    logical_size b(5.0_lu, 10.0_lu);

    SUBCASE("Addition") {
        auto result = a + b;
        CHECK(result.width.value == 15.0);
        CHECK(result.height.value == 30.0);
    }

    SUBCASE("Subtraction") {
        auto result = a - b;
        CHECK(result.width.value == 5.0);
        CHECK(result.height.value == 10.0);
    }

    SUBCASE("Scalar multiplication") {
        auto result = a * 2.0;
        CHECK(result.width.value == 20.0);
        CHECK(result.height.value == 40.0);
    }

    SUBCASE("Left scalar multiplication") {
        auto result = 2.0 * a;
        CHECK(result.width.value == 20.0);
        CHECK(result.height.value == 40.0);
    }

    SUBCASE("Scalar division by double") {
        auto result = a / 2.0;
        CHECK(result.width.value == 5.0);
        CHECK(result.height.value == 10.0);
    }

    SUBCASE("Scalar division by int") {
        auto result = a / 2;
        CHECK(result.width.value == 5.0);
        CHECK(result.height.value == 10.0);
    }
}

TEST_CASE("logical_size - Helpers") {
    SUBCASE("is_empty - true cases") {
        CHECK(logical_size(0.0_lu, 10.0_lu).is_empty());
        CHECK(logical_size(10.0_lu, 0.0_lu).is_empty());
        CHECK(logical_size(-5.0_lu, 10.0_lu).is_empty());
    }

    SUBCASE("is_empty - false cases") {
        CHECK_FALSE(logical_size(10.0_lu, 20.0_lu).is_empty());
    }

    SUBCASE("area") {
        logical_size size(10.0_lu, 5.0_lu);
        CHECK(size.area().value == 50.0);
    }
}

// ============================================================================
// logical_point Tests
// ============================================================================

TEST_CASE("logical_point - Construction") {
    SUBCASE("Default construction") {
        logical_point point;
        CHECK(point.x.value == 0.0);
        CHECK(point.y.value == 0.0);
    }

    SUBCASE("Parameterized construction") {
        logical_point point(10.5_lu, 20.3_lu);
        CHECK(point.x.value == 10.5);
        CHECK(point.y.value == 20.3);
    }
}

TEST_CASE("logical_point - Comparison") {
    logical_point a(10.0_lu, 20.0_lu);
    logical_point b(10.0_lu, 20.0_lu);
    logical_point c(15.0_lu, 25.0_lu);

    SUBCASE("Equality") {
        CHECK(a == b);
        CHECK_FALSE(a == c);
    }

    SUBCASE("Inequality") {
        CHECK(a != c);
        CHECK_FALSE(a != b);
    }
}

TEST_CASE("logical_point - Arithmetic") {
    logical_point p(10.0_lu, 20.0_lu);
    logical_size s(5.0_lu, 10.0_lu);

    SUBCASE("Point + Size") {
        auto result = p + s;
        CHECK(result.x.value == 15.0);
        CHECK(result.y.value == 30.0);
    }

    SUBCASE("Point - Size") {
        auto result = p - s;
        CHECK(result.x.value == 5.0);
        CHECK(result.y.value == 10.0);
    }

    SUBCASE("Point - Point (vector)") {
        logical_point p2(5.0_lu, 10.0_lu);
        auto result = p - p2;
        CHECK(result.width.value == 5.0);
        CHECK(result.height.value == 10.0);
    }

    SUBCASE("Offset") {
        auto result = p.offset(5.0_lu, -3.0_lu);
        CHECK(result.x.value == 15.0);
        CHECK(result.y.value == 17.0);
    }
}

// ============================================================================
// logical_rect Tests
// ============================================================================

TEST_CASE("logical_rect - Construction") {
    SUBCASE("Default construction") {
        logical_rect rect;
        CHECK(rect.x.value == 0.0);
        CHECK(rect.y.value == 0.0);
        CHECK(rect.width.value == 0.0);
        CHECK(rect.height.value == 0.0);
    }

    SUBCASE("Parameterized construction") {
        logical_rect rect(10.0_lu, 20.0_lu, 30.0_lu, 40.0_lu);
        CHECK(rect.x.value == 10.0);
        CHECK(rect.y.value == 20.0);
        CHECK(rect.width.value == 30.0);
        CHECK(rect.height.value == 40.0);
    }

    SUBCASE("Construction from point and size") {
        logical_point pos(10.0_lu, 20.0_lu);
        logical_size size(30.0_lu, 40.0_lu);
        logical_rect rect(pos, size);
        CHECK(rect.x.value == 10.0);
        CHECK(rect.y.value == 20.0);
        CHECK(rect.width.value == 30.0);
        CHECK(rect.height.value == 40.0);
    }
}

TEST_CASE("logical_rect - Accessors") {
    logical_rect rect(10.0_lu, 20.0_lu, 30.0_lu, 40.0_lu);

    SUBCASE("position") {
        auto pos = rect.position();
        CHECK(pos.x.value == 10.0);
        CHECK(pos.y.value == 20.0);
    }

    SUBCASE("size") {
        auto size = rect.size();
        CHECK(size.width.value == 30.0);
        CHECK(size.height.value == 40.0);
    }

    SUBCASE("left/top/right/bottom") {
        CHECK(rect.left().value == 10.0);
        CHECK(rect.top().value == 20.0);
        CHECK(rect.right().value == 40.0);  // 10 + 30
        CHECK(rect.bottom().value == 60.0); // 20 + 40
    }

    SUBCASE("Corner points") {
        CHECK(rect.top_left() == logical_point(10.0_lu, 20.0_lu));
        CHECK(rect.top_right() == logical_point(40.0_lu, 20.0_lu));
        CHECK(rect.bottom_left() == logical_point(10.0_lu, 60.0_lu));
        CHECK(rect.bottom_right() == logical_point(40.0_lu, 60.0_lu));
    }

    SUBCASE("center") {
        auto center = rect.center();
        CHECK(center.x.value == 25.0);  // 10 + 30/2
        CHECK(center.y.value == 40.0);  // 20 + 40/2
    }
}

TEST_CASE("logical_rect - Predicates") {
    logical_rect rect(10.0_lu, 20.0_lu, 30.0_lu, 40.0_lu);

    SUBCASE("is_empty") {
        CHECK_FALSE(rect.is_empty());
        CHECK(logical_rect(10.0_lu, 20.0_lu, 0.0_lu, 40.0_lu).is_empty());
        CHECK(logical_rect(10.0_lu, 20.0_lu, 30.0_lu, 0.0_lu).is_empty());
    }

    SUBCASE("contains point") {
        CHECK(rect.contains(logical_point(15.0_lu, 25.0_lu)));
        CHECK(rect.contains(logical_point(10.0_lu, 20.0_lu)));  // Top-left inclusive
        CHECK_FALSE(rect.contains(logical_point(40.0_lu, 60.0_lu)));  // Bottom-right exclusive
        CHECK_FALSE(rect.contains(logical_point(5.0_lu, 25.0_lu)));  // Outside
    }

    SUBCASE("contains rect") {
        logical_rect inner(15.0_lu, 25.0_lu, 10.0_lu, 10.0_lu);
        CHECK(rect.contains(inner));

        logical_rect outer(5.0_lu, 15.0_lu, 50.0_lu, 60.0_lu);
        CHECK_FALSE(rect.contains(outer));
    }

    SUBCASE("intersects") {
        logical_rect overlap(30.0_lu, 40.0_lu, 20.0_lu, 30.0_lu);
        CHECK(rect.intersects(overlap));

        logical_rect no_overlap(50.0_lu, 70.0_lu, 10.0_lu, 10.0_lu);
        CHECK_FALSE(rect.intersects(no_overlap));
    }
}

TEST_CASE("logical_rect - Transformations") {
    logical_rect rect(10.0_lu, 20.0_lu, 30.0_lu, 40.0_lu);

    SUBCASE("offset by coordinates") {
        auto result = rect.offset(5.0_lu, -3.0_lu);
        CHECK(result.x.value == 15.0);
        CHECK(result.y.value == 17.0);
        CHECK(result.width.value == 30.0);
        CHECK(result.height.value == 40.0);
    }

    SUBCASE("offset by size") {
        auto result = rect.offset(logical_size(5.0_lu, -3.0_lu));
        CHECK(result.x.value == 15.0);
        CHECK(result.y.value == 17.0);
    }

    SUBCASE("inflate") {
        auto result = rect.inflate(5.0_lu, 10.0_lu);
        CHECK(result.x.value == 5.0);   // 10 - 5
        CHECK(result.y.value == 10.0);  // 20 - 10
        CHECK(result.width.value == 40.0);  // 30 + 5*2
        CHECK(result.height.value == 60.0); // 40 + 10*2
    }

    SUBCASE("deflate") {
        auto result = rect.deflate(5.0_lu, 10.0_lu);
        CHECK(result.x.value == 15.0);  // 10 + 5
        CHECK(result.y.value == 30.0);  // 20 + 10
        CHECK(result.width.value == 20.0);  // 30 - 5*2
        CHECK(result.height.value == 20.0); // 40 - 10*2
    }
}

TEST_CASE("logical_rect - Set operations") {
    logical_rect a(10.0_lu, 20.0_lu, 30.0_lu, 40.0_lu);
    logical_rect b(25.0_lu, 35.0_lu, 30.0_lu, 40.0_lu);

    SUBCASE("intersect") {
        auto result = a.intersect(b);
        CHECK(result.left().value == 25.0);
        CHECK(result.top().value == 35.0);
        CHECK(result.right().value == 40.0);
        CHECK(result.bottom().value == 60.0);
        CHECK(result.width.value == 15.0);
        CHECK(result.height.value == 25.0);
    }

    SUBCASE("intersect (no overlap)") {
        logical_rect c(100.0_lu, 100.0_lu, 10.0_lu, 10.0_lu);
        auto result = a.intersect(c);
        CHECK(result.is_empty());
    }

    SUBCASE("union_with") {
        auto result = a.union_with(b);
        CHECK(result.left().value == 10.0);
        CHECK(result.top().value == 20.0);
        CHECK(result.right().value == 55.0);
        CHECK(result.bottom().value == 75.0);
    }

    SUBCASE("union_with empty rect") {
        logical_rect empty;
        CHECK(a.union_with(empty) == a);
        CHECK(empty.union_with(a) == a);
    }
}

TEST_CASE("logical_rect - Comparison") {
    logical_rect a(10.0_lu, 20.0_lu, 30.0_lu, 40.0_lu);
    logical_rect b(10.0_lu, 20.0_lu, 30.0_lu, 40.0_lu);
    logical_rect c(15.0_lu, 25.0_lu, 35.0_lu, 45.0_lu);

    SUBCASE("Equality") {
        CHECK(a == b);
        CHECK_FALSE(a == c);
    }

    SUBCASE("Inequality") {
        CHECK(a != c);
        CHECK_FALSE(a != b);
    }
}

// ============================================================================
// logical_thickness Tests
// ============================================================================

TEST_CASE("logical_thickness - Construction") {
    SUBCASE("Default construction") {
        logical_thickness t;
        CHECK(t.left.value == 0.0);
        CHECK(t.top.value == 0.0);
        CHECK(t.right.value == 0.0);
        CHECK(t.bottom.value == 0.0);
    }

    SUBCASE("Uniform thickness") {
        logical_thickness t(5.0_lu);
        CHECK(t.left.value == 5.0);
        CHECK(t.top.value == 5.0);
        CHECK(t.right.value == 5.0);
        CHECK(t.bottom.value == 5.0);
    }

    SUBCASE("Horizontal/vertical thickness") {
        logical_thickness t(10.0_lu, 20.0_lu);
        CHECK(t.left.value == 10.0);
        CHECK(t.right.value == 10.0);
        CHECK(t.top.value == 20.0);
        CHECK(t.bottom.value == 20.0);
    }

    SUBCASE("Individual sides") {
        logical_thickness t(1.0_lu, 2.0_lu, 3.0_lu, 4.0_lu);
        CHECK(t.left.value == 1.0);
        CHECK(t.top.value == 2.0);
        CHECK(t.right.value == 3.0);
        CHECK(t.bottom.value == 4.0);
    }
}

TEST_CASE("logical_thickness - Helpers") {
    logical_thickness t(1.0_lu, 2.0_lu, 3.0_lu, 4.0_lu);

    SUBCASE("horizontal") {
        CHECK(t.horizontal().value == 4.0);  // 1 + 3
    }

    SUBCASE("vertical") {
        CHECK(t.vertical().value == 6.0);  // 2 + 4
    }

    SUBCASE("total_size") {
        auto size = t.total_size();
        CHECK(size.width.value == 4.0);
        CHECK(size.height.value == 6.0);
    }
}

TEST_CASE("logical_thickness - Comparison") {
    logical_thickness a(1.0_lu, 2.0_lu, 3.0_lu, 4.0_lu);
    logical_thickness b(1.0_lu, 2.0_lu, 3.0_lu, 4.0_lu);
    logical_thickness c(5.0_lu, 6.0_lu, 7.0_lu, 8.0_lu);

    SUBCASE("Equality") {
        CHECK(a == b);
        CHECK_FALSE(a == c);
    }

    SUBCASE("Inequality") {
        CHECK(a != c);
        CHECK_FALSE(a != b);
    }
}

// ============================================================================
// Practical Usage Tests
// ============================================================================

TEST_CASE("logical_geometry - Practical usage") {
    SUBCASE("Centering a window") {
        logical_size screen_size(80.0_lu, 25.0_lu);
        logical_size window_size(60.0_lu, 20.0_lu);

        auto x = (screen_size.width - window_size.width) / 2;
        auto y = (screen_size.height - window_size.height) / 2;

        logical_rect window(x, y, window_size.width, window_size.height);

        CHECK(window.x.value == 10.0);
        CHECK(window.y.value == 2.5);
    }

    SUBCASE("Content area with padding") {
        logical_rect outer(0.0_lu, 0.0_lu, 100.0_lu, 50.0_lu);
        logical_thickness padding(5.0_lu, 10.0_lu, 5.0_lu, 10.0_lu);

        // Content rect = deflate by padding
        auto content = outer.deflate(padding.left, padding.top);
        content.width = content.width - padding.horizontal() / 2;
        content.height = content.height - padding.vertical() / 2;

        CHECK(content.x.value == 5.0);
        CHECK(content.y.value == 10.0);
        CHECK(content.width.value == 85.0);  // 100 - 5 - 5 - 5
        CHECK(content.height.value == 20.0); // 50 - 10 - 10 - 10
    }

    SUBCASE("Clipping rect intersection") {
        logical_rect widget(10.0_lu, 10.0_lu, 50.0_lu, 50.0_lu);
        logical_rect clip(30.0_lu, 30.0_lu, 40.0_lu, 40.0_lu);

        auto visible = widget.intersect(clip);

        CHECK(visible.x.value == 30.0);
        CHECK(visible.y.value == 30.0);
        CHECK(visible.width.value == 30.0);
        CHECK(visible.height.value == 30.0);
    }
}
