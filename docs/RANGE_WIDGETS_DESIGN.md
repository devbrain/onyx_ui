# Range Widgets Design: Slider & Progress Bar

**Status**: Design Phase
**Widgets**: `progress_bar<Backend>` + `slider<Backend>`
**Phase**: 3 (Visual Feedback Widgets)
**Created**: 2025-11-20
**Author**: Architecture Team

---

## Table of Contents

1. [Overview](#overview)
2. [Architecture Analysis](#architecture-analysis)
3. [Shared Components](#shared-components)
4. [Progress Bar Design](#progress-bar-design)
5. [Slider Design](#slider-design)
6. [Implementation Strategy](#implementation-strategy)
7. [Testing Strategy](#testing-strategy)
8. [Implementation Checklist](#implementation-checklist)

---

## Overview

### What Are Range Widgets?

**Range widgets** visualize numeric values within a range using a filled track:

| Widget | Purpose | Interactive | User Input |
|--------|---------|-------------|------------|
| **Progress Bar** | Visual feedback | ❌ No | Programmatic only |
| **Slider** | Value input | ✅ Yes | Mouse drag + keyboard |

### Visual Comparison

```
Progress Bar (45% complete):
┌────────────────────────────────────┐
│████████████████░░░░░░░░░░░░░░░░░░│ 45%
└────────────────────────────────────┘
    ▲ Filled          ▲ Empty

Slider (value 40/100):
┌────────────────────────────────────┐
│████████████░░░░░░░░░░░░░░░░░░░░░░│
│            ●                       │
└────────────────────────────────────┘
    ▲ Filled   ▲ Thumb   ▲ Empty
```

**Key Insight**: Slider = Progress Bar + Thumb + Interaction

---

## Architecture Analysis

### Similarities

Both widgets share:
1. **Track rendering** (filled + empty portions)
2. **Orientation** (horizontal/vertical)
3. **Value-to-pixel mapping** (position calculations)
4. **Range management** (min/max values)
5. **Theme integration** (colors, styles)
6. **Layout** (measure/arrange logic similar)

### Differences

| Aspect | Progress Bar | Slider |
|--------|--------------|--------|
| **Thumb** | ❌ None | ✅ Draggable indicator |
| **Interaction** | ❌ None | ✅ Mouse drag, keyboard |
| **Mode** | Determinate + Indeterminate | Determinate only |
| **Text Overlay** | ✅ "45%" or custom | ✅ Optional value label |
| **Animation** | ✅ Indeterminate mode | ❌ None |
| **Signals** | `value_changed` (rarely used) | `value_changed`, `slider_moved` |
| **Tick Marks** | ❌ None | ✅ Optional |
| **Step Snapping** | ❌ None | ✅ Optional |

### Architecture Decision: Shared Utilities + Independent Widgets

```
┌─────────────────────────────────────────────┐
│  range_helpers.hh (Shared Utilities)        │
│                                             │
│  - pixel_to_value()                         │
│  - value_to_pixel()                         │
│  - snap_to_step()                           │
│  - render_track()  ← NEW                    │
│  - render_filled_portion()  ← NEW           │
└─────────────────────────────────────────────┘
           ▲                    ▲
           │ Uses               │ Uses
           │                    │
  ┌────────┴────────┐  ┌───────┴────────┐
  │ progress_bar    │  │    slider      │
  │                 │  │                │
  │ - Display only  │  │ - Interactive  │
  │ - Indeterminate │  │ - Thumb + drag │
  │ - Text overlay  │  │ - Tick marks   │
  │ - Animation     │  │ - Keyboard nav │
  └─────────────────┘  └────────────────┘
```

**Why Not Inheritance?**
- Different widget purposes (display vs input)
- Different complexity levels
- Avoid "slider is-a progress_bar" confusion
- Easier to maintain independently

**Why Shared Utilities?**
- Reuse track rendering logic
- Consistent visual appearance
- Reduce code duplication
- Easier to test

---

## Shared Components

### Extended `range_helpers.hh`

```cpp
namespace onyxui::range_helpers {

// ===== Existing Functions =====

/// Map position (in backend units) to value within range
///
/// @param position Position along track (in backend units: chars for conio, pixels for SDL)
/// @param track_length Length of track (in backend units)
/// @param min_value Minimum value in range
/// @param max_value Maximum value in range
/// @return Value corresponding to position (not clamped or snapped)
template<typename T = int>
[[nodiscard]] T position_to_value(int position, int track_length, T min_value, T max_value);

/// Map value to position (in backend units) within track
///
/// @param value Current value
/// @param min_value Minimum value in range
/// @param max_value Maximum value in range
/// @param track_length Length of track (in backend units)
/// @return Position along track (in backend units: 0 = start, track_length = end)
template<typename T = int>
[[nodiscard]] int value_to_position(T value, T min_value, T max_value, int track_length);

/// Snap value to step increments
///
/// @param value Value to snap
/// @param step Step size (0 = no snapping)
/// @return Snapped value (or original if step == 0)
template<typename T = int>
[[nodiscard]] T snap_to_step(T value, T step);

// ===== New Track Rendering Functions =====

/// Render horizontal track with filled/empty portions
///
/// @param ctx Render context
/// @param track_rect Track bounding rectangle (absolute coords, in backend units)
/// @param fill_width Width of filled portion (in backend units: chars for conio, pixels for SDL)
/// @param filled_color Color for filled portion
/// @param empty_color Color for empty portion
template<UIBackend Backend>
void render_horizontal_track(
    render_context<Backend>& ctx,
    const typename Backend::rect_type& track_rect,
    int fill_width,
    typename Backend::color_type filled_color,
    typename Backend::color_type empty_color
);

/// Render vertical track with filled/empty portions
///
/// @param ctx Render context
/// @param track_rect Track bounding rectangle (absolute coords, in backend units)
/// @param fill_height Height of filled portion (in backend units)
/// @param filled_color Color for filled portion
/// @param empty_color Color for empty portion
/// @param bottom_to_top If true, fill from bottom up (default for vertical)
template<UIBackend Backend>
void render_vertical_track(
    render_context<Backend>& ctx,
    const typename Backend::rect_type& track_rect,
    int fill_height,
    typename Backend::color_type filled_color,
    typename Backend::color_type empty_color,
    bool bottom_to_top = true
);

/// Calculate fill width/height from value
///
/// @param value Current value
/// @param min_value Minimum value
/// @param max_value Maximum value
/// @param track_length Track length (in backend units)
/// @return Fill length (in backend units: 0 to track_length)
///
/// @note Backend units: characters for conio, pixels for SDL/GUI backends
template<typename T = int>
[[nodiscard]] inline int calculate_fill_length(
    T value,
    T min_value,
    T max_value,
    int track_length
) {
    if (max_value <= min_value || track_length <= 0) {
        return 0;
    }

    T clamped = std::clamp(value, min_value, max_value);
    float ratio = static_cast<float>(clamped - min_value) / (max_value - min_value);
    return static_cast<int>(ratio * track_length);
}

}  // namespace onyxui::range_helpers
```

### Shared Theme Structure

Both widgets use similar theme properties:

```cpp
// In theme.hh

struct progress_bar_style {
    visual_state normal;           // Normal state
    visual_state disabled;         // Disabled state
    color_type filled_color;       // Filled portion color
    color_type empty_color;        // Empty portion color
    int track_thickness;           // Track height (horizontal) or width (vertical)

    // Text overlay
    font_type text_font;
    color_type text_color;

    // Indeterminate animation
    int animation_width;           // Width of moving stripe
    int animation_speed;           // Pixels per frame
};

struct slider_style {
    visual_state track_normal;     // Track colors
    visual_state track_disabled;
    color_type filled_color;       // Filled portion color
    color_type empty_color;        // Empty portion color

    // Thumb
    visual_state thumb_normal;
    visual_state thumb_hover;
    visual_state thumb_pressed;
    visual_state thumb_disabled;
    int thumb_size;                // Thumb diameter

    // Tick marks
    color_type tick_color;
    int tick_length;

    // Value label
    font_type value_font;
    color_type value_color;

    // Dimensions
    int track_thickness;
    int spacing;
};
```

---

## Progress Bar Design

### Purpose

Display **progress of operations** (downloads, loading, processing) with optional text overlay.

### Key Features

✅ Determinate mode (0-100%)
✅ Indeterminate mode (busy indicator, animated)
✅ Horizontal/vertical orientation
✅ Text overlay (e.g., "45% complete")
✅ Customizable text format
✅ Theme integration

### Visual Design

**Horizontal Progress Bar (45%):**
```
┌────────────────────────────────────┐
│████████████████░░░░░░░░░░░░░░░░░░│ 45%
└────────────────────────────────────┘

With custom text:
┌────────────────────────────────────┐
│████████████████░░░░░░░░░░░░░░░░░░│ 45/100 files
└────────────────────────────────────┘
```

**Indeterminate (animated):**
```
Frame 1:
┌────────────────────────────────────┐
│▒▒▒▒████████▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒│
└────────────────────────────────────┘

Frame 2 (stripe moved):
┌────────────────────────────────────┐
│▒▒▒▒▒▒▒▒████████▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒│
└────────────────────────────────────┘
```

**Vertical Progress Bar (60%):**
```
100 ─┬─
     │░
  80 ─┼─
     │░
  60 ─┼─  ← 60%
     │▓
  40 ─┼─
     │▓
  20 ─┼─
     │▓
   0 ─┴─
```

### API Design

```cpp
namespace onyxui {

enum class progress_bar_orientation : std::uint8_t {
    horizontal,
    vertical
};

template<UIBackend Backend>
class progress_bar : public widget<Backend> {
public:
    using base = widget<Backend>;

    // ===== Construction =====

    explicit progress_bar(ui_element<Backend>* parent = nullptr);

    // ===== Value Management =====

    /// Set current value (0-100 for percentage, or custom range)
    void set_value(int value);

    [[nodiscard]] int value() const noexcept { return m_value; }

    /// Set value range (default 0-100)
    void set_range(int min, int max);

    [[nodiscard]] int minimum() const noexcept { return m_min; }
    [[nodiscard]] int maximum() const noexcept { return m_max; }

    // ===== Indeterminate Mode =====

    /// Set indeterminate mode (animated busy indicator)
    void set_indeterminate(bool indeterminate);

    [[nodiscard]] bool is_indeterminate() const noexcept { return m_indeterminate; }

    // ===== Orientation =====

    void set_orientation(progress_bar_orientation orientation);

    [[nodiscard]] progress_bar_orientation orientation() const noexcept {
        return m_orientation;
    }

    // ===== Text Overlay =====

    /// Set text visibility
    void set_text_visible(bool visible);

    [[nodiscard]] bool is_text_visible() const noexcept { return m_text_visible; }

    /// Set text format string
    /// Placeholders: %v = value, %p = percentage, %m = maximum
    /// Example: "%v/%m files" → "45/100 files"
    void set_text_format(const std::string& format);

    [[nodiscard]] std::string text_format() const noexcept { return m_text_format; }

    /// Get formatted text for current value
    [[nodiscard]] std::string formatted_text() const;

    // ===== Signals =====

    /// Emitted when value changes
    signal<int> value_changed;

protected:
    void do_render(render_context<Backend>& ctx) const override;

private:
    int m_value = 0;
    int m_min = 0;
    int m_max = 100;
    bool m_indeterminate = false;
    progress_bar_orientation m_orientation = progress_bar_orientation::horizontal;
    bool m_text_visible = false;
    std::string m_text_format = "%p%";  // Default: "45%"

    // Animation state (for indeterminate mode)
    mutable int m_animation_offset = 0;

    // Helper: format text with placeholders
    [[nodiscard]] std::string format_text_internal() const;
};

}  // namespace onyxui
```

### Usage Examples

**Example 1: File Download Progress**
```cpp
auto progress = std::make_unique<progress_bar<Backend>>();
progress->set_range(0, 100);
progress->set_value(0);
progress->set_text_visible(true);
progress->set_text_format("%v%");

// Update as download progresses
download->progress_updated.connect([progress](int percent) {
    progress->set_value(percent);
});
```

**Example 2: Indeterminate Loading**
```cpp
auto loading = std::make_unique<progress_bar<Backend>>();
loading->set_indeterminate(true);  // Animated busy indicator
loading->set_text_visible(true);
loading->set_text_format("Loading...");

// Stop animation when done
task_completed.connect([loading]() {
    loading->set_indeterminate(false);
    loading->set_value(100);
});
```

**Example 3: File Copy with Custom Text**
```cpp
auto copy_progress = std::make_unique<progress_bar<Backend>>();
copy_progress->set_range(0, total_files);
copy_progress->set_text_visible(true);
copy_progress->set_text_format("%v/%m files copied");

// Update per file
file_copied.connect([copy_progress](int count) {
    copy_progress->set_value(count);
    // Displays: "45/100 files copied"
});
```

---

## Slider Design

### Purpose

Interactive **numeric value input** with visual feedback via draggable thumb.

### Key Features

✅ Mouse drag interaction
✅ Keyboard navigation (arrows, page up/down, home/end)
✅ Step-based value snapping
✅ Optional tick marks at intervals
✅ Optional value display near thumb
✅ Horizontal/vertical orientation
✅ Tracking mode (emit during drag vs on release)

### Visual Design

**Horizontal Slider (40%):**
```
┌────────────────────────────────────┐
│████████████░░░░░░░░░░░░░░░░░░░░░░│
│            ● 40                    │  ← Thumb + value label
└────────────────────────────────────┘
 |     |     |     |     |     |     |  ← Tick marks
 0    20    40    60    80   100
```

**Vertical Slider (60%):**
```
100 ─┬─
   | │░ |
  80 ─┼─
   | │░ |
  60 ─●─  ← Thumb at 60%
   | │▓ |
  40 ─┼─
   | │▓ |
  20 ─┼─
   | │▓ |
   0 ─┴─
```

### API Design

```cpp
namespace onyxui {

enum class slider_orientation : std::uint8_t {
    horizontal,
    vertical
};

enum class tick_position : std::uint8_t {
    none,
    above,       // Above track (horizontal) or left (vertical)
    below,       // Below track (horizontal) or right (vertical)
    both_sides
};

template<UIBackend Backend>
class slider : public widget<Backend> {
public:
    using base = widget<Backend>;

    // ===== Construction =====

    explicit slider(ui_element<Backend>* parent = nullptr);
    explicit slider(slider_orientation orientation, ui_element<Backend>* parent = nullptr);

    // ===== Value Management =====

    void set_value(int value);
    [[nodiscard]] int value() const noexcept { return m_value; }

    void set_range(int min, int max);
    [[nodiscard]] int minimum() const noexcept { return m_min; }
    [[nodiscard]] int maximum() const noexcept { return m_max; }

    // ===== Step Management =====

    void set_single_step(int step);  // Arrow key increment
    void set_page_step(int step);    // Page Up/Down increment

    [[nodiscard]] int single_step() const noexcept { return m_single_step; }
    [[nodiscard]] int page_step() const noexcept { return m_page_step; }

    // ===== Orientation =====

    void set_orientation(slider_orientation orientation);
    [[nodiscard]] slider_orientation orientation() const noexcept { return m_orientation; }

    // ===== Tick Marks =====

    void set_tick_position(tick_position position);
    void set_tick_interval(int interval);

    [[nodiscard]] tick_position get_tick_position() const noexcept { return m_tick_position; }
    [[nodiscard]] int tick_interval() const noexcept { return m_tick_interval; }

    // ===== Value Display =====

    void set_value_visible(bool visible);
    [[nodiscard]] bool is_value_visible() const noexcept { return m_value_visible; }

    // ===== Tracking Mode =====

    /// If tracking enabled, value_changed emits during drag
    /// If disabled, value_changed only emits on mouse release
    void set_tracking(bool enable);
    [[nodiscard]] bool has_tracking() const noexcept { return m_tracking; }

    // ===== Signals =====

    signal<int> value_changed;     // Value changed (respects tracking mode)
    signal<int> slider_moved;      // Emitted during drag
    signal<> slider_pressed;       // Mouse down on thumb
    signal<> slider_released;      // Mouse up after drag

protected:
    void do_render(render_context<Backend>& ctx) const override;
    bool handle_event(const ui_event& event, event_phase phase) override;
    bool handle_semantic_action(hotkey_action action) override;

private:
    int m_value = 0;
    int m_min = 0;
    int m_max = 100;
    int m_single_step = 1;
    int m_page_step = 10;
    slider_orientation m_orientation = slider_orientation::horizontal;
    tick_position m_tick_position = tick_position::none;
    int m_tick_interval = 0;
    bool m_value_visible = false;
    bool m_tracking = true;

    // Drag state
    bool m_is_dragging = false;
    bool m_is_hovered = false;

    // Helpers
    [[nodiscard]] int position_to_value(int x, int y) const;
    [[nodiscard]] typename Backend::rect_type thumb_rect() const;
    [[nodiscard]] typename Backend::rect_type track_rect() const;
    [[nodiscard]] bool is_point_in_thumb(int x, int y) const;
};

}  // namespace onyxui
```

### Usage Examples

See `SLIDER_DESIGN.md` for comprehensive examples.

---

## Implementation Strategy

### Phase 1: Shared Utilities (Day 1)

**Goal**: Create reusable track rendering utilities

1. **Extend `include/onyxui/widgets/utils/range_helpers.hh`**
   - Add `render_horizontal_track()`
   - Add `render_vertical_track()`
   - Add `calculate_fill_length()`
   - Write unit tests for new functions

2. **Test Coverage**:
   - Test horizontal track rendering
   - Test vertical track rendering
   - Test fill length calculation edge cases
   - Visual verification in test backend

### Phase 2: Progress Bar (Days 2-3)

**Goal**: Simple display widget for progress feedback

**Day 2 - Core Features**:
1. Create `include/onyxui/widgets/display/progress_bar.hh`
2. Implement value management (set_value, range)
3. Implement orientation support
4. Implement do_render() using `range_helpers::render_*_track()`
5. Add text overlay rendering
6. Write tests (20 test cases):
   - Construction
   - Value/range management
   - Orientation
   - Text overlay
   - Signals

**Day 3 - Indeterminate Mode**:
1. Implement indeterminate mode (animated stripe)
2. Add animation state tracking
3. Update rendering for animation
4. Add to theme system
5. Write tests (10 test cases):
   - Indeterminate mode
   - Animation state
   - Mode switching
6. Add to demo UI

### Phase 3: Slider (Days 4-5)

**Goal**: Interactive value input widget

**Day 4 - Core Features**:
1. Create `include/onyxui/widgets/input/slider.hh`
2. Implement value/range/step management
3. Implement do_render() using `range_helpers` + thumb
4. Implement mouse drag (handle_event)
5. Implement keyboard navigation (handle_semantic_action)
6. Write tests (30 test cases):
   - Construction, value, range, steps
   - Mouse interaction
   - Keyboard navigation
   - Signals

**Day 5 - Polish**:
1. Implement tick marks rendering
2. Implement value label display
3. Add disabled state
4. Fine-tune theming
5. Write tests (15 test cases):
   - Tick marks
   - Value display
   - Edge cases
6. Add to demo UI

### Phase 4: Integration & Documentation (Day 6)

1. Add both widgets to demo UI
2. Create Docusaurus documentation:
   - `progress-bar.md`
   - `slider.md`
3. Update `widget-library.md`
4. Update `WIDGET_ROADMAP.md`
5. Verify all tests pass (1500+ total)
6. Commit

---

## Testing Strategy

### Progress Bar Tests (~30 test cases)

**File**: `unittest/widgets/test_progress_bar.cc`

1. **Construction** (3 tests)
   - Default construction
   - Orientation construction
   - Initial state

2. **Value Management** (5 tests)
   - Set/get value
   - Value clamping
   - Range changes
   - value_changed signal

3. **Indeterminate Mode** (5 tests)
   - Enable/disable indeterminate
   - Animation state
   - Mode switching
   - Rendering verification

4. **Orientation** (2 tests)
   - Horizontal rendering
   - Vertical rendering

5. **Text Overlay** (8 tests)
   - Show/hide text
   - Text format placeholders (%v, %p, %m)
   - Custom format strings
   - Text rendering position

6. **Layout** (4 tests)
   - Measure horizontal
   - Measure vertical
   - Arrange
   - Theme dimensions

7. **Edge Cases** (3 tests)
   - Zero range (min == max)
   - Negative values
   - Disabled state

### Slider Tests (~45 test cases)

**File**: `unittest/widgets/test_slider.cc`

See `SLIDER_DESIGN.md` for comprehensive test plan (45 test cases).

### Utility Tests (~15 test cases)

**File**: `unittest/utils/test_range_helpers.cc`

1. **Value Mapping** (5 tests)
   - pixel_to_value() accuracy
   - value_to_pixel() accuracy
   - Boundary cases
   - Zero-length tracks

2. **Step Snapping** (3 tests)
   - Snap to multiples
   - Round up/down logic
   - Zero step (no snapping)

3. **Fill Length** (3 tests)
   - Calculate fill length
   - Zero/full range
   - Negative ranges

4. **Track Rendering** (4 tests)
   - Horizontal track
   - Vertical track
   - Partial fill
   - Full/empty states

**Total**: ~90 test cases, ~250 assertions

---

## Implementation Checklist

### Phase 1: Shared Utilities (Day 1)

- [ ] Extend `include/onyxui/widgets/utils/range_helpers.hh`
  - [ ] Add `render_horizontal_track()`
  - [ ] Add `render_vertical_track()`
  - [ ] Add `calculate_fill_length()`
- [ ] Create `unittest/utils/test_range_helpers.cc`
- [ ] Write 15 utility tests
- [ ] Verify all tests pass

### Phase 2: Progress Bar (Days 2-3)

- [ ] Create `include/onyxui/widgets/display/progress_bar.hh`
- [ ] Implement progress_bar class
  - [ ] Value/range management
  - [ ] Orientation support
  - [ ] Text overlay with format strings
  - [ ] Indeterminate mode with animation
- [ ] Add `progress_bar_style` to `theme.hh`
- [ ] Update `conio_themes.hh` with default theme
- [ ] Update Norton Blue YAML theme
- [ ] Create `unittest/widgets/test_progress_bar.cc`
- [ ] Write 30 progress_bar tests
- [ ] Add to `unittest/CMakeLists.txt`
- [ ] Verify all tests pass

### Phase 3: Slider (Days 4-5)

- [ ] Create `include/onyxui/widgets/input/slider.hh`
- [ ] Implement slider class
  - [ ] Value/range/step management
  - [ ] Mouse drag interaction
  - [ ] Keyboard navigation
  - [ ] Tick marks rendering
  - [ ] Value label display
- [ ] Add `slider_style` to `theme.hh`
- [ ] Update `conio_themes.hh` with default theme
- [ ] Update Norton Blue YAML theme
- [ ] Create `unittest/widgets/test_slider.cc`
- [ ] Write 45 slider tests
- [ ] Add to `unittest/CMakeLists.txt`
- [ ] Verify all tests pass

### Phase 4: Integration & Documentation (Day 6)

- [ ] Add progress_bar to demo UI
  - [ ] File download simulation
  - [ ] Indeterminate loading example
- [ ] Add slider to demo UI
  - [ ] Volume control
  - [ ] Vertical fader
- [ ] Create `docusaurus/docs/api-reference/widgets/progress-bar.md`
- [ ] Create `docusaurus/docs/api-reference/widgets/slider.md`
- [ ] Update `docusaurus/docs/api-reference/widget-library.md`
- [ ] Update `docs/WIDGET_ROADMAP.md` (mark Phase 3 complete)
- [ ] Build and verify (1500+ tests passing)
- [ ] Visual testing in conio demo
- [ ] Commit implementation

---

## Success Criteria

After implementation, both widgets must:

### Progress Bar
- ✅ Display horizontal and vertical orientations
- ✅ Show filled portion proportional to value
- ✅ Support indeterminate mode (animated)
- ✅ Render text overlay with format strings
- ✅ Integrate with theme system
- ✅ Pass all 30 test cases

### Slider
- ✅ Display horizontal and vertical orientations
- ✅ Respond to mouse drag (smooth value updates)
- ✅ Respond to keyboard (arrows, page up/down, home/end)
- ✅ Snap values to step increments
- ✅ Emit signals correctly
- ✅ Support tick marks and value label
- ✅ Integrate with theme system
- ✅ Pass all 45 test cases

### Shared Utilities
- ✅ Utility functions work for both widgets
- ✅ Track rendering produces consistent visuals
- ✅ Pass all 15 utility tests

### Overall
- ✅ **All 1500+ tests pass** with zero warnings
- ✅ Widgets visually tested in demo
- ✅ Documentation complete
- ✅ Code review ready

---

## Timeline

**6-Day Implementation Plan**:

| Day | Focus | Deliverables |
|-----|-------|--------------|
| **1** | Shared Utilities | `range_helpers.hh` + 15 tests |
| **2** | Progress Bar Core | Value/range/orientation/text + 20 tests |
| **3** | Progress Bar Polish | Indeterminate mode + 10 tests + demo |
| **4** | Slider Core | Value/drag/keyboard + 30 tests |
| **5** | Slider Polish | Ticks/labels + 15 tests + demo |
| **6** | Integration | Docs + final testing + commit |

**Total**: 6 days, 90 test cases, 2 production-ready widgets

---

## Next Steps

Ready to start implementation! Suggested order:

1. **Day 1**: Create `range_helpers.hh` with shared utilities
2. **Days 2-3**: Implement `progress_bar` (simpler widget first)
3. **Days 4-5**: Implement `slider` (reuses utilities from progress_bar)
4. **Day 6**: Integration, documentation, commit

Would you like me to:
1. ✅ Start with Phase 1 (shared utilities)?
2. ✅ Implement both widgets in sequence?
3. 📋 Create tracking todo list?

**Let's build these widgets!** 🎯📊🎚️
