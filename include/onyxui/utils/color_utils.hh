#pragma once

#include <cstdint>
#include <optional>
#include <string_view>

namespace onyxui::color_utils {

/**
 * @brief RGB color components with alpha channel
 *
 * Represents a color as separate red, green, blue, and alpha components.
 * Each component is in the range [0, 255].
 */
struct rgb_components {
    std::uint8_t r; ///< Red component (0-255)
    std::uint8_t g; ///< Green component (0-255)
    std::uint8_t b; ///< Blue component (0-255)
    std::uint8_t a; ///< Alpha component (0-255, 255=opaque)

    constexpr bool operator==(const rgb_components&) const noexcept = default;
};

/**
 * @brief Parse 24-bit RGB hex color (0xRRGGBB)
 * @param hex 24-bit color value (e.g., 0xFF0000 for red)
 * @return RGB components with alpha=255 (fully opaque)
 *
 * Format: 0xRRGGBB
 * - Bits 23-16: Red channel
 * - Bits 15-8:  Green channel
 * - Bits 7-0:   Blue channel
 * - Alpha:      255 (fully opaque)
 *
 * Example:
 * @code
 * auto red = parse_hex_rgb(0xFF0000);    // {255, 0, 0, 255}
 * auto white = parse_hex_rgb(0xFFFFFF);  // {255, 255, 255, 255}
 * @endcode
 */
[[nodiscard]] constexpr rgb_components parse_hex_rgb(std::uint32_t hex) noexcept {
    return rgb_components{
        .r = static_cast<std::uint8_t>((hex >> 16) & 0xFF),
        .g = static_cast<std::uint8_t>((hex >> 8) & 0xFF),
        .b = static_cast<std::uint8_t>(hex & 0xFF),
        .a = 255 // Fully opaque
    };
}

/**
 * @brief Parse 32-bit RGBA hex color (0xRRGGBBAA)
 * @param hex 32-bit color value (e.g., 0xFF0000FF for opaque red)
 * @return RGB components with specified alpha
 *
 * Format: 0xRRGGBBAA
 * - Bits 31-24: Red channel
 * - Bits 23-16: Green channel
 * - Bits 15-8:  Blue channel
 * - Bits 7-0:   Alpha channel
 *
 * Example:
 * @code
 * auto red_opaque = parse_hex_rgba(0xFF0000FF);    // {255, 0, 0, 255}
 * auto red_half = parse_hex_rgba(0xFF000080);      // {255, 0, 0, 128}
 * auto transparent = parse_hex_rgba(0xFFFFFF00);   // {255, 255, 255, 0}
 * @endcode
 */
[[nodiscard]] constexpr rgb_components parse_hex_rgba(std::uint32_t hex) noexcept {
    return rgb_components{
        .r = static_cast<std::uint8_t>((hex >> 24) & 0xFF),
        .g = static_cast<std::uint8_t>((hex >> 16) & 0xFF),
        .b = static_cast<std::uint8_t>((hex >> 8) & 0xFF),
        .a = static_cast<std::uint8_t>(hex & 0xFF)
    };
}

/**
 * @brief Parse hex color string to integer value
 * @param str String like "0xFF0000", "0x00FF00AA", "FF0000", etc.
 * @return Hex value if valid, nullopt otherwise
 *
 * Accepts formats:
 * - "0xRRGGBB" or "0XRRGGBB" (6 hex digits with prefix)
 * - "0xRRGGBBAA" or "0XRRGGBBAA" (8 hex digits with prefix)
 * - "RRGGBB" (6 hex digits without prefix)
 * - "RRGGBBAA" (8 hex digits without prefix)
 *
 * Returns nullopt if:
 * - String is empty
 * - Invalid characters (not hex digits)
 * - Wrong length (not 6 or 8 digits after optional 0x prefix)
 *
 * Example:
 * @code
 * auto red = parse_hex_string("0xFF0000");      // 0xFF0000
 * auto green = parse_hex_string("00FF00");      // 0x00FF00
 * auto blue_alpha = parse_hex_string("0000FFAA"); // 0x0000FFAA
 * auto invalid = parse_hex_string("ZZZZZZ");    // nullopt
 * @endcode
 */
[[nodiscard]] inline std::optional<std::uint32_t> parse_hex_string(std::string_view str) noexcept {
    if (str.empty()) {
        return std::nullopt;
    }

    // Skip optional 0x/0X prefix
    std::size_t start = 0;
    if (str.size() >= 2 && str[0] == '0' && (str[1] == 'x' || str[1] == 'X')) {
        start = 2;
    }

    // Validate length (6 or 8 hex digits)
    std::size_t const hex_len = str.size() - start;
    if (hex_len != 6 && hex_len != 8) {
        return std::nullopt;
    }

    // Parse hex digits
    std::uint32_t value = 0;
    for (std::size_t i = start; i < str.size(); ++i) {
        char const c = str[i];
        std::uint8_t digit = 0;

        if (c >= '0' && c <= '9') {
            digit = static_cast<std::uint8_t>(c - '0');
        } else if (c >= 'a' && c <= 'f') {
            digit = static_cast<std::uint8_t>(c - 'a' + 10);
        } else if (c >= 'A' && c <= 'F') {
            digit = static_cast<std::uint8_t>(c - 'A' + 10);
        } else {
            // Invalid character
            return std::nullopt;
        }

        value = (value << 4) | digit;
    }

    return value;
}

/**
 * @brief Check if a string is a valid hex color
 * @param str String to validate
 * @return True if string can be parsed as hex color
 *
 * Example:
 * @code
 * is_hex_color("0xFF0000");   // true
 * is_hex_color("FF0000");     // true
 * is_hex_color("0x00FF00AA"); // true
 * is_hex_color("ZZZZZZ");     // false
 * is_hex_color("FF00");       // false (wrong length)
 * @endcode
 */
[[nodiscard]] inline bool is_hex_color(std::string_view str) noexcept {
    return parse_hex_string(str).has_value();
}

} // namespace onyxui::color_utils
