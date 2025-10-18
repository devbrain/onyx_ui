//
// Created by igor on 13/10/2025.
//

#pragma once

#include <memory>
#include <cstdint>
#include "colors.hh"
#include "rect.hh"

namespace onyxui::conio {

    /**
     * @brief Text attributes for terminal rendering
     *
     * These flags can be combined with bitwise OR.
     * Maps to termbox2 attributes (TB_BOLD, TB_UNDERLINE, etc.)
     */
    enum class text_attribute : uint32_t {
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
            rect get_clip() const noexcept;

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
        private:
            struct impl;
            std::unique_ptr<impl> m_pimpl;
    };
}