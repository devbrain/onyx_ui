/**
 * @file measure_context.hh
 * @brief Concrete render context for measurement (no rendering)
 * @author igor
 * @date 19/10/2025
 */

#pragma once

#include <algorithm>
#include <onyxui/core/rendering/render_context.hh>
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
     * - All draw operations update internal bounds tracking
     * - No actual rendering occurs
     * - `get_size()` returns the final measured size
     * - `renderer()` returns nullptr (no renderer during measurement)
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
         * @brief Construct measure context with resolved style
         * @param style Resolved visual style (ensures measurement matches rendering)
         *
         * @details
         * Initializes bounding box tracking. The bounding box expands as draw
         * operations are performed, tracking min/max extents to calculate final size.
         *
         * IMPORTANT: measure_context MUST be constructed with a resolved_style to
         * ensure measurement uses the same theme properties (padding, fonts, etc.)
         * as rendering. This prevents measurement/rendering inconsistencies.
         */
        explicit measure_context(const resolved_style<Backend>& style)
            : base(style, nullptr)
            , m_min_x(INT_MAX)
            , m_min_y(INT_MAX)
            , m_max_right(INT_MIN)
            , m_max_bottom(INT_MIN)
            , m_has_content(false) {
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
         * the tracked bounding box. No actual rendering occurs.
         * Position is tracked to calculate the bounding box of all content.
         */
        [[nodiscard]] size_type draw_text(
            std::string_view text,
            const point_type& position,
            const font_type& font,
            const color_type& /*color*/
        ) override {
            // Measure text size
            auto text_size = renderer_type::measure_text(text, font);

            // Calculate bounding box extents
            int const x = point_utils::get_x(position);
            int const y = point_utils::get_y(position);
            int const w = size_utils::get_width(text_size);
            int const h = size_utils::get_height(text_size);
            int const right = safe_math::add_clamped(x, w);
            int const bottom = safe_math::add_clamped(y, h);

            // Update bounding box
            update_bounding_box(x, y, right, bottom);

            return text_size;
        }

        /**
         * @brief "Draw" a rectangle (measurement only)
         *
         * @details
         * Updates tracked bounding box to include the rectangle. No actual rendering occurs.
         */
        void draw_rect(
            const rect_type& bounds,
            box_style /*style*/
        ) override {
            // Calculate bounding box extents
            int const x = rect_utils::get_x(bounds);
            int const y = rect_utils::get_y(bounds);
            int const w = rect_utils::get_width(bounds);
            int const h = rect_utils::get_height(bounds);
            int const right = safe_math::add_clamped(x, w);
            int const bottom = safe_math::add_clamped(y, h);

            // Update bounding box
            update_bounding_box(x, y, right, bottom);
        }

        /**
         * @brief "Fill" a rectangle (measurement only)
         *
         * @details
         * Tracks the rectangle bounds for measurement.
         * No actual rendering occurs.
         */
        void fill_rect(const rect_type& bounds) override {
            // Same as draw_rect for measurement purposes
            draw_rect(bounds, box_style{});
        }

        /**
         * @brief "Draw" a line (measurement only)
         *
         * @details
         * Updates tracked bounding box to include both line endpoints plus width.
         * No actual rendering occurs.
         */
        void draw_line(
            const point_type& from,
            const point_type& to,
            const color_type& /*color*/,
            int width = 1
        ) override {
            // Track both endpoints
            int const x1 = point_utils::get_x(from);
            int const y1 = point_utils::get_y(from);
            int const x2 = point_utils::get_x(to);
            int const y2 = point_utils::get_y(to);

            // Calculate bounding box including line width
            int const min_x = std::min(x1, x2);
            int const min_y = std::min(y1, y2);
            int const max_x = std::max(x1, x2) + width;
            int const max_y = std::max(y1, y2) + width;

            // Update bounding box
            update_bounding_box(min_x, min_y, max_x, max_y);
        }

        /**
         * @brief "Draw" an icon and return its size (measurement only)
         *
         * @details
         * Calculates icon size from renderer and updates tracked bounding box.
         * No actual rendering occurs. Icon size is queried from the renderer's
         * static get_icon_size() method, allowing backend-specific sizing.
         */
        [[nodiscard]] size_type draw_icon(
            const icon_type& icon,
            const point_type& position
        ) override {
            // Get icon size from renderer (static method - no instance needed)
            size_type icon_size = renderer_type::get_icon_size(icon);

            int const x = point_utils::get_x(position);
            int const y = point_utils::get_y(position);
            int const w = size_utils::get_width(icon_size);
            int const h = size_utils::get_height(icon_size);
            int const right = safe_math::add_clamped(x, w);
            int const bottom = safe_math::add_clamped(y, h);

            // Update bounding box
            update_bounding_box(x, y, right, bottom);

            return icon_size;
        }

        /**
         * @brief Draw horizontal line (measurement only - tracks bounds)
         * @param bounds Line bounds
         * @param style Line style (unused during measurement)
         */
        void draw_horizontal_line(
            const rect_type& bounds,
            [[maybe_unused]] const typename renderer_type::line_style& style
        ) override {
            // During measurement, just track the bounding box
            int const x = rect_utils::get_x(bounds);
            int const y = rect_utils::get_y(bounds);
            int const w = rect_utils::get_width(bounds);
            int const h = rect_utils::get_height(bounds);
            int const right = safe_math::add_clamped(x, w);
            int const bottom = safe_math::add_clamped(y, h);

            update_bounding_box(x, y, right, bottom);
        }

        /**
         * @brief Draw vertical line (measurement only - tracks bounds)
         * @param bounds Line bounds
         * @param style Line style (unused during measurement)
         */
        void draw_vertical_line(
            const rect_type& bounds,
            [[maybe_unused]] const typename renderer_type::line_style& style
        ) override {
            // During measurement, just track the bounding box
            int const x = rect_utils::get_x(bounds);
            int const y = rect_utils::get_y(bounds);
            int const w = rect_utils::get_width(bounds);
            int const h = rect_utils::get_height(bounds);
            int const right = safe_math::add_clamped(x, w);
            int const bottom = safe_math::add_clamped(y, h);

            update_bounding_box(x, y, right, bottom);
        }

        /**
         * @brief Draw shadow (no-op during measurement)
         * @param widget_bounds Widget bounds (unused)
         * @param offset_x Horizontal offset (unused)
         * @param offset_y Vertical offset (unused)
         *
         * @details
         * Shadows don't affect widget measurement, so this is a no-op.
         */
        void draw_shadow(
            [[maybe_unused]] const rect_type& widget_bounds,
            [[maybe_unused]] int offset_x,
            [[maybe_unused]] int offset_y
        ) override {
            // No-op: shadows don't affect measurement
        }

        /**
         * @brief Access underlying renderer
         * @return nullptr (no renderer during measurement)
         */
        [[nodiscard]] renderer_type* renderer() noexcept override {
            return nullptr;
        }

        /**
         * @brief Access underlying renderer (const version)
         * @return nullptr (no renderer during measurement)
         */
        [[nodiscard]] const renderer_type* renderer() const noexcept override {
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
         *
         * The size is calculated from the bounding box: (max - min) for both dimensions.
         * If no content was drawn, returns zero size.
         */
        [[nodiscard]] logical_size get_size() const noexcept {
            if (m_has_content) {
                int const width = m_max_right - m_min_x;
                int const height = m_max_bottom - m_min_y;
                return logical_size{logical_unit(static_cast<double>(width)),
                                   logical_unit(static_cast<double>(height))};
            } else {
                return logical_size{logical_unit(0.0), logical_unit(0.0)};
            }
        }

        /**
         * @brief Reset measured size to zero
         *
         * @details
         * Resets the tracked bounding box. Useful if reusing the same
         * context for multiple measurements.
         */
        void reset() noexcept {
            m_min_x = INT_MAX;
            m_min_y = INT_MAX;
            m_max_right = INT_MIN;
            m_max_bottom = INT_MIN;
            m_has_content = false;
        }

    private:
        /**
         * @brief Update tracked bounding box with new extents
         *
         * @param left Left edge (x position)
         * @param top Top edge (y position)
         * @param right Right edge (x + width)
         * @param bottom Bottom edge (y + height)
         *
         * @details
         * Expands the tracked bounding box to include the new rectangle.
         * Tracks minimum and maximum extents across all draw operations.
         * This allows correct measurement regardless of draw position.
         */
        void update_bounding_box(int left, int top, int right, int bottom) noexcept {
            m_min_x = std::min(m_min_x, left);
            m_min_y = std::min(m_min_y, top);
            m_max_right = std::max(m_max_right, right);
            m_max_bottom = std::max(m_max_bottom, bottom);
            m_has_content = true;
        }

        int m_min_x;         ///< Minimum x coordinate seen
        int m_min_y;         ///< Minimum y coordinate seen
        int m_max_right;     ///< Maximum right edge (x + width)
        int m_max_bottom;    ///< Maximum bottom edge (y + height)
        bool m_has_content;  ///< Whether any content was drawn
    };
}
