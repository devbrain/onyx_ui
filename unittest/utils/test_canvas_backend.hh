/**
 * @file test_canvas_backend.hh
 * @brief Backend implementation using test_canvas for visual validation
 * @author Testing Infrastructure Team
 * @date 2025-10-21
 *
 * @details
 * Provides a UIBackend implementation that renders to test_canvas instead of screen.
 * This allows tests to verify visual output programmatically.
 */

#pragma once

#include "test_canvas.hh"
#include <onyxui/concepts/backend.hh>
#include <onyxui/theme.hh>
#include <onyxui/theme_registry.hh>
#include <memory>
#include <stack>

namespace onyxui::testing {

    /**
     * @struct canvas_rect
     * @brief Rectangle type for canvas backend
     */
    struct canvas_rect {
        int x = 0;
        int y = 0;
        int w = 0;
        int h = 0;

        canvas_rect() = default;
        canvas_rect(int x_, int y_, int w_, int h_) : x(x_), y(y_), w(w_), h(h_) {}
    };

    /**
     * @struct canvas_size
     * @brief Size type for canvas backend
     */
    struct canvas_size {
        int w = 0;
        int h = 0;

        canvas_size() = default;
        canvas_size(int w_, int h_) : w(w_), h(h_) {}

        bool operator==(const canvas_size&) const = default;
    };

    /**
     * @struct canvas_point
     * @brief Point type for canvas backend
     */
    struct canvas_point {
        int x = 0;
        int y = 0;

        canvas_point() = default;
        canvas_point(int x_, int y_) : x(x_), y(y_) {}

        bool operator==(const canvas_point&) const = default;
    };

    /**
     * @struct canvas_color
     * @brief Color type for canvas backend
     */
    struct canvas_color {
        uint8_t r = 0;
        uint8_t g = 0;
        uint8_t b = 0;

        canvas_color() = default;
        canvas_color(uint8_t r_, uint8_t g_, uint8_t b_) : r(r_), g(g_), b(b_) {}
    };

    /**
     * @struct canvas_keyboard_event
     * @brief Keyboard event type for canvas backend
     */
    struct canvas_keyboard_event {
        int key_code = 0;
        bool ctrl = false;
        bool shift = false;
        bool alt = false;
    };

    /**
     * @struct canvas_mouse_button_event
     * @brief Mouse button event type for canvas backend
     */
    struct canvas_mouse_button_event {
        int x = 0;
        int y = 0;
        int button = 0;  // 0=left, 1=right, 2=middle
        bool pressed = false;
    };

    /**
     * @struct canvas_mouse_motion_event
     * @brief Mouse motion event type for canvas backend
     */
    struct canvas_mouse_motion_event {
        int x = 0;
        int y = 0;
    };

    /**
     * @struct canvas_event
     * @brief Event type for canvas backend
     */
    struct canvas_event {
        enum class type { keyboard, mouse_button, mouse_motion, resize, none };
        type event_type = type::none;
    };

    /**
     * @class canvas_renderer
     * @brief Renderer that draws to test_canvas
     */
    class canvas_renderer {
    public:
        using size_type = canvas_size;  // Required by RenderLike concept

        struct box_style {
            bool draw_border = false;
            char corner = '+';
            char horizontal = '-';
            char vertical = '|';
        };

        struct font {
            bool bold = false;
            bool underline = false;
        };

        struct icon_style {
            char icon = '*';
        };

        explicit canvas_renderer(std::shared_ptr<test_canvas> canvas)
            : m_canvas(canvas)
            , m_viewport(0, 0, canvas ? canvas->width() : 80, canvas ? canvas->height() : 25)
        {
        }

        /**
         * @brief Draw box (border) - required by RenderLike
         */
        void draw_box(const canvas_rect& rect, const box_style& style) {
            if (!style.draw_border || !m_canvas) return;

            if (rect.w < 2 || rect.h < 2) return;  // Too small for border

            // Draw corners
            m_canvas->put(rect.x, rect.y, style.corner);
            m_canvas->put(rect.x + rect.w - 1, rect.y, style.corner);
            m_canvas->put(rect.x, rect.y + rect.h - 1, style.corner);
            m_canvas->put(rect.x + rect.w - 1, rect.y + rect.h - 1, style.corner);

            // Draw horizontal edges
            for (int x = rect.x + 1; x < rect.x + rect.w - 1; ++x) {
                m_canvas->put(x, rect.y, style.horizontal);
                m_canvas->put(x, rect.y + rect.h - 1, style.horizontal);
            }

            // Draw vertical edges
            for (int y = rect.y + 1; y < rect.y + rect.h - 1; ++y) {
                m_canvas->put(rect.x, y, style.vertical);
                m_canvas->put(rect.x + rect.w - 1, y, style.vertical);
            }
        }

        /**
         * @brief Draw text - required by RenderLike (different signature!)
         */
        void draw_text(const canvas_rect& rect, std::string_view text, const font& f) {
            if (!m_canvas) return;

            // Draw text at rect position
            for (size_t i = 0; i < text.length() && rect.x + static_cast<int>(i) < rect.x + rect.w; ++i) {
                uint8_t attrs = 0;
                if (f.bold) attrs |= 0x01;
                if (f.underline) attrs |= 0x02;
                m_canvas->put(rect.x + static_cast<int>(i), rect.y, text[i], 7, 0, attrs);
            }
        }

        /**
         * @brief Draw icon - required by RenderLike
         */
        void draw_icon(const canvas_rect& rect, const icon_style& style) {
            if (!m_canvas) return;
            m_canvas->put(rect.x, rect.y, style.icon);
        }

        /**
         * @brief Clear region - required by RenderLike
         */
        void clear_region(const canvas_rect& rect) {
            if (!m_canvas) return;

            for (int y = rect.y; y < rect.y + rect.h; ++y) {
                for (int x = rect.x; x < rect.x + rect.w; ++x) {
                    m_canvas->put(x, y, ' ');
                }
            }
        }

        /**
         * @brief Measure text size
         */
        static canvas_size measure_text(std::string_view text, const font&) {
            return {static_cast<int>(text.length()), 1};
        }

        /**
         * @brief Get border thickness
         */
        static int get_border_thickness(const box_style& style) {
            return style.draw_border ? 1 : 0;
        }

        /**
         * @brief Get icon size
         */
        static canvas_size get_icon_size(const icon_style&) {
            return {1, 1};
        }

        /**
         * @brief Set foreground color (no-op for test canvas)
         */
        void set_foreground(const canvas_color&) {
            // No-op for basic testing
        }

        /**
         * @brief Set background color (no-op for test canvas)
         */
        void set_background(const canvas_color&) {
            // No-op for basic testing
        }

        /**
         * @brief Push clip rectangle (track for debugging)
         */
        void push_clip(const canvas_rect& rect) {
            m_clip_stack.push(rect);
        }

        /**
         * @brief Pop clip rectangle
         */
        void pop_clip() {
            if (!m_clip_stack.empty()) {
                m_clip_stack.pop();
            }
        }

        /**
         * @brief Get current clip rectangle - required by RenderLike
         */
        canvas_rect get_clip_rect() const {
            if (!m_clip_stack.empty()) {
                return m_clip_stack.top();
            }
            return m_viewport;
        }

        /**
         * @brief Get viewport - required by RenderLike
         */
        canvas_rect get_viewport() const {
            return m_viewport;
        }

        /**
         * @brief Present rendered content - required by RenderLike
         */
        void present() {
            // No-op for test canvas (already in memory)
        }

        /**
         * @brief Handle resize - required by RenderLike
         */
        void on_resize() {
            // No-op for test canvas (size is fixed at construction)
        }

    private:
        std::shared_ptr<test_canvas> m_canvas;
        std::stack<canvas_rect> m_clip_stack;
        canvas_rect m_viewport;
    };

    /**
     * @struct test_canvas_backend
     * @brief Backend implementation using test_canvas
     *
     * @details
     * Satisfies the UIBackend concept and allows widgets to be
     * rendered to a test canvas for visual validation.
     */
    struct test_canvas_backend {
        using rect_type = canvas_rect;
        using size_type = canvas_size;
        using point_type = canvas_point;
        using color_type = canvas_color;
        using event_type = canvas_event;
        using keyboard_event_type = canvas_keyboard_event;
        using mouse_button_event_type = canvas_mouse_button_event;
        using mouse_motion_event_type = canvas_mouse_motion_event;
        using renderer_type = canvas_renderer;

        /**
         * @brief Register themes
         */
        static void register_themes(theme_registry<test_canvas_backend>& registry) {
            // Create a simple default theme for testing
            onyxui::ui_theme<test_canvas_backend> default_theme;
            default_theme.name = "Test Theme";

            // Set up box style with border enabled
            default_theme.panel.box_style.draw_border = true;
            default_theme.panel.box_style.corner = '+';
            default_theme.panel.box_style.horizontal = '-';
            default_theme.panel.box_style.vertical = '|';

            // Register as default (first theme)
            registry.register_theme(default_theme);
        }
    };

    // Verify backend satisfies concept
    static_assert(UIBackend<test_canvas_backend>, "test_canvas_backend must satisfy UIBackend concept");

} // namespace onyxui::testing
