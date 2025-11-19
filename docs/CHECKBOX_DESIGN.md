# Checkbox Widget Design Document

**Widget**: `onyxui::checkbox`
**Phase**: 2 (Week 2, Part 1)
**Status**: Design Phase
**Created**: 2025-11-19
**Dependencies**: None (standalone input widget)

---

## Table of Contents

1. [Overview](#overview)
2. [Requirements](#requirements)
3. [API Design](#api-design)
4. [Visual Design](#visual-design)
5. [State Management](#state-management)
6. [Event Handling](#event-handling)
7. [Theme Integration](#theme-integration)
8. [Focus Integration](#focus-integration)
9. [Layout Behavior](#layout-behavior)
10. [Testing Strategy](#testing-strategy)
11. [Implementation Plan](#implementation-plan)
12. [Open Questions](#open-questions)

---

## Overview

### Purpose

The `checkbox` widget provides a toggleable boolean input control with optional tri-state support (unchecked/checked/indeterminate). It's essential for:
- Preference panels (enable/disable features)
- Form inputs (accept terms, remember me)
- Hierarchical selections (select all with partial state)
- Settings dialogs

### Key Features

- **Two-state mode** (default): Unchecked ⟷ Checked
- **Tri-state mode** (optional): Unchecked ⟷ Indeterminate ⟷ Checked
- **Keyboard interaction**: Space key toggles, Enter activates
- **Mouse interaction**: Click anywhere on widget (box + label) toggles
- **Text label**: Optional text with mnemonic support (Alt+key)
- **Signals**: `toggled(bool)` and `state_changed(tri_state)`
- **Theme integration**: Colors, box characters from theme
- **Focus support**: Focusable, visual focus indicator

### Visual Appearance

```
Normal state (unchecked):
[ ] Enable notifications

Checked state:
[✓] Enable notifications

Indeterminate state (tri-state only):
[▪] Select all items (some selected)

Focused:
[✓] Enable notifications  ← Focus indicator (border/background change)
    ^^^^^^^^^^^^^^^^^^^^

Disabled:
[ ] Cannot toggle (grayed out)
```

---

## Requirements

### Functional Requirements

1. **FR1**: Toggle between unchecked and checked states via mouse click
2. **FR2**: Toggle between states via Space key when focused
3. **FR3**: Support tri-state mode (unchecked/indeterminate/checked)
4. **FR4**: Display text label next to checkbox
5. **FR5**: Support mnemonic shortcuts (Alt+key)
6. **FR6**: Emit `toggled(bool)` signal on state change
7. **FR7**: Emit `state_changed(tri_state)` signal on any state change
8. **FR8**: Support disabled state (no interaction)
9. **FR9**: Request focus on mouse click
10. **FR10**: Visual focus indicator when focused

### Non-Functional Requirements

1. **NFR1**: Theme-based appearance (colors, box characters)
2. **NFR2**: Accessible keyboard navigation
3. **NFR3**: Consistent with OnyxUI architecture (Backend pattern, render context)
4. **NFR4**: Comprehensive test coverage (>90%)
5. **NFR5**: Zero compiler warnings
6. **NFR6**: Performance: O(1) state toggle, efficient rendering

### Out of Scope (Future Enhancements)

- **Icon support** (custom checkbox icons)
- **Checkbox groups** (like radio button groups, but allowing multiple selections)
- **Animated transitions** (smooth check/uncheck animation)
- **Custom rendering delegates** (fully custom checkbox appearance)

---

## API Design

### Enumerations

```cpp
/// Checkbox state enumeration
enum class tri_state : std::uint8_t {
    unchecked = 0,      ///< Box is empty
    checked = 1,        ///< Box has checkmark
    indeterminate = 2   ///< Box has partial indicator (tri-state mode only)
};
```

### Class Declaration

```cpp
template<UIBackend Backend>
class checkbox : public widget<Backend> {
public:
    // ===== Construction =====

    /// Create checkbox with optional text label
    explicit checkbox(std::string text = "", ui_element<Backend>* parent = nullptr);

    /// Create checkbox with initial state
    checkbox(std::string text, bool checked, ui_element<Backend>* parent = nullptr);

    // ===== State Management =====

    /// Set checked state (two-state mode)
    void set_checked(bool checked);

    /// Get checked state (true if state == tri_state::checked)
    [[nodiscard]] bool is_checked() const noexcept { return m_state == tri_state::checked; }

    /// Set tri-state value
    void set_tri_state(tri_state state);

    /// Get current tri-state value
    [[nodiscard]] tri_state get_tri_state() const noexcept { return m_state; }

    /// Enable/disable tri-state mode
    /// When disabled, indeterminate state is not allowed
    void set_tri_state_enabled(bool enabled);

    /// Check if tri-state mode is enabled
    [[nodiscard]] bool is_tri_state_enabled() const noexcept { return m_tri_state_enabled; }

    // ===== Text Label =====

    /// Set label text (appears to right of checkbox)
    void set_text(const std::string& text);

    /// Get label text
    [[nodiscard]] const std::string& text() const noexcept { return m_text; }

    // ===== Mnemonic Support =====

    /// Set mnemonic character (Alt+key shortcut)
    /// Example: set_mnemonic('E') → Alt+E toggles checkbox
    void set_mnemonic(char key);

    /// Get mnemonic character ('\0' if none)
    [[nodiscard]] char mnemonic() const noexcept { return m_mnemonic; }

    // ===== Enabled/Disabled =====

    /// Enable/disable checkbox (inherits from widget)
    // void set_enabled(bool enabled);  // Already in widget<Backend>

    /// Check if enabled (inherits from widget)
    // [[nodiscard]] bool is_enabled() const noexcept;  // Already in widget<Backend>

    // ===== Signals =====

    /// Emitted when checkbox toggles between unchecked/checked (two-state logic)
    /// Parameter: true if checked, false if unchecked
    /// Note: NOT emitted when entering indeterminate state
    signal<bool> toggled;

    /// Emitted on ANY state change (including indeterminate)
    /// Parameter: new tri_state value
    signal<tri_state> state_changed;

protected:
    // ===== Rendering =====

    void do_render(render_context<Backend>& ctx) const override;

    // ===== Event Handling =====

    bool handle_event(const ui_event& event, event_phase phase) override;

    /// Handle semantic action (Space key to toggle)
    bool handle_semantic_action(hotkey_action action) override;

    // ===== Theme Accessors =====

    [[nodiscard]] typename Backend::color_type get_theme_background_color() const override;
    [[nodiscard]] typename Backend::color_type get_theme_foreground_color() const override;

private:
    // ===== State =====

    std::string m_text;                     ///< Label text
    tri_state m_state = tri_state::unchecked;  ///< Current state
    bool m_tri_state_enabled = false;       ///< Allow indeterminate state?
    char m_mnemonic = '\0';                 ///< Mnemonic character (or '\0')

    // ===== Internal Helpers =====

    /// Toggle to next state (unchecked → checked → unchecked, or tri-state cycle)
    void toggle();

    /// Get checkbox box character for current state from theme
    [[nodiscard]] std::string get_box_character() const;

    /// Calculate text bounds for mnemonic underline
    [[nodiscard]] typename Backend::rect_type get_text_bounds() const;
};
```

### Usage Examples

```cpp
// Example 1: Simple checkbox (two-state)
auto remember_me = std::make_unique<checkbox<Backend>>("Remember me");
remember_me->toggled.connect([](bool checked) {
    std::cout << "Remember: " << (checked ? "Yes" : "No") << "\n";
});

// Example 2: Checkbox with initial state
auto agree = std::make_unique<checkbox<Backend>>("I agree to terms", false);
agree->toggled.connect([&](bool checked) {
    submit_button->set_enabled(checked);  // Enable submit only if agreed
});

// Example 3: Tri-state checkbox (select all)
auto select_all = std::make_unique<checkbox<Backend>>("Select All");
select_all->set_tri_state_enabled(true);
select_all->state_changed.connect([](tri_state state) {
    switch (state) {
        case tri_state::unchecked:
            std::cout << "None selected\n";
            break;
        case tri_state::checked:
            std::cout << "All selected\n";
            break;
        case tri_state::indeterminate:
            std::cout << "Some selected\n";
            break;
    }
});

// Example 4: Checkbox with mnemonic (Alt+E toggles)
auto enable_feature = std::make_unique<checkbox<Backend>>("&Enable feature");
enable_feature->set_mnemonic('e');  // Alt+E toggles

// Example 5: Disabled checkbox (read-only state display)
auto readonly_option = std::make_unique<checkbox<Backend>>("Feature enabled", true);
readonly_option->set_enabled(false);
```

---

## Visual Design

### Box Characters (Theme-Configurable)

**Two-state mode:**
- Unchecked: `[ ]` or `☐`
- Checked: `[✓]` or `☑`

**Tri-state mode:**
- Unchecked: `[ ]` or `☐`
- Indeterminate: `[▪]` or `☒`
- Checked: `[✓]` or `☑`

**Terminal rendering** (conio backend):
```
Unchecked:  [ ] Label text
Checked:    [X] Label text  or  [✓] Label text
Indeterminate: [-] Label text  or  [▪] Label text
```

### Layout Structure

```
┌─────────────────────────────┐
│ [X] Label text with focus   │
│ ^^^                         │
│  │                          │
│  └─ Box (3 chars wide)      │
│     Followed by space + text│
└─────────────────────────────┘

Measurements:
- Box width: 3 characters  ("[X]")
- Spacing: 1 space between box and text
- Total min width: 3 (box) + 1 (space) + text_width
- Height: 1 line (single-line widget)
```

### Focus Indicator

When focused, the checkbox shows a visual indicator:
- **Option 1**: Change background color of entire widget
- **Option 2**: Add border around widget
- **Option 3**: Change box color only

**Recommendation**: Change background color (consistent with button focus).

### Disabled State

When disabled:
- Box and text rendered in theme's disabled color (grayed out)
- No hover effect
- No click/keyboard interaction

---

## State Management

### State Transition Diagrams

**Two-state mode** (`tri_state_enabled = false`):
```
         toggle()
    ┌──────────────┐
    │              │
    ▼              │
Unchecked ──────> Checked
            toggle()
```

**Tri-state mode** (`tri_state_enabled = true`):
```
Option A (User cycle):
Unchecked → Checked → Unchecked
            (skips indeterminate)

Option B (Programmatic):
set_tri_state(indeterminate) → Indeterminate
```

**Question**: Should user clicks cycle through all 3 states, or skip indeterminate?

**Recommendation**: User clicks skip indeterminate. Only programmatic `set_tri_state()` can set indeterminate.
This matches Qt/GTK behavior where tri-state is used for "select all" logic controlled by code.

### State Change Logic

```cpp
void checkbox::toggle() {
    if (!is_enabled()) {
        return;  // No-op if disabled
    }

    if (m_tri_state_enabled && m_state == tri_state::indeterminate) {
        // Indeterminate → Checked (user click exits indeterminate)
        set_tri_state(tri_state::checked);
    } else if (m_state == tri_state::unchecked) {
        // Unchecked → Checked
        set_checked(true);
    } else {
        // Checked → Unchecked
        set_checked(false);
    }
}

void checkbox::set_checked(bool checked) {
    tri_state new_state = checked ? tri_state::checked : tri_state::unchecked;
    if (m_state == new_state) {
        return;  // No change
    }

    m_state = new_state;
    mark_dirty();

    // Emit signals
    toggled.emit(checked);
    state_changed.emit(m_state);
}

void checkbox::set_tri_state(tri_state state) {
    // Validate state
    if (!m_tri_state_enabled && state == tri_state::indeterminate) {
        // Cannot set indeterminate if tri-state disabled
        return;  // Or throw exception?
    }

    if (m_state == state) {
        return;  // No change
    }

    tri_state old_state = m_state;
    m_state = state;
    mark_dirty();

    // Emit state_changed always
    state_changed.emit(m_state);

    // Emit toggled only for unchecked ⟷ checked transitions
    if ((old_state == tri_state::unchecked && state == tri_state::checked) ||
        (old_state == tri_state::checked && state == tri_state::unchecked)) {
        toggled.emit(state == tri_state::checked);
    }
}
```

---

## Event Handling

### Mouse Events

```cpp
bool checkbox::handle_event(const ui_event& event, event_phase phase) {
    if (phase != event_phase::target) {
        return widget<Backend>::handle_event(event, phase);
    }

    // Mouse click toggles checkbox
    if (event.type == ui_event::event_type::mouse_press) {
        if (!is_enabled()) {
            return false;
        }

        // Request focus
        request_focus();

        // Toggle state
        toggle();

        return true;  // Event handled
    }

    return widget<Backend>::handle_event(event, phase);
}
```

### Keyboard Events (Semantic Actions)

The checkbox responds to semantic actions:

**New semantic actions needed**:
```cpp
enum class hotkey_action : std::uint8_t {
    // ... existing actions ...

    // Widget activation
    activate_widget,  // Space key or Enter → toggle checkbox/button
};
```

**Implementation**:
```cpp
bool checkbox::handle_semantic_action(hotkey_action action) {
    switch (action) {
        case hotkey_action::activate_widget:
            // Space key or Enter toggles checkbox
            if (is_enabled()) {
                toggle();
                return true;
            }
            return false;

        default:
            return widget<Backend>::handle_semantic_action(action);
    }
}
```

**Hotkey scheme binding** (Windows):
```cpp
scheme.set_binding(hotkey_action::activate_widget, parse_key_sequence("Space"));
// Note: Enter should NOT toggle checkbox (only Space)
// Enter is reserved for "submit form" action
```

**Question**: Should Enter key toggle checkbox, or only Space?

**Recommendation**: Only Space toggles. Enter submits form (default button).

### Mnemonic Support

Mnemonics are handled by the mnemonic system:
```cpp
// User types Alt+E
// → mnemonic_manager finds checkbox with mnemonic 'e'
// → calls checkbox->toggle() or checkbox->request_focus()

// Question: Should Alt+key toggle immediately, or just focus?
// Option A: Alt+E toggles checkbox immediately
// Option B: Alt+E focuses checkbox, then user presses Space to toggle
```

**Recommendation**: Alt+key toggles immediately (matches Windows/GTK).

---

## Theme Integration

### Theme Properties

Add to `theme.hh` and theme YAML files:

```yaml
checkbox:
  # Colors
  background: "#000080"              # Background when unfocused
  background_focused: "#0000FF"      # Background when focused
  background_disabled: "#000040"     # Background when disabled

  text: "#FFFFFF"                    # Label text color
  text_disabled: "#808080"           # Label text when disabled

  box_unchecked: "#FFFFFF"           # Color of [ ] box
  box_checked: "#FFFF00"             # Color of [X] box
  box_indeterminate: "#00FFFF"       # Color of [-] box
  box_disabled: "#808080"            # Box color when disabled

  # Box characters (can be customized per theme)
  box_unchecked_icon: "[ ]"          # Unchecked box
  box_checked_icon: "[✓]"            # Checked box
  box_indeterminate_icon: "[▪]"      # Indeterminate box

  # Alternative box characters for terminals without Unicode:
  # box_unchecked_icon: "[ ]"
  # box_checked_icon: "[X]"
  # box_indeterminate_icon: "[-]"
```

### Rendering Implementation

```cpp
void checkbox::do_render(render_context<Backend>& ctx) const {
    const auto& bounds = this->bounds();

    // Get resolved style from theme
    auto bg_color = get_theme_background_color();
    auto text_color = get_theme_foreground_color();

    // Draw background
    ctx.fill_rect(bounds, bg_color);

    // Draw checkbox box (first 3 characters)
    std::string box_char = get_box_character();
    auto box_color = is_enabled() ? get_box_color() : theme.get_color("checkbox.box_disabled");

    const int x = rect_utils::get_x(bounds);
    const int y = rect_utils::get_y(bounds);

    // Draw box characters
    ctx.draw_text(box_char, {x, y}, default_font, box_color);

    // Draw label text (if any)
    if (!m_text.empty()) {
        const int text_x = x + 4;  // After "[X] " (3 chars + 1 space)

        // Check if mnemonic underlining is needed
        if (m_mnemonic != '\0') {
            // Use styled_text for mnemonic rendering
            auto styled = styled_text::from_mnemonic_string(m_text, m_mnemonic);
            ctx.draw_styled_text(styled, {text_x, y}, default_font, text_color);
        } else {
            ctx.draw_text(m_text, {text_x, y}, default_font, text_color);
        }
    }

    // Draw focus indicator if focused
    if (has_focus()) {
        // Option: Draw focus border or just rely on background_focused color
    }
}

std::string checkbox::get_box_character() const {
    const auto& theme = this->get_theme();

    switch (m_state) {
        case tri_state::unchecked:
            return theme.get_string("checkbox.box_unchecked_icon", "[ ]");
        case tri_state::checked:
            return theme.get_string("checkbox.box_checked_icon", "[✓]");
        case tri_state::indeterminate:
            return theme.get_string("checkbox.box_indeterminate_icon", "[▪]");
    }

    return "[ ]";  // Fallback
}

typename Backend::color_type checkbox::get_box_color() const {
    const auto& theme = this->get_theme();

    if (!is_enabled()) {
        return theme.get_color("checkbox.box_disabled");
    }

    switch (m_state) {
        case tri_state::unchecked:
            return theme.get_color("checkbox.box_unchecked");
        case tri_state::checked:
            return theme.get_color("checkbox.box_checked");
        case tri_state::indeterminate:
            return theme.get_color("checkbox.box_indeterminate");
    }

    return theme.get_color("checkbox.box_unchecked");
}
```

---

## Focus Integration

### Focusable Widget

Checkbox is focusable by default:

```cpp
checkbox::checkbox(std::string text, ui_element<Backend>* parent)
    : widget<Backend>(parent)
    , m_text(std::move(text))
{
    // Checkboxes are focusable
    set_focusable(true);
}
```

### Focus Behavior

- **Click**: Requests focus
- **Tab**: Navigates to next focusable widget
- **Shift+Tab**: Navigates to previous focusable widget
- **Alt+Mnemonic**: Focuses (or toggles) checkbox

### Visual Focus Indicator

When focused, checkbox shows `background_focused` color from theme.

---

## Layout Behavior

### Measurement (Measure Pass)

```cpp
typename Backend::size_type checkbox::measure_impl(
    const measure_context<Backend>& ctx,
    int available_width,
    int available_height
) const {
    // Measure text
    int text_width = 0;
    if (!m_text.empty()) {
        auto text_size = renderer_type::measure_text(m_text, default_font);
        text_width = size_utils::get_width(text_size);
    }

    // Total width: box (3) + space (1) + text
    const int total_width = 3 + 1 + text_width;

    // Height: always 1 line
    const int total_height = 1;

    return size_utils::make_size(total_width, total_height);
}
```

### Minimum Size

- **Width**: 3 characters (just the box, if no text)
- **Height**: 1 line

### Preferred Size

- **Width**: Box (3) + Space (1) + Text width
- **Height**: 1 line

### Maximum Size

- **Width**: Unlimited (text can be long)
- **Height**: 1 line (checkbox is single-line only)

---

## Testing Strategy

### Unit Tests

**File**: `unittest/widgets/test_checkbox.cc`

**Test cases** (minimum 15 tests):

1. **Construction**
   - Default construction (unchecked, empty text)
   - Construction with text
   - Construction with text and initial state

2. **State Management**
   - `set_checked(true)` → `is_checked()` returns true
   - `set_checked(false)` → `is_checked()` returns false
   - `toggle()` cycles unchecked ⟷ checked
   - Tri-state enabled: `set_tri_state(indeterminate)`
   - Tri-state disabled: `set_tri_state(indeterminate)` fails/no-op

3. **Signals**
   - `toggled` emitted on `set_checked(true)`
   - `toggled` emitted on `set_checked(false)`
   - `state_changed` emitted on all state changes
   - `toggled` NOT emitted when entering indeterminate

4. **Event Handling**
   - Mouse click toggles state
   - Space key toggles state
   - Events ignored when disabled

5. **Tri-State Mode**
   - User toggle skips indeterminate
   - Programmatic `set_tri_state()` allows indeterminate
   - Indeterminate → Checked on user toggle

6. **Disabled State**
   - `set_enabled(false)` prevents toggling
   - Events ignored when disabled

7. **Text and Mnemonic**
   - `set_text()` updates label
   - `set_mnemonic()` stores mnemonic character

8. **Layout**
   - Measure returns correct size
   - Minimum size is 3 characters wide, 1 line tall

9. **Focus**
   - Mouse click requests focus
   - `is_focusable()` returns true

10. **Theme Integration**
    - `get_theme_background_color()` returns correct color
    - Box character changes based on state

### Integration Tests

Test checkbox in realistic scenarios:

```cpp
TEST_CASE("Preferences dialog with multiple checkboxes") {
    ui_context_fixture<test_canvas_backend> fixture;

    auto dialog = std::make_unique<vbox<test_canvas_backend>>();

    auto* notifications = dialog->template emplace_child<checkbox<test_canvas_backend>>(
        "Enable notifications"
    );
    auto* auto_save = dialog->template emplace_child<checkbox<test_canvas_backend>>(
        "Auto-save documents"
    );
    auto* spell_check = dialog->template emplace_child<checkbox<test_canvas_backend>>(
        "Enable spell check"
    );

    // Test independent toggling
    notifications->set_checked(true);
    auto_save->set_checked(true);
    spell_check->set_checked(false);

    REQUIRE(notifications->is_checked());
    REQUIRE(auto_save->is_checked());
    REQUIRE_FALSE(spell_check->is_checked());
}

TEST_CASE("Select all tri-state checkbox") {
    ui_context_fixture<test_canvas_backend> fixture;

    auto select_all = std::make_unique<checkbox<test_canvas_backend>>("Select All");
    select_all->set_tri_state_enabled(true);

    // Simulate: 3 items total, 0 selected → unchecked
    select_all->set_tri_state(tri_state::unchecked);
    REQUIRE(select_all->get_tri_state() == tri_state::unchecked);

    // User checks select_all → all items selected
    select_all->set_checked(true);
    REQUIRE(select_all->is_checked());

    // One item unchecked → indeterminate
    select_all->set_tri_state(tri_state::indeterminate);
    REQUIRE(select_all->get_tri_state() == tri_state::indeterminate);

    // User clicks select_all while indeterminate → checked (selects all)
    select_all->toggle();
    REQUIRE(select_all->is_checked());
}
```

---

## Implementation Plan

### Step 1: Core Structure (Day 1, Morning)

1. Create `include/onyxui/widgets/input/checkbox.hh`
2. Define `tri_state` enum
3. Define `checkbox` class skeleton
4. Add member variables
5. Implement constructor

### Step 2: State Management (Day 1, Afternoon)

1. Implement `set_checked()`, `is_checked()`
2. Implement `set_tri_state()`, `get_tri_state()`
3. Implement `toggle()` logic
4. Add signal declarations
5. Emit signals on state changes

### Step 3: Event Handling (Day 2, Morning)

1. Implement `handle_event()` for mouse clicks
2. Implement `handle_semantic_action()` for Space key
3. Request focus on click
4. Respect `is_enabled()` state

### Step 4: Rendering (Day 2, Afternoon)

1. Implement `do_render()` with render context
2. Draw background
3. Draw checkbox box with correct character
4. Draw label text
5. Add focus indicator
6. Handle disabled state rendering

### Step 5: Theme Integration (Day 3, Morning)

1. Add theme properties to `theme.hh`
2. Implement `get_theme_background_color()`
3. Implement `get_theme_foreground_color()`
4. Add box characters to theme
5. Update Norton Blue theme

### Step 6: Mnemonic Support (Day 3, Afternoon)

1. Implement `set_mnemonic()`, `mnemonic()`
2. Integrate with mnemonic parser
3. Underline mnemonic character in text
4. Test Alt+key toggling

### Step 7: Testing (Day 4-5)

1. Write all unit tests
2. Write integration tests
3. Fix bugs found during testing
4. Achieve >90% code coverage

### Step 8: Documentation & Demo (Day 5)

1. Add checkbox to widgets_demo
2. Create Docusaurus documentation
3. Update WIDGET_ROADMAP.md
4. Git commit

---

## Open Questions

### Question 1: Tri-State Toggle Behavior

**Question**: When user clicks tri-state checkbox in `indeterminate` state, what should happen?

**Option A**: Indeterminate → Checked (current recommendation)
**Option B**: Indeterminate → Unchecked
**Option C**: Cycle through all 3 states (Unchecked → Indeterminate → Checked)

**Recommendation**: **Option A** - matches Qt/GTK behavior. Indeterminate is a "read-only" state set by code.

### Question 2: Mnemonic Behavior

**Question**: Should Alt+mnemonic toggle immediately or just focus?

**Option A**: Alt+E toggles checkbox immediately (one-step activation)
**Option B**: Alt+E focuses checkbox, user presses Space to toggle (two-step)

**Recommendation**: **Option A** - more efficient UX, matches Windows behavior.

### Question 3: Enter Key Behavior

**Question**: Should Enter key toggle checkbox when focused?

**Option A**: Enter toggles (like Space)
**Option B**: Enter submits form (activates default button)

**Recommendation**: **Option B** - Enter submits form. Only Space toggles checkbox.

### Question 4: Box Character Customization

**Question**: Should box characters be theme strings or hardcoded?

**Option A**: Theme strings (e.g., `checkbox.box_checked_icon: "[✓]"`)
**Option B**: Hardcoded in widget, theme only controls colors

**Recommendation**: **Option A** - allows terminal backends to use ASCII fallbacks.

### Question 5: Semantic Action

**Question**: Should we add `hotkey_action::activate_widget` or reuse existing action?

**Option A**: New `activate_widget` action (Space key)
**Option B**: Reuse `hotkey_action::activate_menu_item` (less clear semantics)

**Recommendation**: **Option A** - clearer intent, allows separate bindings.

### Question 6: Click Area

**Question**: Should only the box be clickable, or the entire widget (box + text)?

**Option A**: Only box (3 characters) is clickable
**Option B**: Entire widget (box + text) is clickable

**Recommendation**: **Option B** - larger click target, better UX (matches all modern UI frameworks).

---

## Next Steps

1. **Review this design document** - Discuss open questions
2. **Finalize API** - Lock in method signatures
3. **Begin implementation** - Start with Step 1 (Core Structure)
4. **Test-driven development** - Write tests alongside implementation
5. **Demo integration** - Add to widgets_demo for visual testing

**Estimated time**: 2 days implementation + 0.5 days testing = 2.5 days total

---

## References

- [Qt QCheckBox](https://doc.qt.io/qt-6/qcheckbox.html)
- [GTK CheckButton](https://docs.gtk.org/gtk4/class.CheckButton.html)
- [OnyxUI CLAUDE.md](../CLAUDE.md)
- [OnyxUI Architecture Guide](CLAUDE/ARCHITECTURE.md)
- [OnyxUI Theming Guide](CLAUDE/THEMING.md)
- [Widget Roadmap](WIDGET_ROADMAP.md)
