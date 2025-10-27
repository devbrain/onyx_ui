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

### Global Theming with CSS-Style Inheritance

**OnyxUI uses a global theming architecture:** themes are applied **once at the root level**, and all children inherit from this global theme via CSS-style property inheritance.

This design provides several benefits:
- **Simplicity**: One theme per UI tree, applied at root
- **Consistency**: Uniform styling across all widgets
- **Performance**: O(depth) style resolution with optimized caching
- **Flexibility**: Individual widgets can override colors while maintaining theme coherence

```cpp
// Apply theme ONCE at the root level (global theming)
auto root = std::make_unique<panel<Backend>>();
root->apply_theme("Norton Blue", ctx.themes());

// All children automatically inherit from the global theme
auto button = root->emplace_child<button>();
auto label = root->emplace_child<label>();
// Both button and label use "Norton Blue" theme automatically!

// Individual widgets can still override specific properties
button->set_background_color({0, 255, 0});  // Green button with Norton Blue text
```

**Important:** Do not call `apply_theme()` on child widgets. The theme is global and inherited automatically through the parent chain.

### The `theme_registry`

The `onyxui::theme_registry` is a global, thread-safe singleton that stores all the available themes for a given backend. When you create your first `ui_context`, the backend automatically registers its default themes with the registry.

You can then access these themes by name:

```cpp
auto* theme = theme_registry.get_theme("Norton Blue");
```

## Applying Themes

The v2.0 refactoring introduced a new, safer API for applying themes with **global theming in mind**. There are three ways to apply a theme, but **all should be used at the root level only**:

1.  **By Name (Recommended):** This is the most common and efficient way to apply a global theme. The theme is looked up in the `theme_registry`, and the root widget stores a reference to it.

    ```cpp
    // Apply to ROOT only
    root->apply_theme("Norton Blue", ctx.themes());
    // All children inherit automatically via CSS
    ```

2.  **By Value (Move):** You can create a custom theme and move it to the root. The root then owns the global theme.

    ```cpp
    // Apply to ROOT only
    ui_theme<Backend> my_theme = create_custom_theme();
    root->apply_theme(std::move(my_theme));
    // All children inherit automatically via CSS
    ```

3.  **By `shared_ptr`:** Less common, but useful for custom themes not in the registry.

    ```cpp
    // Apply to ROOT only
    auto theme_ptr = std::make_shared<ui_theme<Backend>>(my_theme);
    root->apply_theme(theme_ptr);
    // All children inherit automatically via CSS
    ```

**Important Guidelines:**
- Apply themes **only to the root element** of your UI tree
- Do **not** call `apply_theme()` on child widgets
- All children automatically inherit from the global theme via CSS-style property inheritance
- Individual widgets can override specific properties using `set_background_color()`, `set_foreground_color()`, etc.

This API is much safer than the old one, as it eliminates the risk of dangling pointers and makes ownership semantics clear.

## Style Resolution

To ensure high performance, OnyxUI uses an efficient style resolution mechanism optimized for **global theming**. Instead of each widget querying the theme for its style properties on every frame, the style is resolved once per frame for each widget.

This is done through the `resolve_style()` method, which implements CSS-style inheritance with **O(depth) complexity**:

1. Resolves the parent's style **once** (single cache prevents exponential recursion)
2. Walks up the tree to find the global theme (if needed)
3. Calculates the final `resolved_style` using the inheritance chain:
   - **Explicit override** → **Parent's resolved style** → **Global theme** → **Default**

The resulting `resolved_style` is a simple POD-like struct (~50-100 bytes) that contains all visual properties the widget needs to render itself. It is then passed to the `render_context`, making it available during the `do_render()` call.

This approach has several advantages:

-   **Performance:** O(depth) style resolution with optimized parent caching
-   **Scalability:** Tested with 100+ widget trees (both wide and deep)
-   **Simplicity:** Widgets receive a pre-resolved style, which simplifies their rendering logic
-   **Safety:** No exponential recursion risk (parent resolved once per widget)
-   **Consistency:** Single source of truth for a widget's visual properties

## Widget Property Access Pattern (Phase 6 - October 2025)

The latest enhancement to the theming system introduced a **two-tier property access pattern** that optimizes performance while maintaining flexibility.

### `resolved_style` with Optional Properties

The `resolved_style` structure now includes optional widget-specific properties wrapped in strong-typed containers:

```cpp
template<UIBackend Backend>
struct resolved_style {
    // Required properties (always present)
    color_type background_color;
    color_type foreground_color;
    color_type border_color;
    box_style_type box_style;
    font_type font;
    float opacity;
    std::optional<icon_style_type> icon_style;

    // Optional widget-specific properties
    padding_horizontal_t padding_horizontal;  // std::optional<int> internally
    padding_vertical_t padding_vertical;      // std::optional<int> internally
    mnemonic_font_t mnemonic_font;            // std::optional<font_type> internally
};
```

### Theme Pointer in Render Context

For **rare widget-specific properties** that don't belong in the common `resolved_style` (like `text_align`, `line_style`), widgets access the theme directly via `ctx.theme()`:

```cpp
template<UIBackend Backend>
class render_context {
    // Access resolved style (common properties - O(1))
    [[nodiscard]] const resolved_style<Backend>& style() const noexcept;

    // Access theme for rare properties (nullable)
    [[nodiscard]] const ui_theme<Backend>* theme() const noexcept;
};
```

### Two-Tier Access in Widgets

All widgets in the framework follow this pattern:

```cpp
template<UIBackend Backend>
class button : public stateful_widget<Backend> {
    void do_render(render_context<Backend>& ctx) const override {
        // Tier 1: Common properties via pre-resolved style (O(1) access)
        auto const& fg = ctx.style().foreground_color;
        auto const& font = ctx.style().font;

        // Optional properties with explicit defaults
        int padding = ctx.style().padding_horizontal.value.value_or(2);

        // Tier 2: Rare properties via theme pointer (nullable)
        if (auto* theme = ctx.theme()) {
            auto text_align = theme->button.text_align;
        }

        // IMPORTANT: Never directly access ui_services::themes() from widgets!
        // Always use ctx.style() or ctx.theme()
    }
};
```

### Why Two Tiers?

This architecture balances performance and flexibility:

- **Common properties** (colors, fonts, padding) → `resolved_style` → **O(1) access**, no theme lookup
- **Rare properties** (text_align, line_style) → `ctx.theme()` → **Minimal overhead**, avoids bloating `resolved_style`
- **Performance**: Style resolved once per frame, not per widget property access
- **Type Safety**: Optional properties enforce explicit default handling via `.value.value_or(default)`

### Benefits of Phase 6

✅ **Zero Theme Access from Widgets**: All widgets refactored to eliminate direct `ui_services::themes()` calls

✅ **Lazy Mnemonic Parsing**: Button, label, and menu_item parse mnemonics on-demand during render with caching

✅ **Type-Safe Optionals**: Strong-typed wrappers prevent accidental misuse of optional properties

✅ **Better Performance**: Common properties accessed without theme lookup

✅ **Architectural Clarity**: Clear separation between common and rare properties

## Conclusion

OnyxUI's theming system is a powerful and flexible tool for customizing the appearance of your application. By leveraging its CSS-style inheritance, thread-safe API, efficient style resolution, and two-tier property access pattern, you can create beautiful and consistent user interfaces with optimal performance.
