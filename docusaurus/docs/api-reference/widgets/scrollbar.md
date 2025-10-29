---
sidebar_position: 8
---

# scrollbar

The `onyxui::scrollbar` is a visual scrollbar widget that displays scroll position and allows user interaction via mouse dragging.

## Overview

`scrollbar` provides the visual representation and user interaction for scrolling. It's the "visual layer" of the scrolling system, displaying a draggable thumb on a track.

## When to Use

- **Manual Composition**: When building custom scroll layouts with precise scrollbar placement
- **Custom Scroll UI**: When you need scrollbars in non-standard locations or orientations
- **Visual Indicators**: When you want to show scroll position without using scroll_view

:::tip
For most use cases, use [scroll_view](scroll-view.md) instead, which includes scrollbars automatically.
:::

## Key Features

- **Orientation**: Vertical or horizontal
- **Visual Components**: Thumb (draggable), track (background), optional arrows
- **Mouse Interaction**: Drag thumb, click track, click arrows (if enabled)
- **Theming**: Fully customizable via `scrollbar_theme`
- **Synchronization**: Emits signals for integration with scrollable via scroll_controller

## Usage

### Basic Example

```cpp
#include <onyxui/widgets/scrollbar.hh>

// Create vertical scrollbar
auto scrollbar = std::make_unique<scrollbar<Backend>>(orientation::vertical);

// Set scroll info (typically from scrollable)
scroll_info info;
info.viewport_size = {200, 150};
info.content_size = {200, 500};  // Content is taller than viewport
info.scroll_offset = {0, 0};

scrollbar->set_scroll_info(info);

// Listen for user interaction
scrollbar->scroll_changed.connect([](const auto& new_offset) {
    std::cout << "User scrolled to: " << new_offset.y << "\n";
});
```

### Manual Composition

```cpp
#include <onyxui/widgets/scrollable.hh>
#include <onyxui/widgets/scrollbar.hh>
#include <onyxui/widgets/scroll_controller.hh>

// Create components
auto scrollable = std::make_unique<scrollable<Backend>>();
auto vscrollbar = std::make_unique<scrollbar<Backend>>(orientation::vertical);
auto hscrollbar = std::make_unique<scrollbar<Backend>>(orientation::horizontal);

// Store raw pointers before moving
auto* scrollable_ptr = scrollable.get();
auto* vscrollbar_ptr = vscrollbar.get();
auto* hscrollbar_ptr = hscrollbar.get();

// Add to custom layout
my_container->add_child(std::move(scrollable));
my_container->add_child(std::move(vscrollbar));
my_container->add_child(std::move(hscrollbar));

// Connect with controller for bidirectional sync
auto controller = std::make_unique<scroll_controller<Backend>>(
    scrollable_ptr,
    vscrollbar_ptr,
    hscrollbar_ptr
);
```

## API Reference

### Template Parameters

- `Backend`: The backend traits class (must satisfy UIBackend concept)

### Constructor

```cpp
// Create scrollbar with specified orientation
scrollbar(orientation orient);
```

**Orientation**:
- `orientation::vertical` - Vertical scrollbar (scroll up/down)
- `orientation::horizontal` - Horizontal scrollbar (scroll left/right)

### Public Methods

#### Scroll Information

```cpp
// Update scroll state (from scrollable)
void set_scroll_info(const scroll_info& info);

// Get current scroll state
scroll_info get_scroll_info() const;

// Get orientation
orientation get_orientation() const noexcept;
```

#### Signals

```cpp
// Emitted when user interacts with scrollbar
signal<point_type> scroll_changed;
```

The signal emits the new scroll offset when:
- User drags the thumb
- User clicks the track (page up/down)
- User clicks arrow buttons (if enabled by theme)

## Visual Components

### Thumb

The draggable indicator showing current scroll position and viewport size relative to content.

- **Size**: Proportional to `viewport_size / content_size`
- **Position**: Corresponds to `scroll_offset / max_scroll`
- **Interaction**: Click and drag to scroll

### Track

The background area where the thumb moves.

- **Size**: Full height (vertical) or width (horizontal) of scrollbar
- **Interaction**: Click to "page scroll" (jump by viewport size)

### Arrows (Optional)

Small increment/decrement buttons at the ends of the scrollbar.

- **Enabled**: Depends on `scrollbar_theme.style`
- **Interaction**: Click to scroll by small amount

## Mouse Interaction

### Drag Thumb

```cpp
// User clicks and drags thumb
// scrollbar calculates new offset based on thumb position
// Emits scroll_changed signal with new offset
```

### Click Track

```cpp
// User clicks track above/below thumb (or left/right for horizontal)
// Scrollbar "page scrolls" by viewport size in that direction
// Emits scroll_changed signal
```

### Click Arrows

```cpp
// User clicks arrow button (if enabled)
// Scrollbar scrolls by small increment
// Emits scroll_changed signal
```

## Theming

Scrollbars are themed via `scrollbar_theme` in the global theme:

```cpp
struct scrollbar_theme {
    color_type thumb_color;      // Draggable thumb
    color_type track_color;      // Background track
    color_type arrow_color;      // Arrow buttons (if style=arrows)
    scrollbar_style style;       // Visual style
};
```

### Scrollbar Styles

```cpp
enum class scrollbar_style : std::uint8_t {
    simple,   // No arrows, minimal design
    arrows,   // Include arrow buttons at ends
    modern    // Modern flat design (no arrows)
};
```

### Example Theme Configuration

```cpp
ui_theme<Backend> theme;

// Classic scrollbar with arrows
theme.scrollbar.style = scrollbar_style::arrows;
theme.scrollbar.thumb_color = {192, 192, 192};  // Light gray
theme.scrollbar.track_color = {64, 64, 64};     // Dark gray
theme.scrollbar.arrow_color = {255, 255, 255};  // White

// Modern flat scrollbar
theme.scrollbar.style = scrollbar_style::modern;
theme.scrollbar.thumb_color = {128, 128, 128};  // Medium gray
theme.scrollbar.track_color = {32, 32, 32};     // Very dark gray
```

## Examples

### Standalone Visual Indicator

```cpp
// Create a read-only scrollbar as a visual indicator
auto indicator = std::make_unique<scrollbar<Backend>>(orientation::vertical);

// Update position from external source
void update_indicator(int current_line, int total_lines, int visible_lines) {
    scroll_info info;
    info.content_size.h = total_lines * line_height;
    info.viewport_size.h = visible_lines * line_height;
    info.scroll_offset.y = current_line * line_height;

    indicator->set_scroll_info(info);
}

// No need to connect scroll_changed - indicator is read-only
```

### Custom Scroll Layout

```cpp
// Create a horizontal-only scroll layout
auto hbox = std::make_unique<hbox<Backend>>();

auto scrollable = std::make_unique<scrollable<Backend>>();
scrollable->set_scrollbar_visibility_policy({
    .horizontal = scrollbar_visibility::always,
    .vertical = scrollbar_visibility::hidden
});

auto hscrollbar = std::make_unique<scrollbar<Backend>>(orientation::horizontal);

// Vertical layout: scrollable on top, scrollbar on bottom
auto vbox = std::make_unique<vbox<Backend>>();
vbox->add_child(std::move(scrollable));
vbox->add_child(std::move(hscrollbar));

// Connect via controller
auto controller = std::make_unique<scroll_controller<Backend>>(
    scrollable_ptr,
    nullptr,         // No vertical scrollbar
    hscrollbar_ptr
);
```

### Synchronized Scrollbars

```cpp
// Two scrollables with synchronized scrollbars
auto scrollable1 = std::make_unique<scrollable<Backend>>();
auto scrollable2 = std::make_unique<scrollable<Backend>>();

auto shared_scrollbar = std::make_unique<scrollbar<Backend>>(orientation::vertical);

// Connect scrollbar to both scrollables
shared_scrollbar->scroll_changed.connect([scrollable1_ptr, scrollable2_ptr](const auto& offset) {
    scrollable1_ptr->scroll_to(offset.x, offset.y);
    scrollable2_ptr->scroll_to(offset.x, offset.y);
});

// Update scrollbar from either scrollable
scrollable1->scroll_changed.connect([scrollbar_ptr](const auto& offset) {
    auto info = scrollable1_ptr->get_scroll_info();
    scrollbar_ptr->set_scroll_info(info);
});
```

## Calculations

### Thumb Size

```cpp
// Proportional to visible content
float ratio = viewport_size / content_size;
int thumb_size = track_size * ratio;

// Minimum size for usability
thumb_size = std::max(thumb_size, minimum_thumb_size);
```

### Thumb Position

```cpp
// Position corresponds to scroll offset
float scroll_ratio = scroll_offset / max_scroll;
int thumb_position = (track_size - thumb_size) * scroll_ratio;
```

### Offset from Thumb Position

```cpp
// When user drags thumb
int max_thumb_pos = track_size - thumb_size;
float scroll_ratio = thumb_position / max_thumb_pos;
int new_offset = max_scroll * scroll_ratio;
```

## Visibility

Scrollbars can be shown/hidden based on the `scrollbar_visibility_policy` set on the associated scrollable:

- `always` - Scrollbar always visible (even if content fits in viewport)
- `auto_hide` - Scrollbar only visible when `content_size > viewport_size`
- `hidden` - Scrollbar never shown

The scrollbar widget doesn't control its own visibility - that's managed by the parent container (scroll_view or custom layout) based on the policy.

## Performance

- **Efficient Updates**: O(1) scroll info updates
- **Minimal Rendering**: Only redraws on scroll changes
- **Mouse Hit Testing**: O(1) component detection (thumb, track, arrows)

## See Also

- [Scrolling System Guide](../../guides/scrolling-system.md)
- [scroll_view API Reference](scroll-view.md) - High-level wrapper (recommended)
- [scrollable API Reference](scrollable.md) - Core scrolling logic
