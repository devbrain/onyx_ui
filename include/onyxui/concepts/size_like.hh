//
// Created by igor on 12/10/2025.
//

#pragma once

#include <concepts>
#include <onyxui/concepts/common.hh>

// Forward declaration for logical_unit support
namespace onyxui {
    class logical_unit;
}

namespace onyxui {
    namespace detail {
        // Concept to detect logical_unit type
        template<typename T>
        concept is_logical_unit = std::same_as<std::remove_cvref_t<T>, logical_unit>;

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

        // Support for logical_unit width member
        template<typename T>
        concept has_logical_unit_width = requires(T t)
        {
            { t.width } -> is_logical_unit;
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

        // Support for logical_unit height member
        template<typename T>
        concept has_logical_unit_height = requires(T t)
        {
            { t.height } -> is_logical_unit;
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
    }

    /**
     * @concept SizeLike
     * @brief Concept for 2D size types with width and height
     *
     * Supports multiple naming conventions:
     * - Data members: s.w/s.h or s.width/s.height (including logical_unit)
     * - Methods: s.width()/s.height() or s.w()/s.h()
     * - Getter methods: s.get_width()/s.get_height()
     */
    template<typename T>
    concept SizeLike =
        (detail::has_member_w <T> || detail::has_member_width <T> ||
         detail::has_logical_unit_width <T> ||
         detail::has_method_width <T> || detail::has_method_get_width <T>) &&
        (detail::has_member_h <T> || detail::has_member_height <T> ||
         detail::has_logical_unit_height <T> ||
         detail::has_method_height <T> || detail::has_method_get_height <T>);

    namespace size_utils {
        /**
         * @brief Get the width component of a size
         */
        template<SizeLike S>
        [[nodiscard]] constexpr int get_width(const S& s) noexcept {
            if constexpr (detail::has_member_width <S>) {
                return static_cast <int>(s.width);
            } else if constexpr (detail::has_logical_unit_width <S>) {
                return s.width.to_int();
            } else if constexpr (detail::has_member_w <S>) {
                return static_cast <int>(s.w);
            } else if constexpr (detail::has_method_width <S>) {
                return static_cast <int>(s.width());
            } else if constexpr (detail::has_method_get_width <S>) {
                return static_cast <int>(s.get_width());
            } else {
                static_assert(detail::always_false<S>, "No width accessor found");
            }
        }

        /**
         * @brief Get the height component of a size
         */
        template<SizeLike S>
        [[nodiscard]] constexpr int get_height(const S& s) noexcept {
            if constexpr (detail::has_member_height <S>) {
                return static_cast <int>(s.height);
            } else if constexpr (detail::has_logical_unit_height <S>) {
                return s.height.to_int();
            } else if constexpr (detail::has_member_h <S>) {
                return static_cast <int>(s.h);
            } else if constexpr (detail::has_method_height <S>) {
                return static_cast <int>(s.height());
            } else if constexpr (detail::has_method_get_height <S>) {
                return static_cast <int>(s.get_height());
            } else {
                static_assert(detail::always_false<S>, "No height accessor found");
            }
        }

        /**
         * @brief Set both width and height - only works with mutable data members
         */
        template<SizeLike S>
            requires (detail::has_member_w <S> || detail::has_member_width <S>) &&
                     (detail::has_member_h <S> || detail::has_member_height <S>)
        constexpr void set_size(S& s, int width, int height) noexcept {
            if constexpr (detail::has_member_width <S>) {
                s.width = width;
                s.height = height;
            } else {
                s.w = width;
                s.h = height;
            }
        }

        /**
         * @brief Create a size if it's aggregate-initializable
         * @note Aggregate initialization uses positional order, not member names
         */
        template<SizeLike S>
            requires std::is_aggregate_v <S>
        [[nodiscard]] constexpr S make(int w, int h) noexcept {
            return S{w, h};  // Works for both {w, h} and {width, height}
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
}
