---
sidebar_position: 5
---

# Grid

The `onyxui::grid` is a container widget that arranges its children in a grid of rows and columns.

## Overview

A `grid` is a powerful container that allows you to create complex, two-dimensional layouts. It's ideal for forms, tables, and other UIs that require a structured arrangement of widgets.

## Key Features

-   **Rows and Columns:** You can specify the number of rows and columns in the constructor.
-   **Cell Spanning:** Children can be configured to span multiple rows or columns.
-   **Automatic Flow:** Children are added to the grid in a left-to-right, top-to-bottom order.

## Usage

Here's an example of how to use a `grid` to create a simple login form:

```cpp
#include <onyxui/widgets/grid.hh>
#include <onyxui/widgets/label.hh>
#include <onyxui/widgets/textbox.hh>
#include <onyxui/widgets/button.hh>

// ...

// Create a grid with 2 columns and 3 rows
auto form = onyxui::create_grid<Backend>(2, 3);

// Add a username label and textbox
form->add_child(onyxui::create_label<Backend>("Username:"));
form->add_child(onyxui::create_textbox<Backend>());

// Add a password label and textbox
form->add_child(onyxui::create_label<Backend>("Password:"));
form->add_child(onyxui::create_textbox<Backend>());

// Add a login button that spans both columns
auto login_button = onyxui::create_button<Backend>("Login");
form->add_child(std::move(login_button));
form->set_cell_span(login_button.get(), 2, 1); // Span 2 columns, 1 row

// Add the form to a parent container
parent->add_child(std::move(form));
```

## API Reference

### Template Parameters

-   `Backend`: The backend traits class.

### Factory Functions

-   **`create_grid<Backend>(int columns, int rows)`:** Creates a `grid` with the specified number of columns and rows.

### Public Methods

-   **`void add_child(ui_element_ptr child)`:** Adds a child widget to the next available cell in the grid.
-   **`void set_cell_span(ui_element* child, int col_span, int row_span)`:** Sets the column and row span for a specific child.

### Theming

A `grid` is a panel, so its appearance is controlled by the `panel_style` struct in the `ui_theme`. You can customize its background color, border, etc., just like a regular panel.
