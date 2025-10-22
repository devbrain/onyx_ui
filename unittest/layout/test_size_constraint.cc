/**
 * @file test_size_constraint.cc
 * @brief Unit tests for size_constraint and float comparison
 * @author Claude
 * @date 19/10/2025
 */

#include <doctest/doctest.h>
#include <onyxui/layout_strategy.hh>

using namespace onyxui;

TEST_CASE("size_constraint - Float comparison with epsilon") {
    SUBCASE("Exact equality works") {
        size_constraint a, b;
        a.weight = 2.0F;
        b.weight = 2.0F;
        CHECK(a == b);

        a.percentage = 0.5F;
        b.percentage = 0.5F;
        CHECK(a == b);
    }

    SUBCASE("Handles floating-point rounding errors") {
        size_constraint a, b;

        // Classic floating-point issue: 2.0 / 3.0 * 3.0 may not equal 2.0
        a.weight = 2.0F / 3.0F * 3.0F;
        b.weight = 2.0F;
        CHECK(a == b);  // Should be equal within epsilon

        // Another rounding error case
        a.weight = 0.1F + 0.1F + 0.1F;
        b.weight = 0.3F;
        CHECK(a == b);  // Should be equal within epsilon
    }

    SUBCASE("Clearly different values are not equal") {
        size_constraint a, b;
        a.weight = 2.0F;
        b.weight = 2.1F;
        CHECK(a != b);  // Different beyond epsilon

        a.weight = 2.0F;
        b.weight = 3.0F;
        CHECK(a != b);  // Clearly different
    }

    SUBCASE("Zero comparison works correctly") {
        size_constraint a, b;
        a.weight = 0.0F;
        b.weight = 0.0F;
        CHECK(a == b);

        a.weight = 0.0F;
        b.weight = 0.00001F;
        CHECK(a == b);  // Within epsilon

        a.weight = 0.0F;
        b.weight = 0.01F;
        CHECK(a != b);  // Beyond epsilon
    }

    SUBCASE("Large values work correctly") {
        size_constraint a, b;
        a.weight = 1000.0F;
        b.weight = 1000.0F;
        CHECK(a == b);

        a.weight = 1000.0F;
        b.weight = 1000.0001F;
        CHECK(a == b);  // Within relative epsilon

        a.weight = 1000.0F;
        b.weight = 1001.0F;
        CHECK(a != b);  // Beyond relative epsilon
    }

    SUBCASE("Percentage comparison works") {
        size_constraint a, b;
        a.percentage = 0.5F;
        b.percentage = 0.5F;
        CHECK(a == b);

        // Rounding error case
        a.percentage = 1.0F / 3.0F;
        b.percentage = 0.333333F;
        CHECK(a == b);  // Within epsilon
    }

    SUBCASE("All fields must match") {
        size_constraint a, b;

        // Different policy
        a.policy = size_policy::fixed;
        b.policy = size_policy::content;
        CHECK(a != b);

        // Same policy, different preferred_size
        a.policy = b.policy = size_policy::fixed;
        a.preferred_size = 100;
        b.preferred_size = 200;
        CHECK(a != b);

        // Same everything except weight
        a = b;
        a.weight = 1.0F;
        b.weight = 2.0F;
        CHECK(a != b);
    }
}

TEST_CASE("approx_equal - Helper function tests") {
    SUBCASE("Basic equality") {
        CHECK(approx_equal(2.0F, 2.0F));
        CHECK(approx_equal(0.0F, 0.0F));
        CHECK(approx_equal(-1.5F, -1.5F));
    }

    SUBCASE("Near-zero values") {
        CHECK(approx_equal(0.0F, 0.00001F));
        CHECK(approx_equal(0.00005F, 0.00006F));
        CHECK_FALSE(approx_equal(0.0F, 0.01F));
    }

    SUBCASE("Relative epsilon for large values") {
        CHECK(approx_equal(1000.0F, 1000.0001F));
        CHECK(approx_equal(10000.0F, 10000.001F));
        CHECK_FALSE(approx_equal(1000.0F, 1001.0F));
    }

    SUBCASE("Negative values") {
        CHECK(approx_equal(-2.0F, -2.0F));
        CHECK(approx_equal(-2.0F / 3.0F * 3.0F, -2.0F));
        CHECK_FALSE(approx_equal(-2.0F, -2.1F));
    }

    SUBCASE("Mixed signs are not equal") {
        CHECK_FALSE(approx_equal(1.0F, -1.0F));
        CHECK_FALSE(approx_equal(-0.5F, 0.5F));
    }

    SUBCASE("Custom epsilon") {
        // Tighter tolerance
        CHECK(approx_equal(2.0F, 2.00001F, 0.0001F));
        CHECK_FALSE(approx_equal(2.0F, 2.001F, 0.0001F));

        // Looser tolerance
        CHECK(approx_equal(2.0F, 2.01F, 0.01F));
        CHECK(approx_equal(2.0F, 2.1F, 0.1F));
    }
}

TEST_CASE("size_constraint - Integer field comparison") {
    SUBCASE("preferred_size must match exactly") {
        size_constraint a, b;
        a.preferred_size = 100;
        b.preferred_size = 100;
        CHECK(a == b);

        b.preferred_size = 101;
        CHECK(a != b);
    }

    SUBCASE("min_size must match exactly") {
        size_constraint a, b;
        a.min_size = 50;
        b.min_size = 50;
        CHECK(a == b);

        b.min_size = 51;
        CHECK(a != b);
    }

    SUBCASE("max_size must match exactly") {
        size_constraint a, b;
        a.max_size = 200;
        b.max_size = 200;
        CHECK(a == b);

        b.max_size = 201;
        CHECK(a != b);
    }
}

TEST_CASE("size_constraint - Real-world scenarios") {
    SUBCASE("Layout recalculation detection") {
        size_constraint cached, current;

        // User sets constraint
        cached.policy = size_policy::weighted;
        cached.weight = 2.0F;

        // Layout system recalculates and gets slightly different value
        current.policy = size_policy::weighted;
        current.weight = 2.0F / 3.0F * 3.0F;

        // Should still be considered equal (no unnecessary layout invalidation)
        CHECK(cached == current);
    }

    SUBCASE("Percentage from division") {
        size_constraint a, b;

        // One third as percentage
        a.percentage = 1.0F / 3.0F;
        b.percentage = 0.333333F;

        CHECK(a == b);
    }

    SUBCASE("Weighted distribution") {
        size_constraint c1, c2, c3;

        c1.weight = 1.0F;
        c2.weight = 2.0F;
        c3.weight = 1.0F;

        // After some computation
        float const total = c1.weight + c2.weight + c3.weight;
        size_constraint c1_check;
        c1_check.weight = (c1.weight / total) * total;  // May have rounding

        CHECK(c1 == c1_check);
    }
}
