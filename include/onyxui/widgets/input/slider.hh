// Copyright (c) 2025 OnyxUI Framework
// SPDX-License-Identifier: MIT

#pragma once

#include <onyxui/widgets/core/widget.hh>
#include <onyxui/widgets/utils/range_helpers.hh>
#include <onyxui/core/signal.hh>
#include <onyxui/events/ui_event.hh>
#include <onyxui/core/rendering/draw_context.hh>
#include <onyxui/services/ui_services.hh>

namespace onyxui {

/// Slider orientation
enum class slider_orientation : std::uint8_t {
    horizontal,
    vertical
};

/// Tick mark position
enum class tick_position : std::uint8_t {
    none,
    above,       // Above track (horizontal) or left (vertical)
    below,       // Below track (horizontal) or right (vertical)
    both_sides
};

/**
 * @class slider
 * @brief Interactive numeric value input widget
 *
 * @tparam Backend The UI backend type
 *
 * @details
 * Provides interactive value selection within a range using keyboard navigation.
 * Mouse support is limited (click-to-position only, no drag in conio backend).
 *
 * ## Key Features
 *
 * - Keyboard navigation (arrows, page up/down, home/end)
 * - Step-based value snapping
 * - Optional tick marks
 * - Horizontal/vertical orientation
 * - Visual feedback with filled track and thumb
 *
 * ## Keyboard Controls
 *
 * - **Arrow Keys**: Increment/decrement by single_step
 * - **Page Up/Down**: Increment/decrement by page_step
 * - **Home**: Jump to minimum
 * - **End**: Jump to maximum
 *
 * @example
 * @code
 * // Create horizontal slider (0-100, default value 50)
 * auto volume = std::make_unique<slider<Backend>>();
 * volume->set_range(0, 100);
 * volume->set_value(50);
 * volume->set_single_step(1);
 * volume->set_page_step(10);
 *
 * // Connect to value changes
 * volume->value_changed.connect([](int val) {
 *     std::cout << "Volume: " << val << "%\n";
 * });
 * @endcode
 */
template<UIBackend Backend>
class slider : public widget<Backend> {
public:
    using base = widget<Backend>;
    using renderer_type = typename Backend::renderer_type;
    using size_type = typename Backend::size_type;
    using color_type = typename Backend::color_type;
    using rect_type = typename Backend::rect_type;
    using point_type = typename Backend::point_type;

    // ===== Construction =====

    /**
     * @brief Construct a horizontal slider
     * @param parent Parent element (nullptr for none)
     */
    explicit slider(ui_element<Backend>* parent = nullptr)
        : base(parent) {
        this->set_focusable(true);  // Sliders are focusable for keyboard input
    }

    /**
     * @brief Construct a slider with specific orientation
     * @param orientation Horizontal or vertical
     * @param parent Parent element (nullptr for none)
     */
    explicit slider(slider_orientation orientation, ui_element<Backend>* parent = nullptr)
        : base(parent), m_orientation(orientation) {
        this->set_focusable(true);
    }

    /**
     * @brief Destructor
     */
    ~slider() override = default;

    // Rule of Five
    slider(const slider&) = delete;
    slider& operator=(const slider&) = delete;
    slider(slider&&) noexcept = default;
    slider& operator=(slider&&) noexcept = default;

    // ===== Value Management =====

    /**
     * @brief Set current value
     * @param value New value (will be clamped to [min, max] and snapped to step)
     */
    void set_value(int value) {
        // Clamp to range
        int clamped = std::clamp(value, m_min, m_max);

        // Snap to step
        if (m_single_step > 0) {
            clamped = range_helpers::snap_to_step(clamped, m_single_step);
            clamped = std::clamp(clamped, m_min, m_max);  // Re-clamp after snapping
        }

        if (m_value != clamped) {
            m_value = clamped;
            this->mark_dirty();
            value_changed.emit(m_value);
        }
    }

    /**
     * @brief Get current value
     */
    [[nodiscard]] int value() const noexcept { return m_value; }

    /**
     * @brief Set value range
     * @param min Minimum value
     * @param max Maximum value
     */
    void set_range(int min, int max) {
        if (m_min != min || m_max != max) {
            m_min = min;
            m_max = max;
            // Clamp current value to new range
            set_value(m_value);
            this->mark_dirty();
        }
    }

    /**
     * @brief Get minimum value
     */
    [[nodiscard]] int minimum() const noexcept { return m_min; }

    /**
     * @brief Get maximum value
     */
    [[nodiscard]] int maximum() const noexcept { return m_max; }

    // ===== Step Management =====

    /**
     * @brief Set single step (arrow key increment)
     * @param step Step size (0 = no snapping)
     */
    void set_single_step(int step) {
        if (m_single_step != step) {
            m_single_step = step;
            // Re-snap current value
            set_value(m_value);
        }
    }

    /**
     * @brief Get single step
     */
    [[nodiscard]] int single_step() const noexcept { return m_single_step; }

    /**
     * @brief Set page step (Page Up/Down increment)
     * @param step Step size
     */
    void set_page_step(int step) {
        m_page_step = step;
    }

    /**
     * @brief Get page step
     */
    [[nodiscard]] int page_step() const noexcept { return m_page_step; }

    // ===== Orientation =====

    /**
     * @brief Set orientation
     * @param orientation Horizontal or vertical
     */
    void set_orientation(slider_orientation orientation) {
        if (m_orientation != orientation) {
            m_orientation = orientation;
            this->invalidate_measure();
        }
    }

    /**
     * @brief Get orientation
     */
    [[nodiscard]] slider_orientation orientation() const noexcept {
        return m_orientation;
    }

    // ===== Tick Marks =====

    /**
     * @brief Set tick mark position
     * @param position Where to draw tick marks
     */
    void set_tick_position(tick_position position) {
        if (m_tick_position != position) {
            m_tick_position = position;
            this->mark_dirty();
        }
    }

    /**
     * @brief Get tick position
     */
    [[nodiscard]] tick_position get_tick_position() const noexcept {
        return m_tick_position;
    }

    /**
     * @brief Set tick interval
     * @param interval Space between ticks (0 = no ticks)
     */
    void set_tick_interval(int interval) {
        if (m_tick_interval != interval) {
            m_tick_interval = interval;
            this->mark_dirty();
        }
    }

    /**
     * @brief Get tick interval
     */
    [[nodiscard]] int tick_interval() const noexcept {
        return m_tick_interval;
    }

    // ===== Semantic Sizing (Backend-Agnostic) =====

    /**
     * @brief Set preferred track length
     * @param length Track length in backend-agnostic units
     *
     * @details
     * Sets the preferred track length (width for horizontal slider, height for
     * vertical slider) in logical units. This is a backend-agnostic way to size
     * sliders that works correctly across different backends.
     *
     * The widget will use this as a preferred size hint during layout, but may
     * be stretched or shrunk based on parent layout constraints.
     *
     * The track length affects the primary dimension:
     * - **Horizontal slider**: Sets preferred width
     * - **Vertical slider**: Sets preferred height
     *
     * @note The cross-dimension (height for horizontal, width for vertical) is
     *       determined automatically based on tick marks and thumb size.
     *
     * @example
     * @code
     * // Create horizontal slider with 50-unit track
     * auto volume = std::make_unique<slider<Backend>>();
     * volume->set_orientation(slider_orientation::horizontal);
     * volume->set_track_length(50);
     *
     * // Create vertical slider with 20-unit track
     * auto brightness = std::make_unique<slider<Backend>>(slider_orientation::vertical);
     * brightness->set_track_length(20);
     * @endcode
     */
    void set_track_length(int length) {
        if (length < 1) length = 1;

        size_constraint length_constraint;
        length_constraint.policy = size_policy::content;
        length_constraint.preferred_size = logical_unit(static_cast<double>(length));

        // Set the appropriate dimension based on orientation
        if (m_orientation == slider_orientation::horizontal) {
            this->set_width_constraint(length_constraint);
        } else {
            this->set_height_constraint(length_constraint);
        }
    }

    /**
     * @brief Set slider thickness (cross-dimension)
     * @param thickness Thickness in logical units
     *
     * @details
     * Sets the cross-dimension of the slider:
     * - **Horizontal slider**: Sets height
     * - **Vertical slider**: Sets width
     *
     * For pixel-based backends (SDL), typical values are 16-24.
     * For character-based backends (conio), typical value is 1.
     */
    void set_thickness(int thickness) {
        if (thickness < 1) thickness = 1;

        size_constraint constraint;
        constraint.policy = size_policy::content;
        constraint.preferred_size = logical_unit(static_cast<double>(thickness));

        // Set cross-dimension based on orientation
        if (m_orientation == slider_orientation::horizontal) {
            this->set_height_constraint(constraint);
        } else {
            this->set_width_constraint(constraint);
        }
    }

    // ===== Signals =====

    /// Emitted when value changes
    signal<int> value_changed;

    /// Emitted when slider is moved (for tracking mode)
    signal<int> slider_moved;

    /// Emitted when slider interaction starts
    signal<> slider_pressed;

    /// Emitted when slider interaction ends
    signal<> slider_released;

protected:
    /**
     * @brief Calculate natural content size
     *
     * @details Uses theme's track_length and track_thickness for dimensions.
     */
    [[nodiscard]] logical_size get_content_size() const override {
        auto* theme = ui_services<Backend>::themes()->get_current_theme();
        double const track_length = theme->slider.track_length;
        double const track_thickness = theme->slider.track_thickness;

        if (m_orientation == slider_orientation::horizontal) {
            return logical_size{logical_unit(track_length), logical_unit(track_thickness)};
        } else {
            return logical_size{logical_unit(track_thickness), logical_unit(track_length)};
        }
    }

    /**
     * @brief Handle keyboard and mouse events
     */
    bool handle_event(const ui_event& evt, event_phase phase) override {
        if (phase != event_phase::target) {
            return base::handle_event(evt, phase);
        }

        // Handle mouse events (click, drag, release)
        if (auto* mouse = std::get_if<mouse_event>(&evt)) {
            if (mouse->act == mouse_event::action::press) {
                // Get absolute mouse position (logical units, full precision)
                double const mouse_x = mouse->x.value;
                double const mouse_y = mouse->y.value;

                // Get widget absolute position and size (logical units, full precision)
                auto const abs_bounds = this->get_absolute_logical_bounds();
                double const widget_x = abs_bounds.x.value;
                double const widget_y = abs_bounds.y.value;
                double const widget_w = abs_bounds.width.value;
                double const widget_h = abs_bounds.height.value;

                // Calculate relative click position
                double const rel_x = mouse_x - widget_x;
                double const rel_y = mouse_y - widget_y;

                // Check if click is within widget bounds
                if (rel_x >= 0.0 && rel_x < widget_w && rel_y >= 0.0 && rel_y < widget_h) {
                    // Start dragging
                    m_dragging = true;

                    // Capture mouse for drag tracking
                    auto* input = ui_services<Backend>::input();
                    if (input) {
                        input->set_capture(this);
                        if (this->is_focusable()) {
                            input->set_focus(this);
                        }
                    }

                    // Calculate and set new value
                    int const new_value = calculate_value_from_position(rel_x, rel_y, widget_w, widget_h);

                    // Emit pressed signal and update value
                    slider_pressed.emit();
                    set_value(new_value);
                    slider_moved.emit(m_value);

                    return true;
                }
            } else if (mouse->act == mouse_event::action::move && m_dragging) {
                // Handle drag - update value based on mouse position
                double const mouse_x = mouse->x.value;
                double const mouse_y = mouse->y.value;

                auto const abs_bounds = this->get_absolute_logical_bounds();
                double const widget_x = abs_bounds.x.value;
                double const widget_y = abs_bounds.y.value;
                double const widget_w = abs_bounds.width.value;
                double const widget_h = abs_bounds.height.value;

                // Calculate relative position (can be outside widget during drag)
                double const rel_x = mouse_x - widget_x;
                double const rel_y = mouse_y - widget_y;

                // Calculate and set new value (clamping happens in set_value)
                int const new_value = calculate_value_from_position(rel_x, rel_y, widget_w, widget_h);
                set_value(new_value);
                slider_moved.emit(m_value);

                return true;
            } else if (mouse->act == mouse_event::action::release) {
                if (m_dragging) {
                    m_dragging = false;

                    // Release mouse capture
                    auto* input = ui_services<Backend>::input();
                    if (input) {
                        input->release_capture();
                    }
                }
                slider_released.emit();
                return true;
            }
        }

        // Handle keyboard events
        if (auto* kbd = std::get_if<keyboard_event>(&evt)) {
            if (!kbd->pressed) {
                return false;  // Only handle key press events
            }

            switch (kbd->key) {
                case key_code::arrow_left:
                case key_code::arrow_down:
                    // Decrement
                    set_value(m_value - m_single_step);
                    return true;

                case key_code::arrow_right:
                case key_code::arrow_up:
                    // Increment
                    set_value(m_value + m_single_step);
                    return true;

                case key_code::page_up:
                    // Large increment
                    set_value(m_value + m_page_step);
                    return true;

                case key_code::page_down:
                    // Large decrement
                    set_value(m_value - m_page_step);
                    return true;

                case key_code::home:
                    // Jump to minimum
                    set_value(m_min);
                    return true;

                case key_code::end:
                    // Jump to maximum
                    set_value(m_max);
                    return true;

                default:
                    break;
            }
        }

        return base::handle_event(evt, phase);
    }

    /**
     * @brief Render the slider
     */
    void do_render(render_context<Backend>& ctx) const override {
        auto* theme = ctx.theme();
        if (!theme) {
            return;
        }

        // Use slider theme style
        auto const& filled_color = theme->slider.track_filled_color;
        auto const& empty_color = theme->slider.track_empty_color;
        auto const& thumb_color = theme->slider.thumb_color;

        // Get physical position from render context
        const auto& pos = ctx.position();
        int const x = point_utils::get_x(pos);
        int const y = point_utils::get_y(pos);

        // Get dimensions using get_final_dims pattern (handles measurement vs rendering)
        auto logical_bounds = this->bounds();
        auto const [width, height] = ctx.get_final_dims(
            logical_bounds.width.to_int(), logical_bounds.height.to_int());

        if (m_orientation == slider_orientation::horizontal) {
            render_horizontal_slider(ctx, x, y, width, height, filled_color, empty_color, thumb_color);
        } else {
            render_vertical_slider(ctx, x, y, width, height, filled_color, empty_color, thumb_color);
        }
    }

private:
    // ===== State =====
    int m_value = 0;
    int m_min = 0;
    int m_max = 100;
    int m_single_step = 1;
    int m_page_step = 10;
    slider_orientation m_orientation = slider_orientation::horizontal;
    tick_position m_tick_position = tick_position::none;
    int m_tick_interval = 0;
    bool m_dragging = false;  ///< True when mouse is dragging the slider

    // ===== Helper Methods =====

    /**
     * @brief Calculate slider value from mouse position
     * @param rel_x Relative X position within widget
     * @param rel_y Relative Y position within widget
     * @param widget_w Widget width
     * @param widget_h Widget height
     * @return Calculated value (may be outside min/max range - caller should clamp)
     */
    [[nodiscard]] int calculate_value_from_position(
        double rel_x, double rel_y,
        double widget_w, double widget_h
    ) const {
        if (m_orientation == slider_orientation::horizontal) {
            // Horizontal: map X position to value range
            // Clamp position to valid range for calculation
            double const clamped_x = std::clamp(rel_x, 0.0, widget_w);
            int const pos_int = static_cast<int>(clamped_x);
            int const size_int = static_cast<int>(widget_w);
            return range_helpers::position_to_value(pos_int, size_int, m_min, m_max);
        } else {
            // Vertical: map Y position to value range (bottom = min, top = max)
            double const clamped_y = std::clamp(rel_y, 0.0, widget_h);
            int const from_bottom = static_cast<int>(widget_h - clamped_y);
            int const size_int = static_cast<int>(widget_h);
            return range_helpers::position_to_value(from_bottom, size_int, m_min, m_max);
        }
    }

    /**
     * @brief Convert icon enum to character string
     * @param icon Icon style enum
     * @return Single-character string representation
     */
    [[nodiscard]] static std::string icon_to_char(typename Backend::renderer_type::icon_style icon) {
        using icon_type = typename Backend::renderer_type::icon_style;

        switch (icon) {
            case icon_type::slider_filled:  return "=";
            case icon_type::slider_empty:   return "-";
            case icon_type::slider_thumb:   return "O";
            default:                        return " ";
        }
    }

    /**
     * @brief Render horizontal slider
     */
    void render_horizontal_slider(
        render_context<Backend>& ctx,
        int x,
        int y,
        int width,
        int height,
        color_type const& filled_color,
        color_type const& empty_color,
        color_type const& thumb_color
    ) const {
        // Calculate thumb position
        int const thumb_pos = range_helpers::value_to_position(
            m_value, m_min, m_max, width
        );

        // Calculate fill length
        int const fill_length = range_helpers::calculate_fill_length(
            m_value, m_min, m_max, width
        );

        // Get icons from theme
        auto* theme = ctx.theme();
        if (!theme) {
            return;
        }

        // Track uses full widget height for simplicity
        int const track_height = height;
        int const track_y = y;
        int const thumb_width = std::max(1, std::min(height * 2, 16));  // Thumb width

        // Draw empty portion of track (full width, behind filled)
        rect_type empty_rect;
        rect_utils::set_bounds(empty_rect, x, track_y, width, track_height);
        ctx.fill_rect(empty_rect, empty_color);

        // Draw filled portion of track (from left to thumb position)
        if (fill_length > 0) {
            rect_type filled_rect;
            rect_utils::set_bounds(filled_rect, x, track_y, fill_length, track_height);
            ctx.fill_rect(filled_rect, filled_color);
        }

        // Draw thumb as rectangle
        if (thumb_pos >= 0 && thumb_pos < width) {
            int thumb_x = x + thumb_pos - thumb_width / 2;
            // Clamp thumb to track bounds
            thumb_x = std::max(x, std::min(thumb_x, x + width - thumb_width));
            rect_type thumb_rect;
            rect_utils::set_bounds(thumb_rect, thumb_x, y, thumb_width, height);
            ctx.fill_rect(thumb_rect, thumb_color);
        }
    }

    /**
     * @brief Render vertical slider
     */
    void render_vertical_slider(
        render_context<Backend>& ctx,
        int x,
        int y,
        int width,
        int height,
        color_type const& filled_color,
        color_type const& empty_color,
        color_type const& thumb_color
    ) const {
        // Calculate thumb position (bottom to top)
        int const thumb_pos = range_helpers::value_to_position(
            m_value, m_min, m_max, height
        );

        // Calculate fill length
        int const fill_length = range_helpers::calculate_fill_length(
            m_value, m_min, m_max, height
        );

        // Track uses full widget width for simplicity
        int const track_width = width;
        int const track_x = x;
        int const thumb_height = std::max(1, std::min(width * 2, 16));  // Thumb height

        // Draw empty portion of track (full height, behind filled)
        rect_type empty_rect;
        rect_utils::set_bounds(empty_rect, track_x, y, track_width, height);
        ctx.fill_rect(empty_rect, empty_color);

        // Draw filled portion of track (from bottom up to fill_length)
        if (fill_length > 0) {
            rect_type filled_rect;
            rect_utils::set_bounds(filled_rect, track_x, y + height - fill_length, track_width, fill_length);
            ctx.fill_rect(filled_rect, filled_color);
        }

        // Draw thumb as rectangle
        int thumb_y = y + height - thumb_pos - thumb_height / 2;
        // Clamp thumb to track bounds
        thumb_y = std::max(y, std::min(thumb_y, y + height - thumb_height));
        rect_type thumb_rect;
        rect_utils::set_bounds(thumb_rect, x, thumb_y, width, thumb_height);
        ctx.fill_rect(thumb_rect, thumb_color);
    }
};

}  // namespace onyxui
