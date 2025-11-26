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
     * vertical slider) based on the number of units. This is a backend-agnostic
     * way to size sliders that works correctly across different backends:
     *
     * - **TUI backend (conio)**: 1 unit = 1 character cell
     * - **GUI backend (SDL2)**: 1 unit = 1 pixel
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
     * @note For TUI backends, this currently maps 1:1 to character cells.
     * @note For future GUI backends, this represents pixel width/height.
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

        size_constraint constraint;
        constraint.policy = size_policy::content;
        constraint.preferred_size = length;

        // Set the appropriate dimension based on orientation
        if (m_orientation == slider_orientation::horizontal) {
            this->set_width_constraint(constraint);
        } else {
            this->set_height_constraint(constraint);
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
     */
    [[nodiscard]] size_type get_content_size() const override {
        // Slider has a default size based on orientation
        constexpr int DEFAULT_HORIZONTAL_WIDTH = 100;
        constexpr int DEFAULT_HORIZONTAL_HEIGHT = 3;  // Track + thumb
        constexpr int DEFAULT_VERTICAL_WIDTH = 3;
        constexpr int DEFAULT_VERTICAL_HEIGHT = 20;

        if (m_orientation == slider_orientation::horizontal) {
            return size_type{DEFAULT_HORIZONTAL_WIDTH, DEFAULT_HORIZONTAL_HEIGHT};
        } else {
            return size_type{DEFAULT_VERTICAL_WIDTH, DEFAULT_VERTICAL_HEIGHT};
        }
    }

    /**
     * @brief Handle keyboard and mouse events
     */
    bool handle_event(const ui_event& evt, event_phase phase) override {
        if (phase != event_phase::target) {
            return base::handle_event(evt, phase);
        }

        // Handle mouse click events
        if (auto* mouse = std::get_if<mouse_event>(&evt)) {
            if (mouse->act == mouse_event::action::press) {
                // Get absolute mouse position
                int const mouse_x = mouse->x;
                int const mouse_y = mouse->y;

                // Get widget absolute position and size
                auto const abs_bounds = this->get_absolute_bounds();
                int const widget_x = rect_utils::get_x(abs_bounds);
                int const widget_y = rect_utils::get_y(abs_bounds);
                int const widget_w = rect_utils::get_width(abs_bounds);
                int const widget_h = rect_utils::get_height(abs_bounds);

                // Calculate relative click position
                int const rel_x = mouse_x - widget_x;
                int const rel_y = mouse_y - widget_y;

                // Check if click is within widget bounds
                if (rel_x >= 0 && rel_x < widget_w && rel_y >= 0 && rel_y < widget_h) {
                    // Calculate new value based on click position
                    int new_value;
                    if (m_orientation == slider_orientation::horizontal) {
                        // Horizontal: map X position to value range
                        new_value = range_helpers::position_to_value(rel_x, widget_w, m_min, m_max);
                    } else {
                        // Vertical: map Y position to value range (bottom = min, top = max)
                        int const from_bottom = widget_h - rel_y;
                        new_value = range_helpers::position_to_value(from_bottom, widget_h, m_min, m_max);
                    }

                    // Emit pressed signal and update value
                    slider_pressed.emit();
                    set_value(new_value);
                    slider_moved.emit(m_value);

                    // Request focus on click
                    auto* input = ui_services<Backend>::input();
                    if (input && this->is_focusable()) {
                        input->set_focus(this);
                    }

                    return true;
                }
            } else if (mouse->act == mouse_event::action::release) {
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

        // Get absolute position and size
        const auto& pos = ctx.position();
        const auto& bounds = this->bounds();

        int const x = point_utils::get_x(pos);
        int const y = point_utils::get_y(pos);
        int const width = rect_utils::get_width(bounds);
        int const height = rect_utils::get_height(bounds);

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

    // ===== Helper Methods =====

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

        // Get icon characters from theme (use icon enums to get customizable characters)
        auto const& filled_icon = theme->slider.filled_icon;
        auto const& empty_icon = theme->slider.empty_icon;
        auto const& thumb_icon = theme->slider.thumb_icon;

        // Convert icons to character strings
        std::string const filled_char = icon_to_char(filled_icon);
        std::string const empty_char = icon_to_char(empty_icon);
        std::string const thumb_char = icon_to_char(thumb_icon);

        // Render track (filled + empty portions) using theme icons
        int const track_y = y + height / 2;  // Center track vertically
        typename Backend::renderer_type::font track_font{};

        // For filled portion
        if (fill_length > 0) {
            std::string filled_str(static_cast<std::size_t>(fill_length), filled_char[0]);
            point_type const filled_pos{x, track_y};
            ctx.draw_text(filled_str, filled_pos, track_font, filled_color);
        }

        // For empty portion
        if (fill_length < width) {
            std::string empty_str(static_cast<std::size_t>(width - fill_length), empty_char[0]);
            point_type const empty_pos{x + fill_length, track_y};
            ctx.draw_text(empty_str, empty_pos, track_font, empty_color);
        }

        // Render thumb (bright marker at position)
        if (thumb_pos >= 0 && thumb_pos < width) {
            point_type const thumb_point{x + thumb_pos, track_y};
            ctx.draw_text(thumb_char, thumb_point, track_font, thumb_color);
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

        // Get icons from theme
        auto* theme = ctx.theme();
        if (!theme) {
            return;
        }

        // Get icon characters from theme
        auto const& filled_icon = theme->slider.filled_icon;
        auto const& empty_icon = theme->slider.empty_icon;
        auto const& thumb_icon = theme->slider.thumb_icon;

        // Convert icons to character strings
        std::string const filled_char = icon_to_char(filled_icon);
        std::string const empty_char = icon_to_char(empty_icon);
        std::string const thumb_char = icon_to_char(thumb_icon);

        // Render track (filled from bottom, empty from top) using theme icons
        int const track_x = x + width / 2;  // Center track horizontally
        typename Backend::renderer_type::font track_font{};

        for (int i = 0; i < height; ++i) {
            bool const is_filled = i >= (height - fill_length);
            std::string const& track_char = is_filled ? filled_char : empty_char;
            color_type const& color = is_filled ? filled_color : empty_color;

            point_type const pos{track_x, y + i};
            ctx.draw_text(track_char, pos, track_font, color);
        }

        // Render thumb (bright marker)
        int const thumb_y = y + (height - thumb_pos);
        if (thumb_y >= y && thumb_y < y + height) {
            point_type const thumb_point{track_x, thumb_y};
            ctx.draw_text(thumb_char, thumb_point, track_font, thumb_color);
        }
    }
};

}  // namespace onyxui
