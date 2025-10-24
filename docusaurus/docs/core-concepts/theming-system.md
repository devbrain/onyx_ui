---
sidebar_position: 5
---

# The Theming System

OnyxUI features a powerful and flexible theming system that allows you to control the visual appearance of your application with ease. The system is designed to be intuitive, leveraging a CSS-style inheritance model that makes it simple to create consistent and beautiful user interfaces.

The theming system was recently overhauled in the v2.0 refactoring, introducing a thread-safe API, clearer ownership semantics, and a more efficient style resolution mechanism.

## Core Concepts

### `ui_theme`

The `onyxui::ui_theme` struct is the heart of the theming system. It's a simple struct that contains all the style information for the various widgets in the library.

Here's a simplified look at its structure:

```cpp
template<UIBackend Backend>
struct ui_theme {
    using color_type = typename Backend::color_type;
    // ... other backend-specific types

    struct button_style {
        color_type fg_normal;
        color_type bg_normal;
        // ... styles for hover, pressed, disabled states
    };

    struct panel_style {
        color_type background;
        color_type border_color;
    };

    // ... styles for other widgets

    button_style button;
    panel_style panel;
    // ...
};
```

Each backend provides its own concrete implementation of `ui_theme`, tailored to the capabilities of its rendering engine.

### CSS-Style Inheritance

One of the most powerful features of the theming system is its CSS-style inheritance model. When you apply a theme to a widget, that theme is automatically inherited by all of its children. This makes it incredibly easy to create a consistent look and feel across your entire application.

```cpp
// Apply a theme to the root window
window->apply_theme("My Cool Theme", theme_registry);

// All children of the window will automatically use "My Cool Theme"
auto button = onyxui::create_button<Backend>("Click Me");
window->add_child(std::move(button)); // This button will be styled by the theme
```

You can, of course, override the theme for any specific widget or sub-tree of your UI, allowing for a high degree of customization.

### The `theme_registry`

The `onyxui::theme_registry` is a global, thread-safe singleton that stores all the available themes for a given backend. When you create your first `ui_context`, the backend automatically registers its default themes with the registry.

You can then access these themes by name:

```cpp
auto* theme = theme_registry.get_theme("Norton Blue");
```

## Applying Themes

The v2.0 refactoring introduced a new, safer API for applying themes. There are three ways to apply a theme to a widget:

1.  **By Name (Recommended):** This is the most common and efficient way to apply a theme. The theme is looked up in the `theme_registry`, and the widget stores a reference to it.

    ```cpp
    element->apply_theme("Norton Blue", registry);
    ```

2.  **By Value (Move):** You can create a custom theme on the fly and move it into the widget. The widget then takes ownership of the theme.

    ```cpp
    ui_theme<Backend> my_theme = create_custom_theme();
    element->apply_theme(std::move(my_theme));
    ```

3.  **By `shared_ptr`:** If you need to share a theme between multiple widgets without using the registry, you can use a `shared_ptr`.

    ```cpp
    auto theme_ptr = std::make_shared<ui_theme<Backend>>(my_theme);
    element->apply_theme(theme_ptr);
    ```

This new API is much safer than the old one, as it eliminates the risk of dangling pointers and makes ownership semantics clear.

## Style Resolution

To ensure high performance, OnyxUI uses an efficient style resolution mechanism. Instead of each widget querying the theme for its style properties on every frame, the style is resolved once per frame for each widget.

This is done through the `resolve_style()` method, which traverses up the UI tree to find the nearest theme and then calculates the final `resolved_style` for the widget. This `resolved_style` is a simple POD-like struct that contains all the visual properties the widget needs to render itself. It is then passed to the `render_context`, making it available to the widget during the `do_render()` call.

This approach has several advantages:

-   **Performance:** Style resolution is done only once per frame, not for every drawing operation.
-   **Simplicity:** Widgets receive a pre-resolved style, which simplifies their rendering logic.
-   **Consistency:** It provides a single source of truth for a widget's visual properties.

## Conclusion

OnyxUI's theming system is a powerful and flexible tool for customizing the appearance of your application. By leveraging its CSS-style inheritance, thread-safe API, and efficient style resolution, you can create beautiful and consistent user interfaces with ease.
