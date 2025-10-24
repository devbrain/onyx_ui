---
sidebar_position: 2
---

# The Layout System

OnyxUI's layout system is a powerful and flexible tool for arranging widgets on the screen. It's based on the **Strategy pattern**, which means you can plug in different layout algorithms to suit your needs. This guide provides an overview of the built-in layout strategies and how to use them.

## Core Concepts

-   **`layout_strategy`:** An abstract base class that defines the interface for all layout algorithms.
-   **`ui_element`:** The base class for all widgets. Each `ui_element` can have its own `layout_strategy`.
-   **Composition:** You can create complex layouts by nesting `ui_element`s with different layout strategies.

## Built-in Layout Strategies

OnyxUI comes with a number of built-in layout strategies to cover a wide range of use cases.

### `linear_layout`

The `linear_layout` is the most commonly used layout strategy. It arranges its children in a single row (horizontally) or a single column (vertically).

**Key Features:**

-   **Direction:** Can be `horizontal` or `vertical`.
-   **Spacing:** You can specify the spacing between children.
-   **Alignment:** You can control the alignment of children on the cross-axis (e.g., vertical alignment for a horizontal layout).

**Example: A Vertical Menu**

```cpp
auto menu = onyxui::create_vbox<Backend>(5); // 5px spacing
menu->set_horizontal_align(onyxui::horizontal_alignment::stretch);

auto button1 = onyxui::create_button<Backend>("File");
auto button2 = onyxui::create_button<Backend>("Edit");
auto button3 = onyxui::create_button<Backend>("View");

menu->add_child(std::move(button1));
menu->add_child(std::move(button2));
menu->add_child(std::move(button3));
```

### `grid_layout`

The `grid_layout` arranges its children in a grid of rows and columns.

**Key Features:**

-   **Rows and Columns:** You can specify the number of rows and columns.
-   **Cell Spanning:** Children can be configured to span multiple rows or columns.

**Example: A Simple Form**

```cpp
auto form = onyxui::create_grid<Backend>(2, 2); // 2 columns, 2 rows

form->add_child(onyxui::create_label<Backend>("Name:"));
form->add_child(onyxui::create_textbox<Backend>());

form->add_child(onyxui::create_label<Backend>("Email:"));
form->add_child(onyxui::create_textbox<Backend>());
```

### `anchor_layout`

The `anchor_layout` allows you to "anchor" children to the corners or edges of the parent.

**Key Features:**

-   **Anchor Points:** You can anchor children to points like `top_left`, `center`, `bottom_right`, etc.
-   **Offsets:** You can specify an offset from the anchor point.

**Example: A Dialog with OK/Cancel Buttons**

```cpp
auto dialog = onyxui::create_anchor_panel<Backend>();

auto ok_button = onyxui::create_button<Backend>("OK");
dialog->add_child(std::move(ok_button));
dialog->set_anchor(ok_button.get(), onyxui::anchor_point::bottom_right, {-10, -10});

auto cancel_button = onyxui::create_button<Backend>("Cancel");
dialog->add_child(std::move(cancel_button));
dialog->set_anchor(cancel_button.get(), onyxui::anchor_point::bottom_right, {-100, -10});
```

### `absolute_layout`

The `absolute_layout` allows you to position children at explicit coordinates within the parent.

**Key Features:**

-   **Explicit Positioning:** You specify the exact `x` and `y` coordinates for each child.
-   **Fixed Size:** Children in an `absolute_layout` are typically given a fixed size.

**Example: A Custom UI with Overlapping Elements**

```cpp
auto panel = onyxui::create_absolute_panel<Backend>();

auto image = onyxui::create_image<Backend>("background.png");
panel->add_child(std::move(image));
panel->set_position(image.get(), {0, 0});

auto title = onyxui::create_label<Backend>("My Awesome Game");
panel->add_child(std::move(title));
panel->set_position(title.get(), {100, 20});
```

## Layout Composition

The real power of OnyxUI's layout system comes from composition. You can create incredibly complex and flexible UIs by nesting elements with different layout strategies.

For example, you could have a `vbox` as your main window layout, with a `hbox` for the toolbar, a `grid` for the main content area, and an `anchor_panel` for a floating tool window.

By combining these simple, powerful layout strategies, you can build almost any UI you can imagine.
