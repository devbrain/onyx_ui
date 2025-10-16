/**
 * @file safe_math.hh
 * @brief Safe arithmetic utilities to prevent integer overflow
 * @author igor
 * @date 10/10/2025
 *
 * This file provides utility functions for safe arithmetic operations
 * that detect and handle potential integer overflow conditions.
 * Uses compiler builtins when available for optimal performance.
 */

#pragma once

#include <limits>
#include <type_traits>
#include <optional>

/**
 * @namespace safe_math
 * @brief Safe arithmetic operations that prevent overflow
 */
namespace onyxui::safe_math {

    // ======================================================================
    // Type constraints
    // ======================================================================

    template<typename T>
    concept SignedIntegral = std::is_signed_v<T> && std::is_integral_v<T>;

    template<typename T>
    concept UnsignedIntegral = std::is_unsigned_v<T> && std::is_integral_v<T>;

    // ======================================================================
    // Addition
    // ======================================================================

    /**
     * @brief Safely add two integers with overflow detection
     * @tparam T Integer type (signed or unsigned)
     * @param a First operand
     * @param b Second operand
     * @param result Output parameter for result (only modified if no overflow)
     * @return true if addition succeeded, false if overflow would occur
     */
    template<std::integral T>
    [[nodiscard]] constexpr bool safe_add(T a, T b, T& result) noexcept {
#if defined(__has_builtin)
#  if __has_builtin(__builtin_add_overflow)
        return !__builtin_add_overflow(a, b, &result);
#  endif
#endif

        if constexpr (std::is_unsigned_v<T>) {
            // Unsigned: check if result would wrap
            result = a + b;
            return result >= a; // If wrapped, result would be less than a
        } else {
            // Signed: check bounds before operation
            if (b > 0 && a > std::numeric_limits<T>::max() - b) {
                return false; // Positive overflow
            }
            if (b < 0 && a < std::numeric_limits<T>::min() - b) {
                return false; // Negative overflow
            }
            result = a + b;
            return true;
        }
    }

    /**
     * @brief Safely add two integers, returning optional result
     * @tparam T Integer type
     * @param a First operand
     * @param b Second operand
     * @return Result if no overflow, std::nullopt otherwise
     */
    template<std::integral T>
    [[nodiscard]] constexpr std::optional<T> add(T a, T b) noexcept {
        T result;
        if (safe_add(a, b, result)) {
            return result;
        }
        return std::nullopt;
    }

    // ======================================================================
    // Subtraction
    // ======================================================================

    /**
     * @brief Safely subtract two integers with overflow detection
     * @tparam T Integer type
     * @param a First operand (minuend)
     * @param b Second operand (subtrahend)
     * @param result Output parameter for result (only modified if no overflow)
     * @return true if subtraction succeeded, false if overflow would occur
     */
    template<std::integral T>
    [[nodiscard]] constexpr bool safe_subtract(T a, T b, T& result) noexcept {
#if defined(__has_builtin)
#  if __has_builtin(__builtin_sub_overflow)
        return !__builtin_sub_overflow(a, b, &result);
#  endif
#endif

        if constexpr (std::is_unsigned_v<T>) {
            // Unsigned: check if result would wrap
            if (a < b) {
                return false; // Would wrap to large positive
            }
            result = a - b;
            return true;
        } else {
            // Signed: check bounds before operation
            // a - b can overflow if:
            // - b < 0 and a > max + b (becomes a > max - |b|)
            // - b > 0 and a < min + b
            if (b < 0) {
                // Subtracting negative is adding positive
                if (a > std::numeric_limits<T>::max() + b) {
                    return false; // Positive overflow
                }
            } else {
                // Subtracting positive
                if (a < std::numeric_limits<T>::min() + b) {
                    return false; // Negative overflow
                }
            }
            result = a - b;
            return true;
        }
    }

    /**
     * @brief Safely subtract two integers, returning optional result
     * @tparam T Integer type
     * @param a First operand
     * @param b Second operand
     * @return Result if no overflow, std::nullopt otherwise
     */
    template<std::integral T>
    [[nodiscard]] constexpr std::optional<T> subtract(T a, T b) noexcept {
        T result;
        if (safe_subtract(a, b, result)) {
            return result;
        }
        return std::nullopt;
    }

    // ======================================================================
    // Multiplication
    // ======================================================================

    /**
     * @brief Safely multiply two integers with overflow detection
     * @tparam T Integer type
     * @param a First operand
     * @param b Second operand
     * @param result Output parameter for result (only modified if no overflow)
     * @return true if multiplication succeeded, false if overflow would occur
     */
    template<std::integral T>
    [[nodiscard]] constexpr bool safe_multiply(T a, T b, T& result) noexcept {
#if defined(__has_builtin)
#  if __has_builtin(__builtin_mul_overflow)
        return !__builtin_mul_overflow(a, b, &result);
#  endif
#endif

        // Handle zero cases first
        if (a == 0 || b == 0) {
            result = 0;
            return true;
        }

        if constexpr (std::is_unsigned_v<T>) {
            // Unsigned: simple division check
            if (a > std::numeric_limits<T>::max() / b) {
                return false;
            }
            result = a * b;
            return true;
        } else {
            // Signed: handle special cases and all sign combinations

            // Special case: min * -1 is always overflow for signed types
            if ((a == std::numeric_limits<T>::min() && b == -1) ||
                (b == std::numeric_limits<T>::min() && a == -1)) {
                return false;
            }

            // Check using division (avoiding division by zero which we handled above)
            const T max = std::numeric_limits<T>::max();
            const T min = std::numeric_limits<T>::min();

            if (a > 0) {
                if (b > 0) {
                    // Both positive
                    if (a > max / b) {
                        return false;
                    }
                } else {
                    // a positive, b negative
                    if (b < min / a) {
                        return false;
                    }
                }
            } else {
                if (b > 0) {
                    // a negative, b positive
                    if (a < min / b) {
                        return false;
                    }
                } else {
                    // Both negative (result is positive)
                    // Note: We already handled min * -1 case
                    if (a < max / b) {
                        return false;
                    }
                }
            }

            result = a * b;
            return true;
        }
    }

    /**
     * @brief Safely multiply two integers, returning optional result
     * @tparam T Integer type
     * @param a First operand
     * @param b Second operand
     * @return Result if no overflow, std::nullopt otherwise
     */
    template<std::integral T>
    [[nodiscard]] constexpr std::optional<T> multiply(T a, T b) noexcept {
        T result;
        if (safe_multiply(a, b, result)) {
            return result;
        }
        return std::nullopt;
    }

    // ======================================================================
    // Clamped operations
    // ======================================================================

    /**
     * @brief Add two integers with clamping to min/max on overflow
     * @tparam T Integer type
     * @param a First operand
     * @param b Second operand
     * @return Sum of a and b, or appropriate limit if overflow would occur
     */
    template<std::integral T>
    [[nodiscard]] constexpr T add_clamped(T a, T b) noexcept {
        T result;
        if (safe_add(a, b, result)) {
            return result;
        }

        // Clamp to appropriate limit based on direction of overflow
        if constexpr (std::is_unsigned_v<T>) {
            return std::numeric_limits<T>::max(); // Unsigned can only overflow upward
        } else {
            // Signed: determine direction from operands
            if ((a > 0 && b > 0) || (a >= 0 && b > 0) || (a > 0 && b >= 0)) {
                return std::numeric_limits<T>::max(); // Positive overflow
            } else {
                return std::numeric_limits<T>::min(); // Negative overflow
            }
        }
    }

    /**
     * @brief Subtract two integers with clamping to min/max on overflow
     * @tparam T Integer type
     * @param a First operand
     * @param b Second operand
     * @return Difference of a and b, or appropriate limit if overflow would occur
     */
    template<std::integral T>
    [[nodiscard]] constexpr T subtract_clamped(T a, T b) noexcept {
        T result;
        if (safe_subtract(a, b, result)) {
            return result;
        }

        // Clamp to appropriate limit based on direction of overflow
        if constexpr (std::is_unsigned_v<T>) {
            return std::numeric_limits<T>::min(); // Unsigned wraps to 0
        } else {
            // a - b overflows positive when b < 0 (subtracting negative)
            // a - b overflows negative when b > 0 (subtracting positive from small a)
            if (b < 0) {
                return std::numeric_limits<T>::max(); // Positive overflow
            } else {
                return std::numeric_limits<T>::min(); // Negative overflow
            }
        }
    }

    /**
     * @brief Multiply two integers with clamping to min/max on overflow
     * @tparam T Integer type
     * @param a First operand
     * @param b Second operand
     * @return Product of a and b, or appropriate limit if overflow would occur
     */
    template<std::integral T>
    [[nodiscard]] constexpr T multiply_clamped(T a, T b) noexcept {
        T result;
        if (safe_multiply(a, b, result)) {
            return result;
        }

        // Determine sign of result to clamp correctly
        if constexpr (std::is_unsigned_v<T>) {
            return std::numeric_limits<T>::max();
        } else {
            // Result is positive if signs match, negative if they differ
            bool result_positive = (a > 0) == (b > 0);
            return result_positive ? std::numeric_limits<T>::max()
                                   : std::numeric_limits<T>::min();
        }
    }

    /**
     * @brief Accumulate values with overflow protection
     * @tparam T Integer type
     * @param accumulator Current accumulated value (modified in place)
     * @param value Value to add
     * @return true if accumulation succeeded, false if clamped to limit
     */
    template<std::integral T>
    constexpr bool accumulate_safe(T& accumulator, T value) noexcept {
        T result;
        if (safe_add(accumulator, value, result)) {
            accumulator = result;
            return true;
        }

        // Clamp to limit on overflow
        accumulator = add_clamped(accumulator, value);
        return false;
    }
}