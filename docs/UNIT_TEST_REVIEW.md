# Unit Test Review

## Scope and Method
- Scope: All unit and integration tests under `unittest/` (158 files). Categories include core, layout, events, widgets, MVC, theming, reflection, hotkeys, services, focus, geometry, and utilities.
- Method: Pattern scan for low-signal assertions and TODO/skip markers, plus spot reads across each category to assess test value and coverage depth.

## What the Current Tests Do Well
- Core correctness: `unittest/core/` has focused tests that assert concrete invariants (geometry conversions, render context measurement, event routing, dirty tracking). These are high-value because they validate pure logic without UI dependencies.
- Layout behavior: `unittest/layout/` verifies sizes and positions, often with explicit expected coordinates. This is strong for regression protection in layout logic.
- Events and routing: `unittest/events/` validates phase ordering and propagation; these tests protect critical behavioral contracts.
- Widgets and integration: Many widget tests use `ui_context_fixture` and `test_canvas_backend`, providing consistent themes and deterministic measurement/arrangement. Integration tests around scroll and focus provide good behavioral coverage.
- MVC model tests: `unittest/mvc/test_table_model.cc` asserts signaling and data behavior; these tests are high value because they validate observable model effects.

## Findings and Points of Improvement

### 1) Placeholder or crash-only assertions (low signal) - ✅ FIXED (Jan 2026)
These tests previously asserted `CHECK(true)` without validating state. They have been converted to meaningful assertions:

**Fixed:**
- `unittest/core/test_minimal_overflow.cc` - Now verifies bounds dimensions after arrange
- `unittest/core/test_hit_test_path.cc` - Now verifies bounds and children count
- `unittest/mvc/test_mvc_basic.cc` - Now uses `CHECK_NOTHROW` for constructor test
- `unittest/widgets/test_window_theming.cc` - Now verifies measurement returns positive sizes (also fixed missing fixture)
- `unittest/widgets/test_table.cc` - Now verifies row count and cell access after operations
- `unittest/widgets/test_table_view.cc` - Now verifies bounds dimensions after arrange

**Remaining (low priority):**
- `unittest/utils/rule_of_five_tests.hh:69` (static assertions already carry value; the runtime check is acceptable)

### 2) YAML-disabled branches are effectively no-ops - ✅ FIXED (Jan 2026)
When `ONYXUI_ENABLE_YAML_THEMES` is off, these tests now use `doctest::skip(true)` to explicitly mark them as skipped rather than using `CHECK(true)` placeholders.

**Fixed files (now use explicit skip):**
- `unittest/theming/test_theme_inheritance.cc`
- `unittest/theming/test_theme_defaults.cc`
- `unittest/theming/test_theme_palette.cc`
- `unittest/reflection/test_enum_reflection.cc`
- `unittest/reflection/test_color_yaml.cc`
- `unittest/reflection/test_fkyaml_basic.cc`
- `unittest/reflection/test_scrollbar_style_yaml.cc`
- `unittest/reflection/test_nested_structs.cc`
- `unittest/reflection/test_font_reflection.cc`

Value impact: Test gaps are now explicitly visible in doctest output (shown as "skipped") rather than silently passing.

### 3) Skipped or TODO tests leave known gaps - ✅ FIXED (Jan 2026)
The previously skipped layer manager lifetime tests now pass after implementing automatic cleanup via the `destroying` signal.

**FIXED - Layer manager dangling pointer tests:**
- `unittest/core/test_layer_manager_lifetime.cc` - All 5 previously skipped test cases now pass
  - Added `destroying_conn` field to `layer_data` struct
  - Connected to element's `destroying` signal in `show_popup`/`show_tooltip`/`show_modal_dialog`
  - When element is destroyed, signal fires and layer is auto-removed
  - Tests: 1710 → 1715, Skipped: 5 → 0

**Using WARN to document known limitations:**
- `unittest/widgets/test_scrollbar_visibility.cc:74` - test_canvas backend doesn't render graphics

**FIXED - text_view focus layout issue:**
- `unittest/widgets/test_text_view_focus.cc` - Fixed by adding explicit `size_policy::fixed` handling in `linear_layout`. Previously, fixed height constraints were ignored because the layout used measured sizes instead of `preferred_size`.

**FIXED - Modal layer integration (Phase 5):**
- `unittest/widgets/test_window.cc` - `show_modal()` now sets `is_modal` flag
- `unittest/services/test_window_manager.cc` - `get_modal_windows()` now returns modal windows correctly
- `unittest/widgets/test_window_modal.cc` - Added comprehensive modal integration tests:
  - `is_modal()` flag tests (set by `show_modal()`, cleared by `show()`)
  - `layer_manager.has_modal_layer()` integration tests

**FIXED - group_box title rendering:**
- `unittest/widgets/test_group_box_layout.cc` - Title rendering now implemented in `do_render()`
- Titles appear at top border with padding (` Title `) at configurable offset

**FIXED - scroll_view direct keyboard event handling:**
- `unittest/widgets/test_scroll_view_keyboard.cc` - Added comprehensive keyboard event tests
  - Tests handle_event() routes keyboard events to scroll_view::handle_keyboard()
  - Verifies arrow keys, page up/down, home/end are handled
  - Verifies key release events are not handled
  - Verifies unhandled keys fall through to base class
  - Fixed scroll_view.hh to make handle_event() public (matches base class)

Value impact: Critical lifetime safety issue fixed. Modal layer integration complete. scroll_view keyboard handling tested.

### 4) Some tests only check signal wiring, not effects - ✅ FIXED (Jan 2026)
The header click test in `unittest/widgets/test_table_view.cc` now simulates actual header clicks and verifies signal emission:

**Fixed:**
- `table_view - Header clicked signal emits on click` now includes:
  - Click on first/second/third column headers verifies correct column index emitted
  - Click below header area verifies no signal emitted
  - Click with sorting disabled verifies no signal emitted

Value impact: Tests now verify actual behavior, not just signal mechanism existence.

### 5) Visual rendering coverage is constrained by test backends
`test_canvas_backend` does not render all graphics (e.g., borders/fills), and some visual tests log warnings instead of asserting output. This limits the value of visual regressions in the widgets layer.

Value impact: Visual regressions can slip unless manual inspection is done.

## Recommendations (Status as of Jan 2026)
1) ✅ **DONE** - Replace `CHECK(true)` with meaningful assertions. All placeholder assertions now verify observable state.
2) ✅ **DONE** - Use `doctest::skip()` for YAML-disabled branches. All 9 files now use explicit skip.
3) ✅ **DONE** - Layer manager lifetime tests fixed. Added `destroying` signal connection for automatic cleanup.
4) ✅ **DONE** - Add behavior tests for signal wiring. Header click test now simulates clicks and verifies column indices.
5) ⏳ **FUTURE** - Visual testing infrastructure improvements require test_canvas backend enhancements.

## Notes on Adequacy (Updated Jan 2026)
- Core/layout/event tests provide strong safety nets for logic regressions.
- Widget and MVC tests now use meaningful assertions instead of placeholders.
- YAML-disabled tests are explicitly marked as skipped in test output.
- Layer manager lifetime safety fully tested - no more dangling pointer risks.
- Modal layer integration complete - `show_modal()` properly sets flags and integrates with layer_manager.
- group_box title rendering implemented - titles display at top border with configurable positioning.
- scroll_view keyboard event handling tested - direct keyboard events properly routed through handle_event().
- Visual rendering coverage remains constrained by test_canvas backend limitations.

**Test Statistics:**
- Test cases: 1718 passed, 0 failed, 0 skipped
- Assertions: 8554 passed, 0 failed
