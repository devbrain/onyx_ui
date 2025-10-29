---
sidebar_position: 4
---

# Scrolling System

OnyxUI provides a comprehensive scrolling system with three levels of abstraction to suit different use cases.

## Overview

The scrolling system uses a three-layer architecture:

```
┌─────────────────────────────────┐
│       scroll_view (wrapper)      │  ← High-level (recommended)
├─────────────────────────────────┤
│  ┌──────────┐  ┌────────────┐  │
│  │scrollable│←→│  scrollbar │  │  ← Manual composition
│  │ (logic)  │  │  (visual)  │  │
│  └──────────┘  └────────────┘  │
│        ↑             ↑           │
│        └──controller─┘           │  ← Coordination layer
└─────────────────────────────────┘
```

### Components

1. **scroll_view** - High-level convenience wrapper (recommended for most use cases)
2. **scrollable** - Core scrolling logic and viewport management
3. **scrollbar** - Visual scrollbar widget
4. **scroll_controller** - Bidirectional synchronization between scrollable and scrollbars

## Quick Start

The easiest way to add scrolling is using preset factory functions:

```cpp
#include <onyxui/widgets/scroll_view_presets.hh>

// Modern look with auto-hiding scrollbars
auto view = modern_scroll_view<Backend>();
view->add_child(create_my_content());

// That's it! Scrollbars appear automatically when needed
```

### Available Presets

| Preset | Scrollbars | Best For |
|--------|-----------|----------|
| `modern_scroll_view()` | Auto-hide | Modern apps, clean UI |
| `classic_scroll_view()` | Always visible | Traditional desktop apps |
| `compact_scroll_view()` | Auto-hide | Space-constrained layouts |
| `vertical_only_scroll_view()` | Vertical only | Lists, documents, feeds |

## Basic Usage

### Adding Content

scroll_view forwards methods to its internal scrollable:

```cpp
auto view = modern_scroll_view<Backend>();

// Add children like any container
view->add_child(std::make_unique<label<Backend>>("Item 1"));
view->add_child(std::make_unique<label<Backend>>("Item 2"));

// Or emplace directly
view->emplace_child<button>("Click me");
```

### Setting Layout

For multiple children, use a layout strategy:

```cpp
auto view = modern_scroll_view<Backend>();

// Vertical list
view->set_layout_strategy(
    std::make_unique<linear_layout<Backend>>(direction::vertical, 5)  // 5px spacing
);

// Add many items
for (int i = 0; i < 100; ++i) {
    view->emplace_child<label>("Item " + std::to_string(i));
}
```

### Programmatic Scrolling

```cpp
// Scroll to absolute position
view->scroll_to(0, 100);  // x=0, y=100

// Scroll by relative delta
view->scroll_by(0, 50);   // Move down 50 pixels

// Scroll a widget into view
auto* widget = view->emplace_child<label>("Bottom item");
view->scroll_into_view(widget);  // Ensures widget is visible
```

## Configuration

### Scrollbar Visibility

```cpp
auto view = modern_scroll_view<Backend>();

// Set both axes
view->set_scrollbar_policy(scrollbar_visibility::always);

// Set independently
view->set_scrollbar_policy(
    scrollbar_visibility::auto_hide,  // horizontal
    scrollbar_visibility::always      // vertical
);

// Enable/disable axes
view->set_horizontal_scroll_enabled(false);  // Disable horizontal
view->set_vertical_scroll_enabled(true);     // Enable vertical
```

**Visibility Options:**

- `always` - Always visible, even if not needed
- `auto_hide` - Visible when content exceeds viewport
- `hidden` - Never visible (programmatic scroll only)

## Real-World Examples

### Vertical List (Log Viewer)

```cpp
auto view = vertical_only_scroll_view<Backend>();
view->set_layout_strategy(
    std::make_unique<linear_layout<Backend>>(direction::vertical, 2)
);

// Add log entries
for (const auto& entry : log_entries) {
    view->emplace_child<label>(entry.message);
}

// Auto-scroll to bottom (newest log)
auto info = view->content()->get_scroll_info();
int max_y = info.content_size.h - info.viewport_size.h;
view->scroll_to(0, max_y);
```

### Settings Panel

```cpp
auto view = modern_scroll_view<Backend>();
view->set_layout_strategy(
    std::make_unique<linear_layout<Backend>>(direction::vertical, 10)
);

// Add sections
view->emplace_child<label>("Display Settings");
view->emplace_child<checkbox>("Enable dark mode");
view->emplace_child<checkbox>("Show line numbers");

view->emplace_child<label>("Editor Settings");
view->emplace_child<slider>("Font size");
// ... many more settings ...

// Scrollbars appear automatically when content exceeds viewport
```

### Data Grid

```cpp
auto view = classic_scroll_view<Backend>();  // Always-visible scrollbars

auto grid = std::make_unique<grid<Backend>>(5);  // 5 columns

// Add headers
grid->emplace_child<label>("ID");
grid->emplace_child<label>("Name");
grid->emplace_child<label>("Date");
grid->emplace_child<label>("Status");
grid->emplace_child<label>("Actions");

// Add 100 rows of data
for (int i = 0; i < 100; ++i) {
    grid->emplace_child<label>(std::to_string(i));
    grid->emplace_child<label>("Item " + std::to_string(i));
    grid->emplace_child<label>("2025-10-29");
    grid->emplace_child<label>("Active");
    grid->emplace_child<button>("Edit");
}

view->add_child(std::move(grid));
```

## Advanced Usage

### Manual Composition

For custom layouts requiring precise control:

```cpp
// Create components
auto scrollable = std::make_unique<scrollable<Backend>>();
auto vscrollbar = std::make_unique<scrollbar<Backend>>(orientation::vertical);
auto hscrollbar = std::make_unique<scrollbar<Backend>>(orientation::horizontal);

// Connect with controller (bidirectional sync)
auto controller = std::make_unique<scroll_controller<Backend>>(
    scrollable.get(),
    vscrollbar.get(),
    hscrollbar.get()
);

// Add to your custom layout
my_container->add_child(std::move(scrollable));
my_container->add_child(std::move(vscrollbar));
my_container->add_child(std::move(hscrollbar));
```

### Accessing Internals

For power users who need direct access:

```cpp
auto view = modern_scroll_view<Backend>();

// Access components
auto* scrollable = view->content();             // The scrollable area
auto* vscrollbar = view->vertical_scrollbar();  // Vertical scrollbar
auto* hscrollbar = view->horizontal_scrollbar();// Horizontal scrollbar
auto* controller = view->controller();          // Scroll controller

// Listen to scroll events
view->content()->scroll_changed.connect([](const auto& offset) {
    std::cout << "Scrolled to: " << offset.y << "\n";
});
```

## Mouse & Keyboard

### Mouse Wheel (Automatic)

Mouse wheel scrolling is handled automatically by scrollable:
- Wheel up/down: Vertical scroll
- Shift + wheel: Horizontal scroll (if enabled)

### Keyboard Navigation

Connect keyboard events for custom scroll controls:

```cpp
view->key_pressed.connect([view_ptr](const auto& event) {
    switch (event.key) {
        case key::page_down:
            view_ptr->scroll_by(0, viewport_height);
            break;
        case key::page_up:
            view_ptr->scroll_by(0, -viewport_height);
            break;
        case key::home:
            view_ptr->scroll_to(0, 0);
            break;
        case key::end:
            auto info = view_ptr->content()->get_scroll_info();
            int max_y = info.content_size.h - info.viewport_size.h;
            view_ptr->scroll_to(0, max_y);
            break;
    }
});
```

## Performance

The scrolling system is optimized for large content:

- **Viewport Clipping**: Only visible items are rendered (O(visible) not O(total))
- **Large Content**: Tested with 10,000+ items in linear_layout
- **Smooth Scrolling**: Sub-pixel precision for smooth animation support
- **Dirty Regions**: Scrolling triggers minimal redraws
- **Memory**: O(total items) for widget tree, O(visible) for rendering

## Common Patterns

### Scroll to Bottom on New Content

```cpp
void add_log_entry(scroll_view<Backend>* view, const std::string& msg) {
    view->emplace_child<label>(msg);

    // Remeasure
    view->measure(width, height);
    view->arrange(bounds);

    // Scroll to bottom
    auto info = view->content()->get_scroll_info();
    int max_y = info.content_size.h - info.viewport_size.h;
    view->scroll_to(0, std::max(0, max_y));
}
```

### Keep Scroll Position When Updating

```cpp
void refresh_content(scroll_view<Backend>* view) {
    // Save current position
    auto old_offset = view->content()->get_scroll_offset();

    // Update content
    view->clear_children();
    // ... add new content ...

    // Restore position (clamped to valid range)
    view->scroll_to(old_offset.x, old_offset.y);
}
```

### Scroll to Top Button

```cpp
auto scroll_top_btn = std::make_unique<button<Backend>>("Back to Top");
scroll_top_btn->clicked.connect([view_ptr]() {
    view_ptr->scroll_to(0, 0);
});
```

## Theming

Scrollbars support theming via `scrollbar_theme`:

```cpp
struct scrollbar_theme {
    color_type thumb_color;
    color_type track_color;
    color_type arrow_color;
    scrollbar_style style;  // simple, arrows, modern
};
```

Scrollbars automatically inherit from the global theme via CSS inheritance.

## Troubleshooting

### Scrollbars Don't Appear

**Problem**: Content doesn't scroll, scrollbars never appear.

**Solutions**:
1. Ensure content is actually larger than viewport
2. Check scrollbar visibility policy isn't `hidden`
3. Verify layout strategy is set if using multiple children

```cpp
// Debug: Check scroll info
auto info = view->content()->get_scroll_info();
std::cout << "Content: " << info.content_size.w << "x" << info.content_size.h << "\n";
std::cout << "Viewport: " << info.viewport_size.w << "x" << info.viewport_size.h << "\n";
```

### Scroll Doesn't Work

**Problem**: `scroll_to()` has no effect.

**Solutions**:
1. Verify you measured and arranged after adding content
2. Check max_scroll value - content might be smaller than viewport
3. Ensure you're not scrolling a nested child (scroll the parent)

```cpp
// Always remeasure after content changes
view->add_child(new_item);
view->measure(width, height);
view->arrange(bounds);
view->scroll_to(0, 50);  // Now it works
```

## See Also

- [scroll_view API Reference](../api-reference/widgets/scroll-view.md)
- [scrollable API Reference](../api-reference/widgets/scrollable.md)
- [scrollbar API Reference](../api-reference/widgets/scrollbar.md)
