# Code Refactoring Plan

**Generated:** 2025-11-08
**Based on:** Code review of commit 9ac991f
**Status:** Planning

---

## Executive Summary

This document tracks technical debt and refactoring opportunities identified during code review. Issues are prioritized by severity and organized into actionable sprints.

**Statistics:**
- Total Issues: 17
- Critical: 3
- High: 4
- Medium: 5
- Low: 5

---

## Immediate Fixes (Must Do Before Next Commit)

### 1. Remove Debug Traces (std::cerr)

**Severity:** Critical
**Type:** Code Quality
**Effort:** 30 minutes
**Status:** ⏳ Pending

**Action:** Delete all `std::cerr` debug trace lines from library code.

**Files to clean:**

#### `include/onyxui/widgets/button.hh`
```cpp
// DELETE these lines:
std::cerr << "[button] Mouse press on button '" << m_text << "'" << std::endl;
std::cerr << "[button] is_focusable=" << this->is_focusable() << std::endl;
```
**Lines:** 165-172

#### `include/onyxui/widgets/core/widget.hh`
```cpp
// DELETE these lines:
std::cerr << "[widget] Emitting clicked signal" << std::endl;
std::cerr << "[widget] Mouse release NOT inside - clicked NOT emitted (handled=0)" << std::endl;
```
**Lines:** 574-578

#### `examples/demo.hh`
```cpp
// DELETE these lines:
std::cerr << "Error: No renderer available!" << std::endl;
std::cerr << "Screenshot saved to: " << filename << std::endl;
std::cerr << "New action triggered via menu/hotkey!" << std::endl;
std::cerr << "Open action triggered!" << std::endl;
std::cerr << "Quit action triggered - exiting application!" << std::endl;
std::cerr << "About: DOS Theme Showcase v2.0 - Theme System Edition" << std::endl;
```
**Lines:** 128-150, 260-291

**Rationale:**
- Library code should not perform I/O
- Makes testing difficult (output pollution)
- Debug logs left from development

**Test Impact:** None (only removes debug output)

---

### 2. Replace Magic Number 999999

**Severity:** High
**Type:** Code Quality
**Effort:** 15 minutes
**Status:** ⏳ Pending

**Action:** Replace magic number with `std::numeric_limits<int>::max()`

**File:** `include/onyxui/widgets/text_view.hh`

**Before:**
```cpp
case hotkey_action::scroll_end:
    m_scroll_view->scroll_to(0, 999999);  // ← Magic number
    return true;
```

**After:**
```cpp
case hotkey_action::scroll_end:
    m_scroll_view->scroll_to(0, std::numeric_limits<int>::max());
    return true;
```

**Lines:** 251, 402 (appears twice)

**Test Impact:** None (semantic equivalent)

---

### 3. Refactor display() Duplication

**Severity:** Critical
**Type:** Code Duplication
**Effort:** 1 hour
**Status:** ⏳ Pending

**Action:** Eliminate 50+ lines of duplicated code between two `display()` overloads.

**File:** `include/onyxui/ui_handle.hh`
**Lines:** 162-260

**Current State:**
```cpp
void display() {
    if (!m_root) return;
    auto bounds = m_renderer.get_viewport();
    auto dirty_regions = m_root->get_and_clear_dirty_regions();
    // ... 50 more lines ...
}

void display(const rect_type& bounds) {
    if (!m_root) return;
    auto dirty_regions = m_root->get_and_clear_dirty_regions();
    // ... 50 more identical lines ...
}
```

**Refactored:**
```cpp
void display() {
    display(m_renderer.get_viewport());
}

void display(const rect_type& bounds) {
    if (!m_root) return;

    // All logic here (single copy)
    auto dirty_regions = m_root->get_and_clear_dirty_regions();

    // ... rest of implementation ...
}
```

**Benefits:**
- Eliminates 50 lines of duplication
- Single source of truth
- Easier maintenance

**Test Impact:**
- Should verify both overloads still work
- Run full test suite (1217 tests)

---

## Short Term Fixes (Next Sprint)

### 4. Document Mouse Release Workaround

**Severity:** Critical
**Type:** Hack/Workaround
**Effort:** 30 minutes
**Status:** ⏳ Pending

**Action:** Document the synthetic mouse release workaround and create tracking issue.

**File:** `include/onyxui/ui_handle.hh`
**Lines:** 484-501

**Current Code:**
```cpp
// WORKAROUND: Some backends (like termbox2) don't send mouse release events.
// If we're pressing on a different widget than what's captured,
// implicitly release the old capture first by sending it a mouse release event.
auto* currently_captured = input->get_captured();
if (currently_captured && currently_captured != target_widget) {
    // Send mouse release to previously captured widget to clean up its state
    mouse_event release{
        .x = mouse_x,
        .y = mouse_y,
        .btn = mouse_event::button::left,
        .act = mouse_event::action::release,
        .modifiers = {}
    };
    static_cast<widget_type*>(currently_captured)->handle_event(
        ui_event{release}, event_phase::target
    );
    input->release_capture();
}
```

**Enhanced Documentation:**
```cpp
// WORKAROUND for termbox2 backend limitation
// GitHub Issue: #XXX - Backend should guarantee mouse release events
//
// Problem: termbox2 does not send TB_EVENT_MOUSE with release action
// Impact: Widgets stay in pressed state, capture never released
// Solution: Synthesize release event when press detected on different widget
//
// TODO: Either:
//   1. Fix termbox2 backend to normalize mouse events, OR
//   2. Add backend_capabilities trait system, OR
//   3. Create event normalization layer at backend boundary
//
// This is a TEMPORARY workaround - do not rely on this behavior.
```

**Create GitHub Issue:**
- Title: "Backend should guarantee delivery of mouse release events"
- Label: `technical-debt`, `backend`, `mouse-events`
- Milestone: v1.0

**Test Impact:** None

---

### 5. Extract Minimum Size Enforcement Helper

**Severity:** Medium
**Type:** Code Duplication
**Effort:** 2 hours
**Status:** ⏳ Pending

**Action:** Create shared helper class for scrollbar minimum size enforcement.

**Duplicated Code:**
- `include/onyxui/widgets/containers/scroll_view.hh` (lines 317-338)
- `include/onyxui/widgets/text_view.hh` (lines 371-399)

**Solution:**

Create new file: `include/onyxui/widgets/containers/scroll/scrollbar_size_utils.hh`

```cpp
namespace onyxui {

/**
 * @brief Helper utilities for scrollbar size calculations
 */
template<UIBackend Backend>
class scrollbar_size_utils {
public:
    /**
     * @brief Get minimum scrollbar dimensions from theme
     * @param theme Theme pointer (can be nullptr)
     * @return Pair of {thickness, min_length} with fallback to defaults
     */
    static std::pair<int, int> get_scrollbar_dimensions(
        const ui_theme<Backend>* theme
    ) {
        int thickness = theme
            ? theme->scrollbar.width
            : ui_constants::DEFAULT_SCROLLBAR_THICKNESS;

        int min_length = theme
            ? theme->scrollbar.min_render_size
            : ui_constants::DEFAULT_SCROLLBAR_MIN_RENDER_SIZE;

        return {thickness, min_length};
    }

    /**
     * @brief Enforce minimum size for scrollbar visibility
     * @param size Current measured size (modified in-place)
     * @param theme Theme to get dimensions from
     * @param h_policy Horizontal scrollbar visibility
     * @param v_policy Vertical scrollbar visibility
     * @param additional_space Extra space (borders, padding, etc.)
     */
    static void enforce_minimum_size(
        typename Backend::size_type& size,
        const ui_theme<Backend>* theme,
        scrollbar_visibility h_policy,
        scrollbar_visibility v_policy,
        int additional_space = 0
    ) {
        auto [thickness, min_length] = get_scrollbar_dimensions(theme);

        if (h_policy == scrollbar_visibility::always) {
            int min_height = min_length + additional_space;
            if (size.h < min_height) {
                size.h = min_height;
            }
        }

        if (v_policy == scrollbar_visibility::always) {
            int min_width = min_length + additional_space;
            if (size.w < min_width) {
                size.w = min_width;
            }
        }
    }
};

} // namespace onyxui
```

**Usage Example:**
```cpp
// In scroll_view.hh
auto [thickness, min_length] = scrollbar_size_utils<Backend>::get_scrollbar_dimensions(theme);

// In text_view.hh
scrollbar_size_utils<Backend>::enforce_minimum_size(
    measured, theme,
    scrollbar_visibility::hidden,   // horizontal
    scrollbar_visibility::always,   // vertical
    this->has_border() ? 2 : 0      // border space
);
```

**Test Impact:** Add unit tests for new utility class

---

### 6. Fix Unused Parameter Warning

**Severity:** Low
**Type:** Code Quality
**Effort:** 5 minutes
**Status:** ⏳ Pending

**Action:** Remove unnecessary void cast - parameter is actually used.

**File:** `include/onyxui/widgets/text_view.hh`
**Lines:** 327

**Before:**
```cpp
void on_focus_changed(bool gained) override {
    (void)gained;  // Parameter not used, just need to redraw
    this->invalidate_visual();
    base::on_focus_changed(gained);  // ← Actually IS used here!
}
```

**After:**
```cpp
void on_focus_changed(bool gained) override {
    this->invalidate_visual();
    base::on_focused_changed(gained);
}
```

**Note:** For genuinely unused parameters, prefer `[[maybe_unused]]` over `(void)param`:

```cpp
// Good:
void some_method([[maybe_unused]] int unused_param) {
    // ...
}

// Avoid:
void some_method(int unused_param) {
    (void)unused_param;
    // ...
}
```

**Test Impact:** None

---

## Medium Term Fixes (Next Release)

### 7. Implement Logging Abstraction

**Severity:** High
**Type:** Architecture
**Effort:** 1 week
**Status:** 📋 Planned

**Problem:** Direct `std::cerr` calls violate separation of concerns (even after removing debug traces, application code still uses it).

**Solution:** Add logging service to `ui_services`.

**Design:**

Create `include/onyxui/services/logger.hh`:

```cpp
namespace onyxui {

/**
 * @brief Logging abstraction for UI framework
 */
template<UIBackend Backend>
class logger {
public:
    enum class level : std::uint8_t {
        debug,
        info,
        warning,
        error
    };

    virtual ~logger() = default;

    /**
     * @brief Log a message
     * @param lvl Severity level
     * @param component Component name (e.g., "button", "menu")
     * @param message Log message
     */
    virtual void log(
        level lvl,
        std::string_view component,
        std::string_view message
    ) = 0;
};

/**
 * @brief Null logger (no-op) for production
 */
template<UIBackend Backend>
class null_logger : public logger<Backend> {
public:
    void log(
        typename logger<Backend>::level,
        std::string_view,
        std::string_view
    ) override {
        // No-op
    }
};

/**
 * @brief Console logger for debugging
 */
template<UIBackend Backend>
class console_logger : public logger<Backend> {
public:
    void log(
        typename logger<Backend>::level lvl,
        std::string_view component,
        std::string_view message
    ) override {
        std::cerr << "[" << to_string(lvl) << "] "
                  << "[" << component << "] "
                  << message << "\n";
    }
};

/**
 * @brief Capturing logger for tests
 */
template<UIBackend Backend>
class capturing_logger : public logger<Backend> {
    std::vector<std::tuple<
        typename logger<Backend>::level,
        std::string,
        std::string
    >> m_messages;

public:
    void log(
        typename logger<Backend>::level lvl,
        std::string_view component,
        std::string_view message
    ) override {
        m_messages.emplace_back(lvl, std::string(component), std::string(message));
    }

    const auto& messages() const { return m_messages; }
    void clear() { m_messages.clear(); }
};

} // namespace onyxui
```

**Add to ui_services:**

```cpp
// ui_services.hh
template<UIBackend Backend>
class ui_services {
    static logger<Backend>* s_logger;

public:
    static logger<Backend>* logger() noexcept { return s_logger; }
    static void set_logger(logger<Backend>* log) noexcept { s_logger = log; }
};
```

**Usage:**
```cpp
// In widgets (conditional logging)
if (auto* log = ui_services<Backend>::logger()) {
    log->log(logger<Backend>::level::debug, "button",
             std::format("Mouse press at ({}, {})", x, y));
}

// In demo app
console_logger<Backend> debug_log;
ui_services<Backend>::set_logger(&debug_log);

// In tests
capturing_logger<Backend> test_log;
ui_services<Backend>::set_logger(&test_log);
// ... run test ...
REQUIRE(test_log.messages().size() == 2);
```

**Benefits:**
- Testable logging
- Configurable output destination
- No I/O in library code by default
- Performance: null logger has zero overhead

**Test Impact:** Add tests for all logger implementations

---

### 8. Fix Renderer Injection in Demo

**Severity:** High
**Type:** Architecture
**Effort:** 2 hours
**Status:** 📋 Planned

**Problem:** Demo widget holds renderer pointer, violating MVC separation.

**File:** `examples/demo.hh`
**Lines:** 89-91, 468

**Current (Bad):**
```cpp
class main_widget : public onyxui::panel<Backend> {
    typename Backend::renderer_type* m_renderer = nullptr;  // ← BAD

    void set_renderer(typename Backend::renderer_type* renderer) {
        m_renderer = renderer;
    }
};

// In main.cc
widget_ptr->set_renderer(&ui.renderer());  // ← Coupling
```

**Solution Options:**

#### Option 1: Callback Injection (Recommended)
```cpp
// In demo.hh
class main_widget : public onyxui::panel<Backend> {
    std::function<void(std::ostream&)> m_screenshot_callback;

public:
    void set_screenshot_callback(std::function<void(std::ostream&)> callback) {
        m_screenshot_callback = std::move(callback);
    }
};

// In main.cc
widget_ptr->set_screenshot_callback([&ui](std::ostream& out) {
    ui.renderer().take_screenshot(out);
});
```

#### Option 2: Add to ui_services
```cpp
// Add renderer to ui_services
template<UIBackend Backend>
class ui_services {
    static typename Backend::renderer_type* s_renderer;

public:
    static auto* renderer() noexcept { return s_renderer; }
};

// In widget
if (auto* renderer = ui_services<Backend>::renderer()) {
    renderer->take_screenshot(file);
}
```

#### Option 3: Action-Based (Best for Reusability)
```cpp
// Create reusable action
auto screenshot_action = std::make_shared<action<Backend>>();
screenshot_action->set_text("Screenshot");
screenshot_action->set_shortcut('s', key_modifier::ctrl);
screenshot_action->triggered.connect([&ui]() {
    std::ofstream file("screenshot.txt");
    ui.renderer().take_screenshot(file);
});

screenshot_btn->set_action(screenshot_action);
```

**Recommendation:** Use Option 3 (Action-Based) as it's most reusable and follows existing patterns.

**Test Impact:** Verify screenshot functionality still works

---

### 9. Add Theme-Based Focus Colors

**Severity:** High
**Type:** Architecture
**Effort:** 3 hours
**Status:** 📋 Planned

**Problem:** text_view hardcodes Norton Utilities 8 yellow color for focus indicator.

**File:** `include/onyxui/widgets/text_view.hh`
**Lines:** 354-356

**Current (Bad):**
```cpp
if (this->has_focus()) {
    // Norton Utilities 8 yellow: RGB(255, 255, 0)  ← Hardcoded!
    style.foreground_color = typename Backend::color_type{255, 255, 0};
    style.border_color = typename Backend::color_type{255, 255, 0};
}
```

**Solution:**

#### Step 1: Add to theme structure

```cpp
// In include/onyxui/theming/theme.hh
struct text_view_style {
    color_type normal_foreground;
    color_type normal_border;
    color_type focused_foreground;    // ← NEW
    color_type focused_border;        // ← NEW

    // Reflection for YAML
    REFLECTCPP_MEMBERS(
        normal_foreground,
        normal_border,
        focused_foreground,
        focused_border
    );
};

struct ui_theme {
    // ... other styles ...
    text_view_style text_view;  // ← ADD
};
```

#### Step 2: Update text_view to use theme

```cpp
// In text_view.hh
resolved_style<Backend> resolve_style(
    const ui_theme<Backend>* theme,
    const resolved_style<Backend>& parent_style
) const override {
    auto style = base::resolve_style(theme, parent_style);

    if (this->has_focus() && theme) {
        // Use theme colors instead of hardcoded values
        style.foreground_color = theme->text_view.focused_foreground;
        style.border_color = theme->text_view.focused_border;
    } else if (theme) {
        style.foreground_color = theme->text_view.normal_foreground;
        style.border_color = theme->text_view.normal_border;
    }

    return style;
}
```

#### Step 3: Update theme YAML files

```yaml
# themes/nu8.yaml
text_view:
  normal_foreground: { r: 0, g: 255, b: 255 }      # Cyan
  normal_border: { r: 0, g: 255, b: 255 }           # Cyan
  focused_foreground: { r: 255, g: 255, b: 0 }      # Yellow (NU8 style)
  focused_border: { r: 255, g: 255, b: 0 }          # Yellow

# themes/norton_blue.yaml
text_view:
  normal_foreground: { r: 0, g: 170, b: 255 }      # Blue
  normal_border: { r: 0, g: 170, b: 255 }
  focused_foreground: { r: 255, g: 255, b: 0 }      # Yellow
  focused_border: { r: 255, g: 255, b: 0 }
```

**Benefits:**
- Themeable focus indicator
- Works with all themes, not just NU8
- Proper separation of presentation from logic

**Test Impact:** Update theme tests to verify new fields

---

### 10. Establish Error Handling Policy

**Severity:** High
**Type:** Architecture
**Effort:** 1 day (documentation + refactoring)
**Status:** 📋 Planned

**Problem:** Inconsistent error handling across codebase.

**Examples:**
```cpp
// Some code throws
if (!theme) {
    throw std::runtime_error("Theme not initialized");
}

// Some code returns nullopt/invalid
if (!layers) return layer_id::invalid();

// Some code returns false
if (!input) return false;

// Some code prints and continues
if (!m_renderer) {
    std::cerr << "Error: No renderer available!" << std::endl;
    return;
}
```

**Solution: Document Error Handling Policy**

Create `docs/ERROR_HANDLING_POLICY.md`:

```markdown
# Error Handling Policy

## Categories

### 1. Programming Errors (Precondition Violations)
**Examples:**
- Null pointer passed when nullptr not allowed
- Invalid enum value
- Out of bounds index
- Missing required dependency (theme, context)

**Policy:** Throw exceptions (std::logic_error family)
- `std::invalid_argument` - Bad parameter value
- `std::out_of_range` - Index out of bounds
- `std::logic_error` - General precondition violation

### 2. Runtime Errors (External Failures)
**Examples:**
- File I/O failure
- Network timeout
- Out of memory
- Backend initialization failure

**Policy:** Use std::expected<T, Error> or throw std::runtime_error

### 3. Expected Failures (Part of Normal Flow)
**Examples:**
- Widget not found in tree
- Action not registered
- Optional feature disabled

**Policy:** Return std::optional<T> or nullable pointer

### 4. Service Locator Pattern
**Examples:**
- ui_services<Backend>::themes()
- ui_services<Backend>::input()

**Policy:**
- Return nullable pointer (can be nullptr if not initialized)
- Caller must check before dereferencing
- Document with @return tag whether nullptr is possible

## Implementation Guidelines

1. **Add [[nodiscard]] to all error indicators**
   ```cpp
   [[nodiscard]] std::optional<layer_id> find_layer(const std::string& name);
   ```

2. **Document exceptions with @throws**
   ```cpp
   /**
    * @throws std::invalid_argument If text is empty
    * @throws std::runtime_error If theme not initialized
    */
   void set_text(const std::string& text);
   ```

3. **Prefer early returns for preconditions**
   ```cpp
   if (!is_valid()) {
       throw std::logic_error("Widget not in valid state");
   }
   ```

4. **Never catch and log-then-continue in library code**
   ```cpp
   // BAD:
   try {
       do_something();
   } catch (const std::exception& e) {
       std::cerr << "Error: " << e.what() << "\n";
       return;  // Silent failure!
   }

   // GOOD:
   // Let exception propagate to caller
   do_something();
   ```
```

**Then:** Audit all error handling and make consistent.

**Test Impact:** Add tests for error conditions

---

## Low Priority (Future Consideration)

### 11. Add Backend Capability Trait System

**Severity:** Medium
**Type:** Architecture
**Effort:** 2 weeks
**Status:** 💭 Idea

**Problem:** Backends have different capabilities (mouse release events, color depth, etc.)

**Solution:**
```cpp
template<UIBackend Backend>
struct backend_capabilities {
    static constexpr bool guarantees_mouse_release_events = false;
    static constexpr bool supports_truecolor = true;
    static constexpr bool supports_mouse_wheel = true;
    // ...
};

// Specialize for each backend
template<>
struct backend_capabilities<conio_backend> {
    static constexpr bool guarantees_mouse_release_events = false;  // termbox2 limitation
    // ...
};
```

**Usage:**
```cpp
if constexpr (!backend_capabilities<Backend>::guarantees_mouse_release_events) {
    // Apply workaround
}
```

---

### 12. Naming Consistency Pass

**Severity:** Low
**Type:** Code Quality
**Effort:** 2 hours
**Status:** 💭 Idea

**Issues:**
- `m_theme_connection` (singular) vs `m_action_connections` (plural)
- Inconsistent use of `ptr` suffix

**Solution:** Define naming conventions document and apply consistently.

---

### 13. Comment Cleanup

**Severity:** Low
**Type:** Documentation
**Effort:** 1 hour
**Status:** 💭 Idea

**Action:**
- Remove "CRITICAL FIX:" prefixes if solutions are permanent
- Convert to TODO with GitHub issue references if temporary
- Add missing @throws documentation

---

## Completed

*(Empty - track completed refactorings here)*

---

## Status Legend

- ⏳ **Pending** - Not started
- 🚧 **In Progress** - Currently being worked on
- ✅ **Done** - Completed and tested
- 📋 **Planned** - Scheduled for future sprint
- 💭 **Idea** - Under consideration
- ❌ **Rejected** - Not pursuing

---

## Test Requirements

All refactorings must:
1. Pass existing 1217 unit tests
2. Add tests for new functionality
3. Maintain backward compatibility (unless marked as breaking change)
4. Update documentation

---

## Review Checklist

Before merging refactoring:
- [ ] All tests pass (1217/1217)
- [ ] No new compiler warnings
- [ ] clang-tidy passes
- [ ] Documentation updated
- [ ] CHANGELOG.md entry added
- [ ] Code review completed

---

**Next Review:** After immediate fixes are completed
**Owner:** Development Team
**Last Updated:** 2025-11-08
