---
sidebar_position: 6
---

# scroll_view

The `onyxui::scroll_view` is a high-level convenience wrapper that combines scrollable, scrollbars, and scroll_controller into a batteries-included scrolling container.

## Overview

`scroll_view` is the **recommended** way to add scrolling to your UI. It handles all the complexity of setting up scrollable containers, scrollbars, and synchronization automatically.

## Key Features

- **Batteries Included**: Combines scrollable, scrollbars, and controller in one widget
- **Preset Variants**: Factory functions for common use cases (modern, classic, compact, vertical-only)
- **Auto-Configuration**: Scrollbars automatically appear/hide based on content size
- **Method Forwarding**: Transparently forwards methods to internal scrollable
- **Layout Support**: Supports all layout strategies for multiple children
- **Theming**: Scrollbars automatically inherit from global theme

## Usage

### Quick Start with Presets

The easiest way to create a scroll_view is using preset factory functions:

```cpp
#include <onyxui/widgets/scroll_view_presets.hh>

// Modern look with auto-hiding scrollbars
auto view = modern_scroll_view<Backend>();
view->add_child(create_my_content());

// Classic look with always-visible scrollbars
auto view = classic_scroll_view<Backend>();

// Vertical scrolling only
auto view = vertical_only_scroll_view<Backend>();
```

### Manual Construction

```cpp
#include <onyxui/widgets/scroll_view.hh>

auto view = std::make_unique<scroll_view<Backend>>();

// Configure scrollbar visibility
view->set_scrollbar_policy(scrollbar_visibility::auto_hide);

// Add content
view->emplace_child<label>("My content");
```

### Adding Multiple Children

For multiple children, use a layout strategy:

```cpp
auto view = modern_scroll_view<Backend>();

// Set vertical list layout
view->set_layout_strategy(
    std::make_unique<linear_layout<Backend>>(direction::vertical, 5)
);

// Add items
for (int i = 0; i < 100; ++i) {
    view->emplace_child<label>("Item " + std::to_string(i));
}
```

## API Reference

### Template Parameters

- `Backend`: The backend traits class (must satisfy UIBackend concept)

### Public Methods

#### Content Management

```cpp
// Add child (forwarded to scrollable)
template<typename T, typename... Args>
T* emplace_child(Args&&... args);

void add_child(std::unique_ptr<ui_element<Backend>> child);
void remove_child(ui_element<Backend>* child);
void clear_children();

// Set layout strategy
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

// Get scroll information
scroll_info get_scroll_info() const;
point_type get_scroll_offset() const;
```

#### Configuration

```cpp
// Scrollbar visibility
void set_scrollbar_policy(scrollbar_visibility policy);
void set_scrollbar_policy(
    scrollbar_visibility horizontal,
    scrollbar_visibility vertical
);

// Enable/disable axes
void set_horizontal_scroll_enabled(bool enabled);
void set_vertical_scroll_enabled(bool enabled);
```

#### Component Access (Advanced)

```cpp
// Access internal components
scrollable<Backend>* content();
scrollbar<Backend>* vertical_scrollbar();
scrollbar<Backend>* horizontal_scrollbar();
scroll_controller<Backend>* controller();
```

### Preset Factory Functions

```cpp
// Auto-hide scrollbars (modern look)
std::unique_ptr<scroll_view<Backend>> modern_scroll_view();

// Always-visible scrollbars (classic look)
std::unique_ptr<scroll_view<Backend>> classic_scroll_view();

// Auto-hide, space-efficient
std::unique_ptr<scroll_view<Backend>> compact_scroll_view();

// Vertical scrolling only
std::unique_ptr<scroll_view<Backend>> vertical_only_scroll_view();
```

## Preset Comparison

| Preset | Horizontal | Vertical | Best For |
|--------|-----------|----------|----------|
| `modern_scroll_view()` | Auto-hide | Auto-hide | Modern apps, clean UI |
| `classic_scroll_view()` | Always visible | Always visible | Traditional desktop apps |
| `compact_scroll_view()` | Auto-hide | Auto-hide | Space-constrained layouts |
| `vertical_only_scroll_view()` | Hidden | Auto-hide | Lists, documents, feeds |

## Examples

### Settings Panel

```cpp
auto view = modern_scroll_view<Backend>();
view->set_layout_strategy(
    std::make_unique<linear_layout<Backend>>(direction::vertical, 10)
);

// Add settings sections
view->emplace_child<label>("Display Settings");
view->emplace_child<checkbox>("Dark mode");
view->emplace_child<slider>("Brightness");

view->emplace_child<label>("Audio Settings");
view->emplace_child<slider>("Volume");
view->emplace_child<checkbox>("Mute");

// Scrollbars appear automatically when content exceeds viewport
```

### Log Viewer

```cpp
auto view = vertical_only_scroll_view<Backend>();
view->set_layout_strategy(
    std::make_unique<linear_layout<Backend>>(direction::vertical, 2)
);

// Add log entries
for (const auto& entry : logs) {
    view->emplace_child<label>(entry);
}

// Auto-scroll to bottom
auto info = view->content()->get_scroll_info();
int max_y = info.content_size.h - info.viewport_size.h;
view->scroll_to(0, std::max(0, max_y));
```

### Data Grid

```cpp
auto view = classic_scroll_view<Backend>();

auto grid = std::make_unique<grid<Backend>>(5);  // 5 columns

// Add headers and data rows...
grid->emplace_child<label>("ID");
grid->emplace_child<label>("Name");
// ...

view->add_child(std::move(grid));
```

## Architecture

scroll_view uses a 2x2 grid layout internally:

```
┌──────────────┬─────┐
│              │  V  │
│  scrollable  │  E  │
│              │  R  │
├──────────────┼─────┤
│  HORIZONTAL  │ ░░░ │
└──────────────┴─────┘
```

- **Top-left**: scrollable widget (content area)
- **Top-right**: Vertical scrollbar
- **Bottom-left**: Horizontal scrollbar
- **Bottom-right**: Corner filler (spacer)

The scroll_controller automatically synchronizes scrollable and scrollbars via signal/slot connections.

## Theming

Scrollbars are themed via `scrollbar_theme` in the global theme:

```cpp
struct scrollbar_theme {
    color_type thumb_color;      // Draggable thumb
    color_type track_color;      // Background track
    color_type arrow_color;      // Arrow buttons (if style=arrows)
    scrollbar_style style;       // simple, arrows, modern
};
```

Changes to the theme automatically propagate to all scrollbars via CSS inheritance.

## Performance

- **Viewport Clipping**: Only visible content is rendered
- **Large Content**: Efficiently handles 10,000+ items
- **Dirty Regions**: Scrolling triggers minimal redraws
- **Memory**: O(total items) for tree, O(visible) for rendering

## See Also

- [Scrolling System Guide](../../guides/scrolling-system.md)
- [scrollable API Reference](scrollable.md)
- [scrollbar API Reference](scrollbar.md)
