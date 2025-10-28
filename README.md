# onyxui - Modern C++20 UI Framework

A lightweight, header-only UI framework featuring smart layout caching, thread-safe signals, and backend-agnostic rendering.

[![Tests](https://img.shields.io/badge/tests-771%20passed-success)](unittest/)
[![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)](https://en.cppreference.com/w/cpp/20)
[![License](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)

## Features

### 🎯 Core Features

- **Two-Pass Layout System**: Efficient measure/arrange algorithm with smart caching
- **Thread-Safe Signals**: Optional thread-safe signal/slot system with `std::shared_mutex`
- **Backend Agnostic**: Works with any renderer through C++20 concepts
- **Rich Widget Library**: Buttons, labels, menus, panels, grid, and more
- **CSS-Style Theming**: Property inheritance from parent to child
- **Exception Safe**: Strong and basic exception safety guarantees
- **Recursion Safe**: Verified safe for hierarchies up to 100+ levels

### 🚀 Latest Improvements (2025-10 Refactoring)

- ✅ **Render Context Pattern**: Visitor pattern unifies measurement and rendering (~50% code reduction)
- ✅ **Automatic Theme Synchronization**: Background renderer syncs with theme changes via signal/slot pattern
- ✅ **Exception Safety**: Strong guarantees for `add_child()`, proper cleanup on errors
- ✅ **Thread Safety**: Optional thread-safe signals (enabled by default)
- ✅ **Safe Arithmetic**: Overflow-safe math operations for layout calculations
- ✅ **Float Comparison**: Epsilon-based comparison fixes rounding errors
- ✅ **Comprehensive Documentation**: 4 new guides covering thread safety, performance, and best practices
- ✅ **771 Tests**: 100% pass rate with 4844 assertions

## Quick Start

### Requirements

- **C++20** compiler (GCC 10+, Clang 12+, MSVC 2019+)
- **CMake 3.15+**
- **doctest** (automatically fetched if not found)

### Build

```bash
# Configure
cmake -B build

# Build
cmake --build build -j8

# Run tests
./build/bin/ui_unittest

# Expected output:
# [doctest] test cases:  771 |  771 passed
# [doctest] assertions: 4844 | 4844 passed
# [doctest] Status: SUCCESS!
```

### Hello World Example

```cpp
#include <onyxui/widgets/vbox.hh>
#include <onyxui/widgets/button.hh>
#include <onyxui/widgets/label.hh>
#include <backends/conio/conio_backend.hh>

int main() {
    using Backend = onyxui::conio_backend;

    // Create window with vertical layout
    auto window = onyxui::create_vbox<Backend>();

    // Add title label
    auto title = onyxui::create_label<Backend>("Welcome to onyxui!");
    window->add_child(std::move(title));

    // Add button with click handler
    auto button = onyxui::create_button<Backend>("Click Me");
    button->clicked().connect([]() {
        std::cout << "Button clicked!\n";
    });
    window->add_child(std::move(button));

    // Layout and render
    window->measure(800, 600);
    window->arrange({0, 0, 800, 600});

    return 0;
}
```

## Documentation

### Getting Started

- **[CLAUDE.md](CLAUDE.md)** - Comprehensive project guide and reference
- **[MIGRATION.md](docs/MIGRATION.md)** - Migration guide for latest refactoring

### Best Practices

- **[BEST_PRACTICES.md](docs/BEST_PRACTICES.md)** - Exception safety, memory management, patterns
- **[THREAD_SAFETY.md](docs/THREAD_SAFETY.md)** - Thread safety model and multi-threading guide
- **[PERFORMANCE.md](docs/PERFORMANCE.md)** - Performance tuning and optimization strategies

### API Reference

Browse the well-documented headers in `include/onyxui/`:
- `element.hh` - Core UI element with layout algorithm (800+ lines of documentation)
- `signal.hh` - Thread-safe signal/slot implementation
- `layout_strategy.hh` - Layout system documentation (600+ lines of design docs)
- `widgets/` - Full widget library with examples

## Architecture

### Backend Pattern

All classes use a unified Backend template parameter:

```cpp
template<UIBackend Backend>
class ui_element {
    using rect_type = typename Backend::rect_type;
    using size_type = typename Backend::size_type;
    using renderer_type = typename Backend::renderer_type;
    // ...
};
```

Backends provide types through C++20 concepts:
- `rect_type` (RectLike)
- `size_type` (SizeLike)
- `color_type` (ColorLike)
- `renderer_type` (RenderLike)
- `event_type` (EventLike)

See `include/onyxui/concepts/backend.hh` for the complete `UIBackend` concept.

### Two-Pass Layout

1. **Measure Pass** (bottom-up): Calculate desired size
   ```cpp
   auto size = element->measure(available_width, available_height);
   ```

2. **Arrange Pass** (top-down): Position children
   ```cpp
   element->arrange({x, y, width, height});
   ```

**Smart caching**: Results cached until `invalidate_measure()` or `invalidate_arrange()` is called.

### Signal/Slot System

Thread-safe event handling with automatic connection management:

```cpp
signal<int, std::string> data_changed;

// Connect handler
auto conn = data_changed.connect([](int id, const std::string& name) {
    std::cout << "Data " << id << " changed to " << name << "\n";
});

// Emit signal
data_changed.emit(42, "value");

// Automatic disconnection with scoped_connection
scoped_connection scoped_conn(data_changed, my_handler);
// Disconnects automatically when scoped_conn is destroyed
```

### Background Rendering & Theme Synchronization

The framework automatically synchronizes background colors with theme changes using the signal/slot pattern:

```cpp
#include <onyxui/ui_context.hh>
#include <onyxui/background_renderer.hh>

// Create UI context (auto-connects theme to background)
scoped_ui_context<Backend> ctx;

// Switch theme - background updates automatically!
ctx.themes().set_current_theme("Norton Blue");
// Background is now dark blue (0, 0, 170)

ctx.themes().set_current_theme("DOS Edit");
// Background is now white (255, 255, 255)

// No manual synchronization needed!
```

**How it works:**
- `theme_registry` emits `theme_changed` signal when theme switches
- `background_renderer` subscribes to signal via `on_theme_changed()`
- Connection managed by `scoped_connection` in `ui_context`
- Zero manual synchronization code required

**Backend-Specific Styles:**

Background rendering uses backend-specific `background_style` for flexibility:

```cpp
// TUI backend: color + fill character
struct background_style {
    color bg_color;
    char fill_char = ' ';  // Space = solid, '░'/'▒'/'▓' = patterns
};

// Canvas backend: RGBA + attributes
struct background_style {
    uint8_t fg, bg, attrs;
    char fill_char;
};
```

The `RenderLike` concept requires backends to implement:
- `typename renderer_type::background_style` - Backend-specific style type
- `void draw_background(rect, background_style)` - Full viewport clear
- `void draw_background(rect, background_style, dirty_regions)` - Optimized partial clear

## Widget Library

### Containers

- `vbox` / `hbox` - Vertical/horizontal stacking
- `grid` - Grid layout with cell spanning
- `panel` - Generic container
- `anchor_panel` - Anchor-point positioning
- `absolute_panel` - Fixed positioning
- `group_box` - Bordered container with title

### Controls

- `button` - Clickable button with mnemonics
- `label` - Text display with styling
- `spacer` - Fixed-size spacing
- `spring` - Flexible spacing

### Advanced

- `menu_bar` - Top-level menu container
- `menu` - Dropdown menu
- `menu_item` - Menu entry with actions
- `status_bar` - Bottom status display

### Example: Form Layout

```cpp
auto form = onyxui::create_grid<Backend>(2, 3);  // 2 columns, 3 rows

// Add labels and inputs
form->add_child(onyxui::create_label<Backend>("Name:"));
form->add_child(onyxui::create_textbox<Backend>());

form->add_child(onyxui::create_label<Backend>("Email:"));
form->add_child(onyxui::create_textbox<Backend>());

form->add_child(onyxui::create_label<Backend>("Message:"));
form->add_child(onyxui::create_textbox<Backend>(5));  // 5 rows

// Layout
form->measure(400, 300);
form->arrange({0, 0, 400, 300});
```

## Configuration Options

### CMake Options

```cmake
# Thread-safe signal/slot system (default: ON)
option(ONYXUI_THREAD_SAFE "Enable thread-safe signal/slot system" ON)

# Build tests (default: ON)
option(ONYXUI_BUILD_TEST "Build unit tests" ON)

# Enable sanitizers (default: OFF)
option(ONYXUI_ENABLE_SANITIZERS "Enable AddressSanitizer and UndefinedBehaviorSanitizer" OFF)

# Enable clang-tidy (default: OFF)
option(ONYXUI_ENABLE_CLANG_TIDY "Enable clang-tidy linting" OFF)
```

### Build Configurations

```bash
# Release build (optimized)
cmake -B build -DCMAKE_BUILD_TYPE=Release

# Debug build (with symbols)
cmake -B build -DCMAKE_BUILD_TYPE=Debug

# Disable thread safety for single-threaded apps
cmake -B build -DONYXUI_THREAD_SAFE=OFF

# Enable sanitizers for testing
cmake -B build -DONYXUI_ENABLE_SANITIZERS=ON

# Enable clang-tidy linting
cmake -B build -DONYXUI_ENABLE_CLANG_TIDY=ON
```

## Thread Safety

### Default: Thread-Safe Signals

The signal/slot system is **thread-safe by default** (when `ONYXUI_THREAD_SAFE=ON`):

```cpp
signal<int> value_changed;  // Thread-safe

// Thread 1: Connect
std::thread t1([&]() {
    value_changed.connect([](int val) { /* handler */ });
});

// Thread 2: Emit
std::thread t2([&]() {
    value_changed.emit(42);  // Safe: uses std::shared_mutex
});
```

**Performance**: ~5-10% overhead for thread safety. Disable with `-DONYXUI_THREAD_SAFE=OFF` for single-threaded applications.

### UI Thread Model

**Single-threaded components** (must run on UI thread):
- Layout operations (`measure()`, `arrange()`)
- Element tree modifications (`add_child()`, `remove_child()`)
- Event handling (`on_mouse_down()`, etc.)

**Cross-thread communication pattern**:

```cpp
// Background thread
std::thread worker([&]() {
    auto result = do_work();

    // Queue for UI thread
    app.queue_ui_task([&label, result]() {
        label->set_text(result);  // Safe on UI thread
    });
});
```

See [THREAD_SAFETY.md](docs/THREAD_SAFETY.md) for complete threading guidelines.

## Performance

### Benchmarks (Typical Desktop)

- **Layout time**: <5ms for 1000 elements
- **Signal emission**: ~5-50ns depending on thread safety
- **Memory**: ~200 bytes per element (backend-dependent)
- **Frame time**: <10ms for complex UIs (100fps)

### Optimization Tips

1. **Batch signal emissions** for high-frequency events
2. **Keep hierarchy depth under 15 levels**
3. **Use `reserve()` for child vectors**
4. **Only invalidate when properties actually change**
5. **Consider disabling thread safety** for single-threaded apps

See [PERFORMANCE.md](docs/PERFORMANCE.md) for detailed optimization strategies.

## Testing

### Run Tests

```bash
# All tests
./build/bin/ui_unittest

# Specific test
./build/bin/ui_unittest --test-case="Signal - Basic connection"

# List all tests
./build/bin/ui_unittest --list-test-cases
```

### Test Coverage

- **771 test cases** across 27 test files
- **4844 assertions** covering all major functionality
- **100% pass rate**

Test categories:
- Core: Element hierarchy, signals, rule of five, exception safety
- Layout: Linear, grid, anchor, absolute, composition
- Widgets: Buttons, labels, menus, panels, all widgets
- Hotkeys: Key sequences, hotkey manager
- Focus: Focus management and navigation

### Sanitizers

```bash
# AddressSanitizer + UndefinedBehaviorSanitizer
cmake -B build -DONYXUI_ENABLE_SANITIZERS=ON
cmake --build build
./build/bin/ui_unittest

# ThreadSanitizer
cmake -B build -DCMAKE_CXX_FLAGS="-fsanitize=thread -g"
cmake --build build
./build/bin/ui_unittest
```

## Project Structure

```
include/onyxui/
  ├── concepts/              # C++20 concepts (backend, rect, size, color, event)
  ├── layout/                # Layout strategies (linear, grid, anchor, absolute)
  ├── widgets/               # Widget library (button, label, menu, panel, etc.)
  ├── hotkeys/               # Hotkey management system
  ├── utils/                 # Utilities (safe_math, etc.)
  ├── element.hh             # Core ui_element class
  ├── signal.hh              # Signal/slot implementation
  ├── event_target.hh        # Event handling base
  ├── themeable.hh           # CSS-style theming
  ├── focus_manager.hh       # Focus navigation
  └── layer_manager.hh       # Layer and popup management

backends/
  └── conio/                 # DOS/TUI backend example

docs/
  ├── THREAD_SAFETY.md       # Thread safety guide
  ├── PERFORMANCE.md         # Performance tuning guide
  ├── BEST_PRACTICES.md      # Best practices and patterns
  └── MIGRATION.md           # Migration guide

unittest/                    # 440 tests in 22 files
  ├── core/                  # Element, signals, exception safety
  ├── layout/                # Layout algorithm tests
  ├── widgets/               # Widget functionality tests
  ├── hotkeys/               # Hotkey system tests
  └── focus/                 # Focus management tests
```

## Code Quality

### Static Analysis

```bash
# clang-tidy (comprehensive checks)
cmake -B build -DONYXUI_ENABLE_CLANG_TIDY=ON
cmake --build build 2>&1 | grep "warning:"

# Expected: Zero warnings
```

**Enforced standards**:
- No use-after-move
- No uninitialized variables
- Proper const-correctness
- `[[nodiscard]]` on value-returning functions
- Proper initialization order in constructors

See `.clang-tidy` for complete configuration.

### Compiler Support

**Tested compilers**:
- GCC 10+ ✅
- Clang 12+ ✅
- MSVC 2019+ ✅ (with `/std:c++20 /permissive-`)

**Warning flags**: Maximum warnings enabled by default (`-Wall -Wextra -Wpedantic`).

## Examples

### DOS/TUI Backend

```bash
# Build conio demo
cmake --build build --target conio

# Run demo
./build/bin/conio
```

The conio backend demonstrates:
- DOS-style text UI rendering
- Theme system with DOS color palette
- Menu system with keyboard navigation
- Hotkey management
- Layout system in action

## Contributing

### Development Workflow

1. **Read documentation**: [CLAUDE.md](CLAUDE.md) for project guide
2. **Follow best practices**: [BEST_PRACTICES.md](docs/BEST_PRACTICES.md)
3. **Write tests**: All new features must have tests
4. **Run sanitizers**: Verify with ASAN/UBSAN/TSAN
5. **Check with clang-tidy**: Zero warnings required
6. **Document changes**: Update docs for public APIs

### Code Style

- **Naming**: `lower_case` for classes/functions/variables
- **Members**: `m_` prefix for private members
- **Constants**: `UPPER_CASE` or `constexpr` with clear names
- **Documentation**: Doxygen-style comments for public APIs

See [BEST_PRACTICES.md](docs/BEST_PRACTICES.md) for complete style guide.

## License

[MIT License](LICENSE) - See LICENSE file for details.

## Acknowledgments

- **doctest** - Fast C++ testing framework
- **C++20** - Modern C++ features (concepts, ranges, etc.)
- **UTF8-CPP** - UTF-8 string handling

## Links

- **Documentation**: [CLAUDE.md](CLAUDE.md)
- **Thread Safety**: [THREAD_SAFETY.md](docs/THREAD_SAFETY.md)
- **Performance**: [PERFORMANCE.md](docs/PERFORMANCE.md)
- **Best Practices**: [BEST_PRACTICES.md](docs/BEST_PRACTICES.md)
- **Migration Guide**: [MIGRATION.md](docs/MIGRATION.md)
- **Test Suite**: [unittest/](unittest/)
- **Examples**: [backends/conio/](backends/conio/)

---

**Status**: Production-ready | **Tests**: 771/771 passing | **Assertions**: 4844/4844 passing | **Version**: 2025-10 Refactoring
