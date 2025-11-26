/**
 * @file test_udim.cc
 * @brief Comprehensive tests for UDim (Unified Dimensions)
 * @author OnyxUI Framework
 * @date 2025-11-26
 */

#include <doctest/doctest.h>
#include <../../include/onyxui/core/udim.hh>

using namespace onyxui;

// ============================================================================
// udim Construction Tests
// ============================================================================

TEST_CASE("udim - Construction") {
    SUBCASE("Default construction") {
        udim u;
        CHECK(u.scale == 0.0);
        CHECK(u.offset.value == 0.0);
    }

    SUBCASE("Construction with double offset") {
        udim u(0.5, 10.0);
        CHECK(u.scale == 0.5);
        CHECK(u.offset.value == 10.0);
    }

    SUBCASE("Construction with logical_unit offset") {
        udim u(0.75, 20.5_lu);
        CHECK(u.scale == 0.75);
        CHECK(u.offset.value == 20.5);
    }
}

// ============================================================================
// udim Resolution Tests
// ============================================================================

TEST_CASE("udim - Resolution") {
    SUBCASE("Pure percentage (50%)") {
        udim u(0.5, 0.0);
        auto result = u.resolve(100.0_lu);
        CHECK(result.value == 50.0);
    }

    SUBCASE("Pure absolute (10 units)") {
        udim u(0.0, 10.0_lu);
        auto result = u.resolve(100.0_lu);
        CHECK(result.value == 10.0);
    }

    SUBCASE("Mixed: 50% + 10 units") {
        udim u(0.5, 10.0_lu);
        auto result = u.resolve(100.0_lu);
        CHECK(result.value == 60.0);  // (100 * 0.5) + 10 = 60
    }

    SUBCASE("Mixed: 100% - 20 units") {
        udim u(1.0, -20.0_lu);
        auto result = u.resolve(100.0_lu);
        CHECK(result.value == 80.0);  // (100 * 1.0) - 20 = 80
    }

    SUBCASE("Mixed: 25% + 5 units") {
        udim u(0.25, 5.0_lu);
        auto result = u.resolve(80.0_lu);
        CHECK(result.value == 25.0);  // (80 * 0.25) + 5 = 25
    }

    SUBCASE("Fractional parent size") {
        udim u(0.5, 0.0);
        auto result = u.resolve(33.33_lu);
        CHECK(result.value == doctest::Approx(16.665));
    }
}

// ============================================================================
// udim Arithmetic Tests
// ============================================================================

TEST_CASE("udim - Addition") {
    udim a(0.5, 10.0_lu);
    udim b(0.25, 5.0_lu);

    SUBCASE("Basic addition") {
        auto result = a + b;
        CHECK(result.scale == 0.75);
        CHECK(result.offset.value == 15.0);
    }

    SUBCASE("Compound addition") {
        udim c(0.5, 10.0_lu);
        c += udim(0.25, 5.0_lu);
        CHECK(c.scale == 0.75);
        CHECK(c.offset.value == 15.0);
    }
}

TEST_CASE("udim - Subtraction") {
    udim a(0.75, 20.0_lu);
    udim b(0.25, 5.0_lu);

    SUBCASE("Basic subtraction") {
        auto result = a - b;
        CHECK(result.scale == 0.5);
        CHECK(result.offset.value == 15.0);
    }

    SUBCASE("Unary minus") {
        auto result = -a;
        CHECK(result.scale == -0.75);
        CHECK(result.offset.value == -20.0);
    }

    SUBCASE("Compound subtraction") {
        udim c(0.75, 20.0_lu);
        c -= udim(0.25, 5.0_lu);
        CHECK(c.scale == 0.5);
        CHECK(c.offset.value == 15.0);
    }
}

TEST_CASE("udim - Multiplication") {
    udim u(0.5, 10.0_lu);

    SUBCASE("Scalar multiplication") {
        auto result = u * 2.0;
        CHECK(result.scale == 1.0);
        CHECK(result.offset.value == 20.0);
    }

    SUBCASE("Left scalar multiplication") {
        auto result = 2.0 * u;
        CHECK(result.scale == 1.0);
        CHECK(result.offset.value == 20.0);
    }

    SUBCASE("Compound multiplication") {
        udim c(0.5, 10.0_lu);
        c *= 2.0;
        CHECK(c.scale == 1.0);
        CHECK(c.offset.value == 20.0);
    }
}

TEST_CASE("udim - Division") {
    udim u(1.0, 20.0_lu);

    SUBCASE("Division by double") {
        auto result = u / 2.0;
        CHECK(result.scale == 0.5);
        CHECK(result.offset.value == 10.0);
    }

    SUBCASE("Division by int") {
        auto result = u / 2;
        CHECK(result.scale == 0.5);
        CHECK(result.offset.value == 10.0);
    }

    SUBCASE("Compound division by double") {
        udim c(1.0, 20.0_lu);
        c /= 2.0;
        CHECK(c.scale == 0.5);
        CHECK(c.offset.value == 10.0);
    }

    SUBCASE("Compound division by int") {
        udim c(1.0, 20.0_lu);
        c /= 2;
        CHECK(c.scale == 0.5);
        CHECK(c.offset.value == 10.0);
    }
}

// ============================================================================
// udim Comparison Tests
// ============================================================================

TEST_CASE("udim - Comparison") {
    udim a(0.5, 10.0_lu);
    udim b(0.5, 10.0_lu);
    udim c(0.75, 15.0_lu);

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
// udim2 Tests
// ============================================================================

TEST_CASE("udim2 - Construction") {
    SUBCASE("Default construction") {
        udim2 u;
        CHECK(u.width.scale == 0.0);
        CHECK(u.width.offset.value == 0.0);
        CHECK(u.height.scale == 0.0);
        CHECK(u.height.offset.value == 0.0);
    }

    SUBCASE("Construction from udim") {
        udim w(0.5, 10.0_lu);
        udim h(0.75, 20.0_lu);
        udim2 u(w, h);
        CHECK(u.width == w);
        CHECK(u.height == h);
    }

    SUBCASE("Construction from raw values") {
        udim2 u(0.5, 10.0_lu, 0.75, 20.0_lu);
        CHECK(u.width.scale == 0.5);
        CHECK(u.width.offset.value == 10.0);
        CHECK(u.height.scale == 0.75);
        CHECK(u.height.offset.value == 20.0);
    }
}

TEST_CASE("udim2 - Resolution") {
    SUBCASE("Resolve to logical_size") {
        udim2 u(0.5, 10.0_lu, 0.75, 5.0_lu);
        logical_size parent(100.0_lu, 80.0_lu);
        auto result = u.resolve(parent);

        CHECK(result.width.value == 60.0);   // (100 * 0.5) + 10 = 60
        CHECK(result.height.value == 65.0);  // (80 * 0.75) + 5 = 65
    }

    SUBCASE("100% - 20 units") {
        udim2 u(1.0, -20.0_lu, 1.0, -20.0_lu);
        logical_size parent(100.0_lu, 100.0_lu);
        auto result = u.resolve(parent);

        CHECK(result.width.value == 80.0);
        CHECK(result.height.value == 80.0);
    }
}

TEST_CASE("udim2 - Comparison") {
    udim2 a(0.5, 10.0_lu, 0.75, 20.0_lu);
    udim2 b(0.5, 10.0_lu, 0.75, 20.0_lu);
    udim2 c(0.6, 12.0_lu, 0.8, 25.0_lu);

    SUBCASE("Equality") {
        CHECK(a == b);
        CHECK_FALSE(a == c);
    }

    SUBCASE("Inequality") {
        CHECK(a != c);
        CHECK_FALSE(a != b);
    }
}

TEST_CASE("udim2 - Arithmetic") {
    udim2 a(0.5, 10.0_lu, 0.75, 20.0_lu);
    udim2 b(0.25, 5.0_lu, 0.1, 3.0_lu);

    SUBCASE("Addition") {
        auto result = a + b;
        CHECK(result.width.scale == 0.75);
        CHECK(result.width.offset.value == 15.0);
        CHECK(result.height.scale == 0.85);
        CHECK(result.height.offset.value == 23.0);
    }

    SUBCASE("Subtraction") {
        auto result = a - b;
        CHECK(result.width.scale == 0.25);
        CHECK(result.width.offset.value == 5.0);
        CHECK(result.height.scale == 0.65);
        CHECK(result.height.offset.value == 17.0);
    }

    SUBCASE("Scalar multiplication") {
        auto result = a * 2.0;
        CHECK(result.width.scale == 1.0);
        CHECK(result.width.offset.value == 20.0);
        CHECK(result.height.scale == 1.5);
        CHECK(result.height.offset.value == 40.0);
    }

    SUBCASE("Division by double") {
        auto result = a / 2.0;
        CHECK(result.width.scale == 0.25);
        CHECK(result.width.offset.value == 5.0);
        CHECK(result.height.scale == 0.375);
        CHECK(result.height.offset.value == 10.0);
    }

    SUBCASE("Division by int") {
        auto result = a / 2;
        CHECK(result.width.scale == 0.25);
        CHECK(result.width.offset.value == 5.0);
        CHECK(result.height.scale == 0.375);
        CHECK(result.height.offset.value == 10.0);
    }
}

// ============================================================================
// udim_rect Tests
// ============================================================================

TEST_CASE("udim_rect - Construction") {
    SUBCASE("Default construction") {
        udim_rect r;
        CHECK(r.x.scale == 0.0);
        CHECK(r.y.scale == 0.0);
        CHECK(r.width.scale == 0.0);
        CHECK(r.height.scale == 0.0);
    }

    SUBCASE("Construction from udim") {
        udim x(0.1, 5.0_lu);
        udim y(0.2, 10.0_lu);
        udim w(0.5, 0.0_lu);
        udim h(0.75, 0.0_lu);
        udim_rect r(x, y, w, h);

        CHECK(r.x == x);
        CHECK(r.y == y);
        CHECK(r.width == w);
        CHECK(r.height == h);
    }
}

TEST_CASE("udim_rect - Resolution") {
    SUBCASE("Resolve to logical_rect") {
        udim_rect r(
            udim(0.1, 5.0_lu),   // x = 10% + 5
            udim(0.2, 10.0_lu),  // y = 20% + 10
            udim(0.5, 0.0_lu),   // width = 50%
            udim(0.75, -5.0_lu)  // height = 75% - 5
        );

        logical_size parent(100.0_lu, 80.0_lu);
        auto result = r.resolve(parent);

        CHECK(result.x.value == 15.0);      // (100 * 0.1) + 5 = 15
        CHECK(result.y.value == 26.0);      // (80 * 0.2) + 10 = 26
        CHECK(result.width.value == 50.0);  // (100 * 0.5) = 50
        CHECK(result.height.value == 55.0); // (80 * 0.75) - 5 = 55
    }

    SUBCASE("Centered rect (50% - half size)") {
        // Center a 60×40 rect in 100×80 parent
        // x = 50% - 30, y = 50% - 20
        udim_rect r(
            udim(0.5, -30.0_lu),
            udim(0.5, -20.0_lu),
            udim(0.0, 60.0_lu),
            udim(0.0, 40.0_lu)
        );

        logical_size parent(100.0_lu, 80.0_lu);
        auto result = r.resolve(parent);

        CHECK(result.x.value == 20.0);      // (100 * 0.5) - 30 = 20
        CHECK(result.y.value == 20.0);      // (80 * 0.5) - 20 = 20
        CHECK(result.width.value == 60.0);
        CHECK(result.height.value == 40.0);
    }
}

TEST_CASE("udim_rect - Comparison") {
    udim_rect a(udim(0.1, 5.0_lu), udim(0.2, 10.0_lu),
                udim(0.5, 0.0_lu), udim(0.75, 0.0_lu));
    udim_rect b(udim(0.1, 5.0_lu), udim(0.2, 10.0_lu),
                udim(0.5, 0.0_lu), udim(0.75, 0.0_lu));
    udim_rect c(udim(0.2, 10.0_lu), udim(0.3, 15.0_lu),
                udim(0.6, 5.0_lu), udim(0.8, 10.0_lu));

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
// Helper Function Tests
// ============================================================================

TEST_CASE("udim - Helper functions") {
    SUBCASE("percent()") {
        auto u = percent(0.5);
        CHECK(u.scale == 0.5);
        CHECK(u.offset.value == 0.0);
    }

    SUBCASE("absolute()") {
        auto u = absolute(10.0_lu);
        CHECK(u.scale == 0.0);
        CHECK(u.offset.value == 10.0);
    }

    SUBCASE("full()") {
        auto u = full();
        CHECK(u.scale == 1.0);
        CHECK(u.offset.value == 0.0);

        auto result = u.resolve(100.0_lu);
        CHECK(result.value == 100.0);
    }

    SUBCASE("half()") {
        auto u = half();
        CHECK(u.scale == 0.5);
        CHECK(u.offset.value == 0.0);

        auto result = u.resolve(100.0_lu);
        CHECK(result.value == 50.0);
    }

    SUBCASE("percent2()") {
        auto u = percent2(0.75);
        CHECK(u.width.scale == 0.75);
        CHECK(u.width.offset.value == 0.0);
        CHECK(u.height.scale == 0.75);
        CHECK(u.height.offset.value == 0.0);
    }

    SUBCASE("absolute2()") {
        auto u = absolute2(60.0_lu, 40.0_lu);
        CHECK(u.width.scale == 0.0);
        CHECK(u.width.offset.value == 60.0);
        CHECK(u.height.scale == 0.0);
        CHECK(u.height.offset.value == 40.0);
    }

    SUBCASE("full2()") {
        auto u = full2();
        CHECK(u.width.scale == 1.0);
        CHECK(u.height.scale == 1.0);
        CHECK(u.width.offset.value == 0.0);
        CHECK(u.height.offset.value == 0.0);
    }
}

// ============================================================================
// Practical Usage Tests
// ============================================================================

TEST_CASE("udim - Practical usage") {
    SUBCASE("Centered dialog (50% - half size)") {
        // Center a 60-unit wide dialog in 100-unit parent
        udim x = percent(0.5) + absolute(-30.0_lu);  // 50% - 30
        auto result = x.resolve(100.0_lu);
        CHECK(result.value == 20.0);
    }

    SUBCASE("Sidebar (100% - 200 units)") {
        // Content area: full width minus sidebar width
        udim content_width = full() + absolute(-200.0_lu);
        auto result = content_width.resolve(1000.0_lu);
        CHECK(result.value == 800.0);
    }

    SUBCASE("Margin (10 units on each side)") {
        // Inner width: 100% - 20 units (10 left + 10 right)
        udim inner = percent(1.0) + absolute(-20.0_lu);
        auto result = inner.resolve(100.0_lu);
        CHECK(result.value == 80.0);
    }

    SUBCASE("Two equal columns with gap") {
        // Each column: 50% - 5 units (10 unit gap between)
        udim col_width = percent(0.5) + absolute(-5.0_lu);

        logical_unit parent_width = 100.0_lu;
        auto col1_width = col_width.resolve(parent_width);
        auto col2_width = col_width.resolve(parent_width);

        CHECK(col1_width.value == 45.0);
        CHECK(col2_width.value == 45.0);
        // Total: 45 + 10 (gap) + 45 = 100 ✓
    }

    SUBCASE("Dynamic button strip (100% width, fixed height)") {
        udim2 button_strip_size(
            1.0, 0.0_lu,   // Width: 100% of parent
            0.0, 30.0_lu   // Height: fixed 30 units
        );

        logical_size parent(200.0_lu, 100.0_lu);
        auto result = button_strip_size.resolve(parent);

        CHECK(result.width.value == 200.0);  // Fills parent width
        CHECK(result.height.value == 30.0);  // Fixed height
    }
}
