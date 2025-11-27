---
sidebar_position: 4
---

# Widget Development Guide

This guide teaches you how to create custom widgets in OnyxUI, covering architecture, coordinate system, state management, and best practices.

## Table of Contents

- [Widget Architecture Overview](#widget-architecture-overview)
- [Relative Coordinate System](#relative-coordinate-system)
- [Creating a Simple Widget](#creating-a-simple-widget)
- [Stateful Widgets](#stateful-widgets)
- [Container Widgets](#container-widgets)
- [Render Context Pattern](#render-context-pattern)
- [Theming Integration](#theming-integration)
- [Event Handling](#event-handling)
- [Best Practices](#best-practices)
- [Common Pitfalls](#common-pitfalls)

---

## Widget Architecture Overview

OnyxUI widgets follow a class hierarchy:

```
ui_element<Backend>              // Base class - layout, rendering, events
  └─ widget<Backend>             // UI widget with theme support
      ├─ label<Backend>          // Simple widget (no children)
      ├─ button<Backend>         // Simple widget with state
      └─ widget_container<Backend>  // Container with border support
          ├─ panel<Backend>      // Generic container
          ├─ menu<Backend>       // Menu container
          └─ group_box<Backend>  // Bordered container with title
```

### Key Interfaces

- **`ui_element`** - Core layout algorithm, rendering, hit testing, dirty tracking
- **`widget`** - Theme integration, default colors, background rendering
- **`widget_container`** - Automatic border rendering, content area management
- **`stateful_widget`** - State management (normal/highlighted/disabled)

---

## Relative Coordinate System

**CRITICAL CONCEPT:** OnyxUI uses **relative coordinates** (since November 2025).

### The Golden Rule

> Widget bounds are **RELATIVE** to parent's content area.
> Rendering coordinates are **ABSOLUTE** screen positions.

### What This Means

```cpp
// After layout, bounds are RELATIVE to parent
auto bounds = this->bounds();
// {x: 0, y: 0, w: 100, h: 50}  <- Always (0,0) relative to parent!

// During rendering, context provides ABSOLUTE screen position
void do_render(render_context<Backend>& ctx) const override {
    const auto& pos = ctx.position();  // Absolute screen coords
    // {x: 45, y: 78}  <- Real screen position
}
```

### Why Relative Coordinates?

**Benefits:**
- ✅ Moving a parent automatically moves all children
- ✅ No coordinate recalculation when repositioning
- ✅ Simpler layout logic (children start at 0,0)
- ✅ Clean separation: storage vs rendering

**Before (wrong):**
```cpp
void do_render(render_context<Backend>& ctx) const override {
    auto bounds = this->bounds();  // ❌ WRONG!
    int x = rect_utils::get_x(bounds);  // Relative coords!
    ctx.draw_text(m_text, {x, y}, font, color);  // Wrong position!
}
```

**After (correct):**
```cpp
void do_render(render_context<Backend>& ctx) const override {
    const auto& pos = ctx.position();  // ✅ Absolute coords
    int x = point_utils::get_x(pos);
    ctx.draw_text(m_text, {x, y}, font, color);  // Correct!
}
```

### Content Area

Parents have a **content area** where children are positioned:

```
┌─────────────────────────────┐
│  Margin                     │
│  ┌───────────────────────┐  │
│  │ Border                │  │
│  │ ┌─────────────────┐   │  │
│  │ │ Padding         │   │  │
│  │ │ ┌───────────┐   │   │  │
│  │ │ │ Content   │ <- Children positioned here (0,0)
│  │ │ │ Area      │   │   │  │
│  │ │ └───────────┘   │   │  │
│  │ └─────────────────┘   │  │
│  └───────────────────────┘  │
└─────────────────────────────┘
```

**Example:**
```cpp
// Element with padding + border
element.set_padding({5, 5, 5, 5});
element.set_has_border(true);  // +1 logical unit

// Content area is RELATIVE to element's bounds
auto content_area = element.get_content_area();
// {x: 6, y: 6, w: width-12, h: height-12}
//  ^^^
//  border(1) + padding(5) = 6 logical unit offset

// Children positioned at content area origin
child->arrange({0_lu, 0_lu, 100_lu, 50_lu});  // Relative (0,0)
```

---

## Creating a Simple Widget

Let's create a custom `color_box` widget that displays a colored rectangle with text.

### Step 1: Header Structure

```cpp
// include/onyxui/widgets/color_box.hh
#pragma once

#include <onyxui/widgets/core/widget.hh>
#include <string>

namespace onyxui {

template<UIBackend Backend>
class color_box : public widget<Backend> {
public:
    using base = widget<Backend>;
    using color_type = typename Backend::color_type;

    // Constructor
    explicit color_box(std::string text = "",
                       ui_element<Backend>* parent = nullptr)
        : base(parent)
        , m_text(std::move(text))
        , m_color{255, 0, 0}  // Default red
    {}

    // Setters
    void set_text(std::string text) {
        if (m_text != text) {
            m_text = std::move(text);
            this->invalidate_measure();  // Text size changed
        }
    }

    void set_color(color_type color) {
        if (m_color != color) {
            m_color = color;
            this->invalidate_visual();  // Only visual changed
        }
    }

protected:
    // Rendering (measurement + drawing)
    void do_render(render_context<Backend>& ctx) const override;

private:
    std::string m_text;
    color_type m_color;
};

} // namespace onyxui
```

### Step 2: Implement Rendering

The **render context pattern** uses a single `do_render()` method for both **measurement** and **rendering**:

```cpp
template<UIBackend Backend>
void color_box<Backend>::do_render(render_context<Backend>& ctx) const {
    using renderer_type = typename Backend::renderer_type;

    // CRITICAL: Use ctx.position() for absolute screen coords!
    const auto& pos = ctx.position();
    const auto& bounds = this->bounds();  // Relative coords (for size)

    // Get absolute position
    int x = point_utils::get_x(pos);
    int y = point_utils::get_y(pos);
    int w = rect_utils::get_width(bounds);
    int h = rect_utils::get_height(bounds);

    // Draw colored background
    typename Backend::rect_type bg_rect;
    rect_utils::set_bounds(bg_rect, x, y, w, h);
    ctx.fill_rect(bg_rect);  // Uses resolved style background color

    // Draw custom colored box (smaller, centered)
    typename Backend::rect_type color_rect;
    constexpr int MARGIN = 2;
    rect_utils::set_bounds(color_rect,
        x + MARGIN, y + MARGIN,
        w - 2*MARGIN, h - 2*MARGIN);

    // For custom colors not in style, direct renderer access is needed
    // Context methods use style colors, but we need our custom color
    if (auto* renderer = ctx.renderer()) {
        // During rendering: renderer is not null
        renderer->fill_rect(color_rect, m_color);
    }
    // During measurement: renderer is null, no-op (bounding box tracked)

    // Draw text centered
    if (!m_text.empty()) {
        typename renderer_type::font font{};
        auto text_size = renderer_type::measure_text(m_text, font);
        int text_w = size_utils::get_width(text_size);
        int text_h = size_utils::get_height(text_size);

        // Center text
        int text_x = x + (w - text_w) / 2;
        int text_y = y + (h - text_h) / 2;

        typename Backend::point_type text_pos{text_x, text_y};
        typename Backend::color_type text_color{255, 255, 255};  // White
        ctx.draw_text(m_text, text_pos, font, text_color);
    }
}
```

### Step 3: Usage

```cpp
#include <onyxui/widgets/color_box.hh>

// Create widget
auto box = std::make_unique<color_box<Backend>>("Hello!");
box->set_color({0, 255, 0});  // Green

// Add to layout
root->add_child(std::move(box));

// Framework handles measure/arrange/render automatically!
```

---

## Stateful Widgets

For widgets with visual states (normal/highlighted/disabled), inherit from `stateful_widget`:

```cpp
#include <onyxui/widgets/core/stateful_widget.hh>

template<UIBackend Backend>
class custom_button : public stateful_widget<Backend> {
public:
    using base = stateful_widget<Backend>;

    explicit custom_button(std::string label,
                          ui_element<Backend>* parent = nullptr)
        : base(parent)
        , m_label(std::move(label))
    {
        this->set_focusable(true);  // Can receive focus
    }

    // Signal emitted when clicked
    signal<> clicked;

protected:
    void do_render(render_context<Backend>& ctx) const override {
        // ctx.style() provides state-dependent styling!
        auto& style = ctx.style();

        // Colors change based on state automatically
        const auto& bg = style.background_color;
        const auto& fg = style.foreground_color;
        const auto& font = style.font;

        // Get absolute position
        const auto& pos = ctx.position();
        const auto& bounds = this->bounds();

        // Draw background (color depends on state)
        typename Backend::rect_type bg_rect;
        rect_utils::set_bounds(bg_rect,
            point_utils::get_x(pos),
            point_utils::get_y(pos),
            rect_utils::get_width(bounds),
            rect_utils::get_height(bounds));
        ctx.fill_rect(bg_rect);  // Uses state-dependent bg color

        // Draw text
        ctx.draw_text(m_label, pos, font, fg);
    }

    // Handle mouse events
    bool on_mouse_button(int button, bool pressed,
                        int x, int y) override {
        if (button == 0 && pressed) {  // Left click
            clicked.emit();
            return true;  // Event handled
        }
        return base::on_mouse_button(button, pressed, x, y);
    }

    // Theme integration - define state colors
    themed_property<color_type> get_theme_background_color(
        visual_state state) const override
    {
        auto* theme = this->current_theme();
        if (!theme) return {};

        switch (state) {
            case visual_state::normal:
                return theme->button.background_color;
            case visual_state::highlighted:
                return theme->button.highlighted_background;
            case visual_state::disabled:
                return theme->button.disabled_background;
        }
        return {};
    }

private:
    std::string m_label;
};
```

### State Management

`stateful_widget` automatically manages three visual states:

- **`visual_state::normal`** - Default appearance
- **`visual_state::highlighted`** - Mouse hover or keyboard focus
- **`visual_state::disabled`** - Widget is disabled

**State changes handled automatically:**
- Mouse enter/leave → highlighted/normal
- Focus gain/loss → highlighted/normal
- `set_enabled(false)` → disabled

**Rendering gets state-dependent style:**
```cpp
void do_render(render_context<Backend>& ctx) const override {
    auto& style = ctx.style();  // Automatically picks colors for current state!

    ctx.fill_rect(rect, style.background_color);  // State-dependent
    ctx.draw_text(text, pos, style.font, style.foreground_color);
}
```

---

## Container Widgets

For widgets that contain children and need borders, use `widget_container`:

```cpp
#include <onyxui/widgets/core/widget_container.hh>
#include <onyxui/layout/linear_layout.hh>

template<UIBackend Backend>
class custom_panel : public widget_container<Backend> {
public:
    using base = widget_container<Backend>;

    explicit custom_panel(ui_element<Backend>* parent = nullptr)
        : base(parent)
    {
        // Set up vertical layout
        this->set_layout_strategy(
            std::make_unique<linear_layout<Backend>>(
                direction::vertical,
                5,  // 5 logical unit spacing
                horizontal_alignment::left,
                vertical_alignment::top
            )
        );

        // Enable border (rendered automatically by widget_container)
        this->m_has_border = true;
    }

    // Add custom rendering (border is drawn by base class)
    void do_render(render_context<Backend>& ctx) const override {
        // Draw border first (base class)
        base::do_render(ctx);

        // Add custom decoration (e.g., title bar)
        // Remember: use ctx.position() for absolute coords!
        const auto& pos = ctx.position();
        const auto& bounds = this->bounds();

        // Draw custom title bar at top
        typename Backend::rect_type title_rect;
        rect_utils::set_bounds(title_rect,
            point_utils::get_x(pos),
            point_utils::get_y(pos),
            rect_utils::get_width(bounds),
            20);  // 20 logical unit tall title bar

        typename Backend::color_type title_color{0, 100, 200};
        // For custom title bar color, use direct renderer access
        if (auto* renderer = ctx.renderer()) {
            renderer->fill_rect(title_rect, title_color);
        }
    }

    // Override content area to account for title bar
    rect_type get_content_area() const noexcept override {
        auto content = base::get_content_area();  // Gets border/padding area

        // Shrink by title bar height
        int const y = rect_utils::get_y(content) + 20;  // +20 for title
        int const h = std::max(0, rect_utils::get_height(content) - 20);
        rect_utils::set_y(content, y);
        rect_utils::set_height(content, h);

        return content;
    }
};
```

### `widget_container` Features

**Automatic border rendering:**
```cpp
this->m_has_border = true;  // Border drawn automatically
```

**Content area management:**
- Accounts for margin, border, padding automatically
- Override `get_content_area()` for custom content regions

**Layout strategy integration:**
```cpp
this->set_layout_strategy(std::make_unique<linear_layout<Backend>>(...));
// Children arranged by strategy automatically
```

---

## Render Context Pattern

The **render context** is a visitor that handles both **measurement** and **rendering** in a single `do_render()` method.

**Key Principle:** Widgets should NOT distinguish between measurement and rendering phases. The same drawing code works for both!

### Context Methods

```cpp
void do_render(render_context<Backend>& ctx) const override {
    // Query context
    const auto& pos = ctx.position();     // Absolute screen position
    const auto& avail = ctx.available_size(); // Size from parent layout
    auto& style = ctx.style();            // Resolved style (state-dependent)
    auto* theme = ctx.theme();            // Current theme (for rare properties)

    // Drawing operations work transparently for both measurement and rendering
    ctx.draw_text(text, pos, font, color);      // Returns text size
    ctx.fill_rect(rect);                        // Fills with style background
    ctx.draw_rect(rect, box_style);             // Draws border
    ctx.draw_shadow(rect, offset_x, offset_y);  // Draws shadow
}
```

### Two Widget Patterns

#### Pattern 1: Fixed Content Size (like label)

**Use when:** Widget size is determined purely by its content.

```cpp
void do_render(render_context<Backend>& ctx) const override {
    const auto& pos = ctx.position();
    int x = point_utils::get_x(pos);
    int y = point_utils::get_y(pos);

    // Just draw content at natural size
    // Context figures out the size automatically!
    ctx.draw_text(m_text, {x, y}, ctx.style().font, ctx.style().foreground_color);
}
```

**How it works:**
- During measurement: `ctx.draw_text()` returns size, measure_context tracks bounds
- During rendering: `ctx.draw_text()` draws text, same size calculation
- No branching needed!

#### Pattern 2: Expandable Widget (like button)

**Use when:** Widget can expand to fill parent's available space.

```cpp
void do_render(render_context<Backend>& ctx) const override {
    // Measure text
    auto text_size = renderer_type::measure_text(m_text, font);
    int text_width = size_utils::get_width(text_size);
    int text_height = size_utils::get_height(text_size);

    // Calculate natural size
    int natural_width = text_width + padding*2 + border*2;
    int natural_height = text_height + padding*2 + border*2;

    // Get final dimensions using context helpers
    // During measurement: returns (natural_width, natural_height)
    // During rendering: returns parent's assigned size
    auto [final_width, final_height] = ctx.get_final_dims(natural_width, natural_height);

    // Draw at calculated size
    rect_type button_rect{pos.x, pos.y, final_width, final_height};
    ctx.draw_rect(button_rect, ctx.style().box_style);
    ctx.draw_text(m_text, text_pos, font, color);
}
```

**How it works:**
- During measurement: `available_size()` is {0,0} → `get_final_dims()` returns natural size
- During rendering: `available_size()` is parent's assigned size → `get_final_dims()` returns assigned size
- Single code path, no explicit phase checking!

**Convenience helpers available:**
```cpp
// For width only
int final_width = ctx.get_final_width(natural_width);

// For height only
int final_height = ctx.get_final_height(natural_height);

// For both dimensions (preferred)
auto [final_width, final_height] = ctx.get_final_dims(natural_width, natural_height);
```

### Key Differences: available_size() vs this->bounds()

```cpp
// ❌ WRONG - Don't use bounds() for size during rendering
auto bounds = this->bounds();  // Relative coordinates, not useful for drawing!
int width = rect_utils::get_width(bounds);

// ✅ CORRECT - Use context helpers
auto [final_width, final_height] = ctx.get_final_dims(natural_width, natural_height);

// ✅ ALSO CORRECT - Use available_size() directly
auto avail = ctx.available_size();
int width = size_utils::get_width(avail);  // 0 during measurement, actual size during rendering
int final_width = (width > 0) ? width : natural_width;
```

**Rule:** Use `ctx.get_final_dims()` helpers or `ctx.available_size()` to distinguish between natural size and parent-assigned size, not `is_measuring()`!

---

## Theming Integration

Widgets integrate with the theme system by overriding theme accessors:

```cpp
template<UIBackend Backend>
class themed_widget : public widget<Backend> {
protected:
    // Background color from theme
    themed_property<color_type> get_theme_background_color(
        visual_state state) const override
    {
        auto* theme = this->current_theme();
        if (!theme) return {};  // No theme

        // Return state-dependent color
        switch (state) {
            case visual_state::normal:
                return theme->widget.background_color;
            case visual_state::highlighted:
                return theme->widget.highlighted_background;
            case visual_state::disabled:
                return theme->widget.disabled_background;
        }
        return {};
    }

    // Foreground color from theme
    themed_property<color_type> get_theme_foreground_color(
        visual_state state) const override
    {
        auto* theme = this->current_theme();
        if (!theme) return {};

        return theme->widget.foreground_color;
    }

    // Font from theme
    themed_property<font_type> get_theme_font(
        visual_state state) const override
    {
        auto* theme = this->current_theme();
        if (!theme) return {};

        return theme->widget.font;
    }
};
```

### Resolved Style

During rendering, `ctx.style()` provides **resolved** theme properties:

```cpp
void do_render(render_context<Backend>& ctx) const override {
    auto& style = ctx.style();

    // These are already resolved based on:
    // 1. Widget's state (normal/highlighted/disabled)
    // 2. Theme values
    // 3. Explicit overrides (if set)
    const auto& bg = style.background_color;
    const auto& fg = style.foreground_color;
    const auto& font = style.font;

    // Just use them!
    ctx.fill_rect(rect, bg);
    ctx.draw_text(m_text, pos, font, fg);
}
```

---

## Event Handling

Widgets can handle keyboard and mouse events:

```cpp
template<UIBackend Backend>
class interactive_widget : public widget<Backend> {
protected:
    // Mouse events
    bool on_mouse_button(int button, bool pressed,
                        int x, int y) override
    {
        if (button == 0 && pressed) {  // Left click
            std::cout << "Clicked at (" << x << "," << y << ")\n";
            return true;  // Event handled (stops propagation)
        }
        return false;  // Event not handled (propagates to parent)
    }

    bool on_mouse_move(int x, int y) override {
        // Track mouse position
        m_mouse_pos = {x, y};
        this->invalidate_visual();  // Repaint
        return false;
    }

    bool on_mouse_wheel(int delta_x, int delta_y) override {
        // Handle scroll
        return false;
    }

    // Keyboard events
    bool on_key_press(int key_code) override {
        if (key_code == KEY_ENTER) {
            // Handle Enter key
            return true;
        }
        return false;
    }

    bool on_key_release(int key_code) override {
        return false;
    }

    // Focus events
    void on_focus_gained() override {
        std::cout << "Widget gained focus\n";
        this->invalidate_visual();
    }

    void on_focus_lost() override {
        std::cout << "Widget lost focus\n";
        this->invalidate_visual();
    }

private:
    point_type m_mouse_pos{0, 0};
};
```

### Event Return Values

- **`true`** = Event handled, stop propagation
- **`false`** = Event not handled, propagate to parent

### Making Widget Focusable

```cpp
custom_widget() {
    this->set_focusable(true);  // Widget can receive keyboard focus
}
```

---

## Best Practices

### 1. Use Relative Coordinates Correctly

✅ **CORRECT:**
```cpp
void do_render(render_context<Backend>& ctx) const override {
    const auto& pos = ctx.position();  // Absolute
    int x = point_utils::get_x(pos);
    ctx.draw_text(m_text, {x, y}, font, color);
}
```

❌ **WRONG:**
```cpp
void do_render(render_context<Backend>& ctx) const override {
    const auto& bounds = this->bounds();  // Relative!
    int x = rect_utils::get_x(bounds);    // Wrong position!
    ctx.draw_text(m_text, {x, y}, font, color);
}
```

### 2. Invalidation Strategy

Choose the right invalidation:

```cpp
// Text changed → size might change
void set_text(std::string text) {
    m_text = std::move(text);
    this->invalidate_measure();  // Re-measure AND re-render
}

// Color changed → only visual change
void set_color(color_type color) {
    m_color = color;
    this->invalidate_visual();  // Only re-render
}

// Layout changed → re-arrange children
void set_spacing(int spacing) {
    m_spacing = spacing;
    this->invalidate_arrange();  // Re-arrange AND re-render
}
```

### 3. Use Render Context Methods

Prefer context methods over direct renderer access:

✅ **PREFERRED:**
```cpp
ctx.draw_text(text, pos, font, color);  // Works in measure + render
ctx.fill_rect(rect);  // Uses resolved style
```

❌ **AVOID:**
```cpp
auto& renderer = ctx.renderer();
renderer.draw_text(text, pos, font, color);  // Doesn't track bounds during measure
```

### 4. Memory Management

Use smart pointers correctly:

```cpp
// Adding children - transfer ownership with std::move
auto child = std::make_unique<label<Backend>>("Text");
child->set_some_property(...);  // Configure before moving
panel->add_child(std::move(child));  // Ownership transferred
// child is now nullptr - don't use!

// Storing raw pointers to children - safe as long as child alive
auto* label_ptr = panel->emplace_child<label>("Text");
label_ptr->set_text("New text");  // OK - panel owns label
```

### 5. Thread Safety

OnyxUI is **not thread-safe**. All UI operations must happen on the main thread:

```cpp
// ❌ WRONG - UI updates from background thread
std::thread worker([&widget]() {
    widget->set_text("Done");  // CRASH! Not thread-safe!
});

// ✅ CORRECT - Signal from background, update on main thread
signal<std::string> data_ready;
data_ready.connect([&widget](const std::string& result) {
    widget->set_text(result);  // OK - on main thread
});

std::thread worker([&data_ready]() {
    auto result = do_work();
    data_ready.emit(result);  // Signal from background thread
});
```

---

## Common Pitfalls

### Pitfall 1: Using `bounds()` for Rendering

**Problem:**
```cpp
void do_render(render_context<Backend>& ctx) const override {
    auto bounds = this->bounds();  // ❌ RELATIVE coordinates!
    int x = rect_utils::get_x(bounds);
    ctx.draw_text(m_text, {x, y}, font, color);  // Wrong position!
}
```

**Solution:**
```cpp
void do_render(render_context<Backend>& ctx) const override {
    const auto& pos = ctx.position();  // ✅ ABSOLUTE coordinates
    int x = point_utils::get_x(pos);
    ctx.draw_text(m_text, {x, y}, font, color);
}
```

### Pitfall 2: Forgetting to Invalidate

**Problem:**
```cpp
void set_text(std::string text) {
    m_text = std::move(text);
    // ❌ Forgot to invalidate - UI won't update!
}
```

**Solution:**
```cpp
void set_text(std::string text) {
    m_text = std::move(text);
    this->invalidate_measure();  // ✅ Trigger re-layout
}
```

### Pitfall 3: Using `is_measuring()` to Branch Logic

**Problem:**
```cpp
void do_render(render_context<Backend>& ctx) const override {
    if (ctx.is_measuring()) {
        // ❌ ANTIPATTERN - Branching between measurement and rendering!
        calculate_size();
    } else {
        draw_content();
    }
}
```

**Solution:**
```cpp
void do_render(render_context<Backend>& ctx) const override {
    // ✅ CORRECT - Same code for both phases
    // Use available_size() if you need to distinguish natural vs assigned size
    auto avail = ctx.available_size();
    int width = (size_utils::get_width(avail) > 0)
        ? size_utils::get_width(avail)  // Assigned size
        : calculate_natural_width();      // Natural size

    ctx.draw_rect({pos.x, pos.y, width, height}, style);
}
```

**Why:** The render context pattern means the SAME code should handle both measurement and rendering. Use `available_size()` to distinguish between natural size (measurement) and assigned size (rendering), not `is_measuring()`.

**Exception:** Only check `renderer()` for null when you need direct renderer access for custom operations not covered by context methods.

### Pitfall 4: Direct Renderer Access Without Checking

**Problem:**
```cpp
void do_render(render_context<Backend>& ctx) const override {
    auto& renderer = ctx.renderer();
    renderer.draw_text(...);  // ❌ Crashes during measurement (renderer is null)!
}
```

**Solution:**
```cpp
void do_render(render_context<Backend>& ctx) const override {
    // ✅ Use context methods (work in both phases)
    ctx.draw_text(...);

    // ✅ Or check renderer for custom operations
    if (auto* renderer = ctx.renderer()) {
        renderer->draw_custom(...);  // Only during rendering
    }
}
```

### Pitfall 5: Modifying State in `do_render()`

**Problem:**
```cpp
void do_render(render_context<Backend>& ctx) const override {
    m_counter++;  // ❌ ERROR! Method is const!
    m_dirty = false;  // ❌ Violates const correctness
}
```

**Solution:**
```cpp
// Use mutable for cache data
mutable std::optional<size_type> m_cached_size;

void do_render(render_context<Backend>& ctx) const override {
    if (!m_cached_size) {
        m_cached_size = measure_text(...);  // ✅ OK - mutable cache
    }
}
```

### Pitfall 5: Not Calling Base Class Methods

**Problem:**
```cpp
class my_button : public stateful_widget<Backend> {
    bool on_mouse_button(int button, bool pressed, int x, int y) override {
        if (button == 0 && pressed) {
            clicked.emit();
            return true;
        }
        // ❌ Forgot to call base class - state management broken!
    }
};
```

**Solution:**
```cpp
bool on_mouse_button(int button, bool pressed, int x, int y) override {
    if (button == 0 && pressed) {
        clicked.emit();
        return true;
    }
    return base::on_mouse_button(button, pressed, x, y);  // ✅ Call base
}
```

---

## Complete Example: Progress Bar Widget

Putting it all together, here's a complete custom widget:

```cpp
// progress_bar.hh
#pragma once

#include <onyxui/widgets/core/widget.hh>
#include <algorithm>

namespace onyxui {

template<UIBackend Backend>
class progress_bar : public widget<Backend> {
public:
    using base = widget<Backend>;
    using color_type = typename Backend::color_type;

    explicit progress_bar(ui_element<Backend>* parent = nullptr)
        : base(parent)
        , m_value(0.0)
        , m_fill_color{0, 200, 0}  // Green
    {}

    // Value between 0.0 and 1.0
    void set_value(double value) {
        value = std::clamp(value, 0.0, 1.0);
        if (m_value != value) {
            m_value = value;
            this->invalidate_visual();  // Only visual change
        }
    }

    double value() const { return m_value; }

    void set_fill_color(color_type color) {
        m_fill_color = color;
        this->invalidate_visual();
    }

protected:
    void do_render(render_context<Backend>& ctx) const override {
        // Get absolute position for rendering
        const auto& pos = ctx.position();
        const auto& bounds = this->bounds();

        int x = point_utils::get_x(pos);
        int y = point_utils::get_y(pos);
        int w = rect_utils::get_width(bounds);
        int h = rect_utils::get_height(bounds);

        // Draw background (unfilled portion)
        typename Backend::rect_type bg_rect;
        rect_utils::set_bounds(bg_rect, x, y, w, h);
        ctx.fill_rect(bg_rect);  // Uses theme background color

        // Draw border
        auto& style = ctx.style();
        ctx.draw_rect(bg_rect, style.box_style);

        // Draw filled portion with custom color
        if (m_value > 0.0) {
            int fill_width = static_cast<int>(w * m_value);
            typename Backend::rect_type fill_rect;
            constexpr int BORDER = 1;
            rect_utils::set_bounds(fill_rect,
                x + BORDER,
                y + BORDER,
                fill_width - 2*BORDER,
                h - 2*BORDER);

            // Use direct renderer for custom fill color
            if (auto* renderer = ctx.renderer()) {
                renderer->fill_rect(fill_rect, m_fill_color);
            }
        }

        // Draw percentage text
        char text[16];
        snprintf(text, sizeof(text), "%.0f%%", m_value * 100.0);

        typename Backend::renderer_type::font font{};
        auto text_size = Backend::renderer_type::measure_text(text, font);
        int text_w = size_utils::get_width(text_size);
        int text_h = size_utils::get_height(text_size);

        // Center text
        int text_x = x + (w - text_w) / 2;
        int text_y = y + (h - text_h) / 2;

        typename Backend::point_type text_pos{text_x, text_y};
        ctx.draw_text(text, text_pos, font, style.foreground_color);
    }

    // Provide minimum size
    logical_size do_measure(logical_unit available_width,
                           logical_unit available_height) override {
        // Minimum 100x20 logical units
        return {
            logical_unit(std::min(100.0, available_width.value)),
            logical_unit(std::min(20.0, available_height.value))
        };
    }

private:
    double m_value;  // 0.0 to 1.0
    color_type m_fill_color;
};

} // namespace onyxui
```

### Usage:

```cpp
// Create progress bar
auto progress = std::make_unique<progress_bar<Backend>>();
progress->set_value(0.75);  // 75%
progress->set_fill_color({0, 255, 0});  // Green

// Add to layout
vbox->add_child(std::move(progress));

// Update value
progress_ptr->set_value(0.85);  // Updates automatically!
```

---

## Next Steps

- **Study Existing Widgets**: Look at `label.hh`, `button.hh`, `panel.hh` for examples
- **Read Architecture Guide**: `docs/CLAUDE/ARCHITECTURE.md` for deeper concepts
- **Write Tests**: `unittest/widgets/test_*.cc` for testing patterns
- **Check Theming Guide**: `docs/CLAUDE/THEMING.md` for theme integration

**Happy widget coding!** 🎨
