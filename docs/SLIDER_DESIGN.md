# Slider Widget Design

**Status**: Design Phase
**Widget**: `onyxui::slider<Backend>`
**Phase**: 3 (Visual Feedback Widgets)
**Priority**: High (Tier 2)
**Created**: 2025-11-20
**Author**: Architecture Team

---

## Table of Contents

1. [Overview](#overview)
2. [Architecture](#architecture)
3. [API Design](#api-design)
4. [Visual Design](#visual-design)
5. [Theme Integration](#theme-integration)
6. [Event Handling](#event-handling)
7. [Semantic Actions](#semantic-actions)
8. [Usage Examples](#usage-examples)
9. [Testing Strategy](#testing-strategy)
10. [Implementation Checklist](#implementation-checklist)

---

## Overview

### What is a Slider?

A **slider** is an input widget for selecting numeric values within a range by dragging a thumb along a track or using keyboard controls.

**Key Characteristics**:
- Visual feedback of value within range
- Mouse drag interaction
- Keyboard navigation (Arrow keys, Page Up/Down, Home/End)
- Horizontal or vertical orientation
- Optional tick marks and value display
- Step-based snapping (e.g., multiples of 5)

**Use Cases**:
- Volume controls (0-100)
- Brightness/opacity adjustments (0-100)
- Zoom levels (25-400%)
- Playback position (time slider)
- Filter parameters (numeric ranges)
- Settings panels (numeric preferences)

---

## Architecture

### Widget Hierarchy

```
ui_element<Backend>
  └─ widget<Backend>
       └─ slider<Backend>  ← New widget
```

### Core Components

A slider consists of three visual parts:

1. **Track (Groove)**: The background line showing the full range
2. **Thumb (Handle)**: The draggable indicator showing current value
3. **Tick Marks** (optional): Visual markers at intervals

```
Track:  ┌────────────────────────┐
        │░░░░░░░░░░░░░░░░░░░░░░░│  ← Empty portion (background)
        └────────────────────────┘

Filled: ┌────────────────────────┐
        │████████░░░░░░░░░░░░░░░│  ← Filled portion (value indicator)
        └────────────────────────┘

Thumb:  ┌────────────────────────┐
        │████████●░░░░░░░░░░░░░░│  ← Thumb position (draggable)
        └────────────────────────┘

Ticks:  |───|───|───|───|───|───|
        0  20  40  60  80  100
```

### Value Mapping

```
Position on track → Value within range

pixel_position = (value - min) / (max - min) * track_width
value = min + (pixel_position / track_width) * (max - min)

With step snapping:
value = round(value / step) * step
```

### State Machine

```
┌─────────┐  Mouse down   ┌──────────┐  Mouse up   ┌─────────┐
│  Idle   │──────────────>│ Dragging │───────────>│  Idle   │
└─────────┘               └──────────┘             └─────────┘
     │                          │
     │ Hover                    │ Mouse move
     │                          │
     v                          v
┌─────────┐                 Update value
│ Hovered │                 Emit slider_moved
└─────────┘
```

---

## API Design

### Class Declaration

```cpp
namespace onyxui {

/// Orientation for slider widget
enum class slider_orientation : std::uint8_t {
    horizontal,  ///< Left-to-right slider (default)
    vertical     ///< Bottom-to-top slider
};

/// Tick mark position
enum class tick_position : std::uint8_t {
    none,         ///< No tick marks (default)
    above,        ///< Ticks above track (horizontal) or left (vertical)
    below,        ///< Ticks below track (horizontal) or right (vertical)
    both_sides    ///< Ticks on both sides
};

/// Slider widget - numeric range input with visual feedback
///
/// The slider provides intuitive visual selection of numeric values within
/// a range. Users can drag the thumb with the mouse or use keyboard controls
/// for precise adjustments.
///
/// **Key Features:**
/// - Mouse drag interaction
/// - Keyboard navigation (Arrow keys, Page Up/Down, Home/End)
/// - Step-based value snapping
/// - Optional tick marks at intervals
/// - Optional value display near thumb
/// - Horizontal or vertical orientation
///
/// Visual appearance (horizontal slider at 40%):
/// @code
/// Track:   ┌────────────────────────┐
///          │████████●░░░░░░░░░░░░░░│  ← 40/100
///          └────────────────────────┘
/// Ticks:   |───|───|───|───|───|
///          0  25  50  75  100
/// @endcode
///
/// Usage examples:
/// @code
/// // Volume control (0-100)
/// auto volume = std::make_unique<slider<Backend>>();
/// volume->set_range(0, 100);
/// volume->set_value(75);
/// volume->value_changed.connect([](int value) {
///     audio_system::set_volume(value);
/// });
///
/// // Opacity slider with ticks
/// auto opacity = std::make_unique<slider<Backend>>();
/// opacity->set_range(0, 100);
/// opacity->set_single_step(5);
/// opacity->set_tick_interval(25);
/// opacity->set_tick_position(tick_position::below);
/// @endcode
///
/// @tparam Backend The backend traits class
template<UIBackend Backend>
class slider : public widget<Backend> {
public:
    using base = widget<Backend>;
    using typename base::color_type;
    using typename base::rect_type;
    using typename base::size_type;
    using typename base::point_type;
    using typename base::renderer_type;

    // ===== Construction =====

    /// Create slider with default settings (0-100 range, horizontal)
    ///
    /// @param parent Parent element (optional)
    explicit slider(ui_element<Backend>* parent = nullptr);

    /// Create slider with specified orientation
    ///
    /// @param orientation Horizontal or vertical orientation
    /// @param parent Parent element (optional)
    explicit slider(slider_orientation orientation, ui_element<Backend>* parent = nullptr);

    // ===== Value Management =====

    /// Set current value
    ///
    /// The value is clamped to [min, max] range and snapped to step if step > 0.
    /// Emits value_changed signal if the value actually changes.
    ///
    /// @param value New value
    void set_value(int value);

    /// Get current value
    ///
    /// @return Current slider value
    [[nodiscard]] int value() const noexcept { return m_value; }

    // ===== Range Management =====

    /// Set value range
    ///
    /// Updates min and max, and clamps current value to new range.
    /// If min >= max, behavior is undefined.
    ///
    /// @param min Minimum value (inclusive)
    /// @param max Maximum value (inclusive)
    void set_range(int min, int max);

    /// Get minimum value
    ///
    /// @return Minimum slider value
    [[nodiscard]] int minimum() const noexcept { return m_min; }

    /// Get maximum value
    ///
    /// @return Maximum slider value
    [[nodiscard]] int maximum() const noexcept { return m_max; }

    // ===== Step Management =====

    /// Set single step (arrow key increment)
    ///
    /// When user presses arrow keys, value changes by this amount.
    /// Set to 0 to disable snapping (smooth values).
    ///
    /// @param step Single step size (must be > 0)
    void set_single_step(int step);

    /// Get single step size
    ///
    /// @return Single step size
    [[nodiscard]] int single_step() const noexcept { return m_single_step; }

    /// Set page step (Page Up/Down increment)
    ///
    /// When user presses Page Up/Down, value changes by this amount.
    ///
    /// @param step Page step size (must be > 0)
    void set_page_step(int step);

    /// Get page step size
    ///
    /// @return Page step size
    [[nodiscard]] int page_step() const noexcept { return m_page_step; }

    // ===== Orientation =====

    /// Set slider orientation
    ///
    /// Changes between horizontal (left-to-right) and vertical (bottom-to-top).
    /// Invalidates layout.
    ///
    /// @param orientation New orientation
    void set_orientation(slider_orientation orientation);

    /// Get slider orientation
    ///
    /// @return Current orientation
    [[nodiscard]] slider_orientation orientation() const noexcept { return m_orientation; }

    // ===== Tick Marks =====

    /// Set tick mark position
    ///
    /// Controls where tick marks are drawn relative to the track.
    ///
    /// @param position Tick mark position (none/above/below/both_sides)
    void set_tick_position(tick_position position);

    /// Get tick mark position
    ///
    /// @return Current tick position
    [[nodiscard]] tick_position get_tick_position() const noexcept { return m_tick_position; }

    /// Set tick mark interval
    ///
    /// Draws tick marks at regular intervals (e.g., 25 for marks at 0, 25, 50, 75, 100).
    /// Set to 0 to disable tick marks.
    ///
    /// @param interval Interval between tick marks (0 = disabled)
    void set_tick_interval(int interval);

    /// Get tick mark interval
    ///
    /// @return Tick mark interval (0 if disabled)
    [[nodiscard]] int tick_interval() const noexcept { return m_tick_interval; }

    // ===== Value Display =====

    /// Set value label visibility
    ///
    /// When enabled, displays current value near the thumb.
    ///
    /// @param visible true to show value label
    void set_value_visible(bool visible);

    /// Get value label visibility
    ///
    /// @return true if value label is visible
    [[nodiscard]] bool is_value_visible() const noexcept { return m_value_visible; }

    // ===== Tracking Mode =====

    /// Set tracking mode
    ///
    /// When tracking is enabled (default), value_changed emits during drag.
    /// When disabled, value_changed only emits on mouse release.
    ///
    /// @param enable true to enable tracking
    void set_tracking(bool enable);

    /// Get tracking mode
    ///
    /// @return true if tracking enabled
    [[nodiscard]] bool has_tracking() const noexcept { return m_tracking; }

    // ===== Signals =====

    /// Emitted when value changes (any method: drag, keyboard, set_value)
    ///
    /// If tracking is disabled, only emits on mouse release (not during drag).
    signal<int> value_changed;

    /// Emitted when user drags the slider (mouse move while dragging)
    ///
    /// Always emits during drag, regardless of tracking setting.
    /// Parameter: current value under mouse
    signal<int> slider_moved;

    /// Emitted when user presses mouse on thumb
    signal<> slider_pressed;

    /// Emitted when user releases mouse after dragging
    signal<> slider_released;

protected:
    // ===== Rendering =====

    void do_render(render_context<Backend>& ctx) const override;

    // ===== Event Handling =====

    bool handle_event(const ui_event& event, event_phase phase) override;

    /// Handle semantic action (arrow keys, page up/down, home/end)
    bool handle_semantic_action(hotkey_action action) override;

private:
    // ===== State =====

    int m_value = 0;                                          ///< Current value
    int m_min = 0;                                            ///< Minimum value
    int m_max = 100;                                          ///< Maximum value
    int m_single_step = 1;                                    ///< Arrow key increment
    int m_page_step = 10;                                     ///< Page Up/Down increment
    slider_orientation m_orientation = slider_orientation::horizontal;
    tick_position m_tick_position = tick_position::none;
    int m_tick_interval = 0;                                  ///< Tick mark interval (0 = disabled)
    bool m_value_visible = false;                             ///< Show value label
    bool m_tracking = true;                                   ///< Emit value_changed during drag

    // Drag state
    bool m_is_dragging = false;                               ///< Currently dragging thumb
    bool m_is_hovered = false;                                ///< Mouse over thumb
    int m_drag_start_value = 0;                               ///< Value when drag started

    // ===== Internal Helpers =====

    /// Convert screen position to value
    ///
    /// Maps mouse coordinates to value within range, accounting for orientation.
    ///
    /// @param x Mouse X coordinate
    /// @param y Mouse Y coordinate
    /// @return Value at that position (clamped and snapped to step)
    [[nodiscard]] int position_to_value(int x, int y) const;

    /// Convert value to screen position (thumb center)
    ///
    /// @param value Value to convert
    /// @return Screen coordinates of thumb center
    [[nodiscard]] point_type value_to_position(int value) const;

    /// Get thumb rectangle for current value
    ///
    /// @return Bounding rectangle of thumb
    [[nodiscard]] rect_type thumb_rect() const;

    /// Get track rectangle (clickable area)
    ///
    /// @return Bounding rectangle of track
    [[nodiscard]] rect_type track_rect() const;

    /// Check if point is inside thumb
    ///
    /// @param x X coordinate
    /// @param y Y coordinate
    /// @return true if point is inside thumb
    [[nodiscard]] bool is_point_in_thumb(int x, int y) const;

    /// Clamp value to range
    ///
    /// @param value Value to clamp
    /// @return Clamped value
    [[nodiscard]] int clamp_value(int value) const;

    /// Snap value to step
    ///
    /// @param value Value to snap
    /// @return Snapped value (or original if step == 0)
    [[nodiscard]] int snap_to_step(int value) const;

    /// Increment value by step
    ///
    /// @param step Step amount (can be negative)
    void increment_value(int step);
};

}  // namespace onyxui
```

---

## Visual Design

### Horizontal Slider

```
Normal state (40% position):
┌────────────────────────────────────┐
│████████████░░░░░░░░░░░░░░░░░░░░░░│  ← Track
│            ●                       │  ← Thumb
└────────────────────────────────────┘

With ticks below:
┌────────────────────────────────────┐
│████████████░░░░░░░░░░░░░░░░░░░░░░│
│            ●                       │
└────────────────────────────────────┘
 |     |     |     |     |     |     |
 0    20    40    60    80   100

With value label:
┌────────────────────────────────────┐
│████████████░░░░░░░░░░░░░░░░░░░░░░│
│            ● 40                    │  ← Value displayed near thumb
└────────────────────────────────────┘

Hovered state:
┌────────────────────────────────────┐
│████████████░░░░░░░░░░░░░░░░░░░░░░│
│            ◉                       │  ← Thumb highlighted
└────────────────────────────────────┘

Disabled state:
┌────────────────────────────────────┐
│▒▒▒▒▒▒▒▒▒▒▒▒▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓│  ← Grayed out
│            ○                       │  ← Thumb grayed
└────────────────────────────────────┘
```

### Vertical Slider

```
Normal state (60% position):

100 ─┬─
     │▓
     │▓
  80 ─┼─
     │▓
     │▓
  60 ─●─  ← Thumb at 60%
     │░
     │░
  40 ─┼─
     │░
     │░
  20 ─┼─
     │░
     │░
   0 ─┴─

With ticks on both sides:

100 ─┬─
   | │▓ |
   | │▓ |
  80 ─┼─
   | │▓ |
   | │▓ |
  60 ─●─
   | │░ |
   | │░ |
  40 ─┼─
   | │░ |
   | │░ |
  20 ─┼─
   | │░ |
   | │░ |
   0 ─┴─
```

### DOS/TUI Rendering

For `conio_backend`, the slider uses ASCII/box-drawing characters:

**Horizontal Track:**
- Filled portion: `█` (U+2588 Full Block)
- Empty portion: `░` (U+2591 Light Shade)
- Thumb: `●` (U+25CF Black Circle) or `◉` (U+25C9 Fisheye) when hovered
- Tick marks: `|` (vertical pipe)

**Vertical Track:**
- Filled portion: `▓` (U+2593 Dark Shade)
- Empty portion: `░` (U+2591 Light Shade)
- Thumb: `●` (U+25CF Black Circle) or `◉` (U+25C9 Fisheye) when hovered
- Tick marks: `─` (U+2500 Box Drawings Light Horizontal)

---

## Theme Integration

### Theme Structure

```cpp
// Add to theme.hh
struct slider_style {
    // Track (groove)
    visual_state track_normal;         ///< Track colors/font (normal state)
    visual_state track_disabled;       ///< Track colors when disabled
    color_type filled_color;           ///< Color of filled portion
    color_type empty_color;            ///< Color of empty portion

    // Thumb (handle)
    visual_state thumb_normal;         ///< Thumb colors (normal)
    visual_state thumb_hover;          ///< Thumb colors (mouse over)
    visual_state thumb_pressed;        ///< Thumb colors (dragging)
    visual_state thumb_disabled;       ///< Thumb colors (disabled)
    int thumb_size;                    ///< Thumb diameter (pixels)

    // Tick marks
    color_type tick_color;             ///< Tick mark color
    int tick_length;                   ///< Tick mark length (pixels)

    // Value label
    font_type value_font;              ///< Font for value display
    color_type value_color;            ///< Value text color

    // Dimensions
    int track_thickness;               ///< Track height (horizontal) or width (vertical)
    int spacing;                       ///< Space between track and ticks
};
```

### YAML Theme Example

```yaml
slider:
  # Track
  track_normal:
    font: { bold: false, reverse: false, underline: false }
    foreground: { r: 255, g: 255, b: 255 }  # White
    background: { r: 0, g: 0, b: 170 }      # Dark blue

  track_disabled:
    font: { bold: false, reverse: false, underline: false }
    foreground: { r: 85, g: 85, b: 85 }     # Dark gray
    background: { r: 0, g: 0, b: 170 }

  filled_color: { r: 0, g: 255, b: 255 }    # Cyan (filled portion)
  empty_color: { r: 170, g: 170, b: 170 }   # Light gray (empty)

  # Thumb
  thumb_normal:
    font: { bold: false, reverse: false, underline: false }
    foreground: { r: 255, g: 255, b: 255 }  # White
    background: { r: 0, g: 0, b: 170 }

  thumb_hover:
    font: { bold: false, reverse: false, underline: false }
    foreground: { r: 255, g: 255, b: 0 }    # Yellow
    background: { r: 0, g: 0, b: 255 }      # Bright blue

  thumb_pressed:
    font: { bold: false, reverse: false, underline: false }
    foreground: { r: 255, g: 255, b: 255 }  # White
    background: { r: 0, g: 255, b: 255 }    # Cyan

  thumb_disabled:
    font: { bold: false, reverse: false, underline: false }
    foreground: { r: 85, g: 85, b: 85 }     # Dark gray
    background: { r: 0, g: 0, b: 170 }

  thumb_size: 3                              # 3 characters diameter

  # Ticks
  tick_color: { r: 170, g: 170, b: 170 }    # Light gray
  tick_length: 2                             # 2 characters

  # Value label
  value_font: { bold: true, reverse: false, underline: false }
  value_color: { r: 255, g: 255, b: 0 }     # Yellow

  # Dimensions
  track_thickness: 1                         # 1 character height
  spacing: 1                                 # 1 character spacing
```

---

## Event Handling

### Mouse Events

```cpp
bool slider<Backend>::handle_event(const ui_event& event, event_phase phase) {
    if (phase != event_phase::target) {
        return base::handle_event(event, phase);
    }

    // Mouse press - start drag if on thumb, or jump to position if on track
    if (auto* mouse_evt = std::get_if<mouse_event>(&event)) {
        if (mouse_evt->act == mouse_event::action::press) {
            int x = point_utils::get_x(mouse_evt->position);
            int y = point_utils::get_y(mouse_evt->position);

            if (is_point_in_thumb(x, y)) {
                // Start dragging thumb
                m_is_dragging = true;
                m_drag_start_value = m_value;
                slider_pressed.emit();
                return true;
            } else if (is_point_in_track(x, y)) {
                // Jump to clicked position
                int new_value = position_to_value(x, y);
                set_value(new_value);
                return true;
            }
        }

        // Mouse move - update value if dragging
        if (mouse_evt->act == mouse_event::action::move) {
            int x = point_utils::get_x(mouse_evt->position);
            int y = point_utils::get_y(mouse_evt->position);

            // Update hover state
            m_is_hovered = is_point_in_thumb(x, y);

            if (m_is_dragging) {
                int new_value = position_to_value(x, y);

                // Update value
                if (new_value != m_value) {
                    m_value = clamp_value(snap_to_step(new_value));
                    this->mark_dirty();

                    // Emit signals
                    slider_moved.emit(m_value);
                    if (m_tracking) {
                        value_changed.emit(m_value);
                    }
                }
                return true;
            }
        }

        // Mouse release - end drag
        if (mouse_evt->act == mouse_event::action::release) {
            if (m_is_dragging) {
                m_is_dragging = false;
                slider_released.emit();

                // Emit value_changed if tracking disabled
                if (!m_tracking) {
                    value_changed.emit(m_value);
                }
                return true;
            }
        }
    }

    return base::handle_event(event, phase);
}
```

### Keyboard Events

```cpp
bool slider<Backend>::handle_semantic_action(hotkey_action action) {
    switch (action) {
        case hotkey_action::menu_left:
        case hotkey_action::menu_up:
            // Decrement by single step
            increment_value(-m_single_step);
            return true;

        case hotkey_action::menu_right:
        case hotkey_action::menu_down:
            // Increment by single step
            increment_value(m_single_step);
            return true;

        case hotkey_action::page_up:
            // Increment by page step
            increment_value(m_page_step);
            return true;

        case hotkey_action::page_down:
            // Decrement by page step
            increment_value(-m_page_step);
            return true;

        case hotkey_action::move_to_start:  // Home key
            set_value(m_min);
            return true;

        case hotkey_action::move_to_end:    // End key
            set_value(m_max);
            return true;

        default:
            return base::handle_semantic_action(action);
    }
}
```

---

## Semantic Actions

### Reusing Existing Actions

The slider reuses existing semantic actions from menu navigation:

| Action | Key (Windows) | Slider Behavior |
|--------|---------------|-----------------|
| `menu_left` | Left Arrow | Decrease by single_step (horizontal) |
| `menu_right` | Right Arrow | Increase by single_step (horizontal) |
| `menu_up` | Up Arrow | Increase by single_step (vertical) |
| `menu_down` | Down Arrow | Decrease by single_step (vertical) |
| `page_up` | Page Up | Increase by page_step |
| `page_down` | Page Down | Decrease by page_step |
| `move_to_start` | Home | Jump to minimum value |
| `move_to_end` | End | Jump to maximum value |

**No new semantic actions needed** - slider reuses navigation actions.

---

## Usage Examples

### Example 1: Volume Control

```cpp
#include <onyxui/widgets/input/slider.hh>

// Create volume slider (0-100)
auto volume = std::make_unique<slider<Backend>>();
volume->set_range(0, 100);
volume->set_value(75);
volume->set_single_step(5);   // Arrow keys change by 5
volume->set_page_step(10);    // Page Up/Down change by 10
volume->set_value_visible(true);

// Connect signal
volume->value_changed.connect([](int value) {
    audio_system::set_volume(value);
    std::cout << "Volume: " << value << "%\n";
});

// Optional: only update on mouse release (for expensive operations)
volume->set_tracking(false);
volume->slider_released.connect([volume]() {
    apply_volume_setting(volume->value());
});
```

### Example 2: Opacity Slider with Ticks

```cpp
// Create opacity slider (0-100 representing 0.0-1.0)
auto opacity = std::make_unique<slider<Backend>>();
opacity->set_range(0, 100);
opacity->set_value(100);  // Fully opaque
opacity->set_single_step(5);
opacity->set_tick_interval(25);  // Ticks at 0, 25, 50, 75, 100
opacity->set_tick_position(tick_position::below);

opacity->value_changed.connect([](int value) {
    float opacity_float = value / 100.0f;
    window->set_opacity(opacity_float);
});
```

### Example 3: Vertical Slider (Mixer Fader)

```cpp
// Create vertical fader for audio mixer
auto fader = std::make_unique<slider<Backend>>(slider_orientation::vertical);
fader->set_range(-60, 12);  // dB range
fader->set_value(0);         // Unity gain
fader->set_single_step(1);
fader->set_page_step(6);
fader->set_tick_interval(6);
fader->set_tick_position(tick_position::both_sides);
fader->set_value_visible(true);

fader->value_changed.connect([](int value) {
    mixer_channel->set_gain_db(value);
});
```

### Example 4: Zoom Slider (25%-400%)

```cpp
// Create zoom slider
auto zoom = std::make_unique<slider<Backend>>();
zoom->set_range(25, 400);
zoom->set_value(100);  // 100%
zoom->set_single_step(25);
zoom->set_tick_interval(50);
zoom->set_value_visible(true);

zoom->value_changed.connect([](int value) {
    document_view->set_zoom_percent(value);
});

// Reset zoom to 100% with Home key
zoom->set_range(25, 400);
```

### Example 5: Playback Position Slider

```cpp
// Create playback position slider (time in seconds)
auto position = std::make_unique<slider<Backend>>();
position->set_range(0, video_duration_seconds);
position->set_value(0);
position->set_tracking(false);  // Only seek on mouse release

// Update slider when video plays
video->playback_updated.connect([position](int current_time) {
    position->set_value(current_time);
});

// Seek when user drags slider
position->slider_released.connect([position]() {
    video->seek(position->value());
});
```

---

## Testing Strategy

### Test Plan

**File**: `unittest/widgets/test_slider.cc`

#### Test Categories

1. **Construction** (3 tests)
   - Default construction
   - Construction with orientation
   - Initial state verification

2. **Value Management** (5 tests)
   - Set/get value
   - Value clamping to range
   - Value snapping to step
   - value_changed signal emission
   - Tracking mode (emit during drag vs on release)

3. **Range Management** (3 tests)
   - Set/get range
   - Value clamping when range changes
   - Invalid range handling (min >= max)

4. **Step Management** (4 tests)
   - Single step increment/decrement
   - Page step increment/decrement
   - Step snapping behavior
   - Step = 0 (smooth values)

5. **Orientation** (2 tests)
   - Set/get orientation
   - Layout invalidation on orientation change

6. **Tick Marks** (3 tests)
   - Set/get tick position
   - Set/get tick interval
   - Tick rendering (visual verification)

7. **Value Display** (2 tests)
   - Set/get value visibility
   - Value label rendering

8. **Mouse Interaction** (6 tests)
   - Click on thumb starts drag
   - Drag thumb updates value
   - Release thumb emits slider_released
   - Click on track jumps to position
   - Hover state updates
   - Mouse move during drag emits slider_moved

9. **Keyboard Interaction** (6 tests)
   - Arrow keys change by single_step
   - Page Up/Down change by page_step
   - Home key jumps to minimum
   - End key jumps to maximum
   - Orientation affects arrow key behavior
   - Focus required for keyboard input

10. **Signals** (4 tests)
    - value_changed emits on set_value()
    - slider_moved emits during drag
    - slider_pressed emits on mouse down
    - slider_released emits on mouse up

11. **Layout and Measurement** (3 tests)
    - Measure returns correct size (horizontal)
    - Measure returns correct size (vertical)
    - Tick marks affect measured size

12. **Edge Cases** (4 tests)
    - Zero range (min == max)
    - Negative range
    - Large range (stress test)
    - Disabled state prevents interaction

**Total**: ~45 test cases, ~120 assertions

### Test Example

```cpp
#include <doctest/doctest.h>
#include <onyxui/widgets/input/slider.hh>
#include "../../utils/ui_context_fixture.hh"

using namespace onyxui;

TEST_SUITE("slider") {

TEST_CASE("slider - Construction") {
    ui_context_fixture<test_canvas_backend> fixture;

    auto s = std::make_unique<slider<test_canvas_backend>>();

    REQUIRE(s->value() == 0);
    REQUIRE(s->minimum() == 0);
    REQUIRE(s->maximum() == 100);
    REQUIRE(s->single_step() == 1);
    REQUIRE(s->page_step() == 10);
    REQUIRE(s->orientation() == slider_orientation::horizontal);
    REQUIRE(s->get_tick_position() == tick_position::none);
    REQUIRE(s->tick_interval() == 0);
    REQUIRE_FALSE(s->is_value_visible());
    REQUIRE(s->has_tracking());
}

TEST_CASE("slider - Set and get value") {
    ui_context_fixture<test_canvas_backend> fixture;

    auto s = std::make_unique<slider<test_canvas_backend>>();
    s->set_range(0, 100);
    s->set_value(50);

    REQUIRE(s->value() == 50);
}

TEST_CASE("slider - Value clamping") {
    ui_context_fixture<test_canvas_backend> fixture;

    auto s = std::make_unique<slider<test_canvas_backend>>();
    s->set_range(0, 100);

    s->set_value(-10);
    REQUIRE(s->value() == 0);  // Clamped to min

    s->set_value(150);
    REQUIRE(s->value() == 100);  // Clamped to max
}

TEST_CASE("slider - Value snapping to step") {
    ui_context_fixture<test_canvas_backend> fixture;

    auto s = std::make_unique<slider<test_canvas_backend>>();
    s->set_range(0, 100);
    s->set_single_step(5);

    s->set_value(42);
    REQUIRE(s->value() == 40);  // Snapped to nearest multiple of 5

    s->set_value(43);
    REQUIRE(s->value() == 45);  // Snapped up
}

TEST_CASE("slider - value_changed signal") {
    ui_context_fixture<test_canvas_backend> fixture;

    auto s = std::make_unique<slider<test_canvas_backend>>();
    s->set_range(0, 100);

    int last_value = -1;
    s->value_changed.connect([&last_value](int value) {
        last_value = value;
    });

    s->set_value(75);
    REQUIRE(last_value == 75);
}

TEST_CASE("slider - Tracking mode") {
    ui_context_fixture<test_canvas_backend> fixture;

    auto s = std::make_unique<slider<test_canvas_backend>>();
    s->set_range(0, 100);
    s->set_tracking(false);  // Disable tracking

    int value_changed_count = 0;
    s->value_changed.connect([&value_changed_count](int) {
        value_changed_count++;
    });

    // Simulate drag (should NOT emit value_changed)
    // (Drag simulation code here)

    REQUIRE(value_changed_count == 0);

    // Simulate mouse release (should emit value_changed)
    // (Release simulation code here)

    REQUIRE(value_changed_count == 1);
}

TEST_CASE("slider - Arrow keys change value") {
    ui_context_fixture<test_canvas_backend> fixture;

    auto s = std::make_unique<slider<test_canvas_backend>>();
    s->set_range(0, 100);
    s->set_value(50);
    s->set_single_step(5);

    // Simulate Right arrow key (menu_right semantic action)
    bool handled = s->handle_semantic_action(hotkey_action::menu_right);

    REQUIRE(handled);
    REQUIRE(s->value() == 55);  // Increased by single_step
}

TEST_CASE("slider - Page Up/Down keys") {
    ui_context_fixture<test_canvas_backend> fixture;

    auto s = std::make_unique<slider<test_canvas_backend>>();
    s->set_range(0, 100);
    s->set_value(50);
    s->set_page_step(10);

    // Simulate Page Up
    s->handle_semantic_action(hotkey_action::page_up);
    REQUIRE(s->value() == 60);

    // Simulate Page Down
    s->handle_semantic_action(hotkey_action::page_down);
    REQUIRE(s->value() == 50);
}

TEST_CASE("slider - Home/End keys") {
    ui_context_fixture<test_canvas_backend> fixture;

    auto s = std::make_unique<slider<test_canvas_backend>>();
    s->set_range(0, 100);
    s->set_value(50);

    // Home key jumps to minimum
    s->handle_semantic_action(hotkey_action::move_to_start);
    REQUIRE(s->value() == 0);

    // End key jumps to maximum
    s->handle_semantic_action(hotkey_action::move_to_end);
    REQUIRE(s->value() == 100);
}

}  // TEST_SUITE
```

---

## Implementation Checklist

### Phase 1: Core Widget (Day 1)

- [ ] Create `include/onyxui/widgets/input/slider.hh`
- [ ] Implement slider class structure
- [ ] Add value management (set_value, get_value, clamping, snapping)
- [ ] Add range management (set_range, min, max)
- [ ] Add step management (single_step, page_step)
- [ ] Add orientation support (horizontal/vertical)
- [ ] Implement value_changed signal
- [ ] Write basic construction tests (3 tests)
- [ ] Write value management tests (5 tests)
- [ ] Write range management tests (3 tests)
- [ ] Write step management tests (4 tests)

### Phase 2: Rendering (Day 2)

- [ ] Implement do_render() for horizontal slider
  - [ ] Draw track (filled + empty portions)
  - [ ] Draw thumb at correct position
- [ ] Implement do_render() for vertical slider
- [ ] Add tick mark rendering
  - [ ] Calculate tick positions
  - [ ] Draw ticks above/below/both sides
- [ ] Add value label rendering
- [ ] Update `theme.hh` with slider_style structure
- [ ] Update `conio_themes.hh` with default slider theme
- [ ] Update Norton Blue YAML theme with slider configuration
- [ ] Write layout/measurement tests (3 tests)

### Phase 3: Mouse Interaction (Day 3)

- [ ] Implement mouse event handling
  - [ ] Mouse press on thumb starts drag
  - [ ] Mouse press on track jumps to position
  - [ ] Mouse move during drag updates value
  - [ ] Mouse release ends drag
- [ ] Implement hover state detection
- [ ] Add slider_moved signal
- [ ] Add slider_pressed signal
- [ ] Add slider_released signal
- [ ] Implement tracking mode (emit during drag vs on release)
- [ ] Write mouse interaction tests (6 tests)
- [ ] Write signal emission tests (4 tests)

### Phase 4: Keyboard Interaction (Day 4)

- [ ] Implement handle_semantic_action()
  - [ ] Arrow keys increment/decrement by single_step
  - [ ] Page Up/Down increment/decrement by page_step
  - [ ] Home/End jump to min/max
- [ ] Handle orientation-specific arrow key behavior
- [ ] Ensure focus required for keyboard input
- [ ] Write keyboard interaction tests (6 tests)

### Phase 5: Tick Marks & Polish (Day 5)

- [ ] Implement tick mark API (set_tick_position, set_tick_interval)
- [ ] Implement value label API (set_value_visible)
- [ ] Fine-tune rendering (colors, spacing, alignment)
- [ ] Add disabled state styling
- [ ] Write tick mark tests (3 tests)
- [ ] Write value display tests (2 tests)
- [ ] Write edge case tests (4 tests)
- [ ] Add to `unittest/CMakeLists.txt`

### Phase 6: Integration & Documentation

- [ ] Add slider to demo UI (`examples/demo_ui_builder.hh`)
  - [ ] Horizontal volume slider
  - [ ] Vertical fader
- [ ] Update `docusaurus/docs/api-reference/widgets/slider.md`
- [ ] Update `docusaurus/docs/api-reference/widget-library.md`
- [ ] Update `docs/WIDGET_ROADMAP.md` (mark Phase 3 complete)
- [ ] Build and verify all tests pass
- [ ] Visual testing in conio demo
- [ ] Commit implementation

---

## Implementation Notes

### Position-to-Value Mapping

**Horizontal Slider:**
```cpp
int slider<Backend>::position_to_value(int x, int y) const {
    auto track = track_rect();
    int track_x = rect_utils::get_x(track);
    int track_width = rect_utils::get_width(track);

    // Clamp x to track bounds
    int relative_x = std::clamp(x - track_x, 0, track_width);

    // Map to value range
    float ratio = static_cast<float>(relative_x) / track_width;
    int value = m_min + static_cast<int>(ratio * (m_max - m_min));

    return clamp_value(snap_to_step(value));
}
```

**Vertical Slider:**
```cpp
// Vertical sliders go bottom-to-top (inverted Y axis)
int relative_y = track_height - (y - track_y);
float ratio = static_cast<float>(relative_y) / track_height;
int value = m_min + static_cast<int>(ratio * (m_max - m_min));
```

### Thumb Size Considerations

- **DOS/TUI**: Thumb is 1-3 characters wide (configurable via theme)
- **GUI**: Thumb is typically 16-24 pixels diameter
- Thumb size affects clickable area and visual prominence

### Performance Optimizations

- Cache track_rect() and thumb_rect() after layout
- Only recalculate positions when value or bounds change
- Throttle value updates during drag (if needed)

### Accessibility

- Ensure keyboard navigation works without mouse
- Provide value label for screen readers
- Use semantic actions for customizable keybindings

---

## Success Criteria

After implementation, the slider widget must:

- ✅ Display horizontal and vertical orientations correctly
- ✅ Respond to mouse drag (smooth value updates)
- ✅ Respond to keyboard (arrow keys, page up/down, home/end)
- ✅ Snap values to step increments
- ✅ Emit signals correctly (value_changed, slider_moved, etc.)
- ✅ Support tick marks at regular intervals
- ✅ Display optional value label near thumb
- ✅ Integrate with theme system (colors, fonts, dimensions)
- ✅ Work in disabled state (no interaction)
- ✅ Pass all 45+ test cases with zero warnings

---

## Next Steps

1. **Create slider.hh** with complete API
2. **Implement core value/range logic** (TDD approach)
3. **Add rendering** (track, thumb, ticks)
4. **Implement mouse drag** interaction
5. **Add keyboard navigation**
6. **Write comprehensive tests** (~45 test cases)
7. **Integrate with demo** UI
8. **Document** in Docusaurus

**Estimated Time**: 5 days (Phase 3 of widget roadmap)

---

## Questions for Review

1. Should we support floating-point values (double) in addition to int?
   - Pros: More precise for percentages (0.0-1.0)
   - Cons: More complex, two template specializations needed

2. Should thumb size be configurable per-instance or theme-only?
   - Current design: Theme-only (consistent appearance)
   - Alternative: Per-instance API (set_thumb_size)

3. Should we add snap-to-tick feature (thumb jumps to nearest tick)?
   - Useful for discrete values (e.g., font sizes: 8, 10, 12, 14, 16)
   - Can be implemented as optional mode

4. Should we support custom tick mark labels (e.g., "Low", "Medium", "High")?
   - Deferred to future enhancement
   - Current design: numeric value only

---

**Ready to implement!** 🚀
