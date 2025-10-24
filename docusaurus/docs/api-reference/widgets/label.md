---
sidebar_position: 2
---

# Label

The `onyxui::label` is a simple widget for displaying text.

## Overview

A label is a non-interactive widget that is used to display a string of text. It's a fundamental building block for most user interfaces.

## Key Features

-   **Text Display:** The primary purpose of a label is to display text.
-   **Themable:** The label's appearance can be customized using the `label_style` in the `ui_theme`.
-   **Mnemonic Support:** Labels can display a mnemonic character (typically underlined) to indicate a keyboard shortcut.

## Usage

Here's a simple example of how to create a label:

```cpp
#include <onyxui/widgets/label.hh>

// ...

auto label = onyxui::create_label<Backend>("Hello, World!");

// Add the label to a parent container
parent->add_child(std::move(label));
```

## API Reference

### Template Parameters

-   `Backend`: The backend traits class.

### Public Methods

-   **`void set_text(std::string text)`:** Sets the text of the label.
-   **`const std::string& text() const`:** Returns the text of the label.

### Theming

The appearance of the label is controlled by the `label_style` struct in the `ui_theme`. You can customize the following properties:

-   `text`: The color of the label's text.
-   `background`: The background color of the label.
-   `font`: The font used for the label's text.
-   `mnemonic_font`: The font used for the mnemonic character.

By customizing these properties in your theme, you can create labels that match the look and feel of your application.
