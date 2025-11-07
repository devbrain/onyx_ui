//
// Created by igor on 13/10/2025.
//

#pragma once

#include <memory>
#include <cstdint>
#include <onyxui/conio/colors.hh>
#include <onyxui/conio/geometry.hh>

namespace onyxui::conio {

    /**
     * @brief Text attributes for terminal rendering
     *
     * These flags can be combined with bitwise OR.
     * Maps to termbox2 attributes (TB_BOLD, TB_UNDERLINE, etc.)
     */
    enum class text_attribute : uint8_t {
        none      = 0,
        bold      = 1 << 0,  ///< Bold/bright text
        underline = 1 << 1,  ///< Underlined text
        reverse   = 1 << 2,  ///< Reverse video (swap fg/bg)
        italic    = 1 << 3   ///< Italic text (if supported)
    };

    /**
     * @brief Bitwise OR for combining text attributes
     */
    constexpr text_attribute operator|(text_attribute lhs, text_attribute rhs) noexcept {
        return static_cast<text_attribute>(
            static_cast<uint32_t>(lhs) | static_cast<uint32_t>(rhs)
        );
    }

    /**
     * @brief Bitwise AND for testing text attributes
     */
    constexpr text_attribute operator&(text_attribute lhs, text_attribute rhs) noexcept {
        return static_cast<text_attribute>(
            static_cast<uint32_t>(lhs) & static_cast<uint32_t>(rhs)
        );
    }

    /**
     * @brief Bitwise OR assignment
     */
    constexpr text_attribute& operator|=(text_attribute& lhs, text_attribute rhs) noexcept {
        lhs = lhs | rhs;
        return lhs;
    }

    class vram {
        public:
            vram();
            ~vram();

            void set_clip(const rect& r);
            [[nodiscard]] rect get_clip() const noexcept;

            /**
             * @brief Put a character at position with colors and attributes
             *
             * @param x X coordinate
             * @param y Y coordinate
             * @param ch Character code
             * @param fg Foreground color
             * @param bg Background color
             * @param attr Text attributes (bold, underline, reverse, italic)
             */
            void put(int x, int y, int ch, color fg, color bg,
                     text_attribute attr = text_attribute::none);

            /**
             * @brief Darken a rectangular region for shadow effect
             *
             * @param shadow_rect Rectangle to darken
             * @param factor Darkening factor (0.0 = black, 1.0 = unchanged)
             *
             * @details
             * Multiplies the background color of each cell in the region by the factor.
             * Used for rendering drop shadows on menus, dialogs, and tooltips.
             * Respects clipping boundaries.
             */
            void darken_region(const rect& shadow_rect, float factor);

            /**
             * @brief Lighten a rectangular region for highlight effect
             *
             * @param highlight_rect Rectangle to lighten
             * @param factor Lightening factor (1.0 = unchanged, 2.0 = twice as bright, clamped to white)
             *
             * @details
             * Multiplies the background color of each cell in the region by the factor.
             * Used for rendering 3D button highlights (top and left edges).
             * Respects clipping boundaries.
             */
            void lighten_region(const rect& highlight_rect, float factor);

            [[nodiscard]] int get_width() const;
            [[nodiscard]] int get_height() const;

            /**
             * @brief Resize vram buffer to match current terminal dimensions
             *
             * @details
             * Should be called when TB_EVENT_RESIZE is received to reallocate
             * the internal cell buffer to match the new terminal size.
             */
            void resize();

            void present();

            /**
             * @brief Take screenshot of current vram buffer
             * @param sink Output stream to write screenshot to
             *
             * @details
             * Outputs the current vram buffer content as plain text.
             * Each line represents one row of the terminal.
             * Useful for testing, debugging, and documentation.
             */
            void take_screenshot(std::ostream& sink) const;

        private:
            struct impl;
            std::unique_ptr<impl> m_pimpl;
    };
}