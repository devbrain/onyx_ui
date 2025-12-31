/**
 * @file draw_context.hh
 * @brief Concrete render context for actual rendering
 * @author igor
 * @date 19/10/2025
 */

#pragma once

#include <iostream>
#include <vector>
#include <onyxui/core/rendering/render_context.hh>
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
     * - All draw operations forward to the renderer
     * - Size information is obtained by calling renderer's measurement methods
     * - `renderer()` returns valid pointer to renderer
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
         * @brief Construct draw context with renderer, style, position, and size
         * @param renderer Reference to the actual renderer
         * @param style Resolved visual style for this rendering pass
         * @param position Top-left corner where widget should draw
         * @param available_size Size assigned by parent layout
         *
         * @details
         * This is the preferred constructor that fully supports the visitor pattern.
         * Position and size are passed from parent's arrange() to decouple widgets
         * from element state.
         */
        explicit draw_context(
            renderer_type& renderer,
            const resolved_style<Backend>& style,
            const point_type& position,
            const size_type& available_size
        )
            : base(style, position, available_size, nullptr)
            , m_renderer(&renderer) {
        }

        /**
         * @brief Construct draw context with renderer and resolved style
         * @param renderer Reference to the actual renderer
         * @param style Resolved visual style for this rendering pass
         *
         * @details
         * The renderer reference must remain valid for the lifetime of this context.
         * Typically, the draw_context is stack-allocated and short-lived.
         *
         * The style is resolved by the caller (ui_element::render()) through
         * CSS inheritance before creating the context.
         *
         * Position and size default to zero - prefer the full constructor for
         * proper visitor pattern support.
         */
        explicit draw_context(renderer_type& renderer, const resolved_style<Backend>& style)
            : base(style, nullptr)
            , m_renderer(&renderer) {}

        /**
         * @brief Construct draw context with renderer only (backward compatibility)
         * @param renderer Reference to the actual renderer
         *
         * @details
         * Creates context with default-constructed style.
         * Prefer the style-based constructor for new code.
         */
        explicit draw_context(renderer_type& renderer)
            : m_renderer(&renderer) {}

        /**
         * @brief Construct draw context with renderer, style, position, size, and dirty regions
         * @param renderer Reference to the actual renderer
         * @param style Resolved visual style for this rendering pass
         * @param position Top-left corner where widget should draw
         * @param available_size Size assigned by parent layout
         * @param dirty_regions List of rectangles that need redrawing
         * @param theme Optional theme pointer for rare widget-specific properties (nullptr if none)
         *
         * @throws std::bad_alloc if dirty_regions copy fails
         *
         * @details
         * This is the complete constructor with all parameters for full visitor pattern support.
         * Position and size decouple widgets from element state, and dirty regions enable
         * optimized incremental rendering. The theme pointer allows widgets to access rare
         * properties not included in resolved_style (e.g., text_align, line_style).
         */
        draw_context(
            renderer_type& renderer,
            const resolved_style<Backend>& style,
            const point_type& position,
            const size_type& available_size,
            const std::vector<rect_type>& dirty_regions,
            const typename base::theme_type* theme
        )
            : base(style, position, available_size, theme)
            , m_renderer(&renderer)
            , m_dirty_regions(dirty_regions) {
        }

        /**
         * @brief Construct draw context with renderer, style, and dirty regions
         * @param renderer Reference to the actual renderer
         * @param style Resolved visual style for this rendering pass
         * @param dirty_regions List of rectangles that need redrawing
         *
         * @throws std::bad_alloc if dirty_regions copy fails
         *
         * @details
         * When dirty regions are provided, only widgets that intersect with
         * these regions will be rendered, optimizing performance.
         *
         * Position and size default to zero - prefer the full constructor.
         *
         * **Exception Safety**: Basic guarantee
         * - If construction fails, no resources are leaked
         * - Already-constructed members are destructed properly
         * - Caller must handle exception and use fallback rendering strategy
         */
        draw_context(renderer_type& renderer, const resolved_style<Backend>& style, const std::vector<rect_type>& dirty_regions)
            : base(style, nullptr)
            , m_renderer(&renderer)
            , m_dirty_regions(dirty_regions) {}

        /**
         * @brief Construct draw context with renderer and dirty regions (backward compatibility)
         * @param renderer Reference to the actual renderer
         * @param dirty_regions List of rectangles that need redrawing
         *
         * @throws std::bad_alloc if dirty_regions copy fails
         *
         * @details
         * Creates context with default-constructed style.
         * Prefer the style-based constructor for new code.
         *
         * **Exception Safety**: Basic guarantee
         * - If construction fails, no resources are leaked
         * - Caller must handle exception and use fallback rendering strategy
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
            auto text_size = renderer_type::measure_text(text, font);

            // Create a rect for the text at the specified position
            rect_type text_rect;
            rect_utils::set_bounds(text_rect,
                                  point_utils::get_x(position),
                                  point_utils::get_y(position),
                                  size_utils::get_width(text_size),
                                  size_utils::get_height(text_size));

            // Pass colors directly to renderer (stateless!)
            m_renderer->draw_text(text_rect, text, font, color, this->style().background_color);

            return text_size;
        }

        /**
         * @brief Draw a rectangle with border style
         *
         * @details
         * Forwards to renderer's draw_box() method.
         * The rectangle is actually rendered to the screen/buffer.
         * Passes foreground and background colors from resolved style.
         */
        void draw_rect(
            const rect_type& bounds,
            box_style style
        ) override {
            // Pass colors directly to renderer (stateless!)
            m_renderer->draw_box(bounds, style, this->style().foreground_color, this->style().background_color);
        }

        /**
         * @brief Fill a rectangle with solid background color
         *
         * @details
         * Fills the rectangle with background color from resolved style.
         * Creates a box_style with no border and solid fill, then calls draw_box().
         */
        void fill_rect(const rect_type& bounds) override {
            // Get background color from resolved style
            auto const& bg = this->style().background_color;

            // Create fill-only style (no border, solid fill)
            box_style fill_style{};  // Default: border_style::none, is_solid=true

            // Pass colors directly to renderer (stateless!)
            m_renderer->draw_box(bounds, fill_style, bg, bg);
        }

        /**
         * @brief Fill a rectangle with a specific color
         */
        void fill_rect(const rect_type& bounds, const color_type& color) override {
            // Create fill-only style (no border, solid fill)
            box_style fill_style{};  // Default: border_style::none, is_solid=true

            // Pass color directly to renderer
            m_renderer->draw_box(bounds, fill_style, color, color);
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
            // Get icon size to construct bounds
            auto icon_size = renderer_type::get_icon_size(icon);

            // Construct rect from position and size
            rect_type icon_bounds;
            rect_utils::set_bounds(
                icon_bounds,
                point_utils::get_x(position),
                point_utils::get_y(position),
                size_utils::get_width(icon_size),
                size_utils::get_height(icon_size)
            );

            // Pass colors directly to renderer (stateless!)
            m_renderer->draw_icon(icon_bounds, icon, this->style().foreground_color, this->style().background_color);
            return icon_size;
        }

        /**
         * @brief Draw horizontal separator line
         * @param bounds Line bounds (y and height define position, x and w define extent)
         * @param style Line drawing style
         *
         * @details
         * Passes colors from resolved style directly to renderer.
         * No state management needed - fully stateless!
         */
        void draw_horizontal_line(
            const rect_type& bounds,
            const typename renderer_type::line_style& style
        ) override {
            // Pass colors directly to renderer (stateless!)
            m_renderer->draw_horizontal_line(bounds, style, this->style().foreground_color, this->style().background_color);
        }

        /**
         * @brief Draw vertical separator line
         * @param bounds Line bounds (x and width define position, y and h define extent)
         * @param style Line drawing style
         *
         * @details
         * Passes colors from resolved style directly to renderer.
         * No state management needed - fully stateless!
         */
        void draw_vertical_line(
            const rect_type& bounds,
            const typename renderer_type::line_style& style
        ) override {
            // Pass colors directly to renderer (stateless!)
            m_renderer->draw_vertical_line(bounds, style, this->style().foreground_color, this->style().background_color);
        }

        /**
         * @brief Draw shadow for popup elements
         * @param widget_bounds Bounds of widget casting shadow
         * @param offset_x Horizontal shadow offset (cells right)
         * @param offset_y Vertical shadow offset (cells down)
         *
         * @details
         * Delegates to renderer for backend-specific shadow rendering.
         * Backend determines darkening factor, shading pattern, etc.
         */
        void draw_shadow(
            const rect_type& widget_bounds,
            int offset_x,
            int offset_y
        ) override {
            m_renderer->draw_shadow(widget_bounds, offset_x, offset_y);
        }

        /**
         * @brief Check if context is rendering
         * @return true (draw_context is always rendering)
         */
        [[nodiscard]] bool is_rendering() const noexcept override {
            return true;
        }

    protected:
        /**
         * @brief Access underlying renderer (internal use only)
         * @return Pointer to the renderer
         */
        [[nodiscard]] renderer_type* renderer() noexcept override {
            return m_renderer;
        }

        /**
         * @brief Access underlying renderer (const version, internal use only)
         * @return Const pointer to the renderer
         */
        [[nodiscard]] const renderer_type* renderer() const noexcept override {
            return m_renderer;
        }

    public:

        /**
         * @brief Push a clipping rectangle onto the renderer's clip stack
         * @param bounds Clipping rectangle
         *
         * @details
         * Forwards to renderer->push_clip(). Restricts all subsequent drawing
         * operations to the specified rectangle.
         */
        void push_clip(const rect_type& bounds) override {
            if (m_renderer) {
                m_renderer->push_clip(bounds);
            }
        }

        /**
         * @brief Pop the current clipping rectangle from the renderer's clip stack
         *
         * @details
         * Forwards to renderer->pop_clip(). Restores the previous clip region.
         */
        void pop_clip() override {
            if (m_renderer) {
                m_renderer->pop_clip();
            }
        }

        /**
         * @brief Get the current clip rectangle
         * @return Current clipping rectangle from renderer
         *
         * @details
         * Forwards to renderer->get_clip_rect(). Returns the active clipping region.
         */
        rect_type get_clip_rect() const override {
            if (m_renderer) {
                return m_renderer->get_clip_rect();
            }
            return rect_type{0, 0, 0, 0};
        }

        /**
         * @brief Set dirty regions for optimized rendering
         * @param regions List of rectangles that need redrawing
         *
         * @throws std::bad_alloc if vector copy fails
         *
         * @details
         * **Exception Safety**: Basic guarantee
         * - If copy fails, old dirty regions are preserved
         * - Context remains in valid state
         */
        void set_dirty_regions(const std::vector<rect_type>& regions) override {
            m_dirty_regions = regions;
        }

        /**
         * @brief Set dirty regions for optimized rendering (move semantics)
         * @param regions List of rectangles that need redrawing (moved)
         *
         * @details
         * Move overload for performance when caller has a temporary vector.
         * Provides noexcept guarantee by using move semantics.
         *
         * **Exception Safety**: No-throw guarantee
         */
        void set_dirty_regions(std::vector<rect_type>&& regions) noexcept override {
            m_dirty_regions = std::move(regions);
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
