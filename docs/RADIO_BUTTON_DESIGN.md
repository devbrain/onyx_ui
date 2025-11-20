# Radio Button Widget Design

**Status**: Design Phase
**Date**: 2025-11-20
**Based on**: checkbox implementation (completed)

---

## Overview

The **radio_button** widget provides mutually exclusive selection within a group. Unlike checkboxes (which allow multiple selections), radio buttons enforce single selection - checking one radio button automatically unchecks all others in the group.

### Visual Appearance

```
DOS/TUI Style (conio backend):
( ) Unselected       <- Empty circle (3 chars)
(*) Selected         <- Filled circle (3 chars)
( ) Disabled         <- Grayed out

Future GUI Backends:
◯ Unselected         <- Empty circle (Unicode)
◉ Selected           <- Filled circle (Unicode)
○ Disabled           <- Grayed out
```

---

## Architecture

### Core Components

1. **radio_button** - Individual radio button widget
   - Inherits from `widget<Backend>`
   - Similar to checkbox but cannot be tri-state
   - Stores reference to owning button_group (if any)

2. **button_group** - Manager for mutually exclusive radio buttons
   - **NOT** a widget (doesn't inherit from ui_element)
   - Manages group of radio buttons
   - Enforces mutual exclusion
   - Provides group-level API and signals

### Key Differences from Checkbox

| Feature | Checkbox | Radio Button |
|---------|----------|--------------|
| **Multiple selection** | Yes (independent) | No (mutually exclusive) |
| **Tri-state** | Yes (indeterminate) | No (checked/unchecked only) |
| **Grouping** | Optional (for select-all) | Required (for mutual exclusion) |
| **Default state** | Unchecked | One must be checked in group |
| **Icon** | `[ ]` / `[X]` / `[-]` | `( )` / `(*)` |
| **Keyboard nav** | Space toggles | Arrow keys navigate group |

---

## API Design

### radio_button Widget

```cpp
template<UIBackend Backend>
class radio_button : public widget<Backend> {
public:
    using base = widget<Backend>;
    using typename base::color_type;
    using typename base::rect_type;
    using typename base::size_type;
    using typename base::renderer_type;

    // ===== Construction =====

    /// Create radio button with optional text label
    explicit radio_button(std::string text = "", ui_element<Backend>* parent = nullptr);

    // ===== State Management =====

    /// Set checked state
    ///
    /// If this radio button is in a button_group, checking it will automatically
    /// uncheck all other radio buttons in the group.
    ///
    /// @param checked true to check, false to uncheck
    void set_checked(bool checked);

    /// Get checked state
    ///
    /// @return true if checked
    [[nodiscard]] bool is_checked() const noexcept { return m_is_checked; }

    // ===== Text Label =====

    /// Set label text
    void set_text(const std::string& text);

    /// Get label text
    [[nodiscard]] const std::string& text() const noexcept { return m_text; }

    // ===== Mnemonic Support =====

    /// Set mnemonic character for keyboard shortcut (Alt+key)
    void set_mnemonic(char key);

    /// Get mnemonic character
    [[nodiscard]] char mnemonic() const noexcept { return m_mnemonic; }

    // ===== Group Management =====

    /// Set button group (internal use - called by button_group)
    ///
    /// @param group Pointer to owning button_group, or nullptr to remove from group
    void set_group(button_group<Backend>* group);

    /// Get button group
    ///
    /// @return Pointer to owning button_group, or nullptr if not in a group
    [[nodiscard]] button_group<Backend>* group() const noexcept { return m_group; }

    // ===== Signals =====

    /// Emitted when checked state changes
    ///
    /// Parameter: true if checked, false if unchecked
    signal<bool> toggled;

protected:
    void do_render(render_context<Backend>& ctx) const override;
    bool handle_event(const ui_event& event, event_phase phase) override;
    bool handle_semantic_action(hotkey_action action) override;

private:
    std::string m_text;                       ///< Label text
    bool m_is_checked = false;                ///< Current state
    char m_mnemonic = '\0';                   ///< Mnemonic character
    button_group<Backend>* m_group = nullptr; ///< Owning group (non-owning ptr)

    friend class button_group<Backend>;
};
```

### button_group Class

```cpp
/// Manages mutually exclusive radio buttons
///
/// The button_group class enforces single selection among a group of radio buttons.
/// When one radio button is checked, all others in the group are automatically unchecked.
///
/// Usage:
/// @code
/// auto group = std::make_shared<button_group<Backend>>();
///
/// auto radio1 = std::make_unique<radio_button<Backend>>("Option 1");
/// auto radio2 = std::make_unique<radio_button<Backend>>("Option 2");
/// auto radio3 = std::make_unique<radio_button<Backend>>("Option 3");
///
/// group->add_button(radio1.get(), 1);
/// group->add_button(radio2.get(), 2);
/// group->add_button(radio3.get(), 3);
/// group->set_checked_id(1);  // Select first option
/// @endcode
///
/// @tparam Backend The backend traits class
template<UIBackend Backend>
class button_group {
public:
    button_group() = default;

    // ===== Button Management =====

    /// Add radio button to group
    ///
    /// The radio button's group pointer is automatically set to this group.
    /// If id is -1, a unique ID is auto-assigned.
    ///
    /// @param button Pointer to radio button (non-owning - widget is owned by parent)
    /// @param id Unique ID for this button, or -1 for auto-assignment
    void add_button(radio_button<Backend>* button, int id = -1);

    /// Remove radio button from group
    ///
    /// The radio button's group pointer is automatically cleared.
    ///
    /// @param button Pointer to radio button to remove
    void remove_button(radio_button<Backend>* button);

    /// Get all buttons in group
    ///
    /// @return Vector of button pointers
    [[nodiscard]] std::vector<radio_button<Backend>*> buttons() const;

    /// Get button by ID
    ///
    /// @param id Button ID
    /// @return Pointer to button, or nullptr if not found
    [[nodiscard]] radio_button<Backend>* button(int id) const;

    // ===== Selection Management =====

    /// Get checked button ID
    ///
    /// @return ID of checked button, or -1 if none checked
    [[nodiscard]] int checked_id() const noexcept { return m_checked_id; }

    /// Get checked button pointer
    ///
    /// @return Pointer to checked button, or nullptr if none checked
    [[nodiscard]] radio_button<Backend>* checked_button() const;

    /// Set checked button by ID
    ///
    /// Automatically unchecks all other buttons in the group.
    ///
    /// @param id ID of button to check, or -1 to uncheck all
    void set_checked_id(int id);

    /// Set checked button by pointer
    ///
    /// Automatically unchecks all other buttons in the group.
    ///
    /// @param button Pointer to button to check, or nullptr to uncheck all
    void set_checked_button(radio_button<Backend>* button);

    // ===== Signals =====

    /// Emitted when any button in group changes state
    ///
    /// Parameters: (button_id, checked)
    signal<int, bool> button_toggled;

private:
    std::unordered_map<int, radio_button<Backend>*> m_buttons;
    int m_checked_id = -1;
    int m_next_id = 0;

    friend class radio_button<Backend>;
};
```

---

## Icon System (3-Character Rendering)

Following the checkbox pattern, radio buttons use **3-character icons**:

### Icon Style Enums

Add to `conio_renderer.hh` and `test_backend.hh`:

```cpp
enum class icon_style : uint8_t {
    // ... existing icons ...
    radio_unchecked,    // ( ) (unchecked radio button)
    radio_checked       // (*) (checked radio button)
};
```

### Icon Size Handling

Update `get_icon_size()`:

```cpp
[[nodiscard]] static size get_icon_size(const icon_style& icon) noexcept {
    switch (icon) {
        case icon_style::checkbox_unchecked:
        case icon_style::checkbox_checked:
        case icon_style::checkbox_indeterminate:
        case icon_style::radio_unchecked:      // 3x1
        case icon_style::radio_checked:        // 3x1
            return size{3, 1};
        default:
            return size{1, 1};
    }
}
```

### Icon Rendering

Update `draw_icon()` in `conio_renderer.cc`:

```cpp
// Handle multi-character radio button icons (3 characters wide)
if (style == icon_style::radio_unchecked ||
    style == icon_style::radio_checked) {

    // Classic DOS-style 3-character radio button rendering
    const char* radio_str;
    switch (style) {
        case icon_style::radio_unchecked: radio_str = "( )"; break;
        case icon_style::radio_checked:   radio_str = "(*)"; break;
        default: radio_str = "( )"; break;
    }

    // Render 3 characters horizontally
    for (int i = 0; i < 3 && r.x + i < clip.x + clip.w; ++i) {
        if (r.x + i >= clip.x) {
            m_pimpl->m_vram.put(r.x + i, r.y, radio_str[i], fg, bg);
        }
    }
    return;
}
```

---

## Theme System Integration

### radio_button_style Structure

Add to `include/onyxui/theming/theme.hh`:

```cpp
/// Radio button widget theme configuration
template<typename ColorType, typename FontType, typename IconStyleType>
struct radio_button_style {
    visual_state<ColorType, FontType> normal;        ///< Normal state (unchecked)
    visual_state<ColorType, FontType> hover;         ///< Mouse hover state
    visual_state<ColorType, FontType> checked;       ///< Checked state
    visual_state<ColorType, FontType> disabled;      ///< Disabled state
    FontType mnemonic_font{};                        ///< Font for mnemonic character
    IconStyleType unchecked_icon{};                  ///< Icon for unchecked button
    IconStyleType checked_icon{};                    ///< Icon for checked button
    int spacing = 1;  ///< Space between icon and label text
};
```

### Add to ui_theme

```cpp
template<typename Backend>
struct ui_theme {
    // ... existing theme sections ...
    checkbox_style<color_type, font_type, icon_style_type> checkbox{};
    radio_button_style<color_type, font_type, icon_style_type> radio_button{};
};
```

### Theme Configuration

Update `conio_themes.hh`:

```cpp
// Radio Button Configuration
theme.radio_button.unchecked_icon = conio_renderer::icon_style::radio_unchecked;
theme.radio_button.checked_icon = conio_renderer::icon_style::radio_checked;

theme.radio_button.normal = {
    .font = conio_renderer::font{.bold = false, .underline = false, .reverse = false},
    .foreground = builder.resolve_color("button_fg"),
    .background = builder.resolve_color("button_bg"),
    .mnemonic_foreground = builder.resolve_color("button_fg")
};

theme.radio_button.hover = {
    .font = conio_renderer::font{.bold = false, .underline = false, .reverse = false},
    .foreground = builder.resolve_color("selection_fg"),
    .background = builder.resolve_color("button_bg"),
    .mnemonic_foreground = builder.resolve_color("selection_fg")
};

theme.radio_button.checked = {
    .font = conio_renderer::font{.bold = false, .underline = false, .reverse = false},
    .foreground = builder.resolve_color("selection_fg"),  // Highlight when checked
    .background = builder.resolve_color("button_bg"),
    .mnemonic_foreground = builder.resolve_color("selection_fg")
};

theme.radio_button.disabled = {
    .font = conio_renderer::font{.bold = false, .underline = false, .reverse = false},
    .foreground = builder.resolve_color("disabled_fg"),
    .background = builder.resolve_color("button_bg"),
    .mnemonic_foreground = builder.resolve_color("disabled_fg")
};

theme.radio_button.mnemonic_font = {.bold = false, .underline = true, .reverse = false};
theme.radio_button.spacing = 1;
```

---

## Event Handling

### Mouse Interaction

Radio buttons respond to mouse clicks:

```cpp
bool radio_button<Backend>::handle_event(const ui_event& event, event_phase phase) {
    if (auto* mouse_evt = std::get_if<mouse_event>(&event)) {
        if (mouse_evt->act == mouse_event::action::press) {
            if (phase == event_phase::capture) {
                // Request focus in capture phase
                auto* input = ui_services<Backend>::input();
                if (input && this->is_focusable()) {
                    input->set_focus(this);
                }
                return false;  // Continue to target phase
            }

            if (phase == event_phase::target) {
                // Check radio button on click (cannot uncheck by clicking)
                if (this->is_enabled() && !m_is_checked) {
                    set_checked(true);
                    return true;
                }
            }
        }
    }

    return base::handle_event(event, phase);
}
```

**Key Design Decision**: Clicking a checked radio button does **NOT** uncheck it. Users must select a different option to deselect.

### Keyboard Interaction

Radio buttons support:

1. **Space key** - Check the focused radio button
2. **Arrow keys** - Navigate within button group and check:
   - Up/Left arrow: Select previous radio button in group
   - Down/Right arrow: Select next radio button in group

```cpp
bool radio_button<Backend>::handle_semantic_action(hotkey_action action) {
    switch (action) {
        case hotkey_action::activate_widget:
            // Space key checks radio button
            if (this->is_enabled() && !m_is_checked) {
                set_checked(true);
                return true;
            }
            return false;

        case hotkey_action::navigate_up:
        case hotkey_action::navigate_left:
            // Navigate to previous radio button in group
            if (m_group) {
                m_group->select_previous(this);
                return true;
            }
            return false;

        case hotkey_action::navigate_down:
        case hotkey_action::navigate_right:
            // Navigate to next radio button in group
            if (m_group) {
                m_group->select_next(this);
                return true;
            }
            return false;

        default:
            return base::handle_semantic_action(action);
    }
}
```

### New Semantic Actions

Add to `hotkey_action.hh` if not already present:

```cpp
enum class hotkey_action : std::uint8_t {
    // ... existing actions ...
    navigate_up,       // Arrow up (move focus up)
    navigate_down,     // Arrow down (move focus down)
    navigate_left,     // Arrow left (move focus left)
    navigate_right,    // Arrow right (move focus right)
    // ...
};
```

---

## Mutual Exclusion Logic

### Checking a Radio Button

When `radio_button::set_checked(true)` is called:

```cpp
void radio_button<Backend>::set_checked(bool checked) {
    if (m_is_checked == checked) {
        return;  // No change
    }

    m_is_checked = checked;
    this->mark_dirty();

    // If checking this button and it's in a group, uncheck all others
    if (checked && m_group) {
        m_group->notify_button_checked(this);
    }

    // Emit signal
    toggled.emit(m_is_checked);
}
```

### button_group Mutual Exclusion

```cpp
void button_group<Backend>::notify_button_checked(radio_button<Backend>* checked_button) {
    // Find the ID of the checked button
    int checked_id = -1;
    for (const auto& [id, button] : m_buttons) {
        if (button == checked_button) {
            checked_id = id;
            break;
        }
    }

    if (checked_id == -1) {
        return;  // Button not in this group
    }

    // Uncheck all other buttons
    for (const auto& [id, button] : m_buttons) {
        if (id != checked_id && button->is_checked()) {
            button->m_is_checked = false;  // Direct access (friend)
            button->mark_dirty();
            button->toggled.emit(false);
        }
    }

    // Update checked ID
    m_checked_id = checked_id;

    // Emit group signal
    button_toggled.emit(checked_id, true);
}
```

---

## Usage Examples

### Basic Radio Button Group

```cpp
#include <onyxui/widgets/input/radio_button.hh>
#include <onyxui/widgets/input/button_group.hh>

// Create group
auto group = std::make_shared<button_group<Backend>>();

// Create radio buttons
auto small = std::make_unique<radio_button<Backend>>("Small");
auto medium = std::make_unique<radio_button<Backend>>("Medium");
auto large = std::make_unique<radio_button<Backend>>("Large");

// Add to group with IDs
group->add_button(small.get(), 0);
group->add_button(medium.get(), 1);
group->add_button(large.get(), 2);

// Set default selection
group->set_checked_id(1);  // Medium selected

// Listen for changes
group->button_toggled.connect([](int id, bool checked) {
    if (checked) {
        std::cout << "Selected size ID: " << id << "\n";
    }
});

// Add to layout
auto vbox = std::make_unique<vbox<Backend>>();
vbox->add_child(std::move(small));
vbox->add_child(std::move(medium));
vbox->add_child(std::move(large));
```

### Preferences Dialog

```cpp
auto group = std::make_shared<button_group<Backend>>();

auto* light = container->template emplace_child<radio_button<Backend>>("&Light theme");
auto* dark = container->template emplace_child<radio_button<Backend>>("&Dark theme");
auto* auto_theme = container->template emplace_child<radio_button<Backend>>("&Auto (system)");

light->set_mnemonic('l');
dark->set_mnemonic('d');
auto_theme->set_mnemonic('a');

group->add_button(light, 0);
group->add_button(dark, 1);
group->add_button(auto_theme, 2);
group->set_checked_id(0);  // Default to light

group->button_toggled.connect([](int id, bool checked) {
    if (checked) {
        switch (id) {
            case 0: apply_light_theme(); break;
            case 1: apply_dark_theme(); break;
            case 2: apply_auto_theme(); break;
        }
    }
});
```

### Without button_group (Not Recommended)

Radio buttons can exist without a group, but they won't be mutually exclusive:

```cpp
// Standalone radio button (acts like checkbox)
auto standalone = std::make_unique<radio_button<Backend>>("Standalone option");
standalone->toggled.connect([](bool checked) {
    std::cout << "Standalone: " << (checked ? "ON" : "OFF") << "\n";
});
```

**Note**: This is not recommended - radio buttons should always be in a group.

---

## Testing Strategy

### radio_button Tests

```cpp
TEST_CASE("radio_button - Construction") {
    radio_button<Backend> rb("Option 1");
    REQUIRE(rb.text() == "Option 1");
    REQUIRE_FALSE(rb.is_checked());
    REQUIRE(rb.group() == nullptr);
}

TEST_CASE("radio_button - Set/get checked") {
    radio_button<Backend> rb;
    rb.set_checked(true);
    REQUIRE(rb.is_checked());
    rb.set_checked(false);
    REQUIRE_FALSE(rb.is_checked());
}

TEST_CASE("radio_button - toggled signal") {
    radio_button<Backend> rb;
    bool signal_emitted = false;
    bool signal_value = false;

    rb.toggled.connect([&](bool checked) {
        signal_emitted = true;
        signal_value = checked;
    });

    rb.set_checked(true);
    REQUIRE(signal_emitted);
    REQUIRE(signal_value);
}

TEST_CASE("radio_button - Click checks button") {
    radio_button<Backend> rb;
    mouse_event evt{.x = 0, .y = 0, .btn = mouse_event::button::left,
                    .act = mouse_event::action::press, .modifiers = {}};

    rb.handle_event(ui_event{evt}, event_phase::target);
    REQUIRE(rb.is_checked());
}

TEST_CASE("radio_button - Cannot uncheck by clicking") {
    radio_button<Backend> rb;
    rb.set_checked(true);

    mouse_event evt{.x = 0, .y = 0, .btn = mouse_event::button::left,
                    .act = mouse_event::action::press, .modifiers = {}};

    rb.handle_event(ui_event{evt}, event_phase::target);
    REQUIRE(rb.is_checked());  // Still checked
}
```

### button_group Tests

```cpp
TEST_CASE("button_group - Add buttons") {
    button_group<Backend> group;
    radio_button<Backend> rb1, rb2;

    group.add_button(&rb1, 0);
    group.add_button(&rb2, 1);

    REQUIRE(rb1.group() == &group);
    REQUIRE(rb2.group() == &group);
    REQUIRE(group.buttons().size() == 2);
}

TEST_CASE("button_group - Mutual exclusion") {
    button_group<Backend> group;
    radio_button<Backend> rb1, rb2, rb3;

    group.add_button(&rb1, 0);
    group.add_button(&rb2, 1);
    group.add_button(&rb3, 2);

    // Check first button
    group.set_checked_id(0);
    REQUIRE(rb1.is_checked());
    REQUIRE_FALSE(rb2.is_checked());
    REQUIRE_FALSE(rb3.is_checked());

    // Check second button - first should uncheck
    group.set_checked_id(1);
    REQUIRE_FALSE(rb1.is_checked());
    REQUIRE(rb2.is_checked());
    REQUIRE_FALSE(rb3.is_checked());
}

TEST_CASE("button_group - button_toggled signal") {
    button_group<Backend> group;
    radio_button<Backend> rb1, rb2;

    group.add_button(&rb1, 0);
    group.add_button(&rb2, 1);

    int last_id = -1;
    bool last_checked = false;

    group.button_toggled.connect([&](int id, bool checked) {
        last_id = id;
        last_checked = checked;
    });

    group.set_checked_id(1);
    REQUIRE(last_id == 1);
    REQUIRE(last_checked);
}

TEST_CASE("button_group - Auto-assign IDs") {
    button_group<Backend> group;
    radio_button<Backend> rb1, rb2, rb3;

    group.add_button(&rb1);  // ID = 0
    group.add_button(&rb2);  // ID = 1
    group.add_button(&rb3);  // ID = 2

    REQUIRE(group.button(0) == &rb1);
    REQUIRE(group.button(1) == &rb2);
    REQUIRE(group.button(2) == &rb3);
}
```

---

## Implementation Checklist

- [ ] Create `include/onyxui/widgets/input/radio_button.hh`
- [ ] Create `include/onyxui/widgets/input/button_group.hh`
- [ ] Add `radio_button_style` to `theme.hh`
- [ ] Add icon enums to `conio_renderer.hh` and `test_backend.hh`
- [ ] Implement icon rendering in `conio_renderer.cc`
- [ ] Add icon reflection for YAML serialization
- [ ] Update built-in themes (`conio_themes.hh`)
- [ ] Update YAML themes (`norton_blue.yaml`)
- [ ] Write comprehensive tests (`test_radio_button.cc`, `test_button_group.cc`)
- [ ] Add to demo (`demo_ui_builder.hh`)
- [ ] Create Docusaurus documentation (`radio-button.md`)
- [ ] Update `widget-library.md`

---

## Open Questions

1. **Default Selection**: Should one radio button in a group always be checked?
   - **Recommendation**: Yes - require at least one selection for good UX

2. **Arrow Key Navigation**: Should arrow keys wrap around (last → first)?
   - **Recommendation**: Yes - wrap for easier navigation

3. **Tab Key Behavior**: Should Tab skip to next widget or visit all radio buttons?
   - **Recommendation**: Tab moves to next widget group, arrows navigate within group

4. **Vertical vs Horizontal Layout**: Should button_group track orientation?
   - **Recommendation**: No - layout is determined by container (vbox/hbox)

5. **Mnemonic Conflicts**: What if two radio buttons have same mnemonic?
   - **Recommendation**: First match wins (consistent with button behavior)

---

## Next Steps

1. **Implement radio_button.hh** - Core widget class
2. **Implement button_group.hh** - Mutual exclusion manager
3. **Add theme support** - Icon styles and visual states
4. **Implement rendering** - 3-character icons in renderer
5. **Write tests** - Comprehensive coverage
6. **Update demo** - Show radio button groups in action
7. **Create documentation** - Full API reference and examples
