---
sidebar_position: 9
---

# combo_box

**Category**: Input Widget
**MVC Component**: View
**Header**: `<onyxui/widgets/input/combo_box.hh>`
**Since**: v2025.11 (Phase 2)

A dropdown selection widget that uses the MVC (Model-View-Controller) pattern to display a list of items and allow the user to select one.

## Overview

`combo_box` combines:
- A button showing the current selection
- MVC model integration for data binding
- Keyboard navigation for selection
- State-based theming (normal/hover/pressed/disabled)

**Visual Appearance**:
```
┌─────────────────────────┐
│ Medium              ▼   │  ← Button showing current selection
└─────────────────────────┘
```

## Basic Usage

```cpp
#include <onyxui/widgets/input/combo_box.hh>
#include <onyxui/mvc/models/list_model.hh>

// Create model with options
auto model = std::make_shared<list_model<std::string, Backend>>();
model->set_items({"Small", "Medium", "Large", "X-Large"});

// Create combo box
auto combo = std::make_unique<combo_box<Backend>>();
combo->set_model(model.get());
combo->set_current_index(1);  // Select "Medium"

// Handle selection changes
combo->current_index_changed.connect([](int index) {
    std::cout << "Selected index: " << index << "\n";
});

root->add_child(std::move(combo));
```

## Template Parameters

- **`Backend`**: UI backend type (must satisfy `UIBackend` concept)

## Public API

### Construction

```cpp
combo_box()
```
Constructs an empty combo box with no model and no selection.

### Model Management

```cpp
void set_model(model_type* model)
```
Set the data model (not owned by combo box).

```cpp
[[nodiscard]] model_type* model() const noexcept
```
Get the current model pointer.

### Selection

```cpp
[[nodiscard]] int current_index() const noexcept
```
Get the currently selected row index (-1 if no selection).

```cpp
void set_current_index(int index)
```
Set the current selection (or -1 to clear).

```cpp
[[nodiscard]] const std::string& current_text() const noexcept
```
Get the display text of the current selection.

### Popup Management

```cpp
[[nodiscard]] bool is_popup_open() const noexcept
```
Check if the popup is currently open.

```cpp
void open_popup()
```
Open the dropdown popup (stub - requires layer_manager integration).

```cpp
void close_popup()
```
Close the dropdown popup.

## Signals

### current_index_changed

```cpp
signal<int> current_index_changed
```

Emitted when the current selection changes.

**Parameters**:
- `int index` - New current index (-1 if no selection)

**Example**:
```cpp
combo->current_index_changed.connect([&](int index) {
    if (index >= 0) {
        std::cout << "Selected: " << combo->current_text() << "\n";
    } else {
        std::cout << "Selection cleared\n";
    }
});
```

## Keyboard Navigation

The combo box supports full keyboard navigation:

| Key | Action |
|-----|--------|
| **↓** (Down Arrow) | Select next item (wraps to first) |
| **↑** (Up Arrow) | Select previous item (wraps to last) |
| **Home** | Select first item |
| **End** | Select last item |
| **Tab** | Move focus to next widget |
| **Shift+Tab** | Move focus to previous widget |

## Theming

`combo_box` uses button theming for state-based colors:

```cpp
// State-based colors (from button theme)
- normal: button.normal.background, button.normal.foreground
- hovered: button.hovered.background, button.hovered.foreground
- pressed: button.pressed.background, button.pressed.foreground
- disabled: button.disabled.background, button.disabled.foreground
```

Override theming in your theme YAML:
```yaml
button:
  normal:
    background: { r: 60, g: 60, b: 60 }
    foreground: { r: 255, g: 255, b: 255 }
  hovered:
    background: { r: 80, g: 80, b: 80 }
    foreground: { r: 255, g: 255, b: 255 }
```

## Examples

### Basic Selection

```cpp
auto model = std::make_shared<list_model<std::string, Backend>>();
model->set_items({"Red", "Green", "Blue", "Yellow"});

auto combo = std::make_unique<combo_box<Backend>>();
combo->set_model(model.get());
combo->set_current_index(0);  // Select "Red"

root->add_child(std::move(combo));
```

### Custom Type

```cpp
struct Resolution {
    int width;
    int height;
};

// Specialize to_string for Resolution
template<>
std::string list_model<Resolution, Backend>::to_string(const Resolution& r) {
    return std::to_string(r.width) + "x" + std::to_string(r.height);
}

// Use Resolution in combo_box
auto model = std::make_shared<list_model<Resolution, Backend>>();
model->set_items({
    {1920, 1080},
    {2560, 1440},
    {3840, 2160}
});

auto combo = std::make_unique<combo_box<Backend>>();
combo->set_model(model.get());
combo->set_current_index(1);  // Select 2560x1440
```

### Dynamic Model Updates

```cpp
auto model = std::make_shared<list_model<std::string, Backend>>();
model->set_items({"Option 1", "Option 2"});

auto combo = std::make_unique<combo_box<Backend>>();
combo->set_model(model.get());

// Later: Add more options (combo box updates automatically)
model->append("Option 3");
model->append("Option 4");

// Selection index remains valid if within new range
```

### Settings Panel

```cpp
auto panel = std::make_unique<vbox<Backend>>(2);

// Font size selector
auto font_sizes = std::make_shared<list_model<int, Backend>>();
font_sizes->set_items({8, 10, 12, 14, 16, 18, 20, 24});

auto font_combo = panel->template emplace_child<combo_box>();
font_combo->set_model(font_sizes.get());
font_combo->set_current_index(2);  // 12pt default

font_combo->current_index_changed.connect([&](int index) {
    int font_size = font_sizes->at(index);
    apply_font_size(font_size);
});

panel->emplace_child<label>("Font Size:");
panel->add_child(std::move(font_combo));
```

## Implementation Details

### Rendering

The combo box renders as a button-like widget with:
- Background box with border (uses `box_style` from theme)
- Current selection text (or "(select)" if empty)
- Dropdown indicator arrow "▼" on the right

### State Management

Inherits from `stateful_widget<Backend>` which provides:
- State tracking (normal/hovered/pressed/disabled)
- State-based color resolution
- Mouse interaction (hover detection, click handling)

### Model Integration

Uses `abstract_item_model<Backend>` interface:
- Calls `model->data(index, item_data_role::display)` to get text
- Supports any model implementing the interface
- No ownership of model (caller manages lifetime)

## Known Limitations

### Phase 2 Status

✅ **Implemented**:
- MVC model integration
- Keyboard navigation (arrow keys, Home/End)
- Selection tracking with signals
- State-based theming
- Visual rendering (button + arrow)

⚠️ **TODO (requires layer_manager integration)**:
- Popup list rendering on click
- Mouse selection from dropdown
- Click-outside-to-close behavior

### Current Workaround

For terminal UIs, keyboard navigation provides full functionality:
- Tab to focus combo box
- Use arrow keys to navigate options
- Current selection displayed in button

## Related

- **[MVC System Guide](../../guides/mvc-system.md)** - Complete MVC documentation
- **[list_view](./list-view.md)** - Scrollable list widget (used in popup)
- **[list_model](../../guides/mvc-system.md#1-models)** - Data model for combo box
- **[button](./button.md)** - Similar widget (combo box uses button theming)

## See Also

- **Header**: `include/onyxui/widgets/input/combo_box.hh`
- **Tests**: `unittest/widgets/test_combo_box.cc`
- **Demo**: `examples/demo_ui_builder.hh:206-225`
