# Event Routing Architecture: Capture & Bubble Phases
## Comprehensive Implementation Document

---

## Executive Summary

Add industry-standard event routing with **capture** and **bubble** phases to OnyxUI, enabling composite widgets to intercept events before/after children handle them. This solves the text_view focus problem and establishes a foundation for complex event handling.

**Timeline**: 5 phases, ~8-10 hours total
**Backward Compatibility**: Breaking changes acceptable (as requested)
**Test Strategy**: Unit tests after each phase, integration tests at end

---

## Phase 1: Event Phase Infrastructure (2 hours)

### Goals
- Add `event_phase` enum
- Extend event handling signature
- No behavior changes yet (all events in target phase)

### Implementation

#### 1.1 Add Event Phase Enum

**File**: `include/onyxui/events/event_phase.hh` (NEW)
```cpp
#pragma once

namespace onyxui {

    /**
     * @enum event_phase
     * @brief Phase of event propagation through the widget tree
     *
     * @details
     * Follows the DOM/WPF model:
     * 1. CAPTURE: Event travels DOWN from root to target (parent first)
     * 2. TARGET: Event delivered to target element
     * 3. BUBBLE: Event travels UP from target to root (child first)
     *
     * Example for click at (10, 10):
     *   root -> panel -> text_view -> label  (capture)
     *                                 label   (target)
     *   root <- panel <- text_view <- label  (bubble)
     */
    enum class event_phase : uint8_t {
        /**
         * @brief Capture phase - event traveling down to target
         *
         * Handlers in this phase see events BEFORE children.
         * Used by composite widgets to intercept/preprocess events.
         */
        capture,

        /**
         * @brief Target phase - event at target element
         *
         * Event delivered to the element returned by hit_test().
         * Most widgets handle events in this phase.
         */
        target,

        /**
         * @brief Bubble phase - event traveling up from target
         *
         * Handlers in this phase see events AFTER children.
         * Used for event delegation patterns.
         */
        bubble
    };

    /**
     * @brief Convert event_phase to string for debugging
     */
    constexpr const char* to_string(event_phase phase) noexcept {
        switch (phase) {
            case event_phase::capture: return "capture";
            case event_phase::target: return "target";
            case event_phase::bubble: return "bubble";
        }
        return "unknown";
    }

} // namespace onyxui
```

#### 1.2 Update Event Target Interface

**File**: `include/onyxui/core/event_target.hh`

**Add new method:**
```cpp
/**
 * @brief Handle UI event with phase information
 * @param event The event to handle
 * @param phase The propagation phase (capture/target/bubble)
 * @return true if event was consumed (stops further propagation)
 *
 * @details
 * New event handling signature supporting capture/bubble phases.
 *
 * Default implementation:
 * - CAPTURE phase: return false (let it continue)
 * - TARGET phase: delegate to old handle_event()
 * - BUBBLE phase: return false (let it continue)
 *
 * Override to implement phase-specific behavior:
 *
 * @code
 * bool handle_event(const ui_event& event, event_phase phase) override {
 *     if (phase == event_phase::capture) {
 *         // Intercept before children
 *         if (auto* mouse = std::get_if<mouse_event>(&event)) {
 *             request_focus();
 *             return false;  // Let children handle too
 *         }
 *     }
 *     return base::handle_event(event, phase);
 * }
 * @endcode
 */
virtual bool handle_event(const ui_event& event, event_phase phase);
```

**Add implementation:**
```cpp
template<UIBackend Backend>
bool event_target<Backend>::handle_event(const ui_event& event, event_phase phase) {
    // Default: only handle in target phase
    if (phase == event_phase::target) {
        return handle_event(event);  // Call old signature
    }
    return false;  // Ignore capture/bubble by default
}
```

**Keep old method for now (will deprecate in Phase 5):**
```cpp
virtual bool handle_event(const ui_event& event);  // Still works
```

### Tests for Phase 1

**File**: `unittest/events/test_event_phase.cc` (NEW)
```cpp
#include <doctest/doctest.h>
#include "onyxui/events/event_phase.hh"
#include "../utils/test_helpers.hh"

using namespace onyxui;
using namespace onyxui::testing;

TEST_CASE("event_phase - Enum values") {
    CHECK(static_cast<uint8_t>(event_phase::capture) == 0);
    CHECK(static_cast<uint8_t>(event_phase::target) == 1);
    CHECK(static_cast<uint8_t>(event_phase::bubble) == 2);
}

TEST_CASE("event_phase - to_string") {
    CHECK(std::string(to_string(event_phase::capture)) == "capture");
    CHECK(std::string(to_string(event_phase::target)) == "target");
    CHECK(std::string(to_string(event_phase::bubble)) == "bubble");
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "event_target - Phase-aware handle_event") {
    using Backend = test_canvas_backend;

    class test_widget : public widget<Backend> {
    public:
        int capture_count = 0;
        int target_count = 0;
        int bubble_count = 0;

        bool handle_event(const ui_event& event, event_phase phase) override {
            if (std::holds_alternative<mouse_event>(event)) {
                switch (phase) {
                    case event_phase::capture: capture_count++; break;
                    case event_phase::target: target_count++; break;
                    case event_phase::bubble: bubble_count++; break;
                }
            }
            return false;  // Don't consume
        }
    };

    test_widget w;
    mouse_event evt{mouse_event::action::press, 10, 10, mouse_button::left};
    ui_event ui_evt = evt;

    // Default implementation: only target phase is handled
    w.handle_event(ui_evt, event_phase::capture);
    w.handle_event(ui_evt, event_phase::target);
    w.handle_event(ui_evt, event_phase::bubble);

    CHECK(w.capture_count == 1);
    CHECK(w.target_count == 1);
    CHECK(w.bubble_count == 1);
}
```

**Success Criteria:**
- ✅ All tests pass
- ✅ New signature available
- ✅ Old signature still works
- ✅ No behavioral changes to existing code

---

## Phase 2: Hit Test Path Recording (2 hours)

### Goals
- Modify `hit_test()` to return path from root to target
- Store hit test path for event routing

### Implementation

#### 2.1 Add Hit Test Path Structure

**File**: `include/onyxui/events/hit_test_path.hh` (NEW)
```cpp
#pragma once
#include <vector>
#include <onyxui/core/element.hh>

namespace onyxui {

    /**
     * @brief Path from root to target element during hit testing
     *
     * @details
     * Stores elements from root to leaf for event routing:
     * - CAPTURE phase: iterate forward (root → target)
     * - BUBBLE phase: iterate reverse (target → root)
     *
     * Example tree:
     *   root
     *     └─ panel
     *          └─ text_view
     *               └─ label  ← clicked here
     *
     * Path: [root, panel, text_view, label]
     */
    template<UIBackend Backend>
    struct hit_test_path {
        using element_type = ui_element<Backend>;

        std::vector<element_type*> elements;  ///< Root to target (inclusive)

        /**
         * @brief Get target element (deepest in tree)
         */
        [[nodiscard]] element_type* target() const noexcept {
            return elements.empty() ? nullptr : elements.back();
        }

        /**
         * @brief Get root element (shallowest in tree)
         */
        [[nodiscard]] element_type* root() const noexcept {
            return elements.empty() ? nullptr : elements.front();
        }

        /**
         * @brief Number of elements in path
         */
        [[nodiscard]] size_t depth() const noexcept {
            return elements.size();
        }

        /**
         * @brief Check if path is empty
         */
        [[nodiscard]] bool empty() const noexcept {
            return elements.empty();
        }

        /**
         * @brief Clear the path
         */
        void clear() noexcept {
            elements.clear();
        }
    };

} // namespace onyxui
```

#### 2.2 Update Hit Test Signature

**File**: `include/onyxui/core/element.hh`

**Add new overload:**
```cpp
/**
 * @brief Hit test with path recording
 * @param x X coordinate (absolute)
 * @param y Y coordinate (absolute)
 * @param path Output parameter - path from root to target
 * @return Target element (same as path.target())
 *
 * @details
 * Records all elements from root to target for event routing.
 * This enables capture/bubble event propagation.
 */
[[nodiscard]] ui_element* hit_test(int x, int y, hit_test_path<Backend>& path);
```

**Implementation:**
```cpp
template<UIBackend Backend>
ui_element<Backend>* ui_element<Backend>::hit_test(int x, int y, hit_test_path<Backend>& path) {
    if (!m_visible) {
        return nullptr;
    }

    rect_type absolute_bounds = m_bounds;
    if (!rect_utils::contains(absolute_bounds, x, y)) {
        return nullptr;
    }

    // Add self to path
    path.elements.push_back(this);

    // Check children
    rect_type content_area = get_content_area();
    int const child_offset_x = rect_utils::get_x(content_area);
    int const child_offset_y = rect_utils::get_y(content_area);
    int const child_x = x - child_offset_x;
    int const child_y = y - child_offset_y;

    for (auto it = m_children.rbegin(); it != m_children.rend(); ++it) {
        if (ui_element* hit = (*it)->hit_test(child_x, child_y, path)) {
            return hit;  // Child was hit, path already updated
        }
    }

    // No children hit, we are the target
    return this;
}

// Keep old signature for compatibility (delegates to new one)
template<UIBackend Backend>
ui_element<Backend>* ui_element<Backend>::hit_test(int x, int y) {
    hit_test_path<Backend> path;
    return hit_test(x, y, path);
}
```

### Tests for Phase 2

**File**: `unittest/events/test_hit_test_path.cc` (NEW)
```cpp
TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "hit_test_path - Simple hierarchy") {
    using Backend = test_canvas_backend;

    auto root = std::make_unique<panel<Backend>>();
    auto* child1 = root->template emplace_child<panel<Backend>>();
    auto* child2 = child1->template emplace_child<label<Backend>>("Test");

    root->measure(100, 100);
    root->arrange({0, 0, 100, 100});

    hit_test_path<Backend> path;
    auto* hit = root->hit_test(10, 10, path);

    REQUIRE(hit != nullptr);
    CHECK(path.depth() == 3);
    CHECK(path.root() == root.get());
    CHECK(path.elements[1] == child1);
    CHECK(path.target() == child2);
    CHECK(hit == child2);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "hit_test_path - Miss all elements") {
    using Backend = test_canvas_backend;

    auto root = std::make_unique<panel<Backend>>();
    root->measure(100, 100);
    root->arrange({0, 0, 100, 100});

    hit_test_path<Backend> path;
    auto* hit = root->hit_test(200, 200, path);  // Outside bounds

    CHECK(hit == nullptr);
    CHECK(path.empty());
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "hit_test_path - Complex nesting") {
    using Backend = test_canvas_backend;

    // root -> panel -> text_view -> scroll_view -> panel -> label
    auto root = std::make_unique<panel<Backend>>();
    auto* panel1 = root->template emplace_child<panel<Backend>>();
    auto* text_view = panel1->template emplace_child<text_view<Backend>>();
    text_view->set_text("Line 1\nLine 2\nLine 3");

    root->measure(100, 100);
    root->arrange({0, 0, 100, 100});

    hit_test_path<Backend> path;
    auto* hit = root->hit_test(10, 10, path);

    REQUIRE(hit != nullptr);
    CHECK(path.root() == root.get());
    CHECK(path.depth() >= 3);  // At least root -> panel -> text_view
}
```

**Success Criteria:**
- ✅ Hit test records full path
- ✅ Path ordering correct (root → target)
- ✅ Old hit_test() signature still works
- ✅ All existing tests pass

---

## Phase 3: Event Routing Engine (2 hours)

### Goals
- Implement 3-phase event routing
- Route events through capture → target → bubble
- Support event consumption (stop propagation)

### Implementation

#### 3.1 Add Event Routing Function

**File**: `include/onyxui/events/event_router.hh` (NEW)
```cpp
#pragma once
#include <onyxui/events/event_phase.hh>
#include <onyxui/events/hit_test_path.hh>
#include <onyxui/events/ui_event.hh>

namespace onyxui {

    /**
     * @brief Routes events through capture/target/bubble phases
     *
     * @details
     * Implements the standard event propagation model:
     *
     * 1. CAPTURE PHASE (root → target):
     *    - Ancestors get event first
     *    - Can intercept before children
     *    - If handled, stops immediately
     *
     * 2. TARGET PHASE (at target):
     *    - Target element handles event
     *    - If handled, stops immediately
     *
     * 3. BUBBLE PHASE (target → root):
     *    - Only if not handled in earlier phases
     *    - Ancestors process after children
     *
     * @param event The event to route
     * @param path Hit test path from root to target
     * @return true if event was handled in any phase
     */
    template<UIBackend Backend>
    bool route_event(const ui_event& event, const hit_test_path<Backend>& path) {
        if (path.empty()) {
            return false;
        }

        // PHASE 1: CAPTURE (root → target, excluding target)
        for (size_t i = 0; i < path.depth() - 1; ++i) {
            if (path.elements[i]->handle_event(event, event_phase::capture)) {
                return true;  // Consumed in capture phase
            }
        }

        // PHASE 2: TARGET (at target only)
        if (path.target()->handle_event(event, event_phase::target)) {
            return true;  // Consumed at target
        }

        // PHASE 3: BUBBLE (target → root, excluding target)
        for (size_t i = path.depth() - 1; i > 0; --i) {
            if (path.elements[i - 1]->handle_event(event, event_phase::bubble)) {
                return true;  // Consumed in bubble phase
            }
        }

        return false;  // Not handled
    }

} // namespace onyxui
```

#### 3.2 Update UI Handle to Use Router

**File**: `include/onyxui/ui_handle.hh`

**Update `handle_event()` method:**
```cpp
/**
 * @brief Handle UI event with capture/bubble routing
 */
bool handle_event(const backend_event_type& backend_event) {
    auto ui_evt = convert_event(backend_event);

    if (auto* mouse_evt = std::get_if<mouse_event>(&ui_evt)) {
        // Hit test with path recording
        hit_test_path<Backend> path;
        ui_element<Backend>* hit = m_root_widget->hit_test(
            mouse_evt->x, mouse_evt->y, path
        );

        if (hit) {
            // Route through capture → target → bubble
            return route_event(ui_evt, path);
        }
    } else {
        // Non-positional events (keyboard, etc.) go to focused element
        auto* focused = ui_services<Backend>::input()->get_focused_element();
        if (focused) {
            // Create single-element path
            hit_test_path<Backend> path;
            path.elements.push_back(focused);
            return route_event(ui_evt, path);
        }
    }

    return false;
}
```

### Tests for Phase 3

**File**: `unittest/events/test_event_routing.cc` (NEW)
```cpp
TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "event_routing - Capture phase intercepts") {
    using Backend = test_canvas_backend;

    class capture_widget : public panel<Backend> {
    public:
        bool captured = false;

        bool handle_event(const ui_event& event, event_phase phase) override {
            if (phase == event_phase::capture &&
                std::holds_alternative<mouse_event>(event)) {
                captured = true;
                return true;  // Consume in capture
            }
            return panel<Backend>::handle_event(event, phase);
        }
    };

    class target_widget : public label<Backend> {
    public:
        bool handled = false;

        bool handle_event(const ui_event& event, event_phase phase) override {
            if (phase == event_phase::target) {
                handled = true;
            }
            return label<Backend>::handle_event(event, phase);
        }
    };

    auto root = std::make_unique<capture_widget>();
    auto* target = root->template emplace_child<target_widget>();

    root->measure(100, 100);
    root->arrange({0, 0, 100, 100});

    // Hit test
    hit_test_path<Backend> path;
    root->hit_test(10, 10, path);

    // Route event
    mouse_event evt{mouse_event::action::press, 10, 10, mouse_button::left};
    bool handled = route_event(ui_event(evt), path);

    // Parent captured in capture phase, child never saw it
    CHECK(handled);
    CHECK(root->captured);
    CHECK_FALSE(target->handled);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "event_routing - Bubble phase after target") {
    using Backend = test_canvas_backend;

    class bubble_widget : public panel<Backend> {
    public:
        bool bubbled = false;

        bool handle_event(const ui_event& event, event_phase phase) override {
            if (phase == event_phase::bubble &&
                std::holds_alternative<mouse_event>(event)) {
                bubbled = true;
                return false;  // Don't consume
            }
            return panel<Backend>::handle_event(event, phase);
        }
    };

    auto root = std::make_unique<bubble_widget>();
    auto* child = root->template emplace_child<label<Backend>>("Test");

    root->measure(100, 100);
    root->arrange({0, 0, 100, 100});

    hit_test_path<Backend> path;
    root->hit_test(10, 10, path);

    mouse_event evt{mouse_event::action::press, 10, 10, mouse_button::left};
    route_event(ui_event(evt), path);

    // Parent sees event in bubble phase after child
    CHECK(root->bubbled);
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "event_routing - All three phases") {
    using Backend = test_canvas_backend;

    class phase_recorder : public panel<Backend> {
    public:
        std::vector<event_phase> phases_seen;

        bool handle_event(const ui_event& event, event_phase phase) override {
            if (std::holds_alternative<mouse_event>(event)) {
                phases_seen.push_back(phase);
            }
            return false;  // Never consume
        }
    };

    auto root = std::make_unique<phase_recorder>();
    auto* child = root->template emplace_child<phase_recorder>();

    root->measure(100, 100);
    root->arrange({0, 0, 100, 100});

    hit_test_path<Backend> path;
    child->hit_test(10, 10, path);  // Hit child directly

    mouse_event evt{mouse_event::action::press, 10, 10, mouse_button::left};
    route_event(ui_event(evt), path);

    // Root sees capture and bubble
    REQUIRE(root->phases_seen.size() == 2);
    CHECK(root->phases_seen[0] == event_phase::capture);
    CHECK(root->phases_seen[1] == event_phase::bubble);

    // Child sees target
    REQUIRE(child->phases_seen.size() == 1);
    CHECK(child->phases_seen[0] == event_phase::target);
}
```

**Success Criteria:**
- ✅ Events route through all 3 phases correctly
- ✅ Capture phase intercepts before target
- ✅ Bubble phase sees event after target
- ✅ Consuming event stops propagation
- ✅ All tests pass

---

## Phase 4: Fix text_view Focus (1 hour)

### Goals
- Apply new event routing to solve text_view problem
- Test in real application

### Implementation

#### 4.1 Update text_view to Use Capture Phase

**File**: `include/onyxui/widgets/text_view.hh`

**Replace current `handle_event()` with phase-aware version:**
```cpp
/**
 * @brief Handle events with capture phase for focus acquisition
 *
 * @details
 * Uses CAPTURE PHASE to intercept mouse clicks before internal
 * widgets (scroll_view, labels) handle them. This ensures text_view
 * gets focus when clicked anywhere within its bounds.
 */
bool handle_event(const ui_event& event, event_phase phase) override {
    // CAPTURE PHASE: Intercept mouse clicks for focus
    if (phase == event_phase::capture) {
        if (auto* mouse_evt = std::get_if<mouse_event>(&event)) {
            if (mouse_evt->act == mouse_event::action::press) {
                // Request focus when clicked
                auto* input = ui_services<Backend>::input();
                if (input && base::is_focusable()) {
                    input->set_focus(this);
                }
                // DON'T consume - let children handle too
                return false;
            }
        }
    }

    // Delegate other phases to base
    return base::handle_event(event, phase);
}
```

**Remove old handle_event() override** (no longer needed)

#### 4.2 Add Integration Test

**File**: `unittest/widgets/test_text_view_focus.cc` (NEW)
```cpp
TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "text_view - Click anywhere gives focus") {
    using Backend = test_canvas_backend;

    auto text_view_widget = std::make_unique<text_view<Backend>>();

    std::string content;
    for (int i = 1; i <= 20; i++) {
        content += "Line " + std::to_string(i) + "\n";
    }
    text_view_widget->set_text(content);

    auto size = text_view_widget->measure(40, 10);
    (void)size;
    text_view_widget->arrange({0, 0, 40, 10});

    // Initially not focused
    REQUIRE_FALSE(text_view_widget->has_focus());

    // Simulate click with full event routing
    hit_test_path<Backend> path;
    auto* hit = text_view_widget->hit_test(10, 5, path);
    REQUIRE(hit != nullptr);

    // Hit is probably a label deep inside, but path includes text_view
    bool found_text_view = false;
    for (auto* elem : path.elements) {
        if (elem == text_view_widget.get()) {
            found_text_view = true;
            break;
        }
    }
    REQUIRE(found_text_view);

    // Route mouse event through capture/target/bubble
    mouse_event evt{mouse_event::action::press, 10, 5, mouse_button::left};
    route_event(ui_event(evt), path);

    // text_view should have focus now (captured in capture phase)
    CHECK(text_view_widget->has_focus());
}
```

**Success Criteria:**
- ✅ Clicking text_view gives it focus
- ✅ Works regardless of which child is clicked
- ✅ Keyboard scrolling works after click
- ✅ Demo application works correctly

---

## Phase 5: Cleanup & Documentation (1 hour)

### Goals
- Deprecate old event handling signature
- Update all widgets to new signature
- Comprehensive documentation

### Implementation

#### 5.1 Deprecate Old handle_event()

**File**: `include/onyxui/core/event_target.hh`

```cpp
/**
 * @brief Handle UI event (DEPRECATED - use phase-aware version)
 * @deprecated Use handle_event(const ui_event&, event_phase) instead
 *
 * This method is called only during TARGET phase by the default
 * implementation of the new signature. Override the new signature
 * for phase-aware event handling.
 */
[[deprecated("Use handle_event(const ui_event&, event_phase) instead")]]
virtual bool handle_event(const ui_event& event);
```

#### 5.2 Update Documentation

**File**: `docs/EVENT_ROUTING.md` (NEW)
```markdown
# Event Routing in OnyxUI

## Overview

OnyxUI uses a three-phase event routing model inspired by DOM/WPF:

1. **Capture Phase**: Root → Target (parent intercepts first)
2. **Target Phase**: Event at target element
3. **Bubble Phase**: Target → Root (child processes first)

## Event Flow Diagram

```
User clicks at (10, 10)
         ↓
    Hit Test → [root, panel, text_view, label]
         ↓
┌─────────────────────────────────────┐
│  CAPTURE PHASE (root → target)      │
├─────────────────────────────────────┤
│  1. root->handle_event(capture)     │  ← Can intercept here
│  2. panel->handle_event(capture)    │  ← Or here
│  3. text_view->handle_event(capture)│  ← Or here
└─────────────────────────────────────┘
         ↓ (if not consumed)
┌─────────────────────────────────────┐
│  TARGET PHASE (at target)           │
├─────────────────────────────────────┤
│  4. label->handle_event(target)     │  ← Target handles
└─────────────────────────────────────┘
         ↓ (if not consumed)
┌─────────────────────────────────────┐
│  BUBBLE PHASE (target → root)       │
├─────────────────────────────────────┤
│  5. text_view->handle_event(bubble) │
│  6. panel->handle_event(bubble)     │
│  7. root->handle_event(bubble)      │
└─────────────────────────────────────┘
```

## When to Use Each Phase

### Capture Phase
**Use when**: Parent needs to intercept before children
**Examples**:
- Composite widgets requesting focus
- Input validation before child processes
- Event filtering/transformation

```cpp
bool handle_event(const ui_event& event, event_phase phase) override {
    if (phase == event_phase::capture) {
        // Intercept before children
        request_focus();
        return false;  // Let children handle too
    }
    return base::handle_event(event, phase);
}
```

### Target Phase
**Use when**: Widget is the intended recipient
**Examples**:
- Button clicks
- Text input
- Direct widget interaction

```cpp
bool handle_event(const ui_event& event, event_phase phase) override {
    if (phase == event_phase::target) {
        if (auto* mouse = std::get_if<mouse_event>(&event)) {
            if (mouse->act == mouse_event::action::press) {
                on_click();
                return true;  // Consumed
            }
        }
    }
    return base::handle_event(event, phase);
}
```

### Bubble Phase
**Use when**: Parent needs to react after children
**Examples**:
- Event logging/analytics
- Cleanup after child processing
- Event delegation patterns

```cpp
bool handle_event(const ui_event& event, event_phase phase) override {
    if (phase == event_phase::bubble) {
        log_user_interaction(event);
        return false;  // Don't consume
    }
    return base::handle_event(event, phase);
}
```

## Stopping Propagation

Return `true` to stop event from continuing to next phase:

```cpp
bool handle_event(const ui_event& event, event_phase phase) override {
    if (phase == event_phase::capture) {
        if (should_block_children(event)) {
            return true;  // STOP! Children never see it
        }
    }
    return false;
}
```

## Migration Guide

### Old Code (deprecated)
```cpp
bool handle_event(const ui_event& event) override {
    if (auto* mouse = std::get_if<mouse_event>(&event)) {
        // Handle event
        return true;
    }
    return base::handle_event(event);
}
```

### New Code
```cpp
bool handle_event(const ui_event& event, event_phase phase) override {
    if (phase == event_phase::target) {
        if (auto* mouse = std::get_if<mouse_event>(&event)) {
            // Handle event
            return true;
        }
    }
    return base::handle_event(event, phase);
}
```

## Common Patterns

### Composite Widget Focus
```cpp
// text_view, scroll_view, etc.
bool handle_event(const ui_event& event, event_phase phase) override {
    if (phase == event_phase::capture) {
        if (is_mouse_press(event)) {
            request_focus();  // Before children handle
        }
    }
    return base::handle_event(event, phase);
}
```

### Event Delegation
```cpp
// Parent handles all child button clicks
bool handle_event(const ui_event& event, event_phase phase) override {
    if (phase == event_phase::bubble) {
        if (is_button_click(event)) {
            handle_any_button_click();
        }
    }
    return base::handle_event(event, phase);
}
```

### Input Validation
```cpp
// Prevent invalid input from reaching children
bool handle_event(const ui_event& event, event_phase phase) override {
    if (phase == event_phase::capture) {
        if (auto* key = std::get_if<keyboard_event>(&event)) {
            if (!is_valid_input(key->ch)) {
                return true;  // Block invalid input
            }
        }
    }
    return base::handle_event(event, phase);
}
```
```

### Tests for Phase 5

**Update all existing event tests** to use new signature
**Add deprecation warning tests**

**Success Criteria:**
- ✅ All widgets use new signature
- ✅ Documentation complete
- ✅ No compiler warnings (except deprecation)
- ✅ All 1187+ tests pass

---

## Testing Strategy

### Unit Tests (Each Phase)
- Phase-specific behavior
- Edge cases
- Backward compatibility

### Integration Tests (Phase 4)
- Real widget interactions
- Complex hierarchies
- Demo application

### Performance Tests (Phase 5)
- Event routing overhead
- Hit test path allocation
- Memory usage

---

## Rollout Plan

1. **Week 1**: Phases 1-2 (Infrastructure)
2. **Week 2**: Phase 3 (Routing engine)
3. **Week 3**: Phases 4-5 (text_view + cleanup)

---

## Success Metrics

- ✅ text_view focus works with mouse click
- ✅ All existing tests pass
- ✅ New tests cover capture/bubble patterns
- ✅ Performance overhead < 5% for event handling
- ✅ Documentation complete

---

## Related Resources

### Industry References

1. **DOM Event Model** (W3C Standard)
   - Capture → Target → Bubble phases
   - `stopPropagation()` and `stopImmediatePropagation()`
   - Reference: https://www.w3.org/TR/DOM-Level-3-Events/

2. **WPF Routed Events** (Microsoft)
   - Preview events (tunneling) + normal events (bubbling)
   - `e.Handled = true` stops routing
   - Reference: https://learn.microsoft.com/en-us/dotnet/desktop/wpf/events/routed-events-overview

3. **Qt Event System**
   - `eventFilter()` for interception
   - `event->ignore()` for propagation
   - Reference: https://doc.qt.io/qt-6/eventsandfilters.html

4. **Flutter Gesture Arena**
   - `HitTestBehavior` for composite widgets
   - Gesture recognition with conflict resolution
   - Reference: https://api.flutter.dev/flutter/gestures/GestureDetector-class.html

---

## Implementation Checklist

### Phase 1: Infrastructure
- [ ] Create `include/onyxui/events/event_phase.hh`
- [ ] Add `event_phase` enum (capture/target/bubble)
- [ ] Add `to_string()` helper
- [ ] Update `event_target.hh` with new `handle_event(event, phase)` signature
- [ ] Implement default behavior (delegate to old signature in target phase)
- [ ] Create `unittest/events/test_event_phase.cc`
- [ ] Run tests: `./build/bin/ui_unittest --test-case="*event_phase*"`
- [ ] Verify all 1187 existing tests still pass

### Phase 2: Hit Test Path
- [ ] Create `include/onyxui/events/hit_test_path.hh`
- [ ] Add `hit_test_path<Backend>` struct
- [ ] Add path accessors (target(), root(), depth(), empty())
- [ ] Update `element.hh` with new `hit_test(x, y, path)` overload
- [ ] Implement path recording during hit test recursion
- [ ] Keep old `hit_test(x, y)` for compatibility
- [ ] Create `unittest/events/test_hit_test_path.cc`
- [ ] Test simple, complex, and miss scenarios
- [ ] Run tests: `./build/bin/ui_unittest --test-case="*hit_test_path*"`
- [ ] Verify all existing tests still pass

### Phase 3: Event Router
- [ ] Create `include/onyxui/events/event_router.hh`
- [ ] Implement `route_event()` function
- [ ] Add capture phase loop (root → target-1)
- [ ] Add target phase dispatch
- [ ] Add bubble phase loop (target-1 → root)
- [ ] Update `ui_handle.hh` to use new router
- [ ] Handle mouse events with hit test path
- [ ] Handle keyboard events to focused element
- [ ] Create `unittest/events/test_event_routing.cc`
- [ ] Test capture intercept, bubble propagation, all phases
- [ ] Run tests: `./build/bin/ui_unittest --test-case="*event_routing*"`
- [ ] Verify all existing tests still pass

### Phase 4: text_view Fix
- [ ] Update `text_view.hh` with phase-aware `handle_event()`
- [ ] Implement capture phase for focus acquisition
- [ ] Remove old `handle_event()` override
- [ ] Create `unittest/widgets/test_text_view_focus.cc`
- [ ] Test click-to-focus with hit test path
- [ ] Test keyboard scrolling after focus
- [ ] Run tests: `./build/bin/ui_unittest --test-case="*text_view*focus*"`
- [ ] Test in demo application: `./build/bin/widgets_demo`
- [ ] Verify clicking text_view gives focus and scrolling works

### Phase 5: Cleanup
- [ ] Mark old `handle_event(event)` as `[[deprecated]]`
- [ ] Update all widgets to use new signature
  - [ ] button.hh
  - [ ] label.hh
  - [ ] menu_*.hh
  - [ ] scroll_*.hh
  - [ ] etc.
- [ ] Create `docs/EVENT_ROUTING.md`
- [ ] Add usage examples for each phase
- [ ] Add migration guide
- [ ] Add common patterns
- [ ] Update existing event tests to use new signature
- [ ] Run full test suite: `./build/bin/ui_unittest`
- [ ] Verify 1187+ tests pass with no warnings (except deprecation)
- [ ] Performance test: measure event routing overhead

---

## FAQ

### Q: Why not just make hit_test() virtual and override in text_view?
**A**: Virtual call overhead in performance-critical path. Also breaks design principle that hit_test finds actual target, not arbitrary parent.

### Q: Why three phases instead of just two (capture + bubble)?
**A**: Separating TARGET phase makes code clearer - most widgets only need target phase. Also matches DOM/WPF models.

### Q: What about performance?
**A**:
- Hit test path: O(depth) memory, one allocation per click
- Event routing: O(depth) iterations, no allocations
- Expected overhead: < 5% for typical trees (depth < 10)
- Optimization: pool `hit_test_path` objects if needed

### Q: Can events be consumed in bubble phase?
**A**: Yes! Any phase can consume by returning `true`. This stops all further routing.

### Q: What happens if I return `true` in capture phase?
**A**: Event stops immediately. Target and bubble phases never run. Children never see the event.

### Q: Can I handle the same event in multiple phases?
**A**: Yes! Same widget can handle in capture AND target, or target AND bubble, etc. Just don't consume if you want it to continue.

---

## Next Steps

After successful implementation:

1. **Add to CHANGELOG.md** - Document breaking change
2. **Update CLAUDE.md** - Add event routing to core concepts
3. **Create migration script** - Tool to update existing widgets
4. **Performance benchmark** - Compare old vs new routing
5. **Blog post** - Explain design decisions and patterns

---

**Document Version**: 1.0
**Last Updated**: 2025-01-04
**Author**: Claude Code Implementation Plan
