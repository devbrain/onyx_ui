# OnyxUI TODO List

This document tracks non-implemented features, placeholders, and future enhancements for OnyxUI.

**Last Updated**: 2025-10-30
**Status**: 1039 tests passing, 5636 assertions

---

## 🔴 High Priority (Breaks Documented Features)

### Widgets

- [ ] **textbox** - Text input widget
  - Mentioned in `/docusaurus/docs/api-reference/widget-library.md` line 32
  - Status: Not implemented
  - Priority: HIGH - documented as available but missing
  - Dependencies: Text cursor rendering, keyboard input handling, text selection

---

## 🟡 Medium Priority (Incomplete Features)

### Layout System

- [ ] **size_policy::percentage** - Percentage-based sizing
  - Location: `include/onyxui/layout/layout_strategy.hh` (defined)
  - Status: Defined in enum but not implemented in any layout
  - Implementation needed in:
    - `linear_layout.hh` - Horizontal/vertical layouts
    - `grid_layout.hh` - Grid layouts
    - `anchor_layout.hh` - Anchor layouts
  - Test: `unittest/layout/test_linear.cc:203` has TODO placeholder
  - Expected behavior: Child with `{size_policy::percentage, 0, 0, max, 0.5F}` takes 50% of parent

### Scrolling System

- [ ] **Scrollbar arrow glyphs** - Render arrow symbols in arrow buttons
  - Location: `include/onyxui/widgets/containers/scroll/scrollbar.hh:392,399`
  - Status: Arrow buttons render with themed styles but no arrow glyphs
  - Current: Boxes render correctly, but no up/down/left/right symbols
  - Implementation: Use renderer to draw arrow characters or glyphs
  - Depends on: Backend icon/glyph rendering support

### Widget Rendering

- [ ] **group_box title** - Title inset into top border
  - Location: `include/onyxui/widgets/containers/group_box.hh:270`
  - Status: TODO comment
  - Current: Border renders, title does not
  - Test: `unittest/widgets/test_group_box_layout.cc:95` has TODO

- [ ] **status_bar content** - Actual status bar rendering
  - Location: `include/onyxui/widgets/status_bar.hh:182`
  - Status: Rendering stub - currently empty
  - Current: Status bar exists but renders nothing

---

## 🟢 Low Priority (Nice to Have)

### Missing Widgets

These widgets are mentioned in examples/comments but not implemented:

- [ ] **checkbox** - Checkable box with label
  - Mentioned in: Scrolling guide examples (docs/scrolling_guide.md:187)
  - Pattern exists: `action` has checkable behavior
  - Implementation: Widget that uses `action` internally

- [ ] **slider** - Value selection via dragging
  - Mentioned in: Scrolling guide examples (docs/scrolling_guide.md:191)
  - Use case: Volume, brightness, font size controls
  - Implementation: Track + draggable thumb widget

- [ ] **Radio buttons** - Mutually exclusive selection
  - Behavior exists: `action_group` provides mutual exclusivity
  - Missing: Visual radio button widget
  - Implementation: Widget using `action_group` internally

- [ ] **combobox** / **dropdown** - Select from list
  - Use case: Option selection from many choices
  - Implementation: Button + popup menu hybrid

- [ ] **listbox** - Scrollable list of selectable items
  - Use case: File lists, setting selections
  - Implementation: scroll_view + selectable list items

### Hotkey Schemes

Additional keyboard layouts for different user preferences:

- [ ] **Emacs scheme**
  - Location: `include/onyxui/hotkeys/builtin_hotkey_schemes.hh:154-158`
  - Bindings:
    - `Ctrl+X Ctrl+C` for quit
    - Meta (Alt) based navigation
    - Standard Emacs key chords

- [ ] **Vim scheme**
  - Location: `include/onyxui/hotkeys/builtin_hotkey_schemes.hh:160-163`
  - Bindings:
    - `j`/`k` for up/down
    - `hjkl` navigation
    - `ESC` to exit modes

- [ ] **macOS scheme**
  - Location: `include/onyxui/hotkeys/builtin_hotkey_schemes.hh:165-167`
  - Bindings:
    - `Cmd` instead of `Ctrl`
    - `Option` instead of `Alt`
    - macOS conventions

- [ ] **Minimal scheme**
  - Location: `include/onyxui/hotkeys/builtin_hotkey_schemes.hh:169-171`
  - Bindings:
    - Only essential keys (Enter, Escape)
    - Primarily mouse-driven

### Renderer Features

These are placeholders in the rendering system:

- [ ] **draw_line()** - Line drawing support
  - Location: `include/onyxui/core/rendering/draw_context.hh:284`
  - Status: Placeholder - "Most renderers don't have draw_line yet"
  - Current: Returns without drawing
  - Use cases: Separators, underlines, custom graphics

- [ ] **draw_icon()** - Icon rendering support
  - Location: `include/onyxui/core/rendering/draw_context.hh:300`
  - Status: Placeholder - "Not all renderers support icons yet"
  - Current: Limited support in test backends
  - Use cases: Menu item icons, toolbar buttons

- [ ] **Animation system** - Smooth transitions
  - Location: `include/onyxui/services/layer_manager.hh:209`
  - Status: `enable_animations = false` - TODO comment
  - Use cases:
    - Scrollbar fade in/out (theme has `fade_duration_ms`)
    - Menu slide animations
    - Popup transitions

---

## 📊 Test Coverage Gaps

Based on `docs/unittest-review.md` recommendations:

### Integration Tests Needed

- [ ] **Theme switching + rendering verification**
  - Test that switching themes updates rendered output correctly
  - Use mock renderer to verify colors/styles change

- [ ] **Modal dialog focus behavior**
  - Test focus "stealing" when modal opens
  - Test focus restoration when modal closes

- [ ] **Complete ui_handle lifecycle test**
  - Create ui_context
  - Set up ui_handle with root widget
  - Process input events (mouse, keyboard)
  - Verify UI state changes
  - Call display() and verify rendering

- [ ] **Layer manager + input manager integration**
  - Click-outside-to-close behavior
  - Event propagation through layers
  - Layer z-order and hit testing

- [ ] **Hotkeys and focus integration**
  - Global hotkeys work regardless of focus
  - Widget-specific hotkeys only active when focused

---

## 🔧 Code Quality Improvements

### Low-Value Improvements (Skip for now)

- [ ] **Refactor layout tests to use fixtures**
  - Impact: Would save ~200 lines of boilerplate
  - Cost: Risk of introducing bugs, time-consuming
  - Decision: Apply fixtures to NEW tests only (already done for scrolling tests)
  - Status: DEFER - existing tests work fine

---

## ✅ Recently Completed

- [x] **Scrollbar line increment from theme** - Arrow clicks use theme value - 2025-10-30
- [x] **Directory reorganization** - Logical include/ structure - 2025-10-30
- [x] **Viewport clipping** - scoped_clip RAII guard for scrollable - 2025-10-30
- [x] **Scrollbar theme integration** - All theme properties in use - 2025-10-29
- [x] **Scrollbar style rendering** - classic/compact/minimal with arrows - 2025-10-29
- [x] Comprehensive scrolling system (137 tests) - 2025-10-29
- [x] scroll_view, scrollable, scrollbar, scroll_controller - 2025-10-29
- [x] Preset scroll view variants (modern, classic, compact, vertical-only) - 2025-10-29
- [x] Scrolling documentation (guide + API references) - 2025-10-29
- [x] Fix misleading percentage sizing placeholder test - 2025-10-29
- [x] Hotkey scheme system (Windows, Norton Commander) - 2025-10
- [x] Theme System v2.0 refactoring - 2025-10
- [x] Stateless rendering architecture - 2025-10

---

## 📝 Notes

### Implementation Priority Guidelines

1. **HIGH**: Features documented as available but missing (breaks user expectations)
2. **MEDIUM**: Partially implemented features with visible TODOs (technical debt)
3. **LOW**: Nice-to-have features mentioned in examples/comments

### When to Implement

- **Immediately**: HIGH priority items (textbox)
- **As needed**: MEDIUM priority when refactoring related code
- **Future**: LOW priority when requested by users or needed for specific use cases

### Testing Philosophy

- Fix tests reactively when bugs are found
- Add integration tests when working on related systems
- Don't refactor working tests just for cleanliness
- Apply best practices (fixtures) to NEW tests only

---

## 🔗 Related Documents

- `docs/unittest-review.md` - Test suite quality review
- `docs/scrolling_guide.md` - Scrolling system user guide
- `CLAUDE.md` - Development guidelines and architecture
- `unittest/layout/test_linear.cc:203` - Percentage sizing TODO

---

**Last verified against codebase: 2025-10-30**
