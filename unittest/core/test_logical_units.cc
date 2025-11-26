/**
 * @file test_logical_units.cc
 * @brief Comprehensive tests for logical_unit type
 * @author OnyxUI Framework
 * @date 2025-11-26
 */

#include <doctest/doctest.h>
#include <../../include/onyxui/core/types.hh>

using namespace onyxui;

// ============================================================================
// Construction Tests
// ============================================================================

TEST_CASE("logical_unit - Construction") {
    SUBCASE("Default construction") {
        logical_unit lu;
        CHECK(lu.value == 0.0);
    }

    SUBCASE("Explicit construction from double") {
        logical_unit lu(10.5);
        CHECK(lu.value == 10.5);
    }

    SUBCASE("Explicit construction from int") {
        logical_unit lu(42);
        CHECK(lu.value == 42.0);
    }

    SUBCASE("Construction from literals") {
        auto lu1 = 10.5_lu;
        CHECK(lu1.value == 10.5);

        auto lu2 = 42_lu;
        CHECK(lu2.value == 42.0);
    }
}

// ============================================================================
// Arithmetic Operator Tests
// ============================================================================

TEST_CASE("logical_unit - Addition") {
    logical_unit a(10.0);
    logical_unit b(5.5);

    SUBCASE("Basic addition") {
        auto result = a + b;
        CHECK(result.value == 15.5);
    }

    SUBCASE("Addition with literals") {
        auto result = 10.0_lu + 5.5_lu;
        CHECK(result.value == 15.5);
    }

    SUBCASE("Compound addition") {
        logical_unit c(10.0);
        c += logical_unit(5.0);
        CHECK(c.value == 15.0);
    }
}

TEST_CASE("logical_unit - Subtraction") {
    logical_unit a(10.0);
    logical_unit b(5.5);

    SUBCASE("Basic subtraction") {
        auto result = a - b;
        CHECK(result.value == 4.5);
    }

    SUBCASE("Unary minus") {
        auto result = -a;
        CHECK(result.value == -10.0);
    }

    SUBCASE("Compound subtraction") {
        logical_unit c(10.0);
        c -= logical_unit(5.0);
        CHECK(c.value == 5.0);
    }
}

TEST_CASE("logical_unit - Multiplication") {
    logical_unit a(10.0);

    SUBCASE("Multiplication by double") {
        auto result = a * 2.5;
        CHECK(result.value == 25.0);
    }

    SUBCASE("Left multiplication by double") {
        auto result = 2.5 * a;
        CHECK(result.value == 25.0);
    }

    SUBCASE("Compound multiplication") {
        logical_unit c(10.0);
        c *= 2.5;
        CHECK(c.value == 25.0);
    }
}

TEST_CASE("logical_unit - Division") {
    logical_unit a(10.0);

    SUBCASE("Division by double") {
        auto result = a / 2.0;
        CHECK(result.value == 5.0);
    }

    SUBCASE("Division by int (natural syntax)") {
        auto result = a / 2;
        CHECK(result.value == 5.0);
    }

    SUBCASE("Fractional division by int") {
        logical_unit b(11.0);
        auto result = b / 2;
        CHECK(result.value == 5.5);
    }

    SUBCASE("Compound division by double") {
        logical_unit c(10.0);
        c /= 2.0;
        CHECK(c.value == 5.0);
    }

    SUBCASE("Compound division by int") {
        logical_unit c(10.0);
        c /= 2;
        CHECK(c.value == 5.0);
    }
}

// ============================================================================
// Comparison Operator Tests
// ============================================================================

TEST_CASE("logical_unit - Equality") {
    logical_unit a(10.0);
    logical_unit b(10.0);
    logical_unit c(10.5);

    SUBCASE("Exact equality") {
        CHECK(a == b);
        CHECK_FALSE(a == c);
    }

    SUBCASE("Epsilon comparison (floating-point tolerance)") {
        logical_unit d(10.0 + 1e-10);  // Within epsilon
        CHECK(a == d);
    }

    SUBCASE("Inequality") {
        CHECK(a != c);
        CHECK_FALSE(a != b);
    }
}

TEST_CASE("logical_unit - Ordering") {
    logical_unit a(5.0);
    logical_unit b(10.0);

    SUBCASE("Less than") {
        CHECK(a < b);
        CHECK_FALSE(b < a);
    }

    SUBCASE("Less than or equal") {
        CHECK(a <= b);
        CHECK(a <= logical_unit(5.0));
    }

    SUBCASE("Greater than") {
        CHECK(b > a);
        CHECK_FALSE(a > b);
    }

    SUBCASE("Greater than or equal") {
        CHECK(b >= a);
        CHECK(b >= logical_unit(10.0));
    }
}

// ============================================================================
// Math Helper Tests
// ============================================================================

TEST_CASE("logical_unit - Math helpers") {
    SUBCASE("abs") {
        CHECK(logical_unit(-10.5).abs().value == 10.5);
        CHECK(abs(logical_unit(-10.5)).value == 10.5);
    }

    SUBCASE("floor") {
        CHECK(logical_unit(10.7).floor().value == 10.0);
        CHECK(floor(logical_unit(10.7)).value == 10.0);
    }

    SUBCASE("ceil") {
        CHECK(logical_unit(10.3).ceil().value == 11.0);
        CHECK(ceil(logical_unit(10.3)).value == 11.0);
    }

    SUBCASE("round") {
        CHECK(logical_unit(10.4).round().value == 10.0);
        CHECK(logical_unit(10.6).round().value == 11.0);
        CHECK(round(logical_unit(10.5)).value == 11.0);  // Round half to even
    }

    SUBCASE("min") {
        auto result = min(logical_unit(10.0), logical_unit(5.0));
        CHECK(result.value == 5.0);
    }

    SUBCASE("max") {
        auto result = max(logical_unit(10.0), logical_unit(5.0));
        CHECK(result.value == 10.0);
    }

    SUBCASE("clamp") {
        CHECK(clamp(logical_unit(5.0), logical_unit(0.0), logical_unit(10.0)).value == 5.0);
        CHECK(clamp(logical_unit(-5.0), logical_unit(0.0), logical_unit(10.0)).value == 0.0);
        CHECK(clamp(logical_unit(15.0), logical_unit(0.0), logical_unit(10.0)).value == 10.0);
    }
}

// ============================================================================
// Explicit Conversion Tests
// ============================================================================

TEST_CASE("logical_unit - Explicit conversions") {
    logical_unit lu(10.5);

    SUBCASE("Explicit cast to double") {
        double d = static_cast<double>(lu);
        CHECK(d == 10.5);
    }

    // SUBCASE("No implicit conversion to double") {
    //     // This should NOT compile (explicit construction)
    //     // double d = lu;  // Error!
    // }
}

// ============================================================================
// Edge Case Tests
// ============================================================================

TEST_CASE("logical_unit - Edge cases") {
    SUBCASE("Zero") {
        logical_unit zero(0.0);
        CHECK(zero.value == 0.0);
        CHECK(zero == logical_unit(0.0));
    }

    SUBCASE("Negative values") {
        logical_unit neg(-10.5);
        CHECK(neg.value == -10.5);
        CHECK(neg < logical_unit(0.0));
    }

    SUBCASE("Very small values") {
        logical_unit small(0.001);
        CHECK(small.value == 0.001);
    }

    SUBCASE("Very large values") {
        logical_unit large(1e6);
        CHECK(large.value == 1e6);
    }
}

// ============================================================================
// Practical Usage Tests
// ============================================================================

TEST_CASE("logical_unit - Practical usage") {
    SUBCASE("Centering calculation") {
        // Center a 60-unit window on an 80-unit screen
        auto screen_width = 80.0_lu;
        auto window_width = 60.0_lu;
        auto x = (screen_width - window_width) / 2;
        CHECK(x.value == 10.0);
    }

    SUBCASE("Fractional positioning") {
        // Position at 1/3 of screen
        auto screen_width = 90.0_lu;
        auto x = screen_width / 3;
        CHECK(x.value == 30.0);
    }

    SUBCASE("Sub-pixel positioning") {
        // Fractional coordinates for smooth animations
        auto pos = 10.5_lu;
        auto delta = 0.25_lu;
        auto new_pos = pos + delta;
        CHECK(new_pos.value == 10.75);
    }

    SUBCASE("Percentage-based sizing") {
        // 50% of parent
        auto parent_width = 100.0_lu;
        auto child_width = parent_width * 0.5;
        CHECK(child_width.value == 50.0);
    }
}
