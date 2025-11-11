# Session 2025-11-11: Workspace Reference Pattern Implementation

**Date**: 2025-11-11
**Status**: ✅ COMPLETED
**Author**: Claude Code
**Context**: Continuation from previous session on window maximize functionality

---

## Summary

Successfully implemented workspace reference pattern for floating layer windows, solving the problem of windows needing to be workspace-aware without becoming children. This enables windows to respect workspace bounds (central_widget area) when maximizing while maintaining proper event routing through layer_manager.

**Key Achievement**: Windows can now maximize to workspace area (excluding menu bar) while maintaining full button functionality.

---

## Problem Statement

### Initial Issue
After implementing Phases 1-4 of window maximize functionality, a regression was discovered:
- Window control icons (close, minimize, maximize) stopped working in widgets demo
- Cause: Windows were parented to `central_widget` to make maximize respect workspace

### Root Cause Analysis
When windows have `central_widget` as parent:
1. ❌ Windows removed from layer_manager (no longer floating)
2. ❌ Events route through widget tree instead of layer_manager
3. ❌ layer_manager's specialized event routing (absolute coordinates) is lost
4. ❌ Button hit-testing fails due to coordinate system mismatch

### User Questions
User asked two critical questions:
1. "I still don't understand - the central widget should be a transparent panel, isn't it?"
2. "1) Add workspace support 2) it still does not explain why minimize/close don't work"

This clarified that user wanted:
- **Floating layer windows** (not embedded children)
- **Workspace-aware maximize** (respect central_widget bounds)
- **Working buttons** (layer_manager event routing)

---

## Solution: Workspace Reference Pattern

### Architectural Concept

Instead of parent-child relationship, windows optionally reference a workspace element:

```cpp
class window {
    ui_element<Backend>* m_workspace = nullptr;  // NOT a parent!
};
```

**Key Distinction**:
| Aspect | Parent (MDI) | Workspace (SDI) |
|--------|--------------|-----------------|
| Relationship | Child in widget tree | Independent floating layer |
| Coordinates | Relative to parent | Absolute screen coords |
| Event routing | Widget tree hierarchy | layer_manager specialized |
| Maximize | Fills parent bounds | Fills workspace bounds |
| Use case | Embedded windows | Floating windows |

### Implementation Details

#### 1. window.hh - Added Workspace Support

```cpp
private:
    ui_element<Backend>* m_workspace = nullptr;  // line 452

public:
    /**
     * @brief Set workspace area for maximize bounds
     * @param workspace Workspace element (e.g., main_window::central_widget())
     *
     * @details
     * Sets a workspace reference for floating layer windows.
     * When maximized, the window will fill the workspace bounds instead
     * of the entire viewport. This allows windows to respect UI chrome
     * (menu bars, status bars, etc.) without being parented to the workspace.
     *
     * **Key difference from parent:**
     * - **Parent**: Window becomes child in widget tree, uses relative coords,
     *   events route through widget hierarchy
     * - **Workspace**: Window remains independent floating layer, uses absolute
     *   coords, events route through layer_manager, but maximizes to workspace area
     *
     * **Maximize behavior priority**: parent > workspace > viewport
     */
    void set_workspace(ui_element<Backend>* workspace) noexcept {
        m_workspace = workspace;
    }

    [[nodiscard]] ui_element<Backend>* get_workspace() const noexcept {
        return m_workspace;
    }
```

#### 2. window.inl - Updated maximize() Logic

```cpp
void window<Backend>::maximize() {
    // ... state checks ...

    rect_type maximized_bounds;

    // Priority: parent > workspace > viewport
    auto* parent_elem = this->parent();
    if (parent_elem) {
        // Case 1: MDI window - maximize to fill parent
        auto parent_bounds = parent_elem->bounds();
        rect_utils::set_bounds(
            maximized_bounds,
            0, 0,  // Relative to parent
            rect_utils::get_width(parent_bounds),
            rect_utils::get_height(parent_bounds)
        );
    } else if (m_workspace) {
        // Case 2: SDI with workspace - maximize to fill workspace
        // Use FULL workspace bounds (including position offset from menu bar)
        maximized_bounds = m_workspace->bounds();
    } else {
        // Case 3: Standalone - maximize to fill viewport
        auto* layers = ui_services<Backend>::layers();
        if (layers) {
            maximized_bounds = layers->get_viewport();
        } else {
            rect_utils::set_bounds(maximized_bounds, 0, 0, 80, 25);  // Fallback
        }
    }

    // ... measure, arrange, update layer ...
}
```

**Critical Fix**: Using `m_workspace->bounds()` (full bounds) instead of just dimensions ensures window accounts for menu bar offset. If central_widget is at (0, 30) due to menu bar, maximized window correctly positions at (0, 30) with workspace dimensions.

#### 3. demo_windows.hh - Updated Window Creation

Changed all window creation functions to accept workspace parameter:

```cpp
template <UIBackend Backend>
std::shared_ptr<window<Backend>> show_basic_window_with_workspace(
    const std::string& title,
    const std::string& content_text,
    ui_element<Backend>* workspace = nullptr
) {
    auto win = std::make_shared<window<Backend>>(title, flags, nullptr);  // Parentless!

    // ... create content ...

    if (workspace) {
        win->set_workspace(workspace);  // Set workspace reference
        std::cerr << "[DEBUG] Workspace set for maximize behavior" << std::endl;
    }

    win->show();  // Still a floating layer
    return win;
}
```

Similar updates for:
- `show_scrollable_window(workspace)`
- `show_controls_window(workspace)`
- `show_modal_dialog()` (no workspace - always viewport)

#### 4. demo_menu_builder.hh - Pass Workspace Reference

```cpp
// Window menu - Basic Window
basic_win_item->clicked.connect([widget]() {
    auto win = demo_windows::show_basic_window_with_workspace<Backend>(
        "Demo Window",
        "This is a basic window with title bar controls!",
        widget->central_widget()  // Workspace reference, NOT parent!
    );
});

// Scrollable Window
scroll_win_item->clicked.connect([widget]() {
    demo_windows::show_scrollable_window<Backend>(widget->central_widget());
});

// Controls Window
controls_win_item->clicked.connect([widget]() {
    demo_windows::show_controls_window<Backend>(widget->central_widget());
});
```

---

## Why Buttons Work Now

### Event Flow Comparison

#### ❌ WITH PARENT (Broken)
```
User clicks close button at screen coordinates (x=75, y=35)
   ↓
main_window receives event with absolute coords (75, 35)
   ↓
Event routes through widget tree: main_window → central_widget → window → title_bar
   ↓
**PROBLEM**: Window is embedded child, loses layer_manager's absolute event routing
   ↓
Button hit-testing may fail due to focus/visibility/coordinate issues
```

#### ✅ WITH WORKSPACE (Working)
```
User clicks close button at screen coordinates (x=75, y=35)
   ↓
layer_manager receives event with absolute coords (75, 35)
   ↓
layer_manager routes to floating window using absolute coords
   ↓
title_bar receives event, still has absolute coords (75, 35)
   ↓
title_bar uses get_absolute_bounds() for icon hit-testing
   ↓
Both icon bounds and mouse coords are absolute → HIT TEST SUCCEEDS ✅
```

### Technical Details

**title_bar hit-testing code** (window_title_bar.inl:140-150):
```cpp
auto icon_contains = [mouse_evt](ui_element<Backend>* icon) -> bool {
    if (!icon) return false;

    // Get icon's absolute screen bounds
    auto abs_bounds = icon->get_absolute_bounds();

    // Check if mouse is within icon bounds (both absolute coords)
    return (mouse_evt->x >= rect_utils::get_x(abs_bounds) &&
           mouse_evt->x < rect_utils::get_x(abs_bounds) + rect_utils::get_width(abs_bounds) &&
           mouse_evt->y >= rect_utils::get_y(abs_bounds) &&
           mouse_evt->y < rect_utils::get_y(abs_bounds) + rect_utils::get_height(abs_bounds));
};
```

**Key insight**: layer_manager delivers events with absolute coordinates to floating windows, matching the absolute coordinates from `get_absolute_bounds()`. This makes hit-testing reliable.

---

## Testing Results

### Unit Tests
```bash
$ ./build/bin/ui_unittest
[doctest] test cases: 1310 | 1310 passed | 0 failed | 5 skipped
[doctest] assertions: 8056 | 8056 passed | 0 failed |
[doctest] Status: SUCCESS!
```

✅ All 1310 tests pass
✅ 8056 assertions pass
✅ Zero warnings
✅ Zero regressions

### Manual Testing
1. ✅ Created window from Window menu
2. ✅ Close button works (window closes)
3. ✅ Minimize button works (window minimizes)
4. ✅ Maximize button works (fills central_widget area, NOT entire screen)
5. ✅ Menu bar remains visible when window maximized
6. ✅ Dragging works (window moves)
7. ✅ System menu works (hamburger icon)

---

## Files Modified

### Core Framework
1. **include/onyxui/widgets/window/window.hh**
   - Lines 331-365: Added comprehensive set_workspace() documentation
   - Line 452: Added `m_workspace` member variable

2. **include/onyxui/widgets/window/window.inl**
   - Lines 228-233: Updated maximize() to use workspace bounds
   - Critical fix: Use full `m_workspace->bounds()` (not just dimensions)

### Demo Application
3. **examples/demo_windows.hh**
   - Line 59: Renamed to `show_basic_window_with_workspace()`
   - Lines 90-94: Added workspace support to basic window
   - Lines 171-175: Added workspace support to scrollable window
   - Lines 315-319: Added workspace support to controls window

4. **examples/demo_menu_builder.hh**
   - Lines 133-140: Updated basic window menu callback
   - Lines 146-148: Updated scrollable window menu callback
   - Lines 154-156: Updated controls window menu callback

### Documentation
5. **docs/MAIN_WINDOW_IMPLEMENTATION.md**
   - Lines 1148-1347: Added Phase 5 section (workspace reference pattern)
   - Line 1574: Updated document status to COMPLETED

6. **docs/CLAUDE/CHANGELOG.md**
   - Lines 5-153: Added November 2025 entry for workspace reference pattern
   - Includes problem statement, solution, examples, migration notes

7. **SESSION_2025_11_11_WORKSPACE_PATTERN.md** (this file)
   - Complete session documentation

---

## Architecture Decision Record

**Decision**: Use workspace reference pattern for SDI applications.

**Context**:
- Need windows to respect workspace bounds (central_widget) when maximizing
- Windows must remain floating layers for proper UX (drag anywhere, z-order)
- Buttons must work correctly (require layer_manager event routing)

**Considered Alternatives**:

1. **Parent to central_widget (MDI pattern)**
   - ❌ Breaks event routing (buttons stop working)
   - ❌ Loses floating window UX
   - ❌ Windows become embedded children
   - ✅ Would maximize to correct area

2. **Always maximize to viewport**
   - ✅ Buttons work (floating windows)
   - ✅ Floating window UX
   - ❌ Maximized windows cover menu bar (bad UX)

3. **Workspace reference pattern (chosen)**
   - ✅ Buttons work (layer_manager event routing)
   - ✅ Floating window UX preserved
   - ✅ Maximize respects workspace bounds
   - ✅ Clean separation of concerns
   - ✅ No breaking changes to MDI pattern

**Consequences**:
- **Positive**: Best of both worlds (floating + workspace-aware)
- **Positive**: Clear conceptual model (reference vs parent)
- **Positive**: No breaking changes to existing code
- **Negative**: Additional API (`set_workspace()`)
- **Negative**: Developers must understand distinction

**Mitigation**:
- Comprehensive documentation with examples
- Clear naming (`set_workspace` vs `parent`)
- Detailed comments explaining difference

---

## Usage Patterns

### Pattern 1: SDI Application (Floating Windows with Workspace)

```cpp
// Create main window with menu bar
auto main = std::make_unique<main_window<Backend>>();
main->set_menu_bar(build_menu_bar());

// Create floating window
typename window<Backend>::window_flags flags;
flags.has_maximize_button = true;
auto win = std::make_shared<window<Backend>>("Document 1", flags);

// Set workspace reference (NOT parent!)
win->set_workspace(main->central_widget());

// Window remains floating layer
win->show();  // Added to layer_manager

// Maximize respects workspace
win->maximize();  // Fills central_widget area (excludes menu bar)
```

### Pattern 2: MDI Application (Embedded Windows)

```cpp
// Create main window
auto main = std::make_unique<main_window<Backend>>();

// Create embedded window (parent-child relationship)
typename window<Backend>::window_flags flags;
auto win = std::make_shared<window<Backend>>("Document 1", flags, main->central_widget());

// Window is embedded in widget tree
win->show();  // Added to layer_manager but parented to central_widget

// Maximize fills parent
win->maximize();  // Fills central_widget (via parent relationship)
```

### Pattern 3: Standalone Window (No Workspace)

```cpp
// Create standalone window (no main_window)
typename window<Backend>::window_flags flags;
auto win = std::make_shared<window<Backend>>("Standalone", flags);

// No workspace, no parent
win->show();  // Floating layer

// Maximize fills entire viewport
win->maximize();  // Fills entire screen
```

---

## Lessons Learned

1. **Event Routing Architecture Matters**: The distinction between tree-based and layer-based event routing is critical for floating window UX.

2. **Coordinate Systems Are Tricky**: Absolute vs relative coordinates must be carefully managed. Using full `bounds()` (including position) is essential for workspace pattern.

3. **Separation of Concerns**: Workspace reference cleanly separates "maximize bounds" from "widget tree hierarchy".

4. **Regression Testing**: The button regression immediately revealed the architectural incompatibility between MDI and SDI patterns.

5. **Documentation Is Key**: Clear documentation with examples helps users understand subtle distinctions (reference vs parent).

---

## Future Work

### Potential Enhancements

1. **Workspace Change Notifications**: If workspace resizes, notify registered windows
2. **Multiple Workspaces**: Support multiple workspace areas (e.g., split screen)
3. **Workspace Constraints**: Additional constraints beyond maximize (min/max size relative to workspace)
4. **Automatic Workspace Detection**: Heuristics to auto-detect workspace from layer hierarchy

### Related Features

1. **Window Docking**: Snap windows to workspace edges
2. **Workspace Persistence**: Save/restore window positions relative to workspace
3. **Multi-Monitor Support**: Workspace per monitor

---

## Conclusion

The workspace reference pattern successfully solves the challenge of making floating layer windows workspace-aware without breaking their event routing. This is a key architectural pattern that clarifies the distinction between MDI (embedded) and SDI (floating) window models.

**Key Takeaway**: Sometimes the solution isn't to make one pattern work for all cases, but to introduce a clean abstraction (workspace reference) that preserves the strengths of both approaches.

---

**Session Duration**: ~2 hours
**Lines of Code Changed**: ~200
**Tests Passing**: 1310 / 1310 (100%)
**Regression Count**: 0
**Documentation Pages**: 3 (implementation plan, changelog, session)
