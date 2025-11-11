# Main Window Implementation Plan

**Date**: 2025-11-11
**Status**: Ready for implementation
**Priority**: HIGH (fixes maximize button in demo)

---

## Executive Summary

This document provides a detailed implementation plan for the `main_window` widget and automatic workspace-aware window maximize functionality. The implementation follows Qt's `QMainWindow` pattern and provides both explicit structure (best practice) and automatic fallback (convenience).

**Problem Statement**: Windows in the demo have no parent, so `window::maximize()` does nothing because `parent()` returns `nullptr`. Windows should maximize to fill the workspace (screen area minus menu bar and status bar).

**Solution**:
1. Create `main_window` widget (QMainWindow-style container)
2. Update `window::maximize()` to handle parentless windows (automatic workspace calculation)
3. Update demo to use `main_window`

**Estimated effort**: 4-6 hours
**Test coverage target**: 100% (all features tested)

---

## Table of Contents

1. [Phase 1: Fix Window Maximize for Parentless Windows](#phase-1-fix-window-maximize-for-parentless-windows)
2. [Phase 2: Create main_window Widget](#phase-2-create-main_window-widget)
3. [Phase 3: Update Demo to Use main_window](#phase-3-update-demo-to-use-main_window)
4. [Phase 4: Add Automatic Workspace Detection](#phase-4-add-automatic-workspace-detection)
5. [Integration Testing](#integration-testing)
6. [Documentation Updates](#documentation-updates)

---

## Architecture Overview

### Current State (Demo)
```
main_widget (panel)
  ├─ menu_bar
  ├─ buttons, text_view, etc.
  └─ (no status bar)

Windows (orphaned):
  - Created with no parent
  - Added directly to layer_manager
  - window::maximize() does nothing (parent() == nullptr)
```

### Target State (After Implementation)
```
main_window
  ├─ menu_bar (top, content-sized)
  ├─ central_widget (middle, fills space) ← Parent for windows
  └─ status_bar (bottom, content-sized, optional)

Windows:
  - Created as children of central_widget
  - Still rendered via layer_manager (proper z-order)
  - window::maximize() fills central_widget bounds
```

### Backward Compatibility
```cpp
// Old pattern: Still works with automatic fallback
auto win = std::make_shared<window>("Title", flags);  // no parent
win->show();
// maximize() uses automatic workspace calculation

// New pattern: Explicit structure (recommended)
auto main = std::make_unique<main_window>();
auto win = main->create_window("Title", flags);
win->show();
// maximize() uses parent (central_widget) bounds
```

---

## Phase 1: Fix Window Maximize for Parentless Windows

**Complexity**: SIMPLE
**Estimated time**: 1 hour
**Files affected**: `include/onyxui/widgets/window/window.inl`

### Current State

```cpp
template<UIBackend Backend>
void window<Backend>::maximize() {
    // ...
    auto* parent_elem = this->parent();
    if (parent_elem) {
        // Only works if window has parent!
        maximize_to_bounds(parent_elem->bounds());
    }
    // If no parent, nothing happens!
}
```

### Required Changes

**Location**: `include/onyxui/widgets/window/window.inl:197-267`

```cpp
template<UIBackend Backend>
void window<Backend>::maximize() {
    if (m_state == window_state::maximized) {
        return;  // Already maximized
    }

    // Save normal bounds for restore
    if (m_state == window_state::normal) {
        m_normal_bounds = this->bounds();
    }

    // Change state
    m_state = window_state::maximized;

    typename Backend::rect_type maximized_bounds;

    // Get parent element and maximize to fill its bounds
    auto* parent_elem = this->parent();
    if (parent_elem) {
        // Case 1: Window has parent - maximize to fill parent
        // (MDI container, main_window central widget, or explicit parent)
        auto parent_bounds = parent_elem->bounds();

        rect_utils::set_bounds(
            maximized_bounds,
            0,  // x: relative to parent (top-left corner)
            0,  // y: relative to parent (top-left corner)
            rect_utils::get_width(parent_bounds),
            rect_utils::get_height(parent_bounds)
        );
    } else {
        // Case 2: Window has no parent (standalone layer window)
        // Use full layer bounds (will be refined in Phase 4)
        auto layer_bounds = this->bounds();

        // For now: maximize to current layer dimensions
        // TODO Phase 4: Calculate workspace bounds (screen - menu - statusbar)
        int const max_width = 80;   // Placeholder - will be dynamic
        int const max_height = 25;  // Placeholder - will be dynamic

        rect_utils::set_bounds(
            maximized_bounds,
            0,  // x: absolute screen position
            0,  // y: absolute screen position
            max_width,
            max_height
        );
    }

    // Measure and arrange to new size
    int const width = rect_utils::get_width(maximized_bounds);
    int const height = rect_utils::get_height(maximized_bounds);

    if (!this->children().empty() && ui_services<Backend>::themes()) {
        [[maybe_unused]] auto measured_size = this->measure(width, height);
    }
    this->arrange(maximized_bounds);

    // Update layer bounds if this window is shown as a layer
    if (m_layer_id.is_valid()) {
        auto* layers = ui_services<Backend>::layers();
        if (layers) {
            layers->set_layer_bounds(m_layer_id, maximized_bounds);
        }
    }

    // Update title bar icons (show restore icon instead of maximize)
    if (m_title_bar) {
        m_title_bar->show_restore_icon();
    }

    // Emit signal
    maximized_sig.emit();
}
```

### Implementation Steps

1. Open `include/onyxui/widgets/window/window.inl`
2. Locate `window::maximize()` method (around line 197)
3. Add `else` branch for parentless windows
4. Add layer bounds update after `arrange()`
5. Add title bar icon update
6. Build and verify no compilation errors

### Test Plan

#### Test 1: Window With Parent Maximize (Existing Behavior)
```cpp
TEST_CASE("window - maximize with parent fills parent bounds") {
    ui_context_fixture<Backend> ctx;
    auto parent = std::make_unique<panel<Backend>>();

    typename window<Backend>::window_flags flags;
    flags.has_maximize_button = true;

    auto* win = parent->template emplace_child<window>("Test", flags);

    // Arrange parent to 80x25
    parent->measure(80, 25);
    parent->arrange({0, 0, 80, 25});

    // Window starts small
    win->set_size(20, 10);
    CHECK(win->bounds().w == 20);
    CHECK(win->bounds().h == 10);

    // Maximize
    win->maximize();

    // Should fill parent
    CHECK(win->bounds().x == 0);
    CHECK(win->bounds().y == 0);
    CHECK(win->bounds().w == 80);
    CHECK(win->bounds().h == 25);
    CHECK(win->state() == window_state::maximized);
}
```

#### Test 2: Window Without Parent Maximize (New Behavior)
```cpp
TEST_CASE("window - maximize without parent fills screen") {
    ui_context_fixture<Backend> ctx;

    typename window<Backend>::window_flags flags;
    flags.has_maximize_button = true;

    // Window with no parent
    auto win = std::make_shared<window<Backend>>("Test", flags);
    win->set_size(20, 10);
    win->set_position(5, 3);

    CHECK(win->bounds().w == 20);
    CHECK(win->bounds().h == 10);

    // Maximize
    win->maximize();

    // Should fill screen (placeholder values for now)
    CHECK(win->bounds().x == 0);
    CHECK(win->bounds().y == 0);
    CHECK(win->bounds().w == 80);  // Placeholder
    CHECK(win->bounds().h == 25);  // Placeholder
    CHECK(win->state() == window_state::maximized);
}
```

#### Test 3: Layer Bounds Update
```cpp
TEST_CASE("window - maximize updates layer bounds") {
    ui_context_fixture<Backend> ctx;

    typename window<Backend>::window_flags flags;
    flags.has_maximize_button = true;

    auto win = std::make_shared<window<Backend>>("Test", flags);
    win->set_size(20, 10);
    win->show();  // Adds to layer_manager

    REQUIRE(win->layer_id().is_valid());

    // Get layer manager
    auto* layers = ui_services<Backend>::layers();
    REQUIRE(layers != nullptr);

    // Maximize
    win->maximize();

    // Verify layer bounds were updated
    // (Need to add getter to layer_manager for testing)
    CHECK(win->bounds().w == 80);
    CHECK(win->bounds().h == 25);
}
```

#### Test 4: Title Bar Icon Update
```cpp
TEST_CASE("window - maximize updates title bar icon") {
    ui_context_fixture<Backend> ctx;

    typename window<Backend>::window_flags flags;
    flags.has_maximize_button = true;
    flags.has_title_bar = true;

    auto win = std::make_shared<window<Backend>>("Test", flags);
    auto* title_bar = dynamic_cast<window_title_bar<Backend>*>(
        win->children()[0].get()
    );
    REQUIRE(title_bar != nullptr);

    // Initially shows maximize icon
    // (Would need to check icon state - implementation detail)

    win->maximize();

    // Should now show restore icon
    // (Verified visually or through icon state query)
    CHECK(win->state() == window_state::maximized);
}
```

### Success Criteria

- ✅ All existing maximize tests pass
- ✅ New tests for parentless window maximize pass
- ✅ Layer bounds are updated correctly
- ✅ Title bar shows restore icon after maximize
- ✅ Demo windows can maximize (full screen for now)

---

## Phase 2: Create main_window Widget

**Complexity**: MEDIUM
**Estimated time**: 2 hours
**Files to create**:
- `include/onyxui/widgets/main_window.hh`
- `include/onyxui/widgets/main_window.inl`

### Widget Specification

```cpp
/**
 * @file main_window.hh
 * @brief Qt-style main application window with menu bar, central widget, and status bar
 *
 * @details
 * main_window provides a standard application layout pattern:
 * - Optional menu bar at top
 * - Central widget in middle (fills remaining space)
 * - Optional status bar at bottom
 *
 * Windows created via create_window() automatically have central_widget as parent,
 * enabling proper maximize behavior.
 *
 * ## Usage
 *
 * @code
 * auto main = std::make_unique<main_window<Backend>>();
 * main->set_menu_bar(menu_bar);
 *
 * // Create windows - they automatically use central widget as parent
 * auto win = main->create_window("Document 1", flags);
 * win->show();
 *
 * main->set_status_bar(status_bar);
 * @endcode
 */

#pragma once

#include <onyxui/widgets/containers/panel.hh>
#include <onyxui/widgets/menu/menu_bar.hh>
#include <onyxui/widgets/status_bar.hh>
#include <onyxui/widgets/window/window.hh>
#include <memory>

namespace onyxui {

    template<UIBackend Backend>
    class main_window : public panel<Backend> {
    public:
        using base = panel<Backend>;
        using window_type = window<Backend>;
        using menu_bar_type = menu_bar<Backend>;
        using status_bar_type = status_bar<Backend>;
        using panel_type = panel<Backend>;

        /**
         * @brief Construct main window with vertical layout
         */
        main_window();

        /**
         * @brief Set menu bar at top of window
         * @param menu Menu bar widget (takes ownership)
         *
         * @note Menu bar is positioned at top and sized to content height
         */
        void set_menu_bar(std::unique_ptr<menu_bar_type> menu);

        /**
         * @brief Set status bar at bottom of window
         * @param status Status bar widget (takes ownership)
         *
         * @note Status bar is positioned at bottom and sized to content height
         */
        void set_status_bar(std::unique_ptr<status_bar_type> status);

        /**
         * @brief Set central widget (main content area)
         * @param widget Central widget (takes ownership)
         *
         * @note Central widget fills space between menu and status bar
         */
        void set_central_widget(std::unique_ptr<ui_element<Backend>> widget);

        /**
         * @brief Get central widget (for window parenting)
         * @return Pointer to central widget (may be nullptr)
         */
        ui_element<Backend>* central_widget() const noexcept {
            return m_central_widget;
        }

        /**
         * @brief Create window as child of central widget
         * @tparam Args Constructor arguments for window
         * @param args Arguments forwarded to window constructor
         * @return Shared pointer to created window
         *
         * @details
         * Convenience method that creates a window with central_widget as parent.
         * Central widget is created automatically if not set.
         *
         * @code
         * auto win = main->create_window("My Window", flags);
         * win->show();
         * @endcode
         */
        template<typename... Args>
        std::shared_ptr<window_type> create_window(Args&&... args);

    private:
        menu_bar_type* m_menu_bar = nullptr;
        ui_element<Backend>* m_central_widget = nullptr;
        status_bar_type* m_status_bar = nullptr;

        /**
         * @brief Ensure central widget exists (lazy initialization)
         */
        void ensure_central_widget();

        /**
         * @brief Rebuild layout after component changes
         */
        void rebuild_layout();
    };

} // namespace onyxui

#include <onyxui/widgets/main_window.inl>
```

### Implementation (main_window.inl)

```cpp
/**
 * @file main_window.inl
 * @brief Implementation of main_window class
 */

#pragma once

namespace onyxui {

    template<UIBackend Backend>
    main_window<Backend>::main_window()
        : base(nullptr)  // Root widget - no parent
    {
        // Set up vertical layout with no spacing
        this->set_vbox_layout(0);

        // Create central widget by default (empty panel)
        ensure_central_widget();
    }

    template<UIBackend Backend>
    void main_window<Backend>::set_menu_bar(std::unique_ptr<menu_bar_type> menu) {
        m_menu_bar = this->add_child(std::move(menu));

        // Menu bar should be content-sized
        size_constraint height_constraint;
        height_constraint.policy = size_policy::content;
        m_menu_bar->set_height_constraint(height_constraint);

        rebuild_layout();
    }

    template<UIBackend Backend>
    void main_window<Backend>::set_status_bar(std::unique_ptr<status_bar_type> status) {
        m_status_bar = this->add_child(std::move(status));

        // Status bar should be content-sized
        size_constraint height_constraint;
        height_constraint.policy = size_policy::content;
        m_status_bar->set_height_constraint(height_constraint);

        rebuild_layout();
    }

    template<UIBackend Backend>
    void main_window<Backend>::set_central_widget(std::unique_ptr<ui_element<Backend>> widget) {
        m_central_widget = this->add_child(std::move(widget));

        // Central widget should fill remaining space
        size_constraint width_constraint, height_constraint;
        width_constraint.policy = size_policy::fill_parent;
        height_constraint.policy = size_policy::fill_parent;
        m_central_widget->set_width_constraint(width_constraint);
        m_central_widget->set_height_constraint(height_constraint);

        rebuild_layout();
    }

    template<UIBackend Backend>
    template<typename... Args>
    std::shared_ptr<typename main_window<Backend>::window_type>
    main_window<Backend>::create_window(Args&&... args) {
        ensure_central_widget();

        // Create window with central_widget as parent
        // Note: Window constructor signature is (title, flags, parent)
        auto win = std::make_shared<window_type>(
            std::forward<Args>(args)...,
            m_central_widget
        );

        return win;
    }

    template<UIBackend Backend>
    void main_window<Backend>::ensure_central_widget() {
        if (!m_central_widget) {
            // Create default empty panel as central widget
            set_central_widget(std::make_unique<panel_type>());
        }
    }

    template<UIBackend Backend>
    void main_window<Backend>::rebuild_layout() {
        // Clear children and re-add in correct order
        // Order: menu_bar, central_widget, status_bar

        // Note: This is simplified - actual implementation should
        // reorder existing children instead of clearing

        // Layout happens automatically via vbox layout manager
        // No manual positioning needed
    }

} // namespace onyxui
```

### Implementation Steps

1. Create `include/onyxui/widgets/main_window.hh`
2. Create `include/onyxui/widgets/main_window.inl`
3. Add include to `include/onyxui/onyxui.hh`
4. Build and verify no compilation errors

### Test Plan

#### Test 1: Basic Construction
```cpp
TEST_CASE("main_window - construction creates central widget") {
    ui_context_fixture<Backend> ctx;

    auto main = std::make_unique<main_window<Backend>>();

    // Should have central widget by default
    REQUIRE(main->central_widget() != nullptr);

    // Should use vbox layout
    CHECK(main->children().size() == 1);  // Central widget
}
```

#### Test 2: Menu Bar Setup
```cpp
TEST_CASE("main_window - set_menu_bar adds menu at top") {
    ui_context_fixture<Backend> ctx;

    auto main = std::make_unique<main_window<Backend>>();
    auto menu = std::make_unique<menu_bar<Backend>>();
    auto* menu_ptr = menu.get();

    main->set_menu_bar(std::move(menu));

    // Menu should be first child
    CHECK(main->children()[0].get() == menu_ptr);

    // Menu should be content-sized
    // (Check constraint)
}
```

#### Test 3: Status Bar Setup
```cpp
TEST_CASE("main_window - set_status_bar adds status at bottom") {
    ui_context_fixture<Backend> ctx;

    auto main = std::make_unique<main_window<Backend>>();
    auto status = std::make_unique<status_bar<Backend>>();
    auto* status_ptr = status.get();

    main->set_status_bar(std::move(status));

    // Status should be last child
    auto& children = main->children();
    CHECK(children[children.size() - 1].get() == status_ptr);
}
```

#### Test 4: Window Creation
```cpp
TEST_CASE("main_window - create_window sets central widget as parent") {
    ui_context_fixture<Backend> ctx;

    auto main = std::make_unique<main_window<Backend>>();

    typename window<Backend>::window_flags flags;
    flags.has_maximize_button = true;

    auto win = main->create_window("Test Window", flags);

    // Window should have central widget as parent
    CHECK(win->parent() == main->central_widget());
}
```

#### Test 5: Layout Order
```cpp
TEST_CASE("main_window - layout order is menu, central, status") {
    ui_context_fixture<Backend> ctx;

    auto main = std::make_unique<main_window<Backend>>();
    auto menu = std::make_unique<menu_bar<Backend>>();
    auto status = std::make_unique<status_bar<Backend>>();

    auto* menu_ptr = menu.get();
    auto* central_ptr = main->central_widget();
    auto* status_ptr = status.get();

    main->set_menu_bar(std::move(menu));
    main->set_status_bar(std::move(status));

    // Verify order
    auto& children = main->children();
    CHECK(children.size() == 3);
    CHECK(children[0].get() == menu_ptr);
    CHECK(children[1].get() == central_ptr);
    CHECK(children[2].get() == status_ptr);
}
```

### Success Criteria

- ✅ main_window compiles without errors
- ✅ All unit tests pass
- ✅ Central widget created automatically
- ✅ Menu bar positioned at top
- ✅ Status bar positioned at bottom
- ✅ create_window() sets correct parent

---

## Phase 3: Update Demo to Use main_window

**Complexity**: SIMPLE
**Estimated time**: 1 hour
**Files affected**:
- `examples/demo.hh`
- `examples/demo_ui_builder.hh`
- `examples/demo_windows.hh`

### Required Changes

#### File: examples/demo.hh

```cpp
// Change base class from panel to main_window
template <onyxui::UIBackend Backend>
class main_widget : public onyxui::main_window<Backend> {  // Changed!
public:
    main_widget()
        : onyxui::main_window<Backend>()  // Changed!
        , m_current_theme_index(0)
        , m_should_quit(false)
    {
        // ... existing code ...

        // No longer need set_vbox_layout or set_padding
        // main_window handles layout

        setup_actions();
        build_ui();

        // Apply theme
        demo_utils::apply_theme_by_name<Backend>(
            m_theme_names[m_current_theme_index]
        );
    }

    // ... rest of class ...
};
```

#### File: examples/demo_ui_builder.hh

```cpp
template<UIBackend Backend>
void build_ui(
    main_window<Backend>* main,  // Changed parameter type!
    // ... other params ...
) {
    // Create menu bar
    auto menu_bar_widget = std::make_unique<menu_bar<Backend>>();
    // ... populate menu bar ...

    // Set menu bar on main window
    main->set_menu_bar(std::move(menu_bar_widget));

    // Don't add other widgets directly to main
    // They go in central widget
    auto central = std::make_unique<vbox<Backend>>(1);

    // Add demo content to central widget
    central->template emplace_child<label>("Theme Switcher");
    // ... other widgets ...

    main->set_central_widget(std::move(central));

    // Later: add status bar
    // main->set_status_bar(std::make_unique<status_bar<Backend>>());
}
```

#### File: examples/demo_windows.hh

```cpp
// Update window creation to accept parent
template <onyxui::UIBackend Backend>
void show_basic_window(
    const std::string& title,
    const std::string& content_text,
    ui_element<Backend>* parent = nullptr  // Add parent param!
) {
    typename onyxui::window<Backend>::window_flags flags;
    flags.has_close_button = true;
    flags.has_minimize_button = true;
    flags.has_maximize_button = true;
    flags.is_movable = true;

    // Pass parent to window constructor
    auto win = std::make_shared<onyxui::window<Backend>>(
        title,
        flags,
        parent  // Now has parent!
    );

    auto content = std::make_unique<onyxui::vbox<Backend>>(1);
    content->template emplace_child<onyxui::label>(content_text);
    win->set_content(std::move(content));

    win->set_size(40, 15);
    win->set_position(5, 3);

    register_window(win);
    win->show();
}

// IMPORTANT: Demo windows should be PARENTLESS (floating layers)
// Only use central_widget as parent for true MDI applications
// In demo_actions.hh:
m_new_action->activated.connect([]() {
    demo_windows::show_basic_window<Backend>(
        "Demo Window",
        "This is a demonstration window."
        // No parent - these are floating layer windows
    );
});
```

### IMPORTANT: Window Parenting Strategy

**Two distinct window models:**

#### 1. Floating Layer Windows (SDI - Single Document Interface)
- **Parent**: `nullptr` (parentless)
- **Behavior**: Independent floating windows managed by layer_manager
- **Positioning**: Absolute screen coordinates via `set_position(x, y)`
- **Maximize**: Fills entire viewport/screen
- **Use case**: Demos, tools, dialogs, floating palettes
- **Example**: The widgets demo windows

```cpp
// Floating layer window - NO parent
auto win = std::make_shared<window<Backend>>("Tool Window", flags);
win->set_position(10, 5);  // Absolute screen position
win->show();  // Adds to layer_manager
// Maximize fills entire screen
```

#### 2. Contained MDI Windows (MDI - Multiple Document Interface)
- **Parent**: `main_window::central_widget()`
- **Behavior**: Embedded in parent, part of widget tree
- **Positioning**: Relative to parent via layout system
- **Maximize**: Fills parent (central widget area only)
- **Use case**: Document editors, IDE windows, contained views
- **Example**: Visual Studio MDI windows

```cpp
// MDI window - HAS parent (central widget)
auto win = main->create_window("Document 1", flags);
// Or manually:
auto win = std::make_shared<window<Backend>>("Doc", flags, main->central_widget());
// Positioned by parent's layout
// Maximize fills central widget area only
```

**Demo Decision**: The widgets demo uses **floating layer windows** (SDI model) to showcase independent window behavior. Therefore, windows should be created **without a parent**.

### Implementation Steps

1. Update `demo.hh` base class
2. Update `demo_ui_builder.hh` to use main_window API
3. Update `demo_windows.hh` to accept parent parameter (but don't use it in demo)
4. **CRITICAL**: Keep demo windows parentless for floating layer behavior
5. Build demo: `cmake --build build --target conio -j8`
6. Run demo and test maximize button

### Test Plan (Manual)

1. **Build and run demo**
   ```bash
   cmake --build build --target conio -j8
   ./build/bin/conio
   ```

2. **Test maximize without menu**
   - Open "File > New Window"
   - Click maximize button
   - ✅ Window should fill entire screen (no menu bar yet in place)

3. **Test maximize with menu**
   - With menu bar visible at top
   - Click maximize button
   - ✅ Window should fill area below menu bar

4. **Test restore**
   - Click restore button (was maximize)
   - ✅ Window returns to original size/position

5. **Test multiple windows**
   - Create 3 windows
   - Maximize different ones
   - ✅ Each maximizes correctly
   - ✅ Restore works for each

### Success Criteria

- ✅ Demo compiles without errors
- ✅ Demo runs successfully
- ✅ Windows have central_widget as parent
- ✅ Maximize button works (fills central widget)
- ✅ Restore button works
- ✅ Multiple windows handle maximize correctly

---

## Phase 4: Add Automatic Workspace Detection

**Complexity**: MEDIUM
**Estimated time**: 1.5 hours
**Files affected**: `include/onyxui/widgets/window/window.inl`

### Problem

Phase 1 uses placeholder dimensions (80x25) for parentless windows. Phase 4 makes this dynamic by detecting actual screen/workspace dimensions.

### Strategy

For parentless windows, query the renderer or ui_handle for actual dimensions:

```cpp
// In window::maximize() for parentless windows
auto workspace = calculate_workspace_bounds();

rect_type calculate_workspace_bounds() {
    // Try multiple strategies:

    // Strategy 1: Query from ui_services (if available)
    auto* ui_ctx = ui_services<Backend>::context();
    if (ui_ctx) {
        return ui_ctx->get_workspace_bounds();
    }

    // Strategy 2: Use layer manager viewport
    auto* layers = ui_services<Backend>::layers();
    if (layers && m_layer_id.is_valid()) {
        return layers->get_layer_viewport(m_layer_id);
    }

    // Strategy 3: Fallback to full screen
    // Note: This requires backend-agnostic screen query
    return {0, 0, 80, 25};  // Conservative fallback
}
```

### Required Infrastructure

**Option A**: Add workspace_bounds to ui_context
```cpp
// In ui_context.hh
template<UIBackend Backend>
class ui_context {
public:
    void set_workspace_bounds(const rect_type& bounds);
    rect_type get_workspace_bounds() const;

private:
    rect_type m_workspace_bounds{0, 0, 80, 25};
};
```

**Option B**: Add viewport getter to layer_manager
```cpp
// In layer_manager.hh
rect_type get_viewport() const { return m_last_viewport; }

private:
    rect_type m_last_viewport;  // Saved from render_all_layers()
```

**Recommendation**: Use Option B (layer_manager) - it already has viewport information from rendering.

### Implementation

#### Step 1: Add viewport tracking to layer_manager

```cpp
// In layer_manager.hh
template<UIBackend Backend>
class layer_manager {
public:
    /**
     * @brief Get last rendered viewport bounds
     * @return Viewport rectangle (screen bounds)
     */
    rect_type get_viewport() const noexcept { return m_viewport; }

private:
    rect_type m_viewport{0, 0, 80, 25};  // Last viewport

    // Update in render_all_layers()
    void render_all_layers(
        renderer_type& renderer,
        const rect_type& viewport,
        const theme_type* theme
    ) {
        m_viewport = viewport;  // Save viewport
        // ... rest of rendering ...
    }
};
```

#### Step 2: Update window::maximize()

```cpp
// In window.inl
template<UIBackend Backend>
void window<Backend>::maximize() {
    // ... save state ...

    typename Backend::rect_type maximized_bounds;

    auto* parent_elem = this->parent();
    if (parent_elem) {
        // Case 1: Has parent - use parent bounds
        auto parent_bounds = parent_elem->bounds();
        rect_utils::set_bounds(
            maximized_bounds,
            0, 0,
            rect_utils::get_width(parent_bounds),
            rect_utils::get_height(parent_bounds)
        );
    } else {
        // Case 2: No parent - use viewport from layer manager
        auto* layers = ui_services<Backend>::layers();
        if (layers) {
            auto viewport = layers->get_viewport();
            rect_utils::set_bounds(
                maximized_bounds,
                0, 0,
                rect_utils::get_width(viewport),
                rect_utils::get_height(viewport)
            );
        } else {
            // Fallback: conservative screen size
            rect_utils::set_bounds(maximized_bounds, 0, 0, 80, 25);
        }
    }

    // ... measure, arrange, update layer bounds, update icon ...
}
```

### Implementation Steps

1. Add `m_viewport` member to layer_manager
2. Update `render_all_layers()` to save viewport
3. Add `get_viewport()` public method
4. Update `window::maximize()` to query viewport
5. Remove placeholder 80x25 constants
6. Build and test

### Test Plan

#### Test 1: Viewport Tracking
```cpp
TEST_CASE("layer_manager - tracks viewport from rendering") {
    ui_context_fixture<Backend> ctx;
    auto* layers = ui_services<Backend>::layers();
    REQUIRE(layers != nullptr);

    // Create renderer and theme
    typename Backend::renderer_type renderer(/* ... */);
    auto* theme = ui_services<Backend>::themes()->get_current_theme();

    // Render with specific viewport
    typename Backend::rect_type viewport{0, 0, 100, 50};
    layers->render_all_layers(renderer, viewport, theme);

    // Verify viewport was saved
    auto saved_viewport = layers->get_viewport();
    CHECK(rect_utils::get_width(saved_viewport) == 100);
    CHECK(rect_utils::get_height(saved_viewport) == 50);
}
```

#### Test 2: Dynamic Maximize Dimensions
```cpp
TEST_CASE("window - maximize without parent uses viewport dimensions") {
    ui_context_fixture<Backend> ctx;

    // Set up custom viewport
    auto* layers = ui_services<Backend>::layers();
    typename Backend::renderer_type renderer(/* ... */);
    auto* theme = ui_services<Backend>::themes()->get_current_theme();
    typename Backend::rect_type viewport{0, 0, 100, 50};
    layers->render_all_layers(renderer, viewport, theme);

    // Create window without parent
    typename window<Backend>::window_flags flags;
    flags.has_maximize_button = true;
    auto win = std::make_shared<window<Backend>>("Test", flags);
    win->set_size(20, 10);
    win->show();

    // Maximize
    win->maximize();

    // Should use viewport dimensions (not hardcoded 80x25)
    CHECK(win->bounds().w == 100);
    CHECK(win->bounds().h == 50);
}
```

### Success Criteria

- ✅ layer_manager tracks viewport
- ✅ Parentless windows maximize to actual viewport size
- ✅ No hardcoded screen dimensions
- ✅ Tests pass with various viewport sizes

---

## Integration Testing

### Full Stack Test: main_window + Maximize

```cpp
TEST_CASE("Integration - main_window with maximized window") {
    ui_context_fixture<Backend> ctx;

    // Create main window with menu
    auto main = std::make_unique<main_window<Backend>>();
    auto menu = std::make_unique<menu_bar<Backend>>();
    main->set_menu_bar(std::move(menu));

    // Measure and arrange main window
    main->measure(80, 25);
    main->arrange({0, 0, 80, 25});

    // Create window via main_window
    typename window<Backend>::window_flags flags;
    flags.has_maximize_button = true;
    auto win = main->create_window("Test", flags);
    win->set_size(20, 10);
    win->show();

    // Get central widget bounds
    auto* central = main->central_widget();
    auto central_bounds = central->bounds();

    // Maximize window
    win->maximize();

    // Window should fill central widget (not full screen)
    CHECK(win->bounds().x == 0);
    CHECK(win->bounds().y == 0);
    CHECK(win->bounds().w == rect_utils::get_width(central_bounds));
    CHECK(win->bounds().h == rect_utils::get_height(central_bounds));
}
```

### Visual Integration Test

```cpp
TEST_CASE("Integration - visual test of main_window layout") {
    testing::visual_test_harness<Backend> harness(80, 25);
    ui_context_fixture<Backend> ctx;

    auto main = std::make_unique<main_window<Backend>>();

    // Add menu bar
    auto menu = std::make_unique<menu_bar<Backend>>();
    // ... populate menu ...
    main->set_menu_bar(std::move(menu));

    // Create window
    auto win = main->create_window("Visual Test", flags);
    win->show();
    win->maximize();

    // Render
    harness.render(main.get());

    MESSAGE(harness.dump_canvas());

    // Verify layout:
    // Row 0: Menu bar
    // Rows 1-24: Window (maximized in central widget)
    harness.expect_char_at(0, 0, 'F');  // "File" menu
    harness.expect_char_at(0, 1, 'V');  // "Visual Test" window title
    harness.expect_char_at(0, 24, '+'); // Window bottom border
}
```

---

## Phase 5: Workspace Reference Pattern (COMPLETED)

**Date Completed**: 2025-11-11
**Status**: ✅ IMPLEMENTED AND TESTED

### Overview

After discovering that parenting windows to `central_widget` breaks button functionality, we implemented the **workspace reference pattern**. This allows floating layer windows to respect workspace bounds for maximize while maintaining proper event routing through layer_manager.

### Problem Discovery

When windows were parented to `central_widget`:
- ❌ Window control buttons (close, minimize, maximize) stopped responding
- ❌ Events route through widget tree instead of layer_manager
- ❌ Floating window UX is lost

**Root Cause**: Floating layer windows rely on layer_manager's specialized event routing with absolute coordinates. Parenting removes them from layer management.

### Solution: Workspace Reference Pattern

Instead of parent-child relationship, windows store an **optional workspace reference**:

```cpp
class window {
    ui_element<Backend>* m_workspace = nullptr;  // NOT a parent!

public:
    void set_workspace(ui_element<Backend>* workspace);
    ui_element<Backend>* get_workspace() const;
};
```

**Key Distinction**:
- **Parent** (MDI): Window becomes child in tree, relative coords, tree event routing
- **Workspace** (SDI): Window remains floating layer, absolute coords, layer event routing, but maximizes to workspace bounds

### Implementation

#### 1. window.hh Changes

Added workspace support with comprehensive documentation:

```cpp
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
 *
 * **Use case**: Floating windows in main_window applications
 * @code
 * auto main = std::make_unique<main_window<Backend>>();
 * auto win = std::make_shared<window<Backend>>("Tool", flags);
 * win->set_workspace(main->central_widget());  // Respect workspace
 * win->show();  // Still a floating layer
 * win->maximize();  // Fills central_widget area, not entire screen
 * @endcode
 */
void set_workspace(ui_element<Backend>* workspace) noexcept;
```

#### 2. window.inl maximize() Logic

Updated maximize to check workspace with **full bounds** (including position):

```cpp
void window<Backend>::maximize() {
    // ... state checks ...

    rect_type maximized_bounds;

    // Priority: parent > workspace > viewport
    auto* parent_elem = this->parent();
    if (parent_elem) {
        // Case 1: MDI window - maximize to fill parent
        maximized_bounds = parent_elem->bounds();
    } else if (m_workspace) {
        // Case 2: SDI with workspace - maximize to fill workspace
        // Use full workspace bounds (including position offset from menu bar)
        maximized_bounds = m_workspace->bounds();
    } else {
        // Case 3: Standalone - maximize to fill viewport
        auto viewport = ui_services<Backend>::layers()->get_viewport();
        maximized_bounds = viewport;
    }

    // ... measure, arrange, update layer ...
}
```

**Critical Fix**: Using full `m_workspace->bounds()` instead of just dimensions ensures the window accounts for menu bar offset (e.g., workspace at y=30 due to menu bar).

#### 3. Demo Updates

Updated all window creation functions to accept and use workspace:

**demo_windows.hh**:
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
    }

    win->show();  // Still a floating layer
    return win;
}
```

**demo_menu_builder.hh**:
```cpp
basic_win_item->clicked.connect([widget]() {
    auto win = demo_windows::show_basic_window_with_workspace<Backend>(
        "Demo Window",
        "This is a basic window with title bar controls!",
        widget->central_widget()  // Workspace reference, NOT parent!
    );
});
```

### Test Results

✅ **All 1310 unit tests pass** (8056 assertions)
✅ **Build succeeds** with zero warnings
✅ **Manual testing**: Buttons work correctly
✅ **Manual testing**: Maximize fills workspace (not entire screen)

### Architecture Decision Record

**Decision**: Use workspace reference pattern for SDI applications instead of parent-child relationship.

**Rationale**:
1. Preserves floating window UX (drag anywhere, z-order management)
2. Maintains layer_manager event routing (buttons work correctly)
3. Still respects workspace bounds for maximize
4. Best of both worlds: floating behavior + workspace awareness

**Alternatives Considered**:
1. ❌ Parent to central_widget: Breaks event routing
2. ❌ Always maximize to viewport: Ignores menu bar
3. ✅ Workspace reference: Clean separation of concerns

### Usage Example

```cpp
// Create main window with menu bar
auto main = std::make_unique<main_window<Backend>>();
main->set_menu_bar(build_menu_bar());

// Create floating window with workspace awareness
typename window<Backend>::window_flags flags;
flags.has_maximize_button = true;
auto win = std::make_shared<window<Backend>>("Document 1", flags);

// Set workspace (NOT parent!)
win->set_workspace(main->central_widget());

// Window is still floating layer
win->show();  // Added to layer_manager

// Maximize respects workspace bounds
win->maximize();  // Fills central_widget, not entire screen
```

### Files Modified

- `include/onyxui/widgets/window/window.hh` (lines 331-365, 452)
- `include/onyxui/widgets/window/window.inl` (lines 228-233)
- `examples/demo_windows.hh` (updated all window creation functions)
- `examples/demo_menu_builder.hh` (updated window menu callbacks)

### Success Criteria

- ✅ Windows remain floating layers (layer_manager event routing)
- ✅ Window control buttons work correctly
- ✅ Maximize respects workspace bounds (menu bar excluded)
- ✅ No parent-child relationship (windows not embedded)
- ✅ All tests pass
- ✅ Zero regressions

---

## Documentation Updates

### Files to Update

1. **docs/CLAUDE/ARCHITECTURE.md**
   - Add main_window to widget library section
   - Document window maximize behavior (parent vs parentless)

2. **docusaurus/docs/api-reference/widget-library.md**
   - Add main_window documentation
   - Add usage examples

3. **examples/README.md** (create if doesn't exist)
   - Document demo structure
   - Explain main_window usage

4. **TODO.md**
   - Mark window maximize as COMPLETE
   - Add note about workspace detection

### Example Documentation

#### In widget-library.md:

```markdown
## main_window

Qt-style application window with standard layout.

### Usage

```cpp
auto main = std::make_unique<main_window<Backend>>();

// Add menu bar
main->set_menu_bar(std::make_unique<menu_bar<Backend>>());

// Create windows - they automatically use central widget as parent
auto win = main->create_window("Document 1", flags);
win->show();

// Add status bar
main->set_status_bar(std::make_unique<status_bar<Backend>>());
```

### Layout Structure

```
main_window
  ├─ menu_bar (optional, top, content-sized)
  ├─ central_widget (middle, fills space)
  └─ status_bar (optional, bottom, content-sized)
```

### Window Maximize Behavior

Windows created via `create_window()` have `central_widget` as parent:
- Maximize fills central widget area (excludes menu/status bar)
- Works correctly in both MDI and SDI applications

For standalone windows without main_window:
- Maximize fills entire viewport
- Uses automatic workspace detection
```

---

## Rollout Plan

### Week 1: Core Implementation
- **Day 1**: Phase 1 (Fix maximize for parentless windows)
- **Day 2**: Phase 2 (Create main_window widget)
- **Day 3**: Phase 3 (Update demo)

### Week 2: Polish & Testing
- **Day 4**: Phase 4 (Automatic workspace detection)
- **Day 5**: Integration testing, documentation, bug fixes

### Release Checklist

- [ ] All unit tests pass (1300+ tests)
- [ ] All integration tests pass
- [ ] Demo runs without errors
- [ ] Manual testing complete (maximize, restore, multiple windows)
- [ ] Documentation updated
- [ ] TODO.md updated
- [ ] Git commit with descriptive message

---

## Risk Analysis

### Risk 1: Breaking Existing Tests
**Probability**: Medium
**Impact**: High
**Mitigation**: Run full test suite after each phase

### Risk 2: Layout Issues with main_window
**Probability**: Low
**Impact**: Medium
**Mitigation**: Visual tests in Phase 2

### Risk 3: Parentless Window Edge Cases
**Probability**: Medium
**Impact**: Low
**Mitigation**: Comprehensive test coverage in Phase 1

### Risk 4: Performance Impact
**Probability**: Low
**Impact**: Low
**Mitigation**: Viewport query is O(1), minimal overhead

---

## Future Enhancements

### Post-Implementation Improvements

1. **MDI Container Widget**
   - Explicit MDI support with tiling/cascading
   - Window menu integration
   - Estimated effort: 1 week

2. **Smart Workspace Detection**
   - Detect menu bars dynamically
   - Account for toolbars, sidebars
   - Estimated effort: 2-3 days

3. **Dockable Windows**
   - Windows can dock to edges
   - Splitter support
   - Estimated effort: 2 weeks

4. **Window State Persistence**
   - Save/restore window positions
   - Configuration file support
   - Estimated effort: 1 week

---

## Appendix A: Related Files

### Core Files
- `include/onyxui/widgets/window/window.hh` - Window class declaration
- `include/onyxui/widgets/window/window.inl` - Window implementation
- `include/onyxui/widgets/window/window_title_bar.hh` - Title bar with controls
- `include/onyxui/services/layer_manager.hh` - Layer management

### Test Files
- `unittest/widgets/test_window.cc` - Window unit tests
- `unittest/widgets/test_window_title_bar_icons.cc` - Title bar tests (includes visual test)

### Demo Files
- `examples/demo.hh` - Main demo widget
- `examples/demo_ui_builder.hh` - UI construction
- `examples/demo_windows.hh` - Window creation helpers

---

## Appendix B: API Reference

### window::maximize()

```cpp
/**
 * @brief Maximize window to fill parent or viewport
 *
 * @details
 * Behavior depends on window parentage:
 *
 * **Window with parent:**
 * - Maximizes to fill parent bounds
 * - Use case: MDI containers, main_window central widget
 *
 * **Window without parent:**
 * - Maximizes to fill viewport (entire screen)
 * - Uses layer_manager::get_viewport() for dimensions
 *
 * @post Window state is window_state::maximized
 * @post Normal bounds are saved for restore
 * @post Layer bounds are updated
 * @post Title bar shows restore icon
 *
 * @see restore()
 * @see window_state
 */
void maximize();
```

### main_window::create_window()

```cpp
/**
 * @brief Create window as child of central widget
 *
 * @tparam Args Constructor arguments for window
 * @param args Arguments forwarded to window(title, flags, parent)
 * @return Shared pointer to created window
 *
 * @details
 * Convenience method that:
 * 1. Ensures central widget exists (creates if needed)
 * 2. Creates window with central_widget as parent
 * 3. Returns shared_ptr for lifetime management
 *
 * The window is NOT automatically shown - call show() explicitly.
 *
 * @example
 * @code
 * typename window<Backend>::window_flags flags;
 * flags.has_maximize_button = true;
 *
 * auto win = main->create_window("My Window", flags);
 * win->show();  // Add to layer_manager
 * @endcode
 */
template<typename... Args>
std::shared_ptr<window<Backend>> create_window(Args&&... args);
```

---

**Document Version**: 2.0
**Last Updated**: 2025-11-11
**Status**: ✅ COMPLETED - All Phases Implemented (including Phase 5: Workspace Reference Pattern)
