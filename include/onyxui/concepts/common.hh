/**
 * @file common.hh
 * @brief Common utilities for C++20 concepts and template metaprogramming
 * @author igor
 * @date 16/10/2025
 *
 * This file provides common utilities used across concept definitions,
 * particularly for template metaprogramming patterns.
 */

#pragma once

namespace onyxui::detail {
    /**
     * @brief Helper for static_assert in constexpr branches
     *
     * Used to make static_assert depend on a template parameter, ensuring
     * it's only evaluated when the branch is instantiated. This is the
     * proper C++20 way to have static_assert in constexpr else branches.
     *
     * @example
     * @code
     * template<typename T>
     * void foo() {
     *     if constexpr (condition<T>) {
     *         // do something
     *     } else {
     *         static_assert(always_false<T>, "T doesn't meet requirements");
     *     }
     * }
     * @endcode
     */
    template<typename>
    inline constexpr bool always_false = false;
}
