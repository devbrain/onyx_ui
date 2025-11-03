# OnyxUI Architecture Guide

This document provides deep technical details about the OnyxUI framework architecture.

## Table of Contents

- [Backend Pattern](#backend-pattern)
- [Relative Coordinate System](#relative-coordinate-system)
- [Core Concepts](#core-concepts)
- [Two-Pass Layout Algorithm](#two-pass-layout-algorithm)
- [Render Context Pattern (Visitor)](#render-context-pattern-visitor-pattern)
- [Smart Invalidation](#smart-invalidation)
- [Event System](#event-system)
- [Layout Strategies](#layout-strategies)
- [Size Policies](#size-policies)
- [Memory Management](#memory-management)
- [Background Rendering](#background-rendering)

---

## Backend Pattern

All classes use a unified Backend template parameter:

```cpp
template<UIBackend Backend>
class ui_element : public event_target<Backend>, public themeable<Backend> {
    using rect_type = Backend::rect_type;
    using size_type = Backend::size_type;
    using color_type = Backend::color_type;
    using renderer_type = Backend::renderer_type;
    // ...
};
```

The Backend must satisfy the `UIBackend` concept and provide:
- `rect_type` (RectLike)
- `size_type` (SizeLike)
- `point_type` (PointLike)
- `color_type` (ColorLike)
- `event_type` (EventLike)
- `renderer_type` (RenderLike with box_style, font, icon_style)
- `static void register_themes(theme_registry<Backend>&)` - Called automatically on first context creation

See `include/onyxui/concepts/backend.hh` for the full concept definition.

### Automatic Theme Registration

Backends automatically register their built-in themes when the first `ui_context` is created:

```cpp
// Backend provides register_themes() method
struct conio_backend {
    static void register_themes(theme_registry<conio_backend>& registry) {
        // Register themes in order (first is default)
        registry.register_theme(create_norton_blue());     // Default
        registry.register_theme(create_borland_turbo());
        registry.register_theme(create_midnight_commander());
        registry.register_theme(create_dos_edit());
    }
};

// Application code - NO manual registration needed!
scoped_ui_context<conio_backend> ctx;
// Themes already registered automatically

// Access themes immediately
auto* theme = ctx.themes().get_theme("Norton Blue");  // Available!
ctx.themes().apply_theme("Borland Turbo");
```

**Default Theme Convention:**
- The **first theme registered** is considered the default theme
- Backends should register their primary/recommended theme first
- No explicit "default" flag needed - order matters

**When register_themes() is called:**
- Automatically on first `scoped_ui_context` creation
- Only once per application (shared singleton)
- Before any user code accesses the theme registry

**Benefits:**
- No manual registration code needed in applications
- Consistent default theme across all uses of a backend
- Themes available immediately after context creation
- Backend authors control the theme selection and defaults

---

## Relative Coordinate System

**Since:** November 2025

OnyxUI uses a **relative coordinate system** where children store bounds relative to their parent's content area, not absolute screen positions.

### Overview

```cpp
// Widget bounds are RELATIVE to parent's content area
auto child_bounds = child->bounds();  // {x: 0, y: 0, w: 100, h: 50}
// Child is at (0,0) relative to parent's content area

// During rendering, context provides ABSOLUTE screen position
void do_render(render_context<Backend>& ctx) const override {
    const auto& pos = ctx.position();     // Absolute screen coords
    const auto& bounds = this->bounds();   // Relative to parent

    // For drawing, use absolute position from context
    ctx.draw_text("Hello", pos, font, color);
}
```

### Why Relative Coordinates?

**Problems with absolute coordinates:**
- Repositioning a parent requires updating all descendant coordinates
- Complex hit testing and clipping logic
- Coordinate space confusion in scrollable/nested widgets
- Bugs with offset calculations in menus and containers

**Benefits of relative coordinates:**
✅ **Simplified Layout** - Children don't need absolute screen position
✅ **Efficient Repositioning** - Moving parent automatically moves all children
✅ **Clean Architecture** - Clear separation: relative storage, absolute rendering
✅ **Correct Clipping** - Clipping rects calculated in absolute space
✅ **Accurate Hit Testing** - Coordinate conversion at each level

### How It Works

#### 1. Storage: Relative Bounds

Children store bounds relative to parent's content area (0,0 origin):

```cpp
// After arrange(), child bounds are RELATIVE
panel.arrange({10, 10, 200, 200});    // Parent at screen (10,10)
child->bounds();  // {0, 0, 50, 50}  <- Relative to parent's content area!
```

#### 2. Rendering: Absolute Coordinates

During render traversal, offsets accumulate to produce absolute screen positions:

```cpp
// Root starts at (0, 0)
root->render(renderer, theme);
  // Child receives absolute position through context
  -> child->do_render(ctx);  // ctx.position() = (10, 10) absolute
     -> grandchild->do_render(ctx);  // ctx.position() = (20, 30) absolute
```

The `render_context` provides **absolute screen coordinates** via `ctx.position()`.

#### 3. Hit Testing: Coordinate Conversion

Hit testing converts absolute screen coords to relative at each level:

```cpp
// User clicks at absolute screen position (45, 35)
auto* hit = root->hit_test(45, 35);

// hit_test() implementation:
// 1. Check if point is in this element's bounds (absolute check at root)
// 2. Get content_area offset (e.g., {10, 10, ...} for border/padding)
// 3. Convert to child coordinates: child_coords = (45-10, 35-10) = (35, 25)
// 4. Recursively call child->hit_test(35, 25)
```

#### 4. Dirty Regions: Relative→Absolute Conversion

Dirty regions are tracked in absolute coordinates:

```cpp
// Child marks itself dirty
child->mark_dirty();

// mark_dirty() walks up parent chain:
// 1. Start with relative bounds: {0, 0, 50, 50}
// 2. Add parent's content area offset: {10, 10}
// 3. Add grandparent's offset: {0, 0}
// 4. Result: absolute bounds {10, 10, 50, 50}
// 5. Propagate absolute bounds to root
```

### Content Area

The **content area** is the region inside margins, padding, and borders where children are positioned:

```cpp
// Element with padding and border
element.set_padding({5, 5, 5, 5});
element.set_has_border(true);  // +1 pixel border

// Content area is RELATIVE to element's bounds
auto content_area = element.get_content_area();
// {x: 6, y: 6, w: width-12, h: height-12}
//  ^^^
//  margin(0) + border(1) + padding(5) = 6

// Children positioned relative to content area
child->arrange({0, 0, 100, 50});  // At content area origin
```

### Layout Strategy Integration

All layout strategies use relative positioning:

```cpp
// linear_layout.hh - vertical stacking
int current_y = 0;  // Start at content area origin (relative)
for (auto& child : children) {
    child->arrange({0, current_y, child_width, child_height});
    current_y += child_height + spacing;
}
```

### Widget Rendering Pattern

**Correct pattern** for custom widgets:

```cpp
void do_render(render_context<Backend>& ctx) const override {
    // Get absolute screen position from context
    const auto& pos = ctx.position();
    const auto& bounds = this->bounds();  // Relative

    // Use absolute position for drawing
    int x = point_utils::get_x(pos);
    int y = point_utils::get_y(pos);

    ctx.draw_text(m_text, {x, y}, font, color);

    // For rects/borders, reconstruct absolute bounds
    rect_type abs_bounds;
    rect_utils::set_bounds(abs_bounds, x, y,
        rect_utils::get_width(bounds),
        rect_utils::get_height(bounds));
    ctx.draw_rect(abs_bounds, box_style);
}
```

**Common mistake** (will cause offset bugs):

```cpp
void do_render(render_context<Backend>& ctx) const override {
    const auto& bounds = this->bounds();  // WRONG! Relative coords

    // This will render at wrong position if widget has parent offset
    int x = rect_utils::get_x(bounds);  // ❌ Relative, not absolute!
    ctx.draw_text(m_text, {x, y}, font, color);
}
```

### Testing

Tests verify the coordinate system works correctly:

```cpp
// unittest/core/test_relative_coordinates.cc
TEST_CASE("Children have relative bounds, not absolute") {
    panel root;
    root.set_padding({10, 10, 0, 0});
    auto* child = root.emplace_child<label>("Text");

    root.measure(200, 200);
    root.arrange({0, 0, 200, 200});

    // Child is at RELATIVE (0,0), not absolute (10,10)
    auto bounds = child->bounds();
    CHECK(rect_utils::get_x(bounds) == 0);
    CHECK(rect_utils::get_y(bounds) == 0);

    // But hit testing at ABSOLUTE (15, 15) finds the child
    auto* hit = root.hit_test(15, 15);
    CHECK(hit == child);  // Coordinate conversion works!
}
```

### Migration from Old System

**Before (absolute coordinates):**
```cpp
// Children stored absolute screen positions
child->bounds();  // {10, 10, 50, 50} - absolute!
```

**After (relative coordinates):**
```cpp
// Children store relative positions
child->bounds();  // {0, 0, 50, 50} - relative!
```

**Breaking change:** Internal only. No API changes for application code.

**See also:**
- `docs/RELATIVE_COORDINATES_PLAN.md` - Detailed implementation plan
- `docs/CLAUDE/CHANGELOG.md` - Migration notes
- `unittest/core/test_relative_coordinates.cc` - Comprehensive tests

---

## Core Concepts

The library uses C++20 concepts for type flexibility:

- **RectLike** - `{x, y, w, h}` or `{x, y, width, height}`
- **SizeLike** - `{w, h}` or `{width, height}`
- **PointLike** - `{x, y}`
- **ColorLike** - Any color representation
- **EventLike** - Keyboard/mouse events
- **RenderLike** - Drawing operations

Utility namespaces provide generic operations:
- `rect_utils::get_x()`, `rect_utils::contains()`, etc.
- `size_utils::get_width()`, `size_utils::set_size()`, etc.

---

## Two-Pass Layout Algorithm

1. **Measure Pass** (bottom-up):
   - Each element calculates desired size
   - Results cached until invalidation
   - `measure(available_width, available_height) -> size_type`

2. **Arrange Pass** (top-down):
   - Element receives final bounds from parent
   - Positions children within content area
   - `arrange(final_bounds)`

---

## Render Context Pattern (Visitor Pattern)

The framework uses the **visitor pattern** to unify measurement and rendering through a single `do_render()` method.

### Architecture

- **Abstract base**: `render_context<Backend>` defines drawing operations
- **draw_context**: Concrete visitor that renders to a backend renderer
- **measure_context**: Concrete visitor that tracks bounding boxes without rendering

### Unified Interface

```cpp
template<UIBackend Backend>
class my_widget : public widget<Backend> {
    void do_render(render_context<Backend>& ctx) const override {
        // Same code handles both measurement AND rendering!
        auto text_size = ctx.draw_text(m_text, position, font, color);
        ctx.draw_rect(bounds, box_style);
        ctx.draw_icon(icon, icon_position);

        // Branch if needed (rare)
        if (ctx.is_rendering()) {
            // Render-only code
        }
    }
};
```

### Benefits

1. **Single Source of Truth**: Measurement and rendering can never get out of sync
2. **Code Reduction**: ~50-70% less code - no separate `get_content_size()` needed
3. **Maintainability**: Changes to rendering automatically update measurement
4. **Type Safety**: Compile-time guarantee of correct visitor usage

### Automatic Measurement

The base `widget` class provides automatic content sizing via `measure_context`:

```cpp
// Base widget implementation (you don't need to write this!)
size_type get_content_size() const override {
    measure_context<Backend> ctx;
    this->do_render(ctx);  // Reuse rendering code for measurement
    return ctx.get_size();
}
```

### Drawing Operations

Both contexts support the same operations:
- `draw_text(text, position, font, color) -> size_type`
- `draw_rect(bounds, box_style)`
- `draw_line(from, to, color, width)`
- `draw_icon(icon, position) -> size_type`

### Context Queries

- `ctx.is_measuring()` - Returns `true` for `measure_context`
- `ctx.is_rendering()` - Returns `true` for `draw_context`
- `ctx.renderer()` - Returns renderer pointer (nullptr for `measure_context`)

### Renderer Methods

Renderers provide static methods for measurement without instantiation:
- `Renderer::measure_text(text, font) -> size_type` - Get text dimensions
- `Renderer::get_icon_size(icon) -> size_type` - Get icon dimensions (backend-specific)
- `Renderer::get_border_thickness(box_style) -> int` - Get border width

**See also:**
- `include/onyxui/core/rendering/render_context.hh` - Visitor pattern base
- `include/onyxui/core/rendering/measure_context.hh` - Measurement implementation
- `include/onyxui/core/rendering/draw_context.hh` - Rendering implementation

---

## Smart Invalidation

- `invalidate_measure()` - Propagates **upward** (parents need remeasurement)
- `invalidate_arrange()` - Propagates **downward** (children need repositioning)

Layout states: `valid`, `dirty` - prevent redundant invalidation.

---

## Event System

Two complementary systems:

### 1. event_target (Observer pattern)

```cpp
class my_widget : public ui_element<Backend> {
    void on_mouse_down(const event_type& e) override {
        // Handle mouse clicks
    }
};
```

### 2. Signal/Slot (Publish-Subscribe)

```cpp
signal<int, std::string> data_changed;

// Connect handlers
data_changed.connect([](int id, const std::string& name) {
    std::cout << "Data " << id << " changed to " << name << "\n";
});

// Emit signals
data_changed.emit(42, "value");

// Scoped connections (RAII)
scoped_connection conn(data_changed, my_handler);
```

**See also:** `include/onyxui/core/signal.hh` for the full signal/slot implementation.

---

## Layout Strategies

Pluggable layout algorithms (Strategy pattern):

- **linear_layout** - Stack children horizontally/vertically
- **grid_layout** - Grid with cell spanning
- **anchor_layout** - Position at anchor points
- **absolute_layout** - Fixed coordinates

Each implements:
- `measure_children(parent, available_width, available_height) -> size`
- `arrange_children(parent, content_area)`

**See also:**
- `include/onyxui/layout/linear_layout.hh`
- `include/onyxui/layout/grid_layout.hh`
- `include/onyxui/layout/anchor_layout.hh`
- `include/onyxui/layout/absolute_layout.hh`

---

## Size Policies

Elements use `size_constraint` with policies:
- `fixed` - Exact preferred_size
- `content` - Size based on content (default)
- `expand` - Grow to fill space
- `fill_parent` - Match parent dimension
- `percentage` - Percentage of parent
- `weighted` - Proportional distribution

All policies respect `min_size` and `max_size` bounds.

**See also:** `include/onyxui/element.hh` for size constraint implementation.

---

## Memory Management

- Tree structure uses `std::unique_ptr<ui_element>` for ownership
- Parent pointers are raw non-owning pointers
- Layout strategies owned by `std::unique_ptr<layout_strategy>`
- `add_child()` takes ownership, `remove_child()` returns ownership
- Signal connections managed by `scoped_connection` (RAII)

---

## Background Rendering

The framework separates background rendering from the widget tree for architectural clarity and performance.

### Why Not a Widget?

Background rendering is a **global UI concern**, not a widget:
- Renders before the widget tree (establishes drawing surface)
- Not part of the layout hierarchy (no measure/arrange)
- Supports modes impossible for widgets (transparency for game overlays)
- Avoids performance overhead of widget tree traversal

### Architecture

```cpp
// Service managed by ui_context (not a widget!)
background_renderer<Backend>  // Per-context instance
  ├─ Accessed via ui_services<Backend>::background()
  ├─ Renders before widget tree in ui_handle::display()
  ├─ Supports dirty region optimization
  └─ Automatically syncs with theme changes via signal/slot
```

### Automatic Theme Synchronization

Background automatically synchronizes with the current theme via signal/slot pattern:

```cpp
// 1. Set theme (in application code)
ctx.themes().set_current_theme("Norton Blue");

// 2. theme_registry emits theme_changed signal
// 3. background_renderer automatically receives notification
// 4. Background updates to match theme->window_bg

// NO manual synchronization needed!
```

**How It Works:**

```cpp
// In ui_context constructor (automatic setup):
m_theme_connection = s_shared_themes->theme_changed.connect([this](const theme_type* theme) {
    m_background_renderer.on_theme_changed(theme);  // Auto-sync!
});
```

### Two Rendering Modes

1. **Opaque** - Background style is set
   - Respects dirty regions (only fills changed areas)
   - Suitable for traditional desktop applications
   - Optimized: O(dirty_regions) draw calls

2. **Transparent** - No background style
   - UI widgets overlay game/3D rendering
   - Suitable for game HUDs, in-game menus
   - Zero overhead: no rendering

### Usage Pattern

```cpp
// Option 1: Automatic sync with theme (RECOMMENDED)
ctx.themes().set_current_theme("Borland Turbo");
// Background automatically updates to theme->window_bg!

// Option 2: Manual control (if needed)
auto* bg = ui_services<Backend>::background();
bg->set_color({0, 0, 170});  // Blue background
bg->clear_style();            // Transparent mode

// Changes take effect on next display() call
ui.display();  // Background rendered first, then widgets
```

### Frame-Based Behavior

Background changes apply on the **next frame** (next `display()` call):
- No immediate redraw triggered
- Application controls when to redraw
- Consistent with event-driven architecture

### Integration with ui_handle

```cpp
void ui_handle::display() {
    auto dirty_regions = m_root->get_and_clear_dirty_regions();

    // 1. Render background FIRST (from ui_services)
    if (auto* bg = ui_services<Backend>::background()) {
        bg->render(m_renderer, viewport, dirty_regions);
    }

    // 2. Measure/arrange widget tree
    m_root->measure(viewport.w, viewport.h);
    m_root->arrange(viewport);

    // 3. Render widgets on top of background
    m_root->render(m_renderer, dirty_regions);

    // 4. Render popup layers (menus, dialogs)
    layers->render_all_layers(m_renderer, viewport);
}
```

### Per-Context Independence

Each `ui_context` has its own `background_renderer`:

```cpp
{
    scoped_ui_context<Backend> ctx1;
    auto* bg1 = ui_services<Backend>::background();
    bg1->set_color({255, 0, 0});  // Red

    {
        scoped_ui_context<Backend> ctx2;
        auto* bg2 = ui_services<Backend>::background();
        bg2->set_color({0, 255, 0});  // Green
        // ctx2 is active - green background
    }
    // ctx1 is active again - red background preserved
}
```

### Backend-Specific Styles

Each backend defines its own `background_style` type via the `RenderLike` concept:

```cpp
// conio backend (TUI)
struct background_style {
    color bg_color;
    char fill_char = ' ';  // Pattern support: '░', '▒', '▓', etc.
};

// canvas backend (testing)
struct background_style {
    uint8_t fg, bg, attrs;
    char fill_char = ' ';
};
```

### RenderLike Concept Extension

```cpp
template<typename T, typename R>
concept RenderLike = RectLike<R> && requires(T renderer,
                                              const typename T::background_style& bg) {
    typename T::background_style;  // Backend-specific type

    // Draw background (optimized clearing)
    { renderer.draw_background(rect, bg) } -> std::same_as<void>;
    { renderer.draw_background(rect, bg, regions) } -> std::same_as<void>;
};
```

**See also:**
- `include/onyxui/services/background_renderer.hh` - Implementation
- `include/onyxui/theming/theme_registry.hh` - Signal emission
- `include/onyxui/services/ui_context.hh` - Signal connection
- `include/onyxui/concepts/render_like.hh` - Background style concept
- `unittest/core/test_background_renderer.cc` - Tests (7 cases, 62 assertions)

---

## Key Reference Files

- `include/onyxui/core/element.hh` - Core layout algorithm (800+ lines, well-documented)
- `include/onyxui/core/rendering/render_context.hh` - Abstract visitor base for measurement/rendering
- `include/onyxui/core/rendering/measure_context.hh` - Concrete visitor for size measurement
- `include/onyxui/core/rendering/draw_context.hh` - Concrete visitor for rendering
- `include/onyxui/core/rendering/scoped_clip.hh` - RAII clipping guard
- `include/onyxui/layout/layout_strategy.hh` - Layout documentation (600+ lines of design docs)
- `include/onyxui/core/signal.hh` - Signal/slot pattern implementation
- `include/onyxui/actions/action.hh` - Command pattern for UI actions
- `include/onyxui/theming/themeable.hh` - CSS-style inheritance system
