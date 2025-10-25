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

            using size_type = size; // Required by RenderLike concept

            conio_renderer();
            ~conio_renderer();
            // ===================================================================
            // Required Drawing Methods (RenderLike Concept)
            // ===================================================================

            /**
             * @brief Draw a box with the specified style
             * @param r Rectangle defining the box bounds
             * @param style Box drawing style (border and fill behavior)
             */
            void draw_box(const rect& r, const box_style& style);

            /**
             * @brief Draw text within a rectangle
             * @param r Rectangle defining text bounds
             * @param text Text to draw (UTF-8 encoded)
             * @param f Font attributes
             */
            void draw_text(const rect& r, std::string_view text, const font& f);

            /**
             * @brief Draw an icon/glyph
             * @param r Rectangle defining icon bounds
             * @param style Icon to draw
             */
            void draw_icon(const rect& r, icon_style style);

            /**
             * @brief Clear a rectangular region (fill with spaces)
             * @param r Rectangle to clear
             *
             * @details
             * Fills the region with space characters using current background color.
             * This is used by the dirty rectangle system to clear old content.
             */
            void clear_region(const rect& r);

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

            // ===================================================================
            // Color Management
            // ===================================================================

            /**
             * @brief Set current foreground color
             */
            void set_foreground(const color& c);

            /**
             * @brief Set current background color
             */
            void set_background(const color& c);

            /**
             * @brief Get current foreground color
             */
            [[nodiscard]] color get_foreground() const;

            /**
             * @brief Get current background color
             */
            [[nodiscard]] color get_background() const;

        private:
            struct impl;
            std::unique_ptr<impl> m_pimpl;

    };
}
