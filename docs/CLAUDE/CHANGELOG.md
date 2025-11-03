# OnyxUI Changelog

This document tracks recent major changes and feature additions to the OnyxUI framework.

## November 2025 - Relative Coordinate System Refactoring

**Status:** ✅ Complete (1184 tests passing, 6764 assertions)

### Overview

Complete refactoring of the coordinate system from absolute to relative bounds. Children now store coordinates relative to their parent's content area (0,0 origin) instead of absolute screen positions. This enables efficient repositioning, cleaner architecture, and fixes bugs in nested widgets.

### Problem Statement

The previous system stored absolute screen coordinates for all widgets:
- Repositioning a parent required updating all descendant coordinates
- Scrollable widgets had coordinate space confusion causing overlapping children
- Hit testing and clipping were complex due to mixing absolute and relative coordinates
- Menu rendering had offset bugs due to incorrect coordinate usage

### Solution: Relative Coordinates

Children store bounds relative to parent's content area:
- **Relative Storage**: Widget bounds use (0,0) origin relative to parent's content area
- **Absolute Rendering**: During render traversal, offsets accumulate to produce absolute screen coordinates
- **Coordinate Conversion**: Hit testing converts absolute→relative at each level, dirty regions convert relative→absolute

### Core Changes

#### element.hh - Coordinate System Overhaul
- `get_content_area()`: Returns relative coordinates (0,0 origin)
- `arrange()`: Positions children relative to content area, not absolute
- `render()`: Fixed clipping and child offset calculations
- `hit_test()`: Converts absolute screen coords to relative at content area boundary
- `mark_dirty()`: Converts relative bounds to absolute before propagating to root

#### Widget Rendering Fixes
- `label.hh`: Use `ctx.position()` (absolute) not `bounds()` (relative)
- `menu_item.hh`: Same rendering fix for menu items
- `menu.hh`: Reconstruct absolute bounds for shadow drawing
- `widget_container.hh`: Reconstruct absolute bounds for border drawing

#### Layout Strategies
All four layout strategies updated to use relative positioning:
- `linear_layout.hh`: Children positioned at (0,0) relative to content area
- `grid_layout.hh`: Grid cells use relative coordinates
- `anchor_layout.hh`: Anchored children positioned relatively
- `absolute_layout.hh`: Explicit positions are relative to content area

### Test Updates

- **New Tests**: 12 comprehensive tests added
  - `test_relative_coordinates.cc`: 9 tests for core coordinate system
  - `test_menu_visual.cc`: 3 visual rendering tests for menus
- **Updated Tests**: 30+ test assertions fixed to expect relative coordinates
  - test_content_area.cc, test_panel_layout.cc, test_group_box_layout.cc
  - test_complex_layouts.cc, test_composition.cc, test_scrollable.cc
  - test_theme_layout_integration.cc, test_menu_border_layout.cc

### Benefits

✅ **Simplified Layout**: Children don't need to know absolute screen position
✅ **Efficient Repositioning**: Moving a parent automatically moves all children
✅ **Clean Architecture**: Clear separation between relative storage and absolute rendering
✅ **Correct Clipping**: Clipping rectangles properly calculated in absolute space
✅ **Accurate Hit Testing**: Coordinate conversion at each level for precise hit detection
✅ **Fixed Bugs**: Menu rendering offset bug resolved
✅ **Solid Foundation**: Rock-solid base for future development

### Statistics

- **Tests Added**: 12 new tests (9 relative coordinates + 3 menu visual)
- **Tests Updated**: 30+ assertions fixed for relative coordinates
- **Total Tests**: 1184 (was 1172)
- **Total Assertions**: 6764 (was 6743)
- **Breaking Changes**: Internal only (no API changes)
- **Files Modified**: 21 files

### Files Modified

**Core:**
- `include/onyxui/core/element.hh` (6 methods updated)

**Widgets:**
- `include/onyxui/widgets/core/widget_container.hh`
- `include/onyxui/widgets/label.hh`
- `include/onyxui/widgets/menu/menu.hh`
- `include/onyxui/widgets/menu/menu_item.hh`

**Layout:**
- `include/onyxui/layout/linear_layout.hh`
- `include/onyxui/layout/grid_layout.hh`
- `include/onyxui/layout/anchor_layout.hh`
- `include/onyxui/layout/absolute_layout.hh`

**Tests:**
- `unittest/core/test_relative_coordinates.cc` (NEW - 9 tests)
- `unittest/widgets/test_menu_visual.cc` (NEW - 3 tests)
- 8 existing test files updated

**Documentation:**
- `docs/RELATIVE_COORDINATES_PLAN.md` (NEW)

### Migration Notes

**For Framework Developers:**
- When rendering widgets, use `ctx.position()` for absolute screen coordinates
- `bounds()` now returns RELATIVE coordinates (0,0 origin)
- For drawing operations (borders, shadows), reconstruct absolute bounds:
  ```cpp
  const auto& pos = ctx.position();
  const auto& bounds = this->bounds();
  rect_type absolute_bounds;
  rect_utils::set_bounds(absolute_bounds,
      point_utils::get_x(pos),
      point_utils::get_y(pos),
      rect_utils::get_width(bounds),
      rect_utils::get_height(bounds));
  ```

**For Application Developers:**
- No API changes - existing code continues to work
- Improved performance when moving/repositioning widgets
- More predictable behavior in nested containers

---

## October 2025 - Comprehensive Scrolling System

**Status:** ✅ Complete (996 tests passing, 5533 assertions)

### Overview

Implemented a complete three-layer scrolling system with high-level wrappers, manual composition, and standalone options.

### Implementation Phases

- **Phase 0**: Test infrastructure and scrollbar_theme placeholder
- **Phase 1**: scroll_info structure and scrollable container (66 tests)
- **Phase 2**: Scrollbar visibility policies (20 tests)
- **Phase 3**: Visual scrollbar widget with theming (40 tests)
- **Phase 4**: scroll_controller coordination layer (24 tests)
- **Phase 5**: scroll_view high-level wrapper + presets (42 tests)
- **Phase 6**: Integration tests for real-world scenarios (16 tests)
- **Phase 7**: Documentation (scrolling_guide.md, CLAUDE.md updates)

### Key Features

- **Three-Layer Architecture**: High-level scroll_view, manual composition, scrollable alone
- **Preset Variants**: modern, classic, compact, vertical-only scroll views
- **Bidirectional Sync**: scroll_controller keeps scrollable and scrollbars in sync via signals
- **Viewport Clipping**: Only visible items rendered (O(visible) not O(total))
- **Large Content Support**: Tested with 10,000+ items in linear_layout
- **Theming Support**: Scrollbars inherit from global theme via CSS inheritance
- **Mouse & Keyboard**: Automatic wheel scrolling, optional keyboard navigation
- **Performance**: Sub-pixel precision, dirty region optimization, minimal redraws

### Statistics

- **Tests Added**: 137 new scrolling tests across 7 test files
- **Total Tests**: 996 (was 859)
- **Total Assertions**: 5533
- **Breaking Changes**: Zero

### Files Added

- `include/onyxui/widgets/scroll_info.hh`
- `include/onyxui/widgets/scrollable.hh`
- `include/onyxui/widgets/scrollbar.hh`
- `include/onyxui/widgets/scroll_controller.hh`
- `include/onyxui/widgets/scroll_view.hh`
- `include/onyxui/widgets/scroll_view_presets.hh`
- `docs/scrolling_guide.md`
- `unittest/widgets/test_scroll_info.cc`
- `unittest/widgets/test_scrollable.cc`
- `unittest/widgets/test_scrollbar.cc`
- `unittest/widgets/test_scroll_controller.cc`
- `unittest/widgets/test_scroll_view.cc`
- `unittest/widgets/test_scroll_view_presets.cc`
- `unittest/widgets/test_scrolling_integration.cc`

---

## October 2025 - Hotkey Scheme System

**Status:** ✅ Complete (859 tests passing, 6236 assertions)

### Overview

Added customizable keyboard layouts allowing users to switch between Windows (F10 for menu) and Norton Commander (F9 for menu) styles at runtime.

### Implementation Phases

- **Phase 1**: Core structures (hotkey_action, hotkey_scheme, key_sequence parsing)
- **Phase 2**: Registry + built-in schemes (Windows, Norton Commander)
- **Phase 3**: hotkey_manager enhancement (scheme-aware semantic actions with priority)

### Key Features

- **Runtime Scheme Switching**: Users can choose Windows vs Norton Commander keyboard layout
- **Semantic Actions**: Framework-level actions (menu navigation, focus) separate from application hotkeys
- **Priority System**: Framework shortcuts take precedence over application shortcuts
- **Graceful Fallback**: Missing binding = mouse still works (progressive enhancement)
- **Auto-Configuration**: Built-in schemes pre-registered in ui_context, no manual setup required
- **Library-Level**: NOT backend-specific (keys are universal, unlike themes)

### Statistics

- **Tests Added**: 52 new hotkey tests
- **Total Tests**: 859 (was 855)
- **Total Assertions**: 6236 (was 6184)
- **Breaking Changes**: Zero (backward compatible)

### Built-in Schemes

- **Windows**: F10 for menu (standard modern UI convention)
- **Norton Commander**: F9 for menu (classic DOS feel)

### Files Added

- `include/onyxui/hotkeys/hotkey_scheme.hh`
- `include/onyxui/hotkeys/hotkey_scheme_registry.hh`
- `unittest/hotkeys/test_hotkey_scheme.cc`

### Files Modified

- `include/onyxui/hotkeys/hotkey_manager.hh` - Added semantic action support
- `include/onyxui/ui_context.hh` - Auto-registration of built-in schemes

---

## October 2025 - Theme System v2.0 Refactoring

**Status:** ✅ Complete (736 tests passing, 4660 assertions)

### Overview

Major refactoring of the theme system with three key improvements: thread-safe registry, global theming architecture, and style-based rendering.

### Implementation Phases

- **Phase 1**: Added thread-safe theme registry with failsafe logging
- **Phase 2**: Implemented three-way theme API (by-name, by-value, by-shared_ptr)
- **Phase 3**: Style-based rendering architecture with `resolved_style`
- **Phase 4**: Comprehensive testing (11 new test cases, 88 assertions)
- **Phase 5**: Documentation and polish
- **Phase 6**: Widget refactoring to enforce style-based rendering
  - Extended `resolved_style` with optional properties (padding, mnemonic_font)
  - Added theme pointer to `render_context` for rare widget-specific properties
  - Refactored all widgets (button, label, menu_item, menu_bar_item, separator, stateful_widget)
  - Eliminated all direct theme access from widgets (zero `ui_services::themes()` calls)
  - Implemented lazy mnemonic parsing for button, label, and menu_item

### Key Architectural Improvements

- **Thread Safety**: Reader-writer locking on theme registry
- **Performance**: CSS inheritance resolved once per frame, not per widget
- **Safety**: No dangling pointer risk with new ownership semantics
- **Simplicity**: Widgets receive pre-resolved style via render_context
- **Stateful Widgets**: New helper base class for interactive widgets
- **Two-Tier Access**: Common properties via `ctx.style()`, rare properties via `ctx.theme()`
- **Lazy Parsing**: Mnemonics parsed on-demand during render with caching
- **Type Safety**: Optional properties enforce explicit default handling

### Statistics

- **Tests Added**: 83 new theme tests
- **Total Tests**: 736 (was 653)
- **Total Assertions**: 4660
- **Breaking Changes**: Removed unsafe `apply_theme(const theme_type&)` API

### Files Added

- `include/onyxui/resolved_style.hh`
- `include/onyxui/widgets/stateful_widget.hh`
- `unittest/theming/test_resolved_style.cc`
- `unittest/theming/test_style_inheritance.cc`
- `unittest/theming/test_style_edge_cases.cc`

### Files Modified

- `include/onyxui/theme_registry.hh` - Thread-safe implementation
- `include/onyxui/themeable.hh` - Global theming architecture
- `include/onyxui/render_context.hh` - Style and theme access
- `include/onyxui/draw_context.hh` - Style-based rendering
- `include/onyxui/measure_context.hh` - Style-based measurement
- All widget files refactored for style-based rendering

---

## 2025 - Comprehensive Layout Testing Implementation

**Status:** ✅ Complete

### Overview

Implemented comprehensive layout testing framework with visual validation.

### Key Features

- **57 new layout tests** across all priority levels
- **Fixed critical y=65541 overflow bug** in element.hh
- **Created visual testing framework** with canvas-based validation
- **Added edge case, robustness, and complex scenario coverage**

### Files Added

- `unittest/layout/test_layout_*.cc` - Comprehensive layout tests
- `unittest/utils/test_canvas_renderer.hh` - Canvas-based validation

---

## Summary

The OnyxUI framework has undergone significant enhancements in 2025:

### Current Features

- **Complete widget library**: 16+ widgets including scrolling
- **CSS-style theming** with inheritance (v2.0 refactored)
- **Render context pattern** (visitor) for unified measurement/rendering
- **Comprehensive scrolling system** (scroll_view, scrollable, scrollbar, scroll_controller)
- **Comprehensive hotkey system** with customizable schemes (Windows vs Norton Commander)
- **Signal/slot** for event handling
- **Mnemonic support** (Alt+key shortcuts)
- **Thread-safe theme management** with failsafe logging
- **Visual testing framework** for layout validation

### Test Coverage

- **996 test cases** across 34 test files
- **5533 assertions**
- **Zero warnings** in all tests
- **100% widget coverage**
- **100% layout strategy coverage**

### Performance

- **O(depth) style resolution** once per frame
- **O(1) widget rendering** via pre-resolved styles
- **O(visible) scrolling** not O(total)
- **Thread-safe** theme registry with reader-writer locking
- **Sub-pixel precision** for smooth animations

---

## Migration Notes

### From Pre-v2.0 Themes

If you're using the old theme system:

```cpp
// OLD (REMOVED)
root->apply_theme(theme);  // Unsafe - dangling pointer risk

// NEW (v2.0)
root->apply_theme("Norton Blue", ctx.themes());  // Safe - by name
// OR
root->apply_theme(std::move(theme));  // Safe - element owns
// OR
root->apply_theme(theme_ptr);  // Safe - shared ownership
```

### From Pre-Scrolling System

If you implemented custom scrolling:

```cpp
// OLD (Manual implementation)
class my_scroll_container : public panel<Backend> {
    // Custom scrolling logic...
};

// NEW (Use built-in)
auto view = modern_scroll_view<Backend>();
view->add_child(my_content);
// Scrolling handled automatically!
```

### From Pre-Hotkey Schemes

If you registered menu activation hotkeys manually:

```cpp
// OLD (Manual registration)
auto menu_action = std::make_shared<action<Backend>>();
menu_action->set_shortcut(key::f10, key_modifier::none);
hotkeys.register_action(menu_action);

// NEW (Automatic with schemes)
// F10 (Windows) or F9 (Norton) automatically handled!
ctx.hotkeys().register_semantic_action(
    hotkey_action::activate_menu_bar,
    [&menu]() { menu->activate(); }
);
```

---

## Future Work

Potential upcoming features (not yet scheduled):

- Additional built-in widgets (checkbox, slider, text input)
- Animation framework
- Drag-and-drop support
- Touch input support
- Additional hotkey schemes (Emacs, Vim, etc.)
- Performance profiling tools
- Additional layout strategies (flexbox-like)

---

**See also:**
- `docs/scrolling_guide.md` - Scrolling system user guide
- `docs/CLAUDE/THEMING.md` - Theme system v2.0 details
- `docs/CLAUDE/HOTKEYS.md` - Hotkey system details
- `docs/REFACTORING_PLAN.md` - Upcoming refactoring work
