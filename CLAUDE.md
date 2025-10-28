# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

**onyx_ui** is a modern C++20 header-only UI framework featuring:
- **Layout System**: Two-pass measure/arrange algorithm with smart caching
- **Widget Library**: Buttons, labels, menus, status bars, group boxes, and more
- **Theming System**: CSS-style property inheritance for visual styling
- **Event System**: Signal/slot pattern for decoupled communication
- **Hotkeys**: Global keyboard shortcut management
- **Backend Agnostic**: Works with any renderer through C++20 concepts

The framework uses a unified **Backend pattern** where a single template parameter provides all platform-specific types (rect, size, color, renderer, etc.).

## Build Commands

### Initial Configuration
```bash
# Standard build
cmake -B build

# Build with tests disabled
cmake -B build -DONYX_UI_BUILD_TEST=OFF

# Build with sanitizers
cmake -B build -DONYX_UI_ENABLE_SANITIZERS=ON

# Build with clang-tidy linter
cmake -B build -DONYX_UI_ENABLE_CLANG_TIDY=ON
```

### Building
```bash
# Build all targets
cmake --build build -j8

# Build only tests
cmake --build build --target ui_unittest -j8

# Build conio demo
cmake --build build --target conio -j8
```

### Running Tests
```bash
# Run all 653 unit tests
./build/bin/ui_unittest

# List all test cases
./build/bin/ui_unittest --list-test-cases

# Run specific test
./build/bin/ui_unittest --test-case="Signal - Basic connection and emission"

# Run tests matching pattern
./build/bin/ui_unittest --test-case-exclude="*Performance*"
```

### Code Quality
```bash
# Run with clang-tidy (configure with ENABLE_CLANG_TIDY=ON first)
cmake --build build 2>&1 | grep "warning:"

# All 653 tests should pass with zero warnings
```

## Code Architecture

### Backend Pattern (NEW)

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

### Core Concepts

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

### Two-Pass Layout Algorithm

1. **Measure Pass** (bottom-up):
   - Each element calculates desired size
   - Results cached until invalidation
   - `measure(available_width, available_height) -> size_type`

2. **Arrange Pass** (top-down):
   - Element receives final bounds from parent
   - Positions children within content area
   - `arrange(final_bounds)`

### Render Context Pattern (Visitor Pattern)

The framework uses the **visitor pattern** to unify measurement and rendering through a single `do_render()` method.

**Architecture:**

- **Abstract base**: `render_context<Backend>` defines drawing operations
- **draw_context**: Concrete visitor that renders to a backend renderer
- **measure_context**: Concrete visitor that tracks bounding boxes without rendering

**Unified Interface:**

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

**Benefits:**

1. **Single Source of Truth**: Measurement and rendering can never get out of sync
2. **Code Reduction**: ~50-70% less code - no separate `get_content_size()` needed
3. **Maintainability**: Changes to rendering automatically update measurement
4. **Type Safety**: Compile-time guarantee of correct visitor usage

**Automatic Measurement:**

The base `widget` class provides automatic content sizing via `measure_context`:

```cpp
// Base widget implementation (you don't need to write this!)
size_type get_content_size() const override {
    measure_context<Backend> ctx;
    this->do_render(ctx);  // Reuse rendering code for measurement
    return ctx.get_size();
}
```

**Drawing Operations:**

Both contexts support the same operations:
- `draw_text(text, position, font, color) -> size_type`
- `draw_rect(bounds, box_style)`
- `draw_line(from, to, color, width)`
- `draw_icon(icon, position) -> size_type`

**Context Queries:**

- `ctx.is_measuring()` - Returns `true` for `measure_context`
- `ctx.is_rendering()` - Returns `true` for `draw_context`
- `ctx.renderer()` - Returns renderer pointer (nullptr for `measure_context`)

**Renderer Methods:**

Renderers provide static methods for measurement without instantiation:
- `Renderer::measure_text(text, font) -> size_type` - Get text dimensions
- `Renderer::get_icon_size(icon) -> size_type` - Get icon dimensions (backend-specific)
- `Renderer::get_border_thickness(box_style) -> int` - Get border width

See `include/onyxui/render_context.hh`, `measure_context.hh`, and `draw_context.hh` for implementation.

### Smart Invalidation

- `invalidate_measure()` - Propagates **upward** (parents need remeasurement)
- `invalidate_arrange()` - Propagates **downward** (children need repositioning)

Layout states: `valid`, `dirty` - prevent redundant invalidation.

### Event System

Two complementary systems:

**1. event_target (Observer pattern):**
```cpp
class my_widget : public ui_element<Backend> {
    void on_mouse_down(const event_type& e) override {
        // Handle mouse clicks
    }
};
```

**2. Signal/Slot (Publish-Subscribe):**
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

See `include/onyxui/signal.hh` for the full signal/slot implementation.

### Theming System (CSS-style Inheritance)

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

**Inheritable properties:**
- Colors (background, foreground)
- Renderer styles (box_style, font, icon_style)
- Opacity (multiplicative)

See `include/onyxui/themeable.hh` for the inheritance system.

### Theme System v2.0 (NEW - October 2025)

The theme system has been refactored with three major improvements:

**1. Thread-Safe Theme Registry**

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

**2. Global Theming Architecture**

Themes are **global** and applied at the **root level only**, not on individual widgets. This simplifies the architecture and ensures consistent styling throughout the UI tree.

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

**Three safe ownership options** (use at root only):

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

**CSS Inheritance Chain** (priority order):
1. **Explicit override** on widget (`set_background_color()`)
2. **Parent's resolved style** (CSS inheritance)
3. **Global theme** (via `get_theme()` walk-up)
4. **Default value** (fallback)

**Benefits:**
- **Simplicity**: One theme for entire UI tree
- **Performance**: O(depth) style resolution with single parent cache
- **Consistency**: Uniform styling across all widgets
- **Safety**: No dangling pointer risk
- **Flexibility**: Individual widgets can still override colors via `set_background_color()`

**3. Style-Based Rendering Architecture**

The new `resolved_style` structure is the cornerstone of v2.0:

```cpp
// Style is resolved ONCE per frame via CSS inheritance
void render(renderer_type& renderer, const std::vector<rect_type>& dirty_regions) {
    auto style = this->resolve_style();  // Resolve through CSS hierarchy

    draw_context<Backend> ctx(renderer, style, dirty_regions);

    do_render(ctx);  // Widgets use pre-resolved style
}
```

**Key types:**

```cpp
// POD-like structure with all visual properties
template<UIBackend Backend>
struct resolved_style {
    color_type background_color;
    color_type foreground_color;
    color_type border_color;
    box_style_type box_style;
    font_type font;
    float opacity;
    icon_style_type icon_style;

    // Utility methods (immutable)
    [[nodiscard]] resolved_style with_opacity(float op) const noexcept;
    [[nodiscard]] resolved_style with_colors(color_type bg, color_type fg) const noexcept;
    [[nodiscard]] resolved_style with_font(font_type f) const noexcept;
};
```

**render_context carries style:**

```cpp
template<UIBackend Backend>
class render_context {
    // Access resolved style
    [[nodiscard]] const resolved_style<Backend>& style() const noexcept;

    // Convenience methods using context style
    [[nodiscard]] size_type draw_text(std::string_view text, const point_type& position);
    void draw_rect(const rect_type& bounds);
};
```

**Benefits:**
- **Performance**: CSS inheritance resolved once per frame, not per widget
- **Simplicity**: Widgets receive pre-resolved style via context
- **Consistency**: Single source of truth for visual properties
- **Immutability**: `resolved_style` is POD-like with copy semantics

**Complete resolved_style structure:**

The `resolved_style` includes optional widget-specific properties wrapped in strong-typed containers:

```cpp
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
};
```

**Theme pointer in render_context:**

For rare widget-specific properties not in `resolved_style` (like `text_align`, `line_style`), widgets access the theme via `ctx.theme()`:

```cpp
template<UIBackend Backend>
class render_context {
    // Access resolved style (common properties)
    [[nodiscard]] const resolved_style<Backend>& style() const noexcept;

    // Access theme for rare properties (nullable)
    [[nodiscard]] const ui_theme<Backend>* theme() const noexcept;
};
```

**Widget property access pattern:**

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

**Why two-tier access?**

- **Common properties** (colors, fonts, padding) → `resolved_style` → O(1) access
- **Rare properties** (text_align, line_style) → `ctx.theme()` → Minimal overhead
- **Performance**: Avoids bloating `resolved_style` with rarely-used fields
- **Type safety**: Optional properties enforce explicit default handling

**4. Stateful Widget Helper**

New base class for interactive widgets with visual states that automatically integrates with the global theme:

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

**Usage pattern:**

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

**Global theme structure:**
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

**How it works:**
1. Global theme applied at root contains `button_theme` settings
2. Button overrides `get_theme_background_color()` to select color based on state
3. `resolve_style()` calls `get_theme_background_color()` during CSS inheritance
4. Result: buttons automatically use correct state colors from global theme

**Architecture Summary:**

1. **CSS Inheritance** → Resolves to `resolved_style`
2. **resolved_style** → Passed to `render_context` (draw/measure)
3. **render_context** → Provides style to widgets during rendering
4. **Widgets** → Use pre-resolved style, no repeated lookups

**Performance characteristics:**
- **Style resolution**: O(depth) once per frame with single parent cache
- **Widget rendering**: O(1) style access via pre-resolved context
- **Thread-safe**: Reader-writer locking on registry
- **Memory**: POD-like resolved_style (~50-100 bytes)
- **Scalability**: Tested with 100+ widget trees (both wide and deep)

**See also:**
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

### Widget Library

High-level widgets built on ui_element:

**Containers:**
- `vbox` / `hbox` - Vertical/horizontal stacking
- `grid` - Grid layout
- `panel` - Generic container
- `anchor_panel` - Anchor-point positioning
- `absolute_panel` - Fixed positioning
- `group_box` - Bordered container with title

**Controls:**
- `button` - Clickable button with mnemonics
- `label` - Text display with styling
- `spacer` - Fixed-size spacing
- `spring` - Flexible spacing

**Menus:**
- `menu_bar` - Top-level menu container
- `menu` - Dropdown menu
- `menu_item` - Menu entry with actions

**Other:**
- `status_bar` - Bottom status display

All widgets support:
- Actions (trigger callbacks)
- Mnemonics (Alt+key shortcuts)
- Theming (CSS-style inheritance)
- Focus management

### Hotkey System

**Two-Layer System:**
1. **Framework Semantic Actions** (NEW) - Scheme-based keyboard layouts
2. **Application Actions** - User-defined shortcuts

#### Hotkey Schemes (Customizable Keyboard Layouts)

Users can switch between different keyboard layouts at runtime:

```cpp
// Access via ui_context (auto-configured)
scoped_ui_context<Backend> ctx;

// Current scheme is "Windows" (F10 for menu)
auto* current = ctx.hotkey_schemes().get_current_scheme();

// Switch to Norton Commander (F9 for menu)
ctx.hotkey_schemes().set_current_scheme("Norton Commander");

// Register framework semantic action handlers
ctx.hotkeys().register_semantic_action(
    hotkey_action::activate_menu_bar,
    [&menu]() { menu->activate(); }
);

// Now F10 (Windows) or F9 (Norton) will activate menu!
```

**Built-in Schemes:**
- **Windows**: F10 for menu (standard modern UI convention)
- **Norton Commander**: F9 for menu (classic DOS feel)

**Custom Schemes:**
```cpp
hotkey_scheme vim_scheme;
vim_scheme.name = "Vim-style";
vim_scheme.set_binding(hotkey_action::menu_down, parse_key_sequence("j"));
vim_scheme.set_binding(hotkey_action::menu_up, parse_key_sequence("k"));
// ... etc

ctx.hotkey_schemes().register_scheme(std::move(vim_scheme));
ctx.hotkey_schemes().set_current_scheme("Vim-style");
```

#### Application Actions

Application-defined keyboard shortcuts:

```cpp
hotkey_manager<Backend> hotkeys;

// Register hotkeys
auto save_action = std::make_shared<action<Backend>>();
save_action->set_shortcut('s', key_modifier::ctrl);
hotkeys.register_action(save_action);

// Process events
hotkeys.handle_key_event(event);  // Triggers matching actions
```

**Priority System:**
1. Framework semantic actions (from current scheme) - FIRST
2. Element-scoped application actions
3. Global application actions
4. Widget keyboard events - LAST

**Features:**
- Conflict detection with policy enforcement
- Scope management (global, element-scoped)
- Key sequence parsing ("Ctrl+Shift+A", "Alt+F4", "F10", etc.)
- Integration with action system
- Graceful fallback (no binding = mouse still works)

See `include/onyxui/hotkeys/` for implementation.

### Layout Strategies

Pluggable layout algorithms (Strategy pattern):

- **linear_layout** - Stack children horizontally/vertically
- **grid_layout** - Grid with cell spanning
- **anchor_layout** - Position at anchor points
- **absolute_layout** - Fixed coordinates

Each implements:
- `measure_children(parent, available_width, available_height) -> size`
- `arrange_children(parent, content_area)`

### Size Policies

Elements use `size_constraint` with policies:
- `fixed` - Exact preferred_size
- `content` - Size based on content (default)
- `expand` - Grow to fill space
- `fill_parent` - Match parent dimension
- `percentage` - Percentage of parent
- `weighted` - Proportional distribution

All policies respect `min_size` and `max_size` bounds.

### Memory Management

- Tree structure uses `std::unique_ptr<ui_element>` for ownership
- Parent pointers are raw non-owning pointers
- Layout strategies owned by `std::unique_ptr<layout_strategy>`
- `add_child()` takes ownership, `remove_child()` returns ownership
- Signal connections managed by `scoped_connection` (RAII)

### Background Rendering

The framework separates background rendering from the widget tree for architectural clarity and performance.

**Why Not a Widget?**

Background rendering is a **global UI concern**, not a widget:
- Renders before the widget tree (establishes drawing surface)
- Not part of the layout hierarchy (no measure/arrange)
- Supports modes impossible for widgets (transparency for game overlays)
- Avoids performance overhead of widget tree traversal

**Architecture:**

```cpp
// Service managed by ui_context (not a widget!)
background_renderer<Backend>  // Per-context instance
  ├─ Accessed via ui_services<Backend>::background()
  ├─ Renders before widget tree in ui_handle::display()
  ├─ Supports dirty region optimization
  └─ Automatically syncs with theme changes via signal/slot
```

**Automatic Theme Synchronization (NEW):**

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

**Two Rendering Modes:**

1. **Opaque** - Background style is set
   - Respects dirty regions (only fills changed areas)
   - Suitable for traditional desktop applications
   - Optimized: O(dirty_regions) draw calls

2. **Transparent** - No background style
   - UI widgets overlay game/3D rendering
   - Suitable for game HUDs, in-game menus
   - Zero overhead: no rendering

**Usage Pattern:**

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

**Frame-Based Behavior:**

Background changes apply on the **next frame** (next `display()` call):
- No immediate redraw triggered
- Application controls when to redraw
- Consistent with event-driven architecture

**Integration with ui_handle:**

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

**Per-Context Independence:**

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

**Backend-Specific Styles:**

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

**RenderLike Concept Extension:**

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

**Testing:**

Comprehensive tests in `unittest/core/test_background_renderer.cc`:
- 7 test cases, 62 assertions
- Style management (has_style, set_style, clear_style)
- Opaque/transparent rendering
- Dirty region optimization
- ui_context integration
- Copy/move semantics

**See also:**
- `include/onyxui/background_renderer.hh` - Implementation
- `include/onyxui/theme_registry.hh` - Signal emission
- `include/onyxui/ui_context.hh` - Signal connection
- `include/onyxui/concepts/render_like.hh` - Background style concept
- `include/onyxui/ui_handle.hh` - Rendering pipeline
- `examples/demo.cc` - Automatic sync demonstration

## Project Structure

```
include/onyxui/
  concepts/              # C++20 concepts
    backend.hh           # UIBackend concept
    rect_like.hh         # Rectangle concepts
    size_like.hh         # Size concepts
    color_like.hh        # Color concepts
    render_like.hh       # Renderer concepts
    event_like.hh        # Event concepts

  element.hh             # Core ui_element class
  event_target.hh        # Event handling base class
  themeable.hh           # CSS-style theming base class
  theme.hh               # Theme structure
  signal.hh              # Signal/slot implementation
  focus_manager.hh       # Focus navigation
  layout_strategy.hh     # Layout base class + enums
  render_context.hh      # Abstract visitor base for rendering/measurement
  measure_context.hh     # Concrete visitor for measurement
  draw_context.hh        # Concrete visitor for rendering

  layout/
    linear_layout.hh     # Vertical/horizontal stacking
    grid_layout.hh       # Grid with spanning
    anchor_layout.hh     # Anchor positioning
    absolute_layout.hh   # Fixed positioning

  widgets/
    widget.hh            # Base widget class
    button.hh            # Button widget
    label.hh             # Text label
    panel.hh             # Container panel
    vbox.hh / hbox.hh    # Stack layouts
    grid.hh              # Grid container
    menu*.hh             # Menu system
    group_box.hh         # Bordered group
    status_bar.hh        # Status bar
    spacer.hh / spring.hh # Spacing elements
    action.hh            # Action system
    mnemonic_parser.hh   # Mnemonic parsing

  hotkeys/
    key_sequence.hh      # Key combination parsing
    hotkey_manager.hh    # Global hotkey management

  utils/
    safe_math.hh         # Overflow-safe arithmetic

unittest/
  core/                  # Core functionality tests
    test_signal_slot.cc  # Signal/slot tests (92 test cases)
    test_rule_of_five.cc # Move semantics tests

  layout/                # Layout algorithm tests
    test_composition.cc  # Nested layout tests

  widgets/               # Widget tests (200+ test cases)
    test_button.cc
    test_label.cc
    test_menus.cc
    test_group_box.cc
    test_status_bar.cc
    # ... and more

  hotkeys/               # Hotkey system tests
    test_hotkeys.cc
    test_hotkey_manager.cc

  focus/                 # Focus management tests
    test_focus_manager.cc

backends/
  conio/                 # DOS/TUI backend example
    conio_backend.hh     # Backend implementation
    main.cc              # Demo application

.clang-tidy              # Linter configuration
```

## Development Guidelines

### Adding New Widgets

1. Inherit from `widget<Backend>` (which inherits from `ui_element`)
2. Implement `do_render(render_context<Backend>&)` for unified measurement and rendering
3. Override theme accessors if needed (`get_theme_background_color`, etc.)
4. Add to `include/onyxui/widgets/`
5. Write comprehensive tests in `unittest/widgets/`

Example:
```cpp
template<UIBackend Backend>
class my_widget : public widget<Backend> {
    using base = widget<Backend>;
    using render_context_type = render_context<Backend>;

    void do_render(render_context_type& ctx) const override {
        // Same code handles BOTH measurement and rendering!
        auto bounds = this->bounds();

        // Draw operations automatically track size for measurement
        ctx.draw_rect(bounds, m_box_style);
        auto text_size = ctx.draw_text(m_text, {x, y}, m_font, m_color);

        // Measurement happens automatically via widget<Backend>::get_content_size()
        // which calls this method with measure_context
    }

private:
    std::string m_text;
    typename Backend::renderer_type::box_style m_box_style;
    typename Backend::renderer_type::font m_font;
    typename Backend::color_type m_color;
};
```

### Working with ui_element

- Always call `measure()` before `arrange()`
- Use `invalidate_measure()` when size-affecting properties change
- Use `invalidate_arrange()` when position-affecting properties change
- Access children via `children()` getter
- Results cached in `m_last_measured_size`

### Coordinate System

- All coordinates are integers
- Origin is top-left (standard UI convention)
- Width/height must be non-negative
- **Margin**: External spacing (outside bounds)
- **Padding**: Internal spacing (inside bounds)
- **Spacing**: Gap between siblings (in layout)

### Code Quality Standards

The project enforces strict quality standards via clang-tidy:

**Critical:**
- No use-after-move
- No uninitialized variables
- Proper initialization order in constructors

**Important:**
- Mark variables `const` if never modified
- Add `[[nodiscard]]` to value-returning accessors
- Use `std::uint8_t` for small enums (not `int`)
- Suppress recursion warnings on tree algorithms with comments

**Style:**
- lower_case for classes/functions/variables
- UPPER_CASE for constexpr constants
- m_ prefix for private members
- No redundant access specifiers

See `.clang-tidy` for full configuration.

### Testing Strategy

The project uses **doctest** (fetched via CMake FetchContent).

**Test organization:**
- 736 test cases across 27 test files
- 4660 assertions total
- All tests must pass with zero warnings

**Writing tests for widgets (requires theme):**

Widgets need a ui_context with registered themes. Use the `ui_context_fixture` helper:

```cpp
#include <doctest/doctest.h>
#include "../utils/test_helpers.hh"
#include "../utils/test_canvas_backend.hh"

using namespace onyxui;
using namespace onyxui::testing;

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "Widget - Basic functionality") {
    SUBCASE("First scenario") {
        button<test_canvas_backend> btn("Click me");

        // The fixture provides ctx which sets up themes automatically
        auto size = btn.measure(100, 50);
        CHECK(size_utils::get_width(size) > 0);
    }

    SUBCASE("Second scenario") {
        label<test_canvas_backend> lbl("Text");
        CHECK_FALSE(lbl.is_focusable());
    }
}
```

**Writing tests without widgets (no theme needed):**

For non-widget tests, use regular TEST_CASE:

```cpp
#include <doctest/doctest.h>

TEST_CASE("Core - Basic functionality") {
    SUBCASE("First scenario") {
        // Test code
        CHECK(condition);
    }
}
```

**Best practices:**
- Use `ui_context_fixture<test_canvas_backend>` for widget tests
- Use `test_canvas_backend` (not `test_backend`) for consistency
- Test both measure and arrange phases
- Verify exact bounds, not just success
- Test edge cases (empty, zero size, max values)
- Use SUBCASEs for related scenarios
- Each test should be independent

### Common Pitfalls

1. **Backend Pattern** - Use `Backend` template parameter, not separate `TRect`, `TSize`
2. **Initialization Order** - Base classes initialize before members (use static_cast for moves)
3. **Inheritance** - Use CSS-style theming, don't repeat colors on every widget
4. **Cache Invalidation** - Any property change must invalidate layout appropriately
5. **Ownership** - Use `std::move()` when passing children to `add_child()`
6. **Signals** - Use `scoped_connection` for automatic cleanup

## Compiler Requirements

- **C++20** required (concepts, designated initializers)
- **CMake 3.15+** minimum
- **Tested compilers:**
  - GCC 10+
  - Clang 12+
  - MSVC 2019+ (with `/W4 /permissive-`)
- **Comprehensive warning flags** enabled by default

## Key Files for Reference

- `include/onyxui/element.hh` - Core layout algorithm (800+ lines, well-documented)
- `include/onyxui/render_context.hh` - Abstract visitor base for measurement/rendering
- `include/onyxui/measure_context.hh` - Concrete visitor for size measurement
- `include/onyxui/draw_context.hh` - Concrete visitor for rendering
- `include/onyxui/layout_strategy.hh` - Layout documentation (600+ lines of design docs)
- `include/onyxui/signal.hh` - Signal/slot pattern implementation
- `include/onyxui/themeable.hh` - CSS-style inheritance system
- `.clang-tidy` - Code quality standards
- `unittest/widgets/test_*.cc` - Widget usage examples

## Recent Major Changes

**Latest work:** Hotkey Scheme System (October 2025)
- **Phase 1**: Core structures (hotkey_action, hotkey_scheme, key_sequence parsing)
- **Phase 2**: Registry + built-in schemes (Windows with F10, Norton Commander with F9)
- **Phase 3**: hotkey_manager enhancement (scheme-aware semantic actions with priority)
- 859 tests passing (was 855), 6236 assertions (was 6184)
- Runtime scheme switching (user can choose Windows vs Norton Commander keyboard layout)
- Priority system: Semantic actions → Application actions → Widget keyboard events
- Zero breaking changes (backward compatible with existing hotkey code)

**Key features:**
- **Customizable Keyboard Layouts**: Users can switch between Windows (F10) and Norton Commander (F9) at runtime
- **Semantic Actions**: Framework-level actions (menu navigation, focus) separate from application hotkeys
- **Priority System**: Framework shortcuts take precedence over application shortcuts
- **Graceful Fallback**: Missing binding = mouse still works (progressive enhancement)
- **Auto-Configuration**: Built-in schemes pre-registered in ui_context, no manual setup required
- **Library-Level**: NOT backend-specific (keys are universal, unlike themes)

**Previous work:** Theme System v2.0 Refactoring (October 2025)
- **Phase 1**: Added thread-safe theme registry with failsafe logging
- **Phase 2**: Implemented three-way theme API (by-name, by-value, by-shared_ptr)
- **Phase 3**: Style-based rendering architecture with `resolved_style`
- **Phase 4**: Comprehensive testing (11 new test cases, 88 assertions)
- **Phase 5**: Documentation and polish
- **Phase 6**: Widget refactoring to enforce style-based rendering
  - Extended `resolved_style` with optional properties (padding, mnemonic_font)
  - Added theme pointer to `render_context` for rare widget-specific properties
  - Refactored all widgets (button, label, menu_item, menu_bar_item, separator, stateful_widget)
  - Eliminated all direct theme access from widgets (zero `ui_services::themes()` calls)
  - Implemented lazy mnemonic parsing for button, label, and menu_item
  - All 736 tests passing, 4660 assertions, zero breaking changes
- Removed unsafe `apply_theme(const theme_type&)` API
- All 736 tests passing (was 653), zero warnings

**Key architectural improvements:**
- **Thread Safety**: Reader-writer locking on theme registry
- **Performance**: CSS inheritance resolved once per frame, not per widget
- **Safety**: No dangling pointer risk with new ownership semantics
- **Simplicity**: Widgets receive pre-resolved style via render_context
- **Stateful Widgets**: New helper base class for interactive widgets
- **Two-Tier Access**: Common properties via `ctx.style()`, rare properties via `ctx.theme()`
- **Lazy Parsing**: Mnemonics parsed on-demand during render with caching
- **Type Safety**: Optional properties enforce explicit default handling

**Previous work:** Comprehensive layout testing implementation (2025)
- Implemented 57 new layout tests across all priority levels
- Fixed critical y=65541 overflow bug in element.hh
- Created visual testing framework with canvas-based validation
- Added edge case, robustness, and complex scenario coverage

**Key features:**
- Complete widget library (12+ widgets)
- CSS-style theming with inheritance (v2.0 refactored)
- Render context pattern (visitor) for unified measurement/rendering
- Comprehensive hotkey system with customizable schemes (Windows vs Norton Commander)
- Signal/slot for event handling
- Mnemonic support (Alt+key shortcuts)
- 859 tests with 6236 assertions
- Visual testing framework for layout validation
- Thread-safe theme management with failsafe logging
