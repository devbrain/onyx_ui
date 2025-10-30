//
// Created by igor on 13/10/2025.
//

#pragma once

#include <cstdint>
#include <onyxui/utils/color_utils.hh>

namespace onyxui::conio {
    struct color {
        // Color component constants
        static constexpr uint8_t MIN_COMPONENT = 0;
        static constexpr uint8_t MAX_COMPONENT = 255;

        constexpr color(uint8_t red, uint8_t green, uint8_t blue)
            : r(red),
              g(green),
              b(blue) {
        }

        // Default constructor - black
        constexpr color()
            : r(MIN_COMPONENT),
              g(MIN_COMPONENT),
              b(MIN_COMPONENT) {
        }

        /**
         * @brief Parse hex color from integer value
         * @param hex 24-bit RGB value (0xRRGGBB)
         * @return color instance
         *
         * Supports both 24-bit RGB (0xRRGGBB) and 32-bit RGBA (0xRRGGBBAA).
         * For RGBA format, the alpha channel is ignored since conio::color only stores RGB.
         *
         * Example:
         * @code
         * auto red = color::parse_color_hex(0xFF0000);       // Red
         * auto green = color::parse_color_hex(0x00FF00);     // Green
         * auto blue = color::parse_color_hex(0x0000FF);      // Blue
         * auto white = color::parse_color_hex(0xFFFFFF);     // White
         * auto red_alpha = color::parse_color_hex(0xFF0000FF); // Red (alpha ignored)
         * @endcode
         */
        [[nodiscard]] static constexpr color parse_color_hex(std::uint32_t hex) noexcept {
            // Try parsing as RGB first (most common case)
            auto const components = color_utils::parse_hex_rgb(hex);
            return color{components.r, components.g, components.b};
        }

        // Comparison operator for theme defaults
        constexpr bool operator==(const color&) const noexcept = default;

        uint8_t r, g, b;
    };
}
