---
sidebar_position: 7
---

# scrollable

The `onyxui::scrollable` is the core scrolling logic layer that manages viewport clipping, scroll offset, and content size tracking.

## Overview

`scrollable` provides the fundamental scrolling functionality without visual scrollbars. It's the "logic layer" of the scrolling system, handling viewport management and scroll calculations.

## When to Use

- **Custom UI**: When you need scrolling without visible scrollbars (e.g., game HUDs, mobile-style interfaces)
- **Manual Composition**: When building custom scroll layouts with precise control over scrollbar placement
- **Programmatic Scrolling**: When content scrolls via code/animation rather than user interaction

:::tip
For most use cases, use [scroll_view](scroll-view.md) instead, which combines scrollable with visual scrollbars.
:::

## Key Features

- **Viewport Clipping**: Automatically clips content outside the visible area
- **Scroll Offset Management**: Tracks current scroll position (x, y coordinates)
- **Mouse Wheel Support**: Automatic mouse wheel scrolling
- **Content Size Tracking**: Monitors total scrollable content size
- **Layout Support**: Can contain multiple children with layout strategies
- **Visibility Policies**: Configure when scrollbars should appear (if using with scrollbar widgets)

## Usage

### Basic Example

```cpp
#include <onyxui/widgets/scrollable.hh>

auto scrollable = std::make_unique<scrollable<Backend>>();

// Add content
scrollable->add_child(create_large_content());

// Measure and arrange
scrollable->measure(200, 150);
scrollable->arrange({0, 0, 200, 150});

// Programmatic scrolling
scrollable->scroll_to(0, 100);  // Scroll to y=100
```

### With Multiple Children

```cpp
auto scrollable = std::make_unique<scrollable<Backend>>();

// Set layout strategy
scrollable->set_layout_strategy(
    std::make_unique<linear_layout<Backend>>(direction::vertical, 5)
);

// Add many items
for (int i = 0; i < 100; ++i) {
    scrollable->emplace_child<label>("Item " + std::to_string(i));
}
```

### Hidden Scrollbars (Programmatic Only)

```cpp
auto scrollable = std::make_unique<scrollable<Backend>>();

// Disable scrollbar visibility
scrollable->set_scrollbar_visibility_policy({
    .horizontal = scrollbar_visibility::hidden,
    .vertical = scrollbar_visibility::hidden
});

// Scroll via code only (mouse wheel still works)
scrollable->scroll_to(0, 50);
```

## API Reference

### Template Parameters

- `Backend`: The backend traits class (must satisfy UIBackend concept)

### Public Methods

#### Content Management

```cpp
// Add children
template<typename T, typename... Args>
T* emplace_child(Args&&... args);

void add_child(std::unique_ptr<ui_element<Backend>> child);
void remove_child(ui_element<Backend>* child);
void clear_children();

// Layout strategy
void set_layout_strategy(std::unique_ptr<layout_strategy<Backend>> strategy);
```

#### Scrolling Control

```cpp
// Absolute scrolling
void scroll_to(int x, int y);

// Relative scrolling
void scroll_by(int dx, int dy);

// Scroll widget into view
void scroll_into_view(const ui_element<Backend>* widget);

// Get scroll state
scroll_info get_scroll_info() const;
point_type get_scroll_offset() const;
```

#### Configuration

```cpp
// Scrollbar visibility policy
void set_scrollbar_visibility_policy(scrollbar_visibility_policy policy);
scrollbar_visibility_policy get_scrollbar_visibility_policy() const;
```

#### Signals

```cpp
// Emitted when scroll offset changes
signal<point_type> scroll_changed;
```

## scroll_info Structure

The `scroll_info` struct provides comprehensive information about the scroll state:

```cpp
template<UIBackend Backend>
struct scroll_info {
    size_type viewport_size;  // Visible area
    size_type content_size;   // Total scrollable content
    point_type scroll_offset; // Current scroll position

    // Computed properties
    point_type max_scroll() const;           // Maximum valid scroll
    bool needs_horizontal_scroll() const;    // Content exceeds viewport width
    bool needs_vertical_scroll() const;      // Content exceeds viewport height
};
```

### Example Usage

```cpp
auto info = scrollable->get_scroll_info();

std::cout << "Viewport: " << info.viewport_size.w << "x" << info.viewport_size.h << "\n";
std::cout << "Content: " << info.content_size.w << "x" << info.content_size.h << "\n";
std::cout << "Scroll offset: " << info.scroll_offset.x << "," << info.scroll_offset.y << "\n";

// Check if scrolling is needed
if (info.needs_vertical_scroll()) {
    std::cout << "Vertical scrolling available\n";
}

// Get maximum scroll position
auto max = info.max_scroll();
std::cout << "Max scroll: " << max.x << "," << max.y << "\n";
```

## Scrollbar Visibility Policies

```cpp
enum class scrollbar_visibility : std::uint8_t {
    always,      // Always visible, even if not needed
    auto_hide,   // Visible when content exceeds viewport
    hidden       // Never visible (programmatic scroll only)
};

struct scrollbar_visibility_policy {
    scrollbar_visibility horizontal;
    scrollbar_visibility vertical;
};
```

### Example Configuration

```cpp
// Always show both scrollbars
scrollable->set_scrollbar_visibility_policy({
    .horizontal = scrollbar_visibility::always,
    .vertical = scrollbar_visibility::always
});

// Auto-hide horizontal, always show vertical
scrollable->set_scrollbar_visibility_policy({
    .horizontal = scrollbar_visibility::auto_hide,
    .vertical = scrollbar_visibility::always
});

// Programmatic scrolling only (no scrollbars)
scrollable->set_scrollbar_visibility_policy({
    .horizontal = scrollbar_visibility::hidden,
    .vertical = scrollbar_visibility::hidden
});
```

## Mouse Wheel Scrolling

Mouse wheel scrolling is handled automatically:

- **Wheel up/down**: Vertical scroll
- **Shift + wheel**: Horizontal scroll (if horizontal scrolling is enabled)

No additional configuration needed.

## Examples

### Infinite Scroll Feed

```cpp
auto scrollable = std::make_unique<scrollable<Backend>>();
scrollable->set_layout_strategy(
    std::make_unique<linear_layout<Backend>>(direction::vertical)
);

// Add initial items
for (int i = 0; i < 20; ++i) {
    scrollable->emplace_child<panel>(create_feed_item(i));
}

// Listen for scroll events to load more
scrollable->scroll_changed.connect([scrollable_ptr](const auto& offset) {
    auto info = scrollable_ptr->get_scroll_info();
    auto max_scroll = info.max_scroll();

    // Near bottom? Load more items
    if (offset.y >= max_scroll.y - 100) {
        load_more_items(scrollable_ptr);
    }
});
```

### Smooth Scrolling Animation

```cpp
void smooth_scroll_to(scrollable<Backend>* s, int target_y, float duration) {
    auto start_y = s->get_scroll_offset().y;
    auto distance = target_y - start_y;

    // Animate over time (pseudo-code)
    animate(duration, [=](float t) {
        int current_y = start_y + static_cast<int>(distance * t);
        s->scroll_to(0, current_y);
    });
}

// Usage
smooth_scroll_to(scrollable.get(), 500, 0.3f);  // 300ms animation
```

### Viewport-Relative Positioning

```cpp
// Get viewport bounds
auto info = scrollable->get_scroll_info();
auto viewport = info.viewport_size;
auto offset = info.scroll_offset;

// Calculate visible area in content coordinates
int visible_top = offset.y;
int visible_bottom = offset.y + viewport.h;
int visible_left = offset.x;
int visible_right = offset.x + viewport.w;

// Check if widget is visible
bool is_widget_visible(const ui_element<Backend>* widget) {
    auto bounds = widget->bounds();
    return bounds.y + bounds.h >= visible_top &&
           bounds.y <= visible_bottom &&
           bounds.x + bounds.w >= visible_left &&
           bounds.x <= visible_right;
}
```

## Manual Composition with Scrollbars

For custom layouts, combine scrollable with scrollbar and scroll_controller:

```cpp
#include <onyxui/widgets/scrollable.hh>
#include <onyxui/widgets/scrollbar.hh>
#include <onyxui/widgets/scroll_controller.hh>

// Create components
auto scrollable = std::make_unique<scrollable<Backend>>();
auto vscrollbar = std::make_unique<scrollbar<Backend>>(orientation::vertical);

// Store raw pointers before moving
auto* scrollable_ptr = scrollable.get();
auto* vscrollbar_ptr = vscrollbar.get();

// Add to layout
my_container->add_child(std::move(scrollable));
my_container->add_child(std::move(vscrollbar));

// Connect with controller (after adding to layout)
auto controller = std::make_unique<scroll_controller<Backend>>(
    scrollable_ptr,
    vscrollbar_ptr,
    nullptr  // No horizontal scrollbar
);
```

## Performance

- **Viewport Clipping**: Only visible children are rendered
- **Efficient Scrolling**: O(1) offset updates
- **Layout Caching**: Content size cached until invalidation
- **Dirty Regions**: Scroll changes trigger minimal redraws

## See Also

- [Scrolling System Guide](../../guides/scrolling-system.md)
- [scroll_view API Reference](scroll-view.md) - High-level wrapper (recommended)
- [scrollbar API Reference](scrollbar.md) - Visual scrollbar widget
