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
# Run all 364 unit tests
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

# All 364 tests should pass with zero warnings
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

See `include/onyxui/concepts/backend.hh` for the full concept definition.

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
2. Implement `do_measure()` for custom sizing
3. Implement `do_render()` for custom drawing
4. Override theme accessors if needed (`get_theme_background_color`, etc.)
5. Add to `include/onyxui/widgets/`
6. Write comprehensive tests in `unittest/widgets/`

Example:
```cpp
template<UIBackend Backend>
class my_widget : public widget<Backend> {
    using base = widget<Backend>;

    size_type do_measure(int available_width, int available_height) override {
        // Calculate size
        return size_utils::make<size_type>(width, height);
    }

    void do_render(renderer_type& renderer) override {
        // Draw widget
    }
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
- 364 test cases across 22 test files
- 1815 assertions total
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
- `include/onyxui/layout_strategy.hh` - Layout documentation (600+ lines of design docs)
- `include/onyxui/signal.hh` - Signal/slot pattern implementation
- `include/onyxui/themeable.hh` - CSS-style inheritance system
- `.clang-tidy` - Code quality standards
- `unittest/widgets/test_*.cc` - Widget usage examples

## Recent Major Changes

**Latest commit:** Fix all high and medium priority linter issues
- Fixed critical use-after-move bug in move constructors
- Added const-correctness to 27 variables
- Added [[nodiscard]] to 17 accessor functions
- Optimized 5 enums from int to uint8_t (75% memory savings)
- All 364 tests passing, zero warnings

**Key features:**
- Complete widget library (12+ widgets)
- CSS-style theming with inheritance
- Comprehensive hotkey system
- Signal/slot for event handling
- Mnemonic support (Alt+key shortcuts)
- 364 tests with 1815 assertions
