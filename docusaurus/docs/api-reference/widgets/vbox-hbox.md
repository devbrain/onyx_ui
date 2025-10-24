---
sidebar_position: 4
---

# VBox and HBox

The `onyxui::vbox` and `onyxui::hbox` are convenient container widgets that use a `linear_layout` to arrange their children.

## Overview

`vbox` and `hbox` are specialized panels that are pre-configured with a `linear_layout`. They are some of the most common containers used for building UIs.

-   **`vbox`:** Arranges its children in a single vertical column.
-   **`hbox`:** Arranges its children in a single horizontal row.

## Key Features

-   **Pre-configured:** These widgets save you the step of creating a `panel` and then setting a `linear_layout` on it.
-   **Spacing:** You can specify the spacing between children in the constructor.
-   **Alignment:** You can control the alignment of children on the main and cross axes.

## Usage

Here's an example of how to use a `vbox` and an `hbox` to create a simple layout:

```cpp
#include <onyxui/widgets/vbox.hh>
#include <onyxui/widgets/hbox.hh>
#include <onyxui/widgets/button.hh>

// ...

// Create a vertical box with 10px spacing
auto main_window = onyxui::create_vbox<Backend>(10);

// Create a horizontal box for a toolbar
auto toolbar = onyxui::create_hbox<Backend>(5);
toolbar->add_child(onyxui::create_button<Backend>("File"));
toolbar->add_child(onyxui::create_button<Backend>("Edit"));
toolbar->add_child(onyxui::create_button<Backend>("Help"));

// Add the toolbar to the main window
main_window->add_child(std::move(toolbar));

// Add a main content area
auto content_area = onyxui::create_panel<Backend>();
main_window->add_child(std::move(content_area));
```

## API Reference

### Template Parameters

-   `Backend`: The backend traits class.

### Factory Functions

-   **`create_vbox<Backend>(int spacing = 0)`:** Creates a `vbox` with the specified spacing.
-   **`create_hbox<Backend>(int spacing = 0)`:** Creates an `hbox` with the specified spacing.

### Public Methods

Since `vbox` and `hbox` are panels, they have all the same public methods as `onyxui::panel`, including:

-   **`void add_child(ui_element_ptr child)`:** Adds a child widget.
-   **`ui_element_ptr remove_child(ui_element* child)`:** Removes a child widget.

### Theming

`vbox` and `hbox` are panels, so their appearance is controlled by the `panel_style` struct in the `ui_theme`. You can customize their background color, border, etc., just like a regular panel.
