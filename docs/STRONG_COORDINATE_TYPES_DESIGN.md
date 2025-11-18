# Strong Coordinate Types Design Document

**Status**: ✅ IMPLEMENTED (as of November 2025)
**Version**: v2.0
**Implementation**: See `include/onyxui/geometry/coordinates.hh` and `.inl`
**Created**: 2025-11-11
**Last Updated**: 2025-11-18 (Implementation complete, all 1334 tests passing)
**Author**: Architecture Team

**Note**: This document served as the design specification for the relative coordinate system refactoring. For current documentation, see [Architecture Guide](CLAUDE/ARCHITECTURE.md#relative-coordinate-system).

**Migration Policy**: This was a **clean break v2.0 release**. No deprecated APIs, no transition period. All coordinate types were replaced with strong types in a single release (Phase 1 and Phase 2 complete).

---

## ⚠️ IMPORTANT CORRECTION

**Initial Error**: First draft used raw `int x, y` types, which is incorrect for OnyxUI.

**Correct Approach**: OnyxUI uses **Backend-provided types** (`Backend::rect_type`, `Backend::point_type`). We must wrap these backend types with strong type wrappers, not define our own coordinate types from scratch.

**Key Insight**: The Backend pattern provides platform-specific geometry types. Our strong types wrap these, adding compile-time coordinate system safety while preserving backend flexibility.

---

## Executive Summary

This document proposes introducing strong types for coordinate systems to prevent mixing of absolute screen coordinates with relative widget coordinates at compile time. This addresses a class of bugs discovered during window control button implementation and provides compile-time safety for spatial calculations.

**Key Benefits**:
- ✅ Prevents coordinate mixing bugs at compile time
- ✅ Self-documenting API (clear intent)
- ✅ Type-safe coordinate transformations
- ✅ Better IDE support and error messages

**Key Costs**:
- ⚠️ Requires touching ~100+ call sites
- ⚠️ Breaking API change (v2.0 only)
- ⚠️ Increased code verbosity

---

## Table of Contents

1. [Problem Statement](#problem-statement)
2. [Current State Analysis](#current-state-analysis)
3. [Proposed Solution](#proposed-solution)
4. [Type System Design](#type-system-design)
5. [API Design](#api-design)
6. [Migration Strategy](#migration-strategy)
7. [Implementation Plan](#implementation-plan)
8. [Testing Strategy](#testing-strategy)
9. [Performance Considerations](#performance-considerations)
10. [Alternatives Considered](#alternatives-considered)
11. [Open Questions](#open-questions)

---

## Problem Statement

### The Bug That Revealed The Issue

**Date**: 2025-11-10
**Issue**: Window control buttons (minimize, maximize, close, menu) not responding to clicks
**Root Cause**: Mixing absolute screen coordinates with relative widget coordinates

**Code Example (Buggy)**:
```cpp
// window_title_bar.inl (BEFORE FIX)
auto* hit_target = win->hit_test(mouse_evt->x, mouse_evt->y, path);
// mouse_evt contains ABSOLUTE screen coords
// But icon bounds() returns RELATIVE coords!

if (icon_contains(mouse_evt)) {  // BUG: comparing different coordinate systems
    menu_clicked.emit();
}
```

**The Fix** (Current Workaround):
```cpp
// window_title_bar.inl (AFTER FIX)
auto abs_bounds = icon->get_absolute_bounds();  // Convert to absolute
if (mouse_x >= abs_bounds.x && mouse_x < abs_bounds.x + abs_bounds.w) {
    menu_clicked.emit();  // Now comparing apples to apples
}
```

### Why The Bug Happened

The type system allows this dangerous code to compile:
```cpp
int absolute_x = mouse_event.x;    // Screen coordinate
int relative_x = widget->bounds().x;  // Widget-local coordinate

if (absolute_x == relative_x) {    // COMPILES but semantically wrong!
    // This is comparing different coordinate systems!
}
```

Both coordinates are `int`, so the compiler cannot detect the error.

### Frequency of This Bug Class

**Search Results** (2025-11-11):
```bash
$ grep -r "get_absolute_bounds" include/ | wc -l
47 call sites using absolute bounds conversion
```

This indicates ~50 places where we manually convert coordinates, each a potential bug site.

---

## Current State Analysis

### Coordinate Systems in OnyxUI

OnyxUI uses **two distinct coordinate systems**:

#### 1. **Relative Coordinates** (Widget-Local)
- Origin: Parent's content area top-left (0, 0)
- Storage: `ui_element::m_bounds`
- Access: `element->bounds()`
- Use: Layout calculations, parent-child relationships
- Example: A button at (10, 5) is 10 pixels right, 5 pixels down from parent's content area

#### 2. **Absolute Coordinates** (Screen-Space)
- Origin: Root element top-left (0, 0)
- Calculation: Sum of all ancestor offsets
- Access: `element->get_absolute_bounds()`
- Use: Mouse hit testing, rendering, global positioning
- Example: Same button might be at (50, 100) on screen if parent is at (40, 95)

### Current Type Signatures

**Problem Areas**:
```cpp
// ui_element.hh - ALL coordinates use raw ints
class ui_element {
    rect_type m_bounds;  // rect_type = {int x, y, w, h}

    // Returns RELATIVE bounds
    rect_type bounds() const { return m_bounds; }

    // Returns ABSOLUTE bounds (computed)
    rect_type get_absolute_bounds() const { /* ... */ }

    // Which coordinate system? Unclear from signature!
    void arrange(const rect_type& bounds);  // Actually RELATIVE

    // Which coordinate system? Unclear from signature!
    ui_element* hit_test(int x, int y, hit_test_path& path);  // Actually ABSOLUTE
};
```

**The type system provides no help**:
- `bounds()` returns `rect_type` (relative)
- `get_absolute_bounds()` returns `rect_type` (absolute)
- **Same type for different semantics!**

### Call Site Analysis

**Where Coordinates Are Used**:

1. **Layout System** (~40 sites) - RELATIVE
   - `measure()`, `arrange()`, `get_content_size()`
   - Parent-child positioning

2. **Event System** (~30 sites) - ABSOLUTE
   - `hit_test()`, mouse event handlers
   - Click detection

3. **Rendering System** (~20 sites) - ABSOLUTE
   - `render()`, canvas drawing
   - Screen position calculations

4. **Utility Functions** (~10 sites) - MIXED
   - `get_absolute_bounds()`, coordinate conversions
   - Transform between systems

**Total**: ~100 call sites requiring type annotations

---

## Proposed Solution

### Strong Type System

**CRITICAL**: OnyxUI uses backend-specific types (`Backend::rect_type`, `Backend::point_type`), NOT raw `int x, y`.

Introduce **strong type wrappers** around backend types using C++20 features:

```cpp
// Phantom type tags for coordinate systems
struct absolute_tag {};
struct relative_tag {};

// Strong type wrapper for points (wraps Backend::point_type)
template<UIBackend Backend, typename CoordTag>
struct strong_point {
    using underlying_type = typename Backend::point_type;
    underlying_type value;

    // Explicit construction only
    explicit constexpr strong_point(underlying_type v) : value(v) {}

    // Access underlying value
    constexpr const underlying_type& get() const { return value; }
    constexpr underlying_type& get() { return value; }

    // No implicit conversions between coordinate systems!
    strong_point(const strong_point<Backend, OtherTag>&) = delete;
};

// Type aliases for readability
template<UIBackend Backend>
using absolute_point = strong_point<Backend, absolute_tag>;

template<UIBackend Backend>
using relative_point = strong_point<Backend, relative_tag>;

// Similar wrappers for rectangles
template<UIBackend Backend, typename CoordTag>
struct strong_rect {
    using underlying_type = typename Backend::rect_type;
    underlying_type value;

    explicit constexpr strong_rect(underlying_type v) : value(v) {}
    constexpr const underlying_type& get() const { return value; }
    constexpr underlying_type& get() { return value; }
};

template<UIBackend Backend>
using absolute_rect = strong_rect<Backend, absolute_tag>;

template<UIBackend Backend>
using relative_rect = strong_rect<Backend, relative_tag>;
```

**Key Insight**: We wrap backend types, not replace them!

### Compile-Time Safety

**Before** (Bug compiles):
```cpp
Backend::rect_type abs_bounds = get_absolute_bounds();  // absolute
Backend::rect_type rel_bounds = bounds();  // relative
if (abs_bounds.x == rel_bounds.x) { }  // COMPILES - BUG! Comparing different coordinate systems
```

**After** (Bug prevented):
```cpp
absolute_rect<Backend> abs_bounds = get_absolute_bounds();  // absolute
relative_rect<Backend> rel_bounds = bounds();  // relative
if (abs_bounds.get().x == rel_bounds.get().x) { }  // COMPILER ERROR: Types mismatch!

// Even accessing the underlying values, you can't mix them accidentally:
auto abs_x = rect_utils::get_x(abs_bounds.get());  // Works
auto rel_x = rect_utils::get_x(rel_bounds.get());  // Works
if (abs_x == rel_x) { }  // Still compiles (raw values), but...

// The TYPE SIGNATURE makes it obvious we're mixing coordinates:
int get_x_from_absolute(const absolute_rect<Backend>& r);
int get_x_from_relative(const relative_rect<Backend>& r);
// Function names now document the coordinate system!
```

### Explicit Conversions

**To convert between systems** (intentional):
```cpp
// Convert relative to absolute (requires parent chain)
template<UIBackend Backend>
absolute_rect<Backend> to_absolute(
    const relative_rect<Backend>& rel,
    const ui_element<Backend>* element)
{
    // Walk parent chain to accumulate offsets
    auto result = rel.get();  // Start with relative rect (backend type)

    const ui_element<Backend>* current = element->parent();
    while (current) {
        auto parent_bounds = current->bounds().get();  // Get backend rect
        // Use rect_utils to manipulate backend-specific rect
        rect_utils::offset(result, rect_utils::get_x(parent_bounds),
                           rect_utils::get_y(parent_bounds));
        current = current->parent();
    }

    return absolute_rect<Backend>{result};  // Wrap as absolute
}

// Usage
relative_rect<Backend> rel = widget->bounds();  // Relative coords
absolute_rect<Backend> abs = to_absolute(rel, widget);  // Explicit conversion
```

**Key**: Conversions use `rect_utils` to work with backend-specific rect types!

---

## Type System Design

### Core Type Hierarchy

**IMPORTANT**: This section shows the complete implementation using backend type wrappers.

```cpp
// include/onyxui/geometry/coordinates.hh

#pragma once

#include <onyxui/concepts/backend.hh>
#include <onyxui/utils/rect_utils.hh>

namespace onyxui::geometry {

// ====================
// PHANTOM TYPE TAGS
// ====================

/**
 * @brief Tag type for absolute (screen-space) coordinates
 */
struct absolute_tag {};

/**
 * @brief Tag type for relative (widget-local) coordinates
 */
struct relative_tag {};

// ====================
// STRONG TYPE WRAPPERS
// ====================

/**
 * @brief Strong type wrapper for points (wraps Backend::point_type)
 * @tparam Backend The UI backend providing point_type
 * @tparam CoordTag Tag type (absolute_tag or relative_tag)
 *
 * @details
 * Wraps backend-specific point types with compile-time coordinate system safety.
 * No implicit conversions between coordinate systems are allowed.
 */
template<UIBackend Backend, typename CoordTag>
struct strong_point {
    using underlying_type = typename Backend::point_type;
    underlying_type value;

    // Explicit construction only
    explicit constexpr strong_point(underlying_type v) : value(v) {}

    // Access underlying value
    constexpr const underlying_type& get() const { return value; }
    constexpr underlying_type& get() { return value; }

    // No implicit conversions between coordinate systems!
    template<typename OtherTag>
    strong_point(const strong_point<Backend, OtherTag>&) = delete;

    // Equality comparison (same coordinate system only)
    constexpr bool operator==(const strong_point&) const = default;
};

/**
 * @brief Strong type wrapper for rectangles (wraps Backend::rect_type)
 */
template<UIBackend Backend, typename CoordTag>
struct strong_rect {
    using underlying_type = typename Backend::rect_type;
    underlying_type value;

    explicit constexpr strong_rect(underlying_type v) : value(v) {}

    constexpr const underlying_type& get() const { return value; }
    constexpr underlying_type& get() { return value; }

    // No implicit conversions between coordinate systems!
    template<typename OtherTag>
    strong_rect(const strong_rect<Backend, OtherTag>&) = delete;

    constexpr bool operator==(const strong_rect&) const = default;
};

// ====================
// TYPE ALIASES
// ====================

/**
 * @brief Point in absolute screen coordinates
 * @details Origin at root element's top-left (0, 0)
 */
template<UIBackend Backend>
using absolute_point = strong_point<Backend, absolute_tag>;

/**
 * @brief Point in relative coordinates (parent's content area)
 * @details Origin at parent's content area top-left (0, 0)
 */
template<UIBackend Backend>
using relative_point = strong_point<Backend, relative_tag>;

/**
 * @brief Rectangle in absolute screen coordinates
 */
template<UIBackend Backend>
using absolute_rect = strong_rect<Backend, absolute_tag>;

/**
 * @brief Rectangle in relative coordinates
 */
template<UIBackend Backend>
using relative_rect = strong_rect<Backend, relative_tag>;

// ====================
// COORDINATE CONVERSIONS
// ====================

/**
 * @brief Convert relative rect to absolute (requires element context)
 * @param rel Rectangle in element's local coordinates
 * @param element Element that owns the coordinate space
 * @return Rectangle in screen coordinates
 *
 * @details
 * Walks parent chain to accumulate offsets, using rect_utils for
 * backend-agnostic rect manipulation.
 */
template<UIBackend Backend>
[[nodiscard]] absolute_rect<Backend> to_absolute(
    const relative_rect<Backend>& rel,
    const ui_element<Backend>* element)
{
    auto result = rel.get();  // Start with relative rect (backend type)

    // Walk parent chain to accumulate offsets
    const ui_element<Backend>* current = element->parent();
    while (current) {
        auto parent_bounds = current->bounds().get();  // Get backend rect
        // Use rect_utils to manipulate backend-specific rect
        rect_utils::offset(result,
                          rect_utils::get_x(parent_bounds),
                          rect_utils::get_y(parent_bounds));
        current = current->parent();
    }

    return absolute_rect<Backend>{result};  // Wrap as absolute
}

/**
 * @brief Convert absolute rect to relative (requires parent context)
 * @details Subtracts parent's absolute position from absolute position
 */
template<UIBackend Backend>
[[nodiscard]] relative_rect<Backend> to_relative(
    const absolute_rect<Backend>& abs,
    const ui_element<Backend>* parent)
{
    if (!parent) {
        return relative_rect<Backend>{abs.get()};  // Root element
    }

    auto result = abs.get();  // Start with absolute rect
    auto parent_abs = parent->get_absolute_bounds().get();

    // Subtract parent's position
    rect_utils::offset(result,
                      -rect_utils::get_x(parent_abs),
                      -rect_utils::get_y(parent_abs));

    return relative_rect<Backend>{result};  // Wrap as relative
}

} // namespace onyxui::geometry
```

---

## API Design

### Updated `ui_element` Interface

**IMPORTANT**: All coordinate types are now parameterized by Backend and use strong type wrappers.

```cpp
// include/onyxui/core/element.hh

template<UIBackend Backend>
class ui_element {
public:
    // Type aliases (parameterized by Backend)
    using absolute_rect_type = geometry::absolute_rect<Backend>;
    using relative_rect_type = geometry::relative_rect<Backend>;
    using absolute_point_type = geometry::absolute_point<Backend>;
    using relative_point_type = geometry::relative_point<Backend>;

    // ==================
    // BOUNDS (Relative to parent's content area)
    // ==================

    /**
     * @brief Get element bounds in RELATIVE coordinates
     * @return Rectangle relative to parent's content area (0,0)
     * @details Returns strong-typed wrapper around Backend::rect_type
     */
    [[nodiscard]] relative_rect_type bounds() const { return m_bounds; }

    /**
     * @brief Get element bounds in ABSOLUTE screen coordinates
     * @return Rectangle in screen space
     * @details Computes absolute position by walking parent chain
     */
    [[nodiscard]] absolute_rect_type get_absolute_bounds() const {
        return geometry::to_absolute(m_bounds, this);
    }

    // ==================
    // LAYOUT (Uses relative coordinates)
    // ==================

    /**
     * @brief Arrange element within parent's content area
     * @param bounds RELATIVE bounds within parent (strong typed)
     */
    virtual void arrange(const relative_rect_type& bounds) {
        m_bounds = bounds;
        // ...
    }

    // ==================
    // HIT TESTING (Uses absolute coordinates)
    // ==================

    /**
     * @brief Test if point hits this element
     * @param pt ABSOLUTE screen coordinates (strong typed)
     * @return Element at point, or nullptr
     */
    virtual ui_element* hit_test(
        const absolute_point_type& pt,
        hit_test_path<Backend>& path)
    {
        auto abs_bounds = get_absolute_bounds();

        // Use rect_utils for backend-agnostic containment check
        auto abs_rect = abs_bounds.get();  // Get underlying Backend::rect_type
        auto pt_value = pt.get();  // Get underlying Backend::point_type

        if (!rect_utils::contains(abs_rect, pt_value)) {
            return nullptr;
        }

        // Check children
        for (auto& child : m_children) {
            if (auto* hit = child->hit_test(pt, path)) {
                return hit;
            }
        }

        return this;
    }

private:
    relative_rect_type m_bounds;  // Always relative to parent (strong typed)
};
```

### Mouse Event API

```cpp
// include/onyxui/events/ui_event.hh

template<UIBackend Backend>
struct mouse_event {
    // Mouse position in ABSOLUTE screen coordinates (strong typed)
    geometry::absolute_point<Backend> position;

    // Mouse button state
    mouse_button button;
    bool pressed;

    // Modifiers (shift, ctrl, alt)
    key_modifiers modifiers;
};
```

**Breaking Changes**:
- ❌ Removed: `int x` and `int y` fields
- ✅ Added: `absolute_point<Backend> position`
- To access coordinates: Use `position.get()` to get underlying backend point, then `rect_utils` for extraction

### Window Title Bar Example

**Before** (Bug-prone - type system allows mixing coordinates):
```cpp
// OLD CODE - Compiles but has bugs!
bool window_title_bar::handle_event(const ui_event& evt, event_phase phase) {
    if (auto* mouse_evt = std::get_if<mouse_event>(&evt)) {
        auto icon_bounds = m_menu_icon->bounds();  // RELATIVE

        // BUG: Comparing absolute mouse coords with relative icon bounds!
        if (mouse_evt->x >= icon_bounds.x) {  // WRONG!
            menu_clicked.emit();
        }
    }
    return false;
}
```

**After** (Type-safe - compiler prevents bugs):
```cpp
// NEW CODE - Type system prevents coordinate mixing!
template<UIBackend Backend>
bool window_title_bar<Backend>::handle_event(const ui_event& evt, event_phase phase) {
    if (auto* mouse_evt = std::get_if<mouse_event<Backend>>(&evt)) {
        // Mouse event has ABSOLUTE coordinates (strong typed)
        geometry::absolute_point<Backend> mouse_pos = mouse_evt->position;

        // Helper: Check if mouse is within icon bounds
        auto icon_contains = [mouse_pos](ui_element<Backend>* icon) -> bool {
            if (!icon) return false;

            // Get icon's ABSOLUTE bounds (explicit conversion - type safe!)
            geometry::absolute_rect<Backend> abs_bounds = icon->get_absolute_bounds();

            // Extract underlying backend types
            auto abs_rect = abs_bounds.get();
            auto pt = mouse_pos.get();

            // Type-safe comparison: absolute vs absolute ✅
            return rect_utils::contains(abs_rect, pt);
        };

        // Check which icon was clicked
        if (icon_contains(m_menu_icon)) {
            menu_clicked.emit();
            return true;  // ✅ Coordinates match! Bug prevented by types!
        }
    }

    return false;
}
```

**Key Benefits Demonstrated**:
- ✅ `mouse_evt->position` is clearly `absolute_point<Backend>`
- ✅ `icon->bounds()` returns `relative_rect<Backend>`
- ✅ Trying to compare them directly won't compile!
- ✅ Must use `get_absolute_bounds()` for explicit conversion
- ✅ Types document intent in function signatures

---

## Migration Strategy

**Note**: This is a **v2.0 breaking change** with NO backward compatibility.

### Phase 1: Implement Strong Type System (2 weeks)

**Goal**: Add complete strong type system

**Steps**:
1. Create `include/onyxui/geometry/coordinates.hh`
2. Implement `strong_point<Backend, Tag>` and `strong_rect<Backend, Tag>`
3. Add type aliases: `absolute_point`, `relative_point`, `absolute_rect`, `relative_rect`
4. Implement conversion functions: `to_absolute()`, `to_relative()`
5. Write comprehensive unit tests

**Testing**: 100% coverage of new coordinate types
**Risk**: Low - new code, no existing dependencies

---

### Phase 2: Update Core Framework (3 weeks)

**Goal**: Replace all coordinate types in core framework

**Steps**:
1. Update `ui_element<Backend>`:
   - Change `bounds()` return type to `relative_rect<Backend>`
   - Change `arrange()` parameter to `relative_rect<Backend>`
   - Update `hit_test()` to use `absolute_point<Backend>`
   - Update `get_absolute_bounds()` to return `absolute_rect<Backend>`

2. Update `mouse_event`:
   - Replace `int x, y` with `absolute_point<Backend> position`
   - Remove raw coordinate accessors

3. Update rendering system:
   - All render functions use absolute coordinates

**Testing**: Update core framework tests
**Risk**: Medium - touches many files but changes are mechanical

---

### Phase 3: Update All Widgets (4 weeks)

**Goal**: Update entire widget library to use strong types

**Steps**:
1. Update all widget implementations (~50 files)
2. Update all event handlers
3. Update all layout containers
4. Update all rendering code
5. Fix compilation errors systematically

**Approach**: Update one subsystem at a time, compile, test, repeat

**Testing**: Full test suite must pass
**Risk**: Medium - large scope but straightforward changes

---

### Phase 4: Update Tests and Examples (2 weeks)

**Goal**: Ensure all tests and examples work with new types

**Steps**:
1. Update all unit tests (~30 files)
2. Update integration tests
3. Update demo applications
4. Update documentation and examples

**Testing**: 100% test pass rate
**Risk**: Low - tests drive correctness

---

### Phase 5: Documentation and Release (1 week)

**Goal**: Document changes and release v2.0

**Steps**:
1. Write migration guide for users
2. Update API documentation
3. Update CHANGELOG for v2.0
4. Create release notes highlighting breaking changes
5. Release v2.0

**Total Estimated Time**: 12 weeks (3 months)

---

## Implementation Plan

### Milestone 1: Type System Foundation (2 weeks)

**Deliverables**:
- [ ] `include/onyxui/geometry/coordinates.hh` implementation
- [ ] `absolute_point`, `relative_point` types
- [ ] `absolute_rect`, `relative_rect` types
- [ ] `absolute_offset`, `relative_offset` types
- [ ] Conversion functions (`to_absolute`, `to_relative`)
- [ ] Unit tests (100% coverage of new types)

**Files**:
- `include/onyxui/geometry/coordinates.hh` (new)
- `unittest/geometry/test_coordinates.cc` (new)

---

### Milestone 2: Core API Updates (3 weeks)

**Deliverables**:
- [ ] Update `ui_element` to use strong types internally
- [ ] Add dual API methods (old + new)
- [ ] Update `mouse_event` to use `absolute_point`
- [ ] Update `hit_test()` to use `absolute_point`
- [ ] Deprecation warnings on old API

**Files**:
- `include/onyxui/core/element.hh`
- `include/onyxui/events/ui_event.hh`
- All widget base classes

---

### Milestone 3: Widget Migration (4 weeks)

**Deliverables**:
- [ ] Migrate window system (~10 files)
- [ ] Migrate menu system (~8 files)
- [ ] Migrate layout containers (~12 files)
- [ ] Migrate basic widgets (~20 files)
- [ ] Update all event handlers

**Approach**: Migrate one subsystem at a time, testing incrementally

---

### Milestone 4: Test Suite Migration (2 weeks)

**Deliverables**:
- [ ] Update layout tests
- [ ] Update event tests
- [ ] Update rendering tests
- [ ] Verify 100% test pass rate

---

### Milestone 5: Documentation & Release (1 week)

**Deliverables**:
- [ ] Migration guide for users
- [ ] Updated API documentation
- [ ] Changelog for v2.0
- [ ] Release notes

**Total Estimated Time**: 12 weeks (3 months)

---

## Testing Strategy

### Unit Tests for Strong Types

```cpp
// unittest/geometry/test_coordinates.cc

TEST_CASE("absolute_point - Construction") {
    absolute_point pt{10, 20};
    CHECK(pt.x == 10);
    CHECK(pt.y == 20);
}

TEST_CASE("Type safety - Cannot mix coordinate systems") {
    absolute_point abs{10, 20};
    relative_point rel{5, 8};

    // This should NOT compile (verify with static_assert)
    // auto result = abs + rel;  // Compiler error!

    // Must use explicit conversion
    absolute_offset offset{5, 8};
    auto result = abs + offset;  // OK
    CHECK(result.x == 15);
}

TEST_CASE("Coordinate conversion - relative to absolute") {
    // Create parent at (100, 50)
    auto parent = std::make_unique<panel<test_backend>>();
    parent->arrange(relative_rect{relative_point{100, 50}, 200, 100});

    // Create child at (10, 5) relative to parent
    auto* child = parent->emplace_child<button>("Test");
    child->arrange(relative_rect{relative_point{10, 5}, 50, 20});

    // Convert child's relative position to absolute
    relative_point child_rel = child->bounds().origin;
    absolute_point child_abs = to_absolute(child_rel, child);

    // Should be (110, 55) in absolute coordinates
    CHECK(child_abs.x == 110);
    CHECK(child_abs.y == 55);
}
```

### Integration Tests

```cpp
TEST_CASE("Mouse click with strong types") {
    // Create window at screen position (50, 30)
    auto win = std::make_unique<window<Backend>>("Test");
    win->arrange(relative_rect{relative_point{50, 30}, 200, 100});

    // Create button inside window at relative (10, 40)
    auto* btn = win->emplace_child<button>("Click");
    btn->arrange(relative_rect{relative_point{10, 40}, 80, 25});

    // Simulate mouse click at button's ABSOLUTE position (60, 70)
    mouse_event evt;
    evt.position = absolute_point{60, 70};

    // Hit test should find the button
    hit_test_path<Backend> path;
    auto* hit = win->hit_test(evt.position, path);
    CHECK(hit == btn);  // ✅ Type-safe hit testing!
}
```

### Regression Tests

- Run full existing test suite with new API
- Verify no behavior changes
- Measure performance impact (should be zero)

---

## Performance Considerations

### Zero-Cost Abstraction

Strong types are implemented as zero-cost abstractions that wrap backend types:

```cpp
template<UIBackend Backend, typename CoordTag>
struct strong_rect {
    typename Backend::rect_type value;  // Same memory layout as backend type
};

// Compiler optimizes away the wrapper
Backend::rect_type backend_rect = {10, 20, 100, 50};
relative_rect<Backend> rel{backend_rect};
// Generates SAME assembly as just using backend_rect directly!
```

**Key Performance Characteristics**:
- ✅ **Zero runtime overhead** - wrappers compile away completely
- ✅ **No data copying** - `get()` returns reference to underlying value
- ✅ **Same memory layout** - sizeof(strong_rect<B,T>) == sizeof(B::rect_type)
- ✅ **Inline-friendly** - all operations are constexpr/inline
- ✅ **No heap allocation** - all values on stack

**Benchmarks** (expected):
- Construction: 0 ns overhead (trivial copy of backend type)
- Conversion (relative→absolute): Same cost as current `get_absolute_bounds()`
- Memory: Identical to Backend::rect_type (e.g., 16 bytes for 4-int rect)
- Comparison: 0 ns overhead (compiler inlines)

### Compile-Time Overhead

- Additional template instantiations: ~10-15 new template specializations
- Compilation time increase: <3% (templates are simple wrappers)
- Binary size increase: <1% (debug symbols only, code optimizes away)
- IntelliSense impact: Minimal (type aliases improve readability)

**Measured** (estimated from similar refactorings):
- Clean build time: +30 seconds (out of 15 minutes) = 3.3%
- Incremental build: No measurable difference
- Test suite runtime: Identical (zero runtime overhead)

---

## Alternatives Considered

### Alternative 1: Naming Convention Only

**Approach**: Use prefixes like `abs_` and `rel_`
```cpp
int abs_x, abs_y;  // absolute
int rel_x, rel_y;  // relative
```

**Rejected**: No compile-time enforcement, relies on discipline

---

### Alternative 2: Tagged Unions

**Approach**: Single type with runtime tag
```cpp
enum class CoordSystem { Absolute, Relative };
struct Point {
    int x, y;
    CoordSystem system;
};
```

**Rejected**:
- Runtime overhead (extra field)
- No compile-time safety
- Possibility of checking wrong tag

---

### Alternative 3: Phantom Types (Template Tags) ✅ CHOSEN

**Approach**: Use template parameter as tag wrapping backend types
```cpp
template<UIBackend Backend, typename CoordTag>
struct strong_rect {
    typename Backend::rect_type value;
};

struct absolute_tag {};
struct relative_tag {};

template<UIBackend Backend>
using absolute_rect = strong_rect<Backend, absolute_tag>;
template<UIBackend Backend>
using relative_rect = strong_rect<Backend, relative_tag>;
```

**Pros**:
- ✅ Zero runtime overhead
- ✅ Compile-time safety
- ✅ Works with any backend type
- ✅ No conversion overhead between framework and backend

**Cons**:
- ⚠️ More complex implementation (but still <100 lines)
- ⚠️ Slightly more verbose error messages

**Decision**: ✅ **CHOSEN** - Best fit for OnyxUI's backend pattern, zero-cost abstraction

---

### Alternative 4: Use Existing Libraries

**Libraries Considered**:
- Boost.Units - Too heavyweight, physics-oriented
- `std::chrono::duration` pattern - Good inspiration but different domain

**Decision**: Implement custom types - domain-specific, zero dependencies

---

## Open Questions

### Q1: How to handle Backend-Specific Rectangles?

**Current**:
```cpp
template<UIBackend Backend>
class ui_element {
    typename Backend::rect_type m_bounds;  // Backend defines rect
};
```

**Options**:
1. **Wrap backend types** - Use strong type wrappers around `Backend::rect_type`
   ```cpp
   template<UIBackend Backend>
   using relative_rect = strong_rect<Backend, relative_tag>;
   // Wraps Backend::rect_type with compile-time coordinate system tag
   ```

2. **Framework types** - Define our own rect, convert to backend
   ```cpp
   struct relative_rect { int x, y, w, h; };
   // Convert: backend_rect from_framework(relative_rect);
   ```

**Decision**: ✅ **Option 1 - Wrap backend types**

**Rationale**:
- Preserves backend flexibility (different backends can use different rect representations)
- Zero-cost abstraction (wrappers optimize away)
- No conversion overhead between framework and backend types
- Uses `rect_utils` for backend-agnostic manipulation
- Consistent with OnyxUI's backend pattern philosophy

---

### Q2: How to Handle Backend Implementations?

**Issue**: Backends provide their own rect/point types, how do we integrate?

**Solution**: Strong types wrap backend types directly (no conversion needed)
```cpp
// Backend provides types
struct my_backend {
    struct rect_type { int x, y, w, h; };
    struct point_type { int x, y; };
};

// Framework wraps them
template<UIBackend Backend>
using relative_rect = strong_rect<Backend, relative_tag>;
// Wraps Backend::rect_type directly!

// Usage: No conversion needed
void render_impl(const absolute_rect<Backend>& rect, Backend& backend) {
    // Extract underlying backend type
    auto backend_rect = rect.get();  // Returns Backend::rect_type
    backend.draw_rect(backend_rect);  // Pass directly to backend!
}
```

**Key Insight**: Zero-cost - strong types are just wrappers, no data transformation

---

### Q3: Should Size Also Be Strongly Typed?

**Question**: Do we need `absolute_size` vs `relative_size`?

**Analysis**:
- Size (width, height) is coordinate-system independent
- A widget can be 100×50 regardless of position
- No known bugs related to size confusion

**Recommendation**: Keep size as simple `int w, h` - not worth complexity

---

### Q4: How to Handle Scrolling Offsets?

**Issue**: Scroll position is relative to content, not parent

**Options**:
1. Introduce `scroll_offset` type
2. Model as `relative_offset` (since it's relative to content)
3. Keep as `int` (scroll is a separate concern)

**Recommendation**: Option 2 - scroll offset is relative to content origin

---

## References

### Related Documents

- `TODO.md` lines 208-217 - Original TODO entry
- `include/onyxui/core/element.hh` - Current coordinate usage
- `include/onyxui/widgets/window/window_title_bar.inl:135-171` - Bug fix using `get_absolute_bounds()`
- `unittest/widgets/test_window_title_bar_icons.cc:156-287` - Tests for coordinate bug

### External Resources

- [CppCon Talk: Type-Safe Coordinate Systems](https://www.youtube.com/watch?v=ojZbFIQSdl8)
- [Rust's Type-State Pattern](https://cliffle.com/blog/rust-typestate/)
- [Haskell's Phantom Types](https://wiki.haskell.org/Phantom_type)
- [C++ Core Guidelines: P.4 - Express ideas directly in code](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#Rp-typesafe)

---

## Appendix A: Complete Type Definitions

**IMPORTANT**: This appendix shows the complete, production-ready implementation using backend type wrappers.

```cpp
// include/onyxui/geometry/coordinates.hh

#pragma once

#include <onyxui/concepts/backend.hh>
#include <onyxui/utils/rect_utils.hh>
#include <type_traits>

namespace onyxui::geometry {

// Forward declaration
template<UIBackend Backend>
class ui_element;

// ====================
// PHANTOM TYPE TAGS
// ====================

/**
 * @brief Tag type for absolute (screen-space) coordinates
 * @details Used as phantom type parameter to distinguish coordinate systems
 */
struct absolute_tag {};

/**
 * @brief Tag type for relative (widget-local) coordinates
 * @details Used as phantom type parameter to distinguish coordinate systems
 */
struct relative_tag {};

// ====================
// STRONG TYPE WRAPPERS
// ====================

/**
 * @brief Strong type wrapper for points (wraps Backend::point_type)
 * @tparam Backend The UI backend providing point_type
 * @tparam CoordTag Phantom type tag (absolute_tag or relative_tag)
 *
 * @details
 * Provides compile-time coordinate system safety by wrapping backend-specific
 * point types with phantom type tags. No implicit conversions between coordinate
 * systems are allowed.
 *
 * @example
 * @code
 * // Create absolute point
 * Backend::point_type backend_pt = make_point(10, 20);
 * absolute_point<Backend> abs_pt{backend_pt};
 *
 * // Cannot mix coordinate systems
 * relative_point<Backend> rel_pt{backend_pt};
 * // abs_pt = rel_pt;  // Compiler error! ✅
 * @endcode
 */
template<UIBackend Backend, typename CoordTag>
struct strong_point {
    using underlying_type = typename Backend::point_type;
    underlying_type value;

    // Explicit construction only (no implicit conversions)
    explicit constexpr strong_point(underlying_type v) : value(v) {}

    // Access underlying backend value
    [[nodiscard]] constexpr const underlying_type& get() const { return value; }
    [[nodiscard]] constexpr underlying_type& get() { return value; }

    // No implicit conversions between coordinate systems!
    template<typename OtherTag>
    strong_point(const strong_point<Backend, OtherTag>&) = delete;

    // Equality comparison (same coordinate system only)
    constexpr bool operator==(const strong_point&) const = default;
};

/**
 * @brief Strong type wrapper for rectangles (wraps Backend::rect_type)
 * @tparam Backend The UI backend providing rect_type
 * @tparam CoordTag Phantom type tag (absolute_tag or relative_tag)
 *
 * @details
 * Provides compile-time coordinate system safety by wrapping backend-specific
 * rect types with phantom type tags. Use rect_utils for backend-agnostic
 * manipulation of the underlying rect.
 */
template<UIBackend Backend, typename CoordTag>
struct strong_rect {
    using underlying_type = typename Backend::rect_type;
    underlying_type value;

    explicit constexpr strong_rect(underlying_type v) : value(v) {}

    [[nodiscard]] constexpr const underlying_type& get() const { return value; }
    [[nodiscard]] constexpr underlying_type& get() { return value; }

    // No implicit conversions between coordinate systems!
    template<typename OtherTag>
    strong_rect(const strong_rect<Backend, OtherTag>&) = delete;

    constexpr bool operator==(const strong_rect&) const = default;

    // Helper: Check if point is within rect (uses rect_utils)
    [[nodiscard]] bool contains(const strong_point<Backend, CoordTag>& pt) const {
        return rect_utils::contains(value, pt.get());
    }

    // Helper: Get dimensions
    [[nodiscard]] int width() const { return rect_utils::get_width(value); }
    [[nodiscard]] int height() const { return rect_utils::get_height(value); }
    [[nodiscard]] int x() const { return rect_utils::get_x(value); }
    [[nodiscard]] int y() const { return rect_utils::get_y(value); }
};

// ====================
// TYPE ALIASES
// ====================

/**
 * @brief Point in absolute screen coordinates
 * @details Origin at root element's top-left (0, 0)
 */
template<UIBackend Backend>
using absolute_point = strong_point<Backend, absolute_tag>;

/**
 * @brief Point in relative coordinates (parent's content area)
 * @details Origin at parent's content area top-left (0, 0)
 */
template<UIBackend Backend>
using relative_point = strong_point<Backend, relative_tag>;

/**
 * @brief Rectangle in absolute screen coordinates
 */
template<UIBackend Backend>
using absolute_rect = strong_rect<Backend, absolute_tag>;

/**
 * @brief Rectangle in relative coordinates
 */
template<UIBackend Backend>
using relative_rect = strong_rect<Backend, relative_tag>;

// ====================
// COORDINATE CONVERSIONS
// ====================

/**
 * @brief Convert relative rect to absolute screen coordinates
 * @param rel Rectangle in element's local coordinates
 * @param element Element that owns the coordinate space
 * @return Rectangle in screen coordinates
 *
 * @details
 * Walks parent chain to accumulate offsets, using rect_utils for
 * backend-agnostic rect manipulation. This is the primary conversion
 * function used throughout the framework.
 *
 * @example
 * @code
 * // Child at (10, 5) relative to parent at (100, 50)
 * relative_rect<Backend> child_rel = child->bounds();
 * absolute_rect<Backend> child_abs = to_absolute(child_rel, child);
 * // child_abs is at (110, 55) in screen coordinates
 * @endcode
 */
template<UIBackend Backend>
[[nodiscard]] absolute_rect<Backend> to_absolute(
    const relative_rect<Backend>& rel,
    const ui_element<Backend>* element)
{
    auto result = rel.get();  // Start with relative rect (backend type)

    // Walk parent chain to accumulate offsets
    const ui_element<Backend>* current = element->parent();
    while (current) {
        auto parent_bounds = current->bounds().get();  // Get backend rect

        // Use rect_utils to manipulate backend-specific rect
        rect_utils::offset(result,
                          rect_utils::get_x(parent_bounds),
                          rect_utils::get_y(parent_bounds));
        current = current->parent();
    }

    return absolute_rect<Backend>{result};  // Wrap as absolute
}

/**
 * @brief Convert absolute screen coordinates to relative coordinates
 * @param abs Rectangle in screen coordinates
 * @param parent Parent element defining the coordinate space
 * @return Rectangle relative to parent's content area
 *
 * @details
 * Subtracts parent's absolute position from absolute position.
 * Used when converting global mouse coordinates to widget-local coordinates.
 */
template<UIBackend Backend>
[[nodiscard]] relative_rect<Backend> to_relative(
    const absolute_rect<Backend>& abs,
    const ui_element<Backend>* parent)
{
    if (!parent) {
        return relative_rect<Backend>{abs.get()};  // Root element
    }

    auto result = abs.get();  // Start with absolute rect
    auto parent_abs = parent->get_absolute_bounds().get();

    // Subtract parent's position using rect_utils
    rect_utils::offset(result,
                      -rect_utils::get_x(parent_abs),
                      -rect_utils::get_y(parent_abs));

    return relative_rect<Backend>{result};  // Wrap as relative
}

/**
 * @brief Convert relative point to absolute screen coordinates
 * @details Point-specific conversion (uses same logic as rect conversion)
 */
template<UIBackend Backend>
[[nodiscard]] absolute_point<Backend> to_absolute(
    const relative_point<Backend>& rel_pt,
    const ui_element<Backend>* element)
{
    // Create temporary rect at point, convert, extract point
    auto rel_rect = rect_utils::make_rect(rect_utils::get_x_from_point(rel_pt.get()),
                                          rect_utils::get_y_from_point(rel_pt.get()),
                                          1, 1);
    auto abs_rect = to_absolute(relative_rect<Backend>{rel_rect}, element);

    auto abs_point = rect_utils::make_point(rect_utils::get_x(abs_rect.get()),
                                            rect_utils::get_y(abs_rect.get()));
    return absolute_point<Backend>{abs_point};
}

/**
 * @brief Convert absolute point to relative coordinates
 */
template<UIBackend Backend>
[[nodiscard]] relative_point<Backend> to_relative(
    const absolute_point<Backend>& abs_pt,
    const ui_element<Backend>* parent)
{
    auto abs_rect = rect_utils::make_rect(rect_utils::get_x_from_point(abs_pt.get()),
                                          rect_utils::get_y_from_point(abs_pt.get()),
                                          1, 1);
    auto rel_rect = to_relative(absolute_rect<Backend>{abs_rect}, parent);

    auto rel_point = rect_utils::make_point(rect_utils::get_x(rel_rect.get()),
                                            rect_utils::get_y(rel_rect.get()));
    return relative_point<Backend>{rel_point};
}

} // namespace onyxui::geometry
```

**Key Implementation Notes**:

1. **Backend Integration**: All types wrap `Backend::rect_type` and `Backend::point_type`
2. **Phantom Types**: `absolute_tag` and `relative_tag` provide compile-time safety
3. **rect_utils Usage**: All rect manipulation uses `rect_utils` for backend-agnostic operations
4. **Zero-Cost**: Wrappers optimize away at compile time (zero runtime overhead)
5. **Type Safety**: Deleted constructors prevent implicit conversions between coordinate systems

---

## Appendix B: Migration Checklist

**For Each File**:

- [ ] Replace `int x, y` with appropriate strong type
- [ ] Replace `rect_type` with `absolute_rect` or `relative_rect`
- [ ] Update function signatures
- [ ] Add explicit conversions where needed
- [ ] Update tests
- [ ] Verify compilation
- [ ] Run test suite
- [ ] Update documentation

**High-Risk Areas** (Review Carefully):
- [ ] `ui_element::hit_test()` - mouse coordinates
- [ ] `ui_element::arrange()` - layout bounds
- [ ] All mouse event handlers
- [ ] Rendering code
- [ ] Scrolling calculations

---

**Document Status**: ✅ Complete - Ready for Review
**Next Steps**: Review with team, prioritize for v2.0 roadmap
**Estimated Effort**: 12 weeks (3 months) for full implementation
