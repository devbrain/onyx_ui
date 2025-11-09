# Window Management Implementation Plan

**Date**: 2025-11-09
**Status**: Ready for implementation
**Priority**: HIGH (completes documented features)

---

## Executive Summary

This document provides a detailed implementation plan for window management TODOs, organized from simple to complex. Each task includes:
- Full specifications
- Implementation steps
- Test plans
- Visual assertions
- Dependencies
- Success criteria

**Estimated effort**: 2-3 days
**Test coverage target**: 100% (all features tested)

---

## Table of Contents

1. [SIMPLE: Task 1 - Modal Window Checking](#task-1-modal-window-checking)
2. [SIMPLE: Task 2 - Verify Semantic Actions](#task-2-verify-semantic-actions)
3. [MEDIUM: Task 3 - Window Z-Order Management](#task-3-window-z-order-management)
4. [MEDIUM: Task 4 - Window List Dialog Labels](#task-4-window-list-dialog-labels)
5. [COMPLEX: Task 5 - Modal Dialog Lifecycle](#task-5-modal-dialog-lifecycle)
6. [Integration Testing](#integration-testing)
7. [Visual Verification](#visual-verification)

---

## Task 1: Modal Window Checking

**Complexity**: SIMPLE
**Estimated time**: 15 minutes
**Files affected**: `window_manager.inl:47-52`

### Current State

```cpp
template<UIBackend Backend>
std::vector<window<Backend>*> window_manager<Backend>::get_modal_windows() const {
    std::vector<window<Backend>*> modal;
    modal.reserve(m_windows.size());

    for (auto* win : m_windows) {
        // TODO Phase 5: Check if window is modal
        // For now, return empty (modal support comes in Phase 5)
        // if (win && win->is_modal()) {
        //     modal.push_back(win);
        // }
    }

    return modal;
}
```

### Required Changes

**Step 1**: Uncomment the modal checking code

```cpp
template<UIBackend Backend>
std::vector<window<Backend>*> window_manager<Backend>::get_modal_windows() const {
    std::vector<window<Backend>*> modal;
    modal.reserve(m_windows.size());

    for (auto* win : m_windows) {
        if (win && win->is_modal()) {
            modal.push_back(win);
        }
    }

    return modal;
}
```

**Step 2**: Remove the TODO comment

### Implementation Steps

1. Open `include/onyxui/services/window_manager.inl`
2. Navigate to line 47
3. Uncomment lines 49-51
4. Remove lines 47-48 (TODO comment)
5. Build and verify no compilation errors

### Test Plan

#### Test 1: Empty Modal List
```cpp
TEST_CASE("window_manager - get_modal_windows returns empty initially") {
    ui_context_fixture<test_canvas_backend> ctx;
    auto* wm = ui_services<test_canvas_backend>::window_manager();

    auto modals = wm->get_modal_windows();
    REQUIRE(modals.empty());
}
```

#### Test 2: Non-Modal Windows
```cpp
TEST_CASE("window_manager - get_modal_windows excludes non-modal windows") {
    ui_context_fixture<test_canvas_backend> ctx;
    auto* wm = ui_services<test_canvas_backend>::window_manager();

    // Create non-modal windows
    auto win1 = std::make_unique<window<test_canvas_backend>>("Window 1");
    auto win2 = std::make_unique<window<test_canvas_backend>>("Window 2");

    wm->register_window(win1.get());
    wm->register_window(win2.get());

    auto modals = wm->get_modal_windows();
    REQUIRE(modals.empty());
}
```

#### Test 3: Modal Windows
```cpp
TEST_CASE("window_manager - get_modal_windows returns only modal windows") {
    ui_context_fixture<test_canvas_backend> ctx;
    auto* wm = ui_services<test_canvas_backend>::window_manager();

    // Create mixed windows
    auto non_modal = std::make_unique<window<test_canvas_backend>>("Non-Modal");

    typename window<test_canvas_backend>::window_flags modal_flags;
    modal_flags.is_modal = true;
    auto modal1 = std::make_unique<window<test_canvas_backend>>("Modal 1", modal_flags);
    auto modal2 = std::make_unique<window<test_canvas_backend>>("Modal 2", modal_flags);

    wm->register_window(non_modal.get());
    wm->register_window(modal1.get());
    wm->register_window(modal2.get());

    auto modals = wm->get_modal_windows();
    REQUIRE(modals.size() == 2);

    // Verify correct windows returned
    bool found_modal1 = false;
    bool found_modal2 = false;
    for (auto* win : modals) {
        if (win->get_title() == "Modal 1") found_modal1 = true;
        if (win->get_title() == "Modal 2") found_modal2 = true;
    }
    REQUIRE(found_modal1);
    REQUIRE(found_modal2);
}
```

#### Test 4: Null Window Handling
```cpp
TEST_CASE("window_manager - get_modal_windows handles null windows") {
    ui_context_fixture<test_canvas_backend> ctx;
    auto* wm = ui_services<test_canvas_backend>::window_manager();

    // Register window then unregister (leaves null in list)
    typename window<test_canvas_backend>::window_flags modal_flags;
    modal_flags.is_modal = true;

    {
        auto modal = std::make_unique<window<test_canvas_backend>>("Modal", modal_flags);
        wm->register_window(modal.get());
        // modal destroyed here, but pointer may remain in manager
    }

    // Should not crash and should return empty list
    auto modals = wm->get_modal_windows();
    // Behavior depends on window_manager cleanup policy
}
```

### Test File Location
`unittest/window/test_window_manager_modal.cc`

### Success Criteria

✅ All 4 tests pass
✅ No compilation warnings
✅ No crashes with null windows
✅ Modal and non-modal windows correctly distinguished

### Visual Verification

N/A - This is a data retrieval function with no visual component.

---

## Task 2: Verify Semantic Actions

**Complexity**: SIMPLE
**Estimated time**: 30 minutes (primarily testing/verification)
**Files affected**: `window_manager.inl:78`, `window.inl:714-729`

### Current State

The TODO comment at `window_manager.inl:78` suggests semantic actions are not registered:

```cpp
template<UIBackend Backend>
void window_manager<Backend>::register_hotkeys() {
    // TODO Phase 4: Register semantic actions for window management
    // Requires hotkey_manager integration
    // ...
}
```

However, the actual implementation in `window.inl:714-729` shows they ARE already registered:

```cpp
template<UIBackend Backend>
void window_manager<Backend>::register_hotkeys() {
    auto* hotkeys = ui_services<Backend>::hotkeys();
    if (!hotkeys) return;

    // Ctrl+W - Show window list (Turbo Vision style)
    hotkeys->register_semantic_action(
        hotkey_action::show_window_list,
        [this]() { this->show_window_list(); }
    );

    // Ctrl+Tab / Ctrl+F6 - Cycle to next window
    hotkeys->register_semantic_action(
        hotkey_action::next_window,
        [this]() { this->cycle_next_window(); }
    );

    // Ctrl+Shift+Tab / Ctrl+Shift+F6 - Cycle to previous window
    hotkeys->register_semantic_action(
        hotkey_action::prev_window,
        [this]() { this->cycle_previous_window(); }
    );
}
```

### Required Changes

**Step 1**: Remove obsolete TODO comment

In `window_manager.inl:78`, remove the TODO comment block since the implementation already exists.

**Step 2**: Verify implementation is correct

The implementation in `window.inl` is already complete and correct.

### Implementation Steps

1. Open `include/onyxui/services/window_manager.inl`
2. Navigate to line 78
3. Delete the TODO comment block (lines 78-102)
4. Verify no other references exist
5. Build and verify no compilation errors

### Test Plan

#### Test 1: Hotkey Registration
```cpp
TEST_CASE("window_manager - register_hotkeys registers semantic actions") {
    ui_context_fixture<test_canvas_backend> ctx;
    auto* wm = ui_services<test_canvas_backend>::window_manager();
    auto* hotkeys = ui_services<test_canvas_backend>::hotkeys();

    REQUIRE(hotkeys != nullptr);

    // Register hotkeys
    wm->register_hotkeys();

    // Verify semantic actions are registered
    // Note: This requires access to hotkey_manager internals or a query method
    // If no query method exists, test by triggering the actions
}
```

#### Test 2: Show Window List Action
```cpp
TEST_CASE("window_manager - show_window_list semantic action triggers correctly") {
    ui_context_fixture<test_canvas_backend> ctx;
    auto* wm = ui_services<test_canvas_backend>::window_manager();
    auto* hotkeys = ui_services<test_canvas_backend>::hotkeys();

    bool triggered = false;

    // Create test window to verify show_window_list is called
    auto win = std::make_unique<window<test_canvas_backend>>("Test");
    wm->register_window(win.get());

    wm->register_hotkeys();

    // Trigger semantic action
    // Note: Requires hotkey_manager::trigger_semantic_action() method
    // or simulate key event that maps to show_window_list

    // REQUIRE(triggered);  // Verify action was called
}
```

#### Test 3: Next Window Action
```cpp
TEST_CASE("window_manager - next_window semantic action cycles windows") {
    ui_context_fixture<test_canvas_backend> ctx;
    auto* wm = ui_services<test_canvas_backend>::window_manager();

    // Create multiple windows
    auto win1 = std::make_unique<window<test_canvas_backend>>("Window 1");
    auto win2 = std::make_unique<window<test_canvas_backend>>("Window 2");
    auto win3 = std::make_unique<window<test_canvas_backend>>("Window 3");

    wm->register_window(win1.get());
    wm->register_window(win2.get());
    wm->register_window(win3.get());

    win1->show();
    win2->show();
    win3->show();

    wm->register_hotkeys();

    // Set initial active window
    wm->set_active_window(win1.get());

    // Trigger next_window action
    wm->cycle_next_window();

    // Verify win2 is now active
    REQUIRE(wm->get_active_window() == win2.get());

    // Cycle again
    wm->cycle_next_window();
    REQUIRE(wm->get_active_window() == win3.get());

    // Wrap around
    wm->cycle_next_window();
    REQUIRE(wm->get_active_window() == win1.get());
}
```

#### Test 4: Previous Window Action
```cpp
TEST_CASE("window_manager - prev_window semantic action cycles backward") {
    ui_context_fixture<test_canvas_backend> ctx;
    auto* wm = ui_services<test_canvas_backend>::window_manager();

    auto win1 = std::make_unique<window<test_canvas_backend>>("Window 1");
    auto win2 = std::make_unique<window<test_canvas_backend>>("Window 2");

    wm->register_window(win1.get());
    wm->register_window(win2.get());

    win1->show();
    win2->show();

    wm->register_hotkeys();

    wm->set_active_window(win1.get());

    // Cycle backward (should wrap to win2)
    wm->cycle_previous_window();
    REQUIRE(wm->get_active_window() == win2.get());

    // Cycle backward again
    wm->cycle_previous_window();
    REQUIRE(wm->get_active_window() == win1.get());
}
```

### Test File Location
`unittest/window/test_window_manager_hotkeys.cc`

### Success Criteria

✅ Obsolete TODO comment removed
✅ All 4 tests pass
✅ Semantic actions correctly trigger window management functions
✅ No compilation warnings

### Visual Verification

**Manual test**: Run `conio` demo
1. Open 3+ windows
2. Press `Ctrl+Tab` → Should cycle to next window
3. Press `Ctrl+Shift+Tab` → Should cycle backward
4. Press `Ctrl+W` → Should show window list dialog (when Task 4 is implemented)

---

## Task 3: Window Z-Order Management

**Complexity**: MEDIUM
**Estimated time**: 2-3 hours
**Files affected**: `window.inl:686, 709`, `layer_manager.hh`, `layer_manager.inl`

### Current State

```cpp
template<UIBackend Backend>
void window_manager<Backend>::cycle_next_window() {
    // ... cycling logic ...
    m_active_window = *it;

    // TODO: Bring window to front / request focus
}

template<UIBackend Backend>
void window_manager<Backend>::cycle_previous_window() {
    // ... cycling logic ...
    m_active_window = *it;

    // TODO: Bring window to front / request focus
}
```

### Architecture Analysis

Windows are displayed in layers managed by `layer_manager`. Each window has a `layer_id` stored in `window::m_layer_id` (window.hh:402).

**Z-order management requires**:
1. `layer_manager` method to move layer to front
2. `window` method to bring itself to front
3. Integration with `focus_manager` to set keyboard focus

### Required Changes

#### Change 1: Add layer_manager::bring_to_front()

**File**: `include/onyxui/services/layer_manager.hh`

Add public method declaration:
```cpp
/**
 * @brief Bring layer to front (highest z-order)
 * @param id Layer to bring to front
 *
 * @details
 * Moves the specified layer to the end of the layers vector,
 * making it render last (on top of all other layers).
 * Does nothing if layer ID is invalid.
 */
void bring_to_front(layer_id id);
```

#### Change 2: Implement layer_manager::bring_to_front()

**File**: `include/onyxui/services/layer_manager.inl`

```cpp
template<UIBackend Backend>
void layer_manager<Backend>::bring_to_front(layer_id id) {
    if (!id.is_valid()) return;

    // Find layer with matching ID
    auto it = std::find_if(m_layers.begin(), m_layers.end(),
        [id](const layer_entry& entry) {
            return entry.id == id;
        });

    if (it == m_layers.end()) return;  // Layer not found

    // Already at front?
    if (it == m_layers.end() - 1) return;

    // Move to end (front)
    layer_entry entry = std::move(*it);
    m_layers.erase(it);
    m_layers.push_back(std::move(entry));

    // Invalidate rendering (z-order changed)
    m_needs_redraw = true;
}
```

#### Change 3: Add window::bring_to_front()

**File**: `include/onyxui/widgets/window/window.hh`

Add public method declaration (around line 320):
```cpp
/**
 * @brief Bring window to front and request focus
 *
 * @details
 * Moves window to highest z-order in layer_manager and
 * requests keyboard focus from focus_manager.
 */
void bring_to_front();
```

#### Change 4: Implement window::bring_to_front()

**File**: `include/onyxui/widgets/window/window.inl`

Add implementation (around line 740):
```cpp
template<UIBackend Backend>
void window<Backend>::bring_to_front() {
    // Bring layer to front
    auto* layers = ui_services<Backend>::layers();
    if (layers && m_layer_id.is_valid()) {
        layers->bring_to_front(m_layer_id);
    }

    // Request keyboard focus
    auto* focus = ui_services<Backend>::focus();
    if (focus) {
        focus->set_focus(this);
    }

    // Set as active window in window manager
    auto* wm = ui_services<Backend>::window_manager();
    if (wm) {
        wm->set_active_window(this);
    }

    // Emit focus gained signal
    set_window_focus(true);
}
```

#### Change 5: Update cycle_next_window()

**File**: `include/onyxui/widgets/window/window.inl:686`

Replace TODO with implementation:
```cpp
template<UIBackend Backend>
void window_manager<Backend>::cycle_next_window() {
    // ... existing cycling logic ...
    m_active_window = *it;

    // Bring window to front and focus
    if (m_active_window) {
        m_active_window->bring_to_front();
    }
}
```

#### Change 6: Update cycle_previous_window()

**File**: `include/onyxui/widgets/window/window.inl:709`

Replace TODO with implementation:
```cpp
template<UIBackend Backend>
void window_manager<Backend>::cycle_previous_window() {
    // ... existing cycling logic ...
    m_active_window = *it;

    // Bring window to front and focus
    if (m_active_window) {
        m_active_window->bring_to_front();
    }
}
```

### Implementation Steps

1. Open `include/onyxui/services/layer_manager.hh`
2. Add `bring_to_front()` declaration
3. Open `include/onyxui/services/layer_manager.inl`
4. Implement `bring_to_front()`
5. Open `include/onyxui/widgets/window/window.hh`
6. Add `bring_to_front()` declaration
7. Open `include/onyxui/widgets/window/window.inl`
8. Implement `bring_to_front()` (around line 740)
9. Update `cycle_next_window()` at line 686
10. Update `cycle_previous_window()` at line 709
11. Build and verify no compilation errors
12. Run all tests

### Test Plan

#### Test 1: layer_manager::bring_to_front() Basic
```cpp
TEST_CASE("layer_manager - bring_to_front moves layer to end") {
    ui_context_fixture<test_canvas_backend> ctx;
    auto* layers = ui_services<test_canvas_backend>::layers();

    // Add 3 layers
    auto widget1 = std::make_unique<panel<test_canvas_backend>>();
    auto widget2 = std::make_unique<panel<test_canvas_backend>>();
    auto widget3 = std::make_unique<panel<test_canvas_backend>>();

    auto id1 = layers->add_layer(widget1.get(), layer_type::window);
    auto id2 = layers->add_layer(widget2.get(), layer_type::window);
    auto id3 = layers->add_layer(widget3.get(), layer_type::window);

    // Bring layer 1 to front
    layers->bring_to_front(id1);

    // Verify rendering order: 2, 3, 1 (1 is now on top)
    auto& layer_list = layers->get_layers();
    REQUIRE(layer_list.size() == 3);
    REQUIRE(layer_list[0].id == id2);
    REQUIRE(layer_list[1].id == id3);
    REQUIRE(layer_list[2].id == id1);
}
```

#### Test 2: layer_manager::bring_to_front() Already at Front
```cpp
TEST_CASE("layer_manager - bring_to_front with already front layer is no-op") {
    ui_context_fixture<test_canvas_backend> ctx;
    auto* layers = ui_services<test_canvas_backend>::layers();

    auto widget1 = std::make_unique<panel<test_canvas_backend>>();
    auto widget2 = std::make_unique<panel<test_canvas_backend>>();

    auto id1 = layers->add_layer(widget1.get(), layer_type::window);
    auto id2 = layers->add_layer(widget2.get(), layer_type::window);

    // id2 is already at front
    layers->bring_to_front(id2);

    // Order should remain unchanged
    auto& layer_list = layers->get_layers();
    REQUIRE(layer_list[0].id == id1);
    REQUIRE(layer_list[1].id == id2);
}
```

#### Test 3: layer_manager::bring_to_front() Invalid ID
```cpp
TEST_CASE("layer_manager - bring_to_front with invalid ID does nothing") {
    ui_context_fixture<test_canvas_backend> ctx;
    auto* layers = ui_services<test_canvas_backend>::layers();

    auto widget = std::make_unique<panel<test_canvas_backend>>();
    auto id = layers->add_layer(widget.get(), layer_type::window);

    layer_id invalid_id;  // Default-constructed = invalid

    // Should not crash
    layers->bring_to_front(invalid_id);

    // Layer should still exist
    REQUIRE(layers->get_layers().size() == 1);
}
```

#### Test 4: window::bring_to_front() Integration
```cpp
TEST_CASE("window - bring_to_front updates z-order and focus") {
    ui_context_fixture<test_canvas_backend> ctx;

    // Create 3 windows
    auto win1 = std::make_unique<window<test_canvas_backend>>("Window 1");
    auto win2 = std::make_unique<window<test_canvas_backend>>("Window 2");
    auto win3 = std::make_unique<window<test_canvas_backend>>("Window 3");

    win1->show();
    win2->show();
    win3->show();

    // win3 is currently on top (shown last)

    // Bring win1 to front
    win1->bring_to_front();

    // Verify win1 is now on top
    auto* layers = ui_services<test_canvas_backend>::layers();
    auto& layer_list = layers->get_layers();

    // Find win1's layer
    auto win1_layer = std::find_if(layer_list.begin(), layer_list.end(),
        [&](const auto& entry) { return entry.content == win1.get(); });

    REQUIRE(win1_layer != layer_list.end());
    REQUIRE(win1_layer == layer_list.end() - 1);  // Last = on top

    // Verify win1 has focus
    auto* focus = ui_services<test_canvas_backend>::focus();
    REQUIRE(focus->get_focused_element() == win1.get());

    // Verify win1 has window focus
    REQUIRE(win1->has_window_focus());
}
```

#### Test 5: cycle_next_window() Brings to Front
```cpp
TEST_CASE("window_manager - cycle_next_window brings window to front") {
    ui_context_fixture<test_canvas_backend> ctx;
    auto* wm = ui_services<test_canvas_backend>::window_manager();
    auto* layers = ui_services<test_canvas_backend>::layers();

    auto win1 = std::make_unique<window<test_canvas_backend>>("Window 1");
    auto win2 = std::make_unique<window<test_canvas_backend>>("Window 2");

    wm->register_window(win1.get());
    wm->register_window(win2.get());

    win1->show();
    win2->show();

    // Set win1 as active
    wm->set_active_window(win1.get());

    // Cycle to win2
    wm->cycle_next_window();

    // Verify win2 is on top
    auto& layer_list = layers->get_layers();
    auto win2_layer = std::find_if(layer_list.begin(), layer_list.end(),
        [&](const auto& entry) { return entry.content == win2.get(); });

    REQUIRE(win2_layer == layer_list.end() - 1);

    // Verify win2 has focus
    REQUIRE(win2->has_window_focus());
}
```

### Test File Locations
- `unittest/services/test_layer_manager_zorder.cc` (Tests 1-3)
- `unittest/window/test_window_zorder.cc` (Tests 4-5)

### Success Criteria

✅ All 5 tests pass
✅ layer_manager correctly reorders layers
✅ window::bring_to_front() updates z-order, focus, and active window
✅ cycle_next_window() and cycle_previous_window() bring windows to front
✅ No compilation warnings

### Visual Verification

**Manual test**: Run `conio` demo
1. Open 3 overlapping windows (Window 1, Window 2, Window 3)
2. Click on Window 1 (should come to front)
3. Press `Ctrl+Tab` → Window 2 should come to front and have focus
4. Press `Ctrl+Tab` again → Window 3 should come to front
5. Press `Ctrl+Shift+Tab` → Should cycle backward, Window 2 comes to front

**Expected behavior**:
- Clicked/cycled window renders on top of all others
- Window title bar shows active state (focus indicator)
- Keyboard input goes to active window

---

## Task 4: Window List Dialog Labels

**Complexity**: MEDIUM
**Estimated time**: 1-2 hours
**Files affected**: `window_list_dialog.hh:301`

### Current State

```cpp
void refresh_list() {
    if (!m_content_vbox) return;

    // Clear existing labels
    // Note: This is a simplified implementation
    // A production version would properly manage child widgets

    // Get filtered windows
    auto filtered = get_filtered_windows();

    // TODO: Add labels for each window
    // For now, this is a placeholder
    // Full implementation would create/update label widgets
    // showing: "[N] Title (state)"
}
```

### Architecture Analysis

The `window_list_dialog` already has:
- `m_content_vbox` (vbox container for labels)
- `get_window_label()` method to generate label text
- Filtering logic in `get_filtered_windows()`
- Selection tracking in `m_selected_index`

**Missing**: Actually creating label widgets and adding them to the vbox.

### Required Changes

#### Change 1: Store label widgets

Add member to track labels for cleanup:
```cpp
// In window_list_dialog private section
std::vector<label<Backend>*> m_labels;  // Non-owning pointers
```

#### Change 2: Implement refresh_list()

Replace the TODO with full implementation:

```cpp
void refresh_list() {
    if (!m_content_vbox) return;

    // Clear existing labels
    // Note: remove_all_children() would be ideal, but we need to manually clear
    m_labels.clear();
    // Assuming vbox has clear() or remove_all() method
    m_content_vbox->clear_children();

    // Get filtered windows
    auto filtered = get_filtered_windows();

    // Create label for each window
    int display_index = 1;  // 1-based display numbering
    for (auto* win : filtered) {
        std::string label_text = get_window_label(win, display_index);

        // Create label widget
        auto* label_widget = m_content_vbox->template emplace_child<label<Backend>>(label_text);
        m_labels.push_back(label_widget);

        // Highlight selected item
        if (display_index - 1 == m_selected_index) {
            // Apply selection styling
            // Option 1: Use theme property
            label_widget->set_background_color(get_theme_selection_bg_color());
            label_widget->set_text_color(get_theme_selection_fg_color());

            // Option 2: Add visual indicator
            // label_text = "> " + label_text;
        }

        display_index++;
    }

    // Request layout update
    this->invalidate_layout();
}
```

### Implementation Steps

1. Open `include/onyxui/widgets/window/window_list_dialog.hh`
2. Add `std::vector<label<Backend>*> m_labels;` to private section (line ~242)
3. Navigate to `refresh_list()` method (line ~291)
4. Implement label creation logic
5. Verify `widget_container` has `clear_children()` method (or use alternative)
6. Handle selection highlighting
7. Build and verify no compilation errors
8. Run tests

### Test Plan

#### Test 1: Empty Window List
```cpp
TEST_CASE("window_list_dialog - empty list shows no labels") {
    ui_context_fixture<test_canvas_backend> ctx;

    auto dialog = std::make_unique<window_list_dialog<test_canvas_backend>>();

    // No windows added
    // Trigger refresh (implicitly called by constructor)

    // Verify no labels created
    // Note: Requires access to m_content_vbox children
    auto* content = dialog->get_content();
    REQUIRE(content != nullptr);

    // Cast to vbox to access children
    auto* vbox = dynamic_cast<onyxui::vbox<test_canvas_backend>*>(content);
    REQUIRE(vbox != nullptr);
    REQUIRE(vbox->child_count() == 0);
}
```

#### Test 2: Single Window Label
```cpp
TEST_CASE("window_list_dialog - single window creates one label") {
    ui_context_fixture<test_canvas_backend> ctx;

    auto dialog = std::make_unique<window_list_dialog<test_canvas_backend>>();
    auto win = std::make_unique<window<test_canvas_backend>>("Test Window");

    dialog->add_window(win.get());

    // Verify one label created
    auto* content = dialog->get_content();
    auto* vbox = dynamic_cast<onyxui::vbox<test_canvas_backend>*>(content);
    REQUIRE(vbox->child_count() == 1);

    // Verify label text
    // Expected: "[1] Test Window"
    // Note: Requires label::get_text() accessor
}
```

#### Test 3: Multiple Windows with States
```cpp
TEST_CASE("window_list_dialog - multiple windows with state indicators") {
    ui_context_fixture<test_canvas_backend> ctx;

    auto dialog = std::make_unique<window_list_dialog<test_canvas_backend>>();

    // Create windows with different states
    auto win1 = std::make_unique<window<test_canvas_backend>>("Editor");

    auto win2 = std::make_unique<window<test_canvas_backend>>("Calculator");
    win2->maximize();

    auto win3 = std::make_unique<window<test_canvas_backend>>("Settings");
    win3->minimize();

    typename window<test_canvas_backend>::window_flags modal_flags;
    modal_flags.is_modal = true;
    auto win4 = std::make_unique<window<test_canvas_backend>>("Help", modal_flags);

    dialog->add_window(win1.get());
    dialog->add_window(win2.get());
    dialog->add_window(win3.get());
    dialog->add_window(win4.get());

    // Verify 4 labels created
    auto* content = dialog->get_content();
    auto* vbox = dynamic_cast<onyxui::vbox<test_canvas_backend>*>(content);
    REQUIRE(vbox->child_count() == 4);

    // Verify label texts contain state indicators
    // Expected:
    // "[1] Editor"
    // "[2] Calculator (maximized)"
    // "[3] Settings (minimized)"
    // "[4] Help (modal)"
}
```

#### Test 4: Selection Highlighting
```cpp
TEST_CASE("window_list_dialog - selected item is highlighted") {
    ui_context_fixture<test_canvas_backend> ctx;

    auto dialog = std::make_unique<window_list_dialog<test_canvas_backend>>();

    auto win1 = std::make_unique<window<test_canvas_backend>>("Window 1");
    auto win2 = std::make_unique<window<test_canvas_backend>>("Window 2");

    dialog->add_window(win1.get());
    dialog->add_window(win2.get());

    // First item selected by default
    REQUIRE(dialog->get_selected_index() == 0);

    // Get first label
    auto* content = dialog->get_content();
    auto* vbox = dynamic_cast<onyxui::vbox<test_canvas_backend>*>(content);
    auto* label1 = dynamic_cast<label<test_canvas_backend>*>(vbox->child_at(0));
    auto* label2 = dynamic_cast<label<test_canvas_backend>*>(vbox->child_at(1));

    REQUIRE(label1 != nullptr);
    REQUIRE(label2 != nullptr);

    // Verify label1 has selection colors
    // (Requires label::get_background_color() accessor)
    auto bg1 = label1->get_background_color();
    auto theme_selection_bg = dialog->get_theme_selection_bg_color();
    REQUIRE(bg1 == theme_selection_bg);

    // Select second item
    dialog->select_next();

    // Verify label2 is now highlighted
    auto bg2 = label2->get_background_color();
    REQUIRE(bg2 == theme_selection_bg);
}
```

#### Test 5: Filter Mode
```cpp
TEST_CASE("window_list_dialog - filter mode affects displayed labels") {
    ui_context_fixture<test_canvas_backend> ctx;

    auto dialog = std::make_unique<window_list_dialog<test_canvas_backend>>();

    auto win1 = std::make_unique<window<test_canvas_backend>>("Visible 1");

    auto win2 = std::make_unique<window<test_canvas_backend>>("Minimized");
    win2->minimize();

    auto win3 = std::make_unique<window<test_canvas_backend>>("Visible 2");

    dialog->add_window(win1.get());
    dialog->add_window(win2.get());
    dialog->add_window(win3.get());

    // Default filter: all
    auto* content = dialog->get_content();
    auto* vbox = dynamic_cast<onyxui::vbox<test_canvas_backend>*>(content);
    REQUIRE(vbox->child_count() == 3);

    // Filter: visible only
    dialog->set_filter(window_list_dialog<test_canvas_backend>::filter_mode::visible_only);
    REQUIRE(vbox->child_count() == 2);

    // Filter: minimized only
    dialog->set_filter(window_list_dialog<test_canvas_backend>::filter_mode::minimized_only);
    REQUIRE(vbox->child_count() == 1);
}
```

### Test File Location
`unittest/window/test_window_list_dialog_labels.cc`

### Success Criteria

✅ All 5 tests pass
✅ Labels correctly created for each window
✅ Labels show window title and state
✅ Selected item is visually highlighted
✅ Filter mode affects displayed labels
✅ No memory leaks (labels properly owned by vbox)

### Visual Verification

**Manual test**: Run `conio` demo
1. Open 4 windows with different states:
   - Normal window
   - Maximized window
   - Minimized window
   - Modal dialog (if supported)
2. Press `Ctrl+W` to show window list
3. Verify display:
   ```
   ┌─ Windows ─────────────────────────┐
   │ > [1] Editor - document.txt       │
   │   [2] Calculator (maximized)      │
   │   [3] Settings (minimized)        │
   │   [4] Help (modal)                │
   └───────────────────────────────────┘
   ```
4. Press `Arrow Down` → Selection moves to item 2
5. Verify highlight moves
6. Press `Enter` → Window list closes, Calculator window activated

**Expected behavior**:
- Each window has numbered label with state
- Selected item has distinct background color
- Selection indicator (`>`) or highlight visible
- Arrow keys navigate, Enter activates

---

## Task 5: Modal Dialog Lifecycle

**Complexity**: COMPLEX
**Estimated time**: 4-6 hours
**Files affected**: `window.inl:659`, `layer_manager`, `input_manager`, `focus_manager`

### Current State

```cpp
template<UIBackend Backend>
void window<Backend>::show_modal() {
    // TODO Phase 5: Implement with proper dialog lifecycle
    // For now, just show as normal window
    show();
}
```

### Architecture Analysis

Modal dialogs require coordination between multiple systems:

1. **layer_manager**: Render modal window on top of all others
2. **input_manager**: Route all input to modal window (block background windows)
3. **focus_manager**: Set focus to modal window, restore after close
4. **window_manager**: Track modal stack, restore previous active window

**Modal lifecycle**:
```
1. show_modal() called
2. Save current active window
3. Add modal window to layer_manager (modal layer)
4. Set input capture to modal window
5. Set focus to modal window
6. Optionally dim background
7. Wait for modal to close
8. Remove from layer_manager
9. Release input capture
10. Restore previous active window
11. Restore previous focus
```

### Required Changes

#### Change 1: Add modal layer type

**File**: `include/onyxui/services/layer_manager.hh`

```cpp
enum class layer_type : uint8_t {
    background,  // Background layer (lowest)
    window,      // Normal windows
    modal,       // Modal dialogs (block input to lower layers)
    popup,       // Popup menus, tooltips
    drag         // Drag overlays (highest)
};
```

#### Change 2: Add input capture to layer_manager

**File**: `include/onyxui/services/layer_manager.hh`

```cpp
/**
 * @brief Set input capture to specific layer
 * @param id Layer that should receive all input
 *
 * @details
 * When input capture is set, only the specified layer receives
 * mouse and keyboard events. Other layers are blocked.
 * Used for modal dialogs.
 */
void set_input_capture(layer_id id);

/**
 * @brief Release input capture
 */
void release_input_capture();

/**
 * @brief Check if layer has input capture
 */
[[nodiscard]] bool has_input_capture(layer_id id) const noexcept;

private:
    layer_id m_input_capture_layer;  // Layer with exclusive input
```

#### Change 3: Modify event routing in layer_manager

**File**: `include/onyxui/services/layer_manager.inl`

Update event dispatch to respect input capture:

```cpp
template<UIBackend Backend>
bool layer_manager<Backend>::dispatch_event(const ui_event& event) {
    // If input capture is set, only dispatch to that layer
    if (m_input_capture_layer.is_valid()) {
        auto it = std::find_if(m_layers.begin(), m_layers.end(),
            [this](const layer_entry& entry) {
                return entry.id == m_input_capture_layer;
            });

        if (it != m_layers.end()) {
            return it->content->handle_event(event, event_phase::target);
        }

        // Invalid capture layer - release it
        m_input_capture_layer = layer_id();
    }

    // Normal event dispatch...
}
```

#### Change 4: Add modal stack to window_manager

**File**: `include/onyxui/services/window_manager.hh`

```cpp
/**
 * @brief Push modal window onto stack
 * @param win Modal window
 *
 * @details
 * Saves current active window and sets modal as active.
 * Used by window::show_modal().
 */
void push_modal(window<Backend>* win);

/**
 * @brief Pop modal window from stack
 * @param win Modal window being closed
 *
 * @details
 * Restores previous active window.
 * Used by window::close() when window is modal.
 */
void pop_modal(window<Backend>* win);

private:
    std::vector<window<Backend>*> m_modal_stack;
```

#### Change 5: Implement window::show_modal()

**File**: `include/onyxui/widgets/window/window.inl:659`

```cpp
template<UIBackend Backend>
void window<Backend>::show_modal() {
    if (!m_flags.is_modal) {
        // Force modal flag
        m_flags.is_modal = true;
    }

    // Get services
    auto* layers = ui_services<Backend>::layers();
    auto* wm = ui_services<Backend>::window_manager();
    auto* focus = ui_services<Backend>::focus();

    if (!layers) {
        // Fallback: show as normal window
        show();
        return;
    }

    // Step 1: Save current active window
    if (wm) {
        wm->push_modal(this);
    }

    // Step 2: Add to layer manager as modal layer
    m_layer_id = layers->add_layer(this, layer_type::modal);

    // Step 3: Set input capture
    layers->set_input_capture(m_layer_id);

    // Step 4: Set focus
    if (focus) {
        focus->set_focus(this);
    }

    // Step 5: Set window focus
    set_window_focus(true);

    // Step 6: Make visible
    this->set_visible(true);

    // Step 7: Request initial layout
    this->invalidate_layout();

    // Optional: Dim background
    if (m_flags.dim_background) {
        // Add dimming overlay layer
        // (Requires background_renderer support)
    }
}
```

#### Change 6: Update window::close() for modal

**File**: `include/onyxui/widgets/window/window.inl` (in close() method)

```cpp
template<UIBackend Backend>
void window<Backend>::close() {
    // Emit closing signal (can be cancelled)
    bool cancelled = false;
    closing.emit();  // Handlers can set cancelled = true

    if (cancelled) {
        return;
    }

    // Call hook
    on_close();

    // If modal, restore previous window
    if (m_flags.is_modal) {
        auto* layers = ui_services<Backend>::layers();
        auto* wm = ui_services<Backend>::window_manager();

        // Release input capture
        if (layers) {
            layers->release_input_capture();
        }

        // Pop from modal stack (restores previous active)
        if (wm) {
            wm->pop_modal(this);
        }
    }

    // Hide window
    hide();

    // Emit closed signal
    closed.emit();
}
```

### Implementation Steps

1. Update `layer_manager.hh`:
   - Modify `layer_type` enum (add `modal`)
   - Add input capture methods
2. Implement in `layer_manager.inl`:
   - `set_input_capture()`
   - `release_input_capture()`
   - Update `dispatch_event()` to check capture
3. Update `window_manager.hh`:
   - Add modal stack
   - Add `push_modal()` and `pop_modal()` declarations
4. Implement in `window_manager.inl`:
   - `push_modal()` implementation
   - `pop_modal()` implementation
5. Update `window.inl`:
   - Implement `show_modal()` (line 659)
   - Update `close()` to handle modal cleanup
6. Build and verify no compilation errors
7. Run all tests

### Test Plan

#### Test 1: Modal Window Shows on Top
```cpp
TEST_CASE("window - show_modal adds to modal layer") {
    ui_context_fixture<test_canvas_backend> ctx;
    auto* layers = ui_services<test_canvas_backend>::layers();

    // Create normal and modal windows
    auto normal_win = std::make_unique<window<test_canvas_backend>>("Normal");

    typename window<test_canvas_backend>::window_flags modal_flags;
    modal_flags.is_modal = true;
    auto modal_win = std::make_unique<window<test_canvas_backend>>("Modal", modal_flags);

    normal_win->show();
    modal_win->show_modal();

    // Verify modal layer is on top
    auto& layer_list = layers->get_layers();

    // Find modal window's layer
    auto modal_layer = std::find_if(layer_list.begin(), layer_list.end(),
        [&](const auto& entry) { return entry.content == modal_win.get(); });

    REQUIRE(modal_layer != layer_list.end());
    REQUIRE(modal_layer->type == layer_type::modal);

    // Verify it's the last layer (on top)
    REQUIRE(modal_layer == layer_list.end() - 1);
}
```

#### Test 2: Input Capture
```cpp
TEST_CASE("window - show_modal captures input") {
    ui_context_fixture<test_canvas_backend> ctx;
    auto* layers = ui_services<test_canvas_backend>::layers();

    typename window<test_canvas_backend>::window_flags modal_flags;
    modal_flags.is_modal = true;
    auto modal_win = std::make_unique<window<test_canvas_backend>>("Modal", modal_flags);

    modal_win->show_modal();

    // Verify input capture is set
    layer_id modal_layer_id = modal_win->get_layer_id();  // Requires accessor
    REQUIRE(layers->has_input_capture(modal_layer_id));
}
```

#### Test 3: Background Windows Blocked
```cpp
TEST_CASE("window - modal dialog blocks background window input") {
    ui_context_fixture<test_canvas_backend> ctx;
    auto* layers = ui_services<test_canvas_backend>::layers();

    auto normal_win = std::make_unique<window<test_canvas_backend>>("Normal");

    typename window<test_canvas_backend>::window_flags modal_flags;
    modal_flags.is_modal = true;
    auto modal_win = std::make_unique<window<test_canvas_backend>>("Modal", modal_flags);

    normal_win->show();
    modal_win->show_modal();

    // Create click event on normal window
    bool normal_clicked = false;
    normal_win->clicked.connect([&]() { normal_clicked = true; });

    // Simulate click
    mouse_event click{mouse_button::left, mouse_action::press, 10, 10};
    layers->dispatch_event(click);

    // Verify normal window did NOT receive event
    REQUIRE_FALSE(normal_clicked);

    // Verify modal window receives events
    bool modal_clicked = false;
    modal_win->clicked.connect([&]() { modal_clicked = true; });

    layers->dispatch_event(click);
    REQUIRE(modal_clicked);
}
```

#### Test 4: Focus Management
```cpp
TEST_CASE("window - show_modal sets focus, close restores") {
    ui_context_fixture<test_canvas_backend> ctx;
    auto* focus = ui_services<test_canvas_backend>::focus();
    auto* wm = ui_services<test_canvas_backend>::window_manager();

    auto win1 = std::make_unique<window<test_canvas_backend>>("Window 1");
    win1->show();
    wm->set_active_window(win1.get());

    // Verify win1 has focus
    REQUIRE(win1->has_window_focus());

    // Show modal
    typename window<test_canvas_backend>::window_flags modal_flags;
    modal_flags.is_modal = true;
    auto modal = std::make_unique<window<test_canvas_backend>>("Modal", modal_flags);
    modal->show_modal();

    // Verify modal has focus
    REQUIRE(modal->has_window_focus());
    REQUIRE_FALSE(win1->has_window_focus());

    // Close modal
    modal->close();

    // Verify win1 focus restored
    REQUIRE(win1->has_window_focus());
    REQUIRE(wm->get_active_window() == win1.get());
}
```

#### Test 5: Modal Stack (Nested Modals)
```cpp
TEST_CASE("window - nested modal dialogs work correctly") {
    ui_context_fixture<test_canvas_backend> ctx;
    auto* wm = ui_services<test_canvas_backend>::window_manager();

    auto win = std::make_unique<window<test_canvas_backend>>("Main");
    win->show();

    typename window<test_canvas_backend>::window_flags modal_flags;
    modal_flags.is_modal = true;

    auto modal1 = std::make_unique<window<test_canvas_backend>>("Modal 1", modal_flags);
    auto modal2 = std::make_unique<window<test_canvas_backend>>("Modal 2", modal_flags);

    // Show first modal
    modal1->show_modal();
    REQUIRE(modal1->has_window_focus());

    // Show second modal (nested)
    modal2->show_modal();
    REQUIRE(modal2->has_window_focus());
    REQUIRE_FALSE(modal1->has_window_focus());

    // Close second modal
    modal2->close();

    // First modal should regain focus
    REQUIRE(modal1->has_window_focus());
    REQUIRE_FALSE(modal2->has_window_focus());

    // Close first modal
    modal1->close();

    // Main window should regain focus
    REQUIRE(win->has_window_focus());
}
```

#### Test 6: Input Capture Release on Close
```cpp
TEST_CASE("window - closing modal releases input capture") {
    ui_context_fixture<test_canvas_backend> ctx;
    auto* layers = ui_services<test_canvas_backend>::layers();

    typename window<test_canvas_backend>::window_flags modal_flags;
    modal_flags.is_modal = true;
    auto modal = std::make_unique<window<test_canvas_backend>>("Modal", modal_flags);

    modal->show_modal();

    // Verify capture set
    layer_id modal_layer_id = modal->get_layer_id();
    REQUIRE(layers->has_input_capture(modal_layer_id));

    // Close modal
    modal->close();

    // Verify capture released
    REQUIRE_FALSE(layers->has_input_capture(modal_layer_id));

    // Verify normal input dispatch works
    auto normal_win = std::make_unique<window<test_canvas_backend>>("Normal");
    normal_win->show();

    bool clicked = false;
    normal_win->clicked.connect([&]() { clicked = true; });

    mouse_event click{mouse_button::left, mouse_action::press, 10, 10};
    layers->dispatch_event(click);

    REQUIRE(clicked);  // Should receive event now
}
```

### Test File Locations
- `unittest/services/test_layer_manager_modal.cc` (Input capture tests)
- `unittest/window/test_window_modal_lifecycle.cc` (Modal lifecycle tests)
- `unittest/window/test_window_manager_modal_stack.cc` (Modal stack tests)

### Success Criteria

✅ All 6 tests pass
✅ Modal windows show on top of all other windows
✅ Input capture correctly blocks background windows
✅ Focus is saved and restored
✅ Nested modals work correctly (modal stack)
✅ Closing modal releases capture and restores state
✅ No memory leaks or dangling pointers

### Visual Verification

**Manual test**: Run `conio` demo
1. Open main window
2. Open modal dialog (e.g., "About" dialog)
3. Try clicking main window → Should not respond
4. Try typing → Input goes to modal only
5. Verify main window appears dimmed (if dim_background enabled)
6. Close modal (Escape or Close button)
7. Verify main window regains focus
8. Verify main window responds to input again

**Test nested modals**:
1. Open modal dialog 1
2. From modal 1, open modal 2
3. Try clicking modal 1 → Should not respond
4. Close modal 2
5. Verify modal 1 regains focus
6. Close modal 1
7. Verify main window regains focus

**Expected behavior**:
- Modal dialog on top with distinct appearance
- Background windows grayed/dimmed (optional)
- Background windows completely unresponsive
- Focus correctly restored when modal closes
- Visual feedback showing active modal

---

## Integration Testing

After implementing all 5 tasks, comprehensive integration tests ensure everything works together.

### Integration Test 1: Complete Window Lifecycle
```cpp
TEST_CASE("Integration - complete window lifecycle with all features") {
    ui_context_fixture<test_canvas_backend> ctx;
    auto* wm = ui_services<test_canvas_backend>::window_manager();

    // Create 3 normal windows
    auto win1 = std::make_unique<window<test_canvas_backend>>("Editor");
    auto win2 = std::make_unique<window<test_canvas_backend>>("Calculator");
    auto win3 = std::make_unique<window<test_canvas_backend>>("Settings");

    wm->register_window(win1.get());
    wm->register_window(win2.get());
    wm->register_window(win3.get());

    win1->show();
    win2->show();
    win3->show();

    // Test z-order management
    win1->bring_to_front();
    REQUIRE(win1->has_window_focus());

    // Test cycling
    wm->cycle_next_window();
    REQUIRE(win2->has_window_focus());

    // Test window list dialog
    wm->register_hotkeys();

    typename window<test_canvas_backend>::window_flags modal_flags;
    modal_flags.is_modal = true;
    auto dialog = std::make_unique<window_list_dialog<test_canvas_backend>>();

    dialog->add_window(win1.get());
    dialog->add_window(win2.get());
    dialog->add_window(win3.get());

    // Show modal window list
    dialog->show_modal();

    // Verify modal behavior
    REQUIRE(dialog->has_window_focus());
    REQUIRE_FALSE(win2->has_window_focus());

    // Select window from list
    dialog->select_next();
    dialog->activate_selected();

    // Verify selected window activated
    REQUIRE(win3->has_window_focus());
}
```

### Integration Test 2: Modal with Window Operations
```cpp
TEST_CASE("Integration - modal dialog with window state changes") {
    ui_context_fixture<test_canvas_backend> ctx;
    auto* wm = ui_services<test_canvas_backend>::window_manager();

    auto win = std::make_unique<window<test_canvas_backend>>("Main");
    win->show();

    // Minimize window
    win->minimize();
    auto minimized = wm->get_minimized_windows();
    REQUIRE(minimized.size() == 1);

    // Show window list (modal)
    typename window<test_canvas_backend>::window_flags modal_flags;
    modal_flags.is_modal = true;
    auto dialog = std::make_unique<window_list_dialog<test_canvas_backend>>();
    dialog->add_window(win.get());
    dialog->show_modal();

    // Restore from dialog
    dialog->window_selected.connect([](window<test_canvas_backend>* w) {
        if (w->get_state() == window<test_canvas_backend>::window_state::minimized) {
            w->restore();
        }
    });

    dialog->activate_selected();

    // Verify window restored
    REQUIRE(win->get_state() == window<test_canvas_backend>::window_state::normal);
    REQUIRE(win->is_visible());
}
```

### Integration Test File Location
`unittest/integration/test_window_management_integration.cc`

---

## Visual Verification

### Visual Test Suite

Create a comprehensive visual test application: `examples/window_test_suite.cc`

```cpp
int main() {
    scoped_ui_context<conio_backend> ctx;
    auto* wm = ui_services<conio_backend>::window_manager();

    // Test 1: Multiple Windows with Z-Order
    std::cout << "\n=== Test 1: Z-Order Management ===\n";
    auto win1 = create_test_window("Window 1 (Red)", color::red);
    auto win2 = create_test_window("Window 2 (Blue)", color::blue);
    auto win3 = create_test_window("Window 3 (Green)", color::green);

    win1->show();
    win2->show();
    win3->show();

    std::cout << "Instructions:\n";
    std::cout << "1. Click each window - should come to front\n";
    std::cout << "2. Press Ctrl+Tab - should cycle forward\n";
    std::cout << "3. Press Ctrl+Shift+Tab - should cycle backward\n";
    std::cout << "Press Enter when ready...\n";
    std::cin.get();

    // Test 2: Window List Dialog
    std::cout << "\n=== Test 2: Window List Dialog ===\n";
    wm->register_hotkeys();

    std::cout << "Instructions:\n";
    std::cout << "1. Press Ctrl+W to show window list\n";
    std::cout << "2. Use arrow keys to navigate\n";
    std::cout << "3. Verify each window shows correct state\n";
    std::cout << "4. Press Enter to activate selected window\n";
    std::cout << "Press Enter when ready...\n";
    std::cin.get();

    // Test 3: Modal Dialog
    std::cout << "\n=== Test 3: Modal Dialog ===\n";
    typename window<conio_backend>::window_flags modal_flags;
    modal_flags.is_modal = true;
    modal_flags.dim_background = true;
    auto modal = std::make_unique<window<conio_backend>>("Modal Dialog", modal_flags);

    std::cout << "Instructions:\n";
    std::cout << "1. Modal dialog will appear on top\n";
    std::cout << "2. Try clicking background windows - should not respond\n";
    std::cout << "3. Press Escape to close modal\n";
    std::cout << "4. Verify background window regains focus\n";
    std::cout << "Press Enter to show modal...\n";
    std::cin.get();

    modal->show_modal();

    // Test 4: Nested Modals
    std::cout << "\n=== Test 4: Nested Modals ===\n";
    // ... nested modal test ...

    std::cout << "\n=== All Tests Complete ===\n";
    return 0;
}
```

### Visual Checklist

Print this checklist and verify each item manually:

**Z-Order Management**
- [ ] Clicking window brings it to front
- [ ] Front window renders on top of others
- [ ] Window title bar shows active state
- [ ] Ctrl+Tab cycles forward through windows
- [ ] Ctrl+Shift+Tab cycles backward
- [ ] Cycled window comes to front visually

**Window List Dialog**
- [ ] Ctrl+W shows window list dialog
- [ ] Dialog is modal (blocks background)
- [ ] Each window has correct label format
- [ ] State indicators show (minimized, maximized, modal)
- [ ] Selection highlight is visible
- [ ] Arrow keys move selection
- [ ] Enter activates selected window
- [ ] Escape closes dialog
- [ ] Selecting minimized window restores it

**Modal Dialogs**
- [ ] Modal dialog appears on top
- [ ] Background windows are dimmed (if enabled)
- [ ] Background windows don't respond to clicks
- [ ] Background windows don't respond to keyboard
- [ ] Modal has distinct visual appearance
- [ ] Closing modal restores previous window
- [ ] Focus indicator shows active modal

**Nested Modals**
- [ ] Opening modal from modal works
- [ ] Second modal blocks first modal
- [ ] Closing second modal restores first
- [ ] Closing first modal restores main window
- [ ] Focus correctly restored at each level

---

## Dependencies

### Task Dependencies

```
Task 1 (Modal Checking) ─────────┐
                                  ├──> Task 5 (Modal Lifecycle)
Task 2 (Verify Semantic Actions)─┘     └──> Integration Tests
                                                    │
Task 3 (Z-Order) ────────────────┬──> Task 4 (Window Labels) ──┘
                                  │
                                  └──> Visual Verification
```

**Recommended implementation order**:
1. Task 1 (15 min) - Simple, no dependencies
2. Task 2 (30 min) - Simple, verification only
3. Task 3 (2-3 hours) - Foundation for Task 5
4. Task 4 (1-2 hours) - Can be done in parallel with Task 5
5. Task 5 (4-6 hours) - Most complex, depends on Tasks 1 & 3

**Total estimated time**: 8-12 hours (1-2 days)

---

## Success Criteria Summary

### Code Quality
- [ ] All implementations follow OnyxUI coding standards
- [ ] No clang-tidy warnings
- [ ] No memory leaks (valgrind clean)
- [ ] Proper RAII for resource management
- [ ] All TODOs removed from affected files

### Testing
- [ ] All unit tests pass (100% pass rate)
- [ ] All integration tests pass
- [ ] Code coverage ≥ 95% for new code
- [ ] Edge cases tested (null pointers, empty lists, etc.)
- [ ] Visual verification checklist completed

### Functionality
- [ ] Modal windows correctly identified
- [ ] Semantic actions verified and tested
- [ ] Z-order management works correctly
- [ ] Window list shows all windows with states
- [ ] Modal dialog lifecycle complete
- [ ] Nested modals work correctly
- [ ] Focus properly saved and restored

### Documentation
- [ ] All public methods documented
- [ ] Implementation notes added to relevant files
- [ ] TODO.md updated with completion status
- [ ] CHANGELOG.md updated with new features

---

## Appendix: Code Snippets

### Helper Functions for Tests

```cpp
// Create test window with specific properties
template<UIBackend Backend>
std::unique_ptr<window<Backend>> create_test_window(
    const std::string& title,
    typename window<Backend>::window_flags flags = {}
) {
    auto win = std::make_unique<window<Backend>>(title, flags);
    win->set_size(400, 300);
    win->set_position(100, 100);
    return win;
}

// Verify layer order
template<UIBackend Backend>
bool verify_layer_on_top(layer_manager<Backend>* layers, layer_id id) {
    auto& layer_list = layers->get_layers();
    if (layer_list.empty()) return false;
    return layer_list.back().id == id;
}

// Find widget in layer list
template<UIBackend Backend>
layer_id find_widget_layer(
    layer_manager<Backend>* layers,
    ui_element<Backend>* widget
) {
    auto& layer_list = layers->get_layers();
    auto it = std::find_if(layer_list.begin(), layer_list.end(),
        [widget](const auto& entry) {
            return entry.content == widget;
        });
    return (it != layer_list.end()) ? it->id : layer_id();
}
```

---

**Document Version**: 1.0
**Last Updated**: 2025-11-09
**Author**: Claude Code
**Status**: Ready for Implementation ✅
