/**
 * @file tile_slider.hh
 * @brief Interactive slider widget with tile-based rendering
 */

#pragma once

#include <string>
#include <algorithm>
#include <onyxui/widgets/core/widget.hh>
#include <onyxui/tile/tile_core.hh>
#include <onyxui/tile/tile_renderer.hh>
#include <onyxui/events/ui_event.hh>

namespace onyxui::tile {

/// Slider orientation
enum class tile_slider_orientation : std::uint8_t {
    horizontal,
    vertical
};

/**
 * @class tile_slider
 * @brief Interactive slider widget using tile-based rendering
 *
 * @tparam Backend The UI backend type (typically sdlpp_tile_backend)
 *
 * @details
 * A tile_slider provides interactive value selection within a range using
 * tile-based rendering for the track and thumb.
 *
 * ## Keyboard Controls
 * - **Arrow Keys**: Increment/decrement by single_step
 * - **Page Up/Down**: Increment/decrement by page_step
 * - **Home**: Jump to minimum
 * - **End**: Jump to maximum
 *
 * @example
 * @code
 * auto volume = std::make_unique<tile_slider<sdlpp_tile_backend>>();
 * volume->set_range(0, 100);
 * volume->set_value(50);
 * volume->set_single_step(1);
 * volume->set_page_step(10);
 *
 * volume->value_changed.connect([](int val) {
 *     std::cout << "Volume: " << val << "%\n";
 * });
 * @endcode
 */
template<UIBackend Backend>
class tile_slider : public widget<Backend> {
public:
    using base = widget<Backend>;

    /**
     * @brief Construct a slider
     * @param parent Parent element (nullptr for none)
     */
    explicit tile_slider(ui_element<Backend>* parent = nullptr)
        : base(parent)
    {
        this->set_focusable(true);  // Sliders are focusable for keyboard input
    }

    /**
     * @brief Destructor
     */
    ~tile_slider() override = default;

    // Rule of Five
    tile_slider(const tile_slider&) = delete;
    tile_slider& operator=(const tile_slider&) = delete;
    tile_slider(tile_slider&&) noexcept = default;
    tile_slider& operator=(tile_slider&&) noexcept = default;

    // ===== Value Management =====

    /**
     * @brief Set current value
     * @param value New value (will be clamped and snapped)
     */
    void set_value(int value) {
        int clamped = std::clamp(value, m_min, m_max);

        // Snap to step
        if (m_single_step > 0) {
            clamped = snap_to_step(clamped, m_single_step);
            clamped = std::clamp(clamped, m_min, m_max);
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
            set_value(m_value);  // Re-clamp
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
            set_value(m_value);  // Re-snap
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
    void set_orientation(tile_slider_orientation orientation) {
        if (m_orientation != orientation) {
            m_orientation = orientation;
            this->invalidate_measure();
        }
    }

    /**
     * @brief Get orientation
     */
    [[nodiscard]] tile_slider_orientation orientation() const noexcept {
        return m_orientation;
    }

    // ===== Sizing =====

    /**
     * @brief Set preferred track length
     * @param length Track length in pixels
     */
    void set_track_length(int length) {
        if (m_track_length != length) {
            m_track_length = length;
            this->invalidate_measure();
        }
    }

    /**
     * @brief Get track length
     */
    [[nodiscard]] int track_length() const noexcept { return m_track_length; }

    /**
     * @brief Set custom slider tiles (overrides theme)
     * @param tiles Slider tile configuration
     */
    void set_tiles(const slider_tiles& tiles) {
        m_custom_tiles = tiles;
        m_use_custom_tiles = true;
    }

    /**
     * @brief Reset to using theme's slider tiles
     */
    void use_theme_tiles() {
        m_use_custom_tiles = false;
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
     * @brief Handle keyboard and mouse events
     */
    bool handle_event(const ui_event& evt, event_phase phase) override {
        if (phase != event_phase::target) {
            return base::handle_event(evt, phase);
        }

        // Handle mouse events
        if (auto* mouse = std::get_if<mouse_event>(&evt)) {
            if (mouse->act == mouse_event::action::press) {
                return handle_mouse_press(*mouse);
            } else if (mouse->act == mouse_event::action::move && m_dragging) {
                return handle_mouse_drag(*mouse);
            } else if (mouse->act == mouse_event::action::release) {
                return handle_mouse_release();
            }
        }

        // Handle keyboard events
        if (auto* kbd = std::get_if<keyboard_event>(&evt)) {
            if (!kbd->pressed) {
                return false;
            }
            return handle_key_press(kbd->key);
        }

        return base::handle_event(evt, phase);
    }

    /**
     * @brief Render the slider using tile renderer
     */
    void do_render(render_context<Backend>& ctx) const override {
        auto* renderer = get_renderer();
        if (!renderer) {
            return;
        }

        // Get physical position and size from context
        const auto& pos = ctx.position();
        const int x = point_utils::get_x(pos);
        const int y = point_utils::get_y(pos);
        const auto& size = ctx.available_size();
        const int w = size_utils::get_width(size);
        const int h = size_utils::get_height(size);

        // Get tiles
        const slider_tiles* tiles = nullptr;
        if (m_use_custom_tiles) {
            tiles = &m_custom_tiles;
        } else if (has_theme()) {
            tiles = &get_theme().slider;
        }

        if (!tiles) {
            return;
        }

        if (m_orientation == tile_slider_orientation::horizontal) {
            render_horizontal(renderer, x, y, w, h, *tiles);
        } else {
            render_vertical(renderer, x, y, w, h, *tiles);
        }
    }

    /**
     * @brief Get content size based on track length and tile height
     */
    [[nodiscard]] logical_size get_content_size() const override {
        int tile_height = 16;  // Default

        if (has_theme() && get_theme().atlas) {
            tile_height = get_theme().atlas->tile_height;
        }

        if (m_orientation == tile_slider_orientation::horizontal) {
            return logical_size{
                logical_unit(m_track_length),
                logical_unit(tile_height)
            };
        } else {
            return logical_size{
                logical_unit(tile_height),
                logical_unit(m_track_length)
            };
        }
    }

private:
    int m_value = 0;
    int m_min = 0;
    int m_max = 100;
    int m_single_step = 1;
    int m_page_step = 10;
    tile_slider_orientation m_orientation = tile_slider_orientation::horizontal;
    int m_track_length = 100;  // Default track length in pixels
    slider_tiles m_custom_tiles{};
    bool m_use_custom_tiles = false;
    bool m_dragging = false;

    /**
     * @brief Snap value to step grid
     */
    [[nodiscard]] static int snap_to_step(int value, int step) {
        if (step <= 0) return value;
        return ((value + step / 2) / step) * step;
    }

    /**
     * @brief Calculate thumb position for current value
     */
    [[nodiscard]] int calculate_thumb_position(int track_size) const {
        if (m_max <= m_min || track_size <= 0) {
            return 0;
        }
        return ((m_value - m_min) * track_size) / (m_max - m_min);
    }

    /**
     * @brief Calculate value from position
     */
    [[nodiscard]] int calculate_value_from_position(int pos, int track_size) const {
        if (track_size <= 0) {
            return m_min;
        }
        pos = std::clamp(pos, 0, track_size);
        return m_min + (pos * (m_max - m_min)) / track_size;
    }

    /**
     * @brief Handle mouse press
     */
    bool handle_mouse_press(const mouse_event& mouse) {
        // Get widget bounds
        auto const abs_bounds = this->get_absolute_logical_bounds();
        const double widget_x = abs_bounds.x.value;
        const double widget_y = abs_bounds.y.value;
        const double widget_w = abs_bounds.width.value;
        const double widget_h = abs_bounds.height.value;

        // Calculate relative position
        const double rel_x = mouse.x.value - widget_x;
        const double rel_y = mouse.y.value - widget_y;

        // Check bounds
        if (rel_x >= 0 && rel_x < widget_w && rel_y >= 0 && rel_y < widget_h) {
            m_dragging = true;
            slider_pressed.emit();

            // Calculate new value
            int new_value;
            if (m_orientation == tile_slider_orientation::horizontal) {
                new_value = calculate_value_from_position(
                    static_cast<int>(rel_x), static_cast<int>(widget_w)
                );
            } else {
                // Vertical: bottom = min, top = max
                new_value = calculate_value_from_position(
                    static_cast<int>(widget_h - rel_y), static_cast<int>(widget_h)
                );
            }

            set_value(new_value);
            slider_moved.emit(m_value);
            return true;
        }

        return false;
    }

    /**
     * @brief Handle mouse drag
     */
    bool handle_mouse_drag(const mouse_event& mouse) {
        auto const abs_bounds = this->get_absolute_logical_bounds();
        const double widget_x = abs_bounds.x.value;
        const double widget_y = abs_bounds.y.value;
        const double widget_w = abs_bounds.width.value;
        const double widget_h = abs_bounds.height.value;

        const double rel_x = mouse.x.value - widget_x;
        const double rel_y = mouse.y.value - widget_y;

        int new_value;
        if (m_orientation == tile_slider_orientation::horizontal) {
            new_value = calculate_value_from_position(
                static_cast<int>(rel_x), static_cast<int>(widget_w)
            );
        } else {
            new_value = calculate_value_from_position(
                static_cast<int>(widget_h - rel_y), static_cast<int>(widget_h)
            );
        }

        set_value(new_value);
        slider_moved.emit(m_value);
        return true;
    }

    /**
     * @brief Handle mouse release
     */
    bool handle_mouse_release() {
        if (m_dragging) {
            m_dragging = false;
            slider_released.emit();
            return true;
        }
        return false;
    }

    /**
     * @brief Handle key press
     */
    bool handle_key_press(key_code key) {
        switch (key) {
            case key_code::arrow_left:
            case key_code::arrow_down:
                set_value(m_value - m_single_step);
                return true;

            case key_code::arrow_right:
            case key_code::arrow_up:
                set_value(m_value + m_single_step);
                return true;

            case key_code::page_up:
                set_value(m_value + m_page_step);
                return true;

            case key_code::page_down:
                set_value(m_value - m_page_step);
                return true;

            case key_code::home:
                set_value(m_min);
                return true;

            case key_code::end:
                set_value(m_max);
                return true;

            default:
                return false;
        }
    }

    /**
     * @brief Get thumb tile for current state
     */
    [[nodiscard]] int get_thumb_tile(const slider_tiles& tiles) const {
        // TODO: Add hover/pressed state tracking
        return tiles.thumb;
    }

    /**
     * @brief Render horizontal slider
     */
    void render_horizontal(
        tile_renderer* renderer,
        int x, int y, int w, int h,
        const slider_tiles& tiles
    ) const {
        // Draw track
        if (tiles.track.is_valid()) {
            tile_renderer::tile_rect track_rect{x, y, w, h};
            renderer->draw_h_slice(tiles.track, track_rect);
        }

        // Draw thumb
        int const thumb_tile = get_thumb_tile(tiles);
        if (thumb_tile >= 0 && has_theme() && get_theme().atlas) {
            const int thumb_width = get_theme().atlas->tile_width;
            const int thumb_pos = calculate_thumb_position(w - thumb_width);
            tile_renderer::tile_point thumb_point{x + thumb_pos, y};
            renderer->draw_tile(thumb_tile, thumb_point);
        }
    }

    /**
     * @brief Render vertical slider
     */
    void render_vertical(
        tile_renderer* renderer,
        int x, int y, int w, int h,
        const slider_tiles& tiles
    ) const {
        // Draw track (convert h_slice to v_slice)
        if (tiles.track.is_valid()) {
            tile_renderer::tile_rect track_rect{x, y, w, h};
            renderer->draw_v_slice(tiles.track.to_v_slice(), track_rect);
        }

        // Draw thumb
        int const thumb_tile = get_thumb_tile(tiles);
        if (thumb_tile >= 0 && has_theme() && get_theme().atlas) {
            const int thumb_height = get_theme().atlas->tile_height;
            const int thumb_pos = calculate_thumb_position(h - thumb_height);
            // Vertical: thumb_pos=0 at bottom, so y = y + h - thumb_height - thumb_pos
            tile_renderer::tile_point thumb_point{x, y + h - thumb_height - thumb_pos};
            renderer->draw_tile(thumb_tile, thumb_point);
        }
    }
};

} // namespace onyxui::tile
