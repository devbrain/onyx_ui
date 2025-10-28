# Style Resolution Refactoring - Progress Document

**Date Started:** 2025-10-27
**Status:** 95% Complete - Core refactoring done, 2 test files need minor fixes
**Goal:** Eliminate O(depth) recursion in style resolution by passing parent_style top-down

---

## Table of Contents

1. [Problem Statement](#problem-statement)
2. [Solution Approach](#solution-approach)
3. [Completed Work](#completed-work)
4. [Remaining Work](#remaining-work)
5. [Technical Details](#technical-details)
6. [Testing Status](#testing-status)

---

## Problem Statement

### Original Issue

The old style resolution system had **O(depth) recursion** - each widget would call:
- `get_effective_background_color()` → walks up tree
- `get_effective_foreground_color()` → walks up tree
- `get_effective_box_style()` → walks up tree
- `get_effective_font()` → walks up tree
- `get_effective_icon_style()` → walks up tree

**Result:** For a widget at depth 100, we'd walk the tree 500 times (5 properties × 100 levels).

### The Goal

**Zero-recursion CSS-style inheritance:**
- Style resolved **ONCE** at root
- Parent passes `resolved_style` down to children (O(1) per level)
- Total cost: **O(tree_size)** instead of O(tree_size × properties × depth)

---

## Solution Approach

### 1. Strong Type Wrappers (Compile-Time Safety)

Added strong types to `resolved_style` to enforce explicit initialization:

```cpp
struct background_color_t {
    color_type value;
    background_color_t() = delete;  // Must initialize!
    background_color_t(color_type c) : value(c) {}
    operator const color_type&() const { return value; }  // Implicit conversion
};
```

**Benefits:**
- Compiler error if you forget to initialize a field
- Prevents swapping `background_color` with `foreground_color`
- Natural syntax via implicit conversion operators

### 2. Single Virtual Method Pattern

**Before (5 virtual methods):**
```cpp
virtual color_type get_theme_background_color(const theme&);
virtual color_type get_theme_foreground_color(const theme&);
virtual box_style get_theme_box_style(const theme&);
virtual font get_theme_font(const theme&);
virtual icon_style get_theme_icon_style(const theme&);
```

**After (1 virtual method):**
```cpp
virtual resolved_style<Backend> get_theme_style(const theme&);
```

**Benefits:**
- Less code duplication
- Single source of truth
- State-aware color resolution happens once

### 3. Top-Down Style Passing

**Before (recursive):**
```cpp
void render(renderer& r) {
    auto bg = get_effective_background_color();  // Walks up tree!
    auto fg = get_effective_foreground_color();  // Walks up tree!
    // ... render widget
    for (auto& child : children) {
        child->render(r);  // Child repeats same tree walk
    }
}
```

**After (iterative):**
```cpp
void render(renderer& r, const resolved_style& parent_style) {
    auto style = resolve_style(theme, parent_style);  // O(1) - just apply overrides
    draw_context ctx(r, style);
    do_render(ctx);  // Widget uses pre-resolved style

    for (auto& child : children) {
        child->render(r, style);  // Pass MY style down
    }
}
```

---

## Completed Work

### Core Architecture ✅

1. **resolved_style.hh** - Strong type wrappers with implicit conversions
   - ✅ Added `background_color_t`, `foreground_color_t`, `border_color_t`
   - ✅ Added `box_style_t`, `font_t`, `opacity_t`, `icon_style_t`
   - ✅ Implicit conversion operators for natural syntax
   - ✅ `from_theme()` factory method
   - ✅ `with_opacity()`, `with_colors()`, `with_font()` utility methods

2. **themeable.hh** - Zero-recursion style resolution
   - ✅ Removed 5 old virtual methods
   - ✅ Added single `get_theme_style()` virtual method
   - ✅ Rewrote `resolve_style(theme, parent_style)` - no recursion!
   - ✅ Added public `resolve_style()` convenience overload for tests
   - ✅ Fixed `.value` access in inheritance logic

3. **element.hh** - Top-down rendering
   - ✅ Updated `render()` to pass `parent_style` down the tree
   - ✅ Public `render(renderer)` creates root style from theme
   - ✅ Protected `render(renderer, dirty_regions, theme, parent_style)` for recursion

### Widget Updates ✅

4. **button.hh**
   - ✅ Replaced old methods with `get_theme_style()`
   - ✅ Uses state-aware colors (normal/hover/pressed/disabled)
   - ✅ Removed `.value` accesses (uses implicit conversion)

5. **label.hh**
   - ✅ Replaced old methods with `get_theme_style()`
   - ✅ Removed `.value` accesses

6. **panel.hh**
   - ✅ Replaced old methods with `get_theme_style()`

7. **menu_item.hh**
   - ✅ Replaced old methods with `get_theme_style()`
   - ✅ Fixed to use `theme.menu.box_style` (not menu_item.box_style)
   - ✅ Fixed to use `visual_state.font` (not menu_item.font)

8. **menu_bar_item.hh**
   - ✅ Replaced old methods with `get_theme_style()`
   - ✅ Uses state-aware colors (normal/hover/open)

### Supporting Code ✅

9. **widget_container.hh**
   - ✅ Removed `.value` from `ctx.style().box_style`

10. **draw_context.hh**
    - ✅ Removed `.value` from `draw_rect()` and `fill_rect()`

11. **render_context.hh**
    - ✅ Fixed default constructor to use designated initializers

12. **test_render_context.cc**
    - ✅ Fixed all designated initializers to use proper type constructors
    - ✅ Added `.value` where needed for strong type access

---

## Remaining Work

### Test File Fixes (2 files, ~30 minutes)

#### 1. test_theme_switching.cc
**Issue:** Tests access color members directly on strong types
**Fix:** Add `.value` before accessing color members

```cpp
// BEFORE (broken)
CHECK(style.background_color.r == 100);
CHECK(style.foreground_color.g == 255);

// AFTER (fixed)
CHECK(style.background_color.value.r == 100);
CHECK(style.foreground_color.value.g == 255);
```

**Locations:** ~50 assertions throughout the file

**Approach:**
```bash
# Use sed to fix all occurrences
sed -i 's/\.background_color\.r/\.background_color.value.r/g' unittest/widgets/test_theme_switching.cc
sed -i 's/\.background_color\.g/\.background_color.value.g/g' unittest/widgets/test_theme_switching.cc
sed -i 's/\.background_color\.b/\.background_color.value.b/g' unittest/widgets/test_theme_switching.cc
sed -i 's/\.foreground_color\.r/\.foreground_color.value.r/g' unittest/widgets/test_theme_switching.cc
sed -i 's/\.foreground_color\.g/\.foreground_color.value.g/g' unittest/widgets/test_theme_switching.cc
sed -i 's/\.foreground_color\.b/\.foreground_color.value.b/g' unittest/widgets/test_theme_switching.cc
```

#### 2. test_menu_item_measurement.cc
**Issue:** Line 468 uses old render() signature
**Fix:** Update to new signature with parent_style

```cpp
// BEFORE (broken)
panel->render(renderer, dirty_regions, theme);

// AFTER (fixed)
auto parent_style = theme ? resolved_style<Backend>::from_theme(*theme)
                          : resolved_style<Backend>{...default...};
panel->render(renderer, dirty_regions, theme, parent_style);
```

### Verification Steps

After fixing test files:

1. **Full build:**
   ```bash
   cmake --build build --target ui_unittest -j8
   ```

2. **Run all tests:**
   ```bash
   ./build/bin/ui_unittest
   ```

3. **Expected result:** All 859 tests pass (or ~860+ with any new tests)

---

## Technical Details

### resolved_style Structure

```cpp
template<UIBackend Backend>
struct resolved_style {
    using color_type = typename Backend::color_type;
    using box_style_type = typename Backend::renderer_type::box_style;
    using font_type = typename Backend::renderer_type::font;
    using icon_style_type = typename Backend::renderer_type::icon_style;

    // Strong type wrappers with implicit conversion
    background_color_t background_color;  // CSS-inheritable
    foreground_color_t foreground_color;  // CSS-inheritable
    border_color_t border_color;          // Defaults to foreground
    box_style_t box_style;                // CSS-inheritable
    font_t font;                          // CSS-inheritable
    opacity_t opacity;                    // Multiplicative through tree
    icon_style_t icon_style;              // Optional, not always used

    // Factory method
    static resolved_style from_theme(const ui_theme<Backend>&) noexcept;

    // Utility methods
    [[nodiscard]] resolved_style with_opacity(float) const noexcept;
    [[nodiscard]] resolved_style with_colors(color_type bg, color_type fg) const noexcept;
    [[nodiscard]] resolved_style with_font(font_type) const noexcept;
};
```

### Style Resolution Flow

```
Root Element (depth 0)
  ├─ theme.window_bg → style.background_color
  ├─ No parent override
  └─ Pass style to children
      │
      Child Element (depth 1)
        ├─ parent_style.background_color → style.background_color
        ├─ Apply m_background_override if set
        └─ Pass style to children
            │
            Deep Child (depth 100)
              ├─ parent_style.background_color → style.background_color  [O(1)]
              ├─ Apply m_background_override if set
              └─ Done! No tree walk needed
```

### Performance Characteristics

**Before:**
- `get_effective_background()` at depth 100: **100 parent pointer dereferences**
- 5 properties × 100 levels = **500 tree walks per frame**
- Exponential with tree width (siblings repeat walks)

**After:**
- `resolve_style()` at depth 100: **1 override check + 1 assignment** = O(1)
- Total per frame: **O(tree_size)** regardless of depth
- Linear with tree size (each node visited once)

### Implicit Conversion Behavior

```cpp
// Strong type prevents mistakes
resolved_style style;
style.background_color = style.foreground_color;  // ❌ Compile error!

// But allows natural usage
void set_color(const color_type& c);
set_color(style.background_color);  // ✅ Works! Implicitly converts

// Direct member access requires .value
uint8_t r = style.background_color.value.r;  // ✅ Correct
uint8_t g = style.background_color.r;         // ❌ Compile error
```

---

## Testing Status

### Passing Tests ✅
- All core tests (element, render_context, themeable)
- All layout tests
- All widget tests except theme_switching
- All menu tests except menu_item_measurement

### Failing Tests (2 files)
1. **test_theme_switching.cc** - Color member access without `.value`
2. **test_menu_item_measurement.cc** - Old render() signature

### Build Status
- **Core library:** ✅ Compiles cleanly
- **All widgets:** ✅ Compile cleanly
- **Most tests:** ✅ Compile cleanly
- **2 test files:** ❌ Need minor fixes (see Remaining Work)

---

## Files Modified

### Header Files (Core)
- `include/onyxui/resolved_style.hh` - Strong types + conversions
- `include/onyxui/themeable.hh` - Zero-recursion resolution
- `include/onyxui/element.hh` - Top-down rendering
- `include/onyxui/render_context.hh` - Default constructor fix
- `include/onyxui/draw_context.hh` - Removed .value accesses

### Header Files (Widgets)
- `include/onyxui/widgets/button.hh` - get_theme_style()
- `include/onyxui/widgets/label.hh` - get_theme_style()
- `include/onyxui/widgets/panel.hh` - get_theme_style()
- `include/onyxui/widgets/menu_item.hh` - get_theme_style()
- `include/onyxui/widgets/menu_bar_item.hh` - get_theme_style()
- `include/onyxui/widgets/widget_container.hh` - Removed .value

### Test Files
- `unittest/core/test_render_context.cc` - Fixed initializers ✅
- `unittest/widgets/test_theme_switching.cc` - Needs .value fixes ❌
- `unittest/widgets/test_menu_item_measurement.cc` - Needs signature fix ❌

---

## Quick Reference Commands

### Build
```bash
cmake --build build --target ui_unittest -j8
```

### Test specific file
```bash
./build/bin/ui_unittest --test-case="*theme switching*"
```

### Check for errors
```bash
cmake --build build --target ui_unittest -j8 2>&1 | grep "error:"
```

### Run all tests
```bash
./build/bin/ui_unittest
```

---

## Design Rationale

### Why Strong Types?

**Problem:** Easy to forget initializing a field in resolved_style
**Solution:** Delete default constructors - compiler enforces explicit init

**Problem:** Easy to swap background/foreground colors
**Solution:** Different types prevent accidental assignment

**Problem:** Verbose `.value` everywhere
**Solution:** Implicit conversion operators for natural syntax

### Why Single Virtual Method?

**Before:**
- Widget overrides 5 methods
- Each returns one property
- State logic duplicated 5 times

**After:**
- Widget overrides 1 method
- Returns complete style
- State logic written once

**Result:** Less code, single source of truth, easier to maintain

### Why Top-Down Passing?

**Before (recursive):**
```
render() → get_effective_bg() → parent? → parent->get_effective_bg() → ...
       → get_effective_fg() → parent? → parent->get_effective_fg() → ...
       → child->render()    → [child repeats same walks]
```

**After (iterative):**
```
render(parent_style) → resolve_style(parent_style)  [O(1)]
                    → child->render(my_style)       [pass down]
```

**Result:** O(tree_size) instead of O(tree_size × properties × depth)

---

## Notes for Future Work

### If Adding New Inheritable Properties

1. Add strong type wrapper to `resolved_style`
2. Add field to `resolved_style` structure
3. Update `from_theme()` factory method
4. Add override member to `themeable` (e.g., `m_new_property_override`)
5. Update `resolve_style()` to apply inheritance and overrides
6. Update `get_theme_style()` in each widget that uses the property

### If Adding New Widget

1. Inherit from `widget<Backend>` or `stateful_widget<Backend>`
2. Override `get_theme_style(const theme&)` to return widget-specific style
3. Use `ctx.style()` in `do_render()` to access pre-resolved properties
4. No need to call `get_effective_*()` methods (they're gone!)

### Performance Monitoring

To verify O(depth) elimination:
```cpp
// Before: Count increases with depth
struct metrics { int tree_walks = 0; };

// After: Count equals tree size regardless of depth
struct metrics { int resolve_calls = 0; };
CHECK(resolve_calls == tree_size);  // Not tree_size * depth!
```

---

## Completion Checklist

- [x] Add strong types to resolved_style
- [x] Add implicit conversion operators
- [x] Replace 5 virtual methods with get_theme_style()
- [x] Rewrite resolve_style() to eliminate recursion
- [x] Update element.hh to pass parent_style down
- [x] Update button.hh
- [x] Update label.hh
- [x] Update panel.hh
- [x] Update menu_item.hh
- [x] Update menu_bar_item.hh
- [x] Remove .value from production code
- [x] Fix test_render_context.cc
- [ ] Fix test_theme_switching.cc
- [ ] Fix test_menu_item_measurement.cc
- [ ] Full build passes
- [ ] All tests pass

**Estimated time to completion:** 30 minutes

---

*Document created: 2025-10-27*
*Last updated: 2025-10-27*
*Status: Ready for final test fixes*
