---
sidebar_position: 3
---

# Checkbox

The `onyxui::checkbox` is a toggleable input widget for boolean or tri-state selection. It's essential for settings, forms, and any UI that requires user confirmation or option selection.

## Overview

A checkbox widget provides a toggle control with a visual indicator (box) and optional text label. Supports both two-state (checked/unchecked) and tri-state (checked/unchecked/indeterminate) modes.

## Key Features

-   **Two-State Mode:** Simple toggle between unchecked and checked
-   **Tri-State Mode:** Support for indeterminate state (e.g., "select all" scenarios)
-   **Keyboard Interaction:** Space key toggles, Alt+mnemonic shortcuts
-   **Mouse Interaction:** Click anywhere on widget toggles state
-   **Themed Icons:** Backend-specific rendering (3-character DOS-style or Unicode glyphs)
-   **Semantic Actions:** Integrated with hotkey system
-   **Signals:** Dual signal system for two-state and tri-state changes
-   **Mnemonic Support:** Keyboard shortcuts with underlined characters

## Visual Appearance

Checkboxes render using themed icons that adapt to the backend:

**Classic DOS/TUI Style (conio backend):**
```
[ ] Unchecked
[X] Checked
[-] Indeterminate
```

**Future GUI Backends:**
```
☐ Unchecked
☑ Checked
▣ Indeterminate
```

## Usage

Here's a simple example of creating and using checkboxes:

```cpp
#include <onyxui/widgets/input/checkbox.hh>
#include <iostream>

// Simple checkbox (unchecked by default)
auto enable_feature = std::make_unique<checkbox<Backend>>("Enable feature");
enable_feature->toggled.connect([](bool checked) {
    std::cout << "Feature: " << (checked ? "ON" : "OFF") << "\n";
});

// Pre-checked checkbox
auto remember_me = std::make_unique<checkbox<Backend>>("Remember me", true);

// Tri-state checkbox (for "select all" scenarios)
auto select_all = std::make_unique<checkbox<Backend>>("Select All");
select_all->set_tri_state_enabled(true);
select_all->set_tri_state(tri_state::indeterminate);
select_all->state_changed.connect([](tri_state state) {
    switch (state) {
        case tri_state::unchecked:     std::cout << "None selected\n"; break;
        case tri_state::checked:       std::cout << "All selected\n"; break;
        case tri_state::indeterminate: std::cout << "Some selected\n"; break;
    }
});

// Disabled checkbox
auto disabled_option = std::make_unique<checkbox<Backend>>("Disabled option", true);
disabled_option->set_enabled(false);

// Checkbox with mnemonic (Alt+N toggles)
auto notify_checkbox = std::make_unique<checkbox<Backend>>("Enable &notifications");
notify_checkbox->set_mnemonic('n');
```

## Checkbox States

### Two-State Mode (Default)

The checkbox toggles between two states:
- **Unchecked** (`tri_state::unchecked`): Option is disabled/not selected
- **Checked** (`tri_state::checked`): Option is enabled/selected

Toggle behavior:
- **Unchecked → Checked** on first click
- **Checked → Unchecked** on second click

### Tri-State Mode

Enable with `set_tri_state_enabled(true)`. Three possible states:
- **Unchecked** (`tri_state::unchecked`): None selected
- **Checked** (`tri_state::checked`): All selected
- **Indeterminate** (`tri_state::indeterminate`): Some selected

Toggle behavior:
- **Indeterminate → Checked** on click (user exits indeterminate)
- **Unchecked → Checked** on click
- **Checked → Unchecked** on click

**Important:** Programmatically set indeterminate state - users can only toggle between checked/unchecked.

## Keyboard Interaction

### Space Key
- Toggles checkbox state (same as clicking)
- Semantic action: `hotkey_action::activate_widget`

### Mnemonic Keys
- **Alt+Key:** Toggles checkbox if mnemonic is set
- Example: "Enable &notifications" → Alt+N toggles

### Navigation
- **Tab/Shift+Tab:** Move focus to next/previous focusable widget

## API Reference

### Template Parameters

-   `Backend`: The backend traits class providing rendering types.

### Public Signals

-   **`signal<bool> toggled`:** Emitted when toggling between checked/unchecked (two-state only).
    - Parameter: `true` if checked, `false` if unchecked.
    - **NOT** emitted when entering/leaving indeterminate state.

-   **`signal<tri_state> state_changed`:** Emitted on ANY state change (including indeterminate).
    - Parameter: New `tri_state` value.
    - Use this signal in tri-state mode.

### Public Methods

#### Construction
-   **`checkbox()`:** Create unchecked checkbox with no text.
-   **`checkbox(const std::string& text)`:** Create unchecked checkbox with label.
-   **`checkbox(const std::string& text, bool checked)`:** Create checkbox with specified initial state.

#### State Management (Two-State)
-   **`void set_checked(bool checked)`:** Set checked state.
-   **`bool is_checked() const`:** Returns true if checked.
-   **`void toggle()`:** Toggle between unchecked and checked.

#### State Management (Tri-State)
-   **`void set_tri_state_enabled(bool enabled)`:** Enable/disable tri-state mode.
-   **`bool is_tri_state_enabled() const`:** Returns true if tri-state enabled.
-   **`void set_tri_state(tri_state state)`:** Set specific tri-state value.
-   **`tri_state get_tri_state() const`:** Returns current tri-state value.

#### Text Label
-   **`void set_text(const std::string& text)`:** Set label text.
-   **`const std::string& text() const`:** Returns current label text.

#### Mnemonic Support
-   **`void set_mnemonic(char key)`:** Set mnemonic shortcut key (case-insensitive).
-   **`char mnemonic() const`:** Returns mnemonic character (or `'\0'` if none).

### Tri-State Enum

```cpp
enum class tri_state : std::uint8_t {
    unchecked = 0,      ///< Box is empty
    checked = 1,        ///< Box has checkmark
    indeterminate = 2   ///< Box has partial indicator (tri-state only)
};
```

### Theming

The appearance of checkboxes is controlled by the `checkbox` section in the theme:

```yaml
checkbox:
  # Normal state (unchecked)
  normal:
    font: { bold: false, reverse: false, underline: false }
    foreground: { r: 255, g: 255, b: 255 }  # White
    background: { r: 0, g: 0, b: 170 }      # Dark blue

  # Hover state
  hover:
    font: { bold: false, reverse: false, underline: false }
    foreground: { r: 255, g: 255, b: 0 }    # Yellow
    background: { r: 0, g: 0, b: 255 }      # Bright blue

  # Checked state
  checked:
    font: { bold: false, reverse: false, underline: false }
    foreground: { r: 255, g: 255, b: 0 }    # Yellow (highlight)
    background: { r: 0, g: 0, b: 170 }      # Dark blue

  # Disabled state
  disabled:
    font: { bold: false, reverse: false, underline: false }
    foreground: { r: 85, g: 85, b: 85 }     # Dark gray
    background: { r: 0, g: 0, b: 170 }      # Dark blue

  # Mnemonic character styling
  mnemonic_font: { bold: false, reverse: false, underline: true }

  # Icon styles (backend-specific)
  unchecked_icon: checkbox_unchecked       # [ ] or ☐
  checked_icon: checkbox_checked           # [X] or ☑
  indeterminate_icon: checkbox_indeterminate  # [-] or ▣

  # Spacing between box and text
  spacing: 1
```

### Theme Methods

Override these methods to customize appearance:

```cpp
[[nodiscard]] typename Backend::color_type get_theme_background_color() const override;
[[nodiscard]] typename Backend::color_type get_theme_foreground_color() const override;
```

## Complete Example

Here's a preferences dialog using checkboxes:

```cpp
#include <onyxui/ui_context.hh>
#include <onyxui/widgets/containers/vbox.hh>
#include <onyxui/widgets/input/checkbox.hh>
#include <onyxui/widgets/button.hh>
#include <onyxui/widgets/label.hh>

template<typename Backend>
class preferences_dialog : public vbox<Backend> {
public:
    preferences_dialog() {
        // Section label
        this->template emplace_child<label<Backend>>("General Settings:");

        // Individual options
        auto* auto_save = this->template emplace_child<checkbox<Backend>>("&Auto-save", true);
        auto_save->set_mnemonic('a');
        auto_save->toggled.connect([this](bool checked) {
            m_settings.auto_save = checked;
        });

        auto* notifications = this->template emplace_child<checkbox<Backend>>("Enable &notifications");
        notifications->set_mnemonic('n');

        auto* dark_mode = this->template emplace_child<checkbox<Backend>>("&Dark mode");
        dark_mode->set_mnemonic('d');

        // "Select All" tri-state checkbox
        auto* select_all = this->template emplace_child<checkbox<Backend>>("Select &All");
        select_all->set_tri_state_enabled(true);
        select_all->set_mnemonic('l');
        select_all->state_changed.connect([auto_save, notifications, dark_mode](tri_state state) {
            bool checked = (state == tri_state::checked);
            auto_save->set_checked(checked);
            notifications->set_checked(checked);
            dark_mode->set_checked(checked);
        });

        // Update "Select All" based on individual checkboxes
        auto update_select_all = [auto_save, notifications, dark_mode, select_all]() {
            int checked_count = 0;
            if (auto_save->is_checked()) ++checked_count;
            if (notifications->is_checked()) ++checked_count;
            if (dark_mode->is_checked()) ++checked_count;

            if (checked_count == 3) {
                select_all->set_tri_state(tri_state::checked);
            } else if (checked_count == 0) {
                select_all->set_tri_state(tri_state::unchecked);
            } else {
                select_all->set_tri_state(tri_state::indeterminate);
            }
        };

        auto_save->toggled.connect(update_select_all);
        notifications->toggled.connect(update_select_all);
        dark_mode->toggled.connect(update_select_all);

        // Save button
        auto* save_btn = this->template emplace_child<button<Backend>>("Save");
        save_btn->clicked.connect([this]() {
            save_preferences();
        });
    }

private:
    struct {
        bool auto_save = true;
        bool notifications = false;
        bool dark_mode = false;
    } m_settings;

    void save_preferences() {
        // Save settings to file/database
        std::cout << "Preferences saved!\n";
    }
};
```

## Use Cases

### Simple Toggle
```cpp
auto enable = std::make_unique<checkbox<Backend>>("Enable feature");
enable->toggled.connect([](bool checked) {
    if (checked) {
        activate_feature();
    } else {
        deactivate_feature();
    }
});
```

### Select All / Partial Selection
```cpp
// Parent checkbox (tri-state)
auto select_all = std::make_unique<checkbox<Backend>>("Select All");
select_all->set_tri_state_enabled(true);

// Child checkboxes
std::vector<checkbox<Backend>*> items;
for (const auto& item : item_list) {
    auto* cb = panel->template emplace_child<checkbox<Backend>>(item.name);
    items.push_back(cb);

    // Update parent when child changes
    cb->toggled.connect([select_all, &items]() {
        int checked = std::count_if(items.begin(), items.end(),
                                    [](auto* cb) { return cb->is_checked(); });
        if (checked == items.size()) {
            select_all->set_tri_state(tri_state::checked);
        } else if (checked == 0) {
            select_all->set_tri_state(tri_state::unchecked);
        } else {
            select_all->set_tri_state(tri_state::indeterminate);
        }
    });
}

// Select/deselect all children when parent is toggled
select_all->state_changed.connect([&items](tri_state state) {
    if (state != tri_state::indeterminate) {
        bool checked = (state == tri_state::checked);
        for (auto* cb : items) {
            cb->set_checked(checked);
        }
    }
});
```

### Agreement / Terms of Service
```cpp
auto agree = std::make_unique<checkbox<Backend>>("I agree to the terms and conditions");
agree->toggled.connect([submit_button](bool checked) {
    submit_button->set_enabled(checked);  // Enable submit only when agreed
});
```

## Best Practices

1. **Use clear, concise labels** - Describe what the checkbox controls
2. **Default to unchecked for optional features** - Except "Remember me" type options
3. **Use tri-state for hierarchical selection** - Parent checkbox for multiple children
4. **Connect to `toggled` for two-state** - Simpler handler for boolean logic
5. **Connect to `state_changed` for tri-state** - Handles all state transitions
6. **Set mnemonics for accessibility** - Enable keyboard-only interaction
7. **Disable, don't hide** - Show unavailable options as disabled

## Common Pitfalls

❌ **Wrong:** Using `toggled` signal in tri-state mode
```cpp
checkbox->set_tri_state_enabled(true);
checkbox->toggled.connect([](bool checked) {  // Won't fire for indeterminate!
    // This doesn't handle indeterminate state
});
```

✅ **Correct:** Using `state_changed` signal in tri-state mode
```cpp
checkbox->set_tri_state_enabled(true);
checkbox->state_changed.connect([](tri_state state) {
    // Handles all three states
});
```

❌ **Wrong:** Letting users set indeterminate via clicking
```cpp
// Tri-state checkbox will never stay in indeterminate from user clicks!
// User click always moves: indeterminate → checked
```

✅ **Correct:** Programmatically managing indeterminate state
```cpp
// Set indeterminate based on child selection state
update_parent_checkbox_state();  // Calls set_tri_state(indeterminate)
```

## Multi-Character Icon Rendering

Checkboxes use the icon theming system but with special multi-character rendering:

- **Most icons:** 1x1 character (arrows, bullets, etc.)
- **Checkbox icons:** 3x1 characters (`[ ]`, `[X]`, `[-]`)

The renderer automatically handles sizing via `get_icon_size(icon_style)`:
```cpp
// Renderer implementation
size get_icon_size(icon_style icon) {
    switch (icon) {
        case icon_style::checkbox_unchecked:
        case icon_style::checkbox_checked:
        case icon_style::checkbox_indeterminate:
            return {3, 1};  // 3 characters wide
        default:
            return {1, 1};  // Standard icons
    }
}
```

This allows different backends to render checkboxes differently while maintaining a consistent API.

## See Also

-   **[Button](./button.md)** - Often used with checkboxes in forms
-   **[Line Edit](./line-edit.md)** - Text input counterpart
-   **[Label](./label.md)** - For section headers
-   **[VBox](./vbox-hbox.md)** - For arranging checkboxes vertically
