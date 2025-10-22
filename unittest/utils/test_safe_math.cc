/**
 * @file test_safe_math.cc
 * @brief Unit tests for safe arithmetic operations
 * @details Tests for Phase 1.3 of the refactoring plan
 */

#include <doctest/doctest.h>
#include <onyxui/utils/safe_math.hh>
#include <limits>

using namespace onyxui::safe_math;

TEST_SUITE("Safe Math") {

    TEST_CASE("add_clamped - basic operations") {
        SUBCASE("Normal addition") {
            CHECK(add_clamped(5, 3) == 8);
            CHECK(add_clamped(-5, -3) == -8);
            CHECK(add_clamped(5, -3) == 2);
            CHECK(add_clamped(-5, 3) == -2);
        }

        SUBCASE("Zero addition") {
            CHECK(add_clamped(0, 0) == 0);
            CHECK(add_clamped(5, 0) == 5);
            CHECK(add_clamped(0, 5) == 5);
            CHECK(add_clamped(-5, 0) == -5);
        }
    }

    TEST_CASE("add_clamped - signed overflow") {
        SUBCASE("Positive overflow to INT_MAX") {
            constexpr int max = std::numeric_limits<int>::max();
            CHECK(add_clamped(max, 1) == max);
            CHECK(add_clamped(max, 100) == max);
            CHECK(add_clamped(max / 2 + 1, max / 2 + 1) == max);
        }

        SUBCASE("Negative overflow to INT_MIN") {
            constexpr int min = std::numeric_limits<int>::min();
            CHECK(add_clamped(min, -1) == min);
            CHECK(add_clamped(min, -100) == min);
            CHECK(add_clamped(min / 2, min / 2 - 1) == min);
        }

        SUBCASE("Mixed signs cannot overflow") {
            constexpr int max = std::numeric_limits<int>::max();
            constexpr int min = std::numeric_limits<int>::min();
            // These should not overflow because result is between operands
            CHECK(add_clamped(max, -1) == max - 1);
            CHECK(add_clamped(min, 1) == min + 1);
        }
    }

    TEST_CASE("add_clamped - unsigned overflow") {
        SUBCASE("Overflow to UINT_MAX") {
            constexpr unsigned max = std::numeric_limits<unsigned>::max();
            CHECK(add_clamped(max, 1U) == max);
            CHECK(add_clamped(max, 100U) == max);
            CHECK(add_clamped(max / 2 + 1, max / 2 + 1) == max);
        }
    }

    TEST_CASE("subtract_clamped - basic operations") {
        SUBCASE("Normal subtraction") {
            CHECK(subtract_clamped(8, 3) == 5);
            CHECK(subtract_clamped(-5, -3) == -2);
            CHECK(subtract_clamped(5, 8) == -3);
        }

        SUBCASE("Zero subtraction") {
            CHECK(subtract_clamped(0, 0) == 0);
            CHECK(subtract_clamped(5, 0) == 5);
            CHECK(subtract_clamped(0, 5) == -5);
        }
    }

    TEST_CASE("subtract_clamped - signed overflow") {
        SUBCASE("Positive overflow to INT_MAX") {
            constexpr int max = std::numeric_limits<int>::max();
            constexpr int min = std::numeric_limits<int>::min();
            // max - (-1) = max + 1 → overflow
            CHECK(subtract_clamped(max, -1) == max);
            CHECK(subtract_clamped(max, min) == max);
        }

        SUBCASE("Negative overflow to INT_MIN") {
            constexpr int max = std::numeric_limits<int>::max();
            constexpr int min = std::numeric_limits<int>::min();
            // min - 1 → overflow
            CHECK(subtract_clamped(min, 1) == min);
            CHECK(subtract_clamped(min, max) == min);
        }
    }

    TEST_CASE("subtract_clamped - unsigned underflow") {
        SUBCASE("Underflow to 0") {
            CHECK(subtract_clamped(5U, 10U) == 0U);
            CHECK(subtract_clamped(0U, 1U) == 0U);
        }
    }

    TEST_CASE("multiply_clamped - basic operations") {
        SUBCASE("Normal multiplication") {
            CHECK(multiply_clamped(5, 3) == 15);
            CHECK(multiply_clamped(-5, -3) == 15);
            CHECK(multiply_clamped(5, -3) == -15);
            CHECK(multiply_clamped(-5, 3) == -15);
        }

        SUBCASE("Zero multiplication") {
            CHECK(multiply_clamped(0, 0) == 0);
            CHECK(multiply_clamped(5, 0) == 0);
            CHECK(multiply_clamped(0, 5) == 0);
            CHECK(multiply_clamped(std::numeric_limits<int>::max(), 0) == 0);
        }

        SUBCASE("One multiplication") {
            CHECK(multiply_clamped(5, 1) == 5);
            CHECK(multiply_clamped(1, 5) == 5);
            CHECK(multiply_clamped(-5, 1) == -5);
        }
    }

    TEST_CASE("multiply_clamped - signed overflow") {
        SUBCASE("Positive overflow (pos * pos)") {
            constexpr int max = std::numeric_limits<int>::max();
            CHECK(multiply_clamped(max, 2) == max);
            CHECK(multiply_clamped(max / 2 + 1, 3) == max);
        }

        SUBCASE("Positive overflow (neg * neg)") {
            constexpr int max = std::numeric_limits<int>::max();
            constexpr int min = std::numeric_limits<int>::min();
            CHECK(multiply_clamped(min, -1) == max);  // Special case: min * -1
            CHECK(multiply_clamped(-1000, -1000) == (1000 * 1000));  // No overflow for small values
        }

        SUBCASE("Negative overflow (pos * neg)") {
            constexpr int max = std::numeric_limits<int>::max();
            constexpr int min = std::numeric_limits<int>::min();
            CHECK(multiply_clamped(max, -2) == min);
            CHECK(multiply_clamped(-2, max) == min);
        }

        SUBCASE("Negative overflow (neg * pos)") {
            constexpr int min = std::numeric_limits<int>::min();
            CHECK(multiply_clamped(min, 2) == min);
            CHECK(multiply_clamped(min / 2, 3) == min);
        }
    }

    TEST_CASE("multiply_clamped - unsigned overflow") {
        SUBCASE("Overflow to UINT_MAX") {
            constexpr unsigned max = std::numeric_limits<unsigned>::max();
            CHECK(multiply_clamped(max, 2U) == max);
            CHECK(multiply_clamped(max / 2 + 1, 3U) == max);
        }
    }

    TEST_CASE("safe_add - detection") {
        SUBCASE("Successful addition") {
            int result;
            CHECK(safe_add(5, 3, result) == true);
            CHECK(result == 8);
        }

        SUBCASE("Overflow detection") {
            int result;
            constexpr int max = std::numeric_limits<int>::max();
            CHECK(safe_add(max, 1, result) == false);
        }
    }

    TEST_CASE("safe_subtract - detection") {
        SUBCASE("Successful subtraction") {
            int result;
            CHECK(safe_subtract(8, 3, result) == true);
            CHECK(result == 5);
        }

        SUBCASE("Overflow detection") {
            int result;
            constexpr int min = std::numeric_limits<int>::min();
            CHECK(safe_subtract(min, 1, result) == false);
        }
    }

    TEST_CASE("safe_multiply - detection") {
        SUBCASE("Successful multiplication") {
            int result;
            CHECK(safe_multiply(5, 3, result) == true);
            CHECK(result == 15);
        }

        SUBCASE("Overflow detection") {
            int result;
            constexpr int max = std::numeric_limits<int>::max();
            CHECK(safe_multiply(max, 2, result) == false);
        }

        SUBCASE("Special case: INT_MIN * -1") {
            int result;
            constexpr int min = std::numeric_limits<int>::min();
            CHECK(safe_multiply(min, -1, result) == false);
        }
    }

    TEST_CASE("accumulate_safe - basic") {
        SUBCASE("Normal accumulation") {
            int acc = 0;
            CHECK(accumulate_safe(acc, 5) == true);
            CHECK(acc == 5);
            CHECK(accumulate_safe(acc, 3) == true);
            CHECK(acc == 8);
        }

        SUBCASE("Overflow clamping") {
            constexpr int max = std::numeric_limits<int>::max();
            int acc = max;
            CHECK(accumulate_safe(acc, 1) == false);
            CHECK(acc == max);  // Clamped to max
        }
    }

    TEST_CASE("Boundary conditions - comprehensive") {
        SUBCASE("INT_MAX boundary") {
            constexpr int max = std::numeric_limits<int>::max();
            CHECK(add_clamped(max, 0) == max);
            CHECK(add_clamped(max, 1) == max);
            CHECK(add_clamped(max, -1) == max - 1);
        }

        SUBCASE("INT_MIN boundary") {
            constexpr int min = std::numeric_limits<int>::min();
            CHECK(add_clamped(min, 0) == min);
            CHECK(add_clamped(min, -1) == min);
            CHECK(add_clamped(min, 1) == min + 1);
        }

        SUBCASE("UINT_MAX boundary") {
            constexpr unsigned max = std::numeric_limits<unsigned>::max();
            CHECK(add_clamped(max, 0U) == max);
            CHECK(add_clamped(max, 1U) == max);
            CHECK(subtract_clamped(max, 1U) == max - 1);
        }
    }
}
