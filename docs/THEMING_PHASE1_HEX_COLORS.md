# Phase 1: Hex Color Notation

**Status**: Planning
**Prerequisite**: None (foundation)
**Duration**: 3-5 days
**Impact**: 40% verbosity reduction

## Overview

Replace verbose RGB struct notation with concise hex notation:

```yaml
# Before
foreground: { r: 255, g: 255, b: 255 }

# After
foreground: 0xFFFFFF
```

Support both RGB and RGBA formats:
- `0xRRGGBB` - 24-bit RGB (alpha = 255)
- `0xRRGGBBAA` - 32-bit RGBA

## Motivation

**Current verbosity**:
```yaml
# 47 characters per color
foreground: { r: 255, g: 255, b: 255 }
```

**With hex notation**:
```yaml
# 20 characters per color (57% reduction)
foreground: 0xFFFFFF
```

**Typical theme**: 80+ color definitions × 27 character savings = **2,160 characters saved** per theme.

## Architecture

### Design Principle

**Colors are backend-specific opaque types** - backends own color parsing:

```cpp
// Backend defines color type
struct conio_backend {
    struct color {
        uint8_t r, g, b;
    };

    // Backend provides hex parser
    static color parse_color_hex(uint32_t hex) noexcept;
};
```

**Framework provides utility** - helpers for common parsing:

```cpp
namespace onyxui::color_utils {
    // Extract RGB from hex
    struct rgb_components {
        uint8_t r, g, b, a;
    };

    rgb_components parse_hex_rgb(uint32_t hex) noexcept;
    rgb_components parse_hex_rgba(uint32_t hex) noexcept;

    // String to integer
    std::optional<uint32_t> parse_hex_string(std::string_view str) noexcept;
}
```

### Component Diagram

```
┌─────────────────────────────────────────────────┐
│ YAML Theme File                                 │
│                                                 │
│  foreground: 0xFFFFFF                           │
│  background: "0x0000AA"                         │
└─────────────────┬───────────────────────────────┘
                  │
                  ▼
┌─────────────────────────────────────────────────┐
│ theme_loader.hh                                 │
│                                                 │
│  Parse YAML node:                               │
│    if (string starts with "0x")                 │
│      → parse_hex_string()                       │
│    else if (struct {r, g, b})                   │
│      → parse_rgb_struct()                       │
└─────────────────┬───────────────────────────────┘
                  │
                  ▼
┌─────────────────────────────────────────────────┐
│ color_utils::parse_hex_string()                 │
│                                                 │
│  "0xFFFFFF" → 0xFFFFFF (uint32_t)               │
└─────────────────┬───────────────────────────────┘
                  │
                  ▼
┌─────────────────────────────────────────────────┐
│ Backend::parse_color_hex()                      │
│                                                 │
│  0xFFFFFF → color{255, 255, 255}                │
└─────────────────────────────────────────────────┘
```

## Implementation

### Step 1: Color Utility Functions

**File**: `include/onyxui/utils/color_utils.hh`

```cpp
#pragma once

#include <cstdint>
#include <string_view>
#include <optional>

namespace onyxui::color_utils {
    /**
     * @brief RGB/RGBA components extracted from hex value
     */
    struct rgb_components {
        std::uint8_t r;
        std::uint8_t g;
        std::uint8_t b;
        std::uint8_t a;  // 255 if not specified
    };

    /**
     * @brief Parse 24-bit hex RGB (0xRRGGBB)
     * @param hex 24-bit RGB value (e.g., 0xFFFFFF for white)
     * @return RGB components with alpha = 255
     */
    [[nodiscard]] constexpr rgb_components parse_hex_rgb(std::uint32_t hex) noexcept {
        return {
            static_cast<std::uint8_t>((hex >> 16) & 0xFF),  // R
            static_cast<std::uint8_t>((hex >> 8) & 0xFF),   // G
            static_cast<std::uint8_t>(hex & 0xFF),          // B
            255                                              // A (opaque)
        };
    }

    /**
     * @brief Parse 32-bit hex RGBA (0xRRGGBBAA)
     * @param hex 32-bit RGBA value (e.g., 0xFF0000FF for blue with full alpha)
     * @return RGBA components
     */
    [[nodiscard]] constexpr rgb_components parse_hex_rgba(std::uint32_t hex) noexcept {
        return {
            static_cast<std::uint8_t>((hex >> 24) & 0xFF),  // R
            static_cast<std::uint8_t>((hex >> 16) & 0xFF),  // G
            static_cast<std::uint8_t>((hex >> 8) & 0xFF),   // B
            static_cast<std::uint8_t>(hex & 0xFF)           // A
        };
    }

    /**
     * @brief Parse hex string to integer (0xRRGGBB or 0xRRGGBBAA)
     * @param str String representation (e.g., "0xFFFFFF", "0xFF0000FF")
     * @return Hex value if valid, std::nullopt otherwise
     *
     * @details
     * Accepts:
     * - "0xRRGGBB" (6 hex digits) → 24-bit RGB
     * - "0xRRGGBBAA" (8 hex digits) → 32-bit RGBA
     * - Case insensitive (0xFF or 0xff)
     *
     * Rejects:
     * - Missing "0x" prefix
     * - Invalid hex characters
     * - Wrong length (not 6 or 8 digits)
     */
    [[nodiscard]] std::optional<std::uint32_t> parse_hex_string(std::string_view str) noexcept;
}
```

**Implementation** (`color_utils.cc` or inline in header):

```cpp
std::optional<std::uint32_t> parse_hex_string(std::string_view str) noexcept {
    // Must start with "0x" or "0X"
    if (str.size() < 3 || str[0] != '0' || (str[1] != 'x' && str[1] != 'X')) {
        return std::nullopt;
    }

    // Remove "0x" prefix
    str.remove_prefix(2);

    // Must be 6 digits (RGB) or 8 digits (RGBA)
    if (str.size() != 6 && str.size() != 8) {
        return std::nullopt;
    }

    // Parse hex string
    std::uint32_t result = 0;
    for (char c : str) {
        result <<= 4;
        if (c >= '0' && c <= '9') {
            result |= static_cast<std::uint32_t>(c - '0');
        } else if (c >= 'a' && c <= 'f') {
            result |= static_cast<std::uint32_t>(c - 'a' + 10);
        } else if (c >= 'A' && c <= 'F') {
            result |= static_cast<std::uint32_t>(c - 'A' + 10);
        } else {
            return std::nullopt;  // Invalid hex character
        }
    }

    return result;
}
```

### Step 2: Backend Color Parsing

**File**: `backends/conio/include/onyxui/conio/color_parsing.hh` (new)

```cpp
#pragma once

#include <onyxui/utils/color_utils.hh>
#include <onyxui/conio/colors.hh>

namespace onyxui::conio {
    /**
     * @brief Parse hex color to conio color type
     * @param hex 24-bit or 32-bit hex value
     * @return conio color (alpha ignored for TUI)
     */
    [[nodiscard]] constexpr color parse_color_hex(std::uint32_t hex) noexcept {
        // For 24-bit RGB, alpha is in high byte (0x00RRGGBB)
        // For 32-bit RGBA, we have 0xRRGGBBAA

        // Detect format by checking if value > 0xFFFFFF
        if (hex > 0xFFFFFF) {
            // 32-bit RGBA format
            auto const rgba = color_utils::parse_hex_rgba(hex);
            return color{rgba.r, rgba.g, rgba.b};  // Ignore alpha for TUI
        } else {
            // 24-bit RGB format
            auto const rgb = color_utils::parse_hex_rgb(hex);
            return color{rgb.r, rgb.g, rgb.b};
        }
    }
}
```

### Step 3: YAML Deserialization Support

**File**: `include/onyxui/utils/fkyaml_adapter.hh` (modify existing)

Add specialization for color deserialization that supports both formats:

```cpp
// In the color deserialization section

template<>
struct node_converter<onyxui::conio::color> {
    static onyxui::conio::color decode(const fkyaml::node& n) {
        // Try hex string first (new format)
        if (n.is_string()) {
            auto str = n.get_value<std::string>();
            auto hex = onyxui::color_utils::parse_hex_string(str);
            if (hex) {
                return onyxui::conio::parse_color_hex(*hex);
            }
            throw fkyaml::exception("Invalid hex color format: " + str);
        }

        // Fall back to RGB struct (legacy format)
        if (n.is_mapping()) {
            uint8_t r = n["r"].get_value<int>();
            uint8_t g = n["g"].get_value<int>();
            uint8_t b = n["b"].get_value<int>();
            return onyxui::conio::color{r, g, b};
        }

        throw fkyaml::exception("Color must be hex string or {r, g, b} struct");
    }
};
```

### Step 4: Update Theme Examples

Update one example theme to demonstrate new format:

**File**: `themes/examples/norton_blue_hex.yaml` (new)

```yaml
name: "Norton Blue (Hex)"
description: "Classic Norton Utilities - hex notation demo"

# Global colors (hex format)
window_bg: 0x0000AA      # Dark blue
text_fg: 0xFFFFFF        # White
border_color: 0xFFFF00   # Yellow

button:
  normal:
    font: { bold: false, reverse: false, underline: false }
    foreground: 0xFFFFFF   # White
    background: 0x0000AA   # Dark blue

  hover:
    font: { bold: true, reverse: false, underline: false }
    foreground: 0xFFFF00   # Yellow
    background: 0x0000FF   # Bright blue

  pressed:
    foreground: 0x000000   # Black
    background: 0xAAAAAA   # Light gray

  disabled:
    foreground: 0x555555   # Dark gray
    background: 0x0000AA   # Dark blue

# ... rest of theme
```

## Testing

### Unit Tests

**File**: `unittest/utils/test_color_utils.cc`

```cpp
TEST_CASE("color_utils - parse_hex_rgb") {
    // White
    auto white = color_utils::parse_hex_rgb(0xFFFFFF);
    CHECK(white.r == 255);
    CHECK(white.g == 255);
    CHECK(white.b == 255);
    CHECK(white.a == 255);

    // Red
    auto red = color_utils::parse_hex_rgb(0xFF0000);
    CHECK(red.r == 255);
    CHECK(red.g == 0);
    CHECK(red.b == 0);

    // Blue
    auto blue = color_utils::parse_hex_rgb(0x0000FF);
    CHECK(blue.r == 0);
    CHECK(blue.g == 0);
    CHECK(blue.b == 255);
}

TEST_CASE("color_utils - parse_hex_rgba") {
    // Blue with 50% alpha
    auto blue = color_utils::parse_hex_rgba(0x0000FF80);
    CHECK(blue.r == 0);
    CHECK(blue.g == 0);
    CHECK(blue.b == 255);
    CHECK(blue.a == 128);

    // Red fully opaque
    auto red = color_utils::parse_hex_rgba(0xFF0000FF);
    CHECK(red.r == 255);
    CHECK(red.a == 255);
}

TEST_CASE("color_utils - parse_hex_string") {
    // Valid formats
    CHECK(*color_utils::parse_hex_string("0xFFFFFF") == 0xFFFFFF);
    CHECK(*color_utils::parse_hex_string("0xFF0000") == 0xFF0000);
    CHECK(*color_utils::parse_hex_string("0x0000FF") == 0x0000FF);
    CHECK(*color_utils::parse_hex_string("0xFF0000FF") == 0xFF0000FF);

    // Case insensitive
    CHECK(*color_utils::parse_hex_string("0xffffff") == 0xFFFFFF);
    CHECK(*color_utils::parse_hex_string("0xAbCdEf") == 0xABCDEF);

    // Invalid formats
    CHECK_FALSE(color_utils::parse_hex_string("FFFFFF"));      // Missing 0x
    CHECK_FALSE(color_utils::parse_hex_string("0xFFF"));       // Too short
    CHECK_FALSE(color_utils::parse_hex_string("0xFFFFFFFF0")); // Too long
    CHECK_FALSE(color_utils::parse_hex_string("0xGGGGGG"));    // Invalid chars
    CHECK_FALSE(color_utils::parse_hex_string(""));            // Empty
}
```

**File**: `backends/conio/unittest/test_color_parsing.cc`

```cpp
TEST_CASE("conio - parse_color_hex RGB") {
    using namespace onyxui::conio;

    // White
    auto white = parse_color_hex(0xFFFFFF);
    CHECK(white.r == 255);
    CHECK(white.g == 255);
    CHECK(white.b == 255);

    // Norton blue
    auto norton_blue = parse_color_hex(0x0000AA);
    CHECK(norton_blue.r == 0);
    CHECK(norton_blue.g == 0);
    CHECK(norton_blue.b == 170);
}

TEST_CASE("conio - parse_color_hex RGBA") {
    using namespace onyxui::conio;

    // Blue with alpha (alpha ignored for TUI)
    auto blue = parse_color_hex(0x0000FF80);
    CHECK(blue.r == 0);
    CHECK(blue.g == 0);
    CHECK(blue.b == 255);
    // Alpha is ignored for conio backend
}
```

### Integration Tests

**File**: `backends/conio/unittest/test_hex_theme_loading.cc`

```cpp
TEST_CASE("Load theme with hex notation") {
    const char* yaml = R"(
name: "Hex Test"
window_bg: 0x0000AA
text_fg: 0xFFFFFF
border_color: 0xFFFF00

button:
  normal:
    foreground: 0xFFFFFF
    background: 0x0000AA
)";

    auto theme = theme_loader::load_from_string<conio_backend>(yaml);

    CHECK(theme.name == "Hex Test");
    CHECK(theme.window_bg.r == 0);
    CHECK(theme.window_bg.g == 0);
    CHECK(theme.window_bg.b == 170);

    CHECK(theme.button.normal.foreground.r == 255);
    CHECK(theme.button.normal.foreground.g == 255);
    CHECK(theme.button.normal.foreground.b == 255);
}

TEST_CASE("Backward compatibility - RGB struct still works") {
    const char* yaml = R"(
name: "Legacy Test"
window_bg: { r: 0, g: 0, b: 170 }
text_fg: { r: 255, g: 255, b: 255 }
)";

    auto theme = theme_loader::load_from_string<conio_backend>(yaml);

    CHECK(theme.window_bg.b == 170);
    CHECK(theme.text_fg.r == 255);
}

TEST_CASE("Mixed notation in same theme") {
    const char* yaml = R"(
name: "Mixed Test"
window_bg: 0x0000AA                    # Hex
text_fg: { r: 255, g: 255, b: 255 }   # Struct
border_color: 0xFFFF00                 # Hex
)";

    auto theme = theme_loader::load_from_string<conio_backend>(yaml);

    CHECK(theme.window_bg.b == 170);
    CHECK(theme.text_fg.r == 255);
    CHECK(theme.border_color.r == 255);
}
```

### Error Handling Tests

```cpp
TEST_CASE("Invalid hex format error messages") {
    const char* yaml = R"(
window_bg: 0xGGGGGG
)";

    CHECK_THROWS_WITH_AS(
        theme_loader::load_from_string<conio_backend>(yaml),
        "Invalid hex color format: 0xGGGGGG",
        fkyaml::exception
    );
}

TEST_CASE("Missing 0x prefix") {
    const char* yaml = R"(
window_bg: FFFFFF
)";

    // Should fail to parse as hex, try to parse as struct, fail
    CHECK_THROWS(theme_loader::load_from_string<conio_backend>(yaml));
}
```

## Acceptance Criteria

- [ ] `color_utils::parse_hex_rgb()` extracts RGB correctly
- [ ] `color_utils::parse_hex_rgba()` extracts RGBA correctly
- [ ] `color_utils::parse_hex_string()` validates format
- [ ] Backend `parse_color_hex()` creates backend color
- [ ] YAML themes load with `0xRRGGBB` notation
- [ ] YAML themes load with `0xRRGGBBAA` notation
- [ ] Backward compatibility: `{r, g, b}` still works
- [ ] Mixed notation works in same theme
- [ ] Invalid hex rejected with clear error
- [ ] All existing tests pass (1042+)
- [ ] Zero regressions

## Migration Guide

**For theme authors**:

```yaml
# Old way (still works)
foreground: { r: 255, g: 255, b: 255 }

# New way (preferred)
foreground: 0xFFFFFF

# Both work, choose based on preference
```

**Converting existing themes**:

```bash
# Simple regex replacement (manual review recommended)
sed -i 's/{ r: \([0-9]*\), g: \([0-9]*\), b: \([0-9]*\) }/HEX_PLACEHOLDER/g' theme.yaml
# Then manually convert using color picker or calculator
```

## Next Phase

After Phase 1 is complete and tested:
→ **Phase 2: Color Palette System** (see THEMING_PHASE2_PALETTE.md)

---

**Estimated Completion**: 3-5 days
**Lines of Code**: ~200 (utilities + tests)
**Theme Size Reduction**: 40%
