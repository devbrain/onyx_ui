---
sidebar_position: 3
---

# Creating Custom Widgets

One of the most powerful features of OnyxUI is its extensibility. The library is designed to make it easy to create your own custom widgets that integrate seamlessly with the existing ecosystem. This guide will walk you through the process of creating a simple custom widget from scratch.

## The Core Idea: Inherit from `onyxui::widget`

All widgets in OnyxUI inherit from the `onyxui::widget` class, which in turn inherits from `onyxui::ui_element`. The `widget` class provides a number of conveniences that make it easier to create new widgets, such as automatic measurement via the Render Context Pattern.

To create a custom widget, you simply need to:

1.  Create a new class that inherits from `onyxui::widget<Backend>`.
2.  Implement the `do_render()` method to define how your widget is measured and rendered.
3.  Optionally, override `on_...` methods from `event_target` to handle events.
4.  Optionally, add `signal`s for custom, high-level events.

## Example: A Simple Circle Widget

Let's create a simple widget that draws a circle.

### 1. The Header File

First, let's create the header file for our new widget, `circle.hh`:

```cpp
#pragma once

#include <onyxui/widgets/widget.hh>

template<onyxui::UIBackend Backend>
class circle : public onyxui::widget<Backend> {
public:
    using base = onyxui::widget<Backend>;
    using render_context_type = onyxui::render_context<Backend>;
    using color_type = typename Backend::color_type;
    using box_style_type = typename Backend::renderer_type::box_style;

    explicit circle(int radius, color_type color)
        : m_radius(radius), m_color(color) {}

    void do_render(render_context_type& ctx) const override {
        // This single method handles BOTH measurement and rendering!

        // In a real backend, you would have a draw_circle method.
        // For this example, we'll draw a rectangle to represent the circle.

        // The render_context pattern: this code works for both
        // measure_context (size calculation) and draw_context (actual rendering)
        auto bounds = this->bounds();

        // Access pre-resolved style from context (resolved once per frame via CSS)
        const auto& style = ctx.style();

        // Method 1: Use convenience methods (auto-apply style colors)
        ctx.draw_rect(bounds);  // Uses style.box_style automatically

        // Method 2: Explicit control - pass colors from style
        // ctx.draw_rect(bounds, style.box_style);

        // For custom appearance, you can override style properties
        box_style_type custom_style = style.box_style;
        custom_style.draw_border = true;  // Customize for our circle
        ctx.draw_rect(bounds, custom_style);

        // Note: get_content_size() is automatically implemented by the base
        // widget class. It creates a measure_context and calls do_render()!
    }

private:
    int m_radius;
    color_type m_color;
};

// Factory function for easy creation
template<onyxui::UIBackend Backend>
std::unique_ptr<circle<Backend>> create_circle(int radius, typename Backend::color_type color) {
    return std::make_unique<circle<Backend>>(radius, color);
}
```

### 2. Understanding the Code

Let's break down the key parts of this example:

-   **Inheritance:** Our `circle` class inherits from `onyxui::widget<Backend>`, which gives it all the functionality of a standard widget, including automatic measurement through the render_context pattern.

-   **`do_render()`:** This is where the magic happens. We override `do_render()` to define how our widget is both **measured** and **rendered**. The same code works for both operations!
    - When called with a `measure_context`, the draw operations track the bounding box without rendering
    - When called with a `draw_context`, the draw operations render to the screen
    - This single implementation keeps measurement and rendering synchronized automatically

-   **No `get_content_size()` needed:** The base `widget` class automatically implements `get_content_size()` for you! It creates a `measure_context`, calls your `do_render()` method, and returns the measured size. This is the power of the render_context pattern.

-   **Style Access:** The render context provides access to the resolved style through `ctx.style()`, which includes all visual properties after CSS inheritance. This eliminates the need for repeated theme lookups.

-   **Factory Function:** It's a good practice to provide a factory function like `create_circle()` to make it easier to create instances of your widget.

### 3. Accessing Theme Properties (Two-Tier Pattern)

When implementing custom widgets, you need to access visual properties from the theme. OnyxUI uses a **two-tier property access pattern** that balances performance and flexibility:

**Tier 1: Common Properties via `ctx.style()`**

Use the pre-resolved style for common visual properties that all widgets share:

```cpp
void do_render(render_context_type& ctx) const override {
    // Common properties (O(1) access, no theme lookup)
    auto const& background_color = ctx.style().background_color;
    auto const& foreground_color = ctx.style().foreground_color;
    auto const& border_color = ctx.style().border_color;
    auto const& font = ctx.style().font;
    auto const& box_style = ctx.style().box_style;
    float opacity = ctx.style().opacity;

    // Optional common properties with explicit defaults
    int horizontal_padding = ctx.style().padding_horizontal.value.value_or(2);
    int vertical_padding = ctx.style().padding_vertical.value.value_or(0);

    // Use these properties for rendering...
}
```

**Tier 2: Rare Widget-Specific Properties via `ctx.theme()`**

For widget-specific properties that don't belong in the common `resolved_style`, access the theme directly:

```cpp
void do_render(render_context_type& ctx) const override {
    // Common properties from Tier 1
    auto const& fg = ctx.style().foreground_color;

    // Rare widget-specific properties (nullable)
    if (auto* theme = ctx.theme()) {
        // Access widget-specific theme structure
        auto text_align = theme->my_widget.text_align;
        auto line_style = theme->my_widget.line_style;
        // ... use these properties
    } else {
        // Fallback if no theme available (rare)
        auto text_align = text_alignment::left;  // Default
    }
}
```

**Why Two Tiers?**

- **Performance**: Common properties are resolved once per frame via CSS inheritance, cached in `resolved_style`
- **Memory Efficiency**: Rare properties don't bloat the `resolved_style` structure
- **Type Safety**: Optional properties use `.value.value_or(default)` pattern to enforce explicit defaults
- **Clarity**: Clear separation between common and widget-specific properties

**Important Rules:**

❌ **NEVER** directly access `ui_services::themes()` from widget code
✅ **ALWAYS** use `ctx.style()` for common properties
✅ **ALWAYS** use `ctx.theme()` for rare widget-specific properties
✅ **ALWAYS** provide explicit defaults for optional properties

### Migration from Old Pattern (Pre-v2.0)

If you're updating old widget code, here's how the pattern has evolved:

**❌ Old Pattern (Pre-v2.0) - Don't do this:**

```cpp
void do_render(render_context_type& ctx) const override {
    // OLD: Widgets directly queried effective colors
    auto bg = this->get_effective_background_color();
    auto fg = this->get_effective_foreground_color();

    // OLD: Colors passed to renderer methods
    ctx.renderer()->draw_text(bounds, m_text, font, fg, bg);

    // OLD: get_effective_*() walks up tree on every call (expensive!)
}
```

**✅ New Pattern (v2.0+) - Do this instead:**

```cpp
void do_render(render_context_type& ctx) const override {
    // NEW: Style pre-resolved once per frame via CSS inheritance
    const auto& style = ctx.style();

    // NEW Option 1: Convenience methods (cleanest)
    ctx.draw_text(m_text, position);  // Uses style.foreground_color automatically

    // NEW Option 2: Explicit colors from style (more control)
    ctx.draw_text(m_text, position, style.font, style.foreground_color);

    // NEW: O(1) access, no tree walking
}
```

**Why the Change?**

| Old Pattern | New Pattern |
|-------------|-------------|
| `get_effective_*()` walks tree every call | Style resolved once per frame |
| O(depth) per property access | O(1) property access |
| Widgets tightly coupled to theme system | Widgets receive pre-resolved style |
| Difficult to test (requires full tree) | Easy to test (just pass a style) |
| Exponential complexity risk | Linear complexity guaranteed |

**Performance Impact:**

For a widget tree of depth 10 with 100 widgets:
- **Old**: 1000 tree walks per frame (10 per widget × 100 widgets)
- **New**: 100 tree walks per frame (1 per widget × 100 widgets)
- **Improvement**: 10x faster style resolution!

### 4. Using the Custom Widget

Now that we've created our custom widget, we can use it in our application just like any other widget:

```cpp
#include "circle.hh"

// ...

// Create a circle with a radius of 50 and a red color
auto my_circle = create_circle<Backend>(50, {255, 0, 0});

// Add the circle to a parent container
parent->add_child(std::move(my_circle));
```

## Next Steps

This is a very simple example, but it demonstrates the basic principles of creating a custom widget. From here, you can create much more complex widgets by:

-   **Handling events:** Override `on_mouse_down`, `on_key_down`, etc., to make your widget interactive.
-   **Adding signals:** Add `onyxui::signal` members to your widget to emit custom, high-level events.
-   **Creating a custom theme:** Add a new style struct to `ui_theme` to make your widget themable.
-   **Composing other widgets:** Your custom widget can be a container that holds and arranges other widgets.

By following these principles, you can extend the OnyxUI library with a rich set of custom widgets that are tailored to the specific needs of your application.
