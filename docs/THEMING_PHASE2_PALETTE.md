# Phase 2: Color Palette System

**Status**: Planning
**Prerequisite**: Phase 1 (Hex Colors)
**Duration**: 5-7 days
**Impact**: 60-70% verbosity reduction

## Overview

Define colors once in a `palette` section, reference them anywhere with `$name`:

```yaml
palette:
  bg: 0x0000AA          # Define once
  fg: 0xFFFFFF
  accent: 0xFFFF00

button:
  normal:
    foreground: $fg     # Reference by name
    background: $bg
  hover:
    foreground: $accent # Same name, consistent color
    background: $bg
```

**Benefits**:
- Change a color in one place, propagates everywhere
- Semantic names (bg, fg, accent) instead of hex values
- Enforces color consistency across theme
- Reduces theme size by 60-70%

## Motivation

**Current problem** (even with hex):
```yaml
# Define dark blue 20+ times
window_bg: 0x0000AA
button.normal.background: 0x0000AA
button.disabled.background: 0x0000AA
label.background: 0x0000AA
panel.background: 0x0000AA
menu.background: 0x0000AA
# ... 15 more times

# To change dark blue → must edit 20+ places
# Easy to miss one and break consistency
```

**With palette**:
```yaml
palette:
  bg: 0x0000AA          # Define ONCE

# Use everywhere
window_bg: $bg
button.normal.background: $bg
button.disabled.background: $bg
# ... all use $bg

# Change color? Edit one line!
```

## Architecture

### Processing Pipeline

```
┌─────────────────────────────────────┐
│ YAML Theme File                     │
│                                     │
│  palette:                           │
│    bg: 0x0000AA                     │
│                                     │
│  button:                            │
│    normal:                          │
│      background: $bg                │
└──────────────┬──────────────────────┘
               │
               ▼
┌─────────────────────────────────────┐
│ Step 1: Parse YAML to tree          │
│   (fkyaml::node tree)               │
└──────────────┬──────────────────────┘
               │
               ▼
┌─────────────────────────────────────┐
│ Step 2: Extract palette              │
│   palette_map = {                   │
│     "bg": 0x0000AA,                 │
│     "fg": 0xFFFFFF,                 │
│     ...                             │
│   }                                 │
└──────────────┬──────────────────────┘
               │
               ▼
┌─────────────────────────────────────┐
│ Step 3: Resolve $references         │
│   Walk tree, replace "$bg" with     │
│   actual value 0x0000AA             │
└──────────────┬──────────────────────┘
               │
               ▼
┌─────────────────────────────────────┐
│ Step 4: Deserialize to ui_theme     │
│   (existing theme_loader logic)     │
└─────────────────────────────────────┘
```

**Key**: Preprocessing step before deserialization, transparent to existing code.

### Data Structures

```cpp
namespace onyxui::theme_palette {
    /**
     * @brief Palette entry - color value with optional name
     */
    struct palette_entry {
        std::string name;      // "bg", "primary", etc.
        std::uint32_t value;   // Hex color value
    };

    /**
     * @brief Palette map - color name → hex value
     */
    using palette_map = std::unordered_map<std::string, std::uint32_t>;

    /**
     * @brief Extract palette from YAML tree
     * @param root YAML tree root
     * @return Palette map, empty if no palette defined
     */
    [[nodiscard]] palette_map extract_palette(const fkyaml::node& root);

    /**
     * @brief Resolve $references in YAML tree
     * @param tree YAML tree to modify in-place
     * @param palette Palette to resolve references from
     * @throws std::runtime_error if reference not found in palette
     */
    void resolve_references(fkyaml::node& tree, const palette_map& palette);
}
```

## Implementation

### Step 1: Palette Extraction

**File**: `include/onyxui/theming/theme_palette.hh` (new)

```cpp
#pragma once

#ifdef ONYXUI_ENABLE_YAML_THEMES

#include <onyxui/utils/color_utils.hh>
#include <fkyaml/node.hpp>
#include <unordered_map>
#include <string>
#include <cstdint>

namespace onyxui::theme_palette {
    using palette_map = std::unordered_map<std::string, std::uint32_t>;

    /**
     * @brief Extract palette from YAML root node
     *
     * @details
     * Looks for "palette:" section at root level:
     *
     * palette:
     *   bg: 0x0000AA
     *   fg: 0xFFFFFF
     *   accent: 0xFFFF00
     *
     * Each entry can be:
     * - Hex string: "0xRRGGBB" or "0xRRGGBBAA"
     * - Legacy struct: { r: 255, g: 255, b: 255 }
     *
     * @param root YAML root node
     * @return Palette map (name → hex value)
     */
    [[nodiscard]] palette_map extract_palette(const fkyaml::node& root);

    /**
     * @brief Resolve $references in YAML tree
     *
     * @details
     * Walks the entire YAML tree and replaces string nodes
     * starting with "$" with the corresponding palette value.
     *
     * Example:
     *   $bg → 0x0000AA (from palette)
     *   $accent → 0xFFFF00
     *
     * @param tree YAML tree to modify in-place (recursive)
     * @param palette Palette to resolve from
     * @throws std::runtime_error if $reference not in palette
     */
    void resolve_references(fkyaml::node& tree, const palette_map& palette);

    /**
     * @brief Helper: Check if string is a $reference
     */
    [[nodiscard]] bool is_reference(std::string_view str) noexcept;

    /**
     * @brief Helper: Extract reference name (strip $)
     * @example "$bg" → "bg"
     */
    [[nodiscard]] std::string extract_reference_name(std::string_view ref);
}

#endif // ONYXUI_ENABLE_YAML_THEMES
```

**Implementation** (`theme_palette.cc`):

```cpp
#include <onyxui/theming/theme_palette.hh>
#include <stdexcept>

namespace onyxui::theme_palette {

palette_map extract_palette(const fkyaml::node& root) {
    palette_map palette;

    // Check if palette section exists
    if (!root.contains("palette") || !root["palette"].is_mapping()) {
        return palette;  // Empty palette is valid (no references allowed)
    }

    const auto& palette_node = root["palette"];

    // Iterate over palette entries
    for (const auto& [key, value] : palette_node.items()) {
        std::string name = key.template get_value<std::string>();

        // Parse color value (hex or struct)
        std::uint32_t hex_value = 0;

        if (value.is_string()) {
            // Hex format: "0xFFFFFF"
            std::string hex_str = value.template get_value<std::string>();
            auto parsed = color_utils::parse_hex_string(hex_str);
            if (!parsed) {
                throw std::runtime_error(
                    "Invalid hex color in palette '" + name + "': " + hex_str
                );
            }
            hex_value = *parsed;
        } else if (value.is_mapping() && value.contains("r") &&
                   value.contains("g") && value.contains("b")) {
            // Legacy struct format: { r: 255, g: 255, b: 255 }
            uint8_t r = value["r"].template get_value<int>();
            uint8_t g = value["g"].template get_value<int>();
            uint8_t b = value["b"].template get_value<int>();
            hex_value = (r << 16) | (g << 8) | b;
        } else {
            throw std::runtime_error(
                "Invalid color format in palette '" + name +
                "': must be hex string or {r, g, b} struct"
            );
        }

        palette[name] = hex_value;
    }

    return palette;
}

void resolve_references(fkyaml::node& tree, const palette_map& palette) {
    // Handle different node types
    if (tree.is_string()) {
        // Check if this is a $reference
        std::string value = tree.template get_value<std::string>();
        if (is_reference(value)) {
            std::string ref_name = extract_reference_name(value);

            // Lookup in palette
            auto it = palette.find(ref_name);
            if (it == palette.end()) {
                throw std::runtime_error(
                    "Undefined palette reference: " + value +
                    " (available: " + list_palette_names(palette) + ")"
                );
            }

            // Replace with hex string
            std::uint32_t hex = it->second;
            char hex_str[16];
            std::snprintf(hex_str, sizeof(hex_str), "0x%06X", hex);
            tree = fkyaml::node(std::string(hex_str));
        }
    } else if (tree.is_sequence()) {
        // Recursively resolve array elements
        for (auto& item : tree) {
            resolve_references(item, palette);
        }
    } else if (tree.is_mapping()) {
        // Recursively resolve map values (skip "palette" section itself)
        for (auto& [key, value] : tree.items()) {
            std::string key_str = key.template get_value<std::string>();
            if (key_str != "palette") {  // Don't resolve palette itself
                resolve_references(value, palette);
            }
        }
    }
    // Scalars (int, bool, etc.) - no resolution needed
}

bool is_reference(std::string_view str) noexcept {
    return !str.empty() && str[0] == '$';
}

std::string extract_reference_name(std::string_view ref) {
    if (ref.empty() || ref[0] != '$') {
        return std::string(ref);
    }
    return std::string(ref.substr(1));  // Strip leading $
}

// Helper for error messages
std::string list_palette_names(const palette_map& palette) {
    std::string result;
    for (const auto& [name, _] : palette) {
        if (!result.empty()) result += ", ";
        result += name;
    }
    return result.empty() ? "none" : result;
}

} // namespace
```

### Step 2: Integrate into Theme Loader

**File**: `include/onyxui/theming/theme_loader.hh` (modify)

Add palette preprocessing step:

```cpp
template<UIBackend Backend>
ui_theme<Backend> load_from_string(const std::string& yaml_content) {
    LOG_DEBUG("Parsing theme from YAML string");

    try {
        // Step 1: Parse YAML to tree
        fkyaml::node root = fkyaml::node::deserialize(yaml_content);

        // Step 2: Extract palette (NEW)
        auto palette = theme_palette::extract_palette(root);

        // Step 3: Resolve $references (NEW)
        if (!palette.empty()) {
            theme_palette::resolve_references(root, palette);
        }

        // Step 4: Deserialize to theme (existing logic)
        auto theme = root.get_value<ui_theme<Backend>>();

        LOG_DEBUG("Theme '", theme.name, "' loaded successfully");
        return theme;

    } catch (const fkyaml::exception& e) {
        std::string error = "Failed to parse theme: " + std::string(e.what());
        LOG_ERROR(error);
        throw std::runtime_error(error);
    }
}
```

### Step 3: Update Example Theme

**File**: `themes/examples/norton_blue_palette.yaml` (new)

```yaml
name: "Norton Blue (Palette)"
description: "Classic Norton Utilities with color palette"

# Define colors ONCE
palette:
  bg: 0x0000AA          # Dark blue
  fg: 0xFFFFFF          # White
  accent: 0xFFFF00      # Yellow
  hover_bg: 0x0000FF    # Bright blue
  pressed_fg: 0x000000  # Black
  pressed_bg: 0xAAAAAA  # Light gray
  disabled: 0x555555    # Dark gray

# Global colors use palette
window_bg: $bg
text_fg: $fg
border_color: $accent

# Button states use palette
button:
  normal:
    font: { bold: false, underline: false, reverse: false }
    foreground: $fg
    background: $bg

  hover:
    font: { bold: true, underline: false, reverse: false }
    foreground: $accent
    background: $hover_bg

  pressed:
    font: { bold: false, underline: false, reverse: false }
    foreground: $pressed_fg
    background: $pressed_bg

  disabled:
    font: { bold: false, underline: false, reverse: false }
    foreground: $disabled
    background: $bg

  mnemonic_font: { bold: false, underline: true, reverse: false }
  box_style: single_line
  padding_horizontal: 2
  padding_vertical: 0
  text_align: center

# Labels use palette
label:
  text: $fg
  background: $bg
  font: { bold: false, underline: false, reverse: false }
  mnemonic_font: { bold: false, underline: true, reverse: false }

# Panels use palette
panel:
  background: $bg
  border_color: $accent
  box_style: single_line
  has_border: true

# Menu uses palette
menu:
  background: $bg
  border_color: $accent
  box_style: single_line

# Menu items use palette
menu_item:
  normal:
    font: { bold: false, underline: false, reverse: false }
    foreground: $fg
    background: $bg

  highlighted:
    font: { bold: true, underline: false, reverse: false }
    foreground: $accent
    background: $hover_bg

  disabled:
    font: { bold: false, underline: false, reverse: false }
    foreground: $disabled
    background: $bg

  shortcut:
    font: { bold: false, underline: false, reverse: false }
    foreground: $disabled  # Dimmed
    background: $bg

  mnemonic_font: { bold: false, underline: true, reverse: false }
  padding_horizontal: 8
  padding_vertical: 1

# Scrollbar uses palette
scrollbar:
  width: 1
  min_thumb_size: 1
  style: classic

  track_normal:
    foreground: $disabled
    background: $bg
    box_style: { draw_border: true }

  thumb_normal:
    foreground: $fg
    background: $bg
    box_style: { draw_border: true }

  thumb_hover:
    foreground: $accent
    background: $hover_bg
    box_style: { draw_border: true }

  thumb_pressed:
    foreground: $pressed_fg
    background: $pressed_bg
    box_style: { draw_border: true }

  arrow_normal:
    foreground: $fg
    background: $bg
    box_style: { draw_border: true }

  arrow_hover:
    foreground: $accent
    background: $hover_bg
    box_style: { draw_border: true }

  arrow_pressed:
    foreground: $pressed_fg
    background: $pressed_bg
    box_style: { draw_border: true }
```

**Size comparison**:
- Without palette: ~300 lines (20+ color duplicates)
- With palette: ~120 lines (60% reduction)
- Changing primary color: 1 line edit vs 20+ line edits

## Testing

### Unit Tests

**File**: `unittest/theming/test_theme_palette.cc` (new)

```cpp
#include <onyxui/theming/theme_palette.hh>
#include <doctest/doctest.h>

TEST_CASE("theme_palette - extract_palette basic") {
    const char* yaml = R"(
palette:
  bg: 0x0000AA
  fg: 0xFFFFFF
  accent: 0xFFFF00
)";

    fkyaml::node root = fkyaml::node::deserialize(yaml);
    auto palette = theme_palette::extract_palette(root);

    CHECK(palette.size() == 3);
    CHECK(palette["bg"] == 0x0000AA);
    CHECK(palette["fg"] == 0xFFFFFF);
    CHECK(palette["accent"] == 0xFFFF00);
}

TEST_CASE("theme_palette - extract_palette with struct colors") {
    const char* yaml = R"(
palette:
  bg: { r: 0, g: 0, b: 170 }
  fg: 0xFFFFFF
)";

    fkyaml::node root = fkyaml::node::deserialize(yaml);
    auto palette = theme_palette::extract_palette(root);

    CHECK(palette["bg"] == 0x0000AA);
    CHECK(palette["fg"] == 0xFFFFFF);
}

TEST_CASE("theme_palette - resolve_references simple") {
    const char* yaml = R"(
test_color: $bg
)";

    fkyaml::node root = fkyaml::node::deserialize(yaml);
    theme_palette::palette_map palette = {{"bg", 0x0000AA}};

    theme_palette::resolve_references(root, palette);

    CHECK(root["test_color"].get_value<std::string>() == "0x0000AA");
}

TEST_CASE("theme_palette - resolve_references nested") {
    const char* yaml = R"(
button:
  normal:
    foreground: $fg
    background: $bg
  hover:
    foreground: $accent
)";

    fkyaml::node root = fkyaml::node::deserialize(yaml);
    theme_palette::palette_map palette = {
        {"bg", 0x0000AA},
        {"fg", 0xFFFFFF},
        {"accent", 0xFFFF00}
    };

    theme_palette::resolve_references(root, palette);

    CHECK(root["button"]["normal"]["foreground"].get_value<std::string>() == "0xFFFFFF");
    CHECK(root["button"]["normal"]["background"].get_value<std::string>() == "0x0000AA");
    CHECK(root["button"]["hover"]["foreground"].get_value<std::string>() == "0xFFFF00");
}

TEST_CASE("theme_palette - undefined reference throws") {
    const char* yaml = R"(
color: $undefined
)";

    fkyaml::node root = fkyaml::node::deserialize(yaml);
    theme_palette::palette_map palette = {{"bg", 0x0000AA}};

    CHECK_THROWS_WITH_AS(
        theme_palette::resolve_references(root, palette),
        "Undefined palette reference: $undefined",
        std::runtime_error
    );
}

TEST_CASE("theme_palette - is_reference") {
    CHECK(theme_palette::is_reference("$bg"));
    CHECK(theme_palette::is_reference("$primary_color"));
    CHECK_FALSE(theme_palette::is_reference("0xFFFFFF"));
    CHECK_FALSE(theme_palette::is_reference("bg"));
    CHECK_FALSE(theme_palette::is_reference(""));
}

TEST_CASE("theme_palette - extract_reference_name") {
    CHECK(theme_palette::extract_reference_name("$bg") == "bg");
    CHECK(theme_palette::extract_reference_name("$primary_color") == "primary_color");
    CHECK(theme_palette::extract_reference_name("bg") == "bg");  // No $ = passthrough
}
```

### Integration Tests

**File**: `backends/conio/unittest/test_palette_theme_loading.cc`

```cpp
TEST_CASE("Load theme with palette") {
    const char* yaml = R"(
name: "Palette Test"

palette:
  bg: 0x0000AA
  fg: 0xFFFFFF
  accent: 0xFFFF00

window_bg: $bg
text_fg: $fg
border_color: $accent

button:
  normal:
    foreground: $fg
    background: $bg
  hover:
    foreground: $accent
    background: $bg
)";

    auto theme = theme_loader::load_from_string<conio_backend>(yaml);

    CHECK(theme.name == "Palette Test");
    CHECK(theme.window_bg.b == 170);  // $bg = 0x0000AA
    CHECK(theme.text_fg.r == 255);    // $fg = 0xFFFFFF
    CHECK(theme.border_color.r == 255);  // $accent = 0xFFFF00
    CHECK(theme.border_color.g == 255);
    CHECK(theme.border_color.b == 0);

    CHECK(theme.button.normal.foreground.r == 255);  // $fg
    CHECK(theme.button.hover.foreground.r == 255);   // $accent
    CHECK(theme.button.hover.foreground.g == 255);
}

TEST_CASE("Theme without palette works") {
    const char* yaml = R"(
name: "No Palette"
window_bg: 0x0000AA
text_fg: 0xFFFFFF
)";

    auto theme = theme_loader::load_from_string<conio_backend>(yaml);
    CHECK(theme.name == "No Palette");
    CHECK(theme.window_bg.b == 170);
}

TEST_CASE("Palette with undefined reference fails") {
    const char* yaml = R"(
name: "Bad Ref"
palette:
  bg: 0x0000AA

window_bg: $undefined_color
)";

    CHECK_THROWS_WITH_AS(
        theme_loader::load_from_string<conio_backend>(yaml),
        "Undefined palette reference",
        std::runtime_error
    );
}
```

### Visual Test

**File**: `backends/conio/unittest/test_palette_visual.cc`

```cpp
TEST_CASE("Palette theme renders correctly") {
    // Load palette theme
    auto theme = theme_loader::load_from_file<conio_backend>(
        "themes/examples/norton_blue_palette.yaml"
    );

    // Load equivalent non-palette theme
    auto theme_ref = theme_loader::load_from_file<conio_backend>(
        "themes/examples/norton_blue.yaml"
    );

    // Colors should match exactly
    CHECK(theme.window_bg == theme_ref.window_bg);
    CHECK(theme.button.normal.foreground == theme_ref.button.normal.foreground);
    CHECK(theme.button.hover.background == theme_ref.button.hover.background);
    // ... verify all colors match
}
```

## Acceptance Criteria

- [ ] `extract_palette()` parses palette section
- [ ] `resolve_references()` replaces $refs with values
- [ ] Nested $references work (button.normal.foreground)
- [ ] Array $references work (if applicable)
- [ ] Undefined $reference throws with helpful error
- [ ] Empty palette (no palette section) works
- [ ] Mixed $refs and direct colors work
- [ ] Palette with struct colors works
- [ ] All existing tests pass (1042+)
- [ ] Example theme converted to palette

## Documentation

Update documentation to explain palette system:

**File**: `docs/CLAUDE/THEMING.md` - Add section:

```markdown
## Color Palettes

Define colors once, reference everywhere:

\`\`\`yaml
palette:
  primary: 0x0000AA
  accent: 0xFFFF00

# Use anywhere
button:
  normal:
    background: $primary
  hover:
    foreground: $accent
\`\`\`

**Benefits**:
- Change one color, updates everywhere
- Semantic names instead of hex values
- Enforces consistency
```

## Migration Guide

**For theme authors**:

1. Add `palette:` section to existing theme
2. Extract common colors to palette
3. Replace duplicates with $references
4. Test theme loads correctly

**Example conversion**:

```yaml
# Before
window_bg: 0x0000AA
button.normal.background: 0x0000AA
label.background: 0x0000AA

# After
palette:
  bg: 0x0000AA

window_bg: $bg
button.normal.background: $bg
label.background: $bg
```

## Next Phase

After Phase 2 is complete and tested:
→ **Phase 3: Theme Inheritance** (see THEMING_PHASE3_INHERITANCE.md)

---

**Estimated Completion**: 5-7 days
**Lines of Code**: ~400 (palette system + tests)
**Theme Size Reduction**: 60-70% (combined with Phase 1)
