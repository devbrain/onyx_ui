//
// Created by igor on 10/10/2025.
//

#ifndef ONYXUI_TEST_BACKEND_HH
#define ONYXUI_TEST_BACKEND_HH

#pragma once
#include <cstdint>
#include <onyxui/concepts/backend.hh>
#include <onyxui/concepts/event_like.hh>
#include <utf8.h>  // From utf8cpp (included via failsafe)

namespace onyxui {
    // ======================================================================
    // Test Backend (for unit tests)
    // ======================================================================

    /**
     * @struct test_backend
     * @brief Simple backend for unit testing
     *
     * This backend uses simple POD types that don't require any external
     * dependencies, making it perfect for unit tests.
     *
     * ## Why a Separate Test Backend?
     *
     * 1. **Isolation from External Dependencies**: Tests should not depend on SDL, GLFW,
     *    or other external libraries. This ensures tests can run in CI/CD environments
     *    without requiring GUI libraries to be installed.
     *
     * 2. **Deterministic Behavior**: The test backend provides predictable, simple types
     *    that make test assertions straightforward and reliable.
     *
     * 3. **Fast Compilation**: No need to include heavy external headers in test files,
     *    resulting in faster test compilation times.
     *
     * 4. **Focused Testing**: Tests can focus on the layout logic without worrying about
     *    platform-specific event handling or rendering details.
     *
     * 5. **Simplified Debugging**: When tests fail, the simple POD types make it easier
     *    to inspect values and understand what went wrong.
     *
     * ## Usage Example
     *
     * @code
     * // Create UI elements using the test backend
     * using TestElement = ui_element<test_backend>;
     * auto root = std::make_unique<TestElement>(nullptr);
     *
     * // Set up layout
     * root->set_layout_strategy(std::make_unique<linear_layout<test_backend>>());
     *
     * // Add children
     * auto child = std::make_unique<TestElement>(nullptr);
     * root->add_child(std::move(child));
     *
     * // Perform layout
     * root->measure(100, 100);
     * root->arrange(test_backend::rect{0, 0, 100, 100});
     *
     * // Test assertions
     * REQUIRE(root->bounds() == test_backend::rect{0, 0, 100, 100});
     * @endcode
     *
     * ## Event Testing
     *
     * @code
     * // Create keyboard event
     * test_backend::test_keyboard_event key_event;
     * key_event.pressed = true;
     * key_event.key_code = event_traits<test_backend::test_keyboard_event>::KEY_TAB;
     * key_event.shift = false;
     *
     * // Test event traits
     * using traits = event_traits<test_backend::test_keyboard_event>;
     * REQUIRE(traits::is_tab_key(key_event) == true);
     * REQUIRE(traits::is_key_press(key_event) == true);
     * @endcode
     */
    struct test_backend {
        // Simple geometric types for testing
        struct rect {
            int x = 0, y = 0, w = 0, h = 0;

            constexpr rect() = default;

            constexpr rect(int x_, int y_, int w_, int h_)
                : x(x_), y(y_), w(w_), h(h_) {
            }

            bool operator==(const rect&) const = default;
        };

        struct size {
            int w = 0, h = 0;

            constexpr size() = default;

            constexpr size(int w_, int h_)
                : w(w_), h(h_) {
            }

            bool operator==(const size&) const = default;
        };

        struct point {
            int x = 0, y = 0;

            constexpr point() = default;

            constexpr point(int x_, int y_)
                : x(x_), y(y_) {
            }

            bool operator==(const point&) const = default;
        };

        struct renderer {
            struct box_style {};
            struct icon_style {};
            struct font {};
            using size_type = size;  // Required by RenderLike concept

            void draw_box([[maybe_unused]] const rect& r,[[maybe_unused]]  const box_style& box) {

            }

            void draw_text([[maybe_unused]] const rect& r, [[maybe_unused]] std::string_view x, [[maybe_unused]] const font& f) {

            }

            void draw_icon([[maybe_unused]] const rect& r,[[maybe_unused]]  const icon_style& icon) {

            }

            /**
             * @brief Static text measurement for layout calculations
             * @param text Text to measure (UTF-8 encoded)
             * @param f Font to use (unused in test backend)
             * @return Size with width = number of Unicode code points, height = 1
             *
             * @details
             * **Static method** - can be called without a renderer instance!
             * This is the correct approach: renderer knows how to measure text,
             * and widgets can measure during layout without creating a renderer.
             *
             * Uses **utf8cpp** (from failsafe dependency) to correctly count
             * Unicode code points, handling multi-byte sequences properly.
             *
             * @example UTF-8 handling
             * @code
             * renderer::measure_text("ABC", font)          // {w: 3, h: 1}
             * renderer::measure_text("→←↑", font)          // {w: 3, h: 1} - not 9 bytes!
             * renderer::measure_text("Hello", font)        // {w: 5, h: 1}
             * renderer::measure_text("Здравствуй", f)      // {w: 10, h: 1} - Cyrillic
             * renderer::measure_text("🚀🎉", font)          // {w: 2, h: 1} - Emoji
             * @endcode
             *
             * @example Usage in widget sizing
             * @code
             * size_type get_content_size() const override {
             *     typename Backend::renderer_type::font default_font{};
             *     return Backend::renderer_type::measure_text(m_text, default_font);
             * }
             * @endcode
             *
             * @note Using utf8::distance() from utf8cpp library ensures:
             *       - Correct handling of 1-4 byte UTF-8 sequences
             *       - Invalid UTF-8 sequences are handled gracefully
             *       - BOM (Byte Order Mark) is counted as 1 character
             */
            static size measure_text(std::string_view text, [[maybe_unused]] const font& f) {
                // Use utf8cpp to count Unicode code points (glyphs)
                // This correctly handles multi-byte UTF-8 sequences
                int glyph_count = static_cast<int>(
                    utf8::distance(text.begin(), text.end())
                );

                return size{glyph_count, 1};  // width = glyph count, height = 1
            }

            void push_clip([[maybe_unused]] const rect& r) {}
            void pop_clip() {}
            rect get_clip_rect() const {
                return rect {};
            }
        };

        // Simple event types for testing
        struct test_event {
            enum type_t { none, key_down, key_up, mouse_down, mouse_up, mouse_move };

            type_t type = none;
        };

        struct test_keyboard_event {
            bool pressed = false;
            int key_code = 0;
            bool shift = false;
            bool ctrl = false;
            bool alt = false;
        };

        struct test_mouse_event {
            int x = 0, y = 0;
            int button = 0;
            bool pressed = false;
        };

        // Type aliases
        using rect_type = rect;
        using size_type = size;
        using point_type = point;
        using event_type = test_event;
        using keyboard_event_type = test_keyboard_event;
        using mouse_button_event_type = test_mouse_event;
        using mouse_motion_event_type = test_mouse_event;
        using mouse_wheel_event_type = test_mouse_event;
        using text_input_event_type = test_event;
        using window_event_type = test_event;

        // Simple color for testing
        struct color {
            uint8_t r = 0, g = 0, b = 0, a = 255;
        };

        using color_type = color;
        using renderer_type = renderer;
        using texture_type = void*;
        using font_type = void*;

        static constexpr const char* name() { return "Test"; }
    };

    // Event traits specializations for test backend
    template<>
    struct event_traits <test_backend::test_keyboard_event> {
        // Key code constants
        static constexpr int KEY_TAB = '\t';
        static constexpr int KEY_ENTER = '\n';
        static constexpr int KEY_SPACE = ' ';
        static constexpr int KEY_ESCAPE = 27;

        // F-key constants (starting at 1000 for test purposes)
        static constexpr int KEY_F1 = 1000;
        static constexpr int KEY_F2 = 1001;
        static constexpr int KEY_F3 = 1002;
        static constexpr int KEY_F4 = 1003;
        static constexpr int KEY_F5 = 1004;
        static constexpr int KEY_F6 = 1005;
        static constexpr int KEY_F7 = 1006;
        static constexpr int KEY_F8 = 1007;
        static constexpr int KEY_F9 = 1008;
        static constexpr int KEY_F10 = 1009;
        static constexpr int KEY_F11 = 1010;
        static constexpr int KEY_F12 = 1011;

        [[nodiscard]] static int key_code(const test_backend::test_keyboard_event& e) noexcept { return e.key_code; }
        [[nodiscard]] static bool is_key_press(const test_backend::test_keyboard_event& e) noexcept { return e.pressed; }
        [[nodiscard]] static bool is_repeat(const test_backend::test_keyboard_event&) noexcept { return false; }
        [[nodiscard]] static bool shift_pressed(const test_backend::test_keyboard_event& e) noexcept { return e.shift; }
        [[nodiscard]] static bool ctrl_pressed(const test_backend::test_keyboard_event& e) noexcept { return e.ctrl; }
        [[nodiscard]] static bool alt_pressed(const test_backend::test_keyboard_event& e) noexcept { return e.alt; }

        [[nodiscard]] static bool is_tab_key(const test_backend::test_keyboard_event& e) noexcept { return e.key_code == KEY_TAB; }
        [[nodiscard]] static bool is_enter_key(const test_backend::test_keyboard_event& e) noexcept { return e.key_code == KEY_ENTER; }
        [[nodiscard]] static bool is_space_key(const test_backend::test_keyboard_event& e) noexcept { return e.key_code == KEY_SPACE; }
        [[nodiscard]] static bool is_escape_key(const test_backend::test_keyboard_event& e) noexcept { return e.key_code == KEY_ESCAPE; }

        // Hotkey support - convert to ASCII
        [[nodiscard]] static char to_ascii(const test_backend::test_keyboard_event& e) noexcept {
            int code = e.key_code;

            // Lowercase letters
            if (code >= 'a' && code <= 'z') return static_cast<char>(code);

            // Uppercase letters -> lowercase
            if (code >= 'A' && code <= 'Z') return static_cast<char>(code - 'A' + 'a');

            // Digits
            if (code >= '0' && code <= '9') return static_cast<char>(code);

            // Common punctuation
            switch (code) {
                case ' ': case '!': case '"': case '#': case '$': case '%': case '&': case '\'':
                case '(': case ')': case '*': case '+': case ',': case '-': case '.': case '/':
                case ':': case ';': case '<': case '=': case '>': case '?': case '@':
                case '[': case '\\': case ']': case '^': case '_': case '`':
                case '{': case '|': case '}': case '~':
                    return static_cast<char>(code);
                default:
                    return '\0';  // Not an ASCII key
            }
        }

        // Hotkey support - convert to F-key number
        [[nodiscard]] static int to_f_key(const test_backend::test_keyboard_event& e) noexcept {
            int code = e.key_code;
            if (code >= KEY_F1 && code <= KEY_F12) {
                return (code - KEY_F1) + 1;  // F1=1, F2=2, ..., F12=12
            }
            return 0;  // Not an F-key
        }
    };

    template<>
    struct event_traits <test_backend::test_mouse_event> {
        [[nodiscard]] static int mouse_x(const test_backend::test_mouse_event& e) noexcept { return e.x; }
        [[nodiscard]] static int mouse_y(const test_backend::test_mouse_event& e) noexcept { return e.y; }
        [[nodiscard]] static int mouse_button(const test_backend::test_mouse_event& e) noexcept { return e.button; }
        [[nodiscard]] static bool is_button_press(const test_backend::test_mouse_event& e) noexcept { return e.pressed; }
    };
}


#endif // ONYXUI_TEST_BACKEND_HH
