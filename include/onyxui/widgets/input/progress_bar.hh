// Copyright (c) 2025 OnyxUI Framework
// SPDX-License-Identifier: MIT

#pragma once

#include "onyxui/concepts/backend.hh"
#include <string>
#include <onyxui/widgets/core/widget.hh>
#include <onyxui/widgets/utils/range_helpers.hh>
#include <onyxui/core/signal.hh>
#include <onyxui/core/rendering/draw_context.hh>

namespace onyxui {

/// Progress bar orientation
enum class progress_bar_orientation : std::uint8_t {
    horizontal,
    vertical
};

/**
 * @class progress_bar
 * @brief Visual progress indicator widget
 *
 * @tparam Backend The UI backend type
 *
 * @details
 * Displays progress of operations (downloads, loading, processing) with
 * optional text overlay. Supports both determinate (0-100%) and indeterminate
 * (busy indicator) modes.
 *
 * ## Features
 *
 * - Determinate mode (0-100% or custom range)
 * - Indeterminate mode (animated busy indicator)
 * - Horizontal/vertical orientation
 * - Text overlay (e.g., "45% complete" or "45/100 files")
 * - Customizable text format with placeholders
 * - Theme integration
 *
 * @example
 * @code
 * // Simple percentage progress bar
 * auto progress = std::make_unique<progress_bar<Backend>>();
 * progress->set_value(45);  // 45%
 * progress->set_text_visible(true);
 *
 * // Custom range with formatted text
 * auto file_progress = std::make_unique<progress_bar<Backend>>();
 * file_progress->set_range(0, 100);
 * file_progress->set_text_format("%v/%m files");
 * file_progress->set_text_visible(true);
 *
 * // Indeterminate busy indicator
 * auto loading = std::make_unique<progress_bar<Backend>>();
 * loading->set_indeterminate(true);
 * @endcode
 */
template<UIBackend Backend>
class progress_bar : public widget<Backend> {
public:
    using base = widget<Backend>;
    using renderer_type = typename Backend::renderer_type;
    using size_type = typename Backend::size_type;
    using color_type = typename Backend::color_type;
    using rect_type = typename Backend::rect_type;
    using point_type = typename Backend::point_type;

    // ===== Construction =====

    /**
     * @brief Construct a progress bar
     * @param parent Parent element (nullptr for none)
     */
    explicit progress_bar(ui_element<Backend>* parent = nullptr)
        : base(parent) {
        this->set_focusable(false);  // Progress bars aren't focusable
    }

    /**
     * @brief Destructor
     */
    ~progress_bar() override = default;

    // Rule of Five
    progress_bar(const progress_bar&) = delete;
    progress_bar& operator=(const progress_bar&) = delete;
    progress_bar(progress_bar&&) noexcept = default;
    progress_bar& operator=(progress_bar&&) noexcept = default;

    // ===== Value Management =====

    /**
     * @brief Set current value
     * @param value New value (will be clamped to [min, max])
     */
    void set_value(int value) {
        int const clamped = std::clamp(value, m_min, m_max);
        if (m_value != clamped) {
            m_value = clamped;
            this->mark_dirty();  // Visual change only
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

    // ===== Indeterminate Mode =====

    /**
     * @brief Set indeterminate mode (animated busy indicator)
     * @param indeterminate True for indeterminate mode, false for determinate
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
    void set_orientation(progress_bar_orientation orientation) {
        if (m_orientation != orientation) {
            m_orientation = orientation;
            this->invalidate_measure();  // Size may change
        }
    }

    /**
     * @brief Get orientation
     */
    [[nodiscard]] progress_bar_orientation orientation() const noexcept {
        return m_orientation;
    }

    // ===== Text Overlay =====

    /**
     * @brief Set text visibility
     * @param visible True to show text overlay, false to hide
     */
    void set_text_visible(bool visible) {
        if (m_text_visible != visible) {
            m_text_visible = visible;
            this->mark_dirty();
        }
    }

    /**
     * @brief Check if text is visible
     */
    [[nodiscard]] bool is_text_visible() const noexcept { return m_text_visible; }

    /**
     * @brief Set text format string
     * @param format Format string with placeholders:
     *               - %v = current value
     *               - %p = percentage (0-100)
     *               - %m = maximum value
     *
     * @example
     * @code
     * progress->set_text_format("%v%");          // "45%"
     * progress->set_text_format("%v/%m files");  // "45/100 files"
     * progress->set_text_format("%p%% complete"); // "45%% complete" (escape % with %%)
     * @endcode
     */
    void set_text_format(const std::string& format) {
        if (m_text_format != format) {
            m_text_format = format;
            if (m_text_visible) {
                this->mark_dirty();
            }
        }
    }

    /**
     * @brief Get text format string
     */
    [[nodiscard]] std::string text_format() const noexcept { return m_text_format; }

    /**
     * @brief Get formatted text for current value
     * @return Formatted text with placeholders replaced
     */
    [[nodiscard]] std::string formatted_text() const {
        return format_text_internal();
    }

    // ===== Signals =====

    /// Emitted when value changes
    signal<int> value_changed;

protected:
    /**
     * @brief Calculate natural content size based on orientation
     */
    [[nodiscard]] size_type get_content_size() const override {
        // Progress bar has a default size based on orientation
        constexpr int DEFAULT_HORIZONTAL_WIDTH = 100;
        constexpr int DEFAULT_HORIZONTAL_HEIGHT = 1;
        constexpr int DEFAULT_VERTICAL_WIDTH = 1;
        constexpr int DEFAULT_VERTICAL_HEIGHT = 20;

        if (m_orientation == progress_bar_orientation::horizontal) {
            return size_type{DEFAULT_HORIZONTAL_WIDTH, DEFAULT_HORIZONTAL_HEIGHT};
        } else {
            return size_type{DEFAULT_VERTICAL_WIDTH, DEFAULT_VERTICAL_HEIGHT};
        }
    }

    /**
     * @brief Render the progress bar using render context
     */
    void do_render(render_context<Backend>& ctx) const override {
        // Get theme colors
        auto* theme = ctx.theme();
        if (!theme) {
            return;
        }

        // Use progress_bar theme style
        auto const& filled_color = theme->progress_bar.filled_color;
        auto const& empty_color = theme->progress_bar.empty_color;

        // Get absolute position and size from bounds
        const auto& pos = ctx.position();
        const auto& bounds = this->bounds();

        int const track_x = point_utils::get_x(pos);
        int const track_y = point_utils::get_y(pos);
        int const track_w = rect_utils::get_width(bounds);
        int const track_h = rect_utils::get_height(bounds);

        // Render based on orientation
        if (m_orientation == progress_bar_orientation::horizontal) {
            render_horizontal_track(ctx, track_x, track_y, track_w, track_h, filled_color, empty_color);
        } else {
            render_vertical_track(ctx, track_x, track_y, track_w, track_h, filled_color, empty_color);
        }

        // Render text overlay if visible
        if (m_text_visible && !m_indeterminate) {
            render_text_overlay(ctx, track_x, track_y, track_w, track_h);
        }
    }

private:
    // ===== State =====
    int m_value = 0;
    int m_min = 0;
    int m_max = 100;
    bool m_indeterminate = false;
    progress_bar_orientation m_orientation = progress_bar_orientation::horizontal;
    bool m_text_visible = false;
    std::string m_text_format = "%p%";  // Default: "45%"

    // Animation state (for indeterminate mode)
    mutable int m_animation_offset = 0;

    // ===== Helper Methods =====

    /**
     * @brief Convert icon enum to character string
     * @param icon Icon style enum
     * @return Single-character string representation
     */
    [[nodiscard]] static std::string icon_to_char(typename Backend::renderer_type::icon_style icon) {
        using icon_type = typename Backend::renderer_type::icon_style;

        switch (icon) {
            case icon_type::progress_filled:  return "#";
            case icon_type::progress_empty:   return ".";
            default:                          return " ";
        }
    }

    /**
     * @brief Format text with placeholders
     * @return Formatted text string
     */
    [[nodiscard]] std::string format_text_internal() const {
        std::string result;
        result.reserve(m_text_format.size());

        // Calculate percentage
        int percentage = 0;
        if (m_max > m_min) {
            percentage = ((m_value - m_min) * 100) / (m_max - m_min);
        }

        // Replace placeholders
        for (size_t i = 0; i < m_text_format.length(); ++i) {
            if (m_text_format[i] == '%' && i + 1 < m_text_format.length()) {
                char const placeholder = m_text_format[i + 1];
                switch (placeholder) {
                    case 'v':  // Current value
                        result += std::to_string(m_value);
                        ++i;
                        break;
                    case 'p':  // Percentage
                        result += std::to_string(percentage);
                        ++i;
                        break;
                    case 'm':  // Maximum value
                        result += std::to_string(m_max);
                        ++i;
                        break;
                    case '%':  // Escaped percent sign
                        result += '%';
                        ++i;
                        break;
                    default:
                        result += m_text_format[i];
                        break;
                }
            } else {
                result += m_text_format[i];
            }
        }

        return result;
    }

    /**
     * @brief Render horizontal track
     */
    void render_horizontal_track(
        render_context<Backend>& ctx,
        int x,
        int y,
        int width,
        int height,
        color_type const& filled_color,
        color_type const& empty_color
    ) const {
        // Get icons from theme
        auto* theme = ctx.theme();
        if (!theme) {
            return;
        }

        // Convert icons to character strings
        std::string const filled_char = icon_to_char(theme->progress_bar.filled_icon);
        std::string const empty_char = icon_to_char(theme->progress_bar.empty_icon);

        typename Backend::renderer_type::font track_font{};

        if (m_indeterminate) {
            // Indeterminate mode: fill with empty color (animated in future)
            for (int row = 0; row < height; ++row) {
                std::string empty_str(static_cast<std::size_t>(width), empty_char[0]);
                point_type const pos{x, y + row};
                ctx.draw_text(empty_str, pos, track_font, empty_color);
            }
        } else {
            // Determinate mode: render filled portion
            int const fill_length = range_helpers::calculate_fill_length(
                m_value, m_min, m_max, width
            );

            // Render filled and empty portions
            for (int row = 0; row < height; ++row) {
                // Filled portion (left side)
                if (fill_length > 0) {
                    std::string filled_str(static_cast<std::size_t>(fill_length), filled_char[0]);
                    point_type const filled_pos{x, y + row};
                    ctx.draw_text(filled_str, filled_pos, track_font, filled_color);
                }

                // Empty portion (right side)
                if (fill_length < width) {
                    std::string empty_str(static_cast<std::size_t>(width - fill_length), empty_char[0]);
                    point_type const empty_pos{x + fill_length, y + row};
                    ctx.draw_text(empty_str, empty_pos, track_font, empty_color);
                }
            }
        }
    }

    /**
     * @brief Render vertical track
     */
    void render_vertical_track(
        render_context<Backend>& ctx,
        int x,
        int y,
        int width,
        int height,
        color_type const& filled_color,
        color_type const& empty_color
    ) const {
        // Get icons from theme
        auto* theme = ctx.theme();
        if (!theme) {
            return;
        }

        // Convert icons to character strings
        std::string const filled_char = icon_to_char(theme->progress_bar.filled_icon);
        std::string const empty_char = icon_to_char(theme->progress_bar.empty_icon);

        typename Backend::renderer_type::font track_font{};

        if (m_indeterminate) {
            // Indeterminate mode: fill with empty color (animated in future)
            for (int row = 0; row < height; ++row) {
                std::string empty_str(static_cast<std::size_t>(width), empty_char[0]);
                point_type const pos{x, y + row};
                ctx.draw_text(empty_str, pos, track_font, empty_color);
            }
        } else {
            // Determinate mode: render filled portion (bottom to top)
            int const fill_length = range_helpers::calculate_fill_length(
                m_value, m_min, m_max, height
            );

            // Render empty portion (top) and filled portion (bottom)
            for (int row = 0; row < height; ++row) {
                bool const is_filled = row >= (height - fill_length);
                std::string const& row_char = is_filled ? filled_char : empty_char;
                color_type const& color = is_filled ? filled_color : empty_color;

                std::string row_str(static_cast<std::size_t>(width), row_char[0]);
                point_type const pos{x, y + row};
                ctx.draw_text(row_str, pos, track_font, color);
            }
        }
    }

    /**
     * @brief Render text overlay
     */
    void render_text_overlay(
        render_context<Backend>& ctx,
        int x,
        int y,
        int width,
        int height
    ) const {
        std::string const text = format_text_internal();
        if (text.empty()) {
            return;
        }

        auto* theme = ctx.theme();
        if (!theme) {
            return;
        }

        // Get text style from progress_bar theme
        auto const& font = theme->progress_bar.text_font;
        auto const& text_color = theme->progress_bar.text_color;

        // Measure text size using renderer
        auto const text_size = renderer_type::measure_text(text, font);
        int const text_w = size_utils::get_width(text_size);
        int const text_h = size_utils::get_height(text_size);

        // Center text in track
        int const text_x = x + (width - text_w) / 2;
        int const text_y = y + (height - text_h) / 2;

        // Draw text
        point_type const text_pos{text_x, text_y};
        ctx.draw_text(text, text_pos, font, text_color);
    }
};

}  // namespace onyxui
