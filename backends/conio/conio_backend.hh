//
// Created by igor on 14/10/2025.
//

#pragma once

#include <onyxui/concepts/backend.hh>
#include <onyxui/concepts/event_like.hh>
#include "rect.hh"
#include "colors.hh"
#include "drawing_utils.hh"
#include <memory>
#include <string>
#include <termbox2.h>


namespace onyxui::conio {

    // Forward declarations
    class vram;

    // Size type for the backend
    struct size {
        int w = 0, h = 0;

        constexpr size() = default;
        constexpr size(int w_, int h_) : w(w_), h(h_) {}
        bool operator==(const size&) const = default;
    };

    // Point type for the backend
    struct point {
        int x = 0, y = 0;

        constexpr point() = default;
        constexpr point(int x_, int y_) : x(x_), y(y_) {}
        bool operator==(const point&) const = default;
    };

    // Renderer type - manages the vram
    class conio_renderer {
    public:
        explicit conio_renderer(std::shared_ptr<vram> vram);
        ~conio_renderer();

        // Drawing primitives
        void clear(color bg);
        void put_char(int x, int y, int ch, color fg, color bg);
        void put_string(int x, int y, const std::string& str, color fg, color bg);
        void draw_rect(const rect& r, color fg, color bg, bool filled = false);
        void draw_frame(const rect& r, color fg, color bg, box_style style = box_style::single);

        // Clipping
        void push_clip(const rect& r);
        void pop_clip();

        // Present changes to screen
        void present();

    private:
        struct impl;
        std::unique_ptr<impl> m_pimpl;
    };

    /**
     * @struct conio_backend
     * @brief Terminal/console backend using termbox2
     *
     * This backend provides a text-mode UI using terminal capabilities.
     * It supports both ASCII art and Unicode box-drawing characters.
     *
     * Uses termbox2's tb_event directly - no wrapper types needed!
     */
    struct conio_backend {
        // Required geometric types
        using rect_type = rect;
        using size_type = size;
        using point_type = point;

        // Required event types - all use tb_event with different traits!
        using event_type = tb_event;
        using keyboard_event_type = tb_event;
        using mouse_button_event_type = tb_event;
        using mouse_motion_event_type = tb_event;
        using mouse_wheel_event_type = tb_event;
        using text_input_event_type = tb_event;
        using window_event_type = tb_event;

        // Rendering types
        using color_type = color;
        using renderer_type = conio_renderer*;
        using texture_type = void*;  // Not applicable for terminal
        using font_type = void*;      // Not applicable for terminal

        static constexpr const char* name() { return "Conio"; }
    };
}

// Event traits specialization for termbox2's tb_event
namespace onyxui {
    // Keyboard event traits
    template<>
    struct event_traits<tb_event> {
        // Key codes - map termbox2 constants
        static constexpr int KEY_TAB = 0x09;
        static constexpr int KEY_ENTER = 0x0D;
        static constexpr int KEY_SPACE = 0x20;
        static constexpr int KEY_ESCAPE = 0x1B;

        // Keyboard event methods
        [[nodiscard]] static int key_code(const tb_event& e) noexcept {
            return e.key != 0 ? static_cast<int>(e.key) : static_cast<int>(e.ch);
        }

        [[nodiscard]] static bool is_key_press(const tb_event& e) noexcept {
            return e.type == 1; // TB_EVENT_KEY
        }

        [[nodiscard]] static bool is_repeat(const tb_event& e) noexcept {
            return false; // termbox2 doesn't provide repeat info
        }

        // Modifier keys (from tb_event.mod)
        [[nodiscard]] static bool shift_pressed(const tb_event& e) noexcept {
            return (e.mod & 0x01) != 0; // TB_MOD_SHIFT
        }

        [[nodiscard]] static bool ctrl_pressed(const tb_event& e) noexcept {
            return (e.mod & 0x02) != 0; // TB_MOD_CTRL
        }

        [[nodiscard]] static bool alt_pressed(const tb_event& e) noexcept {
            return (e.mod & 0x04) != 0; // TB_MOD_ALT
        }

        // Helper methods for common keys
        [[nodiscard]] static bool is_tab_key(const tb_event& e) noexcept {
            return e.key == KEY_TAB || e.ch == KEY_TAB;
        }

        [[nodiscard]] static bool is_enter_key(const tb_event& e) noexcept {
            return e.key == KEY_ENTER || e.ch == KEY_ENTER;
        }

        [[nodiscard]] static bool is_space_key(const tb_event& e) noexcept {
            return e.key == KEY_SPACE || e.ch == KEY_SPACE;
        }

        [[nodiscard]] static bool is_escape_key(const tb_event& e) noexcept {
            return e.key == KEY_ESCAPE;
        }

        // Mouse event methods
        [[nodiscard]] static int mouse_x(const tb_event& e) noexcept {
            return static_cast<int>(e.x);
        }

        [[nodiscard]] static int mouse_y(const tb_event& e) noexcept {
            return static_cast<int>(e.y);
        }

        [[nodiscard]] static int mouse_button(const tb_event& e) noexcept {
            return e.key; // In mouse events, key field contains button info
        }

        [[nodiscard]] static bool is_button_press(const tb_event& e) noexcept {
            return e.type == 2; // TB_EVENT_MOUSE
        }

        // Mouse wheel (termbox2 uses special key values for wheel)
        [[nodiscard]] static float wheel_delta_x(const tb_event& e) noexcept {
            return 0.0f; // termbox2 doesn't support horizontal scroll
        }

        [[nodiscard]] static float wheel_delta_y(const tb_event& e) noexcept {
            // TB_KEY_MOUSE_WHEEL_UP = 65529, TB_KEY_MOUSE_WHEEL_DOWN = 65530
            if (e.key == 65529) return 1.0f;   // wheel up
            if (e.key == 65530) return -1.0f;  // wheel down
            return 0.0f;
        }

        // Window/resize event methods
        [[nodiscard]] static int window_width(const tb_event& e) noexcept {
            return static_cast<int>(e.w);
        }

        [[nodiscard]] static int window_height(const tb_event& e) noexcept {
            return static_cast<int>(e.h);
        }

        // Text input
        [[nodiscard]] static std::string_view text(const tb_event& e) noexcept {
            static thread_local char buf[5];
            if (e.ch != 0) {
                // Convert Unicode codepoint to UTF-8
                // Simple single-char implementation
                if (e.ch < 0x80) {
                    buf[0] = static_cast<char>(e.ch);
                    buf[1] = '\0';
                    return std::string_view(buf, 1);
                }
            }
            return std::string_view();
        }
    };

    // Mark tb_event as a supported event type
    template<>
    struct is_event<tb_event> : std::true_type {};
}
