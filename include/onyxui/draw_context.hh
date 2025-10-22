/**
 * @file draw_context.hh
 * @brief Concrete render context for actual rendering
 * @author igor
 * @date 19/10/2025
 */

#pragma once

#include <vector>
#include <onyxui/render_context.hh>
#include <onyxui/concepts/size_like.hh>
#include <onyxui/concepts/rect_like.hh>
#include <onyxui/concepts/point_like.hh>

namespace onyxui {
    /**
     * @class draw_context
     * @brief Concrete render context that forwards to actual renderer
     *
     * @tparam Backend The UI backend type
     *
     * @details
     * The draw_context implements the visitor pattern by forwarding all
     * rendering operations to the underlying renderer.
     *
     * ## Behavior
     *
     * - `is_measuring()` returns `false`
     * - `is_rendering()` returns `true`
     * - All draw operations forward to the renderer
     * - Size information is obtained by calling renderer's measurement methods
     *
     * ## Usage
     *
     * @code
     * // In widget's render_internal() method
     * void render_internal(renderer_type& r) {
     *     draw_context<Backend> ctx(r);
     *     do_render(ctx);  // Widget renders to actual renderer
     * }
     * @endcode
     *
     * ## Design Notes
     *
     * The draw_context acts as an adapter between the render_context interface
     * and the backend-specific renderer. It ensures that all rendering operations
     * go through the visitor pattern while maintaining compatibility with
     * existing renderer implementations.
     *
     * @see render_context
     * @see measure_context
     */
    template<UIBackend Backend>
    class draw_context : public render_context<Backend> {
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
         * @brief Construct draw context with renderer
         * @param renderer Reference to the actual renderer
         *
         * @details
         * The renderer reference must remain valid for the lifetime of this context.
         * Typically, the draw_context is stack-allocated and short-lived.
         */
        explicit draw_context(renderer_type& renderer)
            : m_renderer(&renderer) {}

        /**
         * @brief Construct draw context with renderer and dirty regions
         * @param renderer Reference to the actual renderer
         * @param dirty_regions List of rectangles that need redrawing
         *
         * @details
         * When dirty regions are provided, only widgets that intersect with
         * these regions will be rendered, optimizing performance.
         */
        draw_context(renderer_type& renderer, const std::vector<rect_type>& dirty_regions)
            : m_renderer(&renderer), m_dirty_regions(dirty_regions) {}

        /**
         * @brief Destructor
         */
        ~draw_context() override = default;

        // Prevent copying
        draw_context(const draw_context&) = delete;
        draw_context& operator=(const draw_context&) = delete;

        // Allow moving
        draw_context(draw_context&&) noexcept = default;
        draw_context& operator=(draw_context&&) noexcept = default;

        /**
         * @brief Draw text and return its size
         *
         * @details
         * Forwards to renderer's draw_text() method and returns the text size.
         * The text is actually rendered to the screen/buffer.
         */
        [[nodiscard]] size_type draw_text(
            std::string_view text,
            const point_type& position,
            const font_type& font,
            const color_type& color
        ) override {
            // First, measure the text to get its size
            auto text_size = renderer_type::measure_text(std::string(text), font);

            // Create a rect for the text at the specified position
            rect_type text_rect;
            rect_utils::set_bounds(text_rect,
                                  point_utils::get_x(position),
                                  point_utils::get_y(position),
                                  size_utils::get_width(text_size),
                                  size_utils::get_height(text_size));

            // Set colors and draw
            m_renderer->set_foreground(color);
            m_renderer->draw_text(text_rect, std::string(text), font);

            return text_size;
        }

        /**
         * @brief Draw a rectangle with border style
         *
         * @details
         * Forwards to renderer's draw_box() method.
         * The rectangle is actually rendered to the screen/buffer.
         */
        void draw_rect(
            const rect_type& bounds,
            box_style style
        ) override {
            m_renderer->draw_box(bounds, style);
        }

        /**
         * @brief Draw a line
         *
         * @details
         * Forwards to renderer's draw_line() method (if available).
         * The line is actually rendered to the screen/buffer.
         *
         * **Note**: Not all renderers support draw_line(). This is a placeholder
         * that may need backend-specific implementation.
         */
        void draw_line(
            const point_type& from,
            const point_type& to,
            const color_type& color,
            int width = 1
        ) override {
            // TODO: Most renderers don't have draw_line yet
            // This is a placeholder for future implementation
            // For now, we can simulate with draw_box if needed
            (void)from;
            (void)to;
            (void)color;
            (void)width;
        }

        /**
         * @brief Draw an icon and return its size
         *
         * @details
         * Forwards to renderer's draw_icon() method (if available).
         * The icon is actually rendered to the screen/buffer.
         *
         * **Note**: Not all renderers support icons yet. This is a placeholder
         * that may need backend-specific implementation.
         */
        [[nodiscard]] size_type draw_icon(
            const icon_type& icon,
            const point_type& position
        ) override {
            // TODO: Icon rendering not yet implemented in most renderers
            // This is a placeholder for future implementation
            (void)icon;
            (void)position;

            // Return minimal size for now
            size_type size{};
            size_utils::set_size(size, 1, 1);
            return size;
        }

        /**
         * @brief Check if this context is measuring
         * @return false (draw_context is for rendering)
         */
        [[nodiscard]] bool is_measuring() const noexcept override {
            return false;
        }

        /**
         * @brief Check if this context is rendering
         * @return true (draw_context is for rendering)
         */
        [[nodiscard]] bool is_rendering() const noexcept override {
            return true;
        }

        /**
         * @brief Access underlying renderer
         * @return Pointer to the renderer
         */
        [[nodiscard]] renderer_type* renderer() noexcept override {
            return m_renderer;
        }

        /**
         * @brief Set dirty regions for optimized rendering
         * @param regions List of rectangles that need redrawing
         */
        void set_dirty_regions(const std::vector<rect_type>& regions) override {
            m_dirty_regions = regions;
        }

        /**
         * @brief Check if a rectangle should be rendered
         * @param bounds The rectangle to check
         * @return true if should render, false to skip
         *
         * @details
         * - If no dirty regions are set: Returns true (render everything)
         * - If dirty regions are set: Returns true only if bounds intersects
         *   with at least one dirty region
         */
        [[nodiscard]] bool should_render(const rect_type& bounds) const override {
            // No dirty regions means render everything
            if (m_dirty_regions.empty()) {
                return true;
            }

            // Check if bounds intersect with any dirty region
            for (const auto& dirty_rect : m_dirty_regions) {
                if (rects_intersect(bounds, dirty_rect)) {
                    return true;
                }
            }

            // No intersection with any dirty region - skip rendering
            return false;
        }

    private:
        /**
         * @brief Check if two rectangles intersect
         * @param a First rectangle
         * @param b Second rectangle
         * @return true if rectangles overlap
         */
        [[nodiscard]] static bool rects_intersect(const rect_type& a, const rect_type& b) {
            int const a_left = rect_utils::get_x(a);
            int const a_top = rect_utils::get_y(a);
            int const a_right = a_left + rect_utils::get_width(a);
            int const a_bottom = a_top + rect_utils::get_height(a);

            int const b_left = rect_utils::get_x(b);
            int const b_top = rect_utils::get_y(b);
            int const b_right = b_left + rect_utils::get_width(b);
            int const b_bottom = b_top + rect_utils::get_height(b);

            // Check for non-intersection (easier to reason about)
            // Rectangles DON'T intersect if one is completely to the side of the other
            return !(a_right <= b_left ||   // a is completely left of b
                    b_right <= a_left ||    // b is completely left of a
                    a_bottom <= b_top ||    // a is completely above b
                    b_bottom <= a_top);     // b is completely above a
        }

        renderer_type* m_renderer;  ///< Non-owning pointer to renderer
        std::vector<rect_type> m_dirty_regions;  ///< Dirty regions for optimized rendering
    };
}
