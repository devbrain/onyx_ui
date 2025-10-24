---
sidebar_position: 3
---

# The Render Context Pattern

The Render Context Pattern is one of the most innovative features of OnyxUI's architecture. It's a powerful application of the Visitor design pattern that unifies the logic for measuring and rendering widgets. This unification eliminates a significant amount of duplicate code and prevents a common class of bugs where the measurement and rendering of a widget become desynchronized.

## The Problem: Duplicated Logic

In many traditional UI toolkits, a widget has two distinct responsibilities:

1.  **To calculate its own size:** This is often done in a method like `get_preferred_size()`.
2.  **To render itself:** This is done in a method like `paint()` or `draw()`.

The problem with this approach is that the logic for these two operations is often very similar. For example, to calculate the width of a button, you need to measure the width of its text, add some padding, and maybe the width of an icon. To render the button, you do the exact same calculations to determine where to place the text and icon.

This duplicated logic is a breeding ground for bugs. If a developer changes the rendering logic (e.g., adds a border), they must remember to also update the measurement logic to match. If they forget, the widget will report an incorrect size, leading to layout issues that can be difficult to debug.

## The Solution: A Unified Interface

OnyxUI solves this problem by unifying the measurement and rendering logic into a single method: `do_render()`. This method takes a `render_context` as an argument, which is an abstract base class that defines a set of drawing operations.

```cpp
template<UIBackend Backend>
class my_widget : public widget<Backend> {
    void do_render(render_context<Backend>& ctx) const override {
        // This single block of code handles both
        // measurement and rendering!
        auto text_size = ctx.draw_text(m_text, position, font, color);
        ctx.draw_rect(bounds, box_style);
        ctx.draw_icon(icon, icon_position);
    }
};
```

## The Visitors: `draw_context` and `measure_context`

The magic of this pattern lies in the two concrete implementations of `render_context`:

1.  **`draw_context`:** This is the "real" rendering context. When `do_render()` is called with a `draw_context`, the drawing operations are forwarded to the backend renderer, and the widget is drawn to the screen.

2.  **`measure_context`:** This is a "fake" rendering context. When `do_render()` is called with a `measure_context`, the drawing operations don't actually draw anything. Instead, they simply track the bounding box of the drawing operations. After `do_render()` returns, the `measure_context` can be queried to get the total size of the content that *would have been* drawn.

The base `widget` class handles this for you automatically. Its `get_content_size()` method is implemented like this:

```cpp
// A simplified look at the base widget's implementation
size_type get_content_size() const override {
    measure_context<Backend> ctx;
    this->do_render(ctx); // "Draws" the widget to the measure_context
    return ctx.get_size(); // Returns the calculated size
}
```

## Benefits of the Render Context Pattern

This pattern provides a number of significant benefits:

-   **Single Source of Truth:** The measurement and rendering logic for a widget are defined in a single place. This makes it impossible for them to become desynchronized.
-   **Reduced Code Duplication:** You no longer need to write and maintain two separate blocks of code for measuring and rendering.
-   **Improved Maintainability:** When you need to change the appearance of a widget, you only need to modify the `do_render()` method. The measurement logic is updated automatically.
-   **Type Safety:** The use of the Visitor pattern and C++ templates ensures that all operations are type-safe and that you can't accidentally mix up a `draw_context` and a `measure_context`.

## Advanced Features

### Const-Correct Renderer Access

The render context provides const-correct access to the underlying renderer:

```cpp
void my_const_method(const render_context<Backend>& ctx) const {
    // Access renderer for read-only operations
    const auto* renderer = ctx.renderer();
    if (renderer) {
        // Query capabilities or read state
    }
}
```

Both `const` and non-`const` overloads of `renderer()` are available, allowing read-only access from const methods while maintaining full API completeness.

### Exception Safety Guarantees

The render context system is designed with strong exception safety in mind:

**Zero-Copy Text Rendering:**
- All text operations use `std::string_view` to avoid unnecessary allocations
- No `bad_alloc` exceptions in the hot rendering path
- Performance improvement: ~10-15% faster text rendering

**Move Semantics for Dirty Regions:**
```cpp
// Copy when you need to keep the original
std::vector<rect_type> regions = compute_dirty_regions();
ctx.set_dirty_regions(regions);  // May throw bad_alloc

// Move for optimal performance (noexcept)
ctx.set_dirty_regions(std::move(regions));  // No exceptions
ctx.set_dirty_regions(compute_dirty_regions());  // Automatic move
```

**Exception Safety Levels:**
- **No-throw guarantee:** Move operations are marked `noexcept`
- **Strong guarantee:** Text rendering has no allocations in critical path
- **Basic guarantee:** Copy operations are well-defined and documented

### Accessing the Resolved Style

The render context carries a pre-resolved style that includes all visual properties after CSS inheritance:

```cpp
void do_render(render_context<Backend>& ctx) const override {
    // Access the resolved style
    const auto& style = ctx.style();

    // Use pre-resolved properties
    ctx.draw_text(m_text, position, style.font, style.foreground_color);
    ctx.draw_rect(bounds, style.box_style);

    // No repeated theme lookups needed!
}
```

The style is resolved **once per frame** through the theme hierarchy, eliminating the need for repeated `get_effective_*()` calls and improving performance.

### Dirty Region Optimization

The render context supports dirty region tracking for optimized rendering:

```cpp
// Set dirty regions for incremental rendering
ctx.set_dirty_regions(dirty_rects);

// Check if a widget should render
if (ctx.should_render(widget_bounds)) {
    // Only render widgets that intersect dirty regions
    widget->do_render(ctx);
}
```

This allows the framework to skip rendering widgets that haven't changed, dramatically improving performance for large UI hierarchies.

## Conclusion

The Render Context Pattern is a powerful and elegant solution to a common problem in UI toolkit design. By understanding this pattern, you can write widgets that are more robust, easier to maintain, and less prone to bugs.

With its recent improvements including const-correctness, strong exception safety guarantees, and zero-copy optimizations, the render context system provides a high-performance, type-safe foundation for building sophisticated user interfaces.
