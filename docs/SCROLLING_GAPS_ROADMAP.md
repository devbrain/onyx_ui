# Scrolling System Gaps - Testable Roadmap

**Created**: 2025-10-29
**Updated**: 2025-10-29 (discovered clipping infrastructure already exists!)
**Status**: Planning Phase
**Goal**: Close all remaining gaps in the scrolling system to achieve 100% feature completion

---

## 🎉 Key Discovery

**Gap 1 is 90% complete!** The clipping infrastructure already exists:
- ✅ RenderLike concept includes `push_clip()` / `pop_clip()` / `get_clip_rect()`
- ✅ conio_backend has full VRAM clipping implementation
- ✅ test_canvas_backend has clip stack tracking
- ✅ All backends satisfy the concept

**What's missing**: Just using it in `scrollable.hh` (30 minutes of work!)

This reduces Gap 1 from **6 hours → 3 hours** and total effort from **22 hours → 19 hours**.

---

## 📋 Executive Summary

The scrolling system is functionally complete with **137 tests passing** and comprehensive coverage of:
- ✅ scroll_view, scrollable, scrollbar, scroll_controller
- ✅ Preset variants (modern, classic, compact, vertical-only)
- ✅ Bidirectional scrolling, wheel events, keyboard navigation
- ✅ Integration with layout system and focus management

**Remaining Gaps** (3 features):
1. **Viewport clipping** - Optimization to avoid rendering offscreen content ⚡ **90% done!**
2. **Scrollbar style rendering** - Visual variants (classic/compact/minimal)
3. **Scrollbar theme integration** - Use detailed theme properties for styling

**Estimated Effort**: 2.5 days (down from 3!)
**Risk**: Low (all are visual/optimization enhancements, not core functionality)
**Key Discovery**: Clipping infrastructure already exists - just need to use it!

---

## 🎯 Gap 1: Viewport Clipping ✅ COMPLETE

### Current State
- **Status**: ✅ **IMPLEMENTED** (2025-10-29)
- **Impact**: Content outside viewport is now clipped (optimal performance)
- **Behavior**: Scrolling works correctly with efficient viewport clipping

### Infrastructure Status ✅
- ✅ **RenderLike concept**: Already includes `push_clip()` / `pop_clip()` / `get_clip_rect()`
- ✅ **conio_backend**: Fully implemented with VRAM clipping (lines 376-392)
- ✅ **test_canvas_backend**: Has clip stack tracking (line 347-357)
- ✅ **test_backend**: Has stub implementations (line 271-272)

### Problem Statement
```cpp
// Current implementation in scrollable::do_render()
void do_render(render_context<Backend>& ctx) const override {
    // TODO Phase 2: Add viewport clipping via renderer.push_clip/pop_clip

    // Children render even if outside viewport!
    for (const auto& child : this->children()) {
        child->render(renderer, dirty_regions, theme, parent_style);
    }
}
```

**Gap**: The clipping API exists and works, but scrollable.hh doesn't use it yet!

### Implementation Steps

#### Step 1.1: Use Clipping in scrollable (30 minutes) ✨
**File**: `include/onyxui/widgets/scrollable.hh`

Replace the TODO section with actual clipping:

```cpp
void do_render(render_context<Backend>& ctx) const override {
    if (!ctx.is_rendering()) return;

    auto* renderer = ctx.renderer();
    if (!renderer) return;

    // Get viewport bounds (content area excluding scrollbars)
    auto const viewport_bounds = this->get_content_area();

    // Push clip region to viewport
    renderer->push_clip(viewport_bounds);

    // Render children (only visible portions will draw)
    for (const auto& child : this->children()) {
        // Calculate child position with scroll offset applied
        auto child_bounds = child->bounds();
        rect_utils::offset(child_bounds,
                          -point_utils::get_x(m_scroll_offset),
                          -point_utils::get_y(m_scroll_offset));

        // Child renders, clipped to viewport automatically
        child->render(*renderer, {}, ctx.theme(), ctx.style());
    }

    // Restore previous clip region
    renderer->pop_clip();
}
```

- Remove TODO comment at line 466
- **Test**: Visual verification that it compiles and runs

#### Step 1.2: Add Clipping Tests (2 hours)
**File**: `unittest/widgets/test_scrollable_clipping.cc` (NEW)

Create comprehensive tests:

```cpp
TEST_CASE("scrollable - Viewport clipping enabled") {
    // Verify push_clip/pop_clip are called
    // Create scrollable with 50 items (1000px total)
    // Viewport only 100px tall
    // Verify only visible items in clip region
}

TEST_CASE("scrollable - Clipping performance") {
    // Large content (1000 items)
    // Small viewport (100px)
    // Measure render time with clipping
}

TEST_CASE("scrollable - Nested clipping") {
    // Scrollable inside scrollable
    // Verify clip regions stack correctly
}

TEST_CASE("scrollable - Scroll offset affects clipping") {
    // Scroll down
    // Verify different items are visible
}
```

- **Test**: 4+ test cases for clipping behavior

#### Step 1.3: Verify conio Demo (30 minutes)
**File**: `backends/conio/main.cc`

Add large scrollable content to demo:
- Create scrollable with 100+ items
- Verify only visible items render
- Check performance improvement

**Total Time**: 3 hours (reduced from 6!)
**Tests Added**: 4+ test cases (reduced from 8)

---

## 🎨 Gap 2: Scrollbar Style Rendering

### Current State
- **Status**: Enum defined, TODO at `scrollbar.hh:186-188`
- **Impact**: All scrollbars look the same (minimal style)

### Scrollbar Styles
1. **classic** - Arrow buttons at both ends (Windows style)
2. **compact** - Arrow buttons at one end only  
3. **minimal** - No arrow buttons (macOS/mobile style)

### Implementation Steps

#### Step 2.1: Define scrollbar_layout struct (1 hour)
**File**: `include/onyxui/widgets/scrollbar.hh`
- Create `scrollbar_layout<Backend>` with track/thumb/arrow rects
- **Test**: Compile-time verification

#### Step 2.2: Implement calculate_layout() (3 hours)
**File**: `include/onyxui/widgets/scrollbar.hh`
- Handle classic/compact/minimal styles
- Calculate component rectangles based on style
- **Test**: 6 test cases for layout calculation

#### Step 2.3: Implement Styled Rendering (3 hours)
**File**: `include/onyxui/widgets/scrollbar.hh`
- Replace TODO comments with actual rendering
- Draw track, thumb, and arrows based on style
- **Test**: 4 test cases for visual verification

#### Step 2.4: Add Arrow Button Interaction (2 hours)
**File**: `include/onyxui/widgets/scrollbar.hh`
- Handle mouse clicks on arrow buttons
- Emit scroll line up/down signals
- **Test**: 3 test cases for arrow interaction

**Total Time**: 9 hours
**Tests Added**: 13+ test cases

---

## 🎨 Gap 3: Scrollbar Theme Integration

### Current State
- **Status**: Theme struct defined at `theme.hh:170-199`, but mostly unused
- **Impact**: Scrollbars don't respect theme colors/styles

### Implementation Steps

#### Step 3.1: Use Theme Colors in Rendering (3 hours)
**File**: `include/onyxui/widgets/scrollbar.hh`
- Render track/thumb/arrows with theme colors
- Implement state-based styling (normal/hover/pressed/disabled)
- Render arrow glyphs with theme colors
- **Test**: 3 test cases for color usage

#### Step 3.2: Use Theme Geometry (1 hour)
**File**: `include/onyxui/widgets/scrollbar.hh`
- Use `theme->scrollbar.width` in `do_measure()`
- Use `theme->scrollbar.min_thumb_size` in thumb calculation
- **Test**: 2 test cases for geometry

#### Step 3.3: Update Theme YAML Files (1 hour)
**Files**: `themes/*.yaml`
- Add scrollbar properties to all theme files
- Define colors for track/thumb/arrows in all states
- **Test**: Theme loading tests

#### Step 3.4: Add Theming Tests (2 hours)
**File**: `unittest/widgets/test_scrollbar_theming.cc` (NEW)
- Test theme color usage
- Test state-based styling
- Test geometry properties
- **Test**: 8+ test cases

**Total Time**: 7 hours
**Tests Added**: 13+ test cases

---

## 📊 Testing Summary

### New Test Files
1. `unittest/widgets/test_scrollable_clipping.cc` - Viewport clipping (4 cases) ⚡ **Simplified**
2. `unittest/widgets/test_scrollbar_styles.cc` - Style rendering (13 cases)
3. `unittest/widgets/test_scrollbar_theming.cc` - Theme integration (13 cases)

### Test Count Impact
- **Before**: 1001 tests, 5546 assertions
- **After**: ~1031 tests (+30), ~5640 assertions (+94)
- **Note**: Gap 1 tests simplified since infrastructure already exists

### Test Execution
```bash
# Run all tests
./build/bin/ui_unittest

# Run only scrolling tests
./build/bin/ui_unittest --test-case="*scroll*"
```

---

## 🎯 Implementation Timeline

| Day | Task | Hours | Tests | Notes |
|-----|------|-------|-------|-------|
| **Day 1** | Gap 1: Viewport Clipping | **3** ⚡ | +4 | Infrastructure exists! Just use it |
| **Day 2** | Gap 2: Scrollbar Styles | 9 | +13 | Layout + rendering + interaction |
| **Day 3** | Gap 3: Theme Integration | 7 | +13 | Colors + geometry + states |
| **Total** | **All Gaps Closed** | **19** | **+30** | 3 hours saved on Gap 1! |

---

## ✅ Acceptance Criteria

### Gap 1: Viewport Clipping ✅ COMPLETE
- [x] RenderLike concept includes `push_clip()` / `pop_clip()`
- [x] test_canvas_backend implements clipping
- [x] conio_backend implements clipping
- [x] scrollable uses clipping for viewport
- [x] TODO removed from scrollable.hh:466
- [x] 6 new tests added (test_scrollable_clipping.cc), all passing
- [x] Demo builds successfully with clipping enabled

### Gap 2: Scrollbar Styles ✓
- [ ] `scrollbar_layout` struct defined
- [ ] `calculate_layout()` handles all 3 styles
- [ ] Rendering draws track/thumb/arrows correctly
- [ ] Arrow buttons are interactive
- [ ] TODO removed from scrollbar.hh:186-188
- [ ] 13+ new tests pass
- [ ] Visual verification in demo

### Gap 3: Scrollbar Theming ✓
- [ ] All theme properties used in rendering
- [ ] State-based styling works
- [ ] Theme geometry respected (width, min_thumb_size)
- [ ] All YAML files updated
- [ ] 13+ new tests pass
- [ ] Themes look visually distinct

### Overall ✓
- [ ] All 1035+ tests pass
- [ ] No regressions
- [ ] Code coverage >90%
- [ ] Documentation updated
- [ ] All TODO comments removed
- [ ] Demo works in conio

---

## 🔗 Implementation Priority

**Start Here**: Gap 1 (Viewport Clipping)
- Largest performance impact
- Foundational for optimization
- Affects all scrollable content

**Then**: Gap 2 (Scrollbar Styles)
- Visual improvement
- User-facing feature
- Enhances UX

**Finally**: Gap 3 (Theme Integration)
- Polish and refinement
- Completes theming system
- Professional appearance

---

## 📝 Risk Mitigation

### Low Risk Items
- All gaps are **additive** (no breaking changes)
- Existing functionality **unaffected**
- Tests prevent regressions

### Testing Strategy
1. Run full suite after each gap
2. Visual verification in conio demo
3. Performance benchmarks for clipping

### Rollback Plan
- Each gap is independent
- Can merge incrementally
- Easy to revert individual features

---

## 🚀 Next Steps

1. **Review this roadmap** with stakeholders
2. **Approve implementation timeline**
3. **Start with Gap 1** (viewport clipping)
4. **Iterate through Gaps 2 & 3**
5. **Final testing and demo**
6. **Update TODO.md** to mark gaps closed

---

**Ready to close all scrolling gaps and achieve 100% feature completion!**
