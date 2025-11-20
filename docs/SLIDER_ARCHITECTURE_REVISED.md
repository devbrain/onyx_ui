# Slider Architecture - Revised Plan

**After analyzing scrollbar implementation**
**Date**: 2025-11-20

---

## Analysis: Why Not Extract Base Class?

### Scrollbar Architecture
```cpp
scrollbar : public widget_container<Backend>
    ├─ scrollbar_thumb (child widget)
    ├─ scrollbar_arrow (child widget, decrement)
    └─ scrollbar_arrow (child widget, increment)

// Data model
scroll_info {
    size_type content_size;    // Total scrollable content size
    size_type viewport_size;   // Visible area size
    point_type scroll_offset;  // Current scroll position
}

// Thumb size = proportional to (viewport / content)
thumb_size = track_size * (viewport_size / content_size)
```

### Slider Architecture (proposed)
```cpp
slider : public widget<Backend>  // Simple widget, NO children

// Data model
int m_value;    // Current value
int m_min;      // Range minimum
int m_max;      // Range maximum

// Thumb size = FIXED (theme-based)
thumb_size = theme->slider.thumb_size  // e.g., 3 characters
```

### Why They're Too Different for Common Base Class

1. **Inheritance Mismatch**: Scrollbar is `widget_container`, slider is `widget`
2. **Data Models**: Scrollbar has 3 dimensions (content, viewport, offset), slider has 3 different dimensions (min, max, value)
3. **Thumb Sizing**: Scrollbar's thumb is **proportional**, slider's thumb is **fixed size**
4. **Children Management**: Scrollbar has child widgets, slider doesn't
5. **Layout Complexity**: Scrollbar uses `absolute_layout`, slider just renders directly

**Conclusion**: Extracting a common base class would require making it handle both container and non-container widgets, which defeats the purpose of a clean abstraction.

---

## Recommended Approach: Shared Utility Library

Create **range mapping utilities** that both widgets can use independently.

### New File: `include/onyxui/widgets/utils/range_helpers.hh`

```cpp
#pragma once

#include <algorithm>
#include <cmath>

namespace onyxui::range_helpers {

/// Map position (in backend units) to value within range
///
/// @param position Mouse/thumb position (in backend units, relative to track start)
/// @param track_length Length of track (in backend units: chars for conio, pixels for SDL)
/// @param min_value Minimum value in range
/// @param max_value Maximum value in range
/// @return Value corresponding to position (not clamped or snapped)
///
/// @note Backend units: characters for conio, pixels for SDL/GUI backends
template<typename T = int>
[[nodiscard]] inline T position_to_value(
    int position,
    int track_length,
    T min_value,
    T max_value
) {
    if (track_length <= 0) {
        return min_value;
    }

    // Clamp position to track bounds
    int clamped_pos = std::clamp(position, 0, track_length);

    // Calculate ratio (0.0 - 1.0)
    float ratio = static_cast<float>(clamped_pos) / static_cast<float>(track_length);

    // Map to value range
    float range = static_cast<float>(max_value - min_value);
    return min_value + static_cast<T>(ratio * range);
}

/// Map value to position (in backend units) within track
///
/// @param value Current value
/// @param min_value Minimum value in range
/// @param max_value Maximum value in range
/// @param track_length Length of track (in backend units)
/// @return Position along track (in backend units: 0 = start, track_length = end)
///
/// @note Backend units: characters for conio, pixels for SDL/GUI backends
template<typename T = int>
[[nodiscard]] inline int value_to_position(
    T value,
    T min_value,
    T max_value,
    int track_length
) {
    if (max_value <= min_value || track_length <= 0) {
        return 0;
    }

    // Clamp value to range
    T clamped_value = std::clamp(value, min_value, max_value);

    // Calculate ratio (0.0 - 1.0)
    float range = static_cast<float>(max_value - min_value);
    float ratio = static_cast<float>(clamped_value - min_value) / range;

    // Map to position (in backend units)
    return static_cast<int>(ratio * static_cast<float>(track_length));
}

/// Snap value to step increments
///
/// @param value Value to snap
/// @param step Step size (0 = no snapping)
/// @return Snapped value (or original if step == 0)
template<typename T = int>
[[nodiscard]] inline T snap_to_step(T value, T step) {
    if (step <= 0) {
        return value;  // No snapping
    }

    // Round to nearest multiple of step
    T remainder = value % step;
    if (remainder >= step / 2) {
        return value + (step - remainder);  // Round up
    } else {
        return value - remainder;  // Round down
    }
}

/// Calculate proportional thumb size (for scrollbar)
///
/// @param track_length Total track length (in backend units)
/// @param content_size Total content size (in backend units)
/// @param viewport_size Visible viewport size (in backend units)
/// @param min_thumb_size Minimum thumb size (prevents thumb from becoming too small)
/// @return Thumb size (in backend units, clamped to [min_thumb_size, track_length])
///
/// @note Backend units: characters for conio, pixels for SDL/GUI backends
[[nodiscard]] inline int calculate_proportional_thumb_size(
    int track_length,
    int content_size,
    int viewport_size,
    int min_thumb_size
) {
    if (content_size <= viewport_size || track_length <= 0 || viewport_size <= 0) {
        return 0;  // No scrolling needed
    }

    // Thumb size proportional to viewport/content ratio
    float ratio = static_cast<float>(viewport_size) / static_cast<float>(content_size);
    int thumb_size = static_cast<int>(ratio * static_cast<float>(track_length));

    // Enforce minimum size and clamp to track
    return std::clamp(thumb_size, min_thumb_size, track_length);
}

/// Calculate thumb position for proportional thumb (scrollbar)
///
/// @param scroll_offset Current scroll position
/// @param max_scroll Maximum scroll value (content_size - viewport_size)
/// @param track_length Track length (in backend units)
/// @param thumb_size Thumb size (in backend units)
/// @return Thumb position (in backend units: 0 = start of track)
///
/// @note Backend units: characters for conio, pixels for SDL/GUI backends
[[nodiscard]] inline int calculate_proportional_thumb_position(
    int scroll_offset,
    int max_scroll,
    int track_length,
    int thumb_size
) {
    if (max_scroll <= 0 || track_length <= 0 || thumb_size >= track_length) {
        return 0;
    }

    // Clamp scroll offset
    int clamped_scroll = std::clamp(scroll_offset, 0, max_scroll);

    // Calculate available space for thumb movement
    int max_thumb_pos = track_length - thumb_size;

    // Map scroll position to thumb position
    float ratio = static_cast<float>(clamped_scroll) / static_cast<float>(max_scroll);
    return static_cast<int>(ratio * static_cast<float>(max_thumb_pos));
}

}  // namespace onyxui::range_helpers
```

---

## How Scrollbar Would Use These Utilities

```cpp
// In scrollbar::calculate_layout() - calculate thumb for horizontal scrollbar

#include <onyxui/widgets/utils/range_helpers.hh>

using namespace range_helpers;

// Calculate thumb size (proportional, in backend units)
int thumb_length = calculate_proportional_thumb_size(
    track_w,       // Track width (in backend units: chars for conio, pixels for SDL)
    content_w,     // Content width (in backend units)
    viewport_w,    // Viewport width (in backend units)
    min_thumb_size // Minimum thumb size from theme (in backend units)
);

// Calculate thumb position (in backend units)
int max_scroll = content_w - viewport_w;
int thumb_pos = calculate_proportional_thumb_position(
    scroll_x,      // Current scroll offset
    max_scroll,    // Maximum scroll value
    track_w,       // Track width (in backend units)
    thumb_length   // Thumb size (in backend units)
);
```

---

## How Slider Would Use These Utilities

```cpp
// In slider implementation

#include <onyxui/widgets/utils/range_helpers.hh>

using namespace range_helpers;

// Map mouse position to value
int slider::position_to_value(int mouse_x, int mouse_y) const {
    auto track = track_rect();
    int track_start = m_orientation == orientation::horizontal
        ? rect_utils::get_x(track)
        : rect_utils::get_y(track);
    int track_length = m_orientation == orientation::horizontal
        ? rect_utils::get_width(track)
        : rect_utils::get_height(track);

    // Mouse position relative to track (in backend units)
    int pos = (m_orientation == orientation::horizontal ? mouse_x : mouse_y) - track_start;

    // Map to value
    int value = position_to_value(pos, track_length, m_min, m_max);

    // Snap to step
    return snap_to_step(value, m_single_step);
}

// Map value to position (in backend units) for thumb rendering
int slider::value_to_thumb_position() const {
    auto track = track_rect();
    int track_length = m_orientation == orientation::horizontal
        ? rect_utils::get_width(track)
        : rect_utils::get_height(track);

    // Get thumb position (in backend units: chars for conio, pixels for SDL)
    int thumb_pos = value_to_position(m_value, m_min, m_max, track_length);

    // Thumb size is FIXED (from theme, in backend units)
    int thumb_size = theme->slider.thumb_size;

    // Offset thumb so its CENTER is at the value position
    return thumb_pos - (thumb_size / 2);
}
```

---

## Architecture Summary

```
┌─────────────────────────────────────────┐
│   range_helpers.hh (Utility Library)   │
│                                         │
│  - pixel_to_value()                    │
│  - value_to_pixel()                    │
│  - snap_to_step()                      │
│  - calculate_proportional_thumb_size() │
│  - calculate_proportional_thumb_pos()  │
└─────────────────────────────────────────┘
           ▲                    ▲
           │                    │
           │ Uses               │ Uses
           │                    │
  ┌────────┴────────┐  ┌───────┴────────┐
  │   scrollbar     │  │     slider     │
  │   (container)   │  │    (widget)    │
  │                 │  │                │
  │ - Children:     │  │ - No children  │
  │   thumb, arrows │  │ - Fixed thumb  │
  │ - Proportional  │  │ - Simple range │
  │   thumb         │  │ - Tick marks   │
  │ - scroll_info   │  │ - Value label  │
  └─────────────────┘  └────────────────┘
```

---

## Benefits of This Approach

✅ **No Refactoring of Scrollbar**: Scrollbar remains a container widget
✅ **Clean Separation**: Each widget has its own structure
✅ **Code Reuse**: Common math/mapping logic extracted
✅ **Testable**: Utility functions easy to unit test
✅ **Simple**: No complex inheritance hierarchy
✅ **Future-Proof**: Other widgets (progress_bar) can use utilities

---

## Implementation Plan

### Phase 1: Create Utilities
1. Create `include/onyxui/widgets/utils/range_helpers.hh`
2. Write unit tests for utility functions
3. Document each function with examples

### Phase 2: Implement Slider
1. Create `include/onyxui/widgets/input/slider.hh` as **simple widget**
2. Use `range_helpers::pixel_to_value()` for mouse interaction
3. Use `range_helpers::value_to_pixel()` for thumb positioning
4. Use `range_helpers::snap_to_step()` for step snapping
5. Implement tests (45 test cases)

### Phase 3: Optional - Refactor Scrollbar
1. Update scrollbar to use `range_helpers::calculate_proportional_thumb_*()` utilities
2. Replace inline math with utility calls
3. Verify all scrollbar tests still pass

---

## Next Steps

Would you like me to:
1. ✅ Create `range_helpers.hh` with utility functions
2. ✅ Implement slider using these utilities
3. ⏸️ Optionally refactor scrollbar to use utilities (non-breaking)

This approach gives us **code reuse without coupling**, and **no breaking changes** to existing widgets.

---

**Ready to implement!** 🚀
