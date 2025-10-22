//
// Created by igor on 12/10/2025.
//

#pragma once

#include <concepts>
#include <cstdint>
#include <onyxui/concepts/common.hh>

namespace onyxui {
    namespace detail {
        // Color component detection helpers
        template<typename T>
        concept has_member_r = requires(T t)
        {
            { t.r } -> std::convertible_to <uint8_t>;
        };

        template<typename T>
        concept has_method_r = requires(T t)
        {
            { t.r() } -> std::convertible_to <uint8_t>;
        };

        template<typename T>
        concept has_method_get_r = requires(T t)
        {
            { t.get_r() } -> std::convertible_to <uint8_t>;
        };

        template<typename T>
        concept has_member_g = requires(T t)
        {
            { t.g } -> std::convertible_to <uint8_t>;
        };

        template<typename T>
        concept has_method_g = requires(T t)
        {
            { t.g() } -> std::convertible_to <uint8_t>;
        };

        template<typename T>
        concept has_method_get_g = requires(T t)
        {
            { t.get_g() } -> std::convertible_to <uint8_t>;
        };

        template<typename T>
        concept has_member_b = requires(T t)
        {
            { t.b } -> std::convertible_to <uint8_t>;
        };

        template<typename T>
        concept has_method_b = requires(T t)
        {
            { t.b() } -> std::convertible_to <uint8_t>;
        };

        template<typename T>
        concept has_method_get_b = requires(T t)
        {
            { t.get_b() } -> std::convertible_to <uint8_t>;
        };

        template<typename T>
        concept has_member_a = requires(T t)
        {
            { t.a } -> std::convertible_to <uint8_t>;
        };

        template<typename T>
        concept has_method_a = requires(T t)
        {
            { t.a() } -> std::convertible_to <uint8_t>;
        };

        template<typename T>
        concept has_method_get_a = requires(T t)
        {
            { t.get_a() } -> std::convertible_to <uint8_t>;
        };
    }

    /**
     * @concept ColorLike
     * @brief Concept for RGB or RGBA color types with red, green, and blue components (alpha optional)
     *
     * Supports multiple access patterns:
     * - Data members: c.r, c.g, c.b, c.a (0-255)
     * - Methods: c.r(), c.g(), c.b(), c.a()
     * - Getter methods: c.get_r(), c.get_g(), c.get_b(), c.get_a()
     *
     * All components must be convertible to uint8_t (0-255 range).
     * Alpha component is optional - RGB-only colors are also ColorLike.
    */
    template<typename T>
    concept ColorLike =
        (detail::has_member_r <T> || detail::has_method_r <T> || detail::has_method_get_r <T>) &&
        (detail::has_member_g <T> || detail::has_method_g <T> || detail::has_method_get_g <T>) &&
        (detail::has_member_b <T> || detail::has_method_b <T> || detail::has_method_get_b <T>);

    // ======================================================================
    // Improved utility functions that handle both members and methods
    // ======================================================================

    namespace color_utils {
        /**
         * @brief Get the red component of a color
         * Tries: c.r, c.r(), c.get_r() in that order
         */
        template<ColorLike C>
        [[nodiscard]] constexpr uint8_t get_r(const C& c) noexcept {
            if constexpr (detail::has_member_r <C>) {
                return static_cast <uint8_t>(c.r);
            } else if constexpr (detail::has_method_r <C>) {
                return static_cast <uint8_t>(c.r());
            } else if constexpr (detail::has_method_get_r <C>) {
                return static_cast <uint8_t>(c.get_r());
            } else {
                static_assert(detail::always_false<C>, "No red component accessor found");
            }
        }

        /**
         * @brief Get the green component of a color
         * Tries: c.g, c.g(), c.get_g() in that order
         */
        template<ColorLike C>
        [[nodiscard]] constexpr uint8_t get_g(const C& c) noexcept {
            if constexpr (detail::has_member_g <C>) {
                return static_cast <uint8_t>(c.g);
            } else if constexpr (detail::has_method_g <C>) {
                return static_cast <uint8_t>(c.g());
            } else if constexpr (detail::has_method_get_g <C>) {
                return static_cast <uint8_t>(c.get_g());
            } else {
                static_assert(detail::always_false<C>, "No green component accessor found");
            }
        }

        /**
         * @brief Get the blue component of a color
         * Tries: c.b, c.b(), c.get_b() in that order
         */
        template<ColorLike C>
        [[nodiscard]] constexpr uint8_t get_b(const C& c) noexcept {
            if constexpr (detail::has_member_b <C>) {
                return static_cast <uint8_t>(c.b);
            } else if constexpr (detail::has_method_b <C>) {
                return static_cast <uint8_t>(c.b());
            } else if constexpr (detail::has_method_get_b <C>) {
                return static_cast <uint8_t>(c.get_b());
            } else {
                static_assert(detail::always_false<C>, "No blue component accessor found");
            }
        }

        /**
         * @brief Get the alpha component of a color
         * Tries: c.a, c.a(), c.get_a() in that order
         */
        template<ColorLike C>
        [[nodiscard]] constexpr uint8_t get_a(const C& c) noexcept {
            if constexpr (detail::has_member_a <C>) {
                return static_cast <uint8_t>(c.a);
            } else if constexpr (detail::has_method_a <C>) {
                return static_cast <uint8_t>(c.a());
            } else if constexpr (detail::has_method_get_a <C>) {
                return static_cast <uint8_t>(c.get_a());
            } else {
                static_assert(detail::always_false<C>, "No alpha component accessor found");
            }
        }

        /**
         * @brief Create an RGB color with full opacity
         *
         * @param r Red component (0-255)
         * @param g Green component (0-255)
         * @param b Blue component (0-255)
         * @return Color with specified RGB values and alpha=255
         *
         * @example
         * @code
         * auto red = color_utils::rgb<MyColor>(255, 0, 0);
         * @endcode
         */
        template<ColorLike C>
        [[nodiscard]] constexpr C rgb(uint8_t r, uint8_t g, uint8_t b) noexcept {
            return {r, g, b, 255};
        }

        /**
         * @brief Create an RGBA color with custom alpha
         *
         * @param r Red component (0-255)
         * @param g Green component (0-255)
         * @param b Blue component (0-255)
         * @param a Alpha component (0-255, where 0=transparent, 255=opaque)
         * @return Color with specified RGBA values
         *
         * @example
         * @code
         * auto semi_transparent_red = color_utils::rgba<MyColor>(255, 0, 0, 128);
         * @endcode
         */
        template<ColorLike C>
        [[nodiscard]] constexpr C rgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a) noexcept {
            return {r, g, b, a};
        }

        /**
         * @brief Create a color from a 24-bit hex value (0xRRGGBB)
         *
         * @param hex 24-bit color value in format 0xRRGGBB
         * @return Color with alpha=255 (fully opaque)
         *
         * @example
         * @code
         * auto red = color_utils::from_hex<MyColor>(0xFF0000);
         * auto blue = color_utils::from_hex<MyColor>(0x0000FF);
         * @endcode
         */
        template<ColorLike C>
        [[nodiscard]] constexpr C from_hex(uint32_t hex) noexcept {
            return {
                static_cast <uint8_t>((hex >> 16) & 0xFF),
                static_cast <uint8_t>((hex >> 8) & 0xFF),
                static_cast <uint8_t>(hex & 0xFF),
                255
            };
        }

        /**
         * @brief Create a color from a 32-bit hex value with alpha (0xRRGGBBAA)
         *
         * @param hex 32-bit color value in format 0xRRGGBBAA
         * @return Color with all RGBA components from hex
         *
         * @example
         * @code
         * auto semi_red = color_utils::from_rgba_hex<MyColor>(0xFF000080);
         * @endcode
         */
        template<ColorLike C>
        [[nodiscard]] constexpr C from_rgba_hex(uint32_t hex) noexcept {
            return {
                static_cast <uint8_t>((hex >> 24) & 0xFF),
                static_cast <uint8_t>((hex >> 16) & 0xFF),
                static_cast <uint8_t>((hex >> 8) & 0xFF),
                static_cast <uint8_t>(hex & 0xFF)
            };
        }

        /**
         * @brief Convert a color to a 32-bit RGBA hex value
         *
         * @param c The color to convert
         * @return 32-bit value in format 0xRRGGBBAA
         */
        template<ColorLike C>
        [[nodiscard]] constexpr uint32_t to_rgba_hex(const C& c) noexcept {
            return (static_cast <uint32_t>(get_r(c)) << 24) |
                   (static_cast <uint32_t>(get_g(c)) << 16) |
                   (static_cast <uint32_t>(get_b(c)) << 8) |
                   static_cast <uint32_t>(get_a(c));
        }

        /**
         * @brief Compare two colors for equality
         */
        template<ColorLike C1, ColorLike C2>
        [[nodiscard]] constexpr bool equal(const C1& a, const C2& b) noexcept {
            return get_r(a) == get_r(b) && get_g(a) == get_g(b) &&
                   get_b(a) == get_b(b) && get_a(a) == get_a(b);
        }

        /**
         * @brief Create a color if it's aggregate-initializable
         */
        template<ColorLike C>
            requires std::is_aggregate_v <C>
        [[nodiscard]] constexpr C make(uint8_t r, uint8_t g, uint8_t b, uint8_t a) noexcept {
            return {r, g, b, a};
        }

        // -----------------------------------------------------------------------
        // Common color constants (factory functions)
        // -----------------------------------------------------------------------

        template<ColorLike C>
        [[nodiscard]] constexpr C white() noexcept { return {255, 255, 255, 255}; }

        template<ColorLike C>
        [[nodiscard]] constexpr C black() noexcept { return {0, 0, 0, 255}; }

        template<ColorLike C>
        [[nodiscard]] constexpr C red() noexcept { return {255, 0, 0, 255}; }

        template<ColorLike C>
        [[nodiscard]] constexpr C green() noexcept { return {0, 255, 0, 255}; }

        template<ColorLike C>
        [[nodiscard]] constexpr C blue() noexcept { return {0, 0, 255, 255}; }

        template<ColorLike C>
        [[nodiscard]] constexpr C yellow() noexcept { return {255, 255, 0, 255}; }

        template<ColorLike C>
        [[nodiscard]] constexpr C cyan() noexcept { return {0, 255, 255, 255}; }

        template<ColorLike C>
        [[nodiscard]] constexpr C magenta() noexcept { return {255, 0, 255, 255}; }

        template<ColorLike C>
        [[nodiscard]] constexpr C transparent() noexcept { return {0, 0, 0, 0}; }

        template<ColorLike C>
        [[nodiscard]] constexpr C gray(uint8_t value) noexcept { return {value, value, value, 255}; }
    }
}
