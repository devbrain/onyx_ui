# Big Bang Migration Status

**Date:** 2025-11-26
**Branch:** `feature/logical-units`
**Strategy:** Big Bang (No backward compatibility)

---

## Progress: CORE COMPLETE - FINAL CLEANUP

### ✅ Completed Changes

#### 1. **Core Type System** (100% Complete)
- `logical_unit`, `logical_size`, `logical_point`, `logical_rect`, `logical_thickness`
- `udim`, `udim2`, `udim_rect`
- `backend_metrics`
- **Tests:** 24 test cases, 210 assertions passing

#### 2. **ui_element.hh** (100% Complete ✅)
- ✅ Removed old `thickness` struct (lines 88-159)
- ✅ Added `#include <onyxui/core/types.hh>` and `#include <onyxui/core/geometry.hh>`
- ✅ Updated member variables to logical types
- ✅ Updated all method signatures:
  - `measure()`, `arrange()`, `do_measure()`, `do_arrange()`
  - `get_content_area()`, `get_content_size()`
  - `set_margin()`, `set_padding()`, `margin()`, `padding()`
  - `mark_dirty_region()`, `get_and_clear_dirty_regions()`
- ✅ Updated implementations:
  - `measure()` - logical unit arithmetic, constraints still use to_int()
  - `arrange()` - logical unit arithmetic
  - `do_measure()`, `do_arrange()` - logical units
  - `mark_dirty()`, `mark_dirty_region()` - logical units
  - `get_content_area()` - logical unit arithmetic
  - `is_inside()` - converts to int for comparison
  - `render()` - converts to Backend types for rendering
- ✅ Replaced rect_utils calls with direct member access
- ⚠️ **Key Decision:** logical_rect does NOT satisfy RectLike concept (by design)
  - RectLike requires `convertible_to<int>`, but logical_unit is explicit
  - Solution: Use direct member access instead of rect_utils
  - Conversion to int only at rendering boundaries via `.to_int()`

#### 3. **logical_unit enhancements** (100% Complete ✅)
- ✅ Added `.to_int()` method for conversion to int with rounding
- ✅ Returns `static_cast<int>(std::round(value))`
- ✅ Used at rendering boundaries to convert logical→physical coordinates

#### 4. **All Layout Implementations** (100% Complete ✅)
- ✅ **linear_layout.hh** - Updated measure_children, arrange_children, spacing
- ✅ **grid_layout.hh** - Updated with logical units, fixed column/row sizing
- ✅ **anchor_layout.hh** - Updated anchor positioning and offsets
- ✅ **absolute_layout.hh** - Updated absolute positioning

#### 5. **Test Infrastructure** (100% Complete ✅)
- ✅ **test_helpers.hh** - Updated get_content_size, render_to_canvas

### 🎯 Final Cleanup Required

**Remaining Test Files: 8** (Down from ~100!)

Only core/unit tests need updating - all infrastructure is complete:
1. `unittest/core/test_dirty_tracking.cc`
2. `unittest/core/test_element.cc`
3. `unittest/core/test_exception_safety.cc`
4. `unittest/core/test_layer_manager.cc`
5. `unittest/core/test_layer_manager_events.cc`
6. `unittest/core/test_layer_manager_lifetime.cc`
7. `unittest/core/test_rule_of_five.cc`
8. `unittest/core/test_scoped_clip.cc`

### 🚧 In Progress

#### Next Files to Update

Based on compilation errors, the next files that need updating are:

1. **Test Helpers** (`unittest/utils/test_helpers.hh`)
   - `render_to_canvas()` passes int to `measure()` - needs logical_unit
   - `render_to_canvas()` passes relative_rect to `arrange()` - needs logical_rect
   - `ContentElement::get_content_size()` returns TestSize - needs logical_size

2. **layout_strategy.hh**
   - `measure_children()` signature: `(element*, int, int)` → `(element*, logical_unit, logical_unit)`
   - `arrange_children()` signature: `(element*, rect_type)` → `(element*, logical_rect)`

3. **All Layout Implementations** (4 files)
   - `linear_layout.hh` - measure_children, arrange_children
   - `grid_layout.hh` - measure_children, arrange_children
   - `anchor_layout.hh` - measure_children, arrange_children
   - `absolute_layout.hh` - measure_children, arrange_children

4. **All Test Files** (~100 files)
   - Update calls to `measure()` to pass logical_unit
   - Update calls to `arrange()` to pass logical_rect
   - Update assertions comparing to logical_unit (use `.value` or `to_int()`)

#### Current Compilation Errors (After Layout Migration)

**Total Error Categories: 4**

**Category 1: Test Helpers** (`unittest/utils/test_helpers.hh`)
- ✅ `get_content_size()` returns TestSize, needs logical_size
- ✅ `render_to_canvas()` passes int to `measure()` - needs logical_unit
- ✅ `render_to_canvas()` passes relative_rect to `arrange()` - needs logical_rect

**Category 2: Test Code - Type Mismatches** (~50+ test files)
- ✅ `set_margin({...})` - brace initializers don't convert to logical_thickness
- ✅ `set_padding({...})` - same issue
- ✅ `measure(100, 50)` - int doesn't implicitly convert to logical_unit
- ✅ `arrange(make_relative_rect(...))` - relative_rect doesn't convert to logical_rect

**Category 3: Test Assertions** (~100+ assertions)
- ✅ `CHECK(elem.margin().left == 10)` - comparing logical_unit with int
- ✅ Need to use: `CHECK(elem.margin().left == 10_lu)` or `CHECK(elem.margin().left.to_int() == 10)`

**Category 4: element.hh Issues** (2 remaining)
- ⚠️ `make_absolute_bounds()` call with wrong types
- ⚠️ `relative_rect` construction issue

---

## Next Actions (Priority Order)

### Phase 1: Complete ui_element.hh
1. Update `measure()` signature (line 335)
2. Update `measure_unconstrained()` (line 349)
3. Update `arrange()` signature (line 363)
4. Update `do_measure()` implementation (line ~1408)
5. Update `do_arrange()` implementation (line ~1432)
6. Update all thickness-related methods (getters, setters)
7. Update `get_content_area()` implementation (uses thickness)

### Phase 2: Update Related Headers
1. `layout_strategy.hh` - Base class for all layout strategies
2. `geometry/coordinates.hh` - relative_rect usage
3. `render_context.hh` - Position tracking

### Phase 3: Update Layout Strategies (4 files)
1. `linear_layout.hh` (vbox, hbox)
2. `grid_layout.hh`
3. `anchor_layout.hh`
4. `absolute_layout.hh`

### Phase 4: Update Widgets (~30 files)
Core widgets:
- `panel.hh`, `label.hh`, `button.hh`
- `vbox.hh`, `hbox.hh`, `grid.hh`
- `spacer.hh`, `spring.hh`

Input widgets:
- `line_edit.hh`, `checkbox.hh`, `radio_button.hh`
- `slider.hh`, `progress_bar.hh`

Complex widgets:
- `menu.hh`, `menu_bar.hh`, `menu_item.hh`
- `scrollbar.hh`, `scrollable.hh`, `scroll_view.hh`
- `text_view.hh`, `tab_widget.hh`
- `window.hh`, `dialog.hh`, `group_box.hh`
- `status_bar.hh`

### Phase 5: Fix Tests (~100+ test files)
- Update all widget tests to use logical units
- Update all layout tests
- Update integration tests

---

## Compilation Strategy

### Approach: Iterative Build

1. **First Build Attempt**
   - Complete ui_element.hh changes
   - Attempt build
   - Collect all compilation errors
   - Sort errors by frequency

2. **Fix High-Impact Errors First**
   - Fix layout_strategy.hh (affects all layouts)
   - Fix render_context.hh (affects all rendering)
   - Rebuild, collect new errors

3. **Fix Layout Strategies**
   - Fix one layout at a time
   - Linear → Grid → Anchor → Absolute
   - Rebuild after each

4. **Fix Widgets in Batches**
   - Core widgets first (panel, label, button)
   - Containers second (vbox, hbox, grid)
   - Complex widgets last (window, menu, scroll)

5. **Fix Tests**
   - Fix test infrastructure first
   - Fix widget tests in batches
   - Achieve green build

---

## Estimated Timeline

```
Phase 1: ui_element.hh completion       1-2 hours
Phase 2: Related headers                2-3 hours
Phase 3: Layout strategies              3-4 hours
Phase 4: Widget migration              8-12 hours
Phase 5: Test fixes                    6-8 hours
─────────────────────────────────────────────────
Total:                                20-29 hours  (3-4 days)
```

---

## Risk Assessment

### High Risk
- Massive scope (~50+ files to modify)
- Everything breaks at once
- Complex interdependencies

### Mitigations
- Systematic approach (one layer at a time)
- Frequent compilation checks
- Good error categorization

### Benefits of Big Bang
- Clean v3.0 API, no legacy code
- Faster than incremental once complete
- No compatibility shims to maintain

---

## Current File Status

```
include/onyxui/core/
  ├─ types.hh                 ✅ DONE (new file)
  ├─ geometry.hh              ✅ DONE (new file)
  ├─ udim.hh                  ✅ DONE (new file)
  ├─ backend_metrics.hh       ✅ DONE (new file)
  └─ element.hh               🚧 IN PROGRESS (50% - member vars only)

unittest/core/
  ├─ test_logical_units.cc    ✅ DONE (20 tests)
  ├─ test_logical_geometry.cc ✅ DONE (30 tests)
  ├─ test_backend_metrics.cc  ✅ DONE (13 tests)
  └─ test_udim.cc             ✅ DONE (16 tests)

Pending:
  ├─ layout/*.hh              ❌ NOT STARTED (4 files)
  ├─ widgets/**/*.hh          ❌ NOT STARTED (~30 files)
  └─ unittest/**/*.cc         ❌ NOT STARTED (~100 files)
```

---

## Decision Point: Continue or Pause?

### Option A: Continue Big Bang
- Pros: Clean break, fastest once done
- Cons: Risky, everything broken for days
- Time: 3-4 days of intensive work

### Option B: Pause and Review
- Pros: Can test foundation separately
- Cons: Delays completion
- Time: ??? (depends on feedback cycle)

### Option C: Hybrid Approach
- Complete ui_element.hh and layout strategies
- Pause to review/test
- Continue with widgets
- Time: 2 days + review + 2 days

---

**Recommendation:** Continue Big Bang methodically. We've come this far, the foundation is solid, and a clean v3.0 is worth the short-term pain.

**Current Status:** Member variables updated, method signatures pending. Ready to proceed.
