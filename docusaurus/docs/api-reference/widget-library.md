---
sidebar_position: 1
---

# Widget Library

OnyxUI comes with a rich library of widgets that you can use to build your user interfaces. This section provides an overview of the available widgets and links to more detailed documentation for each one.

All widgets in OnyxUI inherit from the `onyxui::ui_element` class, which means they all participate in the two-pass layout system and the `event_target` event system.

## Widget Categories

The widget library can be broadly divided into a few categories:

### Containers

These are widgets that are designed to hold other widgets. They are the building blocks of your UI's layout.

-   **`panel`:** A generic container with a simple layout.
-   **`vbox` & `hbox`:** Containers that arrange their children in a vertical or horizontal line.
-   **`grid`:** A container that arranges its children in a grid.
-   **`anchor_panel`:** A container that allows you to anchor children to its edges and corners.
-   **`absolute_panel`:** A container that allows you to position children at absolute coordinates.
-   **`group_box`:** A container with a border and a title.

### Controls

These are the interactive widgets that the user can interact with.

-   **`button`:** A standard clickable button.
-   **`label`:** A widget for displaying text.

### Input Widgets

These widgets allow users to enter and edit data.

-   **`line_edit`:** Single-line text input with cursor, selection, and horizontal scrolling.
-   **`checkbox`:** Toggleable boolean/tri-state input with themed icons and mnemonic support.
-   **`radio_button`:** Mutually exclusive selection control with button group management.

### Spacing

These are widgets that are used to control the spacing and alignment of other widgets.

-   **`spacer`:** A widget that takes up a fixed amount of space.
-   **`spring`:** A widget that expands to fill available space.

### Scrolling

These widgets provide comprehensive scrolling functionality with three levels of abstraction.

-   **`scroll_view`:** High-level convenience wrapper combining scrollable, scrollbars, and controller (recommended).
-   **`scrollable`:** Core scrolling logic and viewport management.
-   **`scrollbar`:** Visual scrollbar widget with mouse interaction.

### Advanced Widgets

These are more complex widgets that are used for specific purposes.

-   **`menu_bar`, `menu`, `menu_item`:** A set of widgets for creating menus.
-   **`status_bar`:** A widget for displaying status information at the bottom of a window.

## Detailed Widget Reference

For more detailed information on each widget, including its API and usage examples, please refer to the individual widget pages:

**Basic Widgets:**
-   **[Button](./widgets/button.md)**
-   **[Label](./widgets/label.md)**

**Input Widgets:**
-   **[Line Edit](./widgets/line-edit.md)** - Single-line text input
-   **[Checkbox](./widgets/checkbox.md)** - Boolean/tri-state toggle input
-   **[Radio Button](./widgets/radio-button.md)** - Mutually exclusive selection control

**Layout Containers:**
-   **[Panel](./widgets/panel.md)**
-   **[VBox and HBox](./widgets/vbox-hbox.md)**
-   **[Grid](./widgets/grid.md)**

**Scrolling Widgets:**
-   **[scroll_view](./widgets/scroll-view.md)**
-   **[scrollable](./widgets/scrollable.md)**
-   **[scrollbar](./widgets/scrollbar.md)**
