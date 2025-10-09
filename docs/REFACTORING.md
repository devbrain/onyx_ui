# Onyx UI Layout System - Comprehensive Refactoring Document

**Document Version:** 1.0
**Date:** 2025-10-09
**Status:** Approved for Implementation

---

## Table of Contents

1. [Executive Summary](#executive-summary)
2. [Design Principles](#design-principles)
3. [Critical Fixes](#critical-fixes)
4. [API Design Changes](#api-design-changes)
5. [Implementation Plan](#implementation-plan)
6. [Breaking Changes](#breaking-changes)
7. [Testing Strategy](#testing-strategy)
8. [Migration Guide](#migration-guide)

---

## Executive Summary

### Current State

The onyx_ui layout library has excellent architectural design with modern C++20 features, but contains **critical access control issues** that prevent compilation. The code review identified:

- **5 CRITICAL** issues blocking compilation
- **5 HIGH** priority issues causing type safety violations
- **13 MEDIUM** severity design and correctness issues
- **12 LOW** priority improvements

### Refactoring Goals

1. **Make the library compile** by fixing access control
2. **Maintain clean encapsulation** using protected getters, not public members
3. **Ensure immutability** by making layout configuration constructor-only
4. **Preserve type safety** by using utility functions consistently
5. **Keep the elegant API** while fixing implementation details

### Timeline Estimate

- **Phase 1 (Critical):** 8-10 hours - Fix compilation issues
- **Phase 2 (High Priority):** 6-8 hours - Fix type safety and correctness
- **Phase 3 (Medium Priority):** 10-12 hours - Robustness and validation
- **Phase 4 (Polish):** 4-6 hours - Performance and const correctness
- **Total:** 28-36 hours

---

## Design Principles

### 1. Encapsulation Strategy

**Adopted Design:**
- **Private members** with protected const getters for layout strategies
- **Public inheritance** for layout classes (correct polymorphism)
- **Private override methods** in layout implementations (Strategy pattern)
- **Immutable configuration** via constructor parameters

**Rationale:**
- Prevents accidental misuse of internal state
- Layouts access only what they need via protected interface
- Configuration cannot be changed after construction (prevents cache invalidation)
- Only base class interface exposed (proper Strategy pattern)

### 2. Type Safety

**Adopted Design:**
- **Always use utility functions** (`rect_utils::`, `size_utils::`)
- **Never directly access** `.x`, `.width`, `.height` on templated types
- **Consistent naming** throughout codebase

**Rationale:**
- Maintains framework-agnostic design
- Works with any RectLike/SizeLike types
- Compiler catches type errors early

### 3. Const Correctness

**Adopted Design:**
- **Const getters** for read-only access
- **Mutable setters** with invalidation side effects
- **Const methods** for queries (hit_test, accessors)
- **noexcept** for simple getters

**Rationale:**
- Documents mutation vs. observation
- Enables use with const objects
- Prevents accidental modification

---

## Critical Fixes

### Fix #1: ui_element Protected Interface

**Problem:** Layout strategies cannot access private members.

**Solution:** Add protected const getters.

```cpp
template<RectLike TRect, SizeLike TSize>
class ui_element {
public:
    // Public API (unchanged externals)
    explicit ui_element(ui_element* parent);
    void add_child(ui_element_ptr child);
    ui_element_ptr remove_child(ui_element* child);
    TSize measure(int available_width, int available_height);
    void arrange(const TRect& final_bounds);
    void invalidate_measure();
    void invalidate_arrange();
    ui_element* hit_test(int x, int y);

    // Public setters (with side effects)
    void set_visible(bool visible) {
        if (m_visible != visible) {
            m_visible = visible;
            invalidate_arrange();
        }
    }

    void set_enabled(bool enabled) { m_enabled = enabled; }

    void set_layout_strategy(layout_strategy_ptr strategy) {
        m_layout_strategy = std::move(strategy);
        invalidate_measure();
    }

    // Public property accessors (return non-const references for modification)
    size_constraint& width_constraint() {
        invalidate_measure();
        return m_width_constraint;
    }

    size_constraint& height_constraint() {
        invalidate_measure();
        return m_height_constraint;
    }

    horizontal_alignment& horizontal_align() {
        invalidate_arrange();
        return m_h_align;
    }

    vertical_alignment& vertical_align() {
        invalidate_arrange();
        return m_v_align;
    }

    thickness& margin() {
        invalidate_measure();
        return m_margin;
    }

    thickness& padding() {
        invalidate_measure();
        return m_padding;
    }

    int& z_order() {
        invalidate_arrange();
        return m_z_index;
    }

    // Public const accessors
    const TRect& bounds() const noexcept { return m_bounds; }
    bool is_visible() const noexcept { return m_visible; }
    bool is_enabled() const noexcept { return m_enabled; }
    const size_constraint& width_constraint() const noexcept { return m_width_constraint; }
    const size_constraint& height_constraint() const noexcept { return m_height_constraint; }

protected:
    // Protected interface for layout strategies ONLY
    const std::vector<ui_element_ptr>& children() const noexcept { return m_children; }
    const TSize& last_measured_size() const noexcept { return m_last_measured_size; }
    horizontal_alignment h_align() const noexcept { return m_h_align; }
    vertical_alignment v_align() const noexcept { return m_v_align; }

private:
    // All implementation details remain private
    ui_element* m_parent = nullptr;
    std::vector<ui_element_ptr> m_children;
    layout_strategy_ptr m_layout_strategy;

    // ... rest of private members unchanged
};
```

**Changes Required:**
- Add 20+ lines of public accessors
- Add 4 protected const getters
- Update all layout strategies to use `children()` instead of `m_children`
- Update all layout strategies to use `last_measured_size()` instead of direct access

**Files Affected:**
- `include/onyxui/element.hh` (primary changes)
- `include/onyxui/layout/linear_layout.hh` (use protected interface)
- `include/onyxui/layout/grid_layout.hh` (use protected interface)
- `include/onyxui/layout/anchor_layout.hh` (use protected interface)
- `include/onyxui/layout/absolute_layout.hh` (use protected interface)

---

### Fix #2: Layout Classes - Public Inheritance and Immutable Configuration

**Problem:** Private inheritance and mutable configuration members.

**Solution:** Public inheritance with immutable const configuration.

#### linear_layout.hh

```cpp
template<RectLike TRect, SizeLike TSize>
class linear_layout : public layout_strategy<TRect, TSize> {  // PUBLIC inheritance
public:
    /**
     * @brief Construct linear layout with configuration
     * @param dir Stack direction (vertical or horizontal)
     * @param spacing Gap between children in pixels
     * @param h_align Horizontal alignment for children
     * @param v_align Vertical alignment for children
     */
    explicit linear_layout(
        direction dir = direction::vertical,
        int spacing = 0,
        horizontal_alignment h_align = horizontal_alignment::stretch,
        vertical_alignment v_align = vertical_alignment::stretch
    )
        : m_layout_direction(dir)
        , m_spacing(spacing)
        , m_h_align(h_align)
        , m_v_align(v_align)
    {}

    // Public const accessors (for introspection)
    direction layout_direction() const noexcept { return m_layout_direction; }
    int spacing() const noexcept { return m_spacing; }
    horizontal_alignment h_align() const noexcept { return m_h_align; }
    vertical_alignment v_align() const noexcept { return m_v_align; }

private:
    using elt_t = ui_element<TRect, TSize>;

    // Configuration is immutable
    const direction m_layout_direction;
    const int m_spacing;
    const horizontal_alignment m_h_align;
    const vertical_alignment m_v_align;

    // Private overrides (Strategy pattern)
    TSize measure_children(elt_t* parent,
                           int available_width,
                           int available_height) override;

    void arrange_children(elt_t* parent,
                          const TRect& content_area) override;

    void arrange_vertical(std::vector<elt_t*>& children,
                          const TRect& content_area);

    void arrange_horizontal(std::vector<elt_t*>& children,
                            const TRect& content_area);
};
```

**Implementation changes:**
```cpp
// OLD (broken):
if (layout_direction == direction::vertical) {
    for (auto& child : parent->children) {  // ERROR: private member
        if (!child->visible) continue;       // ERROR: private member
        // ...
    }
}

// NEW (fixed):
if (m_layout_direction == direction::vertical) {
    for (auto& child : parent->children()) {  // Protected getter
        if (!child->is_visible()) continue;   // Protected getter
        // ...
    }
}
```

#### grid_layout.hh

```cpp
template<RectLike TRect, SizeLike TSize>
class grid_layout : public layout_strategy<TRect, TSize> {  // PUBLIC inheritance
public:
    /**
     * @brief Construct grid layout with configuration
     * @param num_columns Number of columns in grid
     * @param num_rows Number of rows (-1 for auto-calculate)
     * @param column_spacing Horizontal gap between cells
     * @param row_spacing Vertical gap between cells
     * @param auto_size If true, size cells from content; if false, use fixed sizes
     * @param column_widths Fixed column widths (empty for auto-size)
     * @param row_heights Fixed row heights (empty for auto-size)
     */
    explicit grid_layout(
        int num_columns = 1,
        int num_rows = -1,
        int column_spacing = 0,
        int row_spacing = 0,
        bool auto_size = true,
        std::vector<int> column_widths = {},
        std::vector<int> row_heights = {}
    )
        : m_num_columns(num_columns)
        , m_num_rows(num_rows)
        , m_column_spacing(column_spacing)
        , m_row_spacing(row_spacing)
        , m_auto_size_cells(auto_size)
        , m_column_widths(std::move(column_widths))
        , m_row_heights(std::move(row_heights))
    {
        // Validation
        if (m_num_columns < 1) m_num_columns = 1;
    }

    // Public cell assignment (mutable because it's positional, not layout config)
    void set_cell(ui_element<TRect, TSize>* child, int row, int col,
                  int row_span = 1, int col_span = 1) {
        if (!child || row < 0 || col < 0 || row_span < 1 || col_span < 1) {
            return;  // Validation
        }
        m_cell_mapping[child] = {row, col, row_span, col_span};
    }

    // Public accessors
    int num_columns() const noexcept { return m_num_columns; }
    int num_rows() const noexcept { return m_num_rows; }

private:
    using elt_t = ui_element<TRect, TSize>;

    // Immutable configuration
    int m_num_columns;
    int m_num_rows;
    const int m_column_spacing;
    const int m_row_spacing;
    const bool m_auto_size_cells;

    // Mutable sizing state (calculated during measure)
    mutable std::vector<int> m_column_widths;
    mutable std::vector<int> m_row_heights;

    // Mutable cell mapping (positional data, not layout config)
    std::unordered_map<elt_t*, grid_cell_info> m_cell_mapping;

    // Private overrides
    TSize measure_children(elt_t* parent, int available_width, int available_height) override;
    void arrange_children(elt_t* parent, const TRect& content_area) override;

    // Private helpers
    void auto_assign_cells(elt_t* parent);
    int calculate_row_count(elt_t* parent);
    void measure_auto_sized_grid(elt_t* parent, int available_width,
                                  int available_height, int actual_rows);
    void use_fixed_grid_sizes(int actual_rows);
};
```

#### anchor_layout.hh

```cpp
template<RectLike TRect, SizeLike TSize>
class anchor_layout : public layout_strategy<TRect, TSize> {  // PUBLIC inheritance
public:
    /**
     * @brief Construct anchor layout (no configuration needed)
     */
    anchor_layout() = default;

    /**
     * @brief Set anchor point for a child element
     * @param child Pointer to child element
     * @param point Anchor point to use
     * @param offset_x Horizontal offset in pixels
     * @param offset_y Vertical offset in pixels
     */
    void set_anchor(ui_element<TRect, TSize>* child, anchor_point point,
                    int offset_x = 0, int offset_y = 0) {
        if (!child) return;
        m_anchor_mapping[child] = {point, offset_x, offset_y};
    }

private:
    using elt_t = ui_element<TRect, TSize>;

    struct anchor_info {
        anchor_point point = anchor_point::top_left;
        int offset_x = 0;
        int offset_y = 0;
    };

    std::unordered_map<elt_t*, anchor_info> m_anchor_mapping;

    // Private overrides
    TSize measure_children(elt_t* parent, int available_width, int available_height) override;
    void arrange_children(elt_t* parent, const TRect& content_area) override;

    static void calculate_anchor_position(const TRect& content_area,
                                          const TSize& child_size,
                                          const anchor_info& info,
                                          int& out_x, int& out_y);
};
```

#### absolute_layout.hh

```cpp
template<RectLike TRect, SizeLike TSize>
class absolute_layout : public layout_strategy<TRect, TSize> {  // PUBLIC inheritance
public:
    /**
     * @brief Construct absolute layout (no configuration needed)
     */
    absolute_layout() = default;

    /**
     * @brief Set position for a child element
     * @param child Pointer to child element
     * @param x X coordinate in pixels
     * @param y Y coordinate in pixels
     * @param width Width override (-1 for auto)
     * @param height Height override (-1 for auto)
     */
    void set_position(ui_element<TRect, TSize>* child, int x, int y,
                      int width = -1, int height = -1) {
        if (!child) return;
        m_position_mapping[child] = {x, y, width, height};
    }

private:
    using elt_t = ui_element<TRect, TSize>;

    struct position_info {
        int x = 0;
        int y = 0;
        int width = -1;
        int height = -1;
    };

    std::unordered_map<elt_t*, position_info> m_position_mapping;

    // Private overrides
    TSize measure_children(elt_t* parent, int available_width, int available_height) override;
    void arrange_children(elt_t* parent, const TRect& content_area) override;
};
```

**Changes Required:**
- Add constructors with parameters to all layout classes
- Make configuration members `const` and prefix with `m_`
- Make all override methods `private`
- Add public accessors for configuration introspection
- Update all `visible` to `is_visible()`
- Update all `parent->children` to `parent->children()`

**Files Affected:**
- `include/onyxui/layout/linear_layout.hh`
- `include/onyxui/layout/grid_layout.hh`
- `include/onyxui/layout/anchor_layout.hh`
- `include/onyxui/layout/absolute_layout.hh`

---

### Fix #3: Type-Safe Utility Function Usage

**Problem:** Direct member access breaks type-agnostic design.

**Solution:** Use utility functions consistently everywhere.

#### In anchor_layout::arrange_children

```cpp
// OLD (broken):
auto measured = child->last_measured_size;
child->arrange({child_x, child_y, measured.width, measured.height});

// NEW (fixed):
const TSize& measured = child->last_measured_size();
int w = size_utils::get_width(measured);
int h = size_utils::get_height(measured);
TRect child_bounds;
rect_utils::set_bounds(child_bounds, child_x, child_y, w, h);
child->arrange(child_bounds);
```

#### In anchor_layout::calculate_anchor_position

```cpp
// OLD (broken):
out_x = content_area.x + (content_area.w - child_size.width) / 2;
out_y = content_area.y;

// NEW (fixed):
int area_x = rect_utils::get_x(content_area);
int area_y = rect_utils::get_y(content_area);
int area_w = rect_utils::get_width(content_area);
int area_h = rect_utils::get_height(content_area);
int child_w = size_utils::get_width(child_size);
int child_h = size_utils::get_height(child_size);

switch (info.point) {
    case anchor_point::top_center:
        out_x = area_x + (area_w - child_w) / 2;
        out_y = area_y;
        break;
    // ... etc
}
```

#### In absolute_layout

```cpp
// OLD (broken):
return {max_width, max_height};

// NEW (fixed):
TSize result = {};
size_utils::set_size(result, max_width, max_height);
return result;
```

**Changes Required:**
- Replace ALL direct `.x`, `.y`, `.w`, `.h`, `.width`, `.height` access
- Use `rect_utils::get_x()`, `rect_utils::get_y()`, `rect_utils::get_width()`, `rect_utils::get_height()`
- Use `size_utils::get_width()`, `size_utils::get_height()`, `size_utils::set_size()`
- Replace brace initialization with utility function calls

**Files Affected:**
- `include/onyxui/layout/anchor_layout.hh` (20+ locations)
- `include/onyxui/layout/absolute_layout.hh` (10+ locations)

---

### Fix #4: hit_test Correctness

**Problem:** Calls non-existent `bounds.contains()` method.

**Solution:** Use `rect_utils::contains()`.

```cpp
// In element.hh hit_test() method

// OLD (broken):
if (!m_visible || !m_bounds.contains(x, y)) {
    return nullptr;
}

// NEW (fixed):
if (!m_visible || !rect_utils::contains(m_bounds, x, y)) {
    return nullptr;
}
```

**Changes Required:**
- Replace `m_bounds.contains()` with `rect_utils::contains(m_bounds, ...)`

**Files Affected:**
- `include/onyxui/element.hh` (line ~481)

---

### Fix #5: Parameter Type Mismatch

**Problem:** Declaration vs definition mismatch in `linear_layout::arrange_vertical`.

**Solution:** Fix parameter type.

```cpp
// In linear_layout.hh

// Declaration at line ~102:
void arrange_vertical(std::vector<elt_t*>& children,
                      const TRect& content_area);

// Definition at line ~194:
// OLD (broken):
void linear_layout<TRect, TSize>::arrange_vertical(std::vector<elt_t>& children, ...)

// NEW (fixed):
void linear_layout<TRect, TSize>::arrange_vertical(std::vector<elt_t*>& children, ...)
```

**Changes Required:**
- Change parameter from `std::vector<elt_t>&` to `std::vector<elt_t*>&`

**Files Affected:**
- `include/onyxui/layout/linear_layout.hh` (line ~194)

---

## API Design Changes

### Public API Surface

#### Before (Broken)

```cpp
// User code (doesn't compile):
auto element = std::make_unique<ui_element<SDL_Rect, SDL_Size>>(nullptr);
element->visible = false;  // ERROR: private member
element->width_constraint.policy = size_policy::fixed;  // ERROR: private member
element->width_constraint.preferred_size = 200;  // ERROR: private member

auto layout = std::make_unique<linear_layout<SDL_Rect, SDL_Size>>();
layout->spacing = 10;  // ERROR: private member (no public section)
element->m_layout_strategy = std::move(layout);  // ERROR: private member
```

#### After (Fixed)

```cpp
// User code (compiles and works):
auto element = std::make_unique<ui_element<SDL_Rect, SDL_Size>>(nullptr);
element->set_visible(false);  // Public setter
element->width_constraint().policy = size_policy::fixed;  // Returns reference
element->width_constraint().preferred_size = 200;

auto layout = std::make_unique<linear_layout<SDL_Rect, SDL_Size>>(
    direction::vertical,  // Constructor parameters
    10,                   // spacing
    horizontal_alignment::stretch,
    vertical_alignment::stretch
);
element->set_layout_strategy(std::move(layout));  // Public setter
```

### Configuration Pattern

#### Before (Mutable)

```cpp
auto layout = std::make_unique<linear_layout<SDL_Rect, SDL_Size>>();
layout->layout_direction = direction::horizontal;
layout->spacing = 5;
// ... later in code
layout->spacing = 10;  // Can change anytime (dangerous!)
```

#### After (Immutable)

```cpp
auto layout = std::make_unique<linear_layout<SDL_Rect, SDL_Size>>(
    direction::horizontal,  // Set once at construction
    5                        // spacing - immutable
);
// layout->spacing = 10;  // ERROR: const member
// Must create new layout to change configuration
```

---

## Implementation Plan

### Phase 1: Critical Compilation Fixes (Priority 1)

**Goal:** Make the library compile.

**Tasks:**

1. **Update element.hh** (4 hours)
   - [ ] Add public setter methods (set_visible, set_layout_strategy, etc.)
   - [ ] Add public property accessors returning references
   - [ ] Add public const accessors
   - [ ] Add protected const getters for layouts
   - [ ] Fix hit_test to use rect_utils::contains
   - [ ] Remove duplicate comment
   - [ ] Update documentation

2. **Update linear_layout.hh** (2 hours)
   - [ ] Add public inheritance
   - [ ] Add constructor with parameters
   - [ ] Make configuration const members
   - [ ] Make all methods private
   - [ ] Fix arrange_vertical parameter type
   - [ ] Update all parent->children to parent->children()
   - [ ] Update all child->visible to child->is_visible()
   - [ ] Update all constraint/alignment access to use getters
   - [ ] Remove unused variable

3. **Update grid_layout.hh** (2 hours)
   - [ ] Add public inheritance
   - [ ] Add constructor with parameters
   - [ ] Make configuration const (except mutable sizing state)
   - [ ] Make all override methods private
   - [ ] Add validation to set_cell
   - [ ] Update all member access to use protected getters
   - [ ] Update documentation

4. **Update anchor_layout.hh** (1.5 hours)
   - [ ] Add public inheritance
   - [ ] Make all override methods private
   - [ ] Fix calculate_anchor_position to use utility functions
   - [ ] Fix arrange_children to use utility functions
   - [ ] Update all member access to use protected getters
   - [ ] Update documentation

5. **Update absolute_layout.hh** (1.5 hours)
   - [ ] Add public inheritance
   - [ ] Make all override methods private
   - [ ] Fix measure_children return to use set_size
   - [ ] Fix arrange_children to use utility functions
   - [ ] Update all member access to use protected getters
   - [ ] Update documentation

**Deliverable:** Code compiles without errors.

---

### Phase 2: High Priority Correctness (Priority 2)

**Goal:** Fix runtime bugs and type safety violations.

**Tasks:**

1. **Weighted Distribution Fix** (2 hours)
   - [ ] Implement remainder distribution in linear_layout::arrange_vertical
   - [ ] Implement remainder distribution in linear_layout::arrange_horizontal
   - [ ] Add division-by-zero check
   - [ ] Add unit tests

2. **Negative Size Protection** (1 hour)
   - [ ] Add std::max(0, ...) to content_width/height calculations in element.hh
   - [ ] Add unit tests for edge cases

3. **Grid Bounds Checking** (2 hours)
   - [ ] Add validation before accessing column_positions/row_positions
   - [ ] Add checks for cell.column >= num_columns
   - [ ] Handle empty grids gracefully
   - [ ] Add unit tests

4. **Grid Spanning Cell Fix** (3 hours)
   - [ ] Implement proper span distribution in measure_auto_sized_grid
   - [ ] Handle cells spanning multiple rows/columns correctly
   - [ ] Add unit tests for spanned cells

**Deliverable:** No runtime crashes, correct layout calculation.

---

### Phase 3: Medium Priority Robustness (Priority 3)

**Goal:** Add validation, error handling, and missing features.

**Tasks:**

1. **Implement Percentage Size Policy** (3 hours)
   - [ ] Add percentage handling in linear_layout::arrange_vertical
   - [ ] Add percentage handling in linear_layout::arrange_horizontal
   - [ ] Add percentage handling in grid_layout
   - [ ] Add unit tests

2. **Implement fill_parent for Cross-Axis** (2 hours)
   - [ ] Check fill_parent in horizontal alignment (vertical layout)
   - [ ] Check fill_parent in vertical alignment (horizontal layout)
   - [ ] Add unit tests

3. **Add Parameter Validation** (2 hours)
   - [ ] Validate constructor parameters in all layouts
   - [ ] Add assertions or exceptions for null pointers
   - [ ] Add validation for negative dimensions
   - [ ] Document validation behavior

4. **Add Error Handling** (3 hours)
   - [ ] Decide on error strategy (assertions vs exceptions)
   - [ ] Add error handling to public APIs
   - [ ] Document error conditions
   - [ ] Add error handling tests

**Deliverable:** Robust, production-ready library.

---

### Phase 4: Polish and Performance (Priority 4)

**Goal:** Optimize and improve code quality.

**Tasks:**

1. **Add noexcept Specifications** (2 hours)
   - [ ] Mark simple getters as noexcept
   - [ ] Mark utility functions as noexcept
   - [ ] Mark non-throwing methods as noexcept
   - [ ] Document throwing vs non-throwing

2. **Add [[nodiscard]] Attributes** (1 hour)
   - [ ] Add to measure()
   - [ ] Add to remove_child()
   - [ ] Add to hit_test()
   - [ ] Add to all important return values

3. **Add Move Semantics** (1 hour)
   - [ ] Add defaulted move constructor/assignment
   - [ ] Delete copy constructor/assignment explicitly
   - [ ] Document move-only semantics

4. **Const Correctness Pass** (2 hours)
   - [ ] Make hit_test const
   - [ ] Review all methods for const-correctness
   - [ ] Add const overloads where needed

**Deliverable:** Polished, performant library.

---

## Breaking Changes

### For Existing Users (Migration Required)

#### 1. Layout Construction

**Before:**
```cpp
auto layout = std::make_unique<linear_layout<SDL_Rect, SDL_Size>>();
layout->spacing = 10;
layout->layout_direction = direction::horizontal;
```

**After:**
```cpp
auto layout = std::make_unique<linear_layout<SDL_Rect, SDL_Size>>(
    direction::horizontal,  // direction
    10                      // spacing
);
```

**Migration:** Pass configuration as constructor parameters.

#### 2. Property Access

**Before:**
```cpp
element->visible = false;
element->width_constraint.policy = size_policy::fixed;
```

**After:**
```cpp
element->set_visible(false);
element->width_constraint().policy = size_policy::fixed;
```

**Migration:** Use setters and accessor methods.

#### 3. Layout Strategy Assignment

**Before:**
```cpp
element->m_layout_strategy = std::move(layout);  // Wrong - was never public
```

**After:**
```cpp
element->set_layout_strategy(std::move(layout));
```

**Migration:** Use public setter method.

---

## Testing Strategy

### Unit Tests Required

#### 1. Compilation Tests

```cpp
// test_compilation.cc
TEST_CASE("Public API compiles") {
    using Element = ui_element<SDL_Rect, SDL_Size>;

    auto root = std::make_unique<Element>(nullptr);
    root->set_visible(true);
    root->width_constraint().policy = size_policy::fixed;

    auto layout = std::make_unique<linear_layout<SDL_Rect, SDL_Size>>(
        direction::vertical, 10
    );
    root->set_layout_strategy(std::move(layout));

    CHECK(true);  // Compilation is the test
}
```

#### 2. Layout Correctness Tests

```cpp
TEST_CASE("Linear layout arranges children correctly") {
    // Create parent with 3 fixed-height children
    // Verify positions after arrange
    // Check spacing is correct
    // Verify bounds are correct
}

TEST_CASE("Grid layout with spanning cells") {
    // Create grid with cell spanning 2 columns
    // Verify grid sizes correctly
    // Check child bounds include span
}

TEST_CASE("Weighted distribution with remainder") {
    // 3 equal-weight children with 100px available
    // Verify 34+33+33 or 33+34+33 (no pixel lost)
}
```

#### 3. Edge Case Tests

```cpp
TEST_CASE("Negative available size") {
    // Margins larger than available space
    // Verify no crash, graceful handling
}

TEST_CASE("Zero weight total") {
    // All weights are 0
    // Verify no division by zero
}

TEST_CASE("Empty grid") {
    // No children
    // Verify no crash
}

TEST_CASE("Out of bounds grid cell") {
    // Cell column >= num_columns
    // Verify graceful handling
}
```

#### 4. Immutability Tests

```cpp
TEST_CASE("Configuration is immutable") {
    auto layout = std::make_unique<linear_layout<SDL_Rect, SDL_Size>>(
        direction::vertical, 10
    );

    // This should not compile:
    // layout->m_spacing = 20;  // ERROR: const member

    CHECK(layout->spacing() == 10);
}
```

### Integration Tests

```cpp
TEST_CASE("Complex nested layout") {
    // Root panel (vertical)
    //   ├─ Header (horizontal)
    //   │   ├─ Title (expand)
    //   │   └─ Close button (fixed)
    //   ├─ Content (grid 2x2)
    //   │   ├─ Cell 1
    //   │   ├─ Cell 2
    //   │   ├─ Cell 3 (span 2 columns)
    //   └─ Footer (horizontal)

    // Build hierarchy
    // Perform layout
    // Verify all bounds correct
}
```

---

## Migration Guide

### For Existing Code (If Any)

#### Step 1: Update Layout Construction

Find all layout constructions:
```bash
grep -r "std::make_unique<.*_layout" .
```

Update to use constructor parameters:
```cpp
// OLD:
auto layout = std::make_unique<linear_layout<R, S>>();
layout->spacing = 10;

// NEW:
auto layout = std::make_unique<linear_layout<R, S>>(
    direction::vertical, 10
);
```

#### Step 2: Update Property Access

Find all direct member access:
```bash
grep -r "->visible\s*=" .
grep -r "->width_constraint\." .
```

Update to use accessors:
```cpp
// OLD:
element->visible = false;
element->width_constraint.policy = size_policy::fixed;

// NEW:
element->set_visible(false);
element->width_constraint().policy = size_policy::fixed;
```

#### Step 3: Update Layout Assignment

```cpp
// OLD:
element->m_layout_strategy = std::move(layout);  // Never worked

// NEW:
element->set_layout_strategy(std::move(layout));
```

#### Step 4: Recompile and Test

1. Fix compilation errors
2. Run unit tests
3. Verify layout behavior
4. Check for memory leaks (Valgrind)

---

## Documentation Updates Required

### Files to Update

1. **CLAUDE.md**
   - Update API usage examples
   - Update common patterns section
   - Document immutable configuration
   - Add migration notes

2. **docs/layout.md**
   - Update all code examples
   - Document constructor parameters
   - Update property access patterns
   - Add best practices section

3. **Header Comments**
   - Update @example blocks
   - Update usage patterns
   - Document breaking changes
   - Add version notes

### New Documentation

1. **docs/API_REFERENCE.md**
   - Complete public API reference
   - All public methods documented
   - Usage examples for each
   - Common patterns

2. **docs/MIGRATION_GUIDE.md**
   - Detailed migration steps
   - Before/after examples
   - Known issues
   - FAQ

3. **examples/ directory**
   - Complete working examples
   - SDL integration example
   - SFML integration example
   - Custom types example

---

## Risk Assessment

### High Risk

1. **Breaking API Changes**
   - **Risk:** Existing code breaks
   - **Mitigation:** Comprehensive migration guide, examples, clear documentation
   - **Impact:** High for existing users, zero for new library

2. **Subtle Behavioral Changes**
   - **Risk:** Immutable config changes runtime behavior
   - **Mitigation:** Extensive testing, document behavior clearly
   - **Impact:** Medium - may require code changes

### Medium Risk

1. **Performance Changes**
   - **Risk:** Getter indirection adds overhead
   - **Mitigation:** Mark as inline and noexcept, compiler will optimize
   - **Impact:** Low - negligible performance difference

2. **Missing Features**
   - **Risk:** Percentage policy still not implemented
   - **Mitigation:** Implement in Phase 3, document as "planned"
   - **Impact:** Medium - users may expect it to work

### Low Risk

1. **Documentation Lag**
   - **Risk:** Docs out of sync with code
   - **Mitigation:** Update docs in same PR as code changes
   - **Impact:** Low - users read current docs

---

## Success Criteria

### Phase 1 Success
- [ ] Code compiles without errors
- [ ] All warnings addressed
- [ ] Basic unit tests pass

### Phase 2 Success
- [ ] No runtime crashes
- [ ] All layout calculations correct
- [ ] Edge cases handled gracefully

### Phase 3 Success
- [ ] All features implemented
- [ ] Comprehensive test coverage (>90%)
- [ ] Production-ready quality

### Phase 4 Success
- [ ] Optimal performance
- [ ] Complete documentation
- [ ] Ready for public release

---

## Appendix A: Checklist

### Pre-Implementation

- [ ] Review this document with team
- [ ] Approve design decisions
- [ ] Set up testing infrastructure
- [ ] Create feature branch

### Implementation

- [ ] Phase 1 tasks complete
- [ ] Phase 2 tasks complete
- [ ] Phase 3 tasks complete
- [ ] Phase 4 tasks complete

### Validation

- [ ] All unit tests pass
- [ ] Integration tests pass
- [ ] Performance benchmarks acceptable
- [ ] Memory leak check clean
- [ ] Code review complete

### Documentation

- [ ] API reference updated
- [ ] Migration guide written
- [ ] Examples updated
- [ ] CLAUDE.md updated
- [ ] CHANGELOG.md updated

### Release

- [ ] Tag version
- [ ] Create release notes
- [ ] Update documentation site
- [ ] Announce changes

---

## Appendix B: Code Style Guide

### Naming Conventions

- **Private members:** `m_` prefix (e.g., `m_children`)
- **Constants:** `const` keyword, `m_` prefix (e.g., `const int m_spacing`)
- **Getters:** No prefix (e.g., `spacing()`)
- **Setters:** `set_` prefix (e.g., `set_visible()`)
- **Type aliases:** `_t` suffix (e.g., `elt_t`)

### Documentation Style

- **File headers:** Doxygen `@file`, `@brief`, `@author`
- **Classes:** Doxygen `@class`, description, examples
- **Methods:** Doxygen `@brief`, `@param`, `@return`
- **Inline comments:** `//` for single-line, `/* */` for multi-line

### Code Organization

- **Public first:** Public members and methods at top
- **Protected next:** Protected getters for derived classes
- **Private last:** Implementation details at bottom
- **Sections:** Use comment dividers for major sections

---

## Appendix C: Estimated Timeline

### Week 1
- Day 1-2: Phase 1 (element.hh)
- Day 3-4: Phase 1 (layout classes)
- Day 5: Phase 1 completion and testing

### Week 2
- Day 1-2: Phase 2 (correctness fixes)
- Day 3-4: Phase 2 testing
- Day 5: Phase 3 start (validation)

### Week 3
- Day 1-2: Phase 3 (features and robustness)
- Day 3: Phase 4 (polish)
- Day 4-5: Final testing and documentation

### Week 4
- Day 1-3: Integration testing and examples
- Day 4: Documentation finalization
- Day 5: Release preparation

**Total:** 4 weeks (20 working days)

---

## Document Change Log

| Version | Date | Changes | Author |
|---------|------|---------|--------|
| 1.0 | 2025-10-09 | Initial comprehensive refactoring plan | Claude Code |

---

**END OF DOCUMENT**
