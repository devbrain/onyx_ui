# OnyxUI Theming System v2.0

This document describes the comprehensive theming system refactored in October 2025.

## Table of Contents

- [Overview](#overview)
- [Thread-Safe Theme Registry](#thread-safe-theme-registry)
- [Global Theming Architecture](#global-theming-architecture)
- [Style-Based Rendering](#style-based-rendering)
- [Stateful Widget Helper](#stateful-widget-helper)
- [CSS-Style Inheritance](#css-style-inheritance)
- [Performance Characteristics](#performance-characteristics)

---

## Overview

The theme system has been refactored with three major improvements:

1. **Thread-Safe Theme Registry** - Reader-writer locking for concurrent access
2. **Global Theming Architecture** - Themes applied at root level only
3. **Style-Based Rendering** - Resolved styles passed via context

---

## Thread-Safe Theme Registry

The `theme_registry` now uses reader-writer locking for thread-safe concurrent access:

```cpp
theme_registry<Backend> registry;  // Thread-safe!

// Multiple threads can safely read themes concurrently
auto* theme = registry.get_theme("Norton Blue");  // Shared lock

// Writes are exclusive
registry.register_theme(my_theme);  // Unique lock
```

All operations are logged via failsafe logging (DEBUG level in debug builds):
- Theme registration: `LOG_INFO("Registered theme: {name}")`
- Theme lookup: `LOG_DEBUG("Found theme: {name}")`
- Theme application: `LOG_DEBUG("Applying theme: {name}")`

**See also:** `include/onyxui/theme_registry.hh`

---

## Global Theming Architecture

Themes are **global** and applied at the **root level only**, not on individual widgets. This simplifies the architecture and ensures consistent styling throughout the UI tree.

### Application Pattern

```cpp
// Apply theme ONCE at the root (RECOMMENDED)
scoped_ui_context<Backend> ctx;
auto root = std::make_unique<panel<Backend>>();
root->apply_theme("Norton Blue", ctx.themes());  // Global theme!

// All children inherit from the global theme via CSS inheritance
auto button = root->template emplace_child<button>();
auto label = root->template emplace_child<label>();
// Both button and label use "Norton Blue" theme automatically
```

### Three Safe Ownership Options

Use at root only:

```cpp
// Option 1: By name (registry-based) - RECOMMENDED
root->apply_theme("Norton Blue", registry);  // Zero overhead

// Option 2: By value (copy/move)
ui_theme<Backend> my_theme = create_custom_theme();
root->apply_theme(std::move(my_theme));  // Element owns theme

// Option 3: By shared_ptr
auto theme_ptr = std::make_shared<ui_theme<Backend>>(my_theme);
root->apply_theme(theme_ptr);  // Reference-counted
```

### CSS Inheritance Chain

Priority order:

1. **Explicit override** on widget (`set_background_color()`)
2. **Parent's resolved style** (CSS inheritance)
3. **Global theme** (via `get_theme()` walk-up)
4. **Default value** (fallback)

### Benefits

- **Simplicity**: One theme for entire UI tree
- **Performance**: O(depth) style resolution with single parent cache
- **Consistency**: Uniform styling across all widgets
- **Safety**: No dangling pointer risk
- **Flexibility**: Individual widgets can still override colors via `set_background_color()`

---

## Style-Based Rendering

The new `resolved_style` structure is the cornerstone of v2.0:

### Resolution Flow

```cpp
// Style is resolved ONCE per frame via CSS inheritance
void render(renderer_type& renderer, const std::vector<rect_type>& dirty_regions) {
    auto style = this->resolve_style();  // Resolve through CSS hierarchy

    draw_context<Backend> ctx(renderer, style, dirty_regions);

    do_render(ctx);  // Widgets use pre-resolved style
}
```

### resolved_style Structure

```cpp
// POD-like structure with all visual properties
template<UIBackend Backend>
struct resolved_style {
    // Required properties (always present)
    color_type background_color;
    color_type foreground_color;
    color_type border_color;
    box_style_type box_style;
    font_type font;
    float opacity;
    std::optional<icon_style_type> icon_style;

    // Optional widget-specific properties (strong-typed wrappers)
    padding_horizontal_t padding_horizontal;  // std::optional<int> internally
    padding_vertical_t padding_vertical;      // std::optional<int> internally
    mnemonic_font_t mnemonic_font;            // std::optional<font_type> internally

    // Access optional values with .value.value_or(default)
    int h_pad = style.padding_horizontal.value.value_or(2);

    // Utility methods (immutable)
    [[nodiscard]] resolved_style with_opacity(float op) const noexcept;
    [[nodiscard]] resolved_style with_colors(color_type bg, color_type fg) const noexcept;
    [[nodiscard]] resolved_style with_font(font_type f) const noexcept;
};
```

### render_context Carries Style

```cpp
template<UIBackend Backend>
class render_context {
    // Access resolved style (common properties)
    [[nodiscard]] const resolved_style<Backend>& style() const noexcept;

    // Access theme for rare properties (nullable)
    [[nodiscard]] const ui_theme<Backend>* theme() const noexcept;

    // Convenience methods using context style
    [[nodiscard]] size_type draw_text(std::string_view text, const point_type& position);
    void draw_rect(const rect_type& bounds);
};
```

### Widget Property Access Pattern

Widgets follow a two-tier access pattern:

```cpp
template<UIBackend Backend>
class button : public stateful_widget<Backend> {
    void do_render(render_context<Backend>& ctx) const override {
        // Common properties: Use pre-resolved style (fast, O(1))
        auto const& fg = ctx.style().foreground_color;
        auto const& font = ctx.style().font;
        int padding = ctx.style().padding_horizontal.value.value_or(2);

        // Rare properties: Access via theme pointer (nullable)
        if (auto* theme = ctx.theme()) {
            auto text_align = theme->button.text_align;
        }

        // NEVER directly access ui_services::themes() from widgets!
    }
};
```

### Why Two-Tier Access?

- **Common properties** (colors, fonts, padding) → `resolved_style` → O(1) access
- **Rare properties** (text_align, line_style) → `ctx.theme()` → Minimal overhead
- **Performance**: Avoids bloating `resolved_style` with rarely-used fields
- **Type safety**: Optional properties enforce explicit default handling

### Benefits

- **Performance**: CSS inheritance resolved once per frame, not per widget
- **Simplicity**: Widgets receive pre-resolved style via context
- **Consistency**: Single source of truth for visual properties
- **Immutability**: `resolved_style` is POD-like with copy semantics

---

## Stateful Widget Helper

New base class for interactive widgets with visual states that automatically integrates with the global theme:

### Interface

```cpp
template<UIBackend Backend>
class stateful_widget : public widget<Backend> {
public:
    enum class interaction_state {
        normal, hover, pressed, disabled
    };

protected:
    // State management
    void set_interaction_state(interaction_state state);
    [[nodiscard]] interaction_state get_interaction_state() const noexcept;

    // State-based color helpers (use global theme automatically)
    template<typename WidgetTheme>
    [[nodiscard]] color_type get_state_background(const WidgetTheme& widget_theme) const noexcept;

    template<typename WidgetTheme>
    [[nodiscard]] color_type get_state_foreground(const WidgetTheme& widget_theme) const noexcept;

    // Convenience queries
    [[nodiscard]] bool is_normal() const noexcept;
    [[nodiscard]] bool is_hovered() const noexcept;
    [[nodiscard]] bool is_pressed() const noexcept;
    [[nodiscard]] bool is_disabled() const noexcept;

    void enable();
    void disable();
};
```

### Usage Pattern

```cpp
template<UIBackend Backend>
class button : public stateful_widget<Backend> {
    // Override theme accessors to return state-dependent colors
    [[nodiscard]] color_type get_theme_background_color(const theme_type& theme) const override {
        return this->get_state_background(theme.button);  // Uses global theme!
    }

    [[nodiscard]] color_type get_theme_foreground_color(const theme_type& theme) const override {
        return this->get_state_foreground(theme.button);  // Uses global theme!
    }

    void on_mouse_enter() override {
        this->set_interaction_state(interaction_state::hover);
        // Automatically calls invalidate_arrange() to trigger redraw
    }
};
```

### Global Theme Structure

```cpp
struct ui_theme {
    struct button_theme {
        color_type bg_normal, fg_normal;
        color_type bg_hover, fg_hover;
        color_type bg_pressed, fg_pressed;
        color_type bg_disabled, fg_disabled;
    } button;

    // Other widget themes...
};
```

### How It Works

1. Global theme applied at root contains `button_theme` settings
2. Button overrides `get_theme_background_color()` to select color based on state
3. `resolve_style()` calls `get_theme_background_color()` during CSS inheritance
4. Result: buttons automatically use correct state colors from global theme

**See also:** `include/onyxui/widgets/stateful_widget.hh`

---

## CSS-Style Inheritance

Properties inherit from parent to child:

```cpp
// Set on parent
window->set_background_color({0, 0, 170});
window->set_foreground_color({255, 255, 255});

// Children automatically inherit
auto button = create_button("OK");
window->add_child(button);  // Button inherits colors

// Override on specific children
button->set_background_color({0, 255, 0});  // Green button
```

### Inheritable Properties

- Colors (background, foreground)
- Renderer styles (box_style, font, icon_style)
- Opacity (multiplicative)

**See also:** `include/onyxui/themeable.hh` for the inheritance system.

---

## Performance Characteristics

- **Style resolution**: O(depth) once per frame with single parent cache
- **Widget rendering**: O(1) style access via pre-resolved context
- **Thread-safe**: Reader-writer locking on registry
- **Memory**: POD-like resolved_style (~50-100 bytes)
- **Scalability**: Tested with 100+ widget trees (both wide and deep)

### Architecture Summary

1. **CSS Inheritance** → Resolves to `resolved_style`
2. **resolved_style** → Passed to `render_context` (draw/measure)
3. **render_context** → Provides style to widgets during rendering
4. **Widgets** → Use pre-resolved style, no repeated lookups

---

## Key Reference Files

- `include/onyxui/resolved_style.hh` - Style structure
- `include/onyxui/render_context.hh` - Visitor pattern base
- `include/onyxui/draw_context.hh` - Rendering implementation
- `include/onyxui/measure_context.hh` - Measurement implementation
- `include/onyxui/widgets/stateful_widget.hh` - Interactive widget helper
- `include/onyxui/theme_registry.hh` - Thread-safe registry
- `include/onyxui/themeable.hh` - Global theming implementation
- `unittest/theming/test_resolved_style.cc` - Style resolution tests
- `unittest/theming/test_style_inheritance.cc` - CSS inheritance tests
- `unittest/theming/test_style_edge_cases.cc` - Edge case coverage (100+ widgets)
