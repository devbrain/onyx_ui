//
// Created by igor on 20/10/2025.
//

#pragma once

#include <iostream>  // For std::cerr debug logging
#include <onyxui/conio/termbox2_wrappers.hh>
#include <onyxui/concepts/event_like.hh>

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

        // Arrow keys
        static constexpr uint16_t KEY_ARROW_UP = TB_KEY_ARROW_UP;
        static constexpr uint16_t KEY_ARROW_DOWN = TB_KEY_ARROW_DOWN;
        static constexpr uint16_t KEY_ARROW_LEFT = TB_KEY_ARROW_LEFT;
        static constexpr uint16_t KEY_ARROW_RIGHT = TB_KEY_ARROW_RIGHT;

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
            // Terminal reality: Enter=Ctrl+M, Tab=Ctrl+I (Ctrl bit always set at terminal level)
            // Shift has no semantic meaning for these keys - ignore it
            if (e.key == TB_KEY_ENTER || e.key == TB_KEY_TAB) {
                return false;
            }
            return (e.mod & TB_MOD_SHIFT) != 0;
        }

        [[nodiscard]] static bool ctrl_pressed(const tb_event& e) noexcept {
            // Terminal reality: Enter=Ctrl+M, Tab=Ctrl+I (Ctrl bit always set at terminal level)
            // Ignore Ctrl for these keys so they can be used in hotkeys without Ctrl modifier
            //
            // Note: Escape is NOT a Ctrl combination, so Ctrl+Escape is meaningful!
            if (e.key == TB_KEY_ENTER || e.key == TB_KEY_TAB) {
                return false;
            }
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
            // In termbox2, all mouse events have type TB_EVENT_MOUSE
            // The 'key' field indicates what kind of mouse event:
            // - TB_KEY_MOUSE_LEFT/RIGHT/MIDDLE = button press
            // - TB_KEY_MOUSE_RELEASE = button release
            if (e.type != TB_EVENT_MOUSE) return false;

            // Check if it's a button press (not a release or wheel event)
            return e.key == TB_KEY_MOUSE_LEFT ||
                   e.key == TB_KEY_MOUSE_RIGHT ||
                   e.key == TB_KEY_MOUSE_MIDDLE;
        }

        // Hotkey support - convert to ASCII
        [[nodiscard]] static char to_ascii(const tb_event& e) noexcept {
            // Control characters (for semantic actions like menu navigation)
            if (e.key == TB_KEY_ESC) return 27;
            if (e.key == TB_KEY_ENTER) return '\n';
            if (e.key == TB_KEY_TAB) return '\t';

            // For character input
            if (e.ch != 0) {
                const uint32_t ch = e.ch;

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

        // Hotkey support - convert to special key code (for arrow keys)
        [[nodiscard]] static int to_special_key(const tb_event& e) noexcept {
            // Arrow keys use negative codes to distinguish from ASCII/F-keys
            if (e.key == TB_KEY_ARROW_UP) return -1;
            if (e.key == TB_KEY_ARROW_DOWN) return -2;
            if (e.key == TB_KEY_ARROW_LEFT) return -3;
            if (e.key == TB_KEY_ARROW_RIGHT) return -4;
            return 0;  // Not a special key
        }

        // Hotkey support - convert to F-key number
        [[nodiscard]] static int to_f_key(const tb_event& e) noexcept {
            // Static assertions to verify our assumptions about termbox2 F-key ordering
            static_assert(TB_KEY_F1 > TB_KEY_F12,
                "termbox2 F-key ordering assumption violated: expected F1 > F12");
            static_assert(TB_KEY_F1 - TB_KEY_F12 == 11,
                "termbox2 F-key spacing assumption violated: expected F1-F12 = 11");

            // Note: TB_KEY_F1 (65535) > TB_KEY_F12 (65524), so check range correctly
            if (e.key <= TB_KEY_F1 && e.key >= TB_KEY_F12) {
                return (TB_KEY_F1 - e.key) + 1;  // F1=1, F2=2, ..., F12=12
            }
            return 0;  // Not an F-key
        }

        // Window event methods (for resize)
        [[nodiscard]] static bool is_resize_event(const tb_event& e) noexcept {
            return e.type == TB_EVENT_RESIZE;
        }

        [[nodiscard]] static int window_width([[maybe_unused]] const tb_event& e) noexcept {
            // termbox2 doesn't store dimensions in the event - query via wrapper
            return onyxui::conio::conio_get_width();
        }

        [[nodiscard]] static int window_height([[maybe_unused]] const tb_event& e) noexcept {
            // termbox2 doesn't store dimensions in the event - query via wrapper
            return onyxui::conio::conio_get_height();
        }
    };
} // namespace onyxui

// Compile-time check: does tb_event satisfy WindowEvent?
static_assert(onyxui::WindowEvent<tb_event>, "tb_event must satisfy WindowEvent concept");