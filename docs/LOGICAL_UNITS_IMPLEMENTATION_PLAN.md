# Logical Units - Implementation Plan

**Version:** 1.0
**Date:** 2025-11-26
**Status:** Proposed

---

## Table of Contents

1. [Overview](#overview)
2. [Phase 0: Preparation](#phase-0-preparation)
3. [Phase 1: Core Types](#phase-1-core-types)
4. [Phase 2: Backend Metrics](#phase-2-backend-metrics)
5. [Phase 3: Layout System](#phase-3-layout-system)
6. [Phase 4: Rendering](#phase-4-rendering)
7. [Phase 5: Widgets](#phase-5-widgets)
8. [Phase 6: UDim Support](#phase-6-udim-support)
9. [Phase 7: Polish](#phase-7-polish)
10. [Testing Requirements](#testing-requirements)
11. [Success Criteria](#success-criteria)

---

## Overview

### Principles

1. **No Backward Compatibility**
   - Breaking changes acceptable (major version bump)
   - Clean slate for better architecture

2. **Testable Phases**
   - Each phase is independently testable
   - Tests pass before moving to next phase
   - Integration tests verify phase interactions

3. **Incremental Migration**
   - Replace subsystems one at a time
   - Always maintain working build
   - Feature flags for gradual rollout (if needed)

4. **Documentation-Driven**
   - Update docs alongside code
   - Examples for each new API
   - Migration guide for users

### Timeline Estimate

| Phase | Duration | Complexity | Dependencies |
|-------|----------|------------|--------------|
| Phase 0 | 2 days | Low | None |
| Phase 1 | 3 days | Medium | Phase 0 |
| Phase 2 | 3 days | Medium | Phase 1 |
| Phase 3 | 5 days | High | Phase 2 |
| Phase 4 | 4 days | High | Phase 3 |
| Phase 5 | 7 days | Medium | Phase 4 |
| Phase 6 | 4 days | Medium | Phase 5 |
| Phase 7 | 3 days | Low | Phase 6 |
| **Total** | **~5 weeks** | | |

### Success Metrics

- **Code Quality:** Zero compiler warnings, all tests pass
- **Performance:** < 5% overhead vs current implementation
- **Precision:** Sub-pixel accuracy (< 0.1 logical unit error)
- **Coverage:** 100% of widgets migrated

---

## Phase 0: Preparation

**Goal:** Set up infrastructure for logical units implementation

**Duration:** 2 days

### Tasks

1. **Create Feature Branch**
   ```bash
   git checkout -b feature/logical-units
   ```

2. **Update Documentation**
   - Review and finalize `LOGICAL_UNITS_DESIGN.md`
   - Create this implementation plan
   - Update `CLAUDE.md` with new concepts

3. **Set Up Test Infrastructure**
   - Create `unittest/core/test_logical_units.cc` (empty)
   - Create `unittest/backend/test_backend_metrics.cc` (empty)
   - Configure CMake to build new tests

4. **Prepare Code Locations**
   ```
   include/onyxui/core/
     ├── types.hh           (NEW - logical unit types)
     ├── backend_metrics.hh (NEW - conversion logic)
     └── geometry.hh        (NEW - logical_rect, logical_size)
   ```

5. **Create Stub Headers**
   - Add header guards, namespaces
   - Add placeholder classes/structs
   - Ensure project still builds

### Deliverables

- [ ] Feature branch created
- [ ] Documentation updated and reviewed
- [ ] Empty test files set up
- [ ] Stub headers created
- [ ] Project builds successfully

### Exit Criteria

- ✅ `cmake --build build` succeeds
- ✅ All existing tests still pass
- ✅ No regressions in current functionality

---

## Phase 1: Core Types

**Goal:** Implement `logical_unit`, `logical_size`, `logical_rect`, `logical_point`

**Duration:** 3 days

### Tasks

#### 1.1 Implement `logical_unit`

**File:** `include/onyxui/core/types.hh`

```cpp
namespace onyxui {

struct logical_unit {
    double value;

    // Construction
    constexpr explicit logical_unit(double v) noexcept;
    constexpr explicit logical_unit(int v) noexcept;

    // Arithmetic
    constexpr logical_unit operator+(logical_unit other) const noexcept;
    constexpr logical_unit operator-(logical_unit other) const noexcept;
    constexpr logical_unit operator*(double scalar) const noexcept;
    constexpr logical_unit operator/(double scalar) const noexcept;
    constexpr logical_unit& operator+=(logical_unit other) noexcept;
    constexpr logical_unit& operator-=(logical_unit other) noexcept;
    constexpr logical_unit& operator*=(double scalar) noexcept;
    constexpr logical_unit& operator/=(double scalar) noexcept;

    // Comparison
    constexpr bool operator==(logical_unit other) const noexcept;
    constexpr bool operator!=(logical_unit other) const noexcept;
    constexpr bool operator<(logical_unit other) const noexcept;
    constexpr bool operator<=(logical_unit other) const noexcept;
    constexpr bool operator>(logical_unit other) const noexcept;
    constexpr bool operator>=(logical_unit other) const noexcept;

    // Conversion
    [[nodiscard]] constexpr int to_int() const noexcept;
    [[nodiscard]] constexpr double to_double() const noexcept;
};

// Literals
constexpr logical_unit operator""_lu(long double v) noexcept;
constexpr logical_unit operator""_lu(unsigned long long v) noexcept;

// Math functions
constexpr logical_unit min(logical_unit a, logical_unit b) noexcept;
constexpr logical_unit max(logical_unit a, logical_unit b) noexcept;
constexpr logical_unit abs(logical_unit a) noexcept;
constexpr logical_unit clamp(logical_unit value, logical_unit min_val, logical_unit max_val) noexcept;

} // namespace onyxui
```

**Tests:** `unittest/core/test_logical_units.cc`
- Construction from double, int
- All arithmetic operators
- Comparison with epsilon
- Literal suffix `_lu`
- Math functions (min, max, abs, clamp)
- Edge cases (zero, negative, very large/small)

#### 1.2 Implement Geometry Types

**File:** `include/onyxui/core/geometry.hh`

```cpp
namespace onyxui {

struct logical_size {
    logical_unit width;
    logical_unit height;

    constexpr logical_size() noexcept;
    constexpr logical_size(logical_unit w, logical_unit h) noexcept;
    constexpr logical_size(double w, double h) noexcept;

    constexpr bool operator==(const logical_size& other) const noexcept;
    constexpr bool operator!=(const logical_size& other) const noexcept;
};

struct logical_point {
    logical_unit x;
    logical_unit y;

    constexpr logical_point() noexcept;
    constexpr logical_point(logical_unit x, logical_unit y) noexcept;
    constexpr logical_point(double x, double y) noexcept;

    constexpr bool operator==(const logical_point& other) const noexcept;
    constexpr bool operator!=(const logical_point& other) const noexcept;

    constexpr logical_point operator+(const logical_point& other) const noexcept;
    constexpr logical_point operator-(const logical_point& other) const noexcept;
};

struct logical_rect {
    logical_unit x;
    logical_unit y;
    logical_unit width;
    logical_unit height;

    constexpr logical_rect() noexcept;
    constexpr logical_rect(logical_unit x, logical_unit y,
                          logical_unit w, logical_unit h) noexcept;
    constexpr logical_rect(double x, double y, double w, double h) noexcept;

    // Getters
    [[nodiscard]] constexpr logical_unit left() const noexcept;
    [[nodiscard]] constexpr logical_unit top() const noexcept;
    [[nodiscard]] constexpr logical_unit right() const noexcept;
    [[nodiscard]] constexpr logical_unit bottom() const noexcept;
    [[nodiscard]] constexpr logical_point position() const noexcept;
    [[nodiscard]] constexpr logical_size size() const noexcept;

    // Containment
    [[nodiscard]] constexpr bool contains(logical_point p) const noexcept;
    [[nodiscard]] constexpr bool contains(const logical_rect& r) const noexcept;

    // Intersection
    [[nodiscard]] constexpr bool intersects(const logical_rect& r) const noexcept;
    [[nodiscard]] constexpr logical_rect intersection(const logical_rect& r) const noexcept;

    // Union
    [[nodiscard]] constexpr logical_rect union_with(const logical_rect& r) const noexcept;

    // Inflate/deflate
    [[nodiscard]] constexpr logical_rect inflated(logical_unit amount) const noexcept;
    [[nodiscard]] constexpr logical_rect deflated(logical_unit amount) const noexcept;
};

} // namespace onyxui
```

**Tests:** `unittest/core/test_logical_geometry.cc`
- Construction of size, point, rect
- Comparison operators
- Geometric operations (contains, intersects, union)
- Edge cases (zero size, negative coordinates)

### Deliverables

- [ ] `logical_unit` fully implemented
- [ ] Geometry types (size, point, rect) implemented
- [ ] All unit tests pass (target: 100+ tests)
- [ ] Code compiles with zero warnings
- [ ] Documentation comments complete

### Exit Criteria

- ✅ `./build/bin/ui_unittest --test-case="logical_unit*"` all pass
- ✅ `./build/bin/ui_unittest --test-case="logical_geometry*"` all pass
- ✅ Code coverage > 95% for new types
- ✅ Constexpr evaluation verified (compile-time tests)

---

## Phase 2: Backend Metrics

**Goal:** Implement backend-specific coordinate conversion

**Duration:** 3 days

### Tasks

#### 2.1 Implement Base `backend_metrics` Template

**File:** `include/onyxui/core/backend_metrics.hh`

```cpp
namespace onyxui {

template<UIBackend Backend>
struct backend_metrics {
    // Scaling configuration
    double logical_to_physical_x = 1.0;
    double logical_to_physical_y = 1.0;
    double aspect_ratio = 1.0;
    double dpi_scale = 1.0;

    // Conversion: Logical → Physical (floating point)
    [[nodiscard]] constexpr double to_physical_x(logical_unit lu) const noexcept;
    [[nodiscard]] constexpr double to_physical_y(logical_unit lu) const noexcept;

    // Conversion: Physical → Logical
    [[nodiscard]] constexpr logical_unit from_physical_x(double px) const noexcept;
    [[nodiscard]] constexpr logical_unit from_physical_y(double py) const noexcept;
    [[nodiscard]] constexpr logical_unit from_physical_x(int px) const noexcept;
    [[nodiscard]] constexpr logical_unit from_physical_y(int py) const noexcept;

    // Snapping modes
    enum class snap_mode { nearest, floor, ceil };

    // Snapping: Round to physical grid
    [[nodiscard]] int snap_to_physical_x(logical_unit lu, snap_mode mode = snap_mode::nearest) const noexcept;
    [[nodiscard]] int snap_to_physical_y(logical_unit lu, snap_mode mode = snap_mode::nearest) const noexcept;

    // Rect/size snapping
    [[nodiscard]] typename Backend::rect_type snap_rect(const logical_rect& r) const noexcept;
    [[nodiscard]] typename Backend::size_type snap_size(const logical_size& s) const noexcept;
    [[nodiscard]] typename Backend::point_type snap_point(const logical_point& p) const noexcept;
};

} // namespace onyxui
```

#### 2.2 Specialize for conio_backend

**File:** `backends/conio/include/conio_backend_metrics.hh`

```cpp
namespace onyxui::conio {

template<>
struct backend_metrics<conio_backend> {
    // 1 logical unit = 1 character cell
    double logical_to_physical_x = 1.0;
    double logical_to_physical_y = 1.0;

    // Char cells are 8×16 (aspect = width/height = 0.5)
    double aspect_ratio = 0.5;

    // No DPI scaling in terminal
    double dpi_scale = 1.0;

    // ... (include all methods from base template)
};

} // namespace onyxui::conio
```

#### 2.3 Create Test Backend

**File:** `unittest/utils/test_backend.hh`

```cpp
// Simple test backend for unit testing
struct test_backend {
    struct rect_type { int x, y, width, height; };
    struct size_type { int width, height; };
    struct point_type { int x, y; };
    struct color_type { uint8_t r, g, b, a; };
    // ... minimal implementation for testing
};

// Configurable metrics for testing
template<>
struct backend_metrics<test_backend> {
    double logical_to_physical_x = 1.0;  // Configurable
    double logical_to_physical_y = 1.0;
    double aspect_ratio = 1.0;
    double dpi_scale = 1.0;
    // ... (same methods)
};
```

**Tests:** `unittest/backend/test_backend_metrics.cc`
- Logical → physical conversion (various scales: 1x, 8x, 1.5x, 2x)
- Physical → logical conversion (round-trip accuracy)
- DPI scaling (1x, 1.5x, 2x, 3x)
- Aspect ratio handling
- Snapping modes (nearest, floor, ceil)
- Rect snapping (no gaps, no overlaps)
- Edge cases (zero, negative, very large values)

### Deliverables

- [ ] `backend_metrics` template implemented
- [ ] conio specialization complete
- [ ] Test backend created
- [ ] 150+ backend metric tests passing
- [ ] Documentation complete

### Exit Criteria

- ✅ `./build/bin/ui_unittest --test-case="backend_metrics*"` all pass
- ✅ Round-trip conversion error < 0.01 logical units
- ✅ Snapping produces no gaps/overlaps (verified via distribution tests)
- ✅ Code coverage > 95%

---

## Phase 3: Layout System

**Goal:** Migrate layout algorithm to use logical units

**Duration:** 5 days (most complex phase)

### Tasks

#### 3.1 Update `ui_element` Base Class

**File:** `include/onyxui/core/element.hh`

**Changes:**
1. Replace `int` bounds with `logical_rect`
2. Replace `size_type` with `logical_size`
3. Update measure/arrange signatures
4. Add backend metrics accessor

```cpp
template<UIBackend Backend>
class ui_element {
public:
    // NEW: Logical coordinate API
    virtual logical_size measure(logical_unit available_width,
                                 logical_unit available_height);

    virtual void arrange(const logical_rect& final_bounds);

    // Bounds in logical coordinates
    [[nodiscard]] const logical_rect& bounds() const noexcept {
        return m_bounds;
    }

    void set_bounds(const logical_rect& bounds);

    // Access backend metrics
    [[nodiscard]] const backend_metrics<Backend>& metrics() const noexcept;

protected:
    logical_rect m_bounds;           // Current bounds (logical)
    logical_size m_desired_size;     // Desired size from measure (logical)

    // Helper: Get metrics from ui_services
    [[nodiscard]] const backend_metrics<Backend>& get_metrics() const;
};
```

#### 3.2 Update `size_constraint`

**File:** `include/onyxui/layout/layout_strategy.hh`

**Changes:**
1. Replace `int` fields with `logical_unit`
2. Update `resolve()` to return `logical_unit`
3. Update `clamp()` to use logical units

```cpp
struct size_constraint {
    size_policy policy = size_policy::content;

    logical_unit preferred_size = logical_unit(0.0);
    logical_unit min_size = logical_unit(0.0);
    logical_unit max_size = logical_unit(std::numeric_limits<double>::max());

    double weight = 1.0;
    double percentage = 0.0;

    // Resolve constraint to logical units
    [[nodiscard]] logical_unit resolve(logical_unit parent_size) const noexcept;

    // Clamp value to min/max
    [[nodiscard]] logical_unit clamp(logical_unit value) const noexcept;
};
```

#### 3.3 Update Layout Strategies

**Files:**
- `include/onyxui/layout/linear_layout.hh`
- `include/onyxui/layout/grid_layout.hh`
- `include/onyxui/layout/anchor_layout.hh`
- `include/onyxui/layout/absolute_layout.hh`

**Changes for each:**
1. Replace `int` with `logical_unit` in all calculations
2. Update measure/arrange to use `logical_size`/`logical_rect`
3. Preserve floating-point precision (no premature rounding)
4. Update weighted distribution to use `double` arithmetic

**Critical: Weighted Distribution Algorithm**

```cpp
// OLD (integer - loses precision)
int remaining = total_width;
for (auto& child : children) {
    int child_width = (total_width * child_weight) / total_weight;
    remaining -= child_width;  // Accumulates error!
}

// NEW (floating-point - exact)
logical_unit remaining = total_width;
for (auto& child : children) {
    logical_unit child_width = total_width * (child_weight / total_weight);
    remaining = remaining - child_width;  // Exact!
}
// Final snapping at render time ensures no gaps
```

#### 3.4 Update Helper Utilities

**File:** `include/onyxui/utils/size_utils.hh`

```cpp
// OLD
static int get_width(const size_type& size);
static int get_height(const size_type& size);

// NEW
static logical_unit get_width(const logical_size& size) {
    return size.width;
}
static logical_unit get_height(const logical_size& size) {
    return size.height;
}

// Keep backend-specific versions for physical coordinates
template<UIBackend Backend>
static int get_physical_width(const typename Backend::size_type& size);
```

**Tests:** `unittest/layout/test_layout_logical_units.cc`
- Linear layout (vbox, hbox) with logical units
- Grid layout with logical units
- Weighted distribution (verify no precision loss)
- Percentage sizing (33.33% × 3 = 100%)
- Nested layouts (accumulation error < 0.1 lu)
- Min/max constraints with logical units
- Edge cases (zero size, single child, empty container)

### Deliverables

- [ ] `ui_element` migrated to logical coordinates
- [ ] All layout strategies updated
- [ ] Size constraints use logical units
- [ ] 200+ layout tests passing
- [ ] Precision verified (< 0.1 lu error)

### Exit Criteria

- ✅ All layout tests pass
- ✅ Weighted distribution exact (error < 1e-9)
- ✅ Percentage layouts exact (3 × 33.33% = 100.0%)
- ✅ Nested layouts accumulate < 0.1 lu error
- ✅ Performance < 5% slower than integer version

---

## Phase 4: Rendering

**Goal:** Update render contexts to snap logical → physical coordinates

**Duration:** 4 days

### Tasks

#### 4.1 Update `render_context` Base

**File:** `include/onyxui/core/rendering/render_context.hh`

**Changes:**
1. Add `backend_metrics` member
2. Accept logical coordinates in draw methods
3. Snap to physical coordinates internally

```cpp
template<UIBackend Backend>
class render_context {
public:
    explicit render_context(typename Backend::renderer_type* renderer,
                           const backend_metrics<Backend>& metrics)
        : m_renderer(renderer), m_metrics(metrics) {}

    // Access metrics
    [[nodiscard]] const backend_metrics<Backend>& metrics() const noexcept {
        return m_metrics;
    }

    // Drawing methods accept logical coordinates
    virtual void draw_rect(const logical_rect& rect, const box_style& style) = 0;
    virtual void draw_text(const std::string& text, logical_point position,
                          const font& f, const color_type& color) = 0;
    virtual void draw_line(logical_point from, logical_point to,
                          const color_type& color) = 0;

    // Measurement returns logical size
    virtual logical_size measure_text(const std::string& text, const font& f) const = 0;

protected:
    typename Backend::renderer_type* m_renderer;
    backend_metrics<Backend> m_metrics;
};
```

#### 4.2 Update `draw_context`

**File:** `include/onyxui/core/rendering/draw_context.hh`

**Changes:**
1. Implement snapping in each draw method
2. Convert logical → physical before calling renderer

```cpp
template<UIBackend Backend>
void draw_context<Backend>::draw_rect(const logical_rect& rect,
                                      const box_style& style) {
    // Snap logical rect to physical coordinates
    auto physical_rect = m_metrics.snap_rect(rect);

    // Draw using physical coordinates
    m_renderer->draw_rect(physical_rect, style);
}

template<UIBackend Backend>
void draw_context<Backend>::draw_text(const std::string& text,
                                      logical_point position,
                                      const font& f,
                                      const color_type& color) {
    // Snap logical position to physical coordinates
    auto physical_pos = m_metrics.snap_point(position);

    m_renderer->draw_text(text, physical_pos, f, color);
}
```

#### 4.3 Update `measure_context`

**File:** `include/onyxui/core/rendering/measure_context.hh`

**Changes:**
1. Measure in physical coordinates (from renderer)
2. Convert physical → logical before returning

```cpp
template<UIBackend Backend>
logical_size measure_context<Backend>::measure_text(const std::string& text,
                                                    const font& f) const {
    // Measure in physical coordinates (backend-specific)
    auto physical_size = m_renderer->measure_text(text, f);

    int px_width = size_utils::get_width(physical_size);
    int px_height = size_utils::get_height(physical_size);

    // Convert physical → logical
    logical_unit logical_width = m_metrics.from_physical_x(px_width);
    logical_unit logical_height = m_metrics.from_physical_y(px_height);

    return {logical_width, logical_height};
}
```

#### 4.4 Update Widget Rendering

**Example:** `include/onyxui/widgets/button.hh`

```cpp
template<UIBackend Backend>
void button<Backend>::do_render(render_context<Backend>& ctx) const {
    // Use logical coordinates throughout
    auto text_size = ctx.measure_text(m_text, default_font);
    auto bounds = this->bounds();  // logical_rect

    // Draw border (logical coordinates, snapped internally)
    ctx.draw_rect(bounds, m_box_style);

    // Center text (logical arithmetic)
    logical_point text_pos = {
        bounds.x + (bounds.width - text_size.width) * 0.5,
        bounds.y + (bounds.height - text_size.height) * 0.5
    };

    ctx.draw_text(m_text, text_pos, default_font, m_text_color);
}
```

**Tests:** `unittest/rendering/test_render_context_logical.cc`
- Snapping preserves rect edges (no gaps)
- Snapping distributes rounding fairly (3 rects of 33.33 = 100)
- Measurement round-trip accuracy (measure → logical → physical ≈ original)
- Drawing at fractional positions (verify snapping)
- Edge cases (zero size, negative coords)

### Deliverables

- [ ] `render_context` uses logical coordinates
- [ ] `draw_context` snaps to physical grid
- [ ] `measure_context` converts physical → logical
- [ ] All rendering tests passing
- [ ] Visual verification (screenshot comparison)

### Exit Criteria

- ✅ All render tests pass
- ✅ Visual output identical to pre-migration (pixel-perfect on conio)
- ✅ No visual artifacts (gaps, overlaps, blurriness)
- ✅ Performance overhead < 5%

---

## Phase 5: Widgets

**Goal:** Migrate all widgets to use logical coordinates

**Duration:** 7 days (largest migration, many files)

### Strategy

**Approach:** Migrate widgets in dependency order (leaves first)

**Order:**
1. **Leaf widgets** (no children): label, spacer, spring
2. **Simple containers**: panel, vbox, hbox
3. **Complex containers**: grid, group_box, scroll_view
4. **Input widgets**: button, checkbox, line_edit, slider, progress_bar
5. **Advanced**: menu system, tab_widget, window system

### Tasks per Widget

For each widget:
1. Update member variables (logical units)
2. Update measure() to return logical_size
3. Update arrange() to accept logical_rect
4. Update do_render() to use logical coordinates
5. Update public API (setters/getters)
6. Add/update unit tests
7. Update examples

### Example: Migrating `label`

**Before:**
```cpp
template<UIBackend Backend>
class label : public widget<Backend> {
    size_type measure(int available_width, int available_height) override {
        auto text_size = renderer::measure_text(m_text, m_font);
        return text_size;  // size_type (backend-specific)
    }

    void do_render(render_context<Backend>& ctx) const override {
        auto bounds = this->bounds();  // rect_type (int x, y, w, h)
        ctx.draw_text(m_text, {bounds.x, bounds.y}, m_font, m_color);
    }
};
```

**After:**
```cpp
template<UIBackend Backend>
class label : public widget<Backend> {
    logical_size measure(logical_unit available_width,
                        logical_unit available_height) override {
        auto text_size = ctx.measure_text(m_text, m_font);
        return text_size;  // logical_size
    }

    void do_render(render_context<Backend>& ctx) const override {
        auto bounds = this->bounds();  // logical_rect
        ctx.draw_text(m_text, bounds.position(), m_font, m_color);
    }
};
```

### Widget Migration Checklist

For each widget:
- [ ] measure() signature updated
- [ ] arrange() signature updated
- [ ] do_render() uses logical coordinates
- [ ] Public API (set_size, set_position) uses logical units
- [ ] Size constraints use logical units
- [ ] All tests updated and passing
- [ ] Example code updated
- [ ] Documentation updated

### Deliverables

- [ ] All ~30 widgets migrated
- [ ] All widget tests passing (1554+ tests)
- [ ] Examples updated (6 example files)
- [ ] widgets_demo running correctly

### Exit Criteria

- ✅ `./build/bin/ui_unittest` all tests pass (1554+)
- ✅ `./build/bin/widgets_demo` runs without issues
- ✅ Visual verification: screenshots match pre-migration
- ✅ Zero compiler warnings
- ✅ Code coverage > 90% for all widgets

---

## Phase 6: UDim Support

**Goal:** Add unified dimension (mixed relative+absolute) support

**Duration:** 4 days

### Tasks

#### 6.1 Implement `unified_dimension`

**File:** `include/onyxui/layout/unified_dimension.hh`

```cpp
namespace onyxui {

struct unified_dimension {
    double scale = 0.0;   // Relative (0.0 to 1.0)
    double offset = 0.0;  // Absolute (logical units)

    constexpr unified_dimension() noexcept = default;
    constexpr unified_dimension(double s, double o) noexcept;

    [[nodiscard]] constexpr logical_unit resolve(logical_unit parent_size) const noexcept {
        return logical_unit(parent_size.value * scale + offset);
    }

    // Factory methods
    static constexpr unified_dimension percent(double p) noexcept;
    static constexpr unified_dimension absolute(double units) noexcept;
    static constexpr unified_dimension from_right(double units) noexcept;
    static constexpr unified_dimension from_bottom(double units) noexcept;
    static constexpr unified_dimension centered(double offset_units = 0.0) noexcept;
};

} // namespace onyxui
```

#### 6.2 Add `size_policy::unified`

**File:** `include/onyxui/layout/layout_strategy.hh`

```cpp
enum class size_policy : uint8_t {
    fixed,
    content,
    expand,
    fill_parent,
    percentage,
    weighted,
    unified  // NEW
};

struct size_constraint {
    // ... existing fields ...

    // NEW: UDim support
    unified_dimension udim;

    [[nodiscard]] logical_unit resolve(logical_unit parent_size) const noexcept {
        switch (policy) {
            // ... existing cases ...
            case size_policy::unified:
                return udim.resolve(parent_size);
        }
    }
};
```

#### 6.3 Add Convenience APIs

**File:** `include/onyxui/core/element.hh`

```cpp
template<UIBackend Backend>
class ui_element {
public:
    // ... existing methods ...

    // NEW: UDim convenience setters
    void set_width_udim(double scale, double offset) {
        m_width_constraint.policy = size_policy::unified;
        m_width_constraint.udim = {scale, offset};
    }

    void set_height_udim(double scale, double offset) {
        m_height_constraint.policy = size_policy::unified;
        m_height_constraint.udim = {scale, offset};
    }

    // Position from edges
    void position_from_right(logical_unit offset);
    void position_from_bottom(logical_unit offset);
    void center_horizontally(logical_unit offset = logical_unit(0.0));
    void center_vertically(logical_unit offset = logical_unit(0.0));
};
```

**Tests:** `unittest/layout/test_unified_dimension.cc`
- Pure relative (scale=0.5, offset=0) = 50%
- Pure absolute (scale=0, offset=10) = 10 units
- Mixed (scale=0.5, offset=-5) = centered minus 5
- Edge positioning (from_right, from_bottom)
- Factory methods (percent, absolute, centered)
- Resolution accuracy (< 1e-9 error)

**Examples:**
```cpp
// Example 1: "5 units from right edge"
widget->set_width_udim(1.0, -5.0);

// Example 2: "Centered ±5 gap"
left_widget->set_x_udim(0.5, -5.0);
right_widget->set_x_udim(0.5, 5.0);

// Example 3: "33.33% width, exactly"
widget->set_width_udim(1.0/3.0, 0.0);  // No rounding loss!
```

### Deliverables

- [ ] `unified_dimension` implemented
- [ ] `size_policy::unified` supported in layout
- [ ] Convenience APIs added
- [ ] 50+ UDim tests passing
- [ ] Documentation with examples

### Exit Criteria

- ✅ All UDim tests pass
- ✅ Examples demonstrate common patterns
- ✅ Integration with existing layout strategies
- ✅ Performance negligible impact

---

## Phase 7: Polish

**Goal:** Finalization, documentation, performance tuning

**Duration:** 3 days

### Tasks

#### 7.1 Performance Optimization

**Tasks:**
1. Profile layout performance (before/after comparison)
2. Identify hot paths (likely in weighted distribution)
3. Optimize critical loops (if needed)
4. Verify < 5% overhead vs integer version

**Tools:**
- `perf` (Linux)
- `valgrind --tool=callgrind`
- Custom benchmark suite

**Benchmark:**
```cpp
// Benchmark: Layout 1000 widgets, 100 iterations
auto start = std::chrono::high_resolution_clock::now();
for (int i = 0; i < 100; ++i) {
    root->measure(800_lu, 600_lu);
    root->arrange({0_lu, 0_lu, 800_lu, 600_lu});
}
auto end = std::chrono::high_resolution_clock::now();
auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
std::cout << "Average: " << (duration.count() / 100) << " μs\n";
```

**Target:** < 5% slower than pre-migration integer version

#### 7.2 Documentation

**Update Files:**
1. `docs/ARCHITECTURE.md` - Add logical units section
2. `docs/CLAUDE.md` - Update coordinate system overview
3. `include/onyxui/core/types.hh` - Complete API docs
4. `include/onyxui/core/backend_metrics.hh` - Complete API docs
5. All widget headers - Update examples

**Create New:**
1. `docs/LOGICAL_UNITS_USER_GUIDE.md` - User-facing tutorial
2. `examples/logical_units_demo.cc` - Comprehensive example

#### 7.3 Migration Guide

**Create:** `docs/LOGICAL_UNITS_MIGRATION.md`

**Contents:**
- Overview of breaking changes
- API mapping (old → new)
- Common migration patterns
- Troubleshooting guide
- FAQ

#### 7.4 Visual Testing

**Tasks:**
1. Run widgets_demo on conio backend
2. Take screenshots before/after migration
3. Pixel-perfect comparison (should be identical)
4. Test on different terminal sizes (80×25, 120×40, 200×60)

#### 7.5 Code Review Prep

**Tasks:**
1. Run clang-tidy on all modified files
2. Run clang-format (code style consistency)
3. Verify zero compiler warnings
4. Check for TODO/FIXME comments
5. Update CHANGELOG.md

### Deliverables

- [ ] Performance < 5% overhead
- [ ] All documentation updated
- [ ] Migration guide complete
- [ ] Visual tests pass (pixel-perfect)
- [ ] Code review ready

### Exit Criteria

- ✅ Performance benchmarks meet target
- ✅ All docs updated and reviewed
- ✅ Migration guide tested by team member
- ✅ Visual tests 100% match
- ✅ Zero compiler warnings
- ✅ All tests pass (1600+ tests)

---

## Testing Requirements

### Unit Test Coverage

**Target:** > 95% line coverage for new code

**Categories:**
1. **Core types** (logical_unit, geometry) - 100+ tests
2. **Backend metrics** (conversion, snapping) - 150+ tests
3. **Layout** (strategies with logical units) - 200+ tests
4. **Rendering** (snapping, conversion) - 100+ tests
5. **Widgets** (all widgets migrated) - 1554+ tests (existing)
6. **UDim** (unified dimensions) - 50+ tests

**Total:** ~2150+ tests

### Integration Tests

**Test Suites:**
1. **End-to-end layout** - Complex nested layouts
2. **Backend consistency** - Same logical layout on conio vs test backend
3. **Precision** - Accumulation error tests
4. **Performance** - Benchmark regression tests

### Visual Tests

**Process:**
1. Generate reference screenshots (before migration)
2. Regenerate screenshots (after migration)
3. Pixel-perfect comparison (should be identical for conio)
4. Manual review for SDL backend (once implemented)

### Fuzz Testing

**Strategy:**
- Generate 10,000 random widget trees
- Random sizes, positions, constraints
- Verify no crashes, NaN, inf
- Check for overlaps or gaps

---

## Success Criteria

### Functional

- ✅ All 2150+ tests pass
- ✅ Zero compiler warnings (GCC, Clang, MSVC)
- ✅ Zero runtime errors (ASan, UBSan, TSan)
- ✅ widgets_demo runs flawlessly
- ✅ Visual output identical to pre-migration (conio)

### Performance

- ✅ Layout performance < 5% slower
- ✅ Memory usage < 20% increase
- ✅ No measurable rendering overhead

### Quality

- ✅ Code coverage > 95% for new code
- ✅ All public APIs documented
- ✅ Migration guide complete and tested
- ✅ Examples updated and working

### Precision

- ✅ Weighted distribution exact (error < 1e-9)
- ✅ Percentage layouts exact (3 × 33.33% = 100.0%)
- ✅ Nested layouts accumulate < 0.1 lu error
- ✅ Round-trip conversion < 0.01 lu error

---

## Risk Mitigation

### Risk: Performance Regression

**Mitigation:**
- Benchmark after each phase
- Profile hot paths
- Optimize critical loops
- Accept only < 5% overhead

### Risk: Precision Issues

**Mitigation:**
- Extensive fuzz testing
- Accumulation error tests
- Visual verification
- Unit tests with epsilon comparison

### Risk: Migration Complexity

**Mitigation:**
- Phased approach (one subsystem at a time)
- Comprehensive tests before migration
- Keep main branch stable (feature branch)
- Rollback plan (Git revert)

### Risk: Documentation Debt

**Mitigation:**
- Document while coding (not after)
- Update examples immediately
- Migration guide reviewed by team
- User guide with tutorials

---

## Rollout Plan

### Phase 7 Complete → Code Review

1. **Create PR:** `feature/logical-units` → `master`
2. **Review process:**
   - Architecture review
   - Code quality review
   - Performance review
   - Documentation review
3. **Address feedback**
4. **Final approval**

### Merge → Release

1. **Merge to master**
2. **Update version:** 3.0.0 (major version bump)
3. **Tag release:** `v3.0.0-logical-units`
4. **Update CHANGELOG.md**
5. **Announce breaking changes**

### Post-Release

1. **Monitor issues** (GitHub, user feedback)
2. **Quick fixes** (if needed)
3. **Performance tuning** (if regression detected)
4. **Documentation improvements** (based on feedback)

---

## Appendix: File Checklist

### New Files

- [ ] `include/onyxui/core/types.hh`
- [ ] `include/onyxui/core/geometry.hh`
- [ ] `include/onyxui/core/backend_metrics.hh`
- [ ] `include/onyxui/layout/unified_dimension.hh`
- [ ] `backends/conio/include/conio_backend_metrics.hh`
- [ ] `unittest/core/test_logical_units.cc`
- [ ] `unittest/core/test_logical_geometry.cc`
- [ ] `unittest/backend/test_backend_metrics.cc`
- [ ] `unittest/layout/test_layout_logical_units.cc`
- [ ] `unittest/layout/test_unified_dimension.cc`
- [ ] `unittest/rendering/test_render_context_logical.cc`
- [ ] `docs/LOGICAL_UNITS_DESIGN.md`
- [ ] `docs/LOGICAL_UNITS_IMPLEMENTATION_PLAN.md`
- [ ] `docs/LOGICAL_UNITS_MIGRATION.md`
- [ ] `docs/LOGICAL_UNITS_USER_GUIDE.md`
- [ ] `examples/logical_units_demo.cc`

### Modified Files

- [ ] `include/onyxui/core/element.hh`
- [ ] `include/onyxui/core/rendering/render_context.hh`
- [ ] `include/onyxui/core/rendering/draw_context.hh`
- [ ] `include/onyxui/core/rendering/measure_context.hh`
- [ ] `include/onyxui/layout/layout_strategy.hh`
- [ ] `include/onyxui/layout/linear_layout.hh`
- [ ] `include/onyxui/layout/grid_layout.hh`
- [ ] `include/onyxui/layout/anchor_layout.hh`
- [ ] `include/onyxui/layout/absolute_layout.hh`
- [ ] All widget headers (~30 files)
- [ ] All widget tests (~30 files)
- [ ] All examples (6 files)
- [ ] `CMakeLists.txt` (new test targets)
- [ ] `docs/ARCHITECTURE.md`
- [ ] `docs/CLAUDE.md`
- [ ] `CHANGELOG.md`

---

**Document Status:** Ready for Implementation
**Next Steps:** Begin Phase 0 (Preparation)
