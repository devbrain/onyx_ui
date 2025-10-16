//
// Created by igor on 12/10/2025.
//

#pragma once

#include <onyxui/concepts/point_like.hh>
#include <onyxui/concepts/size_like.hh>

namespace onyxui {
    /**
     * @concept RectLike
     * @brief Concept for 2D rectangle types with position and dimensions
     *
     * Combines PointLike (for x,y) and SizeLike (for width,height)
     */
    template<typename T>
    concept RectLike = PointLike <T> && SizeLike <T>;

    namespace rect_utils {
        /**
         * @brief Get the x coordinate of a rectangle
         * Tries: r.x, r.x(), r.get_x() in that order
         */
        template<RectLike R>
        [[nodiscard]] constexpr int get_x(const R& r) noexcept {
            if constexpr (detail::has_member_x <R>) {
                return static_cast <int>(r.x);
            } else if constexpr (detail::has_method_x <R>) {
                return static_cast <int>(r.x());
            } else if constexpr (detail::has_method_get_x <R>) {
                return static_cast <int>(r.get_x());
            } else {
                static_assert(detail::always_false<R>, "No x accessor found");
            }
        }

        /**
         * @brief Get the y coordinate of a rectangle
         * Tries: r.y, r.y(), r.get_y() in that order
         */
        template<RectLike R>
        [[nodiscard]] constexpr int get_y(const R& r) noexcept {
            if constexpr (detail::has_member_y <R>) {
                return static_cast <int>(r.y);
            } else if constexpr (detail::has_method_y <R>) {
                return static_cast <int>(r.y());
            } else if constexpr (detail::has_method_get_y <R>) {
                return static_cast <int>(r.get_y());
            } else {
                static_assert(detail::always_false<R>, "No y accessor found");
            }
        }

        /**
         * @brief Get the width of a rectangle
         * Tries: r.width, r.w, r.width(), r.get_width() in that order
         */
        template<RectLike R>
        [[nodiscard]] constexpr int get_width(const R& r) noexcept {
            if constexpr (detail::has_member_width <R>) {
                return static_cast <int>(r.width);
            } else if constexpr (detail::has_member_w <R>) {
                return static_cast <int>(r.w);
            } else if constexpr (detail::has_method_width <R>) {
                return static_cast <int>(r.width());
            } else if constexpr (detail::has_method_get_width <R>) {
                return static_cast <int>(r.get_width());
            } else {
                static_assert(detail::always_false<R>, "No width accessor found");
            }
        }

        /**
         * @brief Get the height of a rectangle
         * Tries: r.height, r.h, r.height(), r.get_height() in that order
         */
        template<RectLike R>
        [[nodiscard]] constexpr int get_height(const R& r) noexcept {
            if constexpr (detail::has_member_height <R>) {
                return static_cast <int>(r.height);
            } else if constexpr (detail::has_member_h <R>) {
                return static_cast <int>(r.h);
            } else if constexpr (detail::has_method_height <R>) {
                return static_cast <int>(r.height());
            } else if constexpr (detail::has_method_get_height <R>) {
                return static_cast <int>(r.get_height());
            } else {
                static_assert(detail::always_false<R>, "No height accessor found");
            }
        }

        /**
         * @brief Set bounds - only works with mutable data members
         * For types with setters, use set_x, set_y, set_width, set_height separately
         */
        template<RectLike R>
            requires detail::has_member_x <R> && detail::has_member_y <R> &&
                     (detail::has_member_w <R> || detail::has_member_width <R>) &&
                     (detail::has_member_h <R> || detail::has_member_height <R>)
        constexpr void set_bounds(R& r, int x, int y, int width, int height) noexcept {
            r.x = x;
            r.y = y;
            if constexpr (detail::has_member_width <R>) {
                r.width = width;
                r.height = height;
            } else {
                r.w = width;
                r.h = height;
            }
        }

        /**
         * @brief Test if a point is contained within a rectangle
         */
        template<RectLike R>
        [[nodiscard]] constexpr bool contains(const R& r, int px, int py) noexcept {
            const int x = get_x(r);
            const int y = get_y(r);
            const int w = get_width(r);
            const int h = get_height(r);
            return px >= x && px < x + w && py >= y && py < y + h;
        }

        /**
         * @brief Create a rectangle if it's aggregate-initializable
         */
        template<RectLike R>
            requires std::is_aggregate_v <R>
        [[nodiscard]] constexpr R make(int x, int y, int w, int h) noexcept {
            return R{x, y, w, h}; // Assumes {x, y, width, height} order
        }

        /**
         * @brief Compare two rectangles for equality
         */
        template<RectLike R1, RectLike R2>
        [[nodiscard]] constexpr bool equal(const R1& a, const R2& b) noexcept {
            return get_x(a) == get_x(b) && get_y(a) == get_y(b) &&
                   get_width(a) == get_width(b) && get_height(a) == get_height(b);
        }
    }
}
