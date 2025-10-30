# OnyxUI TODO List

This document tracks non-implemented features, placeholders, and future enhancements for OnyxUI.

**Last Updated**: 2025-10-30
**Status**: 1137 tests passing, 6079 assertions

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

### Theming System Enhancements (October 2025)

**Phases 1-4: 97% Theme Size Reduction (300 lines → 9 lines)**

- [x] **Phase 1: Hex Color Notation** - 2025-10-30
  - Write colors as `0xFF0000` instead of `{r: 255, g: 0, b: 0}`
  - 60% size reduction, easier to read and maintain
  - Tests: 22 test cases in `test_color_yaml.cc`

- [x] **Phase 2: Color Palette System** - 2025-10-30
  - Define colors once with `$references`, reuse everywhere
  - 70% cumulative reduction
  - Tests: 21 test cases in `test_theme_palette.cc`

- [x] **Phase 3: Theme Inheritance** - 2025-10-30
  - Create variants with `extends: "Base Theme"`
  - Theme variants in 5-10 lines (97% reduction)
  - Tests: 15 test cases in `test_theme_inheritance.cc`

- [x] **Phase 4: Smart Defaults** - 2025-10-30
  - Minimal themes with just 3 colors (window_bg, text_fg, border_color)
  - Auto-generates 50+ theme values using color science
  - 97% total reduction (300 lines → **9 lines**!)
  - Color utilities: lighten(), darken(), invert(), luminance(), contrast()
  - Tests: 49 test cases (16 defaults + 33 color utils)
  - Examples: minimal_blue.yaml, minimal_green.yaml

- [x] **Phase 5: Visual State Templates** - Investigation completed
  - Status: **BLOCKED** on fkyaml limitations (no mapping anchors/merge operators)
  - Decision: **DEFERRED** - Phase 4 provides sufficient functionality
  - Tests: 3 test cases document fkyaml limitations

- [x] **Theming Documentation** - 2025-10-30
  - Comprehensive 607-line theme development guide
  - Updated core concepts documentation (+150 lines)
  - Project summary document (625 lines)
  - 8 total documentation files created/updated

**Total Impact**: 15 example themes, 107 theming test cases, 97% size reduction

### Previous Completions

- [x] **size_policy::percentage** - Percentage-based sizing for all layouts - 2025-10-30
- [x] **Scrollbar arrow glyphs** - Render arrow icons using backend icon_style - 2025-10-30
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
- `docs/THEMING_PROJECT_SUMMARY.md` - Comprehensive theming implementation summary
- `docs/THEMING_IMPROVEMENTS.md` - Theming system roadmap and status
- `docs/THEMING_PHASE[1-5]_*.md` - Detailed phase specifications
- `docusaurus/docs/guides/theme-development.md` - User-facing theme development guide
- `CLAUDE.md` - Development guidelines and architecture
- `unittest/layout/test_linear.cc:203` - Percentage sizing TODO

---

**Last verified against codebase: 2025-10-30**
