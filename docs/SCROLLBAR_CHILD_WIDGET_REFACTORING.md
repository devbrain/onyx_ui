# Scrollbar Child Widget Architecture - Implementation Plan

**Status**: NOT STARTED
**Date**: 2025-11-06
**Prerequisites**: Scrollbar visibility fixes completed (all 1215 tests passing)

## Executive Summary

Refactor the scrollbar widget from a monolithic architecture to a composite pattern using child widgets. This enables proper per-component styling through CSS inheritance and the visitor pattern, resolving the current limitation where all scrollbar components (track, thumb, arrows) render with the same color.

## Current Limitation

**Problem**: All scrollbar components blend together (same color) because `resolve_style()` is called once for the entire scrollbar widget.

**Why**: The visitor pattern (`render_context`) uses `ctx.style()` which comes from the widget's `resolve_style()`. There's no way to pass different colors for different parts of the same widget in `do_render()` without breaking the visitor pattern.

**Current workaround**: `resolve_style()` in `scrollbar.hh:150-166` returns thumb colors, making the entire scrollbar light gray. Track and arrows are drawn with explicit `box_style` parameters, but they still use the widget's resolved foreground/background colors.

## Proposed Solution: Composite Pattern

Transform scrollbar from single widget to composite widget with child components:

```
scrollbar (widget_container)
├── scrollbar_thumb (widget)        - Overrides resolve_style() for thumb colors
├── scrollbar_arrow (widget) ×2     - Overrides resolve_style() for arrow colors
└── track rendering (parent itself) - Draws background in do_render()
```

### Benefits

1. **Proper CSS Inheritance**: Each component resolves its own style independently
2. **Visitor Pattern Compliant**: Both `measure_context` and `draw_context` work correctly
3. **Clean Separation**: Track, thumb, arrows are independent widgets with clear responsibilities
4. **State Management**: Hover/pressed states work naturally through child widget events
5. **No Hacks**: No need to call renderer directly or create custom contexts
6. **Extensible**: Easy to add new components (e.g., page up/down regions, custom indicators)

## Implementation Phases

### Phase 1: Create Child Widget Classes

#### 1.1 scrollbar_thumb Widget

**File**: `include/onyxui/widgets/containers/scroll/scrollbar_thumb.hh` (NEW)

```cpp
/**
 * @file scrollbar_thumb.hh
 * @brief Draggable thumb widget for scrollbar
 * @author OnyxUI Framework
 * @date 2025-11-06
 */

#pragma once

#include <onyxui/widgets/core/widget.hh>
#include <onyxui/theming/theme.hh>

namespace onyxui {

    /**
     * @enum thumb_state
     * @brief Visual states for scrollbar thumb
     */
    enum class thumb_state {
        normal,    ///< Default appearance
        hover,     ///< Mouse hovering over thumb
        pressed,   ///< Thumb being dragged
        disabled   ///< Parent scrollbar disabled
    };

    /**
     * @class scrollbar_thumb
     * @brief Draggable thumb indicator widget
     *
     * @details
     * Represents the draggable portion of a scrollbar that indicates:
     * - Current scroll position (thumb location)
     * - Viewport size relative to content (thumb size)
     *
     * Styling:
     * - Automatically resolves colors from theme->scrollbar.thumb_{state}
     * - Supports hover, pressed, disabled states
     * - Renders as simple solid fill (no border for 1-char text UI)
     *
     * Parent Interaction:
     * - Parent scrollbar positions thumb via arrange()
     * - Parent handles drag logic and emits scroll_requested
     * - Thumb only handles visual state (hover/pressed)
     *
     * @tparam Backend UIBackend implementation
     */
    template<UIBackend Backend>
    class scrollbar_thumb : public widget<Backend> {
    public:
        using base = widget<Backend>;
        using rect_type = typename Backend::rect_type;
        using theme_type = ui_theme<Backend>;

        /**
         * @brief Construct thumb widget
         */
        scrollbar_thumb() = default;

        /**
         * @brief Set thumb visual state
         * @param state New state (normal/hover/pressed/disabled)
         */
        void set_state(thumb_state state) {
            if (m_state == state) {
                return;
            }

            m_state = state;
            this->mark_dirty();  // Visual state changed
        }

        /**
         * @brief Get current thumb state
         * @return Current visual state
         */
        [[nodiscard]] thumb_state get_state() const noexcept {
            return m_state;
        }

        /**
         * @brief Override style resolution to use thumb colors from theme
         * @param theme Global theme pointer
         * @param parent_style Parent's resolved style
         * @return Resolved style with thumb colors
         */
        [[nodiscard]] resolved_style<Backend> resolve_style(
            const theme_type* theme,
            const resolved_style<Backend>& parent_style
        ) const override {
            auto style = base::resolve_style(theme, parent_style);

            if (theme) {
                auto const& thumb_style = get_current_thumb_style(*theme);
                style.foreground_color = thumb_style.foreground;
                style.background_color = thumb_style.background;
            }

            return style;
        }

    protected:
        /**
         * @brief Render thumb as simple solid fill
         * @param ctx Render context (measurement or drawing)
         */
        void do_render(render_context<Backend>& ctx) const override {
            // Simple solid fill - no border for 1-char thumb in text UI
            // ctx.style() contains our resolved foreground/background colors
            auto const bounds = this->bounds();

            // For text UI, we just fill with background color
            // For GUI, this could draw a more complex thumb with gradients/shadows
            ctx.fill_rect(bounds);
        }

    private:
        thumb_state m_state = thumb_state::normal;

        /**
         * @brief Get theme style for current state
         * @param theme Theme containing style definitions
         * @return Component style for current state
         */
        [[nodiscard]] auto const& get_current_thumb_style(const theme_type& theme) const {
            switch (m_state) {
                case thumb_state::disabled:
                    return theme.scrollbar.thumb_disabled;
                case thumb_state::pressed:
                    return theme.scrollbar.thumb_pressed;
                case thumb_state::hover:
                    return theme.scrollbar.thumb_hover;
                default:
                    return theme.scrollbar.thumb_normal;
            }
        }
    };

} // namespace onyxui
```

#### 1.2 scrollbar_arrow Widget

**File**: `include/onyxui/widgets/containers/scroll/scrollbar_arrow.hh` (NEW)

```cpp
/**
 * @file scrollbar_arrow.hh
 * @brief Arrow button widget for scrollbar
 * @author OnyxUI Framework
 * @date 2025-11-06
 */

#pragma once

#include <onyxui/widgets/core/widget.hh>
#include <onyxui/theming/theme.hh>
#include <onyxui/core/orientation.hh>

namespace onyxui {

    /**
     * @enum arrow_state
     * @brief Visual states for scrollbar arrow
     */
    enum class arrow_state {
        normal,    ///< Default appearance
        hover,     ///< Mouse hovering over arrow
        pressed,   ///< Arrow being clicked
        disabled   ///< Parent scrollbar disabled
    };

    /**
     * @enum arrow_direction
     * @brief Direction arrow points
     */
    enum class arrow_direction {
        up,     ///< Upward pointing (vertical scroll decrement)
        down,   ///< Downward pointing (vertical scroll increment)
        left,   ///< Leftward pointing (horizontal scroll decrement)
        right   ///< Rightward pointing (horizontal scroll increment)
    };

    /**
     * @class scrollbar_arrow
     * @brief Clickable arrow button for line scrolling
     *
     * @details
     * Represents increment/decrement arrow buttons on scrollbars.
     *
     * Styling:
     * - Automatically resolves colors from theme->scrollbar.arrow_{state}
     * - Supports hover, pressed, disabled states
     * - Renders background fill + centered arrow icon
     *
     * Parent Interaction:
     * - Parent scrollbar positions arrow via arrange()
     * - Parent handles click events and emits scroll_requested
     * - Arrow only handles visual state (hover/pressed)
     *
     * Icon Selection:
     * - Direction determines which theme icon to use
     * - theme->scrollbar.arrow_decrement_icon (up/left)
     * - theme->scrollbar.arrow_increment_icon (down/right)
     *
     * @tparam Backend UIBackend implementation
     */
    template<UIBackend Backend>
    class scrollbar_arrow : public widget<Backend> {
    public:
        using base = widget<Backend>;
        using rect_type = typename Backend::rect_type;
        using point_type = typename Backend::point_type;
        using size_type = typename Backend::size_type;
        using theme_type = ui_theme<Backend>;
        using icon_type = typename Backend::renderer_type::icon_style;
        using renderer_type = typename Backend::renderer_type;

        /**
         * @brief Construct arrow button
         * @param dir Direction arrow points
         */
        explicit scrollbar_arrow(arrow_direction dir)
            : m_direction(dir)
        {
        }

        /**
         * @brief Set arrow visual state
         * @param state New state (normal/hover/pressed/disabled)
         */
        void set_state(arrow_state state) {
            if (m_state == state) {
                return;
            }

            m_state = state;
            this->mark_dirty();  // Visual state changed
        }

        /**
         * @brief Get current arrow state
         * @return Current visual state
         */
        [[nodiscard]] arrow_state get_state() const noexcept {
            return m_state;
        }

        /**
         * @brief Get arrow direction
         * @return Direction arrow points
         */
        [[nodiscard]] arrow_direction get_direction() const noexcept {
            return m_direction;
        }

        /**
         * @brief Override style resolution to use arrow colors from theme
         * @param theme Global theme pointer
         * @param parent_style Parent's resolved style
         * @return Resolved style with arrow colors
         */
        [[nodiscard]] resolved_style<Backend> resolve_style(
            const theme_type* theme,
            const resolved_style<Backend>& parent_style
        ) const override {
            auto style = base::resolve_style(theme, parent_style);

            if (theme) {
                auto const& arrow_style = get_current_arrow_style(*theme);
                style.foreground_color = arrow_style.foreground;
                style.background_color = arrow_style.background;
            }

            return style;
        }

    protected:
        /**
         * @brief Render arrow button (background + centered icon)
         * @param ctx Render context (measurement or drawing)
         */
        void do_render(render_context<Backend>& ctx) const override {
            auto const bounds = this->bounds();
            auto const* theme = ctx.theme();

            if (!theme) {
                return;
            }

            // 1. Background fill (uses our resolved colors)
            ctx.fill_rect(bounds);

            // 2. Draw arrow icon (centered in bounds)
            icon_type const icon = get_icon_for_direction(*theme);
            auto const icon_size = renderer_type::get_icon_size(icon);

            // Calculate centered position
            int const bounds_w = rect_utils::get_width(bounds);
            int const bounds_h = rect_utils::get_height(bounds);
            int const icon_w = size_utils::get_width(icon_size);
            int const icon_h = size_utils::get_height(icon_size);

            int const icon_x = point_utils::get_x(ctx.position()) + (bounds_w - icon_w) / 2;
            int const icon_y = point_utils::get_y(ctx.position()) + (bounds_h - icon_h) / 2;

            point_type const icon_pos{icon_x, icon_y};
            ctx.draw_icon(icon, icon_pos);
        }

    private:
        arrow_direction m_direction;
        arrow_state m_state = arrow_state::normal;

        /**
         * @brief Get theme style for current state
         * @param theme Theme containing style definitions
         * @return Component style for current state
         */
        [[nodiscard]] auto const& get_current_arrow_style(const theme_type& theme) const {
            switch (m_state) {
                case arrow_state::disabled:
                    return theme.scrollbar.arrow_disabled;
                case arrow_state::pressed:
                    return theme.scrollbar.arrow_pressed;
                case arrow_state::hover:
                    return theme.scrollbar.arrow_hover;
                default:
                    return theme.scrollbar.arrow_normal;
            }
        }

        /**
         * @brief Get icon based on arrow direction
         * @param theme Theme containing icon definitions
         * @return Icon to render
         */
        [[nodiscard]] icon_type get_icon_for_direction(const theme_type& theme) const {
            switch (m_direction) {
                case arrow_direction::up:
                case arrow_direction::left:
                    return theme.scrollbar.arrow_decrement_icon;
                case arrow_direction::down:
                case arrow_direction::right:
                    return theme.scrollbar.arrow_increment_icon;
            }

            // Unreachable, but satisfy compiler
            return theme.scrollbar.arrow_decrement_icon;
        }
    };

} // namespace onyxui
```

#### 1.3 Update CMakeLists.txt

Add new headers to the install target (these are header-only, so no source compilation needed).

### Phase 2: Refactor Scrollbar Widget

#### 2.1 Change Base Class

**File**: `include/onyxui/widgets/containers/scroll/scrollbar.hh`

**Line 60** - Change inheritance:

```cpp
// OLD:
template<UIBackend Backend>
class scrollbar : public widget<Backend> {

// NEW:
template<UIBackend Backend>
class scrollbar : public widget_container<Backend> {
```

**Add includes at top**:

```cpp
#include <onyxui/widgets/containers/scroll/scrollbar_thumb.hh>
#include <onyxui/widgets/containers/scroll/scrollbar_arrow.hh>
#include <onyxui/widgets/containers/widget_container.hh>
#include <onyxui/layout/absolute_layout.hh>
```

#### 2.2 Update Constructor

**Line 74** - Replace constructor:

```cpp
explicit scrollbar(orientation orient = orientation::vertical)
    : m_orientation(orient)
{
    // Create child widgets
    auto thumb = std::make_unique<scrollbar_thumb<Backend>>();
    m_thumb = thumb.get();  // Store raw pointer before moving

    auto arrow_dec = std::make_unique<scrollbar_arrow<Backend>>(
        orient == orientation::vertical
            ? arrow_direction::up
            : arrow_direction::left
    );
    m_arrow_dec = arrow_dec.get();

    auto arrow_inc = std::make_unique<scrollbar_arrow<Backend>>(
        orient == orientation::vertical
            ? arrow_direction::down
            : arrow_direction::right
    );
    m_arrow_inc = arrow_inc.get();

    // Add as children (transfers ownership)
    this->add_child(std::move(thumb));
    this->add_child(std::move(arrow_dec));
    this->add_child(std::move(arrow_inc));

    // Use absolute_layout since we manually position components
    this->set_layout_strategy(std::make_unique<absolute_layout<Backend>>());
}
```

#### 2.3 Update set_orientation()

**Line 105** - Update to recreate arrow widgets with new direction:

```cpp
void set_orientation(orientation orient) {
    if (m_orientation == orient) {
        return;
    }

    m_orientation = orient;

    // Recreate arrow widgets with correct direction
    // Remove old arrows
    this->remove_child(m_arrow_dec);
    this->remove_child(m_arrow_inc);

    // Create new arrows with correct direction
    auto arrow_dec = std::make_unique<scrollbar_arrow<Backend>>(
        orient == orientation::vertical
            ? arrow_direction::up
            : arrow_direction::left
    );
    m_arrow_dec = arrow_dec.get();

    auto arrow_inc = std::make_unique<scrollbar_arrow<Backend>>(
        orient == orientation::vertical
            ? arrow_direction::down
            : arrow_direction::right
    );
    m_arrow_inc = arrow_inc.get();

    this->add_child(std::move(arrow_dec));
    this->add_child(std::move(arrow_inc));

    this->invalidate_measure();  // Dimensions swap
}
```

#### 2.4 Update do_measure()

No changes needed - already returns correct size.

#### 2.5 Update do_arrange()

**Line 210** - Position child widgets:

```cpp
void do_arrange(const rect_type& final_bounds) override {
    base::do_arrange(final_bounds);

    // Get theme to determine scrollbar style
    auto const* themes = ui_services<Backend>::themes();
    auto const* theme = themes ? themes->get_current_theme() : nullptr;

    if (!theme) {
        return;
    }

    scrollbar_style const style = theme->scrollbar.style;

    // Calculate component layout (returns RELATIVE coordinates)
    auto const layout = calculate_layout(style);

    // Arrange children at calculated positions
    // Note: arrange() expects RELATIVE bounds (which calculate_layout provides)
    m_thumb->arrange(layout.thumb);

    // Only arrange arrows if they have non-zero size
    if (layout.has_arrows()) {
        m_arrow_dec->arrange(layout.arrow_decrement);
        m_arrow_inc->arrange(layout.arrow_increment);

        // Make arrows visible
        m_arrow_dec->set_visible(true);
        m_arrow_inc->set_visible(true);
    } else {
        // Hide arrows for minimal style
        m_arrow_dec->set_visible(false);
        m_arrow_inc->set_visible(false);
    }
}
```

#### 2.6 Simplify do_render()

**Line 390** - Only draw track background (children render themselves):

```cpp
void do_render(render_context<Backend>& ctx) const override {
    // Get theme to determine scrollbar style
    auto const* theme = ctx.theme();
    if (!theme) {
        return;
    }

    // CRITICAL FIX: Don't render if scrollbar is too small
    auto const bounds = this->bounds();
    int const width = rect_utils::get_width(bounds);
    int const height = rect_utils::get_height(bounds);
    int const min_size = theme->scrollbar.min_render_size;

    if (m_orientation == orientation::vertical && height < min_size) {
        return;
    }
    if (m_orientation == orientation::horizontal && width < min_size) {
        return;
    }

    scrollbar_style const style = theme->scrollbar.style;
    auto const layout = calculate_layout(style);

    // Convert track from relative to absolute coordinates
    rect_type abs_track;
    int const base_x = point_utils::get_x(ctx.position());
    int const base_y = point_utils::get_y(ctx.position());

    rect_utils::set_bounds(abs_track,
        base_x + rect_utils::get_x(layout.track),
        base_y + rect_utils::get_y(layout.track),
        rect_utils::get_width(layout.track),
        rect_utils::get_height(layout.track));

    // Only draw track background - children render themselves!
    ctx.draw_rect(abs_track, theme->scrollbar.track_normal.box_style);

    // Children are rendered automatically by widget_container
}
```

#### 2.7 Update Mouse Event Handlers

**Lines 223-384** - Update to work with child widgets:

```cpp
bool handle_click(int x, int y) override {
    // Check if click is on arrow widgets (they handle their own hit testing)
    // We just need to emit scroll_requested signal if arrow is clicked

    auto const* themes = ui_services<Backend>::themes();
    if (!themes) {
        return false;
    }

    auto const* theme = themes->get_current_theme();
    if (!theme) {
        return false;
    }

    scrollbar_style const style = theme->scrollbar.style;
    auto const layout = calculate_layout(style);

    // Convert point to relative coordinates
    auto const bounds = this->bounds();
    int const rel_x = x - rect_utils::get_x(bounds);
    int const rel_y = y - rect_utils::get_y(bounds);
    point_type const rel_point{rel_x, rel_y};

    // Check if click is in decrement arrow
    if (layout.has_arrows() && point_in_rect(rel_point, layout.arrow_decrement)) {
        int const line_increment = theme->scrollbar.line_increment;
        scroll_requested.emit(-line_increment);
        return true;
    }

    // Check if click is in increment arrow
    if (layout.has_arrows() && point_in_rect(rel_point, layout.arrow_increment)) {
        int const line_increment = theme->scrollbar.line_increment;
        scroll_requested.emit(line_increment);
        return true;
    }

    return false;
}

bool handle_mouse_move(int x, int y) override {
    // Update child widget states based on hover
    auto const* themes = ui_services<Backend>::themes();
    if (!themes) {
        return false;
    }

    auto const* theme = themes->get_current_theme();
    if (!theme) {
        return false;
    }

    scrollbar_style const style = theme->scrollbar.style;
    auto const layout = calculate_layout(style);

    // Convert to relative coordinates
    auto const bounds = this->bounds();
    int const rel_x = x - rect_utils::get_x(bounds);
    int const rel_y = y - rect_utils::get_y(bounds);
    point_type const rel_point{rel_x, rel_y};

    // Update thumb hover state
    if (point_in_rect(rel_point, layout.thumb)) {
        m_thumb->set_state(thumb_state::hover);
    } else if (m_thumb->get_state() == thumb_state::hover) {
        m_thumb->set_state(thumb_state::normal);
    }

    // Update arrow hover states
    if (layout.has_arrows()) {
        bool const dec_hover = point_in_rect(rel_point, layout.arrow_decrement);
        bool const inc_hover = point_in_rect(rel_point, layout.arrow_increment);

        m_arrow_dec->set_state(dec_hover ? arrow_state::hover : arrow_state::normal);
        m_arrow_inc->set_state(inc_hover ? arrow_state::hover : arrow_state::normal);
    }

    return base::handle_mouse_move(x, y);
}

bool handle_mouse_down(int x, int y, int button) override {
    // Update child widget states based on press
    auto const* themes = ui_services<Backend>::themes();
    if (!themes) {
        return false;
    }

    auto const* theme = themes->get_current_theme();
    if (!theme) {
        return false;
    }

    scrollbar_style const style = theme->scrollbar.style;
    auto const layout = calculate_layout(style);

    // Convert to relative coordinates
    auto const bounds = this->bounds();
    int const rel_x = x - rect_utils::get_x(bounds);
    int const rel_y = y - rect_utils::get_y(bounds);
    point_type const rel_point{rel_x, rel_y};

    // Update thumb pressed state
    if (point_in_rect(rel_point, layout.thumb)) {
        m_thumb->set_state(thumb_state::pressed);
    }

    // Update arrow pressed states
    if (layout.has_arrows()) {
        if (point_in_rect(rel_point, layout.arrow_decrement)) {
            m_arrow_dec->set_state(arrow_state::pressed);
        }
        if (point_in_rect(rel_point, layout.arrow_increment)) {
            m_arrow_inc->set_state(arrow_state::pressed);
        }
    }

    return base::handle_mouse_down(x, y, button);
}

bool handle_mouse_up(int x, int y, int button) override {
    // Clear pressed states on all children
    if (m_thumb->get_state() == thumb_state::pressed) {
        m_thumb->set_state(thumb_state::normal);
    }

    if (m_arrow_dec->get_state() == arrow_state::pressed) {
        m_arrow_dec->set_state(arrow_state::normal);
    }

    if (m_arrow_inc->get_state() == arrow_state::pressed) {
        m_arrow_inc->set_state(arrow_state::normal);
    }

    return base::handle_mouse_up(x, y, button);
}

bool handle_mouse_leave() override {
    // Clear hover states on all children
    if (m_thumb->get_state() == thumb_state::hover) {
        m_thumb->set_state(thumb_state::normal);
    }

    if (m_arrow_dec->get_state() == arrow_state::hover) {
        m_arrow_dec->set_state(arrow_state::normal);
    }

    if (m_arrow_inc->get_state() == arrow_state::hover) {
        m_arrow_inc->set_state(arrow_state::normal);
    }

    return base::handle_mouse_leave();
}
```

#### 2.8 Remove Obsolete Code

**Remove** (lines 500-533):
- `get_thumb_style()` - No longer needed, thumb widget handles its own styling
- `get_arrow_style()` - No longer needed, arrow widgets handle their own styling

**Remove** (lines 807-811):
- `m_thumb_hovered` - State now tracked by child widget
- `m_thumb_pressed` - State now tracked by child widget
- `m_arrow_hovered` - State now tracked by child widgets
- `m_arrow_pressed` - State now tracked by child widgets

#### 2.9 Add Private Members

**Line 802** - Add raw pointers to children:

```cpp
private:
    orientation m_orientation = orientation::vertical;
    scroll_info<Backend> m_scroll_info{};
    rect_type m_thumb_bounds{};  // Keep for legacy API compatibility

    // Raw pointers to child widgets (ownership held by widget_container)
    scrollbar_thumb<Backend>* m_thumb = nullptr;
    scrollbar_arrow<Backend>* m_arrow_dec = nullptr;
    scrollbar_arrow<Backend>* m_arrow_inc = nullptr;
```

#### 2.10 Remove resolve_style() Override

**Delete lines 150-166** - No longer needed! Each child resolves its own style.

The parent scrollbar will inherit normal CSS colors from its parent, which is correct for the track background.

### Phase 3: Update Tests

#### 3.1 Test Files to Review

**Likely unchanged** (test public API only):
- `unittest/widgets/test_scrollbar_arrows.cc` - Tests arrow click behavior via scroll_requested signal
- `unittest/widgets/test_scrollbar_interaction.cc` - Tests public scrollbar API

**May need minor updates** (test internal structure):
- `unittest/widgets/test_scrollbar.cc` - May access internal thumb bounds via `get_thumb_bounds()`

#### 3.2 Expected Test Results

All 1215 tests should still pass after refactoring:
- Scrollbar behavior unchanged (same signals, same API)
- Visual appearance improved (distinct component colors)
- Internal structure changed (composite pattern) but public API unchanged

#### 3.3 Test Strategy

1. Run existing tests to verify no regressions: `./build/bin/ui_unittest --test-case="*scrollbar*"`
2. Verify test failures are only due to internal changes (not behavior changes)
3. Update minimal test code to access child widgets if needed
4. Ensure all test cases pass

### Phase 4: Verify and Document

#### 4.1 Build and Test

```bash
# Clean build
rm -rf build
cmake -B build
cmake --build build -j8

# Run all tests
./build/bin/ui_unittest

# Expected: 1215/1215 passing
```

#### 4.2 Visual Verification

```bash
# Run demo application
./build/bin/widgets_demo

# Verify scrollbars:
# 1. Track is visible (dark gray background)
# 2. Thumb is visible and DISTINCT color (light gray)
# 3. Arrows are visible and DISTINCT color (light gray)
# 4. Hover states work (colors change on hover)
# 5. Pressed states work (colors change on click)
```

#### 4.3 Update Documentation

**File**: `docs/scrolling_guide.md`

Add section documenting the child widget architecture:

```markdown
## Architecture: Composite Pattern

As of November 2025, scrollbar uses a composite widget architecture:

### Component Structure

```
scrollbar (widget_container)
├── scrollbar_thumb - Draggable position indicator
├── scrollbar_arrow (decrement) - Up/left arrow button
└── scrollbar_arrow (increment) - Down/right arrow button
```

### Benefits

1. **Proper CSS Inheritance**: Each component resolves colors independently
2. **State Management**: Hover/pressed states handled by child widgets
3. **Visitor Pattern**: Measurement and rendering work correctly
4. **Extensibility**: Easy to add new visual components

### Custom Styling

Each component can be themed independently via `ui_theme.scrollbar`:

```cpp
theme.scrollbar.track_normal = {fg, bg, box_style};  // Track appearance
theme.scrollbar.thumb_normal = {fg, bg, box_style};  // Thumb appearance
theme.scrollbar.arrow_normal = {fg, bg, box_style};  // Arrow appearance
```

All three support `{normal, hover, pressed, disabled}` states.
```

**File**: `docs/CLAUDE/ARCHITECTURE.md`

Add scrollbar as an example of the composite pattern in the event system section.

**File**: `docs/CLAUDE/CHANGELOG.md`

Add entry:

```markdown
### 2025-11-06 - Scrollbar Child Widget Architecture

**Breaking Change**: Internal only (no API changes)

Refactored scrollbar from monolithic widget to composite pattern:
- Track, thumb, arrows are now separate widgets
- Each component resolves its own CSS style
- Enables proper per-component theming
- All 1215 tests still pass

**Files Changed**:
- `include/onyxui/widgets/containers/scroll/scrollbar.hh` - Now inherits from widget_container
- `include/onyxui/widgets/containers/scroll/scrollbar_thumb.hh` - NEW
- `include/onyxui/widgets/containers/scroll/scrollbar_arrow.hh` - NEW

**Migration**: None required - public API unchanged
```

## Open Questions

### Q1: Should widget_container handle child rendering automatically?

**Answer**: YES - `widget_container::do_render()` already iterates children and calls `child->render()`. No changes needed.

### Q2: How do child widgets participate in event routing?

**Answer**: Through normal three-phase capture/target/bubble. Parent scrollbar will receive events first (capture phase), can update child states, then children receive events if applicable.

### Q3: Should thumb dragging be handled by thumb or parent?

**Recommendation**: **Parent** scrollbar handles dragging logic:
- Parent tracks drag start position
- Parent calculates scroll offset from thumb position
- Parent emits `scroll_requested` signal
- Thumb only tracks visual state (normal/hover/pressed)

**Rationale**:
- Dragging requires scrollbar layout knowledge (track length, scroll range)
- Thumb is just a visual indicator, shouldn't know scroll semantics
- Keeps drag logic centralized in one place

### Q4: Can we generalize this pattern?

**Candidates for composite pattern**:
- `button` with icon - Separate icon widget for independent styling
- `progress_bar` - Separate fill widget for animation/theming
- `slider` - Separate thumb/track widgets like scrollbar
- `checkbox` - Separate checkmark widget for state indication

**Recommendation**: Wait for concrete need. Don't over-engineer.

## Risk Assessment

### Low Risk
- ✅ Public API unchanged - no breaking changes for applications
- ✅ Test coverage excellent (1215 tests)
- ✅ Child widget pattern already proven (menu_bar, scroll_view)
- ✅ Visitor pattern supports composite widgets naturally

### Medium Risk
- ⚠️ Event routing complexity - need to test hover/pressed states thoroughly
- ⚠️ Performance - more widgets in tree (negligible for scrollbars, but watch for cascading)

### Mitigation
- Test all mouse interactions (hover, press, drag, leave)
- Profile rendering performance before/after (expect no measurable difference)
- Add visual test cases for each component state

## Success Criteria

1. ✅ All 1215 unit tests pass
2. ✅ Visual demo shows distinct colors for track/thumb/arrows
3. ✅ Hover states work (colors change on mouse over)
4. ✅ Pressed states work (colors change on click)
5. ✅ No performance regression (same frame rate in demo)
6. ✅ Documentation updated (architecture guide, scrolling guide, changelog)

## References

- Session file: `SESSION_2025_11_06_SCROLLBAR_REFACTORING.md`
- Current implementation: `include/onyxui/widgets/containers/scroll/scrollbar.hh`
- Architecture guide: `docs/CLAUDE/ARCHITECTURE.md`
- Scrolling guide: `docs/scrolling_guide.md`
- Composite pattern examples: `menu_bar.hh`, `scroll_view.hh`

---

**Ready to implement?** Start with Phase 1 (create child widget classes).
