//
// Created by igor on 20/10/2025.
//

#pragma once

#include <cstdint>
#include <string_view>
#include <memory>

#include <onyxui/conio/geometry.hh>
#include <onyxui/conio/colors.hh>
#include <onyxui/backends/conio/onyxui_conio_export.h>

#include <termbox2.h>

namespace onyxui::conio {
    class vram;
    /**
    * @class conio_renderer
    * @brief TUI renderer using vram as low-level drawing library
    *
    * This renderer uses the vram class for efficient character-based
    * rendering with clipping support. It provides all required methods
    * for the RenderLike concept.
    */
    class ONYXUI_CONIO_EXPORT conio_renderer {
        public:
            // ===================================================================
            // Required Types for RenderLike Concept
            // ===================================================================

            /**
             * @enum border_style
             * @brief Defines different box border drawing styles
             */
            enum class border_style : uint8_t {
                none, // No border
                single_line, // ┌─┐ Single line borders
                double_line, // ╔═╗ Double line borders
                rounded, // ╭─╮ Rounded corners
                heavy // Heavy/bold lines
            };

            /**
             * @struct box_style
             * @brief Defines box appearance (border style and fill behavior)
             */
            struct box_style {
                border_style style = border_style::none; // Border drawing style
                bool is_solid = true; // Fill interior with background color

                // Convenience constructors
                constexpr box_style() noexcept = default;
                constexpr explicit box_style(border_style s, bool solid = true) noexcept
                    : style(s), is_solid(solid) {}

                // Comparison operators for testing
                constexpr bool operator==(const box_style&) const noexcept = default;
            };

            /**
             * @struct line_style
             * @brief Defines line appearance for separators
             *
             * @details
             * Used for drawing horizontal and vertical separator lines in menus,
             * toolbars, and other UI elements. Line style determines the visual
             * appearance of the line characters used.
             */
            struct line_style {
                border_style style = border_style::single_line; // Line drawing style

                // Convenience constructors
                constexpr line_style() noexcept = default;
                constexpr explicit line_style(border_style s) noexcept
                    : style(s) {}

                // Comparison operators for testing
                constexpr bool operator==(const line_style&) const noexcept = default;
            };

            /**
             * @struct font
             * @brief Font attributes for text rendering in TUI
             *
             * In a text-based UI, "font" represents text attributes
             * rather than actual font faces.
             */
            struct font {
                bool bold = false;
                bool underline = false;
                bool reverse = false; // Swap fg/bg colors
            };

            /**
             * @enum icon_style
             * @brief Predefined icon/glyph styles
             */
            enum class icon_style : uint8_t {
                none,
                check, // ✓
                cross, // ✗
                arrow_up, // ↑
                arrow_down, // ↓
                arrow_left, // ←
                arrow_right, // →
                bullet, // •
                folder, // ▶
                file // ■
            };

            /**
             * @struct background_style
             * @brief Backend-specific background rendering attributes
             *
             * @details
             * Defines how the viewport background should be rendered.
             * This is backend-specific and extensible for patterns, gradients, etc.
             *
             * For conio (TUI), the background consists of:
             * - Background color
             * - Fill character (space by default, but could be patterns)
             *
             * Future extensions:
             * - Pattern support (fill_char could be '░', '▒', '▓', etc.)
             * - Gradient information (for terminals that support 24-bit color)
             * - Texture IDs (for more advanced backends)
             */
            struct background_style {
                color bg_color;              ///< Background color
                char fill_char = ' ';        ///< Fill character (space = solid, others = patterns)

                // Convenience constructors
                constexpr background_style() noexcept = default;
                constexpr explicit background_style(const color& c, char ch = ' ') noexcept
                    : bg_color(c), fill_char(ch) {}

                // Comparison operators for testing
                constexpr bool operator==(const background_style&) const noexcept = default;
            };

            using size_type = size; // Required by RenderLike concept
            using color_type = color; // Required by RenderLike concept (for stateless drawing)

            conio_renderer();
            ~conio_renderer();
            // ===================================================================
            // Required Drawing Methods (RenderLike Concept)
            // ===================================================================

            /**
             * @brief Draw a box with the specified style
             * @param r Rectangle defining the box bounds
             * @param style Box drawing style (border and fill behavior)
             * @param fg Foreground color for borders
             * @param bg Background color for fill
             */
            void draw_box(const rect& r, const box_style& style, const color& fg, const color& bg);

            /**
             * @brief Draw text within a rectangle
             * @param r Rectangle defining text bounds
             * @param text Text to draw (UTF-8 encoded)
             * @param f Font attributes
             * @param fg Foreground color for text
             * @param bg Background color behind text
             */
            void draw_text(const rect& r, std::string_view text, const font& f, const color& fg, const color& bg);

            /**
             * @brief Draw an icon/glyph
             * @param r Rectangle defining icon bounds
             * @param style Icon to draw
             * @param fg Foreground color for icon
             * @param bg Background color behind icon
             */
            void draw_icon(const rect& r, icon_style style, const color& fg, const color& bg);

            /**
             * @brief Draw background fill (full viewport)
             * @param viewport The viewport bounds to fill
             * @param style Background style (color, fill character, etc.)
             *
             * @details
             * Optimized for full viewport background rendering. This is more efficient
             * than draw_box() for large areas. Uses the fill character from the style
             * to create solid fills or patterns.
             *
             * @note This is called by background_renderer for solid/pattern modes.
             */
            void draw_background(const rect& viewport, const background_style& style);

            /**
             * @brief Draw background fill for specific regions (dirty region optimization)
             * @param viewport The viewport bounds (for bounds checking)
             * @param style Background style (color, fill character, etc.)
             * @param dirty_regions Specific regions to fill
             *
             * @details
             * For partial updates (dirty regions), fills only the specified areas.
             * More efficient than redrawing the entire viewport. If dirty_regions
             * is empty, falls back to full viewport fill.
             *
             * @note This is called by background_renderer when dirty regions are available.
             */
            void draw_background(const rect& viewport,
                                const background_style& style,
                                const std::vector<rect>& dirty_regions);

            /**
             * @brief Clear a rectangular region (fill with spaces)
             * @param r Rectangle to clear
             * @param bg Background color for cleared region
             *
             * @details
             * Fills the region with space characters using specified background color.
             * This is used by the dirty rectangle system to clear old content.
             */
            void clear_region(const rect& r, const color& bg);

            /**
             * @brief Draw shadow for popup elements (menus, dialogs, tooltips)
             * @param widget_bounds Bounds of the widget casting the shadow
             * @param offset_x Horizontal shadow offset (cells to the right)
             * @param offset_y Vertical shadow offset (cells down)
             *
             * @details
             * Draws a drop shadow by darkening existing background colors in the
             * shadow region (right and bottom margins). Creates classic DOS-style
             * shadow effect for popup elements.
             *
             * Backend-specific rendering:
             * - Uses 50% darkening factor
             * - Respects clipping boundaries
             * - Works in all color modes (truecolor, 256-color, ANSI)
             */
            void draw_shadow(const rect& widget_bounds, int offset_x, int offset_y);

            /**
             * @brief Draw a horizontal line
             * @param r Rectangle defining the line bounds (y and height define line position, x and w define extent)
             * @param style Line drawing style
             * @param fg Foreground color for line characters
             * @param bg Background color behind line
             *
             * @details
             * Draws a horizontal line using box-drawing characters. The line is drawn
             * at vertical position r.y with width r.w starting from r.x.
             * Used for menu separators, horizontal dividers, etc.
             */
            void draw_horizontal_line(const rect& r, const line_style& style, const color& fg, const color& bg);

            /**
             * @brief Draw a vertical line
             * @param r Rectangle defining the line bounds (x and width define line position, y and h define extent)
             * @param style Line drawing style
             * @param fg Foreground color for line characters
             * @param bg Background color behind line
             *
             * @details
             * Draws a vertical line using box-drawing characters. The line is drawn
             * at horizontal position r.x with height r.h starting from r.y.
             * Used for vertical separators, column dividers, etc.
             */
            void draw_vertical_line(const rect& r, const line_style& style, const color& fg, const color& bg);

            // ===================================================================
            // Required Clipping Methods (RenderLike Concept)
            // ===================================================================

            /**
             * @brief Push a clipping rectangle onto the stack
             * @param r Clipping rectangle
             */
            void push_clip(const rect& r);

            /**
             * @brief Pop the current clipping rectangle from the stack
             */
            void pop_clip();

            /**
             * @brief Get the current clipping rectangle
             * @return Current clip rect
             */
            [[nodiscard]] rect get_clip_rect() const;

            // ===================================================================
            // Presentation Method (Required by RenderLike Concept)
            // ===================================================================

            /**
             * @brief Present the rendered frame to the display
             *
             * @details
             * Swaps the back buffer with the front buffer, making all rendered
             * content visible on screen. This should be called after all rendering
             * operations are complete.
             */
            void present();

            /**
             * @brief Handle window resize event
             *
             * @details
             * Called by ui_handle when a resize event is detected.
             * Resizes the internal vram buffer to match new terminal dimensions.
             */
            void on_resize();

            /**
             * @brief Get the current viewport (full terminal bounds)
             * @return Rectangle covering the entire terminal
             *
             * @details
             * Returns the full vram dimensions as a viewport rect.
             * This is used by ui_handle::display() to get rendering bounds.
             */
            [[nodiscard]] rect get_viewport() const;

            // ===================================================================
            // Static Text Measurement (Required by UIBackend Concept)
            // ===================================================================

            /**
             * @brief Measure text dimensions (static - no instance needed)
             * @param text Text to measure (UTF-8 encoded)
             * @param f Font attributes (unused in TUI - all chars are 1x1)
             * @return Size with width = number of visual characters, height = 1
             *
             * @details Uses utf8cpp to correctly count Unicode code points.
             * This is a static method so widgets can measure text during layout
             * without needing a renderer instance.
             */
            static size measure_text(std::string_view text, const font& f);

            /**
             * @brief Get icon size for TUI rendering (static - no instance needed)
             * @param icon The icon style (unused in TUI - all icons are 1x1)
             * @return Size with width = 1, height = 1 (single character)
             *
             * @details In TUI/console rendering, icons are represented as single
             * Unicode characters (emoji, box-drawing chars, etc.). This method
             * returns the fixed size of 1x1 characters.
             *
             * This is a static method so widgets can calculate sizing during layout
             * without needing a renderer instance.
             *
             * @example
             * @code
             * auto icon_size = conio_renderer::get_icon_size(icon);
             * int total_width = icon_size.w + padding + text_width;
             * @endcode
             */
            [[nodiscard]] static size get_icon_size([[maybe_unused]] const icon_style& icon) noexcept {
                return size{1, 1}; // Icons are 1x1 characters in TUI
            }

            /**
             * @brief Get border thickness for a box style (static - no instance needed)
             * @param style The box style to query
             * @return Border thickness in characters (0 for none, 1 for borders)
             *
             * @details Returns the thickness of ONE side of the border.
             * Total space consumed by borders = thickness * 2 (left+right or top+bottom).
             *
             * This is a static method so widgets can calculate sizing during layout
             * without needing a renderer instance.
             *
             * @example
             * @code
             * int border = conio_renderer::get_border_thickness(theme.button.box_style);
             * int total_width = text_width + padding*2 + border*2;  // border on both sides
             * @endcode
             */
            [[nodiscard]] static constexpr int get_border_thickness(const box_style& style) noexcept {
                return (style.style == border_style::none) ? 0 : 1;
            }

        private:
            struct impl;
            std::unique_ptr<impl> m_pimpl;

    };
}
