---
sidebar_position: 1
---

# The Backend Pattern

The Backend Pattern is the cornerstone of OnyxUI's flexible and extensible architecture. It's the mechanism that allows the library to be completely independent of any specific rendering engine, windowing system, or event loop. This document provides a deep dive into how this pattern works and how you can leverage it to integrate OnyxUI with your own custom backends.

## The Core Idea

At its heart, the Backend Pattern is a form of template-based polymorphism. Instead of relying on virtual functions and inheritance for abstraction, OnyxUI uses a single template parameter, `Backend`, throughout its core classes. This `Backend` type is a "traits" class that provides all the platform-specific types and functionality that the library needs.

Here's a simplified example from the `ui_element` class:

```cpp
template<UIBackend Backend>
class ui_element {
    using rect_type = typename Backend::rect_type;
    using size_type = typename Backend::size_type;
    using renderer_type = typename Backend::renderer_type;
    // ...
};
```

As you can see, `ui_element` doesn't know or care about the concrete types for rectangles, sizes, or renderers. It simply uses the types provided by the `Backend` template parameter. This means that as long as you provide a `Backend` that satisfies the required interface, you can use OnyxUI with any rendering engine you choose.

## The `UIBackend` Concept

To ensure that any `Backend` provided to the library has the necessary types and functions, OnyxUI uses a C++20 concept named `UIBackend`. This concept acts as a compile-time contract, and any type that does not meet its requirements will result in a clear and easy-to-understand compiler error.

Here are the key requirements of the `UIBackend` concept:

-   **`rect_type`:** A type that represents a rectangle. It must be compatible with the `RectLike` concept.
-   **`size_type`:** A type that represents a size (width and height). It must be compatible with the `SizeLike` concept.
-   **`point_type`:** A type that represents a point (x and y). It must be compatible with the `PointLike` concept.
-   **`color_type`:** A type that represents a color. It must be compatible with the `ColorLike` concept.
-   **`event_type`:** A type that represents a UI event (e.g., mouse click, key press). It must be compatible with the `EventLike` concept.
-   **`renderer_type`:** A type that provides the rendering functionality. It must be compatible with the `RenderLike` concept.
-   **`register_themes(theme_registry<Backend>&)`:** A static function that registers the backend's default themes.

For the full details of these concepts, you can refer to the source code in `include/onyxui/concepts/`.

## How it Works in Practice

Let's take a look at how this pattern works in a real-world scenario. Imagine you have two different backends: `conio_backend` for terminal-based UIs and `sdl2_backend` for graphical UIs.

When you write your UI code, you can simply choose which backend you want to use at compile time:

```cpp
// For a terminal-based UI
using MyBackend = onyxui::conio_backend;

// For a graphical UI
using MyBackend = onyxui::sdl2_backend;

// Your UI code remains the same
auto window = onyxui::create_vbox<MyBackend>();
auto button = onyxui::create_button<MyBackend>("Click Me");
// ...
```

This is the power of the Backend Pattern: it allows you to write your UI logic once and then deploy it to different platforms and rendering engines with minimal changes.

## Creating a Custom Backend

If you want to integrate OnyxUI with your own rendering engine, you'll need to create a custom backend. Here's a high-level overview of the steps involved:

1.  **Define your backend struct:**

    ```cpp
    struct MyCustomBackend {
        // ...
    };
    ```

2.  **Provide the required types:**

    ```cpp
    struct MyCustomBackend {
        using rect_type = MyRect;
        using size_type = MySize;
        using point_type = MyPoint;
        using color_type = MyColor;
        using event_type = MyEvent;
        using renderer_type = MyRenderer;
    };
    ```

3.  **Implement the renderer:**

    Your `MyRenderer` class will need to implement the `RenderLike` concept, which includes functions for drawing rectangles, text, icons, etc.

    **Important:** Renderers must be **stateless** with respect to colors and styles. They should not store background/foreground colors internally. Instead, colors are passed to drawing methods or accessed from the render context's resolved style.

    ```cpp
    class MyRenderer {
    public:
        // Drawing methods take colors as parameters (stateless)
        void draw_text(const rect& bounds, std::string_view text,
                      const font& f, const color& fg, const color& bg);

        void draw_box(const rect& r, const box_style& style,
                     const color& fg, const color& bg);

        void draw_icon(const rect& r, const icon_style& icon,
                      const color& fg, const color& bg);

        void draw_background(const rect& viewport, const background_style& style);

        // Static methods for measurement (no instance needed)
        static size measure_text(std::string_view text, const font& f);
        static size get_icon_size(const icon_style& icon);
        static int get_border_thickness(const box_style& style);

        // ❌ WRONG: Don't store colors in renderer
        // color m_foreground_color;
        // color m_background_color;
        // void set_foreground_color(const color& c);  // No!
        // void set_background_color(const color& c);  // No!
    };
    ```

    **Why Stateless?**
    - **Thread Safety**: Renderers can be safely used from multiple threads
    - **Predictability**: No hidden state affects rendering output
    - **Testability**: Drawing methods produce identical output for identical inputs
    - **Clarity**: All visual properties come from the render context's style

4.  **Implement theme registration:**

    ```cpp
    struct MyCustomBackend {
        // ... types ...
        static void register_themes(onyxui::theme_registry<MyCustomBackend>& registry) {
            // Register your custom themes here
        }
    };
    ```

5.  **Ensure your types satisfy the concepts:**

    Make sure that your `MyRect`, `MySize`, etc., types satisfy the corresponding `...Like` concepts. This usually involves providing the necessary member variables (e.g., `x`, `y`, `width`, `height`).

By following these steps, you can create a custom backend that seamlessly integrates with the OnyxUI ecosystem.
