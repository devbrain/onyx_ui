# Menu System Refactoring: Implementation Plan

**Document Version:** 1.0
**Date:** 2025-10-28
**Author:** Assistant
**Status:** Design Document

## Executive Summary

This document outlines a phased approach to refactor the menu system from its current state to a full-fledged composite menu system supporting:
- Cascading submenus (arbitrary depth)
- Context-dependent keyboard navigation
- Menu bar switching with left/right arrows
- Centralized state management
- RAII-based resource management

The refactoring is designed to be **incremental**, **testable**, and **non-breaking** at each phase.

---

## Current State Analysis

### Issues Identified

1. **Bug: Down arrow loses focus** (Critical)
   - Root cause: Multiple menus register same global `menu_down` semantic action
   - Last menu to construct (Help) overwrites previous registrations (File, Theme)
   - When File menu is open, down arrow calls Help menu's handler → focus lost

2. **Missing features** (Enhancement)
   - No left/right arrow support for menu bar switching
   - No submenu infrastructure
   - No context-dependent navigation

3. **Architecture issues** (Technical Debt)
   - Distributed state: menu_bar tracks open menu, but menus manage own hotkeys
   - No RAII: Manual register/unregister is error-prone
   - Tight coupling risk: Any changes require coordinated updates

### Architecture Overview

```
menu_bar
  ├─ menu (File)      → registers menu_down handler in constructor
  ├─ menu (Theme)     → registers menu_down handler (overwrites File)
  └─ menu (Help)      → registers menu_down handler (overwrites Theme)

Result: Only Help menu's handler is active, regardless of which menu is open
```

---

## Refactoring Phases

Each phase is designed to be:
- ✅ **Self-contained**: Can be merged independently
- ✅ **Testable**: Specific acceptance criteria
- ✅ **Non-breaking**: Backward compatible with existing code
- ✅ **Reviewable**: Small, focused changes

---

## Phase 0: Fix Critical Bug (Down Arrow Focus Loss)

**Goal:** Fix the immediate bug where down arrow causes menu focus to be lost.

**Priority:** CRITICAL - Blocking usability

**Estimated Effort:** 1-2 hours

### Root Cause

Multiple menus register the same global semantic action (`menu_down`), causing the last-registered menu to overwrite previous registrations.

### Minimal Fix (Quick Win)

**Approach:** Don't register semantic actions in menu constructor. Instead, register when menu is opened and unregister when closed.

**Files to Change:**
1. `include/onyxui/widgets/menu.hh`
2. `include/onyxui/widgets/menu_bar.hh`

### Implementation Details

#### 1. Remove Constructor Registration

**File:** `include/onyxui/widgets/menu.hh`

```cpp
// BEFORE (lines ~106-118)
explicit menu(ui_element<Backend>* parent = nullptr)
    : base(parent) {
    this->m_has_border = true;
    this->set_focusable(true);
    this->set_layout_strategy(...);

    initialize_hotkeys();  // ❌ REMOVE THIS
}

// AFTER
explicit menu(ui_element<Backend>* parent = nullptr)
    : base(parent) {
    this->m_has_border = true;
    this->set_focusable(true);
    this->set_layout_strategy(...);

    // NOTE: Semantic actions registered when menu opens (not in constructor)
}
```

#### 2. Make initialize_hotkeys() Public

**File:** `include/onyxui/widgets/menu.hh`

```cpp
// Move from private to public section
public:
    /**
     * @brief Register keyboard navigation hotkeys
     * @details Must be called when menu is opened/shown
     */
    void register_navigation_hotkeys() {
        auto* hotkeys = ui_services<Backend>::hotkeys();
        if (!hotkeys) return;

        hotkeys->register_semantic_action(menu_down,
            [this]() { this->focus_next(); });
        // ... other actions
    }

    /**
     * @brief Unregister keyboard navigation hotkeys
     * @details Must be called when menu is closed/hidden
     */
    void unregister_navigation_hotkeys() {
        auto* hotkeys = ui_services<Backend>::hotkeys();
        if (!hotkeys) return;

        hotkeys->unregister_semantic_action(menu_down);
        hotkeys->unregister_semantic_action(menu_up);
        hotkeys->unregister_semantic_action(menu_select);
        hotkeys->unregister_semantic_action(menu_cancel);
    }
```

#### 3. Call Register/Unregister from menu_bar

**File:** `include/onyxui/widgets/menu_bar.hh`

```cpp
template<UIBackend Backend>
void menu_bar<Backend>::open_menu(std::size_t index) {
    if (index >= m_menus.size()) return;

    // Close current menu if different
    if (m_open_menu_index && *m_open_menu_index != index) {
        close_menu();
    }

    m_open_menu_index = index;
    auto& entry = m_menus[index];

    // Update visual state
    if (entry.title_item) {
        entry.title_item->set_menu_open(true);
    }

    // Show popup layer
    m_current_menu = entry.title_item->show_context_menu_scoped(
        entry.dropdown_menu.get()
    );

    // ✅ NEW: Register navigation hotkeys for THIS menu
    if (entry.dropdown_menu) {
        entry.dropdown_menu->register_navigation_hotkeys();
    }

    // Focus menu
    if (auto* focus = ui_services<Backend>::input()) {
        focus->set_focus(entry.dropdown_menu.get());
        if (entry.dropdown_menu) {
            entry.dropdown_menu->focus_first();
        }
    }
}

template<UIBackend Backend>
void menu_bar<Backend>::close_menu() {
    if (!m_open_menu_index) return;

    // ✅ NEW: Unregister navigation hotkeys
    auto& entry = m_menus[*m_open_menu_index];
    if (entry.dropdown_menu) {
        entry.dropdown_menu->unregister_navigation_hotkeys();
    }

    // Update visual state
    if (entry.title_item) {
        entry.title_item->set_menu_open(false);
    }

    // Cleanup
    m_current_menu.reset();
    if (auto* focus = ui_services<Backend>::input()) {
        focus->clear_focus();
    }

    m_open_menu_index = std::nullopt;
}
```

### Acceptance Criteria (Testable)

**Manual Test:**
1. Run `./build/bin/widgets_demo`
2. Press F10 to open File menu
3. Verify "New" is highlighted (yellow text, blue background)
4. Press Down arrow
5. ✅ **PASS**: "Open" becomes highlighted, "New" returns to normal
6. ❌ **FAIL**: Highlighting disappears

**Unit Test:** (Add to `unittest/widgets/test_menus.cc`)

```cpp
TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "Menu navigation - down arrow") {
    auto menu_bar_widget = std::make_unique<menu_bar<test_backend>>();

    // Add File menu with 2 items
    auto file_menu = std::make_unique<menu<test_backend>>();
    auto item1 = std::make_unique<menu_item<test_backend>>("New");
    auto item2 = std::make_unique<menu_item<test_backend>>("Open");
    file_menu->add_item(std::move(item1));
    file_menu->add_item(std::move(item2));
    menu_bar_widget->add_menu("File", std::move(file_menu));

    // Open File menu
    menu_bar_widget->open_menu(0);

    auto* focus_mgr = ui_services<test_backend>::input();
    REQUIRE(focus_mgr != nullptr);

    // First item should be focused
    auto* focused = focus_mgr->get_focused();
    CHECK(focused != nullptr);

    // Simulate down arrow via hotkey manager
    auto* hotkeys = ui_services<test_backend>::hotkeys();
    REQUIRE(hotkeys != nullptr);

    // Create down arrow event
    test_backend::event_type down_event;
    // ... configure down arrow key

    bool handled = hotkeys->handle_key_event(down_event, focused);
    CHECK(handled);

    // Focus should move to second item
    auto* new_focused = focus_mgr->get_focused();
    CHECK(new_focused != focused);
}
```

### Rollback Plan

If issues arise:
1. Revert changes to menu.hh and menu_bar.hh
2. Original bug returns, but no new issues introduced
3. Alternative: Keep constructor registration as fallback

### Success Metrics

- ✅ Down arrow navigation works in File menu
- ✅ Down arrow navigation works in Theme menu
- ✅ Down arrow navigation works in Help menu
- ✅ All existing tests pass
- ✅ No new crashes or hangs

---

## Phase 1: Introduce RAII for Hotkey Registration

**Goal:** Add RAII-based semantic action registration to prevent resource leaks and ensure cleanup.

**Priority:** HIGH - Prevents future bugs

**Estimated Effort:** 2-3 hours

**Depends On:** Phase 0 (bug fix must be merged first)

### Motivation

Manual register/unregister is error-prone:
- ❌ Easy to forget to unregister
- ❌ Not exception-safe
- ❌ No compile-time guarantees
- ❌ Distributed responsibility

RAII provides:
- ✅ Automatic cleanup (destructor)
- ✅ Exception-safe (RAII guarantees)
- ✅ Move semantics (transfer ownership)
- ✅ Single responsibility (guard owns lifecycle)

### Implementation Details

#### 1. Create RAII Guard Class

**File:** `include/onyxui/hotkeys/semantic_action_guard.hh` (NEW)

```cpp
#pragma once

#include <onyxui/concepts/backend.hh>
#include <onyxui/hotkeys/hotkey_action.hh>
#include <functional>

namespace onyxui {

    /**
     * @brief RAII guard for semantic action registration
     *
     * @details
     * Automatically unregisters a semantic action when destroyed.
     * Provides move semantics for transfer of ownership.
     *
     * Similar to std::unique_ptr for semantic action lifecycle.
     *
     * @example
     * @code
     * semantic_action_guard<Backend> guard(
     *     hotkeys,
     *     hotkey_action::menu_down,
     *     []() { handle_menu_down(); }
     * );
     * // Automatic unregister when guard goes out of scope
     * @endcode
     */
    template<UIBackend Backend>
    class semantic_action_guard {
    public:
        using hotkey_manager_type = hotkey_manager<Backend>;

        /**
         * @brief Construct and register semantic action
         */
        semantic_action_guard(
            hotkey_manager_type* manager,
            hotkey_action action,
            std::function<void()> handler
        ) : m_manager(manager), m_action(action) {
            if (m_manager) {
                m_manager->register_semantic_action(m_action, std::move(handler));
            }
        }

        /**
         * @brief Destructor - automatically unregisters
         */
        ~semantic_action_guard() {
            if (m_manager) {
                m_manager->unregister_semantic_action(m_action);
            }
        }

        // Move-only (like std::unique_ptr)
        semantic_action_guard(semantic_action_guard&& other) noexcept
            : m_manager(other.m_manager)
            , m_action(other.m_action) {
            other.m_manager = nullptr;  // Transfer ownership
        }

        semantic_action_guard& operator=(semantic_action_guard&& other) noexcept {
            if (this != &other) {
                // Unregister current
                if (m_manager) {
                    m_manager->unregister_semantic_action(m_action);
                }
                // Transfer ownership
                m_manager = other.m_manager;
                m_action = other.m_action;
                other.m_manager = nullptr;
            }
            return *this;
        }

        // Non-copyable
        semantic_action_guard(const semantic_action_guard&) = delete;
        semantic_action_guard& operator=(const semantic_action_guard&) = delete;

        /**
         * @brief Check if guard is valid (owns a registration)
         */
        [[nodiscard]] bool is_valid() const noexcept {
            return m_manager != nullptr;
        }

        /**
         * @brief Explicitly release ownership without unregistering
         */
        void release() noexcept {
            m_manager = nullptr;
        }

    private:
        hotkey_manager_type* m_manager = nullptr;
        hotkey_action m_action;
    };

} // namespace onyxui
```

#### 2. Update menu_bar to Use RAII Guards

**File:** `include/onyxui/widgets/menu_bar.hh`

```cpp
#include <onyxui/hotkeys/semantic_action_guard.hh>

template<UIBackend Backend>
class menu_bar : public widget_container<Backend> {
private:
    // ✅ NEW: RAII guards for currently open menu
    std::vector<semantic_action_guard<Backend>> m_menu_nav_guards;

    void open_menu(std::size_t index) {
        if (index >= m_menus.size()) return;

        // Close previous (guards auto-cleanup)
        if (m_open_menu_index && *m_open_menu_index != index) {
            close_menu();
        }

        m_open_menu_index = index;
        auto& entry = m_menus[index];

        // Update visual state
        if (entry.title_item) {
            entry.title_item->set_menu_open(true);
        }

        // Show popup
        m_current_menu = entry.title_item->show_context_menu_scoped(
            entry.dropdown_menu.get()
        );

        // ✅ NEW: Register with RAII guards (automatic cleanup)
        auto* hotkeys = ui_services<Backend>::hotkeys();
        if (hotkeys && entry.dropdown_menu) {
            auto* menu = entry.dropdown_menu.get();

            m_menu_nav_guards.clear();  // Clear previous guards (auto-unregister)
            m_menu_nav_guards.emplace_back(hotkeys, hotkey_action::menu_down,
                [menu]() { menu->focus_next(); });
            m_menu_nav_guards.emplace_back(hotkeys, hotkey_action::menu_up,
                [menu]() { menu->focus_previous(); });
            m_menu_nav_guards.emplace_back(hotkeys, hotkey_action::menu_select,
                [menu]() { menu->activate_focused(); });
            m_menu_nav_guards.emplace_back(hotkeys, hotkey_action::menu_cancel,
                [menu]() { menu->closing.emit(); });
        }

        // Focus menu
        if (auto* focus = ui_services<Backend>::input()) {
            focus->set_focus(entry.dropdown_menu.get());
            if (entry.dropdown_menu) {
                entry.dropdown_menu->focus_first();
            }
        }
    }

    void close_menu() {
        if (!m_open_menu_index) return;

        // ✅ NEW: RAII guards auto-unregister when cleared
        m_menu_nav_guards.clear();

        auto& entry = m_menus[*m_open_menu_index];
        if (entry.title_item) {
            entry.title_item->set_menu_open(false);
        }

        m_current_menu.reset();
        if (auto* focus = ui_services<Backend>::input()) {
            focus->clear_focus();
        }

        m_open_menu_index = std::nullopt;
    }
};
```

#### 3. Remove Manual Register/Unregister from menu

**File:** `include/onyxui/widgets/menu.hh`

```cpp
// ✅ DELETE: register_navigation_hotkeys() - no longer needed
// ✅ DELETE: unregister_navigation_hotkeys() - no longer needed
// menu_bar now handles registration via RAII guards
```

### Acceptance Criteria (Testable)

**Unit Test 1: RAII Guard Lifecycle**

```cpp
TEST_CASE("semantic_action_guard - RAII lifecycle") {
    auto hotkeys = std::make_unique<hotkey_manager<test_backend>>();

    bool handler_called = false;

    {
        semantic_action_guard<test_backend> guard(
            hotkeys.get(),
            hotkey_action::menu_down,
            [&]() { handler_called = true; }
        );

        // Handler should be registered
        // ... trigger menu_down ...
        CHECK(handler_called);
    }

    // Guard destroyed - handler should be unregistered
    handler_called = false;
    // ... trigger menu_down ...
    CHECK_FALSE(handler_called);  // Should not call
}
```

**Unit Test 2: Exception Safety**

```cpp
TEST_CASE("semantic_action_guard - exception safety") {
    auto hotkeys = std::make_unique<hotkey_manager<test_backend>>();

    try {
        semantic_action_guard<test_backend> guard(
            hotkeys.get(),
            hotkey_action::menu_down,
            []() {}
        );

        throw std::runtime_error("Simulated error");
    } catch (...) {
        // Guard should auto-unregister despite exception
    }

    // Verify handler is unregistered
    // ... check registration state ...
}
```

**Integration Test: Menu Switching**

```cpp
TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "RAII guards - menu switching") {
    auto menu_bar_widget = std::make_unique<menu_bar<test_backend>>();
    // ... add File and Theme menus ...

    menu_bar_widget->open_menu(0);  // Open File
    // ... verify File menu navigation works ...

    menu_bar_widget->open_menu(1);  // Switch to Theme
    // ... verify Theme menu navigation works ...
    // ... verify File menu navigation no longer works ...

    menu_bar_widget->close_menu();  // Close
    // ... verify no menu navigation works ...
}
```

### Rollback Plan

If RAII guards cause issues:
1. Keep Phase 0 manual register/unregister in menu_bar
2. Remove semantic_action_guard.hh
3. Revert menu_bar to call menu->register_navigation_hotkeys()

### Success Metrics

- ✅ All Phase 0 tests still pass
- ✅ RAII guard unit tests pass
- ✅ No memory leaks (valgrind clean)
- ✅ Exception-safe (no resource leaks on throw)
- ✅ Code simpler (no manual unregister calls)

---

## Phase 2: Add Left/Right Navigation (Menu Bar Switching)

**Goal:** Implement left/right arrow keys to switch between menu bar items.

**Priority:** MEDIUM - Usability enhancement

**Estimated Effort:** 2-3 hours

**Depends On:** Phase 1 (RAII guards)

### User Experience

**Current behavior:**
- User opens File menu, presses right arrow → nothing happens
- User must ESC to close, then click Theme menu

**New behavior:**
- User opens File menu, presses right arrow → Theme menu opens
- User presses left arrow → File menu opens (wraps around)

### Implementation Details

#### 1. Add Navigation Methods to menu_bar

**File:** `include/onyxui/widgets/menu_bar.hh`

```cpp
class menu_bar {
private:
    /**
     * @brief Open previous menu in menu bar (left arrow)
     */
    void open_previous_menu() {
        if (!m_open_menu_index || m_menus.empty()) return;

        size_t prev = (*m_open_menu_index == 0)
            ? m_menus.size() - 1
            : *m_open_menu_index - 1;

        open_menu(prev);
    }

    /**
     * @brief Open next menu in menu bar (right arrow)
     */
    void open_next_menu() {
        if (!m_open_menu_index || m_menus.empty()) return;

        size_t next = (*m_open_menu_index + 1) % m_menus.size();
        open_menu(next);
    }
};
```

#### 2. Register Left/Right Handlers

**File:** `include/onyxui/widgets/menu_bar.hh`

```cpp
void open_menu(std::size_t index) {
    // ... existing code ...

    // Register navigation with RAII guards
    auto* hotkeys = ui_services<Backend>::hotkeys();
    if (hotkeys && entry.dropdown_menu) {
        auto* menu = entry.dropdown_menu.get();

        m_menu_nav_guards.clear();

        // Up/Down - navigate within menu
        m_menu_nav_guards.emplace_back(hotkeys, hotkey_action::menu_down,
            [menu]() { menu->focus_next(); });
        m_menu_nav_guards.emplace_back(hotkeys, hotkey_action::menu_up,
            [menu]() { menu->focus_previous(); });

        // ✅ NEW: Left/Right - switch menu bar items
        m_menu_nav_guards.emplace_back(hotkeys, hotkey_action::menu_left,
            [this]() { this->open_previous_menu(); });
        m_menu_nav_guards.emplace_back(hotkeys, hotkey_action::menu_right,
            [this]() { this->open_next_menu(); });

        // Enter/Escape
        m_menu_nav_guards.emplace_back(hotkeys, hotkey_action::menu_select,
            [menu]() { menu->activate_focused(); });
        m_menu_nav_guards.emplace_back(hotkeys, hotkey_action::menu_cancel,
            [this]() { this->close_menu(); });
    }

    // ... existing code ...
}
```

### Acceptance Criteria (Testable)

**Manual Test:**
1. Run `./build/bin/widgets_demo`
2. Press F10 to open File menu
3. Press Right arrow
4. ✅ **PASS**: Theme menu opens, File menu closes
5. Press Right arrow again
6. ✅ **PASS**: Help menu opens
7. Press Left arrow
8. ✅ **PASS**: Theme menu opens
9. Press Left arrow twice
10. ✅ **PASS**: Help menu opens (wraps around)

**Unit Test:**

```cpp
TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "Menu bar - left/right navigation") {
    auto menu_bar_widget = std::make_unique<menu_bar<test_backend>>();

    // Add 3 menus: File, Edit, View
    auto file_menu = std::make_unique<menu<test_backend>>();
    file_menu->add_item(std::make_unique<menu_item<test_backend>>("New"));
    menu_bar_widget->add_menu("File", std::move(file_menu));

    auto edit_menu = std::make_unique<menu<test_backend>>();
    edit_menu->add_item(std::make_unique<menu_item<test_backend>>("Cut"));
    menu_bar_widget->add_menu("Edit", std::move(edit_menu));

    auto view_menu = std::make_unique<menu<test_backend>>();
    view_menu->add_item(std::make_unique<menu_item<test_backend>>("Zoom"));
    menu_bar_widget->add_menu("View", std::move(view_menu));

    // Open File menu (index 0)
    menu_bar_widget->open_menu(0);
    CHECK(menu_bar_widget->get_open_menu_index() == 0);

    // Simulate right arrow
    // ... trigger menu_right semantic action ...
    CHECK(menu_bar_widget->get_open_menu_index() == 1);  // Edit menu

    // Right arrow again
    CHECK(menu_bar_widget->get_open_menu_index() == 2);  // View menu

    // Right arrow again (wrap around)
    CHECK(menu_bar_widget->get_open_menu_index() == 0);  // File menu

    // Simulate left arrow
    CHECK(menu_bar_widget->get_open_menu_index() == 2);  // View menu (wrap)
}
```

### Edge Cases

1. **Single menu:** Left/right should do nothing (or wrap to same menu)
2. **Empty menu bar:** Should not crash
3. **Menu opens submenu:** Right arrow behavior changes (Phase 5)

### Rollback Plan

If left/right navigation causes issues:
1. Remove menu_left/menu_right registrations from open_menu()
2. Remove open_previous_menu() and open_next_menu() methods
3. Phase 1 (RAII guards) still works

### Success Metrics

- ✅ Right arrow switches to next menu bar item
- ✅ Left arrow switches to previous menu bar item
- ✅ Navigation wraps around (Help → File, File → Help)
- ✅ Focus moves to first item in new menu
- ✅ Previous menu closes cleanly
- ✅ All existing tests pass

---

## Phase 3: Design Submenu Infrastructure

**Goal:** Design and implement core data structures for cascading submenus.

**Priority:** LOW - Future enhancement (no immediate need)

**Estimated Effort:** 4-6 hours

**Depends On:** Phase 2 (left/right navigation)

### Requirements Analysis

**Submenu Features:**
1. Menu items can have child submenus (arbitrary depth)
2. Right arrow on submenu item opens submenu
3. Left arrow in submenu returns to parent
4. Hover over submenu item opens submenu (optional)
5. Clicking submenu item opens submenu
6. Submenus position to the right of parent (or left if no space)

### Design Decisions

#### Option A: menu_item Owns Submenu

```cpp
template<UIBackend Backend>
class menu_item {
private:
    std::unique_ptr<menu<Backend>> m_submenu;  // nullptr = no submenu

public:
    void set_submenu(std::unique_ptr<menu<Backend>> submenu) {
        m_submenu = std::move(submenu);
    }

    [[nodiscard]] bool has_submenu() const noexcept {
        return m_submenu != nullptr;
    }

    [[nodiscard]] menu<Backend>* submenu() const noexcept {
        return m_submenu.get();
    }
};
```

**Pros:**
- ✅ Clear ownership (item owns submenu)
- ✅ Easy to implement
- ✅ Matches typical UI toolkit patterns

**Cons:**
- ❌ Tight coupling (menu_item depends on menu)
- ❌ Circular dependency risk

#### Option B: menu Manages All Submenus

```cpp
template<UIBackend Backend>
class menu {
private:
    std::unordered_map<menu_item<Backend>*, std::unique_ptr<menu<Backend>>> m_submenus;

public:
    void set_item_submenu(menu_item<Backend>* item, std::unique_ptr<menu<Backend>> submenu) {
        m_submenus[item] = std::move(submenu);
    }

    [[nodiscard]] bool has_submenu(menu_item<Backend>* item) const {
        return m_submenus.count(item) > 0;
    }
};
```

**Pros:**
- ✅ No circular dependency
- ✅ Centralized submenu management

**Cons:**
- ❌ Weak coupling (item doesn't know about submenu)
- ❌ More complex API

**Recommendation:** **Option A** (menu_item owns submenu)
- Simpler API for users
- Matches mental model (item "has" submenu)
- Forward declaration can break circular dependency

### Implementation Details

#### 1. Add Submenu to menu_item

**File:** `include/onyxui/widgets/menu_item.hh`

```cpp
// Forward declaration to avoid circular dependency
template<UIBackend Backend>
class menu;

template<UIBackend Backend>
class menu_item : public stateful_widget<Backend> {
private:
    std::unique_ptr<menu<Backend>> m_submenu;  // nullptr = no submenu
    bool m_has_submenu_indicator = false;       // Show ► indicator

public:
    /**
     * @brief Set submenu for this item
     * @param submenu Submenu to attach (nullptr to remove)
     * @details Item takes ownership of submenu
     */
    void set_submenu(std::unique_ptr<menu<Backend>> submenu) {
        m_submenu = std::move(submenu);
        m_has_submenu_indicator = (m_submenu != nullptr);
        this->invalidate_measure();  // Need to redraw with ► indicator
    }

    /**
     * @brief Check if item has submenu
     */
    [[nodiscard]] bool has_submenu() const noexcept {
        return m_submenu != nullptr;
    }

    /**
     * @brief Get submenu pointer (nullptr if none)
     */
    [[nodiscard]] menu<Backend>* submenu() const noexcept {
        return m_submenu.get();
    }

    /**
     * @brief Release submenu ownership
     * @return Submenu unique_ptr
     */
    [[nodiscard]] std::unique_ptr<menu<Backend>> release_submenu() noexcept {
        m_has_submenu_indicator = false;
        this->invalidate_measure();
        return std::move(m_submenu);
    }
};
```

#### 2. Update menu_item Rendering

**File:** `include/onyxui/widgets/menu_item.hh`

```cpp
void do_render(render_context<Backend>& ctx) const override {
    // ... existing rendering code ...

    // ✅ NEW: Draw submenu indicator (►)
    if (m_has_submenu_indicator) {
        // Draw ► on right side
        auto indicator_pos = /* right-aligned position */;
        ctx.draw_text("►", indicator_pos);
        // Or use icon: ctx.draw_icon(icon_type::submenu_arrow, pos);
    }
}
```

#### 3. Add Submenu Helper to menu

**File:** `include/onyxui/widgets/menu.hh`

```cpp
/**
 * @brief Add menu item with submenu
 * @param text Item text
 * @param submenu Submenu to attach
 * @return Pointer to added item
 */
menu_item<Backend>* add_item_with_submenu(
    std::string_view text,
    std::unique_ptr<menu<Backend>> submenu
) {
    auto item = std::make_unique<menu_item<Backend>>(text);
    item->set_submenu(std::move(submenu));
    return add_item(std::move(item));
}
```

### Acceptance Criteria (Testable)

**Unit Test 1: Submenu Ownership**

```cpp
TEST_CASE("menu_item - submenu ownership") {
    auto item = std::make_unique<menu_item<test_backend>>("File");
    auto submenu = std::make_unique<menu<test_backend>>();

    CHECK_FALSE(item->has_submenu());

    item->set_submenu(std::move(submenu));
    CHECK(item->has_submenu());
    CHECK(item->submenu() != nullptr);

    auto released = item->release_submenu();
    CHECK_FALSE(item->has_submenu());
    CHECK(released != nullptr);
}
```

**Unit Test 2: Submenu Indicator Rendering**

```cpp
TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "menu_item - submenu indicator") {
    auto item = std::make_unique<menu_item<test_backend>>("File");
    auto submenu = std::make_unique<menu<test_backend>>();
    item->set_submenu(std::move(submenu));

    // Measure and arrange
    auto size = item->measure(100, 50);
    rect_type bounds;
    rect_utils::set_bounds(bounds, 0, 0, 100, 20);
    item->arrange(bounds);

    // Render to test canvas
    auto canvas = render_to_canvas(*item, 100, 20);

    // Verify ► indicator is rendered
    // ... check canvas for "►" or arrow icon ...
}
```

### Migration Path

1. **Backward Compatible:** Existing menus without submenus work unchanged
2. **Opt-In:** Only use submenus when explicitly added
3. **No Breaking Changes:** All existing APIs remain

### Rollback Plan

If submenu infrastructure causes issues:
1. Remove m_submenu from menu_item
2. Remove submenu-related methods
3. Keep Phase 2 (left/right navigation)

### Success Metrics

- ✅ menu_item can own submenu
- ✅ Submenu indicator (►) renders correctly
- ✅ Existing menus without submenus work unchanged
- ✅ No circular dependency issues
- ✅ All existing tests pass

---

## Phase 4: Implement Composite Menu System

**Goal:** Create centralized menu_system coordinator for state management.

**Priority:** LOW - Foundation for Phase 5

**Estimated Effort:** 6-8 hours

**Depends On:** Phase 3 (submenu infrastructure)

### Architecture Overview

**Current (Phase 3):**
```
menu_bar
  ├─ menu (File)
  ├─ menu (Theme)
  └─ menu (Help)
```

**New (Phase 4):**
```
menu_bar
  └─ menu_system
       ├─ menu_stack: [File Menu]           (when File is open)
       ├─ menu_stack: [File Menu, Open Submenu]  (when submenu open)
       └─ Centralized hotkey handlers
```

### Design: menu_system Class

**File:** `include/onyxui/widgets/menu_system.hh` (NEW)

```cpp
#pragma once

#include <onyxui/concepts/backend.hh>
#include <onyxui/hotkeys/semantic_action_guard.hh>
#include <vector>
#include <optional>

namespace onyxui {

    // Forward declarations
    template<UIBackend Backend> class menu;
    template<UIBackend Backend> class menu_bar;

    /**
     * @brief Centralized coordinator for menu navigation and state
     *
     * @details
     * Manages:
     * - Menu hierarchy stack (for submenus)
     * - Context-dependent keyboard navigation
     * - Semantic action registration (RAII-based)
     *
     * Coordinates between menu_bar and individual menus.
     *
     * @tparam Backend UI backend type
     */
    template<UIBackend Backend>
    class menu_system {
    public:
        using menu_type = menu<Backend>;
        using menu_bar_type = menu_bar<Backend>;

        /**
         * @brief Construct menu system
         * @param menu_bar Parent menu bar (non-owning pointer)
         */
        explicit menu_system(menu_bar_type* menu_bar);

        /**
         * @brief Open top-level menu
         * @param menu Menu to open (non-owning pointer)
         */
        void open_top_level_menu(menu_type* menu);

        /**
         * @brief Open submenu (push onto stack)
         * @param submenu Submenu to open
         */
        void open_submenu(menu_type* submenu);

        /**
         * @brief Close current submenu (pop from stack)
         * @return True if submenu was closed, false if at top level
         */
        bool close_current_submenu();

        /**
         * @brief Close all menus (clear stack)
         */
        void close_all_menus();

        /**
         * @brief Get currently active menu (top of stack)
         */
        [[nodiscard]] menu_type* current_menu() const noexcept;

        /**
         * @brief Get menu hierarchy depth (1 = top-level, 2+ = submenus)
         */
        [[nodiscard]] std::size_t menu_depth() const noexcept {
            return m_menu_stack.size();
        }

        /**
         * @brief Check if at top-level menu
         */
        [[nodiscard]] bool is_top_level() const noexcept {
            return m_menu_stack.size() == 1;
        }

    private:
        /**
         * @brief Register context-dependent navigation hotkeys
         */
        void register_navigation_hotkeys();

        /**
         * @brief Handle menu_down action
         */
        void handle_menu_down();

        /**
         * @brief Handle menu_up action
         */
        void handle_menu_up();

        /**
         * @brief Handle menu_left action (context-dependent)
         */
        void handle_menu_left();

        /**
         * @brief Handle menu_right action (context-dependent)
         */
        void handle_menu_right();

        /**
         * @brief Handle menu_select action
         */
        void handle_menu_select();

        /**
         * @brief Handle menu_cancel action
         */
        void handle_menu_cancel();

        menu_bar_type* m_menu_bar = nullptr;       // Non-owning (parent)
        std::vector<menu_type*> m_menu_stack;      // Menu hierarchy (top = current)
        std::vector<semantic_action_guard<Backend>> m_nav_guards;  // RAII hotkey guards
    };

} // namespace onyxui
```

### Implementation: menu_system.hh (Inline Definitions)

```cpp
template<UIBackend Backend>
menu_system<Backend>::menu_system(menu_bar_type* menu_bar)
    : m_menu_bar(menu_bar) {
    // Register hotkeys ONCE - handlers check state dynamically
    register_navigation_hotkeys();
}

template<UIBackend Backend>
void menu_system<Backend>::register_navigation_hotkeys() {
    auto* hotkeys = ui_services<Backend>::hotkeys();
    if (!hotkeys) return;

    m_nav_guards.clear();

    // Register context-dependent handlers
    m_nav_guards.emplace_back(hotkeys, hotkey_action::menu_down,
        [this]() { this->handle_menu_down(); });
    m_nav_guards.emplace_back(hotkeys, hotkey_action::menu_up,
        [this]() { this->handle_menu_up(); });
    m_nav_guards.emplace_back(hotkeys, hotkey_action::menu_left,
        [this]() { this->handle_menu_left(); });
    m_nav_guards.emplace_back(hotkeys, hotkey_action::menu_right,
        [this]() { this->handle_menu_right(); });
    m_nav_guards.emplace_back(hotkeys, hotkey_action::menu_select,
        [this]() { this->handle_menu_select(); });
    m_nav_guards.emplace_back(hotkeys, hotkey_action::menu_cancel,
        [this]() { this->handle_menu_cancel(); });
}

template<UIBackend Backend>
void menu_system<Backend>::open_top_level_menu(menu_type* menu) {
    m_menu_stack = {menu};  // Reset to single menu
}

template<UIBackend Backend>
void menu_system<Backend>::open_submenu(menu_type* submenu) {
    m_menu_stack.push_back(submenu);
}

template<UIBackend Backend>
bool menu_system<Backend>::close_current_submenu() {
    if (m_menu_stack.size() <= 1) {
        return false;  // At top-level, can't close
    }
    m_menu_stack.pop_back();
    return true;
}

template<UIBackend Backend>
void menu_system<Backend>::close_all_menus() {
    m_menu_stack.clear();
}

template<UIBackend Backend>
auto menu_system<Backend>::current_menu() const noexcept -> menu_type* {
    return m_menu_stack.empty() ? nullptr : m_menu_stack.back();
}

// Handler implementations
template<UIBackend Backend>
void menu_system<Backend>::handle_menu_down() {
    if (auto* menu = current_menu()) {
        menu->focus_next();
    }
}

template<UIBackend Backend>
void menu_system<Backend>::handle_menu_up() {
    if (auto* menu = current_menu()) {
        menu->focus_previous();
    }
}

template<UIBackend Backend>
void menu_system<Backend>::handle_menu_left() {
    // Context-dependent:
    // - In submenu: Close submenu, return to parent
    // - In top-level: Switch to previous menu bar item
    if (is_top_level()) {
        // Top-level menu - switch menu bar
        if (m_menu_bar) {
            m_menu_bar->open_previous_menu();
        }
    } else {
        // Submenu - close and return to parent
        close_current_submenu();
    }
}

template<UIBackend Backend>
void menu_system<Backend>::handle_menu_right() {
    // Context-dependent:
    // - On item with submenu: Open submenu
    // - On regular item in top-level: Switch to next menu bar item
    auto* menu = current_menu();
    if (!menu) return;

    auto* focused_item = menu->focused_item();

    if (focused_item && focused_item->has_submenu()) {
        // Open submenu
        open_submenu(focused_item->submenu());

        // Focus first item in submenu
        if (auto* submenu = focused_item->submenu()) {
            submenu->focus_first();
        }
    } else if (is_top_level()) {
        // Top-level menu on regular item - switch menu bar
        if (m_menu_bar) {
            m_menu_bar->open_next_menu();
        }
    }
    // Else: In submenu on regular item - do nothing
}

template<UIBackend Backend>
void menu_system<Backend>::handle_menu_select() {
    if (auto* menu = current_menu()) {
        menu->activate_focused();
    }
}

template<UIBackend Backend>
void menu_system<Backend>::handle_menu_cancel() {
    // Close current level
    if (!close_current_submenu()) {
        // At top-level - close entire menu bar
        close_all_menus();
        if (m_menu_bar) {
            m_menu_bar->close_menu();
        }
    }
}
```

### Integration: Update menu_bar

**File:** `include/onyxui/widgets/menu_bar.hh`

```cpp
#include <onyxui/widgets/menu_system.hh>

template<UIBackend Backend>
class menu_bar : public widget_container<Backend> {
private:
    menu_system<Backend> m_menu_system;  // ✅ NEW: Centralized coordinator

    // ✅ DELETE: m_menu_nav_guards (moved to menu_system)

public:
    menu_bar() : m_menu_system(this) {}  // ✅ NEW: Initialize with this

    void open_menu(std::size_t index) {
        if (index >= m_menus.size()) return;

        // Close previous
        if (m_open_menu_index && *m_open_menu_index != index) {
            close_menu();
        }

        m_open_menu_index = index;
        auto& entry = m_menus[index];

        // Update visual state
        if (entry.title_item) {
            entry.title_item->set_menu_open(true);
        }

        // Show popup
        m_current_menu = entry.title_item->show_context_menu_scoped(
            entry.dropdown_menu.get()
        );

        // ✅ NEW: Notify menu_system
        m_menu_system.open_top_level_menu(entry.dropdown_menu.get());

        // Focus menu
        if (auto* focus = ui_services<Backend>::input()) {
            focus->set_focus(entry.dropdown_menu.get());
            if (entry.dropdown_menu) {
                entry.dropdown_menu->focus_first();
            }
        }
    }

    void close_menu() {
        if (!m_open_menu_index) return;

        // ✅ NEW: Notify menu_system
        m_menu_system.close_all_menus();

        auto& entry = m_menus[*m_open_menu_index];
        if (entry.title_item) {
            entry.title_item->set_menu_open(false);
        }

        m_current_menu.reset();
        if (auto* focus = ui_services<Backend>::input()) {
            focus->clear_focus();
        }

        m_open_menu_index = std::nullopt;
    }

    // ✅ NEW: Called by menu_system for left/right navigation
    void open_previous_menu() {
        if (!m_open_menu_index || m_menus.empty()) return;
        size_t prev = (*m_open_menu_index == 0)
            ? m_menus.size() - 1
            : *m_open_menu_index - 1;
        open_menu(prev);
    }

    void open_next_menu() {
        if (!m_open_menu_index || m_menus.empty()) return;
        size_t next = (*m_open_menu_index + 1) % m_menus.size();
        open_menu(next);
    }
};
```

### Acceptance Criteria (Testable)

**Unit Test 1: menu_system Lifecycle**

```cpp
TEST_CASE("menu_system - basic lifecycle") {
    auto menu_bar_widget = std::make_unique<menu_bar<test_backend>>();
    menu_system<test_backend> sys(menu_bar_widget.get());

    CHECK(sys.current_menu() == nullptr);
    CHECK(sys.menu_depth() == 0);

    auto menu = std::make_unique<menu<test_backend>>();
    auto* menu_ptr = menu.get();

    sys.open_top_level_menu(menu_ptr);
    CHECK(sys.current_menu() == menu_ptr);
    CHECK(sys.menu_depth() == 1);
    CHECK(sys.is_top_level());

    sys.close_all_menus();
    CHECK(sys.current_menu() == nullptr);
    CHECK(sys.menu_depth() == 0);
}
```

**Unit Test 2: Submenu Stack**

```cpp
TEST_CASE("menu_system - submenu stack") {
    auto menu_bar_widget = std::make_unique<menu_bar<test_backend>>();
    menu_system<test_backend> sys(menu_bar_widget.get());

    auto top_menu = std::make_unique<menu<test_backend>>();
    auto submenu = std::make_unique<menu<test_backend>>();
    auto* top_ptr = top_menu.get();
    auto* sub_ptr = submenu.get();

    sys.open_top_level_menu(top_ptr);
    CHECK(sys.menu_depth() == 1);
    CHECK(sys.is_top_level());

    sys.open_submenu(sub_ptr);
    CHECK(sys.menu_depth() == 2);
    CHECK_FALSE(sys.is_top_level());
    CHECK(sys.current_menu() == sub_ptr);

    bool closed = sys.close_current_submenu();
    CHECK(closed);
    CHECK(sys.menu_depth() == 1);
    CHECK(sys.current_menu() == top_ptr);

    closed = sys.close_current_submenu();
    CHECK_FALSE(closed);  // Can't close top-level
    CHECK(sys.menu_depth() == 1);
}
```

**Integration Test: Full Navigation Flow**

```cpp
TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "menu_system - navigation flow") {
    auto menu_bar_widget = std::make_unique<menu_bar<test_backend>>();

    // Build menu structure:
    // File: [New, Open ▶, Save]
    //   Open ▶: [Project, File, Folder]
    auto file_menu = std::make_unique<menu<test_backend>>();
    file_menu->add_item(std::make_unique<menu_item<test_backend>>("New"));

    auto open_submenu = std::make_unique<menu<test_backend>>();
    open_submenu->add_item(std::make_unique<menu_item<test_backend>>("Project"));
    open_submenu->add_item(std::make_unique<menu_item<test_backend>>("File"));

    auto open_item = std::make_unique<menu_item<test_backend>>("Open");
    open_item->set_submenu(std::move(open_submenu));
    file_menu->add_item(std::move(open_item));

    file_menu->add_item(std::make_unique<menu_item<test_backend>>("Save"));
    menu_bar_widget->add_menu("File", std::move(file_menu));

    // Open File menu
    menu_bar_widget->open_menu(0);
    // ... test navigation ...
}
```

### Rollback Plan

If menu_system causes issues:
1. Keep Phase 3 (submenu infrastructure in menu_item)
2. Remove menu_system class
3. Revert menu_bar to Phase 2 implementation

### Success Metrics

- ✅ menu_system tracks menu hierarchy
- ✅ Hotkeys registered once (no churn)
- ✅ All Phase 2 navigation still works
- ✅ Ready for submenu navigation (Phase 5)
- ✅ All existing tests pass

---

## Phase 5: Context-Dependent Submenu Navigation

**Goal:** Implement full submenu navigation with context-dependent left/right behavior.

**Priority:** LOW - Complete feature set

**Estimated Effort:** 4-6 hours

**Depends On:** Phase 4 (menu_system)

### User Experience

**Scenario 1: Open Submenu**
1. User opens File menu
2. User navigates to "Open ▶" item (down arrow)
3. User presses right arrow
4. ✅ Open submenu appears to the right
5. Focus moves to first item in submenu

**Scenario 2: Close Submenu**
1. User is in Open submenu
2. User presses left arrow
3. ✅ Submenu closes
4. Focus returns to "Open ▶" item in File menu

**Scenario 3: Switch Menu Bar from Top-Level**
1. User is in File menu on regular item (not "Open ▶")
2. User presses right arrow
3. ✅ Theme menu opens (menu bar switch)

**Scenario 4: Do Nothing in Submenu**
1. User is in Open submenu on regular item
2. User presses right arrow
3. ✅ Nothing happens (no menu bar switch in submenu)

### Implementation Details

Most of the logic is already implemented in Phase 4's menu_system! We just need to:

#### 1. Show Submenu as Popup Layer

**File:** `include/onyxui/widgets/menu_system.hh`

```cpp
void menu_system<Backend>::open_submenu(menu_type* submenu) {
    if (!submenu) return;

    // Push onto stack
    m_menu_stack.push_back(submenu);

    // ✅ NEW: Show submenu as popup layer
    auto* layers = ui_services<Backend>::layers();
    if (!layers) return;

    // Position submenu to the right of parent menu
    // (In Phase 3, we'd need to calculate position from focused item bounds)
    auto parent_menu = m_menu_stack[m_menu_stack.size() - 2];  // Parent
    auto parent_bounds = parent_menu->bounds();

    rect_type submenu_anchor;
    rect_utils::set_bounds(submenu_anchor,
        rect_utils::get_x(parent_bounds) + rect_utils::get_width(parent_bounds),  // Right edge
        rect_utils::get_y(parent_bounds),  // Same top
        1, 1  // Minimal anchor
    );

    // Show submenu as popup
    auto layer_id = layers->show_popup(
        submenu,
        submenu_anchor,
        popup_placement::right  // Or: popup_placement::below_right
    );

    // Store layer ID for cleanup
    // ... (needs layer ID tracking)
}
```

#### 2. Close Submenu and Remove Layer

**File:** `include/onyxui/widgets/menu_system.hh`

```cpp
bool menu_system<Backend>::close_current_submenu() {
    if (m_menu_stack.size() <= 1) {
        return false;  // At top-level
    }

    auto* submenu = m_menu_stack.back();

    // ✅ NEW: Remove submenu layer
    auto* layers = ui_services<Backend>::layers();
    if (layers && submenu) {
        // ... remove layer (needs layer ID tracking)
    }

    // Pop from stack
    m_menu_stack.pop_back();

    // Restore focus to parent menu
    if (auto* parent_menu = current_menu()) {
        if (auto* focus = ui_services<Backend>::input()) {
            focus->set_focus(parent_menu);
        }
    }

    return true;
}
```

#### 3. Track Submenu Layer IDs

**File:** `include/onyxui/widgets/menu_system.hh`

```cpp
class menu_system {
private:
    std::vector<menu_type*> m_menu_stack;
    std::vector<layer_id> m_submenu_layer_ids;  // ✅ NEW: Parallel to stack

    void open_submenu(menu_type* submenu) {
        // ... show popup ...
        layer_id id = layers->show_popup(...);
        m_submenu_layer_ids.push_back(id);
        m_menu_stack.push_back(submenu);
    }

    bool close_current_submenu() {
        if (m_menu_stack.size() <= 1) return false;

        // Remove layer
        auto layer_id = m_submenu_layer_ids.back();
        layers->remove_layer(layer_id);
        m_submenu_layer_ids.pop_back();

        // Pop stack
        m_menu_stack.pop_back();
        return true;
    }
};
```

### Edge Cases

1. **Submenu positioning:** Calculate position from focused item, not parent menu bounds
2. **Screen boundaries:** If no space on right, open to left
3. **Submenu of submenu:** Stack should support arbitrary depth
4. **Mouse hover:** Should open/close submenus (future enhancement)
5. **Click outside:** Should close all submenus

### Acceptance Criteria (Testable)

**Manual Test:**
1. Run demo with submenu-enabled menu
2. Navigate to item with submenu (▶ indicator visible)
3. Press right arrow
4. ✅ Submenu opens to the right
5. Press left arrow
6. ✅ Submenu closes, focus returns to parent item
7. Navigate to regular item in top-level menu
8. Press right arrow
9. ✅ Next menu bar item opens

**Unit Test:**

```cpp
TEST_CASE_FIXTURE(ui_context_fixture<test_backend>, "Submenu navigation - full flow") {
    auto menu_bar_widget = std::make_unique<menu_bar<test_backend>>();

    // Build File menu with Open submenu
    auto file_menu = std::make_unique<menu<test_backend>>();
    auto open_submenu = std::make_unique<menu<test_backend>>();
    open_submenu->add_item(std::make_unique<menu_item<test_backend>>("Project"));

    auto open_item = std::make_unique<menu_item<test_backend>>("Open");
    open_item->set_submenu(std::move(open_submenu));
    file_menu->add_item(std::move(open_item));

    menu_bar_widget->add_menu("File", std::move(file_menu));

    // Open File menu
    menu_bar_widget->open_menu(0);

    auto* layers = ui_services<test_backend>::layers();
    CHECK(layers->layer_count() == 1);  // File menu layer

    // Focus "Open ▶" item
    // ... navigate to open_item ...

    // Trigger menu_right (should open submenu)
    // ... trigger hotkey ...
    CHECK(layers->layer_count() == 2);  // File menu + Open submenu

    // Trigger menu_left (should close submenu)
    // ... trigger hotkey ...
    CHECK(layers->layer_count() == 1);  // Only File menu
}
```

### Rollback Plan

If submenu navigation causes issues:
1. Keep Phase 4 (menu_system infrastructure)
2. Disable handle_menu_right submenu opening
3. Disable handle_menu_left submenu closing
4. Phase 2 left/right menu bar switching still works

### Success Metrics

- ✅ Right arrow on submenu item opens submenu
- ✅ Left arrow in submenu closes submenu
- ✅ Submenu positioned correctly (right of parent)
- ✅ Focus management works across levels
- ✅ Multiple submenu levels work (arbitrary depth)
- ✅ All existing tests pass

---

## Testing Strategy

### Test Coverage Goals

| Phase | Unit Tests | Integration Tests | Manual Tests |
|-------|------------|-------------------|--------------|
| Phase 0 | 2 tests | 1 test | 5 scenarios |
| Phase 1 | 3 tests | 1 test | Reuse Phase 0 |
| Phase 2 | 2 tests | 1 test | 10 scenarios |
| Phase 3 | 4 tests | 1 test | 5 scenarios |
| Phase 4 | 3 tests | 2 tests | 8 scenarios |
| Phase 5 | 2 tests | 2 tests | 15 scenarios |

### Continuous Integration

Each phase must:
1. ✅ Pass all existing tests (no regressions)
2. ✅ Add new tests for new functionality
3. ✅ Pass clang-tidy with zero warnings
4. ✅ Pass valgrind (no memory leaks)
5. ✅ Build with sanitizers enabled

### Performance Benchmarks

Track performance impact of each phase:
- Menu open time (should stay < 1ms)
- Hotkey registration time (Phase 1 should reduce)
- Memory usage (Phase 4 adds ~100 bytes per menu)

---

## Risk Assessment

### High Risk

1. **Phase 0:** Critical bug fix - must work perfectly
   - Mitigation: Thorough testing, multiple reviewers

2. **Phase 4:** Large refactoring - potential for subtle bugs
   - Mitigation: Incremental integration, comprehensive tests

### Medium Risk

3. **Phase 2:** New feature - user expectations unclear
   - Mitigation: User testing, clear documentation

4. **Phase 5:** Complex navigation logic
   - Mitigation: State machine diagram, edge case testing

### Low Risk

5. **Phase 1:** RAII - well-understood pattern
6. **Phase 3:** Data structure only - minimal logic

---

## Dependencies

### External Libraries

- termbox2 (keyboard event handling)
- doctest (unit testing)

### Internal Dependencies

- hotkey_manager (semantic actions)
- layer_manager (popup display)
- input_manager (focus management)
- ui_services (service locator)

---

## Timeline Estimate

| Phase | Development | Testing | Review | Total |
|-------|-------------|---------|--------|-------|
| Phase 0 | 1 hour | 1 hour | 1 hour | 3 hours |
| Phase 1 | 2 hours | 1 hour | 1 hour | 4 hours |
| Phase 2 | 2 hours | 1 hour | 1 hour | 4 hours |
| Phase 3 | 4 hours | 2 hours | 2 hours | 8 hours |
| Phase 4 | 6 hours | 2 hours | 2 hours | 10 hours |
| Phase 5 | 4 hours | 2 hours | 2 hours | 8 hours |
| **Total** | | | | **37 hours** |

*Note: Assumes single developer, includes documentation time*

---

## Success Criteria

### Phase 0 (MVP)
- ✅ Down arrow navigation works in all menus
- ✅ Zero regressions

### Phase 1-2 (Usability)
- ✅ RAII cleanup prevents resource leaks
- ✅ Left/right arrow switches menu bar items

### Phase 3-5 (Feature Complete)
- ✅ Submenus work with arbitrary depth
- ✅ Context-dependent navigation matches user expectations
- ✅ Performance remains acceptable (< 1ms menu operations)

---

## Appendix A: Alternative Approaches Considered

### Alternative 1: Per-Menu Hotkey Scopes

**Idea:** Give each menu its own hotkey scope, activated when menu opens.

**Rejected because:**
- More complex implementation
- Doesn't solve context-dependent navigation
- Harder to coordinate menu bar switching

### Alternative 2: State Machine in menu Class

**Idea:** Each menu manages its own state machine for navigation.

**Rejected because:**
- Distributed state (same problem we're fixing)
- Can't handle menu bar switching
- Duplicate logic in every menu

### Alternative 3: Global Hotkey Filter

**Idea:** Single global filter checks which menu is open before routing.

**Rejected because:**
- Tight coupling to global state
- Hard to test
- Not extensible to submenus

---

## Appendix B: Code Review Checklist

### Phase 0
- [ ] Menu constructor no longer calls initialize_hotkeys()
- [ ] menu_bar::open_menu() calls register_navigation_hotkeys()
- [ ] menu_bar::close_menu() calls unregister_navigation_hotkeys()
- [ ] Down arrow works in all three menus (File, Theme, Help)
- [ ] No memory leaks (valgrind clean)

### Phase 1
- [ ] semantic_action_guard follows RAII principles
- [ ] Move semantics implemented correctly
- [ ] menu_bar uses vector of guards
- [ ] Automatic cleanup on close_menu()
- [ ] Exception-safe

### Phase 2
- [ ] Left/right arrow handlers registered
- [ ] open_previous_menu() wraps around correctly
- [ ] open_next_menu() wraps around correctly
- [ ] Focus moves to first item in new menu
- [ ] Visual state updates (menu bar item highlighting)

### Phase 3
- [ ] menu_item has m_submenu member
- [ ] Submenu indicator (►) renders
- [ ] Ownership semantics clear
- [ ] No circular dependency issues
- [ ] Backward compatible (existing menus work)

### Phase 4
- [ ] menu_system class compiles
- [ ] menu_bar owns menu_system
- [ ] Hotkeys registered once (no churn)
- [ ] Stack operations correct (push/pop)
- [ ] All Phase 2 tests still pass

### Phase 5
- [ ] Submenus open as popup layers
- [ ] Submenu positioning correct
- [ ] Layer cleanup on close
- [ ] Context-dependent navigation works
- [ ] Multiple submenu levels work

---

## Appendix C: Known Limitations

### Current (Pre-Refactoring)
1. No submenu support
2. No left/right navigation
3. Hotkey conflicts between menus

### Post-Phase 2
1. No submenu support (deferred to Phase 3)
2. Left/right always switches menu bar (context-dependent in Phase 5)

### Post-Phase 5
1. Mouse hover doesn't open submenus (future enhancement)
2. Submenu positioning is simple (no smart boundary detection)
3. No horizontal submenus (only vertical)

---

## Document Revision History

| Version | Date | Author | Changes |
|---------|------|--------|---------|
| 1.0 | 2025-10-28 | Assistant | Initial document |

---

**End of Implementation Plan**
