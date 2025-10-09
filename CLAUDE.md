# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

**onyx_ui** is a C++ header-only UI layout system using modern C++20 concepts. It's a framework-agnostic retained-mode GUI layout engine that can work with any rectangle/size types (SDL_Rect, SFML IntRect, custom types, etc.) through C++20 concepts.

The project is based on the design document in `docs/layout.md`, which provides a comprehensive specification for the layout system architecture.

## Build Commands

### Initial Configuration
```bash
# Configure the build (from project root)
cmake -B build

# Configure with tests disabled
cmake -B build -DONYX_UI_BUILD_TEST=OFF

# Configure with sanitizers enabled
cmake -B build -DONYX_UI_ENABLE_SANITIZERS=ON
```

### Building
```bash
# Build all targets
cmake --build build

# Build with specific number of jobs
cmake --build build -j8
```

### Running Tests
```bash
# Run all unit tests
./build/bin/ui_unittest

# Run tests with doctest options (e.g., list test cases)
./build/bin/ui_unittest --list-test-cases

# Run specific test suite
./build/bin/ui_unittest --test-suite="suite-name"
```

## Code Architecture

### Core Concepts-Based Design

The library uses C++20 concepts to provide type-agnostic geometric operations. This allows users to bring their own rectangle and size types without wrapper classes or conversions.

**Key Concepts:**
- `RectLike` - Accepts types with `{x, y, w, h}` or `{x, y, width, height}`
- `SizeLike` - Accepts types with `{w, h}` or `{width, height}`
- `PointLike` - Accepts types with `{x, y}`

Utility namespaces `rect_utils` and `size_utils` provide generic operations on these types.

### Two-Pass Layout Algorithm

The layout system uses a measure-then-arrange approach:

1. **Measure Pass** (`measure()`): Bottom-up traversal where each element calculates its desired size based on available space and children's requirements. Results are cached.

2. **Arrange Pass** (`arrange()`): Top-down traversal where each element is given its final bounds and positions its children within those bounds.

### Smart Invalidation

- `invalidate_measure()`: Propagates **upward** (parents need remeasurement when children change)
- `invalidate_arrange()`: Propagates **downward** (children need repositioning when parent changes)

Layout states: `valid`, `dirty`, `propagated` - prevent redundant invalidation propagation.

### Template Architecture

All UI classes are templated on rectangle and size types:
```cpp
template<RectLike TRect, SizeLike TSize>
class ui_element { ... }

template<RectLike TRect, SizeLike TSize>
class layout_strategy { ... }
```

Users create type aliases for convenience:
```cpp
using my_element = ui_element<MyRect, MySize>;
using my_linear_layout = linear_layout<MyRect, MySize>;
```

### Layout Strategies

The system provides pluggable layout strategies (Strategy pattern):

- **`linear_layout`** - Stack children horizontally or vertically with spacing/alignment
- **`grid_layout`** - Arrange children in a grid with cell spanning support
- **`anchor_layout`** - Position children relative to anchor points (top-left, center, etc.)
- **`absolute_layout`** - Fixed positioning with explicit coordinates

Each layout strategy implements:
- `measure_children()` - Calculate total size needed
- `arrange_children()` - Position children within allocated space

### Size Policies

Elements use `size_constraint` with multiple policies:
- `fixed` - Use exact preferred_size
- `content` - Size based on content (wrap)
- `expand` - Grow to fill available space
- `fill_parent` - Match parent's content area
- `percentage` - Percentage of parent space
- `weighted` - Proportional distribution (flex-grow style)

Constraints also enforce `min_size` and `max_size` bounds.

### Memory Management

- Tree structure uses `std::unique_ptr<ui_element>` for ownership
- Parent pointers are raw non-owning pointers
- Layout strategies are owned by `std::unique_ptr<layout_strategy>`
- `add_child()` takes ownership, `remove_child()` returns ownership

## Project Structure

```
include/onyxui/           # All header files (header-only library)
  concepts.hh             # Core C++20 concepts and utility functions
  element.hh              # Main ui_element class with measure/arrange logic
  layout_strategy.hh      # Abstract base and common types (enums, size_constraint)
  layout/
    linear_layout.hh      # Vertical/horizontal stack layout
    grid_layout.hh        # Grid-based layout with spans
    anchor_layout.hh      # Anchor-point positioning
    absolute_layout.hh    # Fixed absolute positioning

unittest/                 # Unit tests using doctest
  CMakeLists.txt          # Test target configuration (fetches doctest)
  main.cc                 # Doctest main entry point

docs/
  layout.md               # Comprehensive design specification (1700+ lines)

build/                    # CMake build output (git-ignored)
  bin/                    # Executable outputs (tests)
  lib/                    # Library outputs (if any)
```

## Development Guidelines

### Adding New Layout Strategies

1. Create a new header in `include/onyxui/layout/`
2. Template on `<RectLike TRect, SizeLike TSize>`
3. Inherit from `layout_strategy<TRect, TSize>`
4. Implement `measure_children()` and `arrange_children()`
5. Add to `CMakeLists.txt` INTERFACE library sources
6. Write tests in `unittest/`

### Working with ui_element

- Always call `measure()` before `arrange()`
- Access children via private `m_children` (use getters in derived classes if needed)
- Use `invalidate_measure()` when properties affecting size change
- Use `invalidate_arrange()` when properties affecting position change
- Cached measure results in `m_last_measured_size`, `m_last_available_width/height`

### Coordinate System

- All coordinates are integers
- Origin is top-left (standard UI convention)
- Width/height must be non-negative
- Margin is outside the element bounds
- Padding is inside the element bounds

### Common Pitfalls

- **Don't access children directly outside ui_element** - they're private members
- **Always respect the concepts** - Your types must satisfy RectLike/SizeLike
- **Remember spacing vs margin vs padding**:
  - Spacing: gap between siblings (in layout strategy)
  - Margin: external spacing around element
  - Padding: internal spacing within element
- **Visibility** - Hidden elements (`visible = false`) are skipped in layout
- **Cache invalidation** - Changing any size-affecting property must call `invalidate_measure()`

## Testing Strategy

The project uses **doctest** for unit testing, fetched automatically via CMake FetchContent.

When writing tests:
- Place test files in `unittest/`
- Include `<doctest/doctest.h>`
- Use `TEST_CASE()` macro with descriptive names
- Test both measure and arrange phases separately
- Verify bounds, not just successful compilation
- Test edge cases: empty children, zero sizes, extreme values

## Compiler Requirements

- **C++20** required (for concepts)
- **CMake 4.0+** (Note: This is unusually high, verify if intentional)
- Tested with GCC and Clang (comprehensive warning flags enabled)
- MSVC support included with `/W4 /permissive-`

## Design Reference

Refer to `docs/layout.md` for:
- Complete architecture rationale
- Detailed algorithm explanations
- Performance optimization strategies
- Hit testing and Z-ordering implementation
- Scrollable panel implementation
- Layout debugging utilities
- Comprehensive usage examples with different frameworks (SDL, SFML)
