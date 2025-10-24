//
// resolved_style.hh - First-class style abstraction for rendering
//
// Created: 2025-10-23 (Theme System Refactoring v2.0)
//

#pragma once

#include <onyxui/concepts/backend.hh>

namespace onyxui {

    /**
     * @struct resolved_style
     * @brief Fully-resolved visual properties for rendering
     *
     * @details
     * This structure represents a widget's complete visual style after:
     * 1. CSS-style inheritance from parents
     * 2. Theme application
     * 3. Property overrides
     *
     * ## Design Rationale
     *
     * Style resolution happens ONCE before rendering (in ui_element::render()),
     * not repeatedly during rendering. This:
     * - Eliminates repeated get_effective_*() calls
     * - Simplifies widget rendering code
     * - Improves performance (resolution is O(tree_height), rendering is O(1))
     * - Provides immutable style contract to render_context
     *
     * ## Usage Pattern
     *
     * @code
     * // In ui_element::render()
     * auto style = this->resolve_style();  // Resolve CSS inheritance ONCE
     * draw_context ctx(renderer, style);   // Pass resolved style
     * this->do_render(ctx);                // Widget just draws!
     * @endcode
     *
     * ## Memory Layout
     *
     * This is a simple POD-like structure designed for:
     * - Fast stack allocation
     * - Efficient passing by value
     * - No heap allocations
     * - Predictable memory layout
     *
     * @tparam Backend The UI backend type
     */
    template<UIBackend Backend>
    struct resolved_style {
        using color_type = typename Backend::color_type;
        using box_style_type = typename Backend::renderer_type::box_style;
        using font_type = typename Backend::renderer_type::font;
        using icon_style_type = typename Backend::renderer_type::icon_style;

        // ===================================================================
        // Visual Properties (All Resolved)
        // ===================================================================

        color_type background_color{};   ///< Background color (inherited)
        color_type foreground_color{};   ///< Text/foreground color (inherited)
        color_type border_color{};       ///< Border color (if applicable)
        box_style_type box_style{};      ///< Border style (inherited)
        font_type font{};                ///< Font (inherited)
        float opacity = 1.0F;            ///< Opacity 0.0-1.0 (multiplicative)
        icon_style_type icon_style{};    ///< Icon rendering style (if applicable)

        // ===================================================================
        // Construction
        // ===================================================================

        /**
         * @brief Default constructor (zero-initialized)
         */
        resolved_style() = default;

        /**
         * @brief Construct from individual properties
         *
         * @note All parameters have sensible defaults for flexibility
         */
        explicit resolved_style(
            color_type bg,
            color_type fg,
            color_type border = color_type{},
            box_style_type box = box_style_type{},
            font_type f = font_type{},
            float op = 1.0F,
            icon_style_type icon = icon_style_type{}
        ) noexcept
            : background_color(bg)
            , foreground_color(fg)
            , border_color(border)
            , box_style(box)
            , font(f)
            , opacity(op)
            , icon_style(icon)
        {}

        // ===================================================================
        // Comparison (for testing)
        // ===================================================================

        /**
         * @brief Equality comparison
         *
         * @note Used primarily in tests to verify style resolution
         */
        bool operator==(const resolved_style& other) const noexcept {
            return background_color == other.background_color
                && foreground_color == other.foreground_color
                && border_color == other.border_color
                && box_style == other.box_style
                && font == other.font
                && opacity == other.opacity
                && icon_style == other.icon_style;
        }

        bool operator!=(const resolved_style& other) const noexcept {
            return !(*this == other);
        }

        // ===================================================================
        // Utility Methods
        // ===================================================================

        /**
         * @brief Create a copy with modified opacity
         *
         * @details Useful for dimming disabled widgets
         * @param new_opacity New opacity value (0.0-1.0)
         * @return New style with adjusted opacity
         */
        [[nodiscard]] resolved_style with_opacity(float new_opacity) const noexcept {
            resolved_style result = *this;
            result.opacity = new_opacity;
            return result;
        }

        /**
         * @brief Create a copy with modified colors
         *
         * @details Useful for state-based styling (hover, pressed)
         * @param bg New background color
         * @param fg New foreground color
         * @return New style with adjusted colors
         */
        [[nodiscard]] resolved_style with_colors(color_type bg, color_type fg) const noexcept {
            resolved_style result = *this;
            result.background_color = bg;
            result.foreground_color = fg;
            return result;
        }

        /**
         * @brief Create a copy with modified font
         *
         * @param new_font New font
         * @return New style with adjusted font
         */
        [[nodiscard]] resolved_style with_font(font_type new_font) const noexcept {
            resolved_style result = *this;
            result.font = new_font;
            return result;
        }
    };

} // namespace onyxui
