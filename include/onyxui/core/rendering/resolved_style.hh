//
// resolved_style.hh - First-class style abstraction for rendering
//
// Created: 2025-10-23 (Theme System Refactoring v2.0)
//

#pragma once

#include <optional>
#include <onyxui/concepts/backend.hh>
#include <onyxui/theming/theme.hh>

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
        // Strong Type Wrappers (Enforce Explicit Initialization)
        // ===================================================================

        /**
         * @brief Strong type for background color - prevents parameter confusion
         * @details Default constructor deleted - must be explicitly initialized
         */
        struct background_color_t {
            color_type value;
            background_color_t() = delete;
            background_color_t(color_type c) : value(c) {}
            operator const color_type&() const { return value; }
        };

        /**
         * @brief Strong type for foreground color - prevents swapping with background
         */
        struct foreground_color_t {
            color_type value;
            foreground_color_t() = delete;
            foreground_color_t(color_type c) : value(c) {}
            operator const color_type&() const { return value; }
        };

        /**
         * @brief Strong type for border color
         */
        struct border_color_t {
            color_type value;
            border_color_t() = delete;
            border_color_t(color_type c) : value(c) {}
            operator const color_type&() const { return value; }
        };

        /**
         * @brief Strong type for box style - backend-specific type
         */
        struct box_style_t {
            box_style_type value;
            box_style_t() = delete;
            box_style_t(box_style_type b) : value(b) {}
            operator const box_style_type&() const { return value; }
        };

        /**
         * @brief Strong type for font - backend-specific type
         */
        struct font_t {
            font_type value;
            font_t() = delete;
            font_t(font_type f) : value(f) {}
            operator const font_type&() const { return value; }
        };

        /**
         * @brief Strong type for opacity
         */
        struct opacity_t {
            float value;
            opacity_t() = delete;
            opacity_t(float o) : value(o) {}
            operator float() const { return value; }
        };

        /**
         * @brief Strong type for icon style - optional but must be explicit
         * @details Even if widget has no icon, must pass std::nullopt explicitly
         */
        struct icon_style_t {
            std::optional<icon_style_type> value;
            icon_style_t() = delete;
            icon_style_t(std::optional<icon_style_type> i) : value(i) {}
            operator const std::optional<icon_style_type>&() const { return value; }
        };

        /**
         * @brief Strong type for horizontal padding - optional, widget-specific
         * @details Used by button, menu_item, menu_bar_item (~37% of widgets)
         */
        struct padding_horizontal_t {
            std::optional<int> value;
            padding_horizontal_t() = delete;
            padding_horizontal_t(std::optional<int> v) : value(v) {}
            operator const std::optional<int>&() const { return value; }
        };

        /**
         * @brief Strong type for vertical padding - optional, widget-specific
         * @details Used by button, menu_item, menu_bar_item (~37% of widgets)
         */
        struct padding_vertical_t {
            std::optional<int> value;
            padding_vertical_t() = delete;
            padding_vertical_t(std::optional<int> v) : value(v) {}
            operator const std::optional<int>&() const { return value; }
        };

        /**
         * @brief Strong type for mnemonic font - optional, widget-specific
         * @details Used by button, label, menu_item, menu_bar_item (~50% of widgets)
         */
        struct mnemonic_font_t {
            std::optional<font_type> value;
            mnemonic_font_t() = delete;
            mnemonic_font_t(std::optional<font_type> f) : value(f) {}
            operator const std::optional<font_type>&() const { return value; }
        };

        // ===================================================================
        // Visual Properties (Inherited, Always Present)
        // ===================================================================

        background_color_t background_color;   ///< Background color (not inherited in CSS)
        foreground_color_t foreground_color;   ///< Text/foreground color (inherited)
        border_color_t border_color;           ///< Border color (inherited)
        box_style_t box_style;                 ///< Border style (not inherited in CSS)
        font_t font;                           ///< Font (inherited)
        opacity_t opacity;                     ///< Opacity 0.0-1.0 (multiplicative, inherited)
        icon_style_t icon_style;               ///< Icon rendering style (inherited, optional)

        // ===================================================================
        // Layout Properties (Widget-Specific, Optional, Not Inherited)
        // ===================================================================

        padding_horizontal_t padding_horizontal; ///< Horizontal padding (widget-specific)
        padding_vertical_t padding_vertical;     ///< Vertical padding (widget-specific)
        mnemonic_font_t mnemonic_font;           ///< Font for mnemonic character (widget-specific)

        // ===================================================================
        // Construction
        // ===================================================================

        /**
         * @brief Aggregate initialization - use designated initializers
         * @details No explicit constructors to maintain aggregate status for C++20 designated initializers
         *
         * @example
         * @code
         * resolved_style style{
         *     .background_color = theme.window_bg,
         *     .foreground_color = theme.text_fg,
         *     // ... all fields required
         * };
         * @endcode
         */

        /**
         * @brief Create initial style from theme (for root element)
         *
         * @param theme Theme to extract base properties from
         * @return Resolved style with theme's default properties
         *
         * @details
         * Uses designated initializers with strong types.
         * Widget-specific properties (box_style, font) use panel defaults.
         * Optional layout properties initialized to std::nullopt (widget-specific).
         */
        static resolved_style from_theme(const ui_theme<Backend>& theme) noexcept {
            return resolved_style{
                .background_color = theme.window_bg,
                .foreground_color = theme.text_fg,
                .border_color = theme.border_color,
                .box_style = theme.panel.box_style,     // Default to panel style
                .font = theme.label.font,                // Default to label font
                .opacity = 1.0F,
                .icon_style = std::optional<icon_style_type>{},
                .padding_horizontal = std::optional<int>{},      // Optional layout properties
                .padding_vertical = std::optional<int>{},
                .mnemonic_font = std::optional<font_type>{}
            };
        }

        // ===================================================================
        // Comparison (for testing)
        // ===================================================================

        /**
         * @brief Equality comparison
         *
         * @note Used primarily in tests to verify style resolution
         * @details Compares the .value members of strong types (including optionals)
         */
        bool operator==(const resolved_style& other) const noexcept {
            return background_color.value == other.background_color.value
                && foreground_color.value == other.foreground_color.value
                && border_color.value == other.border_color.value
                && box_style.value == other.box_style.value
                && font.value == other.font.value
                && opacity.value == other.opacity.value
                && icon_style.value == other.icon_style.value
                && padding_horizontal.value == other.padding_horizontal.value
                && padding_vertical.value == other.padding_vertical.value
                && mnemonic_font.value == other.mnemonic_font.value;
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
            return resolved_style{
                .background_color = background_color.value,
                .foreground_color = foreground_color.value,
                .border_color = border_color.value,
                .box_style = box_style.value,
                .font = font.value,
                .opacity = new_opacity,
                .icon_style = icon_style.value,
                .padding_horizontal = padding_horizontal.value,
                .padding_vertical = padding_vertical.value,
                .mnemonic_font = mnemonic_font.value
            };
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
            return resolved_style{
                .background_color = bg,
                .foreground_color = fg,
                .border_color = border_color.value,
                .box_style = box_style.value,
                .font = font.value,
                .opacity = opacity.value,
                .icon_style = icon_style.value,
                .padding_horizontal = padding_horizontal.value,
                .padding_vertical = padding_vertical.value,
                .mnemonic_font = mnemonic_font.value
            };
        }

        /**
         * @brief Create a copy with modified font
         *
         * @param new_font New font
         * @return New style with adjusted font
         */
        [[nodiscard]] resolved_style with_font(font_type new_font) const noexcept {
            return resolved_style{
                .background_color = background_color.value,
                .foreground_color = foreground_color.value,
                .border_color = border_color.value,
                .box_style = box_style.value,
                .font = new_font,
                .opacity = opacity.value,
                .icon_style = icon_style.value,
                .padding_horizontal = padding_horizontal.value,
                .padding_vertical = padding_vertical.value,
                .mnemonic_font = mnemonic_font.value
            };
        }
    };

} // namespace onyxui
