/**
 * @file safe_math.hh
 * @brief Safe arithmetic utilities to prevent integer overflow
 * @author igor
 * @date 10/10/2025
 *
 * This file provides utility functions for safe arithmetic operations
 * that detect and handle potential integer overflow conditions.
 */

#pragma once

#include <limits>

/**
 * @namespace safe_math
 * @brief Safe arithmetic operations that prevent overflow
 */
namespace onyxui::safe_math {
    /**
     * @brief Safely add two integers with overflow detection
     * @tparam T Integer type
     * @param a First operand
     * @param b Second operand
     * @param result Output parameter for result (only modified if no overflow)
     * @return true if addition succeeded, false if overflow would occur
     */
    template<typename T>
    [[nodiscard]] constexpr bool safe_add(T a, T b, T& result) noexcept {
        static_assert(std::is_integral_v <T>, "safe_add requires integral type");

        if (b > 0 && a > std::numeric_limits <T>::max() - b) {
            return false; // Positive overflow
        }
        if (b < 0 && a < std::numeric_limits <T>::min() - b) {
            return false; // Negative overflow
        }

        result = a + b;
        return true;
    }

    /**
     * @brief Safely subtract two integers with overflow detection
     * @tparam T Integer type
     * @param a First operand (minuend)
     * @param b Second operand (subtrahend)
     * @param result Output parameter for result (only modified if no overflow)
     * @return true if subtraction succeeded, false if overflow would occur
     */
    template<typename T>
    [[nodiscard]] constexpr bool safe_subtract(T a, T b, T& result) noexcept {
        static_assert(std::is_integral_v <T>, "safe_subtract requires integral type");

        if (b > 0 && a < std::numeric_limits <T>::min() + b) {
            return false; // Negative overflow
        }
        if (b < 0 && a > std::numeric_limits <T>::max() + b) {
            return false; // Positive overflow
        }

        result = a - b;
        return true;
    }

    /**
     * @brief Safely multiply two integers with overflow detection
     * @tparam T Integer type
     * @param a First operand
     * @param b Second operand
     * @param result Output parameter for result (only modified if no overflow)
     * @return true if multiplication succeeded, false if overflow would occur
     */
    template<typename T>
    constexpr bool safe_multiply(T a, T b, T& result) noexcept {
        static_assert(std::is_integral_v <T>, "safe_multiply requires integral type");

        if (a == 0 || b == 0) {
            result = 0;
            return true;
        }

        // Check for multiplication overflow
        if (a > 0) {
            if (b > 0) {
                if (a > std::numeric_limits <T>::max() / b) {
                    return false;
                }
            } else {
                if (b < std::numeric_limits <T>::min() / a) {
                    return false;
                }
            }
        } else {
            if (b > 0) {
                if (a < std::numeric_limits <T>::min() / b) {
                    return false;
                }
            } else {
                if (a != 0 && b < std::numeric_limits <T>::max() / a) {
                    return false;
                }
            }
        }

        result = a * b;
        return true;
    }

    /**
     * @brief Add two integers with clamping to max on overflow
     * @tparam T Integer type
     * @param a First operand
     * @param b Second operand
     * @return Sum of a and b, or max value if overflow would occur
     */
    template<typename T>
    [[nodiscard]] constexpr T add_clamped(T a, T b) noexcept {
        static_assert(std::is_integral_v <T>, "add_clamped requires integral type");

        T result;
        if (safe_add(a, b, result)) {
            return result;
        }

        // Clamp to appropriate limit
        if (b > 0) {
            return std::numeric_limits <T>::max();
        } else {
            return std::numeric_limits <T>::min();
        }
    }

    /**
     * @brief Accumulate values with overflow protection
     * @tparam T Integer type
     * @param accumulator Current accumulated value (modified in place)
     * @param value Value to add
     * @return true if accumulation succeeded, false if clamped to max
     */
    template<typename T>
    constexpr bool accumulate_safe(T& accumulator, T value) noexcept {
        static_assert(std::is_integral_v <T>, "accumulate_safe requires integral type");

        T result;
        if (safe_add(accumulator, value, result)) {
            accumulator = result;
            return true;
        }

        // Clamp to max on overflow
        if (value > 0) {
            accumulator = std::numeric_limits <T>::max();
        } else {
            accumulator = std::numeric_limits <T>::min();
        }
        return false;
    }
}
