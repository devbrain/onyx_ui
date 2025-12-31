/**
 * @file colors.hh
 * @brief RGBA color type for SDL++ backend
 */

#pragma once

#include <cstdint>

namespace onyxui::sdlpp {

/**
 * @struct color
 * @brief RGBA color with 8-bit components
 *
 * Standard RGBA color representation for graphical rendering.
 */
struct color {
    std::uint8_t r = 0;      ///< Red component (0-255)
    std::uint8_t g = 0;      ///< Green component (0-255)
    std::uint8_t b = 0;      ///< Blue component (0-255)
    std::uint8_t a = 255;    ///< Alpha component (0=transparent, 255=opaque)

    constexpr color() noexcept = default;

    constexpr color(std::uint8_t red, std::uint8_t green, std::uint8_t blue,
                    std::uint8_t alpha = 255) noexcept
        : r(red), g(green), b(blue), a(alpha) {}

    constexpr bool operator==(const color&) const noexcept = default;

    // Common color constants
    [[nodiscard]] static constexpr color black() noexcept { return {0, 0, 0}; }
    [[nodiscard]] static constexpr color white() noexcept { return {255, 255, 255}; }
    [[nodiscard]] static constexpr color red() noexcept { return {255, 0, 0}; }
    [[nodiscard]] static constexpr color green() noexcept { return {0, 255, 0}; }
    [[nodiscard]] static constexpr color blue() noexcept { return {0, 0, 255}; }
    [[nodiscard]] static constexpr color transparent() noexcept { return {0, 0, 0, 0}; }
};

} // namespace onyxui::sdlpp
