---
sidebar_position: 5
---

# Theme Development Guide

This comprehensive guide will teach you how to create beautiful, maintainable themes for OnyxUI using the advanced theming features introduced in 2025. You'll learn about hex color notation, color palettes, theme inheritance, and smart defaults that reduce theme size from 300+ lines to just 9 lines!

## Quick Start: Minimal Theme

The easiest way to create a theme is using **smart defaults**. You only need to specify 3 colors:

```yaml
# themes/my_minimal_theme.yaml
name: "My Minimal Theme"
description: "A theme created with just 3 colors!"

window_bg: 0x1E1E1E    # Dark gray background
text_fg: 0xD4D4D4      # Light gray text
border_color: 0x007ACC # Blue accents

# That's it! Smart defaults will generate 50+ theme values automatically:
# - Button states (normal, hover, pressed, disabled)
# - Label styles (text, background)
# - Panel styles (border, background)
# - Menu styles (items, bar, hover, disabled)
# - Scrollbar components (track, thumb, arrows, hover states)
```

**Result**: A fully functional theme in just **9 lines**!

## Feature Overview

OnyxUI's theming system offers four powerful features:

| Feature | Benefit | Example |
|---------|---------|---------|
| **Hex Color Notation** | Type less, read easier | `0xFF0000` instead of `{r: 255, g: 0, b: 0}` |
| **Color Palettes** | Define once, reuse everywhere | `$primary` references palette color |
| **Theme Inheritance** | Create variants easily | `extends: "Base Theme"` |
| **Smart Defaults** | Minimal themes (3 colors!) | Auto-generates 50+ values |

### Size Reduction Comparison

| Method | Theme Size | Reduction |
|--------|-----------|-----------|
| **Traditional** | 300 lines | 0% |
| **With Hex Colors** | 120 lines | 60% |
| **With Palette** | 90 lines | 70% |
| **With Inheritance** | 10 lines (variants) | 97% |
| **With Smart Defaults** | **9 lines** | **97%** |

## Tutorial: Building a Custom Theme

Let's create a complete theme from scratch, progressing from basic to advanced techniques.

### Step 1: Traditional Theme (Long Form)

Start with the traditional format to understand all available properties:

```yaml
name: "Ocean Theme"
description: "Professional blue ocean-inspired theme"

# Basic colors
window_bg: { r: 26, g: 27, b: 38 }      # Dark blue-gray
text_fg: { r: 224, g: 247, b: 250 }    # Light cyan
border_color: { r: 41, g: 128, b: 185 } # Ocean blue

# Button styles
button:
  normal:
    font: { bold: false, underline: false, reverse: false }
    foreground: { r: 224, g: 247, b: 250 }
    background: { r: 26, g: 27, b: 38 }
  hover:
    font: { bold: true, underline: false, reverse: false }
    foreground: { r: 41, g: 128, b: 185 }
    background: { r: 34, g: 45, b: 60 }
  pressed:
    font: { bold: false, underline: false, reverse: true }
    foreground: { r: 26, g: 27, b: 38 }
    background: { r: 224, g: 247, b: 250 }
  disabled:
    font: { bold: false, underline: false, reverse: false }
    foreground: { r: 100, g: 100, b: 100 }
    background: { r: 26, g: 27, b: 38 }

# ... 250+ more lines for label, panel, menu, scrollbar, etc.
```

**Size**: ~300 lines
**Maintainability**: Low (repetitive, hard to change colors)

### Step 2: Using Hex Colors (60% Smaller)

Replace verbose color syntax with hex notation:

```yaml
name: "Ocean Theme"
description: "Professional blue ocean-inspired theme"

# Basic colors (hex format - much cleaner!)
window_bg: 0x1A1B26      # Dark blue-gray
text_fg: 0xE0F7FA        # Light cyan
border_color: 0x2980B9   # Ocean blue

# Button styles
button:
  normal:
    font: { bold: false, underline: false, reverse: false }
    foreground: 0xE0F7FA
    background: 0x1A1B26
    mnemonic_foreground: 0xE0F7FA  # Color for underlined mnemonic characters (optional)
  hover:
    font: { bold: true, underline: false, reverse: false }
    foreground: 0x2980B9
    background: 0x222D3C
    mnemonic_foreground: 0x3498DB  # Brighter blue for mnemonic when hovering
  # ... rest of button states

# ... rest of widgets
```

**Size**: ~120 lines
**Maintainability**: Medium (easier to read, still repetitive)

### Step 3: Using Color Palette (70% Smaller)

Define colors once in a palette and reference them:

```yaml
name: "Ocean Theme"
description: "Professional blue ocean-inspired theme"

# Define palette ONCE
palette:
  bg_dark: 0x1A1B26       # Dark blue-gray
  bg_medium: 0x222D3C     # Medium gray
  fg_light: 0xE0F7FA      # Light cyan
  accent: 0x2980B9        # Ocean blue
  accent_bright: 0x3498DB # Bright blue
  disabled: 0x646464      # Gray

# Use palette colors everywhere with $references
window_bg: $bg_dark
text_fg: $fg_light
border_color: $accent

button:
  normal:
    foreground: $fg_light
    background: $bg_dark
    mnemonic_foreground: $fg_light  # Optional: mnemonic color (defaults to foreground)
  hover:
    font: { bold: true }
    foreground: $accent
    background: $bg_medium
    mnemonic_foreground: $accent_bright  # Brighter mnemonic on hover
  pressed:
    font: { reverse: true }
    foreground: $bg_dark
    background: $fg_light
  disabled:
    foreground: $disabled
    background: $bg_dark

# ... rest of widgets using palette
```

**Size**: ~90 lines
**Maintainability**: High (change color once, updates everywhere)

### Step 4: Using Theme Inheritance (97% Smaller for Variants)

Create theme variants by extending an existing theme:

```yaml
# themes/ocean_light.yaml
extends: "Ocean Theme"
name: "Ocean Light"
description: "Light variant of Ocean Theme"

# Override only what you need!
palette:
  bg_dark: 0xF5F5F5       # Light background
  bg_medium: 0xE0E0E0     # Medium light
  fg_light: 0x1A1B26      # Dark text

# That's it! Everything else inherited from Ocean Theme
```

**Size**: ~10 lines
**Maintainability**: Excellent (minimal duplication)

### Step 5: Using Smart Defaults (97% Smaller - Minimal)

Let the framework generate most theme values automatically:

```yaml
# themes/ocean_minimal.yaml
name: "Ocean Minimal"
description: "Minimal Ocean theme with smart defaults"

# Only 3 colors needed!
window_bg: 0x1A1B26      # Dark blue-gray
text_fg: 0xE0F7FA        # Light cyan
border_color: 0x2980B9   # Ocean blue

# Smart defaults automatically generate:
# ✓ Button: normal (from window_bg + text_fg)
# ✓ Button: hover (lightened background + accent)
# ✓ Button: pressed (inverted colors)
# ✓ Button: disabled (darkened foreground)
# ✓ Button: mnemonic colors (defaults to match foreground for each state)
# ✓ Label: text color (from text_fg)
# ✓ Label: background (from window_bg)
# ✓ Panel: border (from border_color)
# ✓ Menu: all states (auto-generated, including mnemonics)
# ✓ Scrollbar: all components (auto-generated)
# ... and 40+ more values!
```

**Size**: **9 lines**
**Maintainability**: Excellent (change 3 colors, everything updates)

## Smart Defaults: How It Works

The smart defaults system uses color science to generate beautiful, consistent themes:

### Color Manipulation Algorithms

```cpp
// Hover states: Lighten background 20%
button.hover.background = lighten(button.normal.background, 0.2f);
button.hover.foreground = border_color;  // Use accent
button.hover.font.bold = true;

// Pressed states: Invert colors
button.pressed.foreground = button.normal.background;
button.pressed.background = button.normal.foreground;

// Disabled states: Darken foreground 40%
button.disabled.foreground = darken(button.normal.foreground, 0.4f);
button.disabled.background = button.normal.background;
```

### Luminance Calculations

Smart defaults use **ITU-R BT.709** coefficients for perceptually accurate brightness:

```
luminance = 0.2126 * R + 0.7152 * G + 0.0722 * B
```

This ensures:
- Light backgrounds get dark text
- Dark backgrounds get light text
- Hover states are always visible
- Disabled states are clearly distinguished

## Advanced Features

### Custom Palette with Smart Defaults

Combine palette and smart defaults for maximum control:

```yaml
name: "Custom Advanced"

palette:
  # Define your color scheme
  primary: 0x2980B9
  secondary: 0x27AE60
  accent: 0xE74C3C
  bg: 0x1A1B26
  fg: 0xE0F7FA

# Use palette for key colors
window_bg: $bg
text_fg: $fg
border_color: $primary

# Optionally override specific generated values
button:
  hover:
    foreground: $accent  # Use custom accent instead of generated
```

### Theme Inheritance with Palette Override

Create sophisticated theme families:

```yaml
# themes/ocean_base.yaml
name: "Ocean Base"
palette:
  primary: 0x2980B9
  bg: 0x1A1B26
  fg: 0xE0F7FA

window_bg: $bg
text_fg: $fg
border_color: $primary
```

```yaml
# themes/ocean_green.yaml
extends: "Ocean Base"
name: "Ocean Green"

# Change just the accent color!
palette:
  primary: 0x27AE60  # Green instead of blue

# Entire theme now uses green accents!
```

### Accessibility Themes

Create high-contrast accessible themes:

```yaml
name: "High Contrast"
description: "WCAG AAA compliant high contrast theme"

window_bg: 0x000000      # Pure black
text_fg: 0xFFFFFF        # Pure white
border_color: 0xFFFF00   # Yellow (high visibility)

# Override generated styles for maximum contrast
button:
  normal:
    font: { bold: true }  # Bold for readability
  hover:
    foreground: 0x000000
    background: 0xFFFFFF  # Maximum contrast
    font: { bold: true, underline: true }
```

## Best Practices

### 1. Start with Smart Defaults

Always start with a minimal 3-color theme and only add explicit values when needed:

```yaml
# ✅ GOOD: Start minimal
name: "My Theme"
window_bg: 0x1E1E1E
text_fg: 0xD4D4D4
border_color: 0x007ACC

# ❌ BAD: Don't define everything upfront
button:
  normal:
    foreground: 0xD4D4D4  # Unnecessary - smart defaults will generate
```

### 2. Use Meaningful Palette Names

```yaml
# ✅ GOOD: Semantic names
palette:
  primary: 0x007ACC
  danger: 0xF14C4C
  success: 0x89D185
  bg_panel: 0x252526
  fg_muted: 0x808080

# ❌ BAD: Generic names
palette:
  color1: 0x007ACC
  color2: 0xF14C4C
  blue: 0x007ACC
```

### 3. Test with Different UI Components

```cpp
// Test your theme with all widget types
auto root = std::make_unique<vbox<Backend>>();
root->apply_theme("My Theme", ctx.themes());

root->emplace_child<button>("Button");
root->emplace_child<label>("Label");
root->emplace_child<panel>();
root->emplace_child<menu_bar>();

// Verify:
// - Text is readable on all backgrounds
// - Hover states are clearly visible
// - Disabled states are distinguishable
```

### 4. Consider Color Blindness

Use tools like [Color Oracle](https://colororacle.org/) to test your theme:

```yaml
# Use patterns in addition to color
button:
  hover:
    font: { bold: true }      # Bold + color
  disabled:
    font: { underline: true } # Underline + dimmed
```

### 5. Version Your Themes

```yaml
name: "My Theme v2.0"
description: "Updated with new accent colors - October 2025"

# Document changes in description
# Include version in theme name for clarity
```

## Theme Loading and Registration

### Loading from File

```cpp
#include <onyxui/theming/theme_loader.hh>

// Load single theme
auto theme = theme_loader::load_from_file<Backend>("themes/ocean.yaml");

// Apply to root widget
root->apply_theme(std::move(theme));
```

### Loading from Directory

```cpp
// Load all themes from a directory
auto themes = theme_loader::load_from_directory<Backend>("themes/");

// Themes are automatically added to the registry
// Access by name
root->apply_theme("Ocean Theme", ctx.themes());
```

### Custom Theme Registration

```cpp
// Create theme programmatically
ui_theme<Backend> my_theme;
my_theme.name = "Custom Theme";
my_theme.window_bg = Backend::color_type{30, 30, 30};
// ... set other properties

// Register with theme registry
ctx.themes().register_theme(std::move(my_theme));

// Now available by name
root->apply_theme("Custom Theme", ctx.themes());
```

## Example: Complete Theme Development

Let's create a complete "Gruvbox Dark" inspired theme:

```yaml
name: "Gruvbox Dark"
description: "Retro groove inspired theme with warm colors"

# Step 1: Define comprehensive palette
palette:
  # Background colors
  bg0: 0x282828      # Main background
  bg1: 0x3C3836      # Slightly lighter
  bg2: 0x504945      # Lighter still
  bg3: 0x665C54      # Lightest

  # Foreground colors
  fg0: 0xFBF1C7      # Main foreground
  fg1: 0xEBDBB2      # Slightly dimmer
  fg2: 0xD5C4A1      # Dimmer
  fg3: 0xBDAE93      # Dimmest

  # Accent colors
  red: 0xFB4934      # Bright red
  green: 0xB8BB26    # Bright green
  yellow: 0xFABD2F   # Bright yellow
  blue: 0x83A598     # Bright blue
  purple: 0xD3869B   # Bright purple
  aqua: 0x8EC07C     # Bright aqua
  orange: 0xFE8019   # Bright orange

  # Muted accents
  red_dim: 0xCC2412
  green_dim: 0x98971A
  yellow_dim: 0xD79921
  blue_dim: 0x458588
  purple_dim: 0xB16286
  aqua_dim: 0x689D6A
  orange_dim: 0xD65D0E

# Step 2: Apply palette to base colors
window_bg: $bg0
text_fg: $fg1
border_color: $orange

# Step 3: Smart defaults will generate most styles automatically!

# Step 4: Optionally override specific styles for brand consistency
button:
  hover:
    foreground: $yellow  # Warm yellow highlight
  pressed:
    foreground: $bg0
    background: $orange  # Orange pressed state

menu_bar:
  bar_background: $bg1   # Slightly lighter bar
  bar_foreground: $fg0

scrollbar:
  track_color: $bg1
  thumb_color: $bg2
```

**Result**: A beautiful, warm retro theme in ~60 lines (vs. 300+ traditionally)!

## Troubleshooting

### Theme Not Loading

```cpp
// Check if theme file exists
if (!std::filesystem::exists("themes/my_theme.yaml")) {
    std::cerr << "Theme file not found!\n";
}

// Try loading with error handling
try {
    auto theme = theme_loader::load_from_file<Backend>("themes/my_theme.yaml");
} catch (const std::exception& e) {
    std::cerr << "Failed to load theme: " << e.what() << "\n";
}
```

### Colors Look Wrong

```yaml
# Make sure you're using 0x prefix for hex
window_bg: 0xFF0000  # ✅ Correct
window_bg: FF0000    # ❌ Wrong - parsed as decimal

# Check color order (RGB not BGR)
window_bg: 0xFF0000  # ✅ Red
window_bg: 0x0000FF  # ✅ Blue
```

### Palette References Not Working

```yaml
# Make sure palette is defined BEFORE usage
palette:
  primary: 0x007ACC

window_bg: $primary  # ✅ Correct - palette defined above

# Also check reference syntax
window_bg: $primary   # ✅ Correct - $ prefix
window_bg: primary    # ❌ Wrong - no $ prefix
```

### Theme Inheritance Not Working

```yaml
# Make sure parent theme name matches exactly
extends: "Ocean Theme"  # ✅ Correct - exact name
extends: "ocean theme"  # ❌ Wrong - case sensitive
extends: Ocean Theme    # ❌ Wrong - needs quotes
```

## Next Steps

- **Explore Example Themes**: See `themes/examples/` for 15 pre-built themes
- **Read Theme API Reference**: See [Theming System](../core-concepts/theming-system.md) for API details
- **Color Science Deep Dive**: Read `docs/THEMING_PHASE4_DEFAULTS.md` for algorithm details
- **Contribute Your Theme**: Submit themes to the project via pull request

## Resources

- **Color Palette Generators**:
  - [Coolors](https://coolors.co/) - Generate color schemes
  - [Adobe Color](https://color.adobe.com/) - Color wheel and harmonies
  - [Color Hunt](https://colorhunt.co/) - Curated color palettes

- **Accessibility Testing**:
  - [Color Oracle](https://colororacle.org/) - Simulate color blindness
  - [WebAIM Contrast Checker](https://webaim.org/resources/contrastchecker/) - WCAG compliance

- **Inspiration**:
  - [VS Code Themes](https://vscodethemes.com/) - Modern editor themes
  - [iTerm2 Color Schemes](https://iterm2colorschemes.com/) - Terminal themes
  - [Base16](http://chriskempson.com/projects/base16/) - Color scheme architecture

## Summary

OnyxUI's theming system is designed for both power and simplicity:

- ✅ **Start simple**: 3 colors, 9 lines, fully functional
- ✅ **Scale up**: Add palettes, inheritance, custom overrides as needed
- ✅ **Maintain easily**: Change colors in one place, updates everywhere
- ✅ **Create variants**: Extend existing themes with minimal code
- ✅ **Professional results**: Color science ensures beautiful, accessible themes

**Happy theming!** 🎨
