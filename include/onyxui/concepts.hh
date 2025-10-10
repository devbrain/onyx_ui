/**
 * @file concepts.hh
 * @brief Improved C++20 concepts supporting both member variables and methods
 * @author igor
 * @date 10/10/2025
 *
 * This file provides improved concepts that can detect both data members and
 * accessor methods, making them more flexible for different API styles.
 */

#pragma once

#include <concepts>
#include <type_traits>

namespace onyxui {
    // ======================================================================
    // Helper concepts for member detection
    // ======================================================================

    template<typename T>
    concept has_member_x = requires(T t)
    {
        { t.x } -> std::convertible_to <int>;
    };

    template<typename T>
    concept has_method_x = requires(T t)
    {
        { t.x() } -> std::convertible_to <int>;
    };

    template<typename T>
    concept has_method_get_x = requires(T t)
    {
        { t.get_x() } -> std::convertible_to <int>;
    };

    template<typename T>
    concept has_member_y = requires(T t)
    {
        { t.y } -> std::convertible_to <int>;
    };

    template<typename T>
    concept has_method_y = requires(T t)
    {
        { t.y() } -> std::convertible_to <int>;
    };

    template<typename T>
    concept has_method_get_y = requires(T t)
    {
        { t.get_y() } -> std::convertible_to <int>;
    };

    template<typename T>
    concept has_member_w = requires(T t)
    {
        { t.w } -> std::convertible_to <int>;
    };

    template<typename T>
    concept has_member_width = requires(T t)
    {
        { t.width } -> std::convertible_to <int>;
    };

    template<typename T>
    concept has_method_width = requires(T t)
    {
        { t.width() } -> std::convertible_to <int>;
    };

    template<typename T>
    concept has_method_get_width = requires(T t)
    {
        { t.get_width() } -> std::convertible_to <int>;
    };

    template<typename T>
    concept has_member_h = requires(T t)
    {
        { t.h } -> std::convertible_to <int>;
    };

    template<typename T>
    concept has_member_height = requires(T t)
    {
        { t.height } -> std::convertible_to <int>;
    };

    template<typename T>
    concept has_method_height = requires(T t)
    {
        { t.height() } -> std::convertible_to <int>;
    };

    template<typename T>
    concept has_method_get_height = requires(T t)
    {
        { t.get_height() } -> std::convertible_to <int>;
    };

    // ======================================================================
    // Main concepts (improved to support both members and methods)
    // ======================================================================

    /**
     * @concept PointLike
     * @brief Concept for 2D point types with x and y coordinates
     *
     * Supports:
     * - Data members: p.x, p.y
     * - Methods: p.x(), p.y()
     * - Getter methods: p.get_x(), p.get_y()
     */
    template<typename T>
    concept PointLike =
        (has_member_x <T> || has_method_x <T> || has_method_get_x <T>) &&
        (has_member_y <T> || has_method_y <T> || has_method_get_y <T>);

    /**
     * @concept SizeLike
     * @brief Concept for 2D size types with width and height
     *
     * Supports multiple naming conventions:
     * - Data members: s.w/s.h or s.width/s.height
     * - Methods: s.width()/s.height() or s.w()/s.h()
     * - Getter methods: s.get_width()/s.get_height()
     */
    template<typename T>
    concept SizeLike =
        (has_member_w <T> || has_member_width <T> ||
         has_method_width <T> || has_method_get_width <T>) &&
        (has_member_h <T> || has_member_height <T> ||
         has_method_height <T> || has_method_get_height <T>);

    /**
     * @concept RectLike
     * @brief Concept for 2D rectangle types with position and dimensions
     *
     * Combines PointLike (for x,y) and SizeLike (for width,height)
     */
    template<typename T>
    concept RectLike = PointLike <T> && SizeLike <T>;

    // ======================================================================
    // Improved utility functions that handle both members and methods
    // ======================================================================

    namespace rect_utils {
        /**
         * @brief Get the x coordinate of a rectangle
         * Tries: r.x, r.x(), r.get_x() in that order
         */
        template<RectLike R>
        [[nodiscard]] constexpr int get_x(const R& r) noexcept {
            if constexpr (has_member_x <R>) {
                return static_cast <int>(r.x);
            } else if constexpr (has_method_x <R>) {
                return static_cast <int>(r.x());
            } else if constexpr (has_method_get_x <R>) {
                return static_cast <int>(r.get_x());
            } else {
                static_assert(false, "No x accessor found");
            }
        }

        /**
         * @brief Get the y coordinate of a rectangle
         * Tries: r.y, r.y(), r.get_y() in that order
         */
        template<RectLike R>
        [[nodiscard]] constexpr int get_y(const R& r) noexcept {
            if constexpr (has_member_y <R>) {
                return static_cast <int>(r.y);
            } else if constexpr (has_method_y <R>) {
                return static_cast <int>(r.y());
            } else if constexpr (has_method_get_y <R>) {
                return static_cast <int>(r.get_y());
            } else {
                static_assert(false, "No y accessor found");
            }
        }

        /**
         * @brief Get the width of a rectangle
         * Tries: r.width, r.w, r.width(), r.get_width() in that order
         */
        template<RectLike R>
        [[nodiscard]] constexpr int get_width(const R& r) noexcept {
            if constexpr (has_member_width <R>) {
                return static_cast <int>(r.width);
            } else if constexpr (has_member_w <R>) {
                return static_cast <int>(r.w);
            } else if constexpr (has_method_width <R>) {
                return static_cast <int>(r.width());
            } else if constexpr (has_method_get_width <R>) {
                return static_cast <int>(r.get_width());
            } else {
                static_assert(false, "No width accessor found");
            }
        }

        /**
         * @brief Get the height of a rectangle
         * Tries: r.height, r.h, r.height(), r.get_height() in that order
         */
        template<RectLike R>
        [[nodiscard]] constexpr int get_height(const R& r) noexcept {
            if constexpr (has_member_height <R>) {
                return static_cast <int>(r.height);
            } else if constexpr (has_member_h <R>) {
                return static_cast <int>(r.h);
            } else if constexpr (has_method_height <R>) {
                return static_cast <int>(r.height());
            } else if constexpr (has_method_get_height <R>) {
                return static_cast <int>(r.get_height());
            } else {
                static_assert(false, "No height accessor found");
            }
        }

        /**
         * @brief Set bounds - only works with mutable data members
         * For types with setters, use set_x, set_y, set_width, set_height separately
         */
        template<RectLike R>
            requires has_member_x <R> && has_member_y <R> &&
                     (has_member_w <R> || has_member_width <R>) &&
                     (has_member_h <R> || has_member_height <R>)
        constexpr void set_bounds(R& r, int x, int y, int width, int height) noexcept {
            r.x = x;
            r.y = y;
            if constexpr (has_member_width <R>) {
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
            int x = get_x(r);
            int y = get_y(r);
            int w = get_width(r);
            int h = get_height(r);
            return px >= x && px < x + w && py >= y && py < y + h;
        }

        /**
         * @brief Create a rectangle if it's aggregate-initializable
         */
        template<RectLike R>
            requires std::is_aggregate_v <R>
        [[nodiscard]] constexpr R make(int x, int y, int w, int h) noexcept {
            if constexpr (has_member_width <R>) {
                return R{x, y, w, h}; // Assumes {x, y, width, height} order
            } else {
                return R{x, y, w, h}; // Assumes {x, y, w, h} order
            }
        }
    }

    namespace size_utils {
        /**
         * @brief Get the width component of a size
         */
        template<SizeLike S>
        [[nodiscard]] constexpr int get_width(const S& s) noexcept {
            if constexpr (has_member_width <S>) {
                return static_cast <int>(s.width);
            } else if constexpr (has_member_w <S>) {
                return static_cast <int>(s.w);
            } else if constexpr (has_method_width <S>) {
                return static_cast <int>(s.width());
            } else if constexpr (has_method_get_width <S>) {
                return static_cast <int>(s.get_width());
            } else {
                static_assert(false, "No width accessor found");
            }
        }

        /**
         * @brief Get the height component of a size
         */
        template<SizeLike S>
        [[nodiscard]] constexpr int get_height(const S& s) noexcept {
            if constexpr (has_member_height <S>) {
                return static_cast <int>(s.height);
            } else if constexpr (has_member_h <S>) {
                return static_cast <int>(s.h);
            } else if constexpr (has_method_height <S>) {
                return static_cast <int>(s.height());
            } else if constexpr (has_method_get_height <S>) {
                return static_cast <int>(s.get_height());
            } else {
                static_assert(false, "No height accessor found");
            }
        }

        /**
         * @brief Set both width and height - only works with mutable data members
         */
        template<SizeLike S>
            requires (has_member_w <S> || has_member_width <S>) &&
                     (has_member_h <S> || has_member_height <S>)
        constexpr void set_size(S& s, int width, int height) noexcept {
            if constexpr (has_member_width <S>) {
                s.width = width;
                s.height = height;
            } else {
                s.w = width;
                s.h = height;
            }
        }

        /**
         * @brief Create a size if it's aggregate-initializable
         */
        template<SizeLike S>
            requires std::is_aggregate_v <S>
        [[nodiscard]] constexpr S make(int w, int h) noexcept {
            if constexpr (has_member_width <S>) {
                return S{w, h}; // Assumes {width, height} order
            } else {
                return S{w, h}; // Assumes {w, h} order
            }
        }

        /**
         * @brief Compare two sizes for equality
         */
        template<SizeLike S1, SizeLike S2>
        [[nodiscard]] constexpr bool equal(const S1& a, const S2& b) noexcept {
            return get_width(a) == get_width(b) &&
                   get_height(a) == get_height(b);
        }
    }

    namespace point_utils {
        /**
         * @brief Get the x coordinate of a point
         */
        template<PointLike P>
        [[nodiscard]] constexpr int get_x(const P& p) noexcept {
            if constexpr (has_member_x <P>) {
                return static_cast <int>(p.x);
            } else if constexpr (has_method_x <P>) {
                return static_cast <int>(p.x());
            } else if constexpr (has_method_get_x <P>) {
                return static_cast <int>(p.get_x());
            } else {
                static_assert(false, "No x accessor found");
            }
        }

        /**
         * @brief Get the y coordinate of a point
         */
        template<PointLike P>
        [[nodiscard]] constexpr int get_y(const P& p) noexcept {
            if constexpr (has_member_y <P>) {
                return static_cast <int>(p.y);
            } else if constexpr (has_method_y <P>) {
                return static_cast <int>(p.y());
            } else if constexpr (has_method_get_y <P>) {
                return static_cast <int>(p.get_y());
            } else {
                static_assert(false, "No y accessor found");
            }
        }

        /**
         * @brief Create a point if it's aggregate-initializable
         */
        template<PointLike P>
            requires std::is_aggregate_v <P>
        [[nodiscard]] constexpr P make(int x, int y) noexcept {
            return P{x, y};
        }
    }
}
