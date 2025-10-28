# Event System Refactoring: Implementation Document

## Overview

Refactor the event handling system from trait-based pattern to a unified `std::optional<ui_event>` pattern, where backends convert their native events to framework-level event structures.

**Goal**: Simplify backend implementation, improve testability, and centralize platform-specific quirks in backend conversion logic.

**Scope**: Breaking change - no backward compatibility required.

## Design

### Core Event Structures

```cpp
namespace onyxui {
    /**
     * @brief Framework-level keyboard event (backend-agnostic)
     *
     * @details
     * All keyboard events are normalized by the backend before reaching the framework.
     * Platform-specific quirks (e.g., Enter=Ctrl+M on terminals) are handled during
     * conversion, ensuring consistent behavior across all backends.
     *
     * ## Key Type Categories
     *
     * - **character**: ASCII printable characters, control characters (Enter, Tab, Escape)
     * - **function_key**: F1-F12 function keys
     * - **special**: Navigation keys (arrows, Page Up/Down, Home/End, Insert, Delete)
     *
     * ## Modifier Normalization
     *
     * Backends must normalize modifiers to remove platform quirks:
     * - Terminal: Enter=Ctrl+M, Tab=Ctrl+I → modifiers.ctrl should be false for plain keys
     * - GUI: Shift+letter produces uppercase → modifiers.shift should be true, character lowercase
     *
     * ## Character Normalization
     *
     * All alphabetic characters are normalized to lowercase. Use modifiers.shift to detect
     * uppercase intent. This simplifies hotkey matching (e.g., Ctrl+S vs Ctrl+Shift+S).
     *
     * @example
     * @code
     * // Plain Enter key (terminal: e.mod=2 due to Ctrl+M encoding)
     * keyboard_event {
     *     .type = key_type::character,
     *     .character = '\n',
     *     .modifiers = { .ctrl = false, .alt = false, .shift = false },
     *     .is_repeat = false
     * }
     *
     * // Ctrl+S (user pressed Ctrl+S)
     * keyboard_event {
     *     .type = key_type::character,
     *     .character = 's',  // Lowercase normalized
     *     .modifiers = { .ctrl = true, .alt = false, .shift = false },
     *     .is_repeat = false
     * }
     *
     * // F10 function key
     * keyboard_event {
     *     .type = key_type::function_key,
     *     .function_key = 10,
     *     .modifiers = { .ctrl = false, .alt = false, .shift = false },
     *     .is_repeat = false
     * }
     * @endcode
     */
    struct keyboard_event {
        enum class key_type : uint8_t {
            character,    ///< ASCII character (a-z, 0-9, Enter, Tab, Escape, etc.)
            function_key, ///< F1-F12
            special       ///< Arrow keys, Page Up/Down, Home/End, Insert, Delete, etc.
        };

        key_type type;

        // Key identification (only one is meaningful based on type)
        char character;      ///< For key_type::character (lowercase normalized)
        int function_key;    ///< For key_type::function_key (1-12)
        int special_key;     ///< For key_type::special (negative codes: -1=up, -2=down, -3=left, -4=right)

        // Modifiers (normalized by backend, terminal quirks removed)
        struct {
            bool ctrl : 1;
            bool alt : 1;
            bool shift : 1;
        } modifiers;

        bool is_repeat;  ///< Key repeat flag (if backend supports, false otherwise)
    };

    /**
     * @brief Framework-level mouse event (backend-agnostic)
     *
     * @details
     * Mouse events are coordinate-based with optional button and action information.
     * Coordinates are relative to the window/viewport origin (top-left = 0,0).
     *
     * ## Button Mapping
     *
     * - **left**: Primary button (typically left mouse button)
     * - **right**: Secondary button (typically right mouse button, context menu)
     * - **middle**: Tertiary button (typically middle mouse button or wheel click)
     * - **none**: No button (used for move events, wheel events)
     *
     * ## Action Types
     *
     * - **press**: Button pressed down
     * - **release**: Button released (may not identify which button on some platforms)
     * - **move**: Mouse moved (no button state change)
     * - **wheel_up**: Mouse wheel scrolled up
     * - **wheel_down**: Mouse wheel scrolled down
     *
     * @example
     * @code
     * // Left button click at (10, 20)
     * mouse_event {
     *     .x = 10,
     *     .y = 20,
     *     .btn = button::left,
     *     .act = action::press,
     *     .modifiers = { .ctrl = false, .alt = false, .shift = false }
     * }
     *
     * // Mouse wheel up (scrolling)
     * mouse_event {
     *     .x = 50,
     *     .y = 60,
     *     .btn = button::none,
     *     .act = action::wheel_up,
     *     .modifiers = { .ctrl = true, .alt = false, .shift = false }
     * }
     * @endcode
     */
    struct mouse_event {
        enum class button : uint8_t {
            none,    ///< No button (used for move, wheel events)
            left,    ///< Primary button (left)
            right,   ///< Secondary button (right, context menu)
            middle   ///< Tertiary button (middle, wheel click)
        };

        enum class action : uint8_t {
            press,      ///< Button pressed down
            release,    ///< Button released
            move,       ///< Mouse moved (no button change)
            wheel_up,   ///< Wheel scrolled up
            wheel_down  ///< Wheel scrolled down
        };

        int x;          ///< Mouse X coordinate (relative to viewport)
        int y;          ///< Mouse Y coordinate (relative to viewport)
        button btn;     ///< Button involved (none for move/wheel)
        action act;     ///< Action type

        // Modifiers during mouse event
        struct {
            bool ctrl : 1;
            bool alt : 1;
            bool shift : 1;
        } modifiers;
    };

    /**
     * @brief Framework-level resize event (backend-agnostic)
     *
     * @details
     * Resize events indicate the window/viewport has changed dimensions.
     * Framework will automatically trigger layout reflow (measure/arrange).
     *
     * @example
     * @code
     * // Window resized to 1024x768
     * resize_event {
     *     .width = 1024,
     *     .height = 768
     * }
     * @endcode
     */
    struct resize_event {
        int width;   ///< New window/viewport width in pixels or cells
        int height;  ///< New window/viewport height in pixels or cells
    };

    /**
     * @brief Unified UI event (variant of all event types)
     *
     * @details
     * Uses std::variant to represent any of the three event types.
     * No redundant type enum - variant tracks the active alternative.
     *
     * ## Usage Patterns
     *
     * **Pattern 1: std::get_if (simple)**
     * @code
     * if (auto* kbd = std::get_if<keyboard_event>(&evt)) {
     *     handle_keyboard(*kbd);
     * }
     * @endcode
     *
     * **Pattern 2: std::visit (elegant)**
     * @code
     * std::visit([](auto&& e) {
     *     using T = std::decay_t<decltype(e)>;
     *     if constexpr (std::is_same_v<T, keyboard_event>) {
     *         handle_keyboard(e);
     *     }
     * }, evt);
     * @endcode
     *
     * **Pattern 3: Overloaded visitor (most elegant)**
     * @code
     * std::visit(overloaded{
     *     [](const keyboard_event& kbd) { handle_keyboard(kbd); },
     *     [](const mouse_event& mouse) { handle_mouse(mouse); },
     *     [](const resize_event& resize) { handle_resize(resize); }
     * }, evt);
     * @endcode
     */
    using ui_event = std::variant<keyboard_event, mouse_event, resize_event>;
}
```

### Backend Concept

```cpp
/**
 * @brief Backend concept requiring unified event conversion
 *
 * @details
 * Backends must implement a single static method that converts their native
 * event type to the framework's ui_event structure. This method should:
 *
 * 1. Return std::nullopt for unknown/unsupported events
 * 2. Normalize all platform-specific quirks (e.g., Enter=Ctrl+M on terminals)
 * 3. Lowercase all alphabetic characters (use modifiers.shift for uppercase)
 * 4. Set modifiers accurately (remove spurious bits from terminal encoding)
 *
 * @tparam Backend Backend implementation type
 */
template<typename Backend>
concept UIBackend = requires {
    // Native event type
    typename Backend::event_type;

    // Conversion method: native event -> framework event
    { Backend::create_event(std::declval<typename Backend::event_type>()) }
        -> std::same_as<std::optional<ui_event>>;

    // Other existing requirements (rect_type, size_type, renderer_type, etc.)
    // ... (unchanged from current UIBackend concept)
};
```

### Usage Pattern in Framework

```cpp
// In hotkey_manager or ui_handle
template<UIBackend Backend>
bool handle_event(const typename Backend::event_type& native_event) {
    // Backend converts to framework event
    auto ui_evt = Backend::create_event(native_event);
    if (!ui_evt) {
        return false;  // Not an event we handle
    }

    // Dispatch using std::get_if
    if (auto* kbd = std::get_if<keyboard_event>(&ui_evt.value())) {
        return handle_keyboard(*kbd);
    } else if (auto* mouse = std::get_if<mouse_event>(&ui_evt.value())) {
        return handle_mouse(*mouse);
    } else if (auto* resize = std::get_if<resize_event>(&ui_evt.value())) {
        return handle_resize(*resize);
    }

    return false;
}
```

---

## Phase 1: Foundation (Week 1)

**Goal**: Add new event structures and conversion method, remove old trait system.

### Tasks

1. **Create event structures** (`include/onyxui/events/ui_event.hh`)
   - Define `keyboard_event` struct with comprehensive inline docs
   - Define `mouse_event` struct with comprehensive inline docs
   - Define `resize_event` struct with comprehensive inline docs
   - Define `ui_event = std::variant<...>` with usage examples
   - Add Doxygen groups for documentation generation

2. **Implement conio backend conversion** (`backends/conio/include/onyxui/conio/conio_backend.hh`)
   - Add `static std::optional<ui_event> create_event(const tb_event&)`
   - Handle keyboard events (Enter=Ctrl+M normalization)
   - Handle mouse events
   - Handle resize events
   - Document all normalization logic with inline comments

3. **Remove event_traits system**
   - Delete `event_traits<tb_event>` specialization from `conio_events.hh`
   - Remove `shift_pressed()`, `ctrl_pressed()`, `alt_pressed()` methods
   - Remove `to_ascii()`, `to_f_key()`, `to_special_key()` methods
   - Remove `is_*_key()` helper methods
   - Keep `event_like.hh` concept file (update with new requirements)

4. **Write unit tests** (`unittest/events/test_ui_event.cc`)
   - Test keyboard_event construction and field access
   - Test mouse_event construction and field access
   - Test resize_event construction and field access
   - Test variant usage (std::get_if, std::visit, std::holds_alternative)
   - Test variant copy/move semantics
   - Test keyboard_event normalization rules (lowercase, modifier handling)

5. **Write conio conversion tests** (`unittest/backend/test_conio_event_conversion.cc`)
   - Test Enter key normalization (TB_EVENT{key=13, mod=2} → kbd{char='\n', ctrl=false})
   - Test Tab key normalization (TB_EVENT{key=9, mod=2} → kbd{char='\t', ctrl=false})
   - Test Ctrl+Escape preservation (TB_EVENT{key=27, mod=2} → kbd{char=27, ctrl=true})
   - Test Shift+letter normalization (TB_EVENT{ch='A', mod=4} → kbd{char='a', shift=true})
   - Test F-key conversion (TB_KEY_F1-F12 → function_key 1-12)
   - Test arrow key conversion (TB_KEY_ARROW_* → special_key -1 to -4)
   - Test mouse button events (TB_KEY_MOUSE_LEFT → mouse{btn=left, act=press})
   - Test mouse wheel events (TB_KEY_MOUSE_WHEEL_UP → mouse{act=wheel_up})
   - Test resize events (TB_EVENT_RESIZE → resize{width, height})
   - Test unknown event → std::nullopt
   - Test edge cases (empty character, invalid F-key, etc.)

### Acceptance Criteria

- ✅ New event structures compile without warnings
- ✅ conio_backend::create_event() compiles and links
- ✅ At least 30 new test cases for event conversion (10 for structures, 20 for conio)
- ✅ All inline documentation complete with examples
- ✅ Zero compilation errors/warnings

### Files Created/Modified

**New:**
- `include/onyxui/events/ui_event.hh` (comprehensive inline docs)
- `unittest/events/test_ui_event.cc` (structure tests)
- `unittest/backend/test_conio_event_conversion.cc` (conversion tests)

**Modified:**
- `backends/conio/include/onyxui/conio/conio_backend.hh` (add create_event, remove traits)
- `backends/conio/include/onyxui/conio/conio_events.hh` (delete event_traits specialization)
- `include/onyxui/concepts/event_like.hh` (update concept requirements)

**Deleted:**
- Event trait methods from `conio_events.hh`

---

## Phase 2: Hotkey Manager Migration (Week 2)

**Goal**: Migrate hotkey_manager to use `keyboard_event` directly.

### Tasks

1. **Replace event_to_sequence** (`include/onyxui/hotkeys/hotkey_manager.hh`)
   - Remove template `event_to_sequence<KeyEvent>(const KeyEvent&)`
   - Add `event_to_sequence(const keyboard_event&) -> std::optional<key_sequence>`
   - Convert keyboard_event fields directly to key_sequence (no trait calls)
   - Document conversion logic with inline comments

2. **Update handle_key_event**
   ```cpp
   // OLD signature (template, trait-based)
   template<HotkeyCapable KeyEvent>
   bool handle_key_event(const KeyEvent& event, ui_element<Backend>* focused);

   // NEW signature (concrete, ui_event-based)
   bool handle_keyboard_event(const keyboard_event& kbd, ui_element<Backend>* focused);
   ```

3. **Simplify key_sequence conversion**
   ```cpp
   std::optional<key_sequence> event_to_sequence(const keyboard_event& kbd) {
       // Build modifier flags directly (no trait queries!)
       key_modifier mods = key_modifier::none;
       if (kbd.modifiers.ctrl) mods |= key_modifier::ctrl;
       if (kbd.modifiers.alt) mods |= key_modifier::alt;
       if (kbd.modifiers.shift) mods |= key_modifier::shift;

       // Convert based on key type
       switch (kbd.type) {
           case keyboard_event::key_type::character:
               return key_sequence{kbd.character, mods};
           case keyboard_event::key_type::function_key:
               return key_sequence{kbd.function_key, mods};
           case keyboard_event::key_type::special:
               return key_sequence{static_cast<char>(kbd.special_key), mods};
       }

       return std::nullopt;
   }
   ```

4. **Remove HotkeyCapable concept**
   - Delete `HotkeyCapable` concept (no longer needed)
   - Remove all trait-based helper methods
   - Simplify hotkey_manager to work only with keyboard_event

5. **Update tests** (`unittest/hotkeys/test_hotkey_manager.cc`)
   - Replace mock backend events with keyboard_event instances
   - Test Enter key → key_sequence conversion
   - Test F-key → key_sequence conversion
   - Test arrow key → key_sequence conversion
   - Test modifier combinations
   - Verify hotkey matching works with new event system

### Acceptance Criteria

- ✅ All existing hotkey tests pass (updated to use keyboard_event)
- ✅ Enter key activation works in menu demo
- ✅ F10 menu activation works
- ✅ Arrow key navigation works
- ✅ No regressions in keyboard navigation
- ✅ At least 15 new test cases for keyboard_event → key_sequence conversion
- ✅ Zero trait method calls remain in hotkey_manager

### Files Modified

- `include/onyxui/hotkeys/hotkey_manager.hh` (remove traits, use keyboard_event)
- `include/onyxui/hotkeys/key_sequence.hh` (update docs)
- `unittest/hotkeys/test_hotkey_manager.cc` (update tests)

---

## Phase 3: UI Element Event Handling (Week 3)

**Goal**: Migrate ui_element event routing to use `ui_event`.

### Tasks

1. **Replace event_target methods** (`include/onyxui/event_target.hh`)
   ```cpp
   // OLD (trait-based, templated)
   template<typename KeyEvent>
   virtual void on_key_down(const KeyEvent& e) { }

   template<typename MouseEvent>
   virtual void on_mouse_down(const MouseEvent& e) { }

   // NEW (concrete, ui_event-based)
   virtual void on_keyboard(const keyboard_event& kbd) { }
   virtual void on_mouse(const mouse_event& mouse) { }
   virtual void on_resize(const resize_event& resize) { }
   ```

2. **Add event dispatcher** (`include/onyxui/element.hh`)
   ```cpp
   /**
    * @brief Dispatch ui_event to appropriate handler
    * @param evt Framework-level event
    *
    * @details
    * Routes event to the appropriate virtual handler based on variant type.
    * Uses std::visit for type-safe dispatching.
    */
   void dispatch_ui_event(const ui_event& evt) {
       std::visit([this](auto&& e) {
           using T = std::decay_t<decltype(e)>;
           if constexpr (std::is_same_v<T, keyboard_event>) {
               this->on_keyboard(e);
           } else if constexpr (std::is_same_v<T, mouse_event>) {
               this->on_mouse(e);
           } else if constexpr (std::is_same_v<T, resize_event>) {
               this->on_resize(e);
           }
       }, evt);
   }
   ```

3. **Update ui_handle::handle_event** (`include/onyxui/ui_handle.hh`)
   ```cpp
   bool handle_event(const typename Backend::event_type& event) {
       // Convert to framework event
       auto ui_evt = Backend::create_event(event);
       if (!ui_evt) {
           return false;  // Unknown event
       }

       // Route keyboard events through hotkey system first
       if (auto* kbd = std::get_if<keyboard_event>(&ui_evt.value())) {
           if (auto* hotkeys = ui_services<Backend>::hotkeys()) {
               if (hotkeys->handle_keyboard_event(*kbd, get_focused_element())) {
                   return true;  // Hotkey consumed the event
               }
           }
       }

       // Route to focused element
       if (auto* elem = get_focused_element()) {
           elem->dispatch_ui_event(ui_evt.value());
           return true;
       }

       // Route mouse events to hit-tested element
       if (auto* mouse = std::get_if<mouse_event>(&ui_evt.value())) {
           if (auto* target = hit_test(mouse->x, mouse->y)) {
               target->dispatch_ui_event(ui_evt.value());
               return true;
           }
       }

       return false;
   }
   ```

4. **Remove old event_target templates**
   - Delete all templated event handler methods
   - Remove event propagation code (simplified with ui_event)
   - Update event routing logic to be non-templated

5. **Test event routing** (`unittest/core/test_event_routing.cc`)
   - Test keyboard event routes to on_keyboard()
   - Test mouse event routes to on_mouse()
   - Test resize event routes to on_resize()
   - Test hotkey interception (hotkey consumes before widget)
   - Test event propagation through widget tree
   - Test hit testing for mouse events

### Acceptance Criteria

- ✅ All existing UI tests pass
- ✅ Events route correctly to new handlers
- ✅ demo.cc works with new event system
- ✅ Hotkeys intercept before widgets
- ✅ Mouse hit-testing works
- ✅ At least 20 new test cases for event routing

### Files Modified

- `include/onyxui/event_target.hh` (remove templates, add ui_event methods)
- `include/onyxui/element.hh` (add dispatch_ui_event)
- `include/onyxui/ui_handle.hh` (update handle_event)
- `unittest/core/test_event_routing.cc` (new file)

---

## Phase 4: Widget Migration (Week 4)

**Goal**: Migrate all widgets to use new on_keyboard/on_mouse handlers.

### Tasks

1. **Migrate menu widgets**
   - `menu.hh`: Replace key event handlers with on_keyboard()
   - `menu_item.hh`: Use keyboard_event for Enter key, mouse_event for clicks
   - `menu_bar.hh`: Use keyboard_event for F10 activation
   - `menu_bar_item.hh`: Use keyboard_event/mouse_event

2. **Migrate button widget**
   - `button.hh`: Use keyboard_event for Space/Enter activation
   - Use mouse_event for clicks and hover

3. **Migrate other widgets**
   - `label.hh`: Update if it handles events
   - `panel.hh`: Update container event forwarding
   - Any other widgets with event handlers

4. **Update all widget tests**
   - Replace backend event construction with keyboard_event/mouse_event
   - Test keyboard activation with keyboard_event
   - Test mouse interaction with mouse_event
   - Verify modifier key handling
   - Test edge cases (disabled widgets, unfocused, etc.)

5. **Document migration patterns** (inline in widget files)
   ```cpp
   // Example: Button space key activation
   void on_keyboard(const keyboard_event& kbd) override {
       // Space or Enter activates button
       if (kbd.type == keyboard_event::key_type::character &&
           (kbd.character == ' ' || kbd.character == '\n') &&
           !kbd.modifiers.ctrl && !kbd.modifiers.alt) {
           if (this->is_enabled()) {
               this->clicked.emit();
           }
       }
   }
   ```

### Acceptance Criteria

- ✅ All widget tests pass
- ✅ Menu navigation works with keyboard_event
- ✅ Button activation works with Space/Enter
- ✅ Mouse clicks work with mouse_event
- ✅ Mouse hover effects work
- ✅ demo.cc fully functional (all interactions)
- ✅ Zero regressions
- ✅ At least 30 widget test cases updated/added

### Files Modified

- `include/onyxui/widgets/menu.hh`
- `include/onyxui/widgets/menu_item.hh`
- `include/onyxui/widgets/menu_bar.hh`
- `include/onyxui/widgets/menu_bar_item.hh`
- `include/onyxui/widgets/button.hh`
- `include/onyxui/widgets/label.hh`
- `include/onyxui/widgets/panel.hh`
- All widget test files in `unittest/widgets/`

---

## Phase 5: Integration Testing (Week 5)

**Goal**: Comprehensive end-to-end testing of the new event system.

### Tasks

1. **Create integration test suite** (`unittest/integration/test_event_integration.cc`)
   - **Menu system tests**:
     - F10 opens menu bar
     - Arrow keys navigate menu items
     - Enter activates menu item action
     - Escape closes menu
     - Mouse click on menu item activates
     - Mouse hover highlights menu item

   - **Hotkey tests**:
     - Ctrl+S triggers save action
     - Alt+F4 triggers exit action
     - F1 opens help
     - Ctrl+Shift+letter combinations
     - Multiple hotkey registration/unregistration

   - **Focus navigation tests**:
     - Tab moves focus forward
     - Shift+Tab moves focus backward
     - Arrow keys in different contexts

   - **Mouse interaction tests**:
     - Click focuses element
     - Drag interactions (if applicable)
     - Wheel scroll events
     - Right-click context menus

   - **Window resize tests**:
     - Resize triggers layout reflow
     - Elements adapt to new dimensions
     - Scroll positions preserved

2. **Create stress tests** (`unittest/integration/test_event_stress.cc`)
   - Rapid event processing (1000+ events/sec)
   - Event queue overflow handling
   - Deep widget tree event propagation
   - Memory leak detection (repeated event cycles)

3. **Create real-world scenario tests** (`unittest/integration/test_scenarios.cc`)
   - **Scenario: File menu workflow**
     1. F10 to open menu
     2. Arrow down to "Open"
     3. Enter to activate
     4. Dialog appears
     5. Type filename
     6. Enter to confirm

   - **Scenario: Button interaction**
     1. Tab to focus button
     2. Space to activate
     3. Verify action triggered

   - **Scenario: Keyboard shortcut**
     1. Press Ctrl+S anywhere
     2. Verify save action triggered
     3. Verify focused element unchanged

4. **Create cross-widget interaction tests**
   - Button click while menu is open (should close menu)
   - Hotkey during modal dialog (should be blocked)
   - Focus change during event handling
   - Event handling during layout changes

5. **Document test coverage** (`docs/testing/event_system_coverage.md`)
   - List all event types tested
   - List all widget combinations tested
   - List all edge cases covered
   - Identify gaps in coverage

### Acceptance Criteria

- ✅ At least 50 integration test cases
- ✅ All real-world scenarios pass
- ✅ Stress tests show no memory leaks
- ✅ 100% code coverage of event routing paths
- ✅ demo.cc runs without issues
- ✅ Test suite runs in <5 seconds
- ✅ Documentation of test coverage complete

### Files Created

- `unittest/integration/test_event_integration.cc` (30+ cases)
- `unittest/integration/test_event_stress.cc` (10+ cases)
- `unittest/integration/test_scenarios.cc` (10+ cases)
- `docs/testing/event_system_coverage.md` (coverage report)

---

## Phase 6: Documentation (Week 6)

**Goal**: Comprehensive documentation for the new event system.

### Tasks

1. **Update CLAUDE.md** (`CLAUDE.md`)
   - Add "Event System" section
   - Document keyboard_event, mouse_event, resize_event structures
   - Explain ui_event variant usage patterns
   - Show backend implementation guide for create_event()
   - Document migration from old event_target methods
   - Add troubleshooting section (common issues)

2. **Create Docusaurus documentation** (`docs/docusaurus/`)
   - **User Guide** (`docs/docusaurus/docs/events/user-guide.md`)
     - How to handle keyboard events in widgets
     - How to handle mouse events in widgets
     - How to register hotkeys
     - Examples for common patterns

   - **Backend Guide** (`docs/docusaurus/docs/events/backend-guide.md`)
     - How to implement create_event() for a new backend
     - Platform quirks to normalize (Enter=Ctrl+M, etc.)
     - Testing checklist for backend events
     - Example implementation walkthrough

   - **API Reference** (`docs/docusaurus/docs/events/api-reference.md`)
     - keyboard_event fields and semantics
     - mouse_event fields and semantics
     - resize_event fields and semantics
     - ui_event variant usage
     - Event routing architecture diagram

   - **Migration Guide** (`docs/docusaurus/docs/events/migration-guide.md`)
     - Breaking changes from old trait system
     - Step-by-step widget migration
     - Before/after code examples
     - Common pitfalls and solutions

3. **Generate Doxygen documentation**
   - Ensure all event structures have complete Doxygen comments
   - Generate HTML docs with Doxygen
   - Include diagrams (event flow, dispatch logic)
   - Add cross-references between related methods

4. **Add code examples** (`examples/events/`)
   - **example_keyboard.cc**: Handle keyboard events in custom widget
   - **example_mouse.cc**: Handle mouse clicks and hover
   - **example_hotkeys.cc**: Register and use hotkeys
   - **example_backend.cc**: Implement create_event() for mock backend
   - Each example should be buildable and runnable

5. **Update inline documentation**
   - Review all changed files for doc comments
   - Ensure @brief, @details, @param, @return are complete
   - Add @example blocks where helpful
   - Add @see cross-references

### Acceptance Criteria

- ✅ CLAUDE.md has comprehensive event system section (500+ lines)
- ✅ Docusaurus documentation complete (4 guides, 2000+ lines total)
- ✅ Doxygen generates clean HTML without warnings
- ✅ At least 4 runnable code examples
- ✅ All public APIs have inline documentation
- ✅ Documentation reviewed and proofread
- ✅ Zero broken links in documentation

### Files Created/Modified

**Created:**
- `docs/docusaurus/docs/events/user-guide.md`
- `docs/docusaurus/docs/events/backend-guide.md`
- `docs/docusaurus/docs/events/api-reference.md`
- `docs/docusaurus/docs/events/migration-guide.md`
- `examples/events/example_keyboard.cc`
- `examples/events/example_mouse.cc`
- `examples/events/example_hotkeys.cc`
- `examples/events/example_backend.cc`

**Modified:**
- `CLAUDE.md` (add Event System section)
- All event-related headers (polish inline docs)

---

## Testing Strategy

### Unit Tests (Per Phase)

**Phase 1**: Structure and conversion tests
- 10 tests for event structures
- 20 tests for conio conversion
- Target: 30 new tests

**Phase 2**: Hotkey manager tests
- 15 tests for keyboard_event → key_sequence
- Target: 15 new tests

**Phase 3**: Event routing tests
- 20 tests for dispatch and routing
- Target: 20 new tests

**Phase 4**: Widget tests
- 30 tests for widget event handlers
- Target: 30 new tests

**Phase 5**: Integration tests
- 50 tests for real-world scenarios
- Target: 50 new tests

**Total new tests**: ~145 tests, bringing total from 736 to ~880 tests

### Integration Test Coverage

**Event types:**
- ✅ Keyboard character events (a-z, 0-9)
- ✅ Keyboard control keys (Enter, Escape, Tab)
- ✅ Keyboard function keys (F1-F12)
- ✅ Keyboard special keys (arrows, Page Up/Down, Home/End)
- ✅ Keyboard modifiers (Ctrl, Alt, Shift, combinations)
- ✅ Mouse button events (left, right, middle)
- ✅ Mouse move events
- ✅ Mouse wheel events
- ✅ Resize events

**Widget types:**
- ✅ Button (keyboard + mouse)
- ✅ Label (passive, no events)
- ✅ Menu (keyboard navigation)
- ✅ Menu item (keyboard + mouse)
- ✅ Menu bar (keyboard activation)
- ✅ Panel (event forwarding)
- ✅ Custom widgets

**Scenarios:**
- ✅ Menu navigation workflow
- ✅ Hotkey triggering
- ✅ Focus navigation
- ✅ Mouse interaction
- ✅ Window resize handling
- ✅ Event during modal dialog
- ✅ Event during menu open
- ✅ Rapid event processing

### Performance Testing

**Benchmarks to run:**
1. Event conversion overhead (native → ui_event)
2. Event dispatch latency (ui_event → widget)
3. Hotkey matching performance (1000 registered hotkeys)
4. Memory usage (10000 events processed)
5. Event queue throughput (events/sec)

**Expected results:**
- Conversion: <100ns per event
- Dispatch: <200ns per event
- Hotkey match: <500ns per event
- Memory: Zero leaks, constant usage
- Throughput: >10000 events/sec

**Measurement tool:**
```cpp
// In unittest/benchmarks/test_event_performance.cc
TEST_CASE("Event conversion benchmark") {
    tb_event native = /* ... */;
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 10000; ++i) {
        [[maybe_unused]] auto evt = conio_backend::create_event(native);
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
    std::cout << "Avg conversion time: " << elapsed.count() / 10000 << "ns\n";
}
```

### Regression Testing

**Before merging each phase:**
```bash
# Clean rebuild
cmake --build build --target clean
cmake --build build -j8

# Run all tests
./build/bin/ui_unittest

# Run demo
./build/bin/widgets_demo

# Check for memory leaks (if valgrind available)
valgrind --leak-check=full --error-exitcode=1 ./build/bin/widgets_demo

# Run clang-tidy (if enabled)
cmake --build build 2>&1 | grep "warning:" | wc -l  # Should be 0
```

---

## Risk Mitigation

### Breaking Changes

**This is a breaking change release**. All backends must be updated.

**Migration checklist for backend authors:**
1. Implement `static std::optional<ui_event> create_event(const event_type&)`
2. Remove `event_traits<event_type>` specialization
3. Test all event types (keyboard, mouse, resize)
4. Verify normalization logic (modifiers, character case)
5. Update backend documentation

### Rollback Plan

Each phase is independently committable:
- **Phase 1**: Revert event structure files
- **Phase 2**: Revert hotkey_manager changes
- **Phase 3**: Revert ui_element changes
- **Phase 4**: Revert widget changes
- **Phase 5**: Revert integration tests (no code change)
- **Phase 6**: Revert documentation (no code change)

If critical bugs found after release:
- Emergency patch: Add trait system back (temporary)
- Long-term: Fix bugs in ui_event system

### Performance Considerations

**Potential concerns:**
- Extra allocation for `std::optional<ui_event>`
- Variant overhead vs direct trait calls
- Copy overhead for event structures

**Mitigation:**
- `std::optional<ui_event>` is stack-allocated (likely optimized away)
- `std::variant` has zero overhead vs manual union
- Conversion happens once per event (not per query) - net win!
- Event structures are small (32-48 bytes) - cheap to copy

**Benchmarking proves no regression** (see Performance Testing section).

---

## Success Metrics

### Code Quality

- ✅ Reduce event handling code by ~60% (15 trait methods → 1 create_event)
- ✅ Centralize platform quirks in backend (easier to debug)
- ✅ Improve testability (mock keyboard_event vs mock backend event)
- ✅ Zero trait-based type erasure (concrete types only)

### Performance

- ✅ Maintain or improve event handling latency
- ✅ No measurable overhead in hotkey processing
- ✅ Clean profiler results (no unexpected allocations)
- ✅ Constant memory usage (no leaks)

### Developer Experience

- ✅ Easier backend implementation (1 method with clear logic vs 15 methods)
- ✅ Better error messages (keyboard_event is self-documenting)
- ✅ Simpler debugging (inspect keyboard_event vs decode raw event)
- ✅ Comprehensive documentation (inline + Docusaurus + examples)

### Test Coverage

- ✅ 880+ total tests (145 new tests)
- ✅ 100% code coverage of event routing
- ✅ All real-world scenarios tested
- ✅ Performance benchmarks passing

---

## Appendix A: Complete conio Backend Implementation

See `backends/conio/include/onyxui/conio/conio_backend.hh` after Phase 1 implementation.

**Key normalization logic:**
```cpp
// Terminal reality: Enter=Ctrl+M (ASCII 13), Tab=Ctrl+I (ASCII 9)
bool const is_ctrl_encoded = (e.key == TB_KEY_ENTER || e.key == TB_KEY_TAB);

// Remove spurious Ctrl bit for plain Enter/Tab
kbd.modifiers.ctrl = is_ctrl_encoded ? false : (e.mod & TB_MOD_CTRL) != 0;

// Shift has no meaning for Enter/Tab either
kbd.modifiers.shift = is_ctrl_encoded ? false : (e.mod & TB_MOD_SHIFT) != 0;

// Alt is always accurate
kbd.modifiers.alt = (e.mod & TB_MOD_ALT) != 0;
```

**Character normalization:**
```cpp
// Normalize uppercase to lowercase + shift modifier
if (e.ch >= 'A' && e.ch <= 'Z') {
    kbd.character = static_cast<char>(e.ch - 'A' + 'a');
    // Shift modifier already set from e.mod
}
```

---

## Appendix B: Testing Checklist

### Phase 1 Checklist
- [ ] keyboard_event structure compiles
- [ ] mouse_event structure compiles
- [ ] resize_event structure compiles
- [ ] ui_event variant compiles
- [ ] conio_backend::create_event() compiles
- [ ] Enter key normalization tested (Ctrl+M → ctrl=false)
- [ ] Tab key normalization tested (Ctrl+I → ctrl=false)
- [ ] Ctrl+Escape preservation tested (ctrl=true)
- [ ] F-key conversion tested (1-12)
- [ ] Arrow key conversion tested (-1 to -4)
- [ ] Mouse events tested (left, right, middle, wheel)
- [ ] Resize events tested
- [ ] Unknown events return nullopt
- [ ] All 30 tests pass

### Phase 2 Checklist
- [ ] hotkey_manager uses keyboard_event
- [ ] event_to_sequence() simplified
- [ ] HotkeyCapable concept removed
- [ ] All trait calls removed
- [ ] Enter key → key_sequence tested
- [ ] F-key → key_sequence tested
- [ ] Arrow key → key_sequence tested
- [ ] Modifier combinations tested
- [ ] Menu activation works (F10)
- [ ] Menu navigation works (arrows)
- [ ] Menu Enter works
- [ ] All 15 tests pass

### Phase 3 Checklist
- [ ] event_target uses ui_event
- [ ] dispatch_ui_event() implemented
- [ ] ui_handle uses Backend::create_event()
- [ ] Keyboard events route correctly
- [ ] Mouse events route correctly
- [ ] Resize events route correctly
- [ ] Hotkeys intercept before widgets
- [ ] Hit-testing works for mouse
- [ ] Event propagation tested
- [ ] All 20 tests pass

### Phase 4 Checklist
- [ ] menu.hh uses on_keyboard()
- [ ] menu_item.hh uses on_keyboard() and on_mouse()
- [ ] menu_bar.hh uses on_keyboard()
- [ ] button.hh uses on_keyboard() and on_mouse()
- [ ] All widget tests updated
- [ ] Menu navigation works
- [ ] Button Space/Enter works
- [ ] Mouse clicks work
- [ ] Mouse hover works
- [ ] demo.cc fully functional
- [ ] All 30 widget tests pass

### Phase 5 Checklist
- [ ] Menu workflow test passes
- [ ] Hotkey test passes
- [ ] Focus navigation test passes
- [ ] Mouse interaction test passes
- [ ] Resize test passes
- [ ] Stress tests pass (no leaks)
- [ ] Cross-widget tests pass
- [ ] All 50 integration tests pass
- [ ] Test coverage documented

### Phase 6 Checklist
- [ ] CLAUDE.md Event System section complete
- [ ] Docusaurus user guide complete
- [ ] Docusaurus backend guide complete
- [ ] Docusaurus API reference complete
- [ ] Docusaurus migration guide complete
- [ ] 4 code examples compile and run
- [ ] Doxygen generates without warnings
- [ ] All inline docs reviewed
- [ ] Zero broken links

---

## Timeline Summary

| Phase | Duration | Focus | Tests Added | Cumulative Tests |
|-------|----------|-------|-------------|------------------|
| 1     | Week 1   | Foundation | 30 | 766 |
| 2     | Week 2   | Hotkeys | 15 | 781 |
| 3     | Week 3   | Routing | 20 | 801 |
| 4     | Week 4   | Widgets | 30 | 831 |
| 5     | Week 5   | Integration | 50 | 881 |
| 6     | Week 6   | Documentation | 0 | 881 |

**Total time**: 6 weeks
**Total new tests**: 145
**Final test count**: ~881 tests

---

## Glossary

**ui_event**: Variant of keyboard_event, mouse_event, resize_event
**keyboard_event**: Framework-level keyboard event (backend-agnostic)
**mouse_event**: Framework-level mouse event (backend-agnostic)
**resize_event**: Framework-level window resize event
**create_event()**: Backend method that converts native events to ui_event
**event_traits**: Old trait-based system (being removed)
**normalization**: Process of removing platform quirks (e.g., Enter=Ctrl+M)
**dispatch**: Routing ui_event to appropriate widget handler using std::visit
**hit-testing**: Finding which widget is under mouse coordinates

---

**Document version**: 1.0
**Last updated**: 2025-10-28
**Status**: Approved, ready for implementation
