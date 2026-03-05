/**
 * @file tile_progress_bar.hh
 * @brief Progress bar widget with tile-based rendering
 */

#pragma once

#include <string>
#include <algorithm>
#include <onyxui/widgets/core/widget.hh>
#include <onyxui/tile/tile_core.hh>
#include <onyxui/tile/tile_renderer.hh>

namespace onyxui::tile {

/// Progress bar orientation
enum class tile_progress_orientation : std::uint8_t {
    horizontal,
    vertical
};

/**
 * @class tile_progress_bar
 * @brief Progress bar widget using tile-based rendering
 *
 * @tparam Backend The UI backend type (typically sdlpp_tile_backend)
 *
 * @details
 * A tile_progress_bar displays progress using h_slice (horizontal) or
 * v_slice (vertical) for both the track and fill portions.
 *
 * @example
 * @code
 * auto progress = std::make_unique<tile_progress_bar<sdlpp_tile_backend>>();
 * progress->set_value(45);  // 45%
 *
 * progress->value_changed.connect([](int val) {
 *     std::cout << "Progress: " << val << "%\n";
 * });
 * @endcode
 */
template<UIBackend Backend>
class tile_progress_bar : public widget<Backend> {
public:
    using base = widget<Backend>;

    /**
     * @brief Construct a progress bar
     * @param parent Parent element (nullptr for none)
     */
    explicit tile_progress_bar(ui_element<Backend>* parent = nullptr)
        : base(parent)
    {
        this->set_focusable(false);  // Progress bars aren't focusable
    }

    /**
     * @brief Destructor
     */
    ~tile_progress_bar() override = default;

    // Rule of Five
    tile_progress_bar(const tile_progress_bar&) = delete;
    tile_progress_bar& operator=(const tile_progress_bar&) = delete;
    tile_progress_bar(tile_progress_bar&&) noexcept = default;
    tile_progress_bar& operator=(tile_progress_bar&&) noexcept = default;

    // ===== Value Management =====

    /**
     * @brief Set current value
     * @param value New value (will be clamped to [min, max])
     */
    void set_value(int value) {
        int const clamped = std::clamp(value, m_min, m_max);
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

    // ===== Indeterminate Mode =====

    /**
     * @brief Set indeterminate mode (animated busy indicator)
     * @param indeterminate True for indeterminate mode
     */
    void set_indeterminate(bool indeterminate) {
        if (m_indeterminate != indeterminate) {
            m_indeterminate = indeterminate;
            this->mark_dirty();
        }
    }

    /**
     * @brief Check if in indeterminate mode
     */
    [[nodiscard]] bool is_indeterminate() const noexcept { return m_indeterminate; }

    // ===== Orientation =====

    /**
     * @brief Set orientation
     * @param orientation Horizontal or vertical
     */
    void set_orientation(tile_progress_orientation orientation) {
        if (m_orientation != orientation) {
            m_orientation = orientation;
            this->invalidate_measure();
        }
    }

    /**
     * @brief Get orientation
     */
    [[nodiscard]] tile_progress_orientation orientation() const noexcept {
        return m_orientation;
    }

    // ===== Sizing =====

    /**
     * @brief Set preferred bar length
     * @param length Bar length in pixels
     */
    void set_bar_length(int length) {
        if (m_bar_length != length) {
            m_bar_length = length;
            this->invalidate_measure();
        }
    }

    /**
     * @brief Get bar length
     */
    [[nodiscard]] int bar_length() const noexcept { return m_bar_length; }

    /**
     * @brief Set custom progress bar tiles (overrides theme)
     * @param tiles Progress bar tile configuration
     */
    void set_tiles(const progress_bar_tiles& tiles) {
        m_custom_tiles = tiles;
        m_use_custom_tiles = true;
    }

    /**
     * @brief Reset to using theme's progress bar tiles
     */
    void use_theme_tiles() {
        m_use_custom_tiles = false;
    }

    // ===== Signals =====

    /// Emitted when value changes
    signal<int> value_changed;

protected:
    /**
     * @brief Render the progress bar using tile renderer
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
        const progress_bar_tiles* tiles = nullptr;
        if (m_use_custom_tiles) {
            tiles = &m_custom_tiles;
        } else if (has_theme()) {
            tiles = &get_theme().progress_bar;
        }

        if (!tiles) {
            return;
        }

        if (m_orientation == tile_progress_orientation::horizontal) {
            render_horizontal(renderer, x, y, w, h, *tiles);
        } else {
            render_vertical(renderer, x, y, w, h, *tiles);
        }
    }

    /**
     * @brief Get content size based on bar length and tile height
     */
    [[nodiscard]] logical_size get_content_size() const override {
        int tile_height = 16;  // Default

        if (has_theme() && get_theme().atlas) {
            tile_height = get_theme().atlas->tile_height;
        }

        if (m_orientation == tile_progress_orientation::horizontal) {
            return logical_size{
                logical_unit(m_bar_length),
                logical_unit(tile_height)
            };
        } else {
            return logical_size{
                logical_unit(tile_height),
                logical_unit(m_bar_length)
            };
        }
    }

private:
    int m_value = 0;
    int m_min = 0;
    int m_max = 100;
    bool m_indeterminate = false;
    tile_progress_orientation m_orientation = tile_progress_orientation::horizontal;
    int m_bar_length = 100;  // Default bar length in pixels
    progress_bar_tiles m_custom_tiles{};
    bool m_use_custom_tiles = false;

    /**
     * @brief Calculate fill length for current value
     */
    [[nodiscard]] int calculate_fill_length(int total_length) const {
        if (m_max <= m_min || total_length <= 0) {
            return 0;
        }
        return ((m_value - m_min) * total_length) / (m_max - m_min);
    }

    /**
     * @brief Render horizontal progress bar
     */
    void render_horizontal(
        tile_renderer* renderer,
        int x, int y, int w, int h,
        const progress_bar_tiles& tiles
    ) const {
        tile_renderer::tile_rect track_rect{x, y, w, h};

        if (m_indeterminate) {
            // Just draw empty track for indeterminate
            if (tiles.track.is_valid()) {
                renderer->draw_h_slice(tiles.track, track_rect);
            }
        } else {
            // Draw track (background)
            if (tiles.track.is_valid()) {
                renderer->draw_h_slice(tiles.track, track_rect);
            }

            // Draw fill (foreground) up to fill length
            int const fill_length = calculate_fill_length(w);
            if (fill_length > 0 && tiles.fill.is_valid()) {
                tile_renderer::tile_rect fill_rect{x, y, fill_length, h};
                renderer->draw_h_slice(tiles.fill, fill_rect);
            }
        }
    }

    /**
     * @brief Render vertical progress bar
     */
    void render_vertical(
        tile_renderer* renderer,
        int x, int y, int w, int h,
        const progress_bar_tiles& tiles
    ) const {
        tile_renderer::tile_rect track_rect{x, y, w, h};

        if (m_indeterminate) {
            // Just draw empty track for indeterminate
            if (tiles.track.is_valid()) {
                renderer->draw_v_slice(tiles.track.to_v_slice(), track_rect);
            }
        } else {
            // Draw track (background)
            if (tiles.track.is_valid()) {
                renderer->draw_v_slice(tiles.track.to_v_slice(), track_rect);
            }

            // Draw fill (foreground) from bottom up
            int const fill_length = calculate_fill_length(h);
            if (fill_length > 0 && tiles.fill.is_valid()) {
                tile_renderer::tile_rect fill_rect{x, y + h - fill_length, w, fill_length};
                renderer->draw_v_slice(tiles.fill.to_v_slice(), fill_rect);
            }
        }
    }
};

} // namespace onyxui::tile
