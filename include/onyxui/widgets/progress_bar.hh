// Copyright (c) 2025 OnyxUI Framework
// SPDX-License-Identifier: MIT

#pragma once

#include "onyxui/concepts/backend.hh"
#include <string>
#include <onyxui/widgets/core/widget.hh>
#include <onyxui/widgets/utils/range_helpers.hh>
#include <onyxui/core/signal.hh>
#include <onyxui/core/rendering/draw_context.hh>
#include <onyxui/services/ui_services.hh>

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

    // ===== Semantic Sizing (Backend-Agnostic) =====

    /**
     * @brief Set preferred bar width/length
     * @param width Bar width in backend-agnostic units
     *
     * @details
     * Sets the preferred bar width (length for horizontal bar, width for vertical
     * bar) in logical units. This is a backend-agnostic way to size progress bars
     * that works correctly across different backends.
     *
     * The widget will use this as a preferred size hint during layout, but may
     * be stretched or shrunk based on parent layout constraints.
     *
     * The bar width affects the primary dimension:
     * - **Horizontal progress bar**: Sets preferred width
     * - **Vertical progress bar**: Sets preferred height
     *
     * @note The cross-dimension (height for horizontal, width for vertical) is
     *       typically 1 logical unit for simple bars.
     *
     * @example
     * @code
     * // Create horizontal progress bar with 40-unit width
     * auto download = std::make_unique<progress_bar<Backend>>();
     * download->set_orientation(progress_bar_orientation::horizontal);
     * download->set_bar_width(40);
     *
     * // Create vertical progress bar with 15-unit height
     * auto upload = std::make_unique<progress_bar<Backend>>();
     * upload->set_orientation(progress_bar_orientation::vertical);
     * upload->set_bar_width(15);
     * @endcode
     */
    void set_bar_width(int width) {
        if (width < 1) width = 1;

        size_constraint constraint;
        constraint.policy = size_policy::content;
        constraint.preferred_size = logical_unit(static_cast<double>(width));

        // Set the appropriate dimension based on orientation
        if (m_orientation == progress_bar_orientation::horizontal) {
            this->set_width_constraint(constraint);
        } else {
            this->set_height_constraint(constraint);
        }
    }

    // ===== Signals =====

    /// Emitted when value changes
    signal<int> value_changed;

protected:
    /**
     * @brief Calculate natural content size based on orientation
     *
     * @details Uses theme's bar_length and bar_thickness for dimensions.
     */
    [[nodiscard]] logical_size get_content_size() const override {
        auto* theme = ui_services<Backend>::themes()->get_current_theme();
        double const bar_length = theme->progress_bar.bar_length;
        double const bar_thickness = theme->progress_bar.bar_thickness;

        if (m_orientation == progress_bar_orientation::horizontal) {
            return logical_size{logical_unit(bar_length), logical_unit(bar_thickness)};
        } else {
            return logical_size{logical_unit(bar_thickness), logical_unit(bar_length)};
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

        // Get physical position from render context
        const auto& pos = ctx.position();

        int const track_x = point_utils::get_x(pos);
        int const track_y = point_utils::get_y(pos);

        // Get dimensions using get_final_dims pattern (handles measurement vs rendering)
        auto logical_bounds = this->bounds();
        auto const [track_w, track_h] = ctx.get_final_dims(
            logical_bounds.width.to_int(), logical_bounds.height.to_int());

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
        if (m_indeterminate) {
            // Indeterminate mode: fill with empty color (animated in future)
            rect_type empty_rect;
            rect_utils::set_bounds(empty_rect, x, y, width, height);
            ctx.fill_rect(empty_rect, empty_color);
        } else {
            // Determinate mode: render filled and empty portions as rectangles
            int const fill_length = range_helpers::calculate_fill_length(
                m_value, m_min, m_max, width
            );

            // Filled portion (left side)
            if (fill_length > 0) {
                rect_type filled_rect;
                rect_utils::set_bounds(filled_rect, x, y, fill_length, height);
                ctx.fill_rect(filled_rect, filled_color);
            }

            // Empty portion (right side)
            if (fill_length < width) {
                rect_type empty_rect;
                rect_utils::set_bounds(empty_rect, x + fill_length, y, width - fill_length, height);
                ctx.fill_rect(empty_rect, empty_color);
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
        if (m_indeterminate) {
            // Indeterminate mode: fill with empty color (animated in future)
            rect_type empty_rect;
            rect_utils::set_bounds(empty_rect, x, y, width, height);
            ctx.fill_rect(empty_rect, empty_color);
        } else {
            // Determinate mode: render filled portion (bottom to top)
            int const fill_length = range_helpers::calculate_fill_length(
                m_value, m_min, m_max, height
            );

            // Empty portion (top)
            int const empty_height = height - fill_length;
            if (empty_height > 0) {
                rect_type empty_rect;
                rect_utils::set_bounds(empty_rect, x, y, width, empty_height);
                ctx.fill_rect(empty_rect, empty_color);
            }

            // Filled portion (bottom)
            if (fill_length > 0) {
                rect_type filled_rect;
                rect_utils::set_bounds(filled_rect, x, y + empty_height, width, fill_length);
                ctx.fill_rect(filled_rect, filled_color);
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
