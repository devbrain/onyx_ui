---
sidebar_position: 1
---

# Button

The `onyxui::button` is a standard clickable button widget. It's one of the most common interactive elements in a UI.

## Overview

A button is a simple widget that the user can click to trigger an action. It can display text and can be styled using the theming system.

## Key Features

-   **Clickable:** The button emits a `clicked` signal when the user clicks on it.
-   **Keyboard Activation:** When focused, the button can be activated via the Enter key (or any key bound to the `activate_focused` semantic action).
-   **Text Label:** It can display a simple text label.
-   **Themable:** The button's appearance can be customized using the `button_style` in the `ui_theme`.
-   **Stateful:** The button has different visual states for `normal`, `hover`, `pressed`, `disabled`, and `focused`.

## Usage

Here's a simple example of how to create a button and connect to its `clicked` signal:

```cpp
#include <onyxui/widgets/button.hh>
#include <iostream>

// ...

auto button = onyxui::create_button<Backend>("Click Me");

button->clicked().connect([]() {
    std::cout << "Button was clicked!" << std::endl;
});

// Add the button to a parent container
parent->add_child(std::move(button));
```

## Keyboard Interaction

Buttons are focusable and can be activated using the keyboard:

1. **Tab Navigation:** Press `Tab` to move focus between widgets until the button is focused.
2. **Activation:** Press `Enter` (or any key bound to `activate_focused` in the current hotkey scheme) to trigger the button's `clicked` signal.
3. **Visual Feedback:** When focused, the button displays the same visual state as when hovered.

### Customizing Activation Key

The activation key is configurable via hotkey schemes:

```cpp
// Users can customize which key activates focused buttons
auto& schemes = ctx.hotkey_schemes();
auto* scheme = schemes.get_scheme("Windows");
scheme->set_binding(hotkey_action::activate_focused,
                    parse_key_sequence("Space"));  // Use Space instead of Enter
```

## API Reference

### Template Parameters

-   `Backend`: The backend traits class.

### Public Methods

-   **`signal<>& clicked()`:** Returns a reference to the signal that is emitted when the button is clicked.
-   **`void set_text(std::string text)`:** Sets the text label of the button.
-   **`const std::string& text() const`:** Returns the text label of the button.

### Theming

The appearance of the button is controlled by the `button_style` struct in the `ui_theme`. You can customize the following properties:

-   `fg_normal`, `bg_normal`: Foreground and background colors for the normal state.
-   `fg_hover`, `bg_hover`: Colors for the hover state.
-   `fg_pressed`, `bg_pressed`: Colors for the pressed state.
-   `fg_disabled`, `bg_disabled`: Colors for the disabled state.
-   `box_style`: The style of the button's border.
-   `font`: The font used for the button's text.
-   `padding_horizontal`, `padding_vertical`: The padding around the button's text.
-   `text_align`: The alignment of the button's text.

By customizing these properties in your theme, you can create buttons that match the look and feel of your application.
