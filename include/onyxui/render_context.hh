/**
 * @file render_context.hh
 * @brief Abstract render context for visitor pattern (measure + render)
 * @author igor
 * @date 19/10/2025
 */

#pragma once

#include <string>
#include <string_view>
#include <onyxui/concepts/backend.hh>

namespace onyxui {
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
         * @brief Check if this context is measuring (not rendering)
         * @return True if measure_context, false otherwise
         *
         * @details
         * Allows widgets to skip expensive calculations during measurement.
         *
         * @example
         * @code
         * if (!ctx.is_measuring()) {
         *     // Only do this during actual rendering
         *     draw_expensive_decoration();
         * }
         * @endcode
         */
        [[nodiscard]] virtual bool is_measuring() const noexcept = 0;

        /**
         * @brief Check if this context is rendering (not measuring)
         * @return True if draw_context, false otherwise
         *
         * @details
         * Convenience method, equivalent to `!is_measuring()`
         */
        [[nodiscard]] virtual bool is_rendering() const noexcept = 0;

        /**
         * @brief Access underlying renderer (for advanced use cases)
         * @return Pointer to renderer, or nullptr if measuring
         *
         * @details
         * Returns nullptr during measurement pass.
         * Use `is_rendering()` to check before accessing.
         *
         * **Warning**: Directly using the renderer bypasses the visitor pattern
         * and may cause measurement/rendering to diverge. Use sparingly.
         */
        [[nodiscard]] virtual renderer_type* renderer() noexcept {
            return nullptr;
        }

    protected:
        /**
         * @brief Protected constructor (abstract class)
         */
        render_context() = default;
    };
}
