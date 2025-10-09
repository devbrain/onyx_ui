/**
 * @file concepts.hh
 * @brief C++20 concepts and utility functions for type-agnostic geometric operations
 * @author igor
 * @date 08/10/2025
 *
 * This file defines the core concepts that enable framework-agnostic geometric types.
 * Users can bring their own Point, Size, and Rectangle types without wrappers or conversions,
 * as long as they satisfy the PointLike, SizeLike, or RectLike concepts.
 *
 * @example
 * @code
 * // SDL_Rect works automatically
 * SDL_Rect rect = {10, 20, 100, 50};
 * int w = rect_utils::get_width(rect);  // Returns 100
 *
 * // Custom types work too
 * struct MyRect { int x, y, width, height; };
 * MyRect my_rect = {0, 0, 200, 100};
 * rect_utils::set_bounds(my_rect, 10, 10, 150, 80);
 * @endcode
 */

#pragma once

#include <concepts>

namespace onyxui {
    /**
     * @concept PointLike
     * @brief Concept for 2D point types with x and y coordinates
     *
     * A type satisfies PointLike if it has members `x` and `y` that are
     * convertible to int. This allows working with various point types
     * from different frameworks (SDL_Point, sf::Vector2i, etc.).
     *
     * @tparam T The type to check
     */
    template<typename T>
    concept PointLike = requires(T p)
    {
        { p.x } -> std::convertible_to <int>;
        { p.y } -> std::convertible_to <int>;
    };

    /**
     * @concept SizeLike
     * @brief Concept for 2D size types with width and height
     *
     * A type satisfies SizeLike if it has either:
     * - Members `width` and `height`, OR
     * - Members `w` and `h`
     *
     * Both must be convertible to int. This handles different naming conventions
     * across frameworks (SDL uses w/h, SFML uses width/height).
     *
     * @tparam T The type to check
     */
    template<typename T>
    concept SizeLike = requires(T s)
                       {
                           { s.width } -> std::convertible_to <int>;
                           { s.height } -> std::convertible_to <int>;
                       } || requires(T s)
                       {
                           { s.w } -> std::convertible_to <int>;
                           { s.h } -> std::convertible_to <int>;
                       };

    /**
     * @concept RectLike
     * @brief Concept for 2D rectangle types with position and dimensions
     *
     * A type satisfies RectLike if it has members x, y and either:
     * - Members `w` and `h`, OR
     * - Members `width` and `height`
     *
     * All must be convertible to int. This enables working with SDL_Rect,
     * SFML rectangles, and custom types seamlessly.
     *
     * @tparam T The type to check
     */
    template<typename T>
    concept RectLike = requires(T r)
                       {
                           { r.x } -> std::convertible_to <int>;
                           { r.y } -> std::convertible_to <int>;
                           { r.w } -> std::convertible_to <int>;
                           { r.h } -> std::convertible_to <int>;
                       } || requires(T r)
                       {
                           { r.x } -> std::convertible_to <int>;
                           { r.y } -> std::convertible_to <int>;
                           { r.width } -> std::convertible_to <int>;
                           { r.height } -> std::convertible_to <int>;
                       };

    /**
     * @namespace rect_utils
     * @brief Utility functions for working with RectLike types
     *
     * These template functions provide a uniform interface for accessing
     * and modifying rectangle properties, regardless of whether the underlying
     * type uses w/h or width/height naming conventions.
     */
    namespace rect_utils {
        /**
         * @brief Get the x coordinate of a rectangle
         * @tparam R A RectLike type
         * @param r The rectangle
         * @return The x coordinate
         */
        template<RectLike R>
        int get_x(const R& r) {
            return r.x;
        }

        /**
         * @brief Get the y coordinate of a rectangle
         * @tparam R A RectLike type
         * @param r The rectangle
         * @return The y coordinate
         */
        template<RectLike R>
        int get_y(const R& r) {
            return r.y;
        }

        /**
         * @brief Get the width of a rectangle
         *
         * Automatically detects whether the type uses `width` or `w` member.
         *
         * @tparam R A RectLike type
         * @param r The rectangle
         * @return The width in pixels
         */
        template<RectLike R>
        int get_width(const R& r) {
            if constexpr (requires { r.width; }) {
                return r.width;
            } else {
                return r.w;
            }
        }

        /**
         * @brief Get the height of a rectangle
         *
         * Automatically detects whether the type uses `height` or `h` member.
         *
         * @tparam R A RectLike type
         * @param r The rectangle
         * @return The height in pixels
         */
        template<RectLike R>
        int get_height(const R& r) {
            if constexpr (requires { r.height; }) {
                return r.height;
            } else {
                return r.h;
            }
        }

        /**
         * @brief Set all bounds of a rectangle at once
         *
         * Automatically handles different member naming conventions.
         *
         * @tparam R A RectLike type
         * @param r The rectangle to modify
         * @param x The x coordinate
         * @param y The y coordinate
         * @param width The width in pixels
         * @param height The height in pixels
         */
        template<RectLike R>
        void set_bounds(R& r, int x, int y, int width, int height) {
            r.x = x;
            r.y = y;
            if constexpr (requires { r.width; }) {
                r.width = width;
                r.height = height;
            } else {
                r.w = width;
                r.h = height;
            }
        }

        /**
         * @brief Test if a point is contained within a rectangle
         *
         * Uses half-open interval: point is inside if px >= x && px < x+w
         * and py >= y && py < y+h. The right and bottom edges are exclusive.
         *
         * @tparam R A RectLike type
         * @param r The rectangle
         * @param px The x coordinate of the point
         * @param py The y coordinate of the point
         * @return true if the point is inside the rectangle, false otherwise
         */
        template<RectLike R>
        bool contains(const R& r, int px, int py) {
            int x = get_x(r);
            int y = get_y(r);
            int w = get_width(r);
            int h = get_height(r);
            return px >= x && px < x + w && py >= y && py < y + h;
        }
    }

    /**
     * @namespace size_utils
     * @brief Utility functions for working with SizeLike types
     *
     * These template functions provide a uniform interface for accessing
     * and modifying size properties, regardless of whether the underlying
     * type uses w/h or width/height naming conventions.
     */
    namespace size_utils {
        /**
         * @brief Get the width component of a size
         *
         * Automatically detects whether the type uses `width` or `w` member.
         *
         * @tparam S A SizeLike type
         * @param s The size
         * @return The width in pixels
         */
        template<SizeLike S>
        int get_width(const S& s) {
            if constexpr (requires { s.width; }) {
                return s.width;
            } else {
                return s.w;
            }
        }

        /**
         * @brief Get the height component of a size
         *
         * Automatically detects whether the type uses `height` or `h` member.
         *
         * @tparam S A SizeLike type
         * @param s The size
         * @return The height in pixels
         */
        template<SizeLike S>
        int get_height(const S& s) {
            if constexpr (requires { s.height; }) {
                return s.height;
            } else {
                return s.h;
            }
        }

        /**
         * @brief Set both width and height of a size
         *
         * Automatically handles different member naming conventions.
         *
         * @tparam S A SizeLike type
         * @param s The size to modify
         * @param width The width in pixels
         * @param height The height in pixels
         */
        template<SizeLike S>
        void set_size(S& s, int width, int height) {
            if constexpr (requires { s.width; }) {
                s.width = width;
                s.height = height;
            } else {
                s.w = width;
                s.h = height;
            }
        }

        /**
         * @brief Compare two sizes for equality
         *
         * Returns true if both width and height are equal, regardless of
         * whether the types use the same naming convention.
         *
         * @tparam S1 First SizeLike type
         * @tparam S2 Second SizeLike type
         * @param a First size
         * @param b Second size
         * @return true if sizes are equal, false otherwise
         */
        template<SizeLike S1, SizeLike S2>
        bool equal(const S1& a, const S2& b) {
            return get_width(a) == get_width(b) &&
                   get_height(a) == get_height(b);
        }
    }
}
