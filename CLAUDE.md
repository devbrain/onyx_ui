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

**2. Three-Way Theme API (Safe Ownership)**

The old unsafe `apply_theme(const theme_type&)` has been removed. Use one of three safe options:

```cpp
// Option 1: By name (registry-based) - RECOMMENDED
// Zero overhead, registry owns the theme
element->apply_theme("Norton Blue", registry);

// Option 2: By value (copy/move)
// Element takes ownership via unique_ptr
ui_theme<Backend> my_theme = create_custom_theme();
element->apply_theme(std::move(my_theme));  // Moved!

// Option 3: By shared_ptr
// Reference-counted shared ownership
auto theme_ptr = std::make_shared<ui_theme<Backend>>(my_theme);
element->apply_theme(theme_ptr);
```

**Benefits:**
- No dangling pointer risk (old API removed)
- Clear ownership semantics
- Flexible for different use cases
- Type-safe and explicit

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

**4. Stateful Widget Helper**

New base class for interactive widgets with visual states:

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

    // State-based color helpers
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
    void do_render(render_context<Backend>& ctx) const override {
        auto* theme = this->get_theme();
        if (theme) {
            // Get state-appropriate colors automatically
            auto bg = this->get_state_background(theme->button);
            auto fg = this->get_state_foreground(theme->button);

            // Render with state colors...
        }
    }

    void on_mouse_enter() override {
        this->set_interaction_state(interaction_state::hover);
        // Automatically calls invalidate_arrange() to trigger redraw
    }
};
```

**Theme structure expects:**
```cpp
struct button_theme {
    color_type bg_normal, fg_normal;
    color_type bg_hover, fg_hover;
    color_type bg_pressed, fg_pressed;
    color_type bg_disabled, fg_disabled;
};
```

**Architecture Summary:**

1. **CSS Inheritance** → Resolves to `resolved_style`
2. **resolved_style** → Passed to `render_context` (draw/measure)
3. **render_context** → Provides style to widgets during rendering
4. **Widgets** → Use pre-resolved style, no repeated lookups

**Performance characteristics:**
- Style resolution: O(depth) once per frame
- Widget rendering: O(1) style access
- Thread-safe: Reader-writer locking on registry
- Memory: POD-like resolved_style (~50-100 bytes)

**See also:**
- `include/onyxui/resolved_style.hh` - Style structure
- `include/onyxui/render_context.hh` - Visitor pattern base
- `include/onyxui/draw_context.hh` - Rendering implementation
- `include/onyxui/measure_context.hh` - Measurement implementation
- `include/onyxui/widgets/stateful_widget.hh` - Interactive widget helper
- `include/onyxui/theme_registry.hh` - Thread-safe registry
- `include/onyxui/themeable.hh` - Three-way theme API
- `unittest/theming/test_resolved_style.cc` - Comprehensive tests (11 test cases, 88 assertions)

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

Global keyboard shortcut management:

```cpp
hotkey_manager<Backend> hotkeys;

// Register hotkeys
hotkeys.register_hotkey(
    key_sequence::from_string("Ctrl+S"),
    save_action,
    hotkey_scope::global
);

// Process events
hotkeys.process_event(event);  // Triggers matching actions
```

Features:
- Conflict detection with policy enforcement
- Scope management (global, local)
- Key sequence parsing ("Ctrl+Shift+A", "Alt+F4", etc.)
- Integration with action system

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
  └─ Supports dirty region optimization
```

**Three Rendering Modes:**

1. **Solid Mode** (default) - Fill with solid color
   - Respects dirty regions (only fills changed areas)
   - Suitable for traditional desktop applications
   - Optimized: O(dirty_regions) draw calls

2. **Transparent Mode** - No background
   - UI widgets overlay game/3D rendering
   - Suitable for game HUDs, in-game menus
   - Zero overhead: no rendering

3. **Pattern Mode** (future) - Tiled/stretched textures
   - Currently falls back to solid mode
   - Reserved for future texture support

**Usage Pattern:**

```cpp
// Access from ui_services (not widget tree)
auto* bg = ui_services<Backend>::background();
if (bg) {
    bg->set_mode(background_mode::solid);
    bg->set_color({0, 0, 170});  // Blue background
}

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

**Testing:**

Comprehensive tests in `unittest/core/test_background_renderer.cc`:
- 8 test cases, 67 assertions
- Mode switching, color changes, dirty regions
- Renderer state verification
- ui_context integration
- Copy/move semantics

**See also:**
- `include/onyxui/background_renderer.hh` - Implementation
- `include/onyxui/ui_context.hh` - Service integration
- `include/onyxui/ui_handle.hh` - Rendering pipeline
- `examples/demo.cc` - Usage example

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
- 569 test cases across 27 test files
- 2771 assertions total
- All tests must pass with zero warnings

**Writing tests:**
```cpp
#include <doctest/doctest.h>

TEST_CASE("Widget - Basic functionality") {
    SUBCASE("First scenario") {
        // Test code
        CHECK(condition);
    }

    SUBCASE("Second scenario") {
        // Test code
        CHECK_FALSE(condition);
    }
}
```

**Best practices:**
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

**Latest work:** Theme System v2.0 Refactoring (October 2025)
- **Phase 1**: Added thread-safe theme registry with failsafe logging
- **Phase 2**: Implemented three-way theme API (by-name, by-value, by-shared_ptr)
- **Phase 3**: Style-based rendering architecture with `resolved_style`
- **Phase 4**: Comprehensive testing (11 new test cases, 88 assertions)
- **Phase 5**: Documentation and polish
- Removed unsafe `apply_theme(const theme_type&)` API
- All 653 tests passing (was 642), zero warnings

**Key architectural improvements:**
- **Thread Safety**: Reader-writer locking on theme registry
- **Performance**: CSS inheritance resolved once per frame, not per widget
- **Safety**: No dangling pointer risk with new ownership semantics
- **Simplicity**: Widgets receive pre-resolved style via render_context
- **Stateful Widgets**: New helper base class for interactive widgets

**Previous work:** Comprehensive layout testing implementation (2025)
- Implemented 57 new layout tests across all priority levels
- Fixed critical y=65541 overflow bug in element.hh
- Created visual testing framework with canvas-based validation
- Added edge case, robustness, and complex scenario coverage

**Key features:**
- Complete widget library (12+ widgets)
- CSS-style theming with inheritance (v2.0 refactored)
- Render context pattern (visitor) for unified measurement/rendering
- Comprehensive hotkey system
- Signal/slot for event handling
- Mnemonic support (Alt+key shortcuts)
- 653 tests with 4052 assertions
- Visual testing framework for layout validation
- Thread-safe theme management with failsafe logging
