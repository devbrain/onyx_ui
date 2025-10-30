/**
 * @file render_context.hh
 * @brief Abstract render context for visitor pattern (measure + render)
 * @author igor
 * @date 19/10/2025
 */

#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <onyxui/concepts/backend.hh>
#include <onyxui/core/rendering/resolved_style.hh>

namespace onyxui {
    // Forward declaration
    template<UIBackend Backend>
    struct ui_theme;

    /**
     * @class render_context
     * @brief Abstract visitor base class for widget rendering and measurement
     *
     * @tparam Backend The UI backend type
     *
     * @details
     * The render_context implements the visitor pattern to eliminate duplication
     * between `do_render()` and `get_content_size()` in widgets.
     *
     * ## Design Pattern: Visitor
     *
     * Instead of having two separate methods that duplicate logic:
     * - `get_content_size()` - measures content
     * - `do_render(renderer&)` - renders content
     *
     * Widgets now implement a single method:
     * - `do_render(render_context&)` - both measures AND renders
     *
     * The context type determines the behavior:
     * - `measure_context` - tracks bounds without rendering
     * - `draw_context` - performs actual rendering
     *
     * ## Usage Example
     *
     * @code
     * template<UIBackend Backend>
     * class button : public widget<Backend> {
     *     void do_render(render_context<Backend>& ctx) const override {
     *         // This code works for BOTH measurement and rendering!
     *         auto text_size = ctx.draw_text(m_text, pos, font, color);
     *         ctx.draw_rect(bounds, box_style);
     *
     *         // When measuring: text_size is calculated, nothing rendered
     *         // When rendering: text_size is calculated, text is drawn
     *     }
     * };
     * @endcode
     *
     * ## Primitive Operations
     *
     * All primitive operations return size information:
     * - `draw_text()` - Returns size of text
     * - `draw_rect()` - Draws/measures rectangle
     * - `draw_line()` - Draws/measures line
     * - `draw_icon()` - Returns size of icon
     *
     * This allows widgets to use the size information for layout calculations
     * during BOTH measurement and rendering passes.
     *
     * ## Concrete Implementations
     *
     * - `measure_context` - Tracks bounding box, performs no actual rendering
     * - `draw_context` - Forwards calls to actual renderer
     *
     * ## Benefits
     *
     * 1. **Single source of truth** - Measurement and rendering use same code
     * 2. **Impossible to get out of sync** - No separate implementations
     * 3. **Simpler widgets** - ~50-70% less code
     * 4. **Extensible** - Easy to add new passes (hit testing, accessibility)
     *
     * @see draw_context
     * @see measure_context
     */
    template<UIBackend Backend>
    class render_context {
    public:
        using size_type = typename Backend::size_type;
        using rect_type = typename Backend::rect_type;
        using point_type = typename Backend::point_type;
        using color_type = typename Backend::color_type;
        using renderer_type = typename Backend::renderer_type;

        // Font and box_style types from renderer
        using font_type = typename renderer_type::font;
        using box_style = typename renderer_type::box_style;
        using icon_type = typename renderer_type::icon_style;

        // Theme type
        using theme_type = ui_theme<Backend>;

        /**
         * @brief Virtual destructor
         */
        virtual ~render_context() = default;

        // Prevent copying
        render_context(const render_context&) = delete;
        render_context& operator=(const render_context&) = delete;

        // Allow moving
        render_context(render_context&&) noexcept = default;
        render_context& operator=(render_context&&) noexcept = default;

        /**
         * @brief Draw text and return its size
         *
         * @param text Text to draw
         * @param position Top-left position
         * @param font Font to use
         * @param color Text color
         * @return Size of the rendered text
         *
         * @details
         * - **measure_context**: Calculates text size without rendering
         * - **draw_context**: Draws text and returns size
         *
         * The returned size can be used for further layout calculations.
         */
        [[nodiscard]] virtual size_type draw_text(
            std::string_view text,
            const point_type& position,
            const font_type& font,
            const color_type& color
        ) = 0;

        /**
         * @brief Draw a rectangle with border style
         *
         * @param bounds Rectangle bounds
         * @param style Box style (border type)
         *
         * @details
         * - **measure_context**: Tracks bounds, no rendering
         * - **draw_context**: Draws actual rectangle
         */
        virtual void draw_rect(
            const rect_type& bounds,
            box_style style
        ) = 0;

        /**
         * @brief Fill a rectangle with solid background color
         *
         * @param bounds Rectangle bounds
         *
         * @details
         * Fills the rectangle with the background color from resolved style.
         * No border is drawn - this is pure background fill.
         *
         * - **measure_context**: Tracks bounds, no rendering
         * - **draw_context**: Fills rectangle with background color
         *
         * This is the preferred method for drawing widget backgrounds with
         * state-dependent colors (hover, focus, etc.).
         */
        virtual void fill_rect(const rect_type& bounds) = 0;

        /**
         * @brief Draw a line
         *
         * @param from Start point
         * @param to End point
         * @param color Line color
         * @param width Line width (default: 1)
         *
         * @details
         * - **measure_context**: Tracks line bounds, no rendering
         * - **draw_context**: Draws actual line
         */
        virtual void draw_line(
            const point_type& from,
            const point_type& to,
            const color_type& color,
            int width = 1
        ) = 0;

        /**
         * @brief Draw an icon and return its size
         *
         * @param icon Icon to draw
         * @param position Top-left position
         * @return Size of the icon
         *
         * @details
         * - **measure_context**: Calculates icon size without rendering
         * - **draw_context**: Draws icon and returns size
         */
        [[nodiscard]] virtual size_type draw_icon(
            const icon_type& icon,
            const point_type& position
        ) = 0;

        /**
         * @brief Draw a horizontal separator line
         *
         * @param bounds Rectangle defining line position and extent
         * @param style Line drawing style
         *
         * @details
         * Draws a horizontal line using box-drawing characters. The line is drawn
         * at vertical position bounds.y with width bounds.w starting from bounds.x.
         *
         * - **measure_context**: Tracks line bounds, no rendering
         * - **draw_context**: Draws actual horizontal line
         *
         * Used for menu separators, horizontal dividers, etc.
         */
        virtual void draw_horizontal_line(
            const rect_type& bounds,
            const typename renderer_type::line_style& style
        ) = 0;

        /**
         * @brief Draw a vertical separator line
         *
         * @param bounds Rectangle defining line position and extent
         * @param style Line drawing style
         *
         * @details
         * Draws a vertical line using box-drawing characters. The line is drawn
         * at horizontal position bounds.x with height bounds.h starting from bounds.y.
         *
         * - **measure_context**: Tracks line bounds, no rendering
         * - **draw_context**: Draws actual vertical line
         *
         * Used for vertical separators, column dividers, etc.
         */
        virtual void draw_vertical_line(
            const rect_type& bounds,
            const typename renderer_type::line_style& style
        ) = 0;

        /**
         * @brief Check if context is in rendering mode
         * @return true for draw_context, false for measure_context
         *
         * @details
         * Use this to conditionally execute render-only code paths.
         * Prefer using this over checking renderer() pointer.
         *
         * @example
         * @code
         * void do_render(render_context& ctx) const {
         *     // Common code for both measure and render
         *     auto size = ctx.draw_text(m_text, pos, font, color);
         *
         *     // Render-only code
         *     if (ctx.is_rendering()) {
         *         // Expensive decoration only during rendering
         *         ctx.draw_icon(icon, icon_pos);
         *     }
         * }
         * @endcode
         */
        [[nodiscard]] virtual bool is_rendering() const noexcept {
            return false;  // Base class and measure_context return false
        }

        /**
         * @brief Check if context is in measurement mode
         * @return true for measure_context, false for draw_context
         *
         * @details
         * Convenience method, equivalent to `!is_rendering()`.
         */
        [[nodiscard]] bool is_measuring() const noexcept {
            return !is_rendering();
        }

        /**
         * @brief Push a clipping rectangle onto the renderer's clip stack
         * @param bounds Clipping rectangle
         *
         * @details
         * Restricts rendering to the specified rectangle. All subsequent draw operations
         * will be clipped to this region. Clip regions can be nested (stacked).
         *
         * - **measure_context**: No-op (clipping not needed during measurement)
         * - **draw_context**: Forwards to renderer->push_clip()
         *
         * **Important**: Every push_clip() must have a matching pop_clip()!
         *
         * @example
         * @code
         * void do_render(render_context& ctx) const {
         *     auto viewport = this->get_content_area();
         *     ctx.push_clip(viewport);  // Clip to viewport
         *     // ... render children ...
         *     ctx.pop_clip();  // Restore previous clip
         * }
         * @endcode
         */
        virtual void push_clip(const rect_type& bounds) {
            // Default: no-op (measure_context doesn't clip)
            (void)bounds;
        }

        /**
         * @brief Pop the current clipping rectangle from the renderer's clip stack
         *
         * @details
         * Restores the previous clip region. Must be paired with push_clip().
         *
         * - **measure_context**: No-op
         * - **draw_context**: Forwards to renderer->pop_clip()
         */
        virtual void pop_clip() {
            // Default: no-op (measure_context doesn't clip)
        }

        /**
         * @brief Get the current clip rectangle
         * @return Current clipping rectangle
         *
         * @details
         * Returns the active clipping region, or the full viewport if no clip is active.
         *
         * - **measure_context**: Returns full viewport (no clipping during measurement)
         * - **draw_context**: Forwards to renderer->get_clip_rect()
         *
         * @note For testing purposes - widgets should not typically need this
         */
        virtual rect_type get_clip_rect() const {
            // Default: return full viewport (measure_context)
            return rect_type{0, 0, 0, 0};
        }

    protected:
        /**
         * @brief Access underlying renderer (internal use only)
         * @return Pointer to renderer, or nullptr if measuring
         *
         * @details
         * Protected to prevent widgets from bypassing the visitor pattern.
         * Only draw_context should override this for internal forwarding.
         *
         * Widgets should use render_context methods (draw_text, draw_rect, etc.),
         * not direct renderer access.
         */
        [[nodiscard]] virtual renderer_type* renderer() noexcept {
            return nullptr;
        }

        /**
         * @brief Access underlying renderer (const version, internal use only)
         * @return Const pointer to renderer, or nullptr if measuring
         */
        [[nodiscard]] virtual const renderer_type* renderer() const noexcept {
            return nullptr;
        }

    public:

        // ===================================================================
        // Style Access (v2.0)
        // ===================================================================

        /**
         * @brief Get the resolved style for this rendering pass
         *
         * @return Reference to the immutable resolved style
         *
         * @details
         * The style contains all visual properties resolved through CSS inheritance:
         * - background_color, foreground_color
         * - font, box_style, icon_style
         * - opacity, border_color
         *
         * This style is resolved ONCE before rendering and passed to the context,
         * eliminating the need for repeated get_effective_*() calls.
         *
         * @example
         * @code
         * void do_render(render_context& ctx) const {
         *     auto& style = ctx.style();
         *     ctx.draw_text(m_text, pos, style.font, style.foreground_color);
         *     ctx.draw_rect(bounds, style.box_style);
         * }
         * @endcode
         */
        [[nodiscard]] const resolved_style<Backend>& style() const noexcept {
            return m_style;
        }

        /**
         * @brief Get the top-left position where widget should draw
         *
         * @return Position assigned by parent layout
         *
         * @details
         * The position is set by the parent during arrange() and passed through
         * the render context. This decouples widgets from element state.
         *
         * - **During measurement**: Returns {0, 0} (position irrelevant)
         * - **During rendering**: Returns actual position from bounds
         *
         * @example
         * @code
         * void do_render(render_context& ctx) const {
         *     auto pos = ctx.position();
         *     int x = point_utils::get_x(pos);
         *     int y = point_utils::get_y(pos);
         *     ctx.draw_text(m_text, {x, y}, font, color);
         * }
         * @endcode
         */
        [[nodiscard]] const point_type& position() const noexcept {
            return m_position;
        }

        /**
         * @brief Get the available size assigned by parent layout
         *
         * @return Size constraint from parent (0,0 during measurement)
         *
         * @details
         * The available size is set by the parent during arrange():
         *
         * - **During measurement**: Returns {0, 0} → use natural size
         * - **During rendering**: Returns bounds.size → use assigned size
         *
         * Widgets that can expand (like buttons) use this to respect parent layout:
         * @code
         * void do_render(render_context& ctx) const {
         *     auto avail = ctx.available_size();
         *     int width = size_utils::get_width(avail);
         *
         *     // Use assigned width if available, otherwise calculate natural width
         *     int final_width = (width > 0) ? width : calculate_natural_width();
         *
         *     // Draw at ctx.position() with final_width
         * }
         * @endcode
         */
        [[nodiscard]] const size_type& available_size() const noexcept {
            return m_available_size;
        }

        /**
         * @brief Get the theme pointer for accessing rare widget-specific properties
         *
         * @return Pointer to theme (should not be nullptr in normal usage)
         *
         * @details
         * Provides access to rare widget-specific properties that aren't common enough
         * to include in resolved_style, such as:
         * - button's text_align
         * - separator's line_style
         * - menu_item's separator line_style
         *
         * **Theme Policy**:
         * - Public rendering entry points (ui_element::render) enforce non-null themes
         * - The API uses pointers (not references) for safety and defensive programming
         * - Widgets should defensively check for nullptr, though it should not occur
         *
         * **Note**: Most widgets should use ctx.style() for common properties.
         * Only access theme directly for rare properties specific to your widget type.
         *
         * @example
         * @code
         * void do_render(render_context& ctx) const {
         *     if (auto* theme = ctx.theme()) {
         *         auto text_align = theme->button.text_align;  // Rare property
         *     }
         *     // Use ctx.style() for common properties
         *     ctx.draw_text(m_text, pos, ctx.style().font, ctx.style().foreground_color);
         * }
         * @endcode
         */
        [[nodiscard]] const theme_type* theme() const noexcept {
            return m_theme;
        }

        // ===================================================================
        // Convenience Methods (Using Context Style)
        // ===================================================================

        /**
         * @brief Draw text using context style (foreground color and font)
         *
         * @param text Text to draw
         * @param position Top-left position
         * @return Size of the rendered text
         *
         * @details
         * Convenience method that uses the context's resolved style.
         * Equivalent to: draw_text(text, position, style().font, style().foreground_color)
         */
        [[nodiscard]] size_type draw_text(
            std::string_view text,
            const point_type& position
        ) {
            return draw_text(text, position, m_style.font, m_style.foreground_color);
        }

        /**
         * @brief Draw rectangle using context style (box_style)
         *
         * @param bounds Rectangle bounds
         *
         * @details
         * Convenience method that uses the context's resolved box_style.
         * Equivalent to: draw_rect(bounds, style().box_style)
         */
        void draw_rect(const rect_type& bounds) {
            draw_rect(bounds, m_style.box_style);
        }

        /**
         * @brief Set dirty regions for optimized rendering
         * @param regions List of rectangles that need redrawing
         *
         * @details
         * When dirty regions are set, the context can optimize rendering by:
         * - Skipping widgets that don't intersect with dirty regions
         * - Clipping rendering operations to dirty regions
         *
         * **Usage**:
         * @code
         * draw_context<Backend> ctx(renderer, style);
         * ctx.set_dirty_regions(dirty_rects);
         * widget->do_render(ctx);  // Only renders if intersects dirty regions
         * @endcode
         *
         * **Note**: Empty vector means "render everything" (default behavior)
         */
        virtual void set_dirty_regions(const std::vector<rect_type>& regions) {
            // Default: no-op (measure_context doesn't care about dirty regions)
            (void)regions;
        }

        /**
         * @brief Set dirty regions for optimized rendering (move semantics)
         * @param regions List of rectangles that need redrawing (moved)
         *
         * @details
         * Move overload for performance when caller has a temporary vector.
         * Allows noexcept move instead of potentially throwing copy.
         *
         * **Exception Safety**: No-throw guarantee via move semantics
         */
        virtual void set_dirty_regions(std::vector<rect_type>&& regions) noexcept {
            // Default: no-op (measure_context doesn't care about dirty regions)
            (void)regions;
        }

        /**
         * @brief Check if a rectangle should be rendered
         * @param bounds The rectangle to check
         * @return true if should render, false to skip
         *
         * @details
         * - **No dirty regions**: Returns true (render everything)
         * - **Has dirty regions**: Returns true only if bounds intersects with any dirty region
         *
         * Widgets can call this to optimize their rendering:
         * @code
         * void do_render(render_context& ctx) const {
         *     if (!ctx.should_render(m_bounds)) return;  // Skip if not dirty
         *     // ... render content ...
         * }
         * @endcode
         */
        [[nodiscard]] virtual bool should_render(const rect_type& bounds) const {
            // Default: always render (no optimization)
            (void)bounds;
            return true;
        }

    protected:
        /**
         * @brief Protected constructor with resolved style, position, and size
         *
         * @param style The resolved style for this rendering pass
         * @param position Top-left corner where widget should draw
         * @param available_size Size assigned by parent layout (0,0 during measurement)
         * @param theme Theme pointer for rare widget-specific properties (can be nullptr)
         *
         * @details
         * The style is resolved by the caller (typically ui_element::render())
         * through CSS inheritance before creating the render context.
         *
         * Position and size are extracted from bounds during rendering, or set to
         * {0,0} during measurement.
         */
        explicit render_context(
            const resolved_style<Backend>& style,
            const point_type& position,
            const size_type& available_size,
            const theme_type* theme
        )
            : m_style(style)
            , m_position(position)
            , m_available_size(available_size)
            , m_theme(theme) {
        }

        /**
         * @brief Constructor with resolved style only (zero position/size)
         *
         * @param style The resolved style for this rendering pass
         * @param theme Theme pointer for rare widget-specific properties (can be nullptr)
         *
         * @details
         * Position and size are initialized to zero.
         * Use this for measurement contexts.
         */
        explicit render_context(const resolved_style<Backend>& style, const theme_type* theme)
            : m_style(style)
            , m_theme(theme) {
            // Initialize position and size to zero (measurement defaults)
            size_utils::set_size(m_available_size, 0, 0);
        }

        /**
         * @brief Default constructor (for backward compatibility)
         *
         * @details
         * Creates context with default-valued style, zero position/size.
         * Prefer the style-based constructors for new code.
         */
        render_context()
            : m_style{
                .background_color = typename Backend::color_type{},
                .foreground_color = typename Backend::color_type{},
                .border_color = typename Backend::color_type{},
                .box_style = typename Backend::renderer_type::box_style{},
                .font = typename Backend::renderer_type::font{},
                .opacity = 1.0f,
                .icon_style = std::optional<typename Backend::renderer_type::icon_style>{},
                .padding_horizontal = std::optional<int>{},
                .padding_vertical = std::optional<int>{},
                .mnemonic_font = std::optional<typename Backend::renderer_type::font>{}
            } {
            // Member variables are default-initialized to zero
            size_utils::set_size(m_available_size, 0, 0);
        }

    private:
        resolved_style<Backend> m_style;        ///< Resolved visual style for this pass
        point_type m_position{};                ///< Top-left position where widget draws
        size_type m_available_size{};           ///< Size assigned by parent (0,0 during measurement)
        const theme_type* m_theme = nullptr;    ///< Theme pointer for rare widget-specific properties
    };
}
