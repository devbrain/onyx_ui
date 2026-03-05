/**
 * @file tile_scrollbar.hh
 * @brief Scrollbar widget with tile-based rendering
 */

#pragma once

#include <algorithm>
#include <onyxui/widgets/core/widget.hh>
#include <onyxui/tile/tile_core.hh>
#include <onyxui/tile/tile_renderer.hh>
#include <onyxui/events/ui_event.hh>
#include <onyxui/core/orientation.hh>

namespace onyxui::tile {

/**
 * @class tile_scrollbar
 * @brief Scrollbar widget using tile-based rendering
 *
 * @tparam Backend The UI backend type (typically sdlpp_tile_backend)
 *
 * @details
 * A tile_scrollbar provides visual feedback and interactive control
 * for scrolling using tile-based rendering.
 *
 * Components:
 * - Track: Background bar (v_slice/h_slice)
 * - Thumb: Draggable indicator showing viewport position/size (v_slice/h_slice)
 * - Arrow buttons: Optional increment/decrement buttons (single tiles)
 *
 * @example
 * @code
 * auto scrollbar = std::make_unique<tile_scrollbar<sdlpp_tile_backend>>();
 * scrollbar->set_orientation(orientation::vertical);
 * scrollbar->set_content_size(500);   // Total content size
 * scrollbar->set_viewport_size(100);  // Visible area size
 * scrollbar->set_scroll_position(0);  // Current scroll offset
 *
 * scrollbar->scroll_requested.connect([](int new_pos) {
 *     // Update content scroll position
 * });
 * @endcode
 */
template<UIBackend Backend>
class tile_scrollbar : public widget<Backend> {
public:
    using base = widget<Backend>;

    /**
     * @brief Construct a scrollbar
     * @param orient Scrollbar orientation (default vertical)
     * @param parent Parent element (nullptr for none)
     */
    explicit tile_scrollbar(
        orientation orient = orientation::vertical,
        ui_element<Backend>* parent = nullptr
    )
        : base(parent), m_orientation(orient)
    {
        this->set_focusable(true);  // Scrollbars can receive keyboard focus
    }

    /**
     * @brief Destructor
     */
    ~tile_scrollbar() override = default;

    // Rule of Five
    tile_scrollbar(const tile_scrollbar&) = delete;
    tile_scrollbar& operator=(const tile_scrollbar&) = delete;
    tile_scrollbar(tile_scrollbar&&) noexcept = default;
    tile_scrollbar& operator=(tile_scrollbar&&) noexcept = default;

    // ===== Scroll Configuration =====

    /**
     * @brief Set total content size (scrollable area)
     * @param size Content size in pixels
     */
    void set_content_size(int size) {
        if (m_content_size != size) {
            m_content_size = std::max(0, size);
            this->mark_dirty();
        }
    }

    /**
     * @brief Get content size
     */
    [[nodiscard]] int content_size() const noexcept { return m_content_size; }

    /**
     * @brief Set viewport size (visible area)
     * @param size Viewport size in pixels
     */
    void set_viewport_size(int size) {
        if (m_viewport_size != size) {
            m_viewport_size = std::max(0, size);
            this->mark_dirty();
        }
    }

    /**
     * @brief Get viewport size
     */
    [[nodiscard]] int viewport_size() const noexcept { return m_viewport_size; }

    /**
     * @brief Set current scroll position
     * @param position Scroll offset in pixels
     */
    void set_scroll_position(int position) {
        int const max_scroll = std::max(0, m_content_size - m_viewport_size);
        int const clamped = std::clamp(position, 0, max_scroll);
        if (m_scroll_position != clamped) {
            m_scroll_position = clamped;
            this->mark_dirty();
        }
    }

    /**
     * @brief Get current scroll position
     */
    [[nodiscard]] int scroll_position() const noexcept { return m_scroll_position; }

    // ===== Orientation =====

    /**
     * @brief Set scrollbar orientation
     * @param orient Horizontal or vertical
     */
    void set_orientation(orientation orient) {
        if (m_orientation != orient) {
            m_orientation = orient;
            this->invalidate_measure();
        }
    }

    /**
     * @brief Get orientation
     */
    [[nodiscard]] orientation get_orientation() const noexcept {
        return m_orientation;
    }

    // ===== Step Configuration =====

    /**
     * @brief Set line step (arrow click / keyboard arrow)
     * @param step Step size in pixels
     */
    void set_line_step(int step) {
        m_line_step = std::max(1, step);
    }

    /**
     * @brief Get line step
     */
    [[nodiscard]] int line_step() const noexcept { return m_line_step; }

    /**
     * @brief Set page step (track click / Page Up/Down)
     * @param step Step size in pixels
     */
    void set_page_step(int step) {
        m_page_step = std::max(1, step);
    }

    /**
     * @brief Get page step
     */
    [[nodiscard]] int page_step() const noexcept { return m_page_step; }

    // ===== Arrow Buttons =====

    /**
     * @brief Enable or disable arrow buttons
     * @param show True to show arrows
     */
    void set_show_arrows(bool show) {
        if (m_show_arrows != show) {
            m_show_arrows = show;
            this->invalidate_measure();
        }
    }

    /**
     * @brief Check if arrows are shown
     */
    [[nodiscard]] bool show_arrows() const noexcept { return m_show_arrows; }

    /**
     * @brief Set custom scrollbar tiles (overrides theme)
     * @param tiles Scrollbar tile configuration
     */
    void set_tiles(const scrollbar_tiles& tiles) {
        m_custom_tiles = tiles;
        m_use_custom_tiles = true;
    }

    /**
     * @brief Reset to using theme's scrollbar tiles
     */
    void use_theme_tiles() {
        m_use_custom_tiles = false;
    }

    // ===== Signals =====

    /// Emitted when user scrolls via scrollbar interaction
    signal<int> scroll_requested;

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
     * @brief Render the scrollbar using tile renderer
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
        const scrollbar_tiles* tiles = nullptr;
        if (m_use_custom_tiles) {
            tiles = &m_custom_tiles;
        } else if (has_theme()) {
            tiles = &get_theme().scrollbar;
        }

        if (!tiles) {
            return;
        }

        if (m_orientation == orientation::vertical) {
            render_vertical(renderer, x, y, w, h, *tiles);
        } else {
            render_horizontal(renderer, x, y, w, h, *tiles);
        }
    }

    /**
     * @brief Get content size based on orientation
     */
    [[nodiscard]] logical_size get_content_size() const override {
        int tile_size = 16;  // Default tile size

        if (has_theme() && get_theme().atlas) {
            tile_size = m_orientation == orientation::vertical
                ? get_theme().atlas->tile_width
                : get_theme().atlas->tile_height;
        }

        // Scrollbar is fixed thickness, variable length
        if (m_orientation == orientation::vertical) {
            return logical_size{
                logical_unit(tile_size),
                logical_unit(m_min_length)
            };
        } else {
            return logical_size{
                logical_unit(m_min_length),
                logical_unit(tile_size)
            };
        }
    }

private:
    orientation m_orientation = orientation::vertical;
    int m_content_size = 100;       ///< Total scrollable content size
    int m_viewport_size = 100;      ///< Visible viewport size
    int m_scroll_position = 0;      ///< Current scroll offset
    int m_line_step = 20;           ///< Arrow click step
    int m_page_step = 100;          ///< Track click step
    int m_min_length = 50;          ///< Minimum scrollbar length
    bool m_show_arrows = true;      ///< Show arrow buttons?
    scrollbar_tiles m_custom_tiles{};
    bool m_use_custom_tiles = false;
    bool m_dragging = false;
    int m_drag_start_pos = 0;       ///< Scroll position at drag start
    int m_drag_start_mouse = 0;     ///< Mouse position at drag start

    /**
     * @brief Get arrow size from theme
     */
    [[nodiscard]] int get_arrow_size() const {
        if (!m_show_arrows) {
            return 0;
        }
        if (has_theme() && get_theme().atlas) {
            return m_orientation == orientation::vertical
                ? get_theme().atlas->tile_height
                : get_theme().atlas->tile_width;
        }
        return 16;  // Default
    }

    /**
     * @brief Calculate track bounds (excluding arrows)
     */
    [[nodiscard]] std::tuple<int, int, int, int> get_track_bounds(int x, int y, int w, int h) const {
        int const arrow_size = get_arrow_size();

        if (m_orientation == orientation::vertical) {
            int const track_y = y + arrow_size;
            int const track_h = std::max(0, h - 2 * arrow_size);
            return {x, track_y, w, track_h};
        } else {
            int const track_x = x + arrow_size;
            int const track_w = std::max(0, w - 2 * arrow_size);
            return {track_x, y, track_w, h};
        }
    }

    /**
     * @brief Calculate thumb position and size
     */
    [[nodiscard]] std::tuple<int, int> calculate_thumb(int track_size) const {
        if (m_content_size <= m_viewport_size || track_size <= 0) {
            return {0, track_size};  // Thumb fills track
        }

        // Thumb size proportional to viewport/content
        int const min_thumb = 16;  // Minimum thumb size
        int thumb_size = std::max(min_thumb,
            (m_viewport_size * track_size) / m_content_size);
        thumb_size = std::min(thumb_size, track_size);

        // Thumb position proportional to scroll position
        int const max_scroll = m_content_size - m_viewport_size;
        int const max_thumb_travel = track_size - thumb_size;
        int thumb_pos = 0;

        if (max_scroll > 0) {
            thumb_pos = (m_scroll_position * max_thumb_travel) / max_scroll;
        }

        thumb_pos = std::clamp(thumb_pos, 0, max_thumb_travel);
        return {thumb_pos, thumb_size};
    }

    /**
     * @brief Calculate scroll position from thumb position
     */
    [[nodiscard]] int calculate_scroll_from_thumb(int thumb_pos, int track_size, int thumb_size) const {
        int const max_scroll = m_content_size - m_viewport_size;
        int const max_thumb_travel = track_size - thumb_size;

        if (max_thumb_travel <= 0 || max_scroll <= 0) {
            return 0;
        }

        return (thumb_pos * max_scroll) / max_thumb_travel;
    }

    /**
     * @brief Handle mouse press
     */
    bool handle_mouse_press(const mouse_event& mouse) {
        auto const abs_bounds = this->get_absolute_logical_bounds();
        int const widget_x = static_cast<int>(abs_bounds.x.value);
        int const widget_y = static_cast<int>(abs_bounds.y.value);
        int const widget_w = static_cast<int>(abs_bounds.width.value);
        int const widget_h = static_cast<int>(abs_bounds.height.value);

        int const mouse_x = static_cast<int>(mouse.x.value);
        int const mouse_y = static_cast<int>(mouse.y.value);
        int const rel_x = mouse_x - widget_x;
        int const rel_y = mouse_y - widget_y;

        // Check bounds
        if (rel_x < 0 || rel_x >= widget_w || rel_y < 0 || rel_y >= widget_h) {
            return false;
        }

        int const arrow_size = get_arrow_size();
        auto [track_x, track_y, track_w, track_h] = get_track_bounds(0, 0, widget_w, widget_h);

        if (m_orientation == orientation::vertical) {
            // Check arrows
            if (m_show_arrows && arrow_size > 0) {
                if (rel_y < arrow_size) {
                    // Up arrow
                    scroll_by(-m_line_step);
                    return true;
                }
                if (rel_y >= widget_h - arrow_size) {
                    // Down arrow
                    scroll_by(m_line_step);
                    return true;
                }
            }

            // Check track/thumb
            int const track_size = track_h;
            auto [thumb_pos, thumb_size] = calculate_thumb(track_size);
            int const thumb_y = track_y + thumb_pos;

            if (rel_y >= thumb_y && rel_y < thumb_y + thumb_size) {
                // Click on thumb - start drag
                m_dragging = true;
                m_drag_start_pos = m_scroll_position;
                m_drag_start_mouse = mouse_y;
                return true;
            } else if (rel_y >= track_y && rel_y < track_y + track_h) {
                // Click on track - page scroll
                if (rel_y < thumb_y) {
                    scroll_by(-m_page_step);
                } else {
                    scroll_by(m_page_step);
                }
                return true;
            }
        } else {
            // Horizontal
            if (m_show_arrows && arrow_size > 0) {
                if (rel_x < arrow_size) {
                    scroll_by(-m_line_step);
                    return true;
                }
                if (rel_x >= widget_w - arrow_size) {
                    scroll_by(m_line_step);
                    return true;
                }
            }

            int const track_size = track_w;
            auto [thumb_pos, thumb_size] = calculate_thumb(track_size);
            int const thumb_x = track_x + thumb_pos;

            if (rel_x >= thumb_x && rel_x < thumb_x + thumb_size) {
                m_dragging = true;
                m_drag_start_pos = m_scroll_position;
                m_drag_start_mouse = mouse_x;
                return true;
            } else if (rel_x >= track_x && rel_x < track_x + track_w) {
                if (rel_x < thumb_x) {
                    scroll_by(-m_page_step);
                } else {
                    scroll_by(m_page_step);
                }
                return true;
            }
        }

        return false;
    }

    /**
     * @brief Handle mouse drag
     */
    bool handle_mouse_drag(const mouse_event& mouse) {
        auto const abs_bounds = this->get_absolute_logical_bounds();
        int const widget_w = static_cast<int>(abs_bounds.width.value);
        int const widget_h = static_cast<int>(abs_bounds.height.value);

        auto [track_x, track_y, track_w, track_h] = get_track_bounds(0, 0, widget_w, widget_h);

        int track_size;
        int mouse_pos;
        if (m_orientation == orientation::vertical) {
            track_size = track_h;
            mouse_pos = static_cast<int>(mouse.y.value);
        } else {
            track_size = track_w;
            mouse_pos = static_cast<int>(mouse.x.value);
        }

        auto [thumb_pos_unused, thumb_size] = calculate_thumb(track_size);

        // Calculate mouse delta and convert to scroll delta
        int const mouse_delta = mouse_pos - m_drag_start_mouse;
        int const max_scroll = m_content_size - m_viewport_size;
        int const max_thumb_travel = track_size - thumb_size;

        if (max_thumb_travel > 0 && max_scroll > 0) {
            int const scroll_delta = (mouse_delta * max_scroll) / max_thumb_travel;
            int const new_scroll = std::clamp(m_drag_start_pos + scroll_delta, 0, max_scroll);

            if (new_scroll != m_scroll_position) {
                m_scroll_position = new_scroll;
                this->mark_dirty();
                scroll_requested.emit(m_scroll_position);
            }
        }

        return true;
    }

    /**
     * @brief Handle mouse release
     */
    bool handle_mouse_release() {
        if (m_dragging) {
            m_dragging = false;
            return true;
        }
        return false;
    }

    /**
     * @brief Handle key press
     */
    bool handle_key_press(key_code key) {
        switch (key) {
            case key_code::arrow_up:
            case key_code::arrow_left:
                scroll_by(-m_line_step);
                return true;

            case key_code::arrow_down:
            case key_code::arrow_right:
                scroll_by(m_line_step);
                return true;

            case key_code::page_up:
                scroll_by(-m_page_step);
                return true;

            case key_code::page_down:
                scroll_by(m_page_step);
                return true;

            case key_code::home:
                scroll_to(0);
                return true;

            case key_code::end:
                scroll_to(m_content_size - m_viewport_size);
                return true;

            default:
                return false;
        }
    }

    /**
     * @brief Scroll by delta
     */
    void scroll_by(int delta) {
        scroll_to(m_scroll_position + delta);
    }

    /**
     * @brief Scroll to position
     */
    void scroll_to(int position) {
        int const max_scroll = std::max(0, m_content_size - m_viewport_size);
        int const new_pos = std::clamp(position, 0, max_scroll);

        if (new_pos != m_scroll_position) {
            m_scroll_position = new_pos;
            this->mark_dirty();
            scroll_requested.emit(m_scroll_position);
        }
    }

    /**
     * @brief Render vertical scrollbar
     */
    void render_vertical(
        tile_renderer* renderer,
        int x, int y, int w, int h,
        const scrollbar_tiles& tiles
    ) const {
        int const arrow_size = get_arrow_size();

        // Draw up arrow
        if (m_show_arrows && tiles.arrow_up >= 0) {
            tile_renderer::tile_point arrow_pos{x, y};
            renderer->draw_tile(tiles.arrow_up, arrow_pos);
        }

        // Draw down arrow
        if (m_show_arrows && tiles.arrow_down >= 0) {
            tile_renderer::tile_point arrow_pos{x, y + h - arrow_size};
            renderer->draw_tile(tiles.arrow_down, arrow_pos);
        }

        // Draw track
        int const track_y = y + arrow_size;
        int const track_h = std::max(0, h - 2 * arrow_size);

        if (track_h > 0 && tiles.track_v.is_valid()) {
            tile_renderer::tile_rect track_rect{x, track_y, w, track_h};
            renderer->draw_v_slice(tiles.track_v, track_rect);
        }

        // Draw thumb
        if (track_h > 0 && tiles.thumb_v.is_valid()) {
            auto [thumb_pos, thumb_size] = calculate_thumb(track_h);
            if (thumb_size > 0) {
                tile_renderer::tile_rect thumb_rect{x, track_y + thumb_pos, w, thumb_size};
                renderer->draw_v_slice(tiles.thumb_v, thumb_rect);
            }
        }
    }

    /**
     * @brief Render horizontal scrollbar
     */
    void render_horizontal(
        tile_renderer* renderer,
        int x, int y, int w, int h,
        const scrollbar_tiles& tiles
    ) const {
        int const arrow_size = get_arrow_size();

        // Draw left arrow
        if (m_show_arrows && tiles.arrow_left >= 0) {
            tile_renderer::tile_point arrow_pos{x, y};
            renderer->draw_tile(tiles.arrow_left, arrow_pos);
        }

        // Draw right arrow
        if (m_show_arrows && tiles.arrow_right >= 0) {
            tile_renderer::tile_point arrow_pos{x + w - arrow_size, y};
            renderer->draw_tile(tiles.arrow_right, arrow_pos);
        }

        // Draw track
        int const track_x = x + arrow_size;
        int const track_w = std::max(0, w - 2 * arrow_size);

        if (track_w > 0 && tiles.track_h.is_valid()) {
            tile_renderer::tile_rect track_rect{track_x, y, track_w, h};
            renderer->draw_h_slice(tiles.track_h, track_rect);
        }

        // Draw thumb
        if (track_w > 0 && tiles.thumb_h.is_valid()) {
            auto [thumb_pos, thumb_size] = calculate_thumb(track_w);
            if (thumb_size > 0) {
                tile_renderer::tile_rect thumb_rect{track_x + thumb_pos, y, thumb_size, h};
                renderer->draw_h_slice(tiles.thumb_h, thumb_rect);
            }
        }
    }
};

} // namespace onyxui::tile
