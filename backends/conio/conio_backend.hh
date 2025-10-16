//
// Created by igor on 16/10/2025.
//

#pragma once

#include <onyxui/concepts/backend.hh>
#include <onyxui/concepts/event_like.hh>
#include "rect.hh"
#include "colors.hh"
#include "vram.hh"
#include <termbox2.h>
#include <memory>
#include <string_view>

namespace onyxui::conio {
    // ======================================================================
    // Basic Types
    // ======================================================================

    struct size {
        size(int w, int h) : w(w), h(h) {}
        size() : size(0, 0) {}
        int w, h;
        bool operator==(const size&) const = default;
    };

    struct point {
        point(int x, int y) : x(x), y(y) {}
        point() : point(0, 0) {}
        int x, y;
        bool operator==(const point&) const = default;
    };

    // ======================================================================
    // Renderer Type
    // ======================================================================

    /**
     * @class conio_renderer
     * @brief TUI renderer using vram as low-level drawing library
     *
     * This renderer uses the vram class for efficient character-based
     * rendering with clipping support. It provides all required methods
     * for the RenderLike concept.
     */
    class conio_renderer {
    public:
        // ===================================================================
        // Required Types for RenderLike Concept
        // ===================================================================

        /**
         * @enum box_style
         * @brief Defines different box drawing styles
         */
        enum class box_style {
            none,           // No border
            single_line,    // ┌─┐ Single line borders
            double_line,    // ╔═╗ Double line borders
            rounded,        // ╭─╮ Rounded corners
            heavy           // Heavy/bold lines
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
            bool reverse = false;  // Swap fg/bg colors
        };

        /**
         * @enum icon_style
         * @brief Predefined icon/glyph styles
         */
        enum class icon_style {
            none,
            check,          // ✓
            cross,          // ✗
            arrow_up,       // ↑
            arrow_down,     // ↓
            arrow_left,     // ←
            arrow_right,    // →
            bullet,         // •
            folder,         // ▶
            file            // ■
        };

        using size_type = size;  // Required by RenderLike concept

    public:
        /**
         * @brief Construct renderer with a vram instance
         * @param vram_ptr Shared pointer to vram for drawing operations
         */
        explicit conio_renderer(std::shared_ptr<vram> vram_ptr)
            : m_vram(std::move(vram_ptr)) {}

        // ===================================================================
        // Required Drawing Methods (RenderLike Concept)
        // ===================================================================

        /**
         * @brief Draw a box with the specified style
         * @param r Rectangle defining the box bounds
         * @param style Box drawing style
         */
        void draw_box(const rect& r, box_style style);

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
        rect get_clip_rect() const;

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
        [[nodiscard]] color get_foreground() const { return m_fg; }

        /**
         * @brief Get current background color
         */
        [[nodiscard]] color get_background() const { return m_bg; }

    private:
        std::shared_ptr<vram> m_vram;
        // Default white foreground
        color m_fg{color::MAX_COMPONENT, color::MAX_COMPONENT, color::MAX_COMPONENT};
        // Default black background
        color m_bg{color::MIN_COMPONENT, color::MIN_COMPONENT, color::MIN_COMPONENT};
    };

    // ======================================================================
    // Backend Definition
    // ======================================================================

    /**
     * @struct conio_backend
     * @brief Console I/O backend for terminal-based UIs using termbox2
     *
     * This backend provides all required types for the UIBackend concept:
     * - Geometric types: rect, size, point
     * - Event types: termbox2 events
     * - Renderer: conio_renderer using vram
     *
     * @example Usage
     * @code
     * using element = ui_element<conio_backend>;
     * auto root = std::make_unique<element>();
     *
     * auto vram_instance = std::make_shared<vram>();
     * conio_renderer renderer(vram_instance);
     *
     * root->measure(80, 24);
     * root->arrange(conio_backend::rect{0, 0, 80, 24});
     * root->render(&renderer);
     * @endcode
     */
    struct conio_backend {
        // Required geometric types
        using rect_type = rect;
        using size_type = size;
        using point_type = point;

        // Required event types (from termbox2)
        using event_type = tb_event;
        using keyboard_event_type = tb_event;
        using mouse_button_event_type = tb_event;
        using mouse_motion_event_type = tb_event;
        using mouse_wheel_event_type = tb_event;
        using text_input_event_type = tb_event;
        using window_event_type = tb_event;

        // Required rendering types
        using color_type = color;
        using renderer_type = conio_renderer;
        using texture_type = void*;  // Not used in TUI
        using font_type = void*;     // Not used in TUI

        // Backend identification
        static constexpr const char* name() { return "Conio"; }
    };

} // namespace onyxui::conio

// ======================================================================
// Event Traits Specializations (must be in onyxui namespace)
// ======================================================================

namespace onyxui {
    /**
     * @brief Event traits for termbox2 keyboard events
     */
    template<>
    struct event_traits<tb_event> {
        // Event type constants (from termbox2)
        static constexpr uint8_t EVENT_KEY = TB_EVENT_KEY;
        static constexpr uint8_t EVENT_RESIZE = TB_EVENT_RESIZE;
        static constexpr uint8_t EVENT_MOUSE = TB_EVENT_MOUSE;

        // Key code constants (from termbox2)
        static constexpr uint16_t KEY_TAB = TB_KEY_TAB;
        static constexpr uint16_t KEY_ENTER = TB_KEY_ENTER;
        static constexpr uint16_t KEY_SPACE = TB_KEY_SPACE;
        static constexpr uint16_t KEY_ESCAPE = TB_KEY_ESC;

        // F-key constants
        static constexpr uint16_t KEY_F1 = TB_KEY_F1;
        static constexpr uint16_t KEY_F2 = TB_KEY_F2;
        static constexpr uint16_t KEY_F3 = TB_KEY_F3;
        static constexpr uint16_t KEY_F4 = TB_KEY_F4;
        static constexpr uint16_t KEY_F5 = TB_KEY_F5;
        static constexpr uint16_t KEY_F6 = TB_KEY_F6;
        static constexpr uint16_t KEY_F7 = TB_KEY_F7;
        static constexpr uint16_t KEY_F8 = TB_KEY_F8;
        static constexpr uint16_t KEY_F9 = TB_KEY_F9;
        static constexpr uint16_t KEY_F10 = TB_KEY_F10;
        static constexpr uint16_t KEY_F11 = TB_KEY_F11;
        static constexpr uint16_t KEY_F12 = TB_KEY_F12;

        // Modifier constants
        static constexpr uint8_t MOD_SHIFT = TB_MOD_SHIFT;
        static constexpr uint8_t MOD_CTRL = TB_MOD_CTRL;
        static constexpr uint8_t MOD_ALT = TB_MOD_ALT;

        // Keyboard event methods
        [[nodiscard]] static int key_code(const tb_event& e) noexcept {
            return (e.key != 0) ? static_cast<int>(e.key) : static_cast<int>(e.ch);
        }

        [[nodiscard]] static bool is_key_press(const tb_event& e) noexcept {
            return e.type == TB_EVENT_KEY;
        }

        [[nodiscard]] static bool is_repeat(const tb_event&) noexcept {
            return false;  // termbox2 doesn't provide repeat info
        }

        [[nodiscard]] static bool shift_pressed(const tb_event& e) noexcept {
            return (e.mod & TB_MOD_SHIFT) != 0;
        }

        [[nodiscard]] static bool ctrl_pressed(const tb_event& e) noexcept {
            return (e.mod & TB_MOD_CTRL) != 0;
        }

        [[nodiscard]] static bool alt_pressed(const tb_event& e) noexcept {
            return (e.mod & TB_MOD_ALT) != 0;
        }

        [[nodiscard]] static bool is_tab_key(const tb_event& e) noexcept {
            return e.key == TB_KEY_TAB;
        }

        [[nodiscard]] static bool is_enter_key(const tb_event& e) noexcept {
            return e.key == TB_KEY_ENTER;
        }

        [[nodiscard]] static bool is_space_key(const tb_event& e) noexcept {
            return e.ch == ' ' || e.key == TB_KEY_SPACE;
        }

        [[nodiscard]] static bool is_escape_key(const tb_event& e) noexcept {
            return e.key == TB_KEY_ESC;
        }

        // Mouse event methods
        [[nodiscard]] static int mouse_x(const tb_event& e) noexcept {
            return e.x;
        }

        [[nodiscard]] static int mouse_y(const tb_event& e) noexcept {
            return e.y;
        }

        [[nodiscard]] static int mouse_button(const tb_event& e) noexcept {
            return e.key;  // termbox2 uses 'key' field for mouse buttons
        }

        [[nodiscard]] static bool is_button_press(const tb_event& e) noexcept {
            return e.type == TB_EVENT_MOUSE;
        }

        // Hotkey support - convert to ASCII
        [[nodiscard]] static char to_ascii(const tb_event& e) noexcept {
            // For character input
            if (e.ch != 0) {
                uint32_t ch = e.ch;

                // Lowercase letters
                if (ch >= 'a' && ch <= 'z') return static_cast<char>(ch);

                // Uppercase letters -> lowercase
                if (ch >= 'A' && ch <= 'Z') return static_cast<char>(ch - 'A' + 'a');

                // Digits
                if (ch >= '0' && ch <= '9') return static_cast<char>(ch);

                // Common punctuation (ASCII range)
                if (ch >= 32 && ch <= 126) return static_cast<char>(ch);
            }

            return '\0';  // Not an ASCII key
        }

        // Hotkey support - convert to F-key number
        [[nodiscard]] static int to_f_key(const tb_event& e) noexcept {
            if (e.key >= TB_KEY_F1 && e.key <= TB_KEY_F12) {
                return (e.key - TB_KEY_F1) + 1;  // F1=1, F2=2, ..., F12=12
            }
            return 0;  // Not an F-key
        }
    };
} // namespace onyxui

// Static assertion to verify backend compliance (after event_traits specialization)
static_assert(onyxui::UIBackend<onyxui::conio::conio_backend>,
              "conio_backend must satisfy UIBackend concept");
