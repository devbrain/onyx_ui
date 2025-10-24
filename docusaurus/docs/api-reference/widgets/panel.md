---
sidebar_position: 3
---

# Panel

The `onyxui::panel` is a generic container widget. It's one of the most fundamental building blocks for creating complex layouts.

## Overview

A panel is a simple container that can hold other widgets. By default, it doesn't have a layout strategy, so its children will not be automatically arranged. To arrange the children of a panel, you must set a layout strategy.

## Key Features

-   **Container:** A panel can hold any number of child widgets.
-   **Layout Control:** You can set a layout strategy on a panel to control how its children are arranged.
-   **Themable:** The panel's appearance can be customized using the `panel_style` in the `ui_theme`.

## Usage

Here's an example of how to create a panel and set a `linear_layout` on it:

```cpp
#include <onyxui/widgets/panel.hh>
#include <onyxui/layout/linear_layout.hh>

// ...

auto panel = onyxui::create_panel<Backend>();

// Create a vertical layout with 5px spacing
auto layout = std::make_unique<onyxui::linear_layout<Backend>>(
    onyxui::direction::vertical,
    5
);

// Set the layout strategy on the panel
panel->set_layout_strategy(std::move(layout));

// Add children to the panel
panel->add_child(onyxui::create_button<Backend>("Button 1"));
panel->add_child(onyxui::create_button<Backend>("Button 2"));

// Add the panel to a parent container
parent->add_child(std::move(panel));
```

## API Reference

### Template Parameters

-   `Backend`: The backend traits class.

### Public Methods

-   **`void set_layout_strategy(layout_strategy_ptr strategy)`:** Sets the layout strategy for the panel.
-   **`void add_child(ui_element_ptr child)`:** Adds a child widget to the panel.
-   **`ui_element_ptr remove_child(ui_element* child)`:** Removes a child widget from the panel.

### Theming

The appearance of the panel is controlled by the `panel_style` struct in the `ui_theme`. You can customize the following properties:

-   `background`: The background color of the panel.
-   `border_color`: The color of the panel's border.
-   `box_style`: The style of the panel's border.
-   `has_border`: Whether the panel has a border.

By customizing these properties in your theme, you can create panels that match the look and feel of your application.
