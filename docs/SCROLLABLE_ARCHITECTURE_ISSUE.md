# Scrollable/Text_View Coordinate System Architecture Issue

**Status:** Identified - Awaiting Refactor
**Severity:** High - Causes content display corruption
**Components:** `scrollable`, `text_view`, coordinate system, arrangement pipeline
**Date:** 2025-11-04

---

## Executive Summary

The current architecture has a **fundamental design conflict** between `scrollable`'s scroll offset management and widgets that need to pre-arrange their children for sizing purposes (like `text_view`). This results in:

1. **Content displaying at wrong scroll position** (shows end instead of beginning)
2. **Text corruption** (truncated/garbage characters)
3. **Missing content** (only one line visible instead of full scrollable area)

The root cause is **double-arrangement with conflicting coordinate systems**: pre-arrangement sets absolute coordinates that conflict with scrollable's scroll-offset-based coordinates.

---

## Current Problem

### Observable Symptoms

Running `widgets_demo` with a bordered `text_view` shows:

```
┌──────────────────────────────────────┐
│                                      │
│ [LOG 15] Entry at timestamp 2500 msdapt.y,
│                                      │
└──────────────────────────────────────┘
```

**Expected:**
```
┌──────────────────────────────────────┐
│                                      │
│ Welcome to OnyxUI Text View Demo!   │
│                                      │
│ This widget demonstrates:            │
│   * Multi-line text display          │
│   * Automatic scrolling              │
└──────────────────────────────────────┘
```

### Failing Tests

- `text_view - Initial scroll position (diagnose LOG 15 issue)` - FAILS
- `text_view - Multi-line content display` - FAILS
- `text_view - Basic text display` - FAILS
- 6 out of 19 text_view tests fail

---

## Root Cause Analysis

### The Conflict

1. **text_view's Pre-Arrange Need**
   ```cpp
   // text_view::create_scroll_view() needs to know content size
   auto panel_size = content_container_ptr->measure_unconstrained();
   content_container_ptr->arrange({0, 0, panel_size.w, panel_size.h});
   // ↑ This arranges the panel AND all nested label children
   ```

2. **scrollable's Re-Arrange**
   ```cpp
   // scrollable::do_arrange() re-arranges with scroll offset
   void do_arrange(const rect_type& final_bounds) override {
       base::do_arrange(final_bounds);  // First arrangement

       // Second arrangement with scroll offset
       for (auto& child : this->children()) {
           child->arrange({x - scroll_x, y - scroll_y, w, h});  // Overwrites!
       }
   }
   ```

3. **Result: Coordinate Chaos**
   - Panel's children (labels) keep their pre-arranged absolute positions (e.g., `{1, 0}`, `{1, 1}`)
   - Panel itself gets re-arranged to `{base_x - scroll_x, base_y - scroll_y, w, h}`
   - Label positions are no longer relative to parent → **rendering at wrong locations**

### Attempted Fixes (Why They Didn't Work)

#### ❌ Fix #1: Remove base::do_arrange() from scrollable
```cpp
void do_arrange(const rect_type& final_bounds) override {
    // DON'T call base::do_arrange()
    // Arrange children manually
}
```
**Result:** Didn't fix the issue because text_view's pre-arrange still sets stale coordinates

#### ❌ Fix #2: Remove text_view's pre-arrange
```cpp
// Just add content without pre-arrange
scroll_view_ptr->add_child(std::move(content_container_ptr));
```
**Result:** Content disappears because scrollable can't measure unmeasured children

#### ❌ Fix #3: Measure without arrange
```cpp
scroll_view_ptr->add_child(std::move(content_container_ptr));
scroll_view_ptr->measure_unconstrained();  // Trigger measurement
scroll_view_ptr->scroll_to(0, 0);
```
**Result:** Still shows wrong content (LOG 15 instead of Welcome)

---

## Current Architecture

### Layout Pipeline

```
┌──────────────────────────────────────────────────────────────┐
│ 1. measure()                                                  │
│    ├─ Widget measures children with available space          │
│    ├─ Returns desired size                                   │
│    └─ Caches m_desired_size                                  │
├──────────────────────────────────────────────────────────────┤
│ 2. arrange(final_bounds)                                      │
│    ├─ Sets m_bounds = final_bounds                           │
│    ├─ Calls do_arrange(final_bounds)                         │
│    │   ├─ Layout strategy arranges children                  │
│    │   └─ Each child->arrange(child_bounds)  [RECURSIVE]     │
│    └─ Marks layout_state as valid                            │
├──────────────────────────────────────────────────────────────┤
│ 3. render(renderer, ...)                                      │
│    ├─ Renders widget's own content (borders, etc)            │
│    └─ Renders children at their arranged bounds              │
└──────────────────────────────────────────────────────────────┘
```

### scrollable's Current Behavior

```cpp
// scrollable::do_measure() - works correctly
size_type do_measure(int available_width, int available_height) override {
    // Measure children with UNCONSTRAINED size
    for (auto& child : children()) {
        child_size = child->measure(UNCONSTRAINED, UNCONSTRAINED);
        m_content_size = max(m_content_size, child_size);
    }
    return m_content_size;  // Return total scrollable size
}

// scrollable::do_arrange() - PROBLEM AREA
void do_arrange(const rect_type& final_bounds) override {
    base::do_arrange(final_bounds);  // ← Arranges via layout strategy

    // Calculate scroll offset position
    int child_x = base_x - scroll_x;
    int child_y = base_y - scroll_y;

    // Re-arrange children with scroll offset
    for (auto& child : children()) {
        child->arrange({child_x, child_y, content_w, content_h});  // ← OVERWRITES
    }
}
```

**Problem:** Children get arranged **twice**:
1. First by `base::do_arrange()` via layout strategy
2. Second with scroll offset

If children have nested descendants (like text_view's panel with labels), the first arrangement sets their positions, and the second arrangement only moves the parent, leaving descendants with stale positions.

---

## Proposed Solutions

### Solution 1: Single-Pass Arrangement (Recommended)

**Concept:** `scrollable` should ONLY arrange children once, directly with scroll offset.

**Changes:**

1. **scrollable::do_arrange()** - Don't call base::do_arrange()
   ```cpp
   void do_arrange(const rect_type& final_bounds) override {
       // Store viewport size
       auto content_area = get_content_area();
       m_viewport_size = {width(content_area), height(content_area)};

       if (children().empty()) return;

       // Calculate child position with scroll offset
       int child_x = get_x(content_area) - get_x(m_scroll_offset);
       int child_y = get_y(content_area) - get_y(m_scroll_offset);

       // Arrange children ONCE with scroll offset baked in
       for (auto& child : children()) {
           rect_type child_bounds{
               child_x, child_y,
               get_width(m_content_size),
               get_height(m_content_size)
           };
           child->arrange(child_bounds);  // Single arrangement!
       }
   }
   ```

2. **Remove all pre-arrange calls** from text_view
   ```cpp
   void create_scroll_view() {
       // Create content
       auto panel = make_unique<panel<Backend>>();
       for (const auto& line : m_lines) {
           panel->add_child(make_unique<label<Backend>>(line));
       }

       // Add to scroll_view WITHOUT pre-arrange
       scroll_view->add_child(std::move(panel));

       // Measurement will happen naturally during layout pipeline
   }
   ```

**Pros:**
- Single source of truth for child positions
- No coordinate conflicts
- Clean, simple logic

**Cons:**
- Requires updating all widgets that currently rely on base::do_arrange()

---

### Solution 2: Defer Pre-Arrange Until After Measurement

**Concept:** Allow pre-arrangement but ensure it happens at the right time.

**Changes:**

1. **Add lazy arrangement flag** to scrollable
   ```cpp
   class scrollable {
       bool m_needs_child_arrangement = false;

       void do_arrange(const rect_type& final_bounds) override {
           base::do_arrange(final_bounds);

           // Only re-arrange if needed (after scroll changes)
           if (m_needs_child_arrangement) {
               // Arrange with scroll offset
               m_needs_child_arrangement = false;
           }
       }
   };
   ```

2. **text_view defers pre-arrange**
   ```cpp
   void create_scroll_view() {
       // Create content WITHOUT pre-arrange
       scroll_view->add_child(std::move(panel));

       // Trigger measurement (not arrangement)
       scroll_view->invalidate_measure();
   }
   ```

**Pros:**
- Minimal changes to existing code
- Preserves current behavior for non-scrolling widgets

**Cons:**
- More complex state management
- Still has timing dependencies

---

### Solution 3: Introduce Virtual Coordinate System

**Concept:** Children are arranged in "virtual" coordinates (0-based), and scrollable translates to "viewport" coordinates during rendering.

**Changes:**

1. **scrollable arranges in virtual space**
   ```cpp
   void do_arrange(const rect_type& final_bounds) override {
       base::do_arrange(final_bounds);

       // Children are arranged at (0, 0) to (content_w, content_h)
       // NO scroll offset applied during arrangement
   }
   ```

2. **scrollable::do_render() applies scroll offset**
   ```cpp
   void do_render(render_context& ctx) const override {
       // Create viewport clip
       auto viewport = get_content_area();
       scoped_clip clip(ctx, viewport);

       // Translate render position by scroll offset
       auto translated_pos = ctx.position();
       translated_pos.x -= m_scroll_offset.x;
       translated_pos.y -= m_scroll_offset.y;

       // Render children with translated position
       for (auto& child : children()) {
           child->render(ctx, ..., translated_pos);
       }
   }
   ```

**Pros:**
- Clean separation: arrangement = logical, rendering = visual
- No coordinate conflicts
- Matches how many GUI frameworks work (virtual canvas)

**Cons:**
- **Major architectural change** to rendering pipeline
- Requires updating render_context and all rendering code
- May break assumptions in existing widgets

---

## Recommended Implementation Plan

### Phase 1: Fix scrollable (Solution 1)

1. **Update scrollable::do_arrange()**
   - Remove `base::do_arrange()` call
   - Arrange children directly with scroll offset
   - Test with simple scrollable content (non-nested widgets)

2. **Update text_view**
   - Remove all pre-arrange calls
   - Rely on natural measurement flow
   - Add tests to verify content displays correctly

3. **Update documentation**
   - Document new arrangement behavior in scrollable
   - Update scrolling guide with new architecture

**Estimated Effort:** 2-4 hours
**Risk:** Medium (may break existing scroll_view usage)

### Phase 2: Validate and Fix Regressions

1. **Run full test suite**
   - Identify all broken tests
   - Fix any widgets that relied on base::do_arrange()

2. **Manual testing**
   - Test all scroll_view presets
   - Test scrollable with various content types
   - Test nested scrolling scenarios

**Estimated Effort:** 2-3 hours
**Risk:** Low (tests will catch issues)

### Phase 3: Consider Virtual Coordinates (Future)

If Phase 1-2 proves insufficient or causes too many regressions, consider implementing Solution 3 (Virtual Coordinate System). This is a larger refactor but provides the cleanest long-term architecture.

**Estimated Effort:** 8-16 hours
**Risk:** High (major architectural change)

---

## Testing Strategy

### Unit Tests to Add

1. **scrollable arrangement test**
   ```cpp
   TEST_CASE("scrollable - Single arrangement with scroll offset") {
       auto scrollable = make_unique<scrollable<Backend>>();
       auto child = make_unique<panel<Backend>>();

       scrollable->add_child(std::move(child));
       scrollable->measure(100, 100);
       scrollable->arrange({0, 0, 100, 100});

       // Verify child was only arranged once
       // Verify child position includes scroll offset
   }
   ```

2. **text_view content display test**
   ```cpp
   TEST_CASE("text_view - Shows beginning content, not end") {
       auto view = make_unique<text_view<Backend>>();
       view->set_text("Line 1\nLine 2\n...\nLine 100");

       auto canvas = render_to_canvas(*view, 40, 10);

       // Should see Line 1, NOT Line 100
       CHECK(canvas->contains_text("Line 1"));
       CHECK_FALSE(canvas->contains_text("Line 100"));
   }
   ```

3. **Nested scrolling test**
   ```cpp
   TEST_CASE("scrollable - Nested content renders correctly") {
       auto outer = make_unique<scrollable<Backend>>();
       auto inner_panel = make_unique<panel<Backend>>();

       // Add many labels to inner panel
       for (int i = 0; i < 50; ++i) {
           inner_panel->add_child(make_unique<label<Backend>>("Line " + to_string(i)));
       }

       outer->add_child(std::move(inner_panel));

       auto canvas = render_to_canvas(*outer, 40, 10);

       // Verify first lines are visible, not last
       CHECK(canvas->contains_text("Line 0"));
       CHECK_FALSE(canvas->contains_text("Line 49"));
   }
   ```

### Manual Test Cases

1. **widgets_demo** - text_view should show "Welcome" message at top
2. **Scroll to bottom** - After scrolling, should show LOG 15 at bottom
3. **Theme switching** - Content should remain at correct scroll position
4. **Resize window** - Content should reflow but maintain scroll position ratio

---

## Impact Analysis

### Components Affected

- `scrollable.hh` - Core arrangement logic
- `text_view.hh` - Remove pre-arrange calls
- `scroll_view.hh` - May need updates if it relies on scrollable behavior
- `scroll_view_presets.hh` - Verify presets still work

### Widgets That May Break

Any widget that:
1. Pre-arranges children before adding to scrollable
2. Relies on `base::do_arrange()` being called in scrollable
3. Assumes children are arranged via layout strategy

**Action:** Search codebase for:
```bash
grep -r "scrollable" include/onyxui/widgets/
grep -r "arrange.*{.*0.*,.*0" include/onyxui/widgets/  # Pre-arrange patterns
```

### API Compatibility

**Breaking Changes:**
- `scrollable::do_arrange()` behavior changes
- Widgets can no longer pre-arrange children added to scrollable

**Mitigation:**
- Add deprecation warning for pre-arrange pattern
- Update all in-tree widgets
- Document migration in CHANGELOG.md

---

## Related Issues

### Fixed Issues (Border Rendering)

The border rendering bug has been **completely fixed** by:
1. Adding border drawing support to `widget_container`
2. Implementing text clipping in `test_canvas_backend`
3. Fixing clip rect intersection

### Remaining Issues (This Document)

1. **Content displaying at wrong scroll position** - PRIMARY
2. **Text corruption/truncation** - Side effect of #1
3. **Missing content lines** - Side effect of #1

---

## References

### Code Locations

- `include/onyxui/widgets/containers/scroll/scrollable.hh:406-463` - scrollable::do_arrange()
- `include/onyxui/widgets/text_view.hh:320-357` - text_view::create_scroll_view()
- `include/onyxui/core/element.hh:1080-1260` - Base arrange() implementation
- `unittest/widgets/test_scroll_view_keyboard.cc:49-88` - Failing LOG 15 test

### Documentation

- `docs/scrolling_guide.md` - Scrolling system overview
- `docs/CLAUDE/ARCHITECTURE.md` - Layout algorithm documentation
- `docs/REFACTORING_PLAN.md` - Code quality improvements

### Related Discussions

- GitHub Issue: "text_view shows wrong content" (to be created)
- Original bug report: Border rendering corruption (FIXED)

---

## Conclusion

The scrollable/text_view coordination issue is a **fundamental architectural problem** requiring refactoring of the arrangement pipeline. The recommended solution is:

1. **Short-term:** Implement Solution 1 (Single-Pass Arrangement)
2. **Long-term:** Consider Solution 3 (Virtual Coordinates) if needed

The border rendering bug that was the original issue has been completely fixed. The content display issue is separate and requires the architectural changes outlined in this document.

**Next Steps:**
1. Create GitHub issue tracking this work
2. Implement Solution 1 in a feature branch
3. Run tests and validate no regressions
4. Update documentation

---

---

## Implementation Log

### 2025-11-04 - Solution 1 Implemented (Partial Success)

**Changes Made:**
- Updated `scrollable.hh:406-455` to implement single-pass arrangement
- Removed nested/absolute coordinate heuristic
- Children now arranged once with scroll offset baked in
- No `base::do_arrange()` call

**Results:**
- ✅ Border rendering: Still perfect (original bug remains fixed)
- ✅ Border integrity test: PASSES
- ❌ Content display: **No improvement**
- ❌ Overall tests: 1204/1213 pass (9 failures, was 8 before - 1 regression)

**Screenshot Evidence:**
```
Line 23: │ [LOG 15] Entry at timestamp 2500 msdapt.y,
```
- Still shows LOG 15 (end) instead of "Welcome" (beginning)
- Still only one corrupted line visible
- Single-pass arrangement didn't resolve the issue

**Analysis:**
The issue is **deeper than double-arrangement**. Possible causes:

1. **Scroll position calculation wrong** - `scroll_to(0, 0)` may not work correctly
2. **Content measurement issue** - `m_content_size` may not be calculated properly
3. **Viewport/content size mismatch** - Arrangement uses wrong dimensions
4. **Label positions** - Individual labels may have incorrect bounds

**Next Steps:**
1. Debug `scrollable::scroll_to()` to verify it actually scrolls to (0,0)
2. Add logging to see actual `m_content_size`, `m_viewport_size`, `m_scroll_offset`
3. Verify that `vbox` layout is measuring all child labels correctly
4. Check if labels are being created at all (might be a `create_scroll_view()` issue)

**Status:** Solution 1 implemented but **insufficient** - requires deeper investigation.

---

**Document Version:** 1.1
**Last Updated:** 2025-11-04 (Implementation log added)
**Author:** Claude (via architectural analysis)
**Reviewers:** (awaiting human review)
