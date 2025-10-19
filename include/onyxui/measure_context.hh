/**
 * @file measure_context.hh
 * @brief Concrete render context for measurement (no rendering)
 * @author igor
 * @date 19/10/2025
 */

#pragma once

#include <algorithm>
#include <onyxui/render_context.hh>
#include <onyxui/concepts/size_like.hh>
#include <onyxui/concepts/rect_like.hh>
#include <onyxui/concepts/point_like.hh>
#include <onyxui/utils/safe_math.hh>

namespace onyxui {
    /**
     * @class measure_context
     * @brief Concrete render context that tracks bounds without rendering
     *
     * @tparam Backend The UI backend type
     *
     * @details
     * The measure_context implements the visitor pattern by tracking the
     * bounding box of all "draw" operations without actually rendering anything.
     *
     * ## Behavior
     *
     * - `is_measuring()` returns `true`
     * - `is_rendering()` returns `false`
     * - All draw operations update internal bounds tracking
     * - No actual rendering occurs
     * - `get_size()` returns the final measured size
     *
     * ## Bounds Tracking
     *
     * The context tracks the **maximum extents** of all draw operations:
     * - Text: position + text_size
     * - Rectangles: rect bounds
     * - Lines: line endpoints
     * - Icons: position + icon_size
     *
     * The final size is the bounding box that encompasses all operations.
     *
     * ## Usage
     *
     * @code
     * // In widget's get_content_size() method
     * size_type get_content_size() const override {
     *     measure_context<Backend> ctx;
     *     do_render(ctx);  // Widget "renders" for measurement
     *     return ctx.get_size();
     * }
     * @endcode
     *
     * ## Design Notes
     *
     * The measure_context uses the same draw operations as rendering, but
     * instead of drawing, it calculates and tracks the bounding box. This
     * ensures that measurement and rendering can never get out of sync.
     *
     * @see render_context
     * @see draw_context
     */
    template<UIBackend Backend>
    class measure_context : public render_context<Backend> {
    public:
        using base = render_context<Backend>;
        using size_type = typename base::size_type;
        using rect_type = typename base::rect_type;
        using point_type = typename base::point_type;
        using color_type = typename base::color_type;
        using renderer_type = typename base::renderer_type;
        using font_type = typename base::font_type;
        using box_style = typename base::box_style;
        using icon_type = typename base::icon_type;

        /**
         * @brief Construct measure context
         *
         * @details
         * Initializes with zero size. The size expands as draw operations
         * are performed.
         */
        measure_context() {
            size_utils::set_size(m_measured_size, 0, 0);
        }

        /**
         * @brief Destructor
         */
        ~measure_context() override = default;

        // Prevent copying
        measure_context(const measure_context&) = delete;
        measure_context& operator=(const measure_context&) = delete;

        // Allow moving
        measure_context(measure_context&&) noexcept = default;
        measure_context& operator=(measure_context&&) noexcept = default;

        /**
         * @brief "Draw" text and return its size (measurement only)
         *
         * @details
         * Measures the text size using renderer's static method and updates
         * the tracked bounds. No actual rendering occurs.
         */
        [[nodiscard]] size_type draw_text(
            std::string_view text,
            const point_type& position,
            const font_type& font,
            const color_type& /*color*/
        ) override {
            // Measure text size
            auto text_size = renderer_type::measure_text(std::string(text), font);

            // Calculate extents: position + size
            int x = point_utils::get_x(position);
            int y = point_utils::get_y(position);
            int right = safe_math::add_clamped(x, size_utils::get_width(text_size));
            int bottom = safe_math::add_clamped(y, size_utils::get_height(text_size));

            // Update tracked bounds
            update_bounds(right, bottom);

            return text_size;
        }

        /**
         * @brief "Draw" a rectangle (measurement only)
         *
         * @details
         * Updates tracked bounds to include the rectangle. No actual rendering occurs.
         */
        void draw_rect(
            const rect_type& bounds,
            box_style /*style*/
        ) override {
            // Calculate extents: x + width, y + height
            int right = safe_math::add_clamped(
                rect_utils::get_x(bounds),
                rect_utils::get_width(bounds)
            );
            int bottom = safe_math::add_clamped(
                rect_utils::get_y(bounds),
                rect_utils::get_height(bounds)
            );

            // Update tracked bounds
            update_bounds(right, bottom);
        }

        /**
         * @brief "Draw" a line (measurement only)
         *
         * @details
         * Updates tracked bounds to include both endpoints. No actual rendering occurs.
         */
        void draw_line(
            const point_type& from,
            const point_type& to,
            const color_type& /*color*/,
            int width = 1
        ) override {
            // Track both endpoints
            int x1 = point_utils::get_x(from);
            int y1 = point_utils::get_y(from);
            int x2 = point_utils::get_x(to);
            int y2 = point_utils::get_y(to);

            // Calculate bounds including line width
            int right = std::max(x1, x2) + width;
            int bottom = std::max(y1, y2) + width;

            // Update tracked bounds
            update_bounds(right, bottom);
        }

        /**
         * @brief "Draw" an icon and return its size (measurement only)
         *
         * @details
         * Calculates icon size and updates tracked bounds. No actual rendering occurs.
         *
         * **Note**: Icon size calculation is backend-specific. For now, we assume
         * a default size of 16x16 (typical icon size in TUI/GUI).
         */
        [[nodiscard]] size_type draw_icon(
            const icon_type& /*icon*/,
            const point_type& position
        ) override {
            // TODO: Icon size should come from renderer or theme
            // For now, use typical icon size
            constexpr int default_icon_size = 16;

            size_type icon_size;
            size_utils::set_size(icon_size, default_icon_size, default_icon_size);

            // Calculate extents: position + icon_size
            int right = safe_math::add_clamped(
                point_utils::get_x(position),
                default_icon_size
            );
            int bottom = safe_math::add_clamped(
                point_utils::get_y(position),
                default_icon_size
            );

            // Update tracked bounds
            update_bounds(right, bottom);

            return icon_size;
        }

        /**
         * @brief Check if this context is measuring
         * @return true (measure_context is for measurement)
         */
        [[nodiscard]] bool is_measuring() const noexcept override {
            return true;
        }

        /**
         * @brief Check if this context is rendering
         * @return false (measure_context is for measurement)
         */
        [[nodiscard]] bool is_rendering() const noexcept override {
            return false;
        }

        /**
         * @brief Access underlying renderer
         * @return nullptr (no renderer during measurement)
         */
        [[nodiscard]] renderer_type* renderer() noexcept override {
            return nullptr;
        }

        /**
         * @brief Get the measured size
         * @return The bounding box size of all draw operations
         *
         * @details
         * Returns the size that encompasses all draw operations performed
         * on this context. This is typically called after `do_render(ctx)`
         * to get the widget's content size.
         */
        [[nodiscard]] size_type get_size() const noexcept {
            return m_measured_size;
        }

        /**
         * @brief Reset measured size to zero
         *
         * @details
         * Resets the tracked bounds to zero. Useful if reusing the same
         * context for multiple measurements.
         */
        void reset() noexcept {
            size_utils::set_size(m_measured_size, 0, 0);
        }

    private:
        /**
         * @brief Update tracked bounds with new extents
         *
         * @param right Right edge (x + width)
         * @param bottom Bottom edge (y + height)
         *
         * @details
         * Expands the tracked size to include the new extents. Uses maximum
         * of current and new values.
         */
        void update_bounds(int right, int bottom) noexcept {
            int current_width = size_utils::get_width(m_measured_size);
            int current_height = size_utils::get_height(m_measured_size);

            // Expand to maximum extents
            int new_width = std::max(current_width, right);
            int new_height = std::max(current_height, bottom);

            size_utils::set_size(m_measured_size, new_width, new_height);
        }

        size_type m_measured_size;  ///< Tracked bounding box size
    };
}
