# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

---

## 📚 Documentation Index

**Quick Navigation:**
- [Architecture Guide](docs/CLAUDE/ARCHITECTURE.md) - Core design patterns, layout algorithm, render context
- [Theming Guide](docs/CLAUDE/THEMING.md) - Theme system v2.0, CSS inheritance, styling
- [Hotkeys Guide](docs/CLAUDE/HOTKEYS.md) - Keyboard shortcuts, schemes, semantic actions
- [Testing Guide](docs/CLAUDE/TESTING.md) - Test strategy, fixtures, best practices
- [Changelog](docs/CLAUDE/CHANGELOG.md) - Recent major changes, migration notes
- [Scrolling Guide](docs/scrolling_guide.md) - Comprehensive scrolling system documentation
- [Refactoring Plan](docs/REFACTORING_PLAN.md) - Upcoming code improvements

---

## Project Overview

**onyx_ui** is a modern C++20 header-only UI framework featuring:
- **Relative Coordinate System**: Children store bounds relative to parent's content area (0,0 origin) for efficient repositioning and clean architecture
- **Layout System**: Two-pass measure/arrange algorithm with smart caching
- **Widget Library**: Buttons, labels, menus, status bars, group boxes, scrolling, and more
- **Scrolling System**: Three-layer architecture (scroll_view, scrollable, scrollbar) with presets
- **Theming System**: CSS-style property inheritance for visual styling
- **Event System**: Signal/slot pattern for decoupled communication
- **Hotkeys**: Global keyboard shortcut management with customizable schemes
- **Backend Agnostic**: Works with any renderer through C++20 concepts

The framework uses a unified **Backend pattern** where a single template parameter provides all platform-specific types (rect, size, color, renderer, etc.).

---

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
# Run all 1184 unit tests
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

# All 1184 tests should pass with zero warnings
```

---

## Quick Start Guide

### Creating a Simple UI

```cpp
#include <onyxui/ui_context.hh>
#include <onyxui/widgets/vbox.hh>
#include <onyxui/widgets/button.hh>
#include <onyxui/widgets/label.hh>

// Backend-specific includes
#include <backends/conio/conio_backend.hh>

int main() {
    // 1. Create UI context (themes auto-registered)
    scoped_ui_context<conio_backend> ctx;

    // 2. Create root container
    auto root = std::make_unique<vbox<conio_backend>>(5);  // 5px spacing

    // 3. Apply theme
    root->apply_theme("Norton Blue", ctx.themes());

    // 4. Add widgets
    root->emplace_child<label>("Hello, OnyxUI!");
    auto* btn = root->template emplace_child<button>("Click Me");

    // 5. Connect events
    btn->clicked.connect([]() {
        std::cout << "Button clicked!\n";
    });

    // 6. Measure, arrange, render
    root->measure(80, 25);
    root->arrange({0, 0, 80, 25});

    // Render loop...
}
```

### Using Scrolling

```cpp
#include <onyxui/widgets/scroll_view_presets.hh>

// Create scroll view with preset
auto view = modern_scroll_view<Backend>();  // Auto-hide scrollbars

// Add content
view->emplace_child<label>("Item 1");
view->emplace_child<button>("Item 2");
// ... many more items ...

// Scrolling is automatic!
```

---

## Core Concepts (Quick Reference)

### Backend Pattern

```cpp
template<UIBackend Backend>
class ui_element {
    using rect_type = Backend::rect_type;
    using size_type = Backend::size_type;
    using color_type = Backend::color_type;
    // ...
};
```

See [Architecture Guide](docs/CLAUDE/ARCHITECTURE.md) for details.

### Two-Pass Layout

1. **Measure Pass** (bottom-up): `measure(available_width, available_height) -> size_type`
2. **Arrange Pass** (top-down): `arrange(final_bounds)`

See [Architecture Guide](docs/CLAUDE/ARCHITECTURE.md#two-pass-layout-algorithm) for details.

### Render Context Pattern (Visitor)

Unified measurement and rendering through a single `do_render()` method:

```cpp
void do_render(render_context<Backend>& ctx) const override {
    // Same code handles both measurement AND rendering!
    auto text_size = ctx.draw_text(m_text, position, font, color);
    ctx.draw_rect(bounds, box_style);
}
```

See [Architecture Guide](docs/CLAUDE/ARCHITECTURE.md#render-context-pattern-visitor-pattern) for details.

### Theming

```cpp
// Apply theme at root
root->apply_theme("Norton Blue", ctx.themes());

// Children inherit automatically via CSS
auto button = root->template emplace_child<button>();  // Inherits theme!

// Override on specific widgets
button->set_background_color({0, 255, 0});  // Green button
```

See [Theming Guide](docs/CLAUDE/THEMING.md) for details.

### Event System

OnyxUI provides both three-phase event routing and signal/slot communication:

```cpp
// 1. Three-phase event routing (capture/target/bubble)
class my_widget : public widget<Backend> {
    bool handle_event(const ui_event& evt, event_phase phase) override {
        // Handle events in CAPTURE, TARGET, or BUBBLE phase
        if (phase == event_phase::capture) {
            request_focus();  // Before children handle
            return false;     // Continue to children
        }
        return base::handle_event(evt, phase);
    }
};

// 2. Signal/slot pattern for decoupled communication
signal<int> value_changed;

// Connect handler
value_changed.connect([](int val) {
    std::cout << "Value: " << val << "\n";
});

// Emit signal
value_changed.emit(42);

// Scoped connections (RAII)
scoped_connection conn(value_changed, my_handler);
```

**Three-phase routing** enables composite widgets to intercept events before children (e.g., text_view requesting focus on any click). **Signals** provide loose coupling for widget communication.

See [Architecture Guide](docs/CLAUDE/ARCHITECTURE.md#event-system) for comprehensive event documentation.

### Hotkeys

```cpp
// Framework semantic actions (scheme-based)
ctx.hotkeys().register_semantic_action(
    hotkey_action::activate_menu_bar,
    [&menu]() { menu->activate(); }
);

// F10 (Windows) or F9 (Norton Commander) activates menu automatically!

// Application-specific hotkeys
auto save_action = std::make_shared<action<Backend>>();
save_action->set_shortcut('s', key_modifier::ctrl);
hotkeys.register_action(save_action);
```

See [Hotkeys Guide](docs/CLAUDE/HOTKEYS.md) for details.

---

## Widget Library

**Containers:**
- `vbox` / `hbox` - Vertical/horizontal stacking
- `grid` - Grid layout
- `panel` - Generic container
- `anchor_panel` - Anchor-point positioning
- `absolute_panel` - Fixed positioning
- `group_box` - Bordered container with title
- `tab_widget` - Multi-page container with tab navigation

**Controls:**
- `button` - Clickable button with mnemonics
- `label` - Text display with styling
- `spacer` - Fixed-size spacing
- `spring` - Flexible spacing

**Menus:**
- `menu_bar` - Top-level menu container
- `menu` - Dropdown menu
- `menu_item` - Menu entry with actions

**Scrolling:**
- `scroll_view` - Batteries-included scrolling container
- `scrollable` - Scrollable content viewport
- `scrollbar` - Visual scrollbar widget
- `scroll_controller` - Bidirectional synchronization

See [Scrolling Guide](docs/scrolling_guide.md) for comprehensive scrolling documentation.

**Input Widgets:**
- `line_edit` - Single-line text input with scrolling and cursor support
- `checkbox` - Boolean toggle with theming
- `radio_button` - Mutually exclusive options with button_group
- `slider` - Numeric range input with keyboard and mouse support
- `progress_bar` - Visual progress indicator (determinate/indeterminate)

**Other:**
- `status_bar` - Bottom status display

---

## Project Structure

```
include/onyxui/
  actions/               # Command pattern (action, action_group, mnemonic_parser)
  concepts/              # C++20 concepts (backend, rect, size, color, render, event)
  core/                  # Framework fundamentals (element, signal, event_target)
    raii/                # RAII guards (scoped_clip, scoped_layer, scoped_tooltip)
    rendering/           # Render contexts (render_context, draw_context, measure_context, resolved_style)
  events/                # Event system (ui_event)
  hotkeys/               # Keyboard shortcuts (hotkey_manager, key_sequence, hotkey_schemes)
  layout/                # Layout strategies (linear, grid, anchor, absolute)
  services/              # Framework services (ui_context, focus_manager, layer_manager, background_renderer)
  theming/               # Theme system (theme, themeable, theme_registry, theme_loader)
  utils/                 # Utilities (safe_math, fkyaml_adapter)
  widgets/               # UI components (button, label, status_bar, styled_text)
    containers/          # Layout containers (panel, grid, hbox, vbox, group_box)
      scroll/            # Scrolling subsystem (scrollable, scrollbar, scroll_view, scroll_controller)
    core/                # Base widget classes (widget, widget_container, stateful_widget)
    layout/              # Layout helper widgets (spacer, spring)
    menu/                # Menu subsystem (menu, menu_bar, menu_item)

unittest/
  core/                  # Core tests (signal, element, rendering, layer_manager, etc.)
  events/                # Event system tests
  focus/                 # Focus management tests
  hotkeys/               # Hotkey tests
  layout/                # Layout strategy tests
  layer/                 # Layer management tests
  reflection/            # YAML/theme reflection tests
  utils/                 # Test utilities and helpers
  widgets/               # Widget tests (button, label, scrolling, menus, etc.)

backends/
  conio/                 # DOS/TUI backend example

docs/
  CLAUDE/                # Claude Code documentation (architecture, theming, hotkeys, testing)
  scrolling_guide.md     # Comprehensive scrolling system user guide
  REFACTORING_PLAN.md    # Code quality improvements
```

---

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
    void do_render(render_context<Backend>& ctx) const override {
        // Same code handles both measurement AND rendering!
        ctx.draw_rect(this->bounds(), m_box_style);
        auto text_size = ctx.draw_text(m_text, {x, y}, m_font, m_color);
    }

private:
    std::string m_text;
    typename Backend::renderer_type::box_style m_box_style;
    typename Backend::renderer_type::font m_font;
    typename Backend::color_type m_color;
};
```

See [Architecture Guide](docs/CLAUDE/ARCHITECTURE.md) for architectural details.

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

### Testing

See [Testing Guide](docs/CLAUDE/TESTING.md) for comprehensive testing documentation.

**Quick summary:**
- Use `ui_context_fixture<test_canvas_backend>` for widget tests
- Use regular `TEST_CASE` for non-widget tests
- Test both measure and arrange phases
- Test edge cases (empty, zero size, max values)
- All tests must pass with zero warnings

### Common Pitfalls

1. **Backend Pattern** - Use `Backend` template parameter, not separate `TRect`, `TSize`
2. **Initialization Order** - Base classes initialize before members (use static_cast for moves)
3. **Inheritance** - Use CSS-style theming, don't repeat colors on every widget
4. **Cache Invalidation** - Any property change must invalidate layout appropriately
5. **Ownership** - Use `std::move()` when passing children to `add_child()`
6. **Signals** - Use `scoped_connection` for automatic cleanup

---

## Compiler Requirements

- **C++20** required (concepts, designated initializers)
- **CMake 3.15+** minimum
- **Tested compilers:**
  - GCC 10+
  - Clang 12+
  - MSVC 2019+ (with `/W4 /permissive-`)
- **Comprehensive warning flags** enabled by default

---

## Key Reference Files

- `include/onyxui/core/element.hh` - Core layout algorithm (800+ lines, well-documented)
- `include/onyxui/core/rendering/render_context.hh` - Abstract visitor base for measurement/rendering
- `include/onyxui/core/rendering/draw_context.hh` - Concrete rendering visitor
- `include/onyxui/core/rendering/measure_context.hh` - Concrete measurement visitor
- `include/onyxui/layout/layout_strategy.hh` - Layout documentation (600+ lines of design docs)
- `include/onyxui/core/signal.hh` - Signal/slot pattern implementation
- `include/onyxui/core/raii/scoped_clip.hh` - RAII clipping guard
- `include/onyxui/theming/themeable.hh` - CSS-style inheritance system
- `include/onyxui/theming/theme.hh` - Theme structure definition
- `include/onyxui/actions/action.hh` - Command pattern for UI actions
- `.clang-tidy` - Code quality standards
- `unittest/widgets/test_*.cc` - Widget usage examples

---

## Current Status

**Version:** 2025-11 (November 2025)

**Test Coverage:**
- **1184 test cases** across 38 test files
- **6764 assertions**
- **Zero warnings**
- **100% widget coverage**

**Recent Major Features:**
- **Relative coordinate system refactoring** (Nov 2025) - Children store relative bounds for efficient repositioning and clean architecture
- Comprehensive scrolling system (137 new tests)
- Hotkey schemes (Windows vs Norton Commander)
- Theme system v2.0 refactoring (thread-safe, style-based rendering)

See [Changelog](docs/CLAUDE/CHANGELOG.md) for detailed recent changes.

---

## Getting Help

For detailed information on specific topics:
- **Architecture & Design Patterns** → [Architecture Guide](docs/CLAUDE/ARCHITECTURE.md)
- **Theming & Styling** → [Theming Guide](docs/CLAUDE/THEMING.md)
- **Keyboard Shortcuts** → [Hotkeys Guide](docs/CLAUDE/HOTKEYS.md)
- **Writing Tests** → [Testing Guide](docs/CLAUDE/TESTING.md)
- **Recent Changes** → [Changelog](docs/CLAUDE/CHANGELOG.md)
- **Scrolling System** → [Scrolling Guide](docs/scrolling_guide.md)
- **Code Quality** → [Refactoring Plan](docs/REFACTORING_PLAN.md)

For examples, see:
- `backends/conio/main.cc` - Complete demo application
- `unittest/widgets/test_*.cc` - Widget usage examples
- `unittest/widgets/test_scrolling_integration.cc` - Real-world integration tests

---

**Happy coding with OnyxUI!** 🚀
