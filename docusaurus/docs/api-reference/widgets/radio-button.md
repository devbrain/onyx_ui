---
sidebar_position: 4
---

# Radio Button

The `onyxui::radio_button` is a selection widget for mutually exclusive options. When grouped together using `button_group`, only one radio button can be selected at a time - essential for forms, settings, and any UI requiring single-choice selection.

## Overview

A radio button widget provides a toggle control with a visual indicator (circle) and optional text label. Unlike checkboxes (which allow multiple independent selections), radio buttons enforce mutual exclusion when placed in a `button_group`.

## Key Features

- **Mutual Exclusion:** Only one radio button in a group can be checked at a time
- **Keyboard Navigation:** Arrow keys navigate between options, Space selects
- **Mouse Interaction:** Click to select (cannot uncheck by clicking again)
- **Themed Icons:** Backend-specific rendering (3-character DOS-style or Unicode glyphs)
- **Button Groups:** Managed via `button_group` for coordinated behavior
- **Signals:** Emit events on state changes and group-level changes
- **Mnemonic Support:** Keyboard shortcuts with underlined characters
- **Focus Management:** Fully integrated with focus system

## Visual Appearance

Radio buttons render using themed icons that adapt to the backend:

**Classic DOS/TUI Style (conio backend):**
```
( ) Unchecked
(*) Checked
```

**Future GUI Backends:**
```
○ Unchecked
◉ Checked
```

## Basic Usage

Here's a simple example of creating radio buttons with a button group:

```cpp
#include <onyxui/widgets/input/radio_button.hh>
#include <onyxui/widgets/input/button_group.hh>
#include <iostream>

// Create button group to manage mutual exclusion
auto size_group = std::make_shared<button_group<Backend>>();

// Create radio buttons
auto small = std::make_unique<radio_button<Backend>>("Small");
auto medium = std::make_unique<radio_button<Backend>>("Medium");
auto large = std::make_unique<radio_button<Backend>>("Large");

// Add buttons to group with unique IDs
size_group->add_button(small.get(), 0);
size_group->add_button(medium.get(), 1);
size_group->add_button(large.get(), 2);

// Set default selection
size_group->set_checked_id(1);  // Medium selected

// Listen for selection changes
size_group->button_toggled.connect([](int id, bool checked) {
    if (checked) {
        const char* sizes[] = {"Small", "Medium", "Large"};
        std::cout << "Selected: " << sizes[id] << "\n";
    }
});
```

:::warning Important: Keep button_group Alive
The `button_group` must remain alive for the lifetime of the radio buttons! Store it as a member variable or `shared_ptr` to prevent it from being destroyed.
:::

## Radio Button Groups

### Creating a Button Group

```cpp
// Create group (keep alive as member variable or shared_ptr)
auto group = std::make_shared<button_group<Backend>>();

// Add buttons with auto-assigned IDs
group->add_button(radio1.get());  // ID = 0
group->add_button(radio2.get());  // ID = 1
group->add_button(radio3.get());  // ID = 2

// Or add buttons with explicit IDs
group->add_button(radio1.get(), 10);
group->add_button(radio2.get(), 20);
group->add_button(radio3.get(), 30);
```

### Managing Selection

```cpp
// Set selection by ID
group->set_checked_id(1);  // Check button with ID 1

// Set selection by pointer
group->set_checked_button(medium.get());

// Clear selection (no button checked)
group->set_checked_id(-1);

// Query current selection
int current_id = group->checked_id();  // Returns -1 if none checked
auto* current_button = group->checked_button();  // Returns nullptr if none checked
```

### Navigation

```cpp
// Navigate to next button in group (with wraparound)
group->select_next(current_button);

// Navigate to previous button (with wraparound)
group->select_previous(current_button);
```

## Radio Button API

### Construction

```cpp
// Default construction (unchecked)
radio_button<Backend> rb;

// With label text
radio_button<Backend> rb("Option 1");

// With label and initial state
radio_button<Backend> rb("Option 1", true);  // Initially checked
```

### State Management

```cpp
// Set checked state
rb.set_checked(true);
rb.set_checked(false);

// Query state
bool is_selected = rb.is_checked();
```

:::note Radio Button Behavior
Unlike checkboxes, clicking an already-checked radio button does NOT uncheck it. You must select a different option to change the selection.
:::

### Text and Mnemonics

```cpp
// Set label text
rb.set_text("New label");

// Get label text
std::string text = rb.text();

// Set mnemonic (Alt+S selects this button)
rb.set_mnemonic('s');

// Get mnemonic
char key = rb.mnemonic();  // Returns '\0' if none set
```

### Group Management

```cpp
// Get owning group
button_group<Backend>* group = rb.group();  // nullptr if not in a group

// Note: Use button_group::add_button() to add radio buttons to groups
// Do NOT call set_group() directly
```

## Signals

### Radio Button Signals

```cpp
// Emitted when checked state changes
rb.toggled.connect([](bool checked) {
    std::cout << "Radio button " << (checked ? "selected" : "deselected") << "\n";
});
```

### Button Group Signals

```cpp
// Emitted when any button in group changes state
group->button_toggled.connect([](int id, bool checked) {
    std::cout << "Button " << id << " is now "
              << (checked ? "checked" : "unchecked") << "\n";
});
```

:::tip Group Signal Advantage
When switching selection, `button_toggled` fires twice:
1. First for the previously selected button (checked=false)
2. Then for the newly selected button (checked=true)

This allows you to track complete state transitions.
:::

## Keyboard Interaction

Radio buttons support comprehensive keyboard navigation:

| Key | Action |
|-----|--------|
| **Space** | Select the focused radio button |
| **↑ / ←** | Navigate to previous button in group |
| **↓ / →** | Navigate to next button in group |
| **Alt + Mnemonic** | Select button with matching mnemonic |
| **Tab** | Move focus to next focusable widget |
| **Shift+Tab** | Move focus to previous focusable widget |

:::note Arrow Key Behavior
Arrow keys both navigate AND select. Unlike tab navigation (which only moves focus), pressing an arrow key immediately selects the next/previous radio button.
:::

## Theming

Radio buttons are fully themed via the `radio_button_style` structure:

```cpp
struct radio_button_style {
    visual_state normal;         // Normal state colors/font
    visual_state hover;          // Hover state colors/font
    visual_state checked;        // Checked state colors/font
    visual_state disabled;       // Disabled state colors/font
    font_type mnemonic_font;     // Font for mnemonic character
    icon_style unchecked_icon;   // Icon for unchecked state
    icon_style checked_icon;     // Icon for checked state
    int spacing;                 // Space between icon and text
};
```

### Theme Example (YAML)

```yaml
radio_button:
  # Normal state (unchecked)
  normal:
    font: { bold: false, reverse: false, underline: false }
    foreground: { r: 255, g: 255, b: 255 }  # White text
    background: { r: 0, g: 0, b: 170 }      # Dark blue

  # Hover state
  hover:
    font: { bold: false, reverse: false, underline: false }
    foreground: { r: 255, g: 255, b: 0 }    # Yellow text
    background: { r: 0, g: 0, b: 255 }      # Bright blue

  # Checked state
  checked:
    font: { bold: false, reverse: false, underline: false }
    foreground: { r: 255, g: 255, b: 0 }    # Yellow text (highlight)
    background: { r: 0, g: 0, b: 170 }      # Dark blue

  # Disabled state
  disabled:
    font: { bold: false, reverse: false, underline: false }
    foreground: { r: 85, g: 85, b: 85 }     # Dark gray
    background: { r: 0, g: 0, b: 170 }      # Dark blue

  mnemonic_font: { bold: false, reverse: false, underline: true }
  unchecked_icon: radio_unchecked  # ( )
  checked_icon: radio_checked      # (*)
  spacing: 1
```

## Advanced Examples

### Complete Form with Multiple Groups

```cpp
// Size selection group
auto size_group = std::make_shared<button_group<Backend>>();
auto* small = container->emplace_child<radio_button<Backend>>("Small");
auto* medium = container->emplace_child<radio_button<Backend>>("Medium");
auto* large = container->emplace_child<radio_button<Backend>>("Large");
size_group->add_button(small, 0);
size_group->add_button(medium, 1);
size_group->add_button(large, 2);
size_group->set_checked_id(1);  // Default: Medium

// Color selection group
auto color_group = std::make_shared<button_group<Backend>>();
auto* red = container->emplace_child<radio_button<Backend>>("Red");
auto* green = container->emplace_child<radio_button<Backend>>("Green");
auto* blue = container->emplace_child<radio_button<Backend>>("Blue");
color_group->add_button(red, 0);
color_group->add_button(green, 1);
color_group->add_button(blue, 2);
color_group->set_checked_id(2);  // Default: Blue

// Keep groups alive!
// Store as member variables: m_size_group, m_color_group
```

### Dynamic Selection Changes

```cpp
// Listen to one group and update another
size_group->button_toggled.connect([color_group](int id, bool checked) {
    if (checked) {
        // When size changes, update color selection accordingly
        if (id == 0) {
            color_group->set_checked_id(0);  // Small → Red
        } else if (id == 1) {
            color_group->set_checked_id(1);  // Medium → Green
        } else {
            color_group->set_checked_id(2);  // Large → Blue
        }
    }
});
```

### Disabled Radio Buttons

```cpp
auto* option1 = container->emplace_child<radio_button<Backend>>("Option 1");
auto* option2 = container->emplace_child<radio_button<Backend>>("Option 2");

// Disable specific option
option2->set_enabled(false);

// Disabled buttons:
// - Cannot be selected via mouse or keyboard
// - Render with disabled theme colors
// - Still count as part of the group
// - Can still be selected programmatically via set_checked()
```

## Best Practices

### ✅ Do

- **Keep button_group alive:** Store as member variable or `shared_ptr`
- **Use groups for mutual exclusion:** Don't rely on radio buttons alone
- **Provide clear labels:** Make options self-explanatory
- **Set default selection:** Always have one option pre-selected
- **Use mnemonics for accessibility:** Alt+key shortcuts improve usability
- **Group related options:** Keep radio button groups focused and cohesive

### ❌ Don't

- **Don't use radio buttons for on/off toggles:** Use checkbox instead
- **Don't make groups too large:** Consider using a dropdown/combo box for >7 options
- **Don't use inconsistent IDs:** Use sequential or meaningful ID schemes
- **Don't forget to store button_group:** It must outlive the radio buttons
- **Don't mix checkboxes and radio buttons:** They serve different purposes

## Common Patterns

### Settings Panel

```cpp
// Quality settings
auto quality_group = std::make_shared<button_group<Backend>>();
panel->emplace_child<label>("Quality:");
panel->emplace_child<radio_button>("Low")->add_to_group(quality_group, 0);
panel->emplace_child<radio_button>("Medium")->add_to_group(quality_group, 1);
panel->emplace_child<radio_button>("High")->add_to_group(quality_group, 2);
quality_group->set_checked_id(1);  // Default: Medium
```

### Multi-Step Wizard

```cpp
// Step indicator (non-interactive)
auto step_group = std::make_shared<button_group<Backend>>();
auto* step1 = wizard->emplace_child<radio_button>("1. Account Info");
auto* step2 = wizard->emplace_child<radio_button>("2. Preferences");
auto* step3 = wizard->emplace_child<radio_button>("3. Confirm");
step_group->add_button(step1, 0);
step_group->add_button(step2, 1);
step_group->add_button(step3, 2);

// Disable all except current step
step1->set_enabled(false);
step2->set_enabled(true);  // Current step
step3->set_enabled(false);

// Navigate programmatically
step_group->set_checked_id(1);  // Show step 2
```

## Comparison with Checkbox

| Feature | Radio Button | Checkbox |
|---------|--------------|----------|
| **Selection** | Single choice from group | Multiple independent choices |
| **Clicking checked item** | No effect | Unchecks |
| **Typical use** | Mutually exclusive options | Independent toggles |
| **Group required** | Yes (`button_group`) | No (optional) |
| **Tri-state** | No | Yes |
| **Icon** | ( ) / (*) | [ ] / [X] / [-] |

## Related Components

- **[Checkbox](./checkbox.md)** - For independent boolean options
- **[Button](./button.md)** - For actions rather than selection
- **Button Group** - Manages radio button mutual exclusion

## See Also

- **API Reference:** `include/onyxui/widgets/input/radio_button.hh`
- **API Reference:** `include/onyxui/widgets/input/button_group.hh`
- **Example:** `examples/demo_ui_builder.hh` (radio button demo)
- **Tests:** `unittest/widgets/test_radio_button.cc`
- **Tests:** `unittest/widgets/test_button_group.cc`
