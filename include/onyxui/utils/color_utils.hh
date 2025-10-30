#pragma once

#include <cstdint>
#include <optional>
#include <string_view>
#include <string>
#include <cstdio>  // For snprintf

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

// ===========================================================================
// Phase 4: Color Manipulation Utilities
// ===========================================================================

/**
 * @brief Clamp value to range [0, 255]
 * @param value Value to clamp
 * @return Clamped value as uint8_t
 */
[[nodiscard]] constexpr std::uint8_t clamp_u8(int value) noexcept {
    if (value < 0) return 0;
    if (value > 255) return 255;
    return static_cast<std::uint8_t>(value);
}

/**
 * @brief Lighten a color by factor
 * @param color Input color
 * @param factor Lighten factor (0.0-1.0, where 1.0 = full white)
 * @return Lightened color
 *
 * Moves RGB values toward white (255).
 * Alpha channel preserved.
 *
 * Example:
 * @code
 * auto dark_blue = rgb_components{0, 0, 170, 255};
 * auto light_blue = lighten(dark_blue, 0.3f);  // Lighter blue
 * @endcode
 */
[[nodiscard]] constexpr rgb_components lighten(rgb_components color, float factor) noexcept {
    if (factor < 0.0f) factor = 0.0f;
    if (factor > 1.0f) factor = 1.0f;

    return rgb_components{
        .r = clamp_u8(static_cast<int>(static_cast<float>(color.r) + (255.0f - static_cast<float>(color.r)) * factor)),
        .g = clamp_u8(static_cast<int>(static_cast<float>(color.g) + (255.0f - static_cast<float>(color.g)) * factor)),
        .b = clamp_u8(static_cast<int>(static_cast<float>(color.b) + (255.0f - static_cast<float>(color.b)) * factor)),
        .a = color.a  // Preserve alpha
    };
}

/**
 * @brief Darken a color by factor
 * @param color Input color
 * @param factor Darken factor (0.0-1.0, where 1.0 = full black)
 * @return Darkened color
 *
 * Moves RGB values toward black (0).
 * Alpha channel preserved.
 *
 * Example:
 * @code
 * auto bright_yellow = rgb_components{255, 255, 0, 255};
 * auto dark_yellow = darken(bright_yellow, 0.5f);  // Darker yellow
 * @endcode
 */
[[nodiscard]] constexpr rgb_components darken(rgb_components color, float factor) noexcept {
    if (factor < 0.0f) factor = 0.0f;
    if (factor > 1.0f) factor = 1.0f;

    return rgb_components{
        .r = clamp_u8(static_cast<int>(static_cast<float>(color.r) * (1.0f - factor))),
        .g = clamp_u8(static_cast<int>(static_cast<float>(color.g) * (1.0f - factor))),
        .b = clamp_u8(static_cast<int>(static_cast<float>(color.b) * (1.0f - factor))),
        .a = color.a  // Preserve alpha
    };
}

/**
 * @brief Dim a color by reducing alpha
 * @param color Input color
 * @param factor Dim factor (0.0-1.0, where 1.0 = fully transparent)
 * @return Dimmed color with reduced alpha
 *
 * Reduces alpha channel for transparency/dimming effect.
 * RGB channels preserved.
 *
 * Example:
 * @code
 * auto solid_white = rgb_components{255, 255, 255, 255};
 * auto dim_white = dim(solid_white, 0.5f);  // {255, 255, 255, 128}
 * @endcode
 */
[[nodiscard]] constexpr rgb_components dim(rgb_components color, float factor) noexcept {
    if (factor < 0.0f) factor = 0.0f;
    if (factor > 1.0f) factor = 1.0f;

    return rgb_components{
        .r = color.r,
        .g = color.g,
        .b = color.b,
        .a = clamp_u8(static_cast<int>(static_cast<float>(color.a) * (1.0f - factor)))
    };
}

/**
 * @brief Invert a color
 * @param color Input color
 * @return Inverted color (255-r, 255-g, 255-b)
 *
 * Inverts RGB channels: each component becomes (255 - component).
 * Alpha channel preserved.
 *
 * Example:
 * @code
 * auto black = rgb_components{0, 0, 0, 255};
 * auto white = invert(black);  // {255, 255, 255, 255}
 *
 * auto blue = rgb_components{0, 0, 255, 255};
 * auto yellow = invert(blue);  // {255, 255, 0, 255}
 * @endcode
 */
[[nodiscard]] constexpr rgb_components invert(rgb_components color) noexcept {
    return rgb_components{
        .r = static_cast<std::uint8_t>(255 - color.r),
        .g = static_cast<std::uint8_t>(255 - color.g),
        .b = static_cast<std::uint8_t>(255 - color.b),
        .a = color.a  // Preserve alpha
    };
}

/**
 * @brief Calculate relative luminance (perceived brightness)
 * @param color Input color
 * @return Luminance value (0.0-1.0)
 *
 * Uses ITU-R BT.709 coefficients for human perception:
 * Y = 0.2126*R + 0.7152*G + 0.0722*B
 *
 * Example:
 * @code
 * auto black = rgb_components{0, 0, 0, 255};
 * auto lum_black = luminance(black);  // ~0.0
 *
 * auto white = rgb_components{255, 255, 255, 255};
 * auto lum_white = luminance(white);  // ~1.0
 * @endcode
 */
[[nodiscard]] constexpr float luminance(rgb_components color) noexcept {
    // ITU-R BT.709 coefficients
    float const r = static_cast<float>(color.r) / 255.0f;
    float const g = static_cast<float>(color.g) / 255.0f;
    float const b = static_cast<float>(color.b) / 255.0f;

    return 0.2126f * r + 0.7152f * g + 0.0722f * b;
}

/**
 * @brief Get contrasting color (black or white) for readability
 * @param color Input color
 * @return Black {0,0,0,255} or White {255,255,255,255}
 *
 * Returns black for light backgrounds, white for dark backgrounds.
 * Uses luminance threshold of 0.5 (ITU-R BT.709).
 *
 * Example:
 * @code
 * auto dark_blue = rgb_components{0, 0, 170, 255};
 * auto text_color = contrast(dark_blue);  // White (for readability)
 *
 * auto light_yellow = rgb_components{255, 255, 0, 255};
 * auto text_color2 = contrast(light_yellow);  // Black (for readability)
 * @endcode
 */
[[nodiscard]] constexpr rgb_components contrast(rgb_components color) noexcept {
    float const lum = luminance(color);

    // If luminance > 0.5, background is light → use black text
    // If luminance <= 0.5, background is dark → use white text
    if (lum > 0.5f) {
        return rgb_components{0, 0, 0, 255};  // Black
    } else {
        return rgb_components{255, 255, 255, 255};  // White
    }
}

/**
 * @brief Convert RGB components to hex string
 * @param color Input color
 * @param include_alpha If true, format as 0xRRGGBBAA, otherwise 0xRRGGBB
 * @return Hex string representation
 *
 * Example:
 * @code
 * auto red = rgb_components{255, 0, 0, 255};
 * auto hex = to_hex_string(red);  // "0xFF0000"
 * auto hex_alpha = to_hex_string(red, true);  // "0xFF0000FF"
 * @endcode
 */
[[nodiscard]] inline std::string to_hex_string(rgb_components color, bool include_alpha = false) {
    char buffer[16];
    if (include_alpha) {
        snprintf(buffer, sizeof(buffer), "0x%02X%02X%02X%02X",
                color.r, color.g, color.b, color.a);
    } else {
        snprintf(buffer, sizeof(buffer), "0x%02X%02X%02X",
                color.r, color.g, color.b);
    }
    return std::string(buffer);
}

} // namespace onyxui::color_utils
