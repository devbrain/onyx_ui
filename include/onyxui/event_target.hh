/**
 * @file event_target.hh
 * @brief Base class for objects that can handle events
 * @author igor
 * @date 10/10/2025
 *
 * This file defines event_target, which provides event handling capabilities
 * independent of any UI or layout concerns. It can be tested in isolation
 * and inherited by any class that needs event handling.
 *
 * The design uses templates to work with native backend events directly,
 * avoiding any abstraction layer while maintaining backend flexibility.
 */

#pragma once

#include <functional>
#include <onyxui/events.hh>

namespace onyxui {

    /**
     * @class event_target
     * @brief Base class providing event handling capabilities
     *
     * This class is completely independent of ui_element or any layout concerns.
     * It simply provides a way to handle events and track state.
     *
     * @tparam EventType The native event type this target handles (SDL_Event, etc.)
     *
     * @example Testing event_target independently
     * @code
     * // Test class that just handles events, no UI
     * template<typename EventType>
     * class test_target : public event_target<EventType> {
     * public:
     *     int click_count = 0;
     *
     *     bool handle_click(int x, int y) override {
     *         click_count++;
     *         return true;
     *     }
     *
     *     bool is_inside(int x, int y) const override {
     *         // Simple test bounds
     *         return x >= 0 && x < 100 && y >= 0 && y < 50;
     *     }
     * };
     *
     * // Test it
     * test_target<SDL_Event> target;
     * SDL_MouseButtonEvent evt;
     * evt.x = 50; evt.y = 25; evt.type = SDL_MOUSEBUTTONDOWN;
     * target.process_event(evt);
     * assert(target.click_count == 1);
     * @endcode
     */
    template<typename EventType>
    class event_target {
    public:
        virtual ~event_target() = default;

        // -----------------------------------------------------------------------
        // Main Event Processing
        // -----------------------------------------------------------------------

        /**
         * @brief Process any event type that has traits defined
         *
         * This template function accepts any event type that has event_traits
         * specialized for it. It extracts the necessary data using traits and
         * calls the appropriate virtual handler.
         */
        template<typename E>
        bool process_event(const E& event) {
            if (!m_enabled) {
                return false;
            }

            // Handle mouse position events (motion)
            if constexpr (MousePositionEvent<E>) {
                int x = event_traits<E>::mouse_x(event);
                int y = event_traits<E>::mouse_y(event);
                return process_mouse_move(x, y);
            }
            // Handle mouse button events
            else if constexpr (MouseButtonEvent<E>) {
                int x = event_traits<E>::mouse_x(event);
                int y = event_traits<E>::mouse_y(event);
                int button = event_traits<E>::mouse_button(event);
                bool pressed = event_traits<E>::is_button_press(event);
                return process_mouse_button(x, y, button, pressed);
            }
            // Handle mouse wheel events
            else if constexpr (MouseWheelEvent<E>) {
                float dx = event_traits<E>::wheel_delta_x(event);
                float dy = event_traits<E>::wheel_delta_y(event);
                return handle_wheel(dx, dy);
            }
            // Handle keyboard events
            else if constexpr (KeyboardEvent<E>) {
                int key = event_traits<E>::key_code(event);
                bool pressed = event_traits<E>::is_key_press(event);
                bool repeat = event_traits<E>::is_repeat(event);

                // Extract modifiers if available
                bool shift = false, ctrl = false, alt = false;
                if constexpr (ModifierState<E>) {
                    shift = event_traits<E>::shift_pressed(event);
                    ctrl = event_traits<E>::ctrl_pressed(event);
                    alt = event_traits<E>::alt_pressed(event);
                }

                return process_key(key, pressed, repeat, shift, ctrl, alt);
            }
            // Handle text input events
            else if constexpr (TextInputEvent<E>) {
                auto text = event_traits<E>::text(event);
                return handle_text_input(text);
            }

            return false;
        }

        // -----------------------------------------------------------------------
        // Virtual Handlers - Override these in derived classes
        // -----------------------------------------------------------------------

        /**
         * @brief Handle mouse enter event
         */
        virtual bool handle_mouse_enter() {
            if (m_on_mouse_enter) return m_on_mouse_enter();
            return false;
        }

        /**
         * @brief Handle mouse leave event
         */
        virtual bool handle_mouse_leave() {
            if (m_on_mouse_leave) return m_on_mouse_leave();
            return false;
        }

        /**
         * @brief Handle mouse move event
         */
        virtual bool handle_mouse_move(int x, int y) {
            if (m_on_mouse_move) return m_on_mouse_move(x, y);
            return false;
        }

        /**
         * @brief Handle mouse button down event
         */
        virtual bool handle_mouse_down(int x, int y, int button) {
            if (m_on_mouse_down) return m_on_mouse_down(x, y, button);
            return false;
        }

        /**
         * @brief Handle mouse button up event
         */
        virtual bool handle_mouse_up(int x, int y, int button) {
            if (m_on_mouse_up) return m_on_mouse_up(x, y, button);
            return false;
        }

        /**
         * @brief Handle click event (generated from down+up)
         */
        virtual bool handle_click(int x, int y) {
            if (m_on_click) return m_on_click(x, y);
            return false;
        }

        /**
         * @brief Handle mouse wheel event
         */
        virtual bool handle_wheel(float delta_x, float delta_y) {
            if (m_on_wheel) return m_on_wheel(delta_x, delta_y);
            return false;
        }

        /**
         * @brief Handle key down event
         */
        virtual bool handle_key_down(int key, bool shift, bool ctrl, bool alt) {
            if (m_on_key_down) return m_on_key_down(key, shift, ctrl, alt);
            return false;
        }

        /**
         * @brief Handle key up event
         */
        virtual bool handle_key_up(int key, bool shift, bool ctrl, bool alt) {
            if (m_on_key_up) return m_on_key_up(key, shift, ctrl, alt);
            return false;
        }

        /**
         * @brief Handle text input event
         */
        virtual bool handle_text_input(std::string_view text) {
            if (m_on_text_input) return m_on_text_input(text);
            return false;
        }

        /**
         * @brief Handle focus gained event
         */
        virtual bool handle_focus_gained() {
            m_has_focus = true;
            if (m_on_focus_gained) return m_on_focus_gained();
            return false;
        }

        /**
         * @brief Handle focus lost event
         */
        virtual bool handle_focus_lost() {
            m_has_focus = false;
            m_is_pressed = false;  // Clear pressed state when losing focus
            if (m_on_focus_lost) return m_on_focus_lost();
            return false;
        }

        // -----------------------------------------------------------------------
        // Hit Testing - Must be implemented by derived class
        // -----------------------------------------------------------------------

        /**
         * @brief Check if a point is inside this target
         *
         * Derived classes must implement this to define their hit area.
         * This is kept abstract so event_target doesn't need to know about
         * rectangles, bounds, or any geometric types.
         */
        virtual bool is_inside(int x, int y) const = 0;

        // -----------------------------------------------------------------------
        // Callback Setters
        // -----------------------------------------------------------------------

        using mouse_callback = std::function<bool(int x, int y)>;
        using button_callback = std::function<bool(int x, int y, int button)>;
        using wheel_callback = std::function<bool(float dx, float dy)>;
        using key_callback = std::function<bool(int key, bool shift, bool ctrl, bool alt)>;
        using text_callback = std::function<bool(std::string_view text)>;
        using simple_callback = std::function<bool()>;

        void set_on_mouse_enter(simple_callback cb) { m_on_mouse_enter = std::move(cb); }
        void set_on_mouse_leave(simple_callback cb) { m_on_mouse_leave = std::move(cb); }
        void set_on_mouse_move(mouse_callback cb) { m_on_mouse_move = std::move(cb); }
        void set_on_mouse_down(button_callback cb) { m_on_mouse_down = std::move(cb); }
        void set_on_mouse_up(button_callback cb) { m_on_mouse_up = std::move(cb); }
        void set_on_click(mouse_callback cb) { m_on_click = std::move(cb); }
        void set_on_wheel(wheel_callback cb) { m_on_wheel = std::move(cb); }
        void set_on_key_down(key_callback cb) { m_on_key_down = std::move(cb); }
        void set_on_key_up(key_callback cb) { m_on_key_up = std::move(cb); }
        void set_on_text_input(text_callback cb) { m_on_text_input = std::move(cb); }
        void set_on_focus_gained(simple_callback cb) { m_on_focus_gained = std::move(cb); }
        void set_on_focus_lost(simple_callback cb) { m_on_focus_lost = std::move(cb); }

        // -----------------------------------------------------------------------
        // State Queries
        // -----------------------------------------------------------------------

        bool is_hovered() const noexcept { return m_is_hovered; }
        bool is_pressed() const noexcept { return m_is_pressed; }
        bool has_focus() const noexcept { return m_has_focus; }
        bool is_focusable() const noexcept { return m_focusable; }
        bool is_enabled() const noexcept { return m_enabled; }
        int tab_index() const noexcept { return m_tab_index; }

        // -----------------------------------------------------------------------
        // State Setters
        // -----------------------------------------------------------------------

        void set_focusable(bool focusable) noexcept { m_focusable = focusable; }
        void set_enabled(bool enabled) noexcept { m_enabled = enabled; }
        void set_tab_index(int index) noexcept { m_tab_index = index; }

        /**
         * @brief Manually set focus state
         *
         * This should typically be called by a focus manager, not directly.
         */
        void set_focus(bool focus) {
            if (focus && !m_has_focus) {
                handle_focus_gained();
            } else if (!focus && m_has_focus) {
                handle_focus_lost();
            }
        }

    protected:
        /**
         * @brief Set whether Enter/Space keys trigger click events
         */
        void set_accept_keys_as_click(bool accept) noexcept {
            m_accept_keys_as_click = accept;
        }

    private:
        // -----------------------------------------------------------------------
        // Internal Processing (State Management)
        // -----------------------------------------------------------------------

        /**
         * @brief Process mouse movement and track hover state
         */
        bool process_mouse_move(int x, int y) {
            bool in_bounds = is_inside(x, y);
            bool was_hovered = m_is_hovered;
            m_is_hovered = in_bounds;

            // Generate enter/leave events
            if (m_is_hovered && !was_hovered) {
                return handle_mouse_enter();
            } else if (!m_is_hovered && was_hovered) {
                return handle_mouse_leave();
            }

            return in_bounds ? handle_mouse_move(x, y) : false;
        }

        /**
         * @brief Process mouse button and track pressed state
         */
        bool process_mouse_button(int x, int y, int button, bool pressed) {
            if (pressed) {
                if (is_inside(x, y)) {
                    m_is_pressed = true;
                    m_press_started_inside = true;
                    return handle_mouse_down(x, y, button);
                }
                m_press_started_inside = false;
            } else {
                bool was_pressed = m_is_pressed;
                m_is_pressed = false;

                bool in_bounds = is_inside(x, y);
                bool handled = false;

                // Call mouse up if we're inside or were pressed
                if (in_bounds || was_pressed) {
                    handled = handle_mouse_up(x, y, button);
                }

                // Generate click if pressed and released inside
                if (was_pressed && m_press_started_inside && in_bounds) {
                    handled |= handle_click(x, y);
                }

                m_press_started_inside = false;
                return handled;
            }

            return false;
        }

        /**
         * @brief Process keyboard input
         */
        bool process_key(int key, bool pressed, bool repeat,
                        bool shift, bool ctrl, bool alt) {
            if (!m_has_focus) {
                return false;
            }

            if (pressed) {
                // Handle Enter/Space as click for focused elements
                if (m_accept_keys_as_click && !repeat) {
                    // Common key codes (these would need backend-specific mapping)
                    constexpr int KEY_ENTER = 13;
                    constexpr int KEY_SPACE = 32;

                    if (key == KEY_ENTER || key == KEY_SPACE) {
                        // Generate a click at the center (derived class can override)
                        return handle_click(0, 0);
                    }
                }
                return handle_key_down(key, shift, ctrl, alt);
            } else {
                return handle_key_up(key, shift, ctrl, alt);
            }
        }

        // Event callbacks
        simple_callback m_on_mouse_enter;
        simple_callback m_on_mouse_leave;
        mouse_callback m_on_mouse_move;
        button_callback m_on_mouse_down;
        button_callback m_on_mouse_up;
        mouse_callback m_on_click;
        wheel_callback m_on_wheel;
        key_callback m_on_key_down;
        key_callback m_on_key_up;
        text_callback m_on_text_input;
        simple_callback m_on_focus_gained;
        simple_callback m_on_focus_lost;

        // State tracking
        bool m_is_hovered = false;
        bool m_is_pressed = false;
        bool m_press_started_inside = false;
        bool m_has_focus = false;
        bool m_focusable = false;
        bool m_enabled = true;
        bool m_accept_keys_as_click = false;
        int m_tab_index = 0;
    };
}