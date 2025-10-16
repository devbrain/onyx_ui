//
// Created by igor on 12/10/2025.
//

#pragma once
#include <concepts>
#include <onyxui/concepts/common.hh>

namespace onyxui {
    namespace detail {
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


    }

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
        (detail::has_member_x <T> || detail::has_method_x <T> || detail::has_method_get_x <T>) &&
        (detail::has_member_y <T> || detail::has_method_y <T> || detail::has_method_get_y <T>);

    namespace point_utils {
        /**
         * @brief Get the x coordinate of a point
         */
        template<PointLike P>
        [[nodiscard]] constexpr int get_x(const P& p) noexcept {
            if constexpr (detail::has_member_x <P>) {
                return static_cast <int>(p.x);
            } else if constexpr (detail::has_method_x <P>) {
                return static_cast <int>(p.x());
            } else if constexpr (detail::has_method_get_x <P>) {
                return static_cast <int>(p.get_x());
            } else {
                static_assert(detail::always_false<P>, "No x accessor found");
            }
        }

        /**
         * @brief Get the y coordinate of a point
         */
        template<PointLike P>
        [[nodiscard]] constexpr int get_y(const P& p) noexcept {
            if constexpr (detail::has_member_y <P>) {
                return static_cast <int>(p.y);
            } else if constexpr (detail::has_method_y <P>) {
                return static_cast <int>(p.y());
            } else if constexpr (detail::has_method_get_y <P>) {
                return static_cast <int>(p.get_y());
            } else {
                static_assert(detail::always_false<P>, "No y accessor found");
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
