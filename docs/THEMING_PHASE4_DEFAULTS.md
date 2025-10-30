# Phase 4: Smart Defaults

**Status**: Planning
**Prerequisite**: Phase 2 (Palette), Phase 3 (Inheritance)
**Duration**: 3-4 days
**Impact**: Minimal themes in <20 lines

## Overview

Allow partial themes - fill missing values with intelligent defaults:

```yaml
name: "Minimal Blue"

# Just define essential colors
palette:
  primary: 0x0000AA
  accent: 0xFFFF00

# Everything else auto-generated:
# - hover state = accent color + bold
# - pressed = inverted colors
# - disabled = dimmed foreground
# - all widgets get consistent styling
```

## Motivation

**Current**: Must specify 50+ color values even if most follow patterns.

**With defaults**: Specify 3-5 colors, system generates 50+ consistent values.

## Default Generation Rules

### Visual State Defaults

```cpp
// If not specified:
button.hover = {
    .foreground = palette["accent"],
    .background = palette["primary"],
    .font = {.bold = true}  // Highlight
};

button.pressed = {
    .foreground = invert(palette["primary"]),
    .background = invert(palette["accent"])
};

button.disabled = {
    .foreground = dim(palette["foreground"], 0.4),  // 40% opacity
    .background = palette["primary"]
};
```

### Color Derivation

```cpp
// Generate missing colors from base
if (!palette.contains("hover_bg")) {
    palette["hover_bg"] = lighten(palette["primary"], 0.2);
}

if (!palette.contains("disabled")) {
    palette["disabled"] = dim(palette["foreground"], 0.5);
}
```

### Widget Defaults

```cpp
// If label not specified, copy from button
if (!theme.label.specified()) {
    theme.label.text = theme.button.normal.foreground;
    theme.label.background = theme.button.normal.background;
}
```

## Implementation

### Step 1: Theme Completeness Checker

```cpp
namespace onyxui::theme_defaults {
    /**
     * @brief Check which fields are missing
     */
    template<UIBackend Backend>
    struct theme_completeness {
        bool has_button_normal = false;
        bool has_button_hover = false;
        // ... for all fields
    };

    template<UIBackend Backend>
    theme_completeness<Backend> check_completeness(const ui_theme<Backend>& theme);
}
```

### Step 2: Default Generator

```cpp
template<UIBackend Backend>
ui_theme<Backend> apply_defaults(ui_theme<Backend> theme) {
    auto completeness = check_completeness(theme);

    // Apply visual state defaults
    if (!completeness.has_button_hover) {
        theme.button.hover = generate_hover_state(
            theme.button.normal,
            theme.palette["accent"]
        );
    }

    // ... continue for all missing fields

    return theme;
}
```

### Step 3: Color Utilities

```cpp
namespace color_utils {
    // Lighten color by factor (0.0-1.0)
    rgb_components lighten(rgb_components color, float factor);

    // Darken color
    rgb_components darken(rgb_components color, float factor);

    // Dim (reduce alpha or gray out)
    rgb_components dim(rgb_components color, float factor);

    // Invert
    rgb_components invert(rgb_components color);

    // Contrast color (black or white depending on luminance)
    rgb_components contrast(rgb_components color);
}
```

## Testing

```cpp
TEST_CASE("Minimal theme generation") {
    const char* yaml = R"(
name: "Minimal"

palette:
  primary: 0x0000AA
  foreground: 0xFFFFFF
  accent: 0xFFFF00
)";

    auto theme = load_from_string_with_defaults<Backend>(yaml);

    // Verify normal state uses palette
    CHECK(theme.button.normal.background.b == 170);

    // Verify hover state was generated
    CHECK(theme.button.hover.foreground != theme.button.normal.foreground);
    CHECK(theme.button.hover.font.bold == true);  // Default hover = bold

    // Verify disabled state was dimmed
    CHECK(theme.button.disabled.foreground.r < 255);  // Dimmed
}

TEST_CASE("Partial theme fills gaps") {
    const char* yaml = R"(
button:
  normal:
    foreground: 0xFFFFFF
    background: 0x0000AA
  # hover/pressed/disabled missing
)";

    auto theme = load_from_string_with_defaults<Backend>(yaml);

    // Normal specified
    CHECK(theme.button.normal.foreground.r == 255);

    // Others generated
    CHECK(theme.button.hover.font.bold == true);
    CHECK(theme.button.disabled.foreground.r < 255);
}
```

## Acceptance Criteria

- [ ] Minimal theme (3-5 colors) generates complete theme
- [ ] Visual states follow patterns (hover=bold+accent, etc.)
- [ ] Missing widgets copy from similar widgets
- [ ] Color derivation functions work (lighten, darken, etc.)
- [ ] Explicit values override defaults
- [ ] Validation ensures no undefined colors
- [ ] All 1042+ tests pass

## Example

**Before (200 lines)**:
```yaml
button:
  normal: {...}
  hover: {...}
  pressed: {...}
  disabled: {...}
# Repeat for 10 widgets
```

**After (15 lines)**:
```yaml
palette:
  primary: 0x0000AA
  foreground: 0xFFFFFF
  accent: 0xFFFF00

button:
  normal:
    foreground: $foreground
    background: $primary
# Rest auto-generated!
```

---

**Estimated Completion**: 3-4 days
**Lines of Code**: ~250 (defaults + color utils + tests)
**Impact**: Reduce simple themes to <20 lines
