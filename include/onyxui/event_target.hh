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
 * The design uses the Backend template to work with native backend events directly,
 * avoiding any abstraction layer while maintaining backend flexibility.
 *
 * ## Architecture
 *
 * The event_target class uses a two-layer event handling approach:
 *
 * 1. **Event Processing Layer**: The `process_event()` template method accepts any
 *    event type with defined traits. It uses event_traits to extract information
 *    and routes to the appropriate handler.
 *
 * 2. **Virtual Handler Layer**: Protected virtual methods (`handle_*`) that derived
 *    classes can override to implement custom behavior. Each method has a sensible
 *    default that calls the corresponding callback if set.
 *
 * ## Event Flow
 *
 * 1. External event arrives via `process_event(event)`
 * 2. Event traits extract relevant data (coordinates, keys, modifiers)
 * 3. Internal processing updates state (hover, pressed, focus)
 * 4. Virtual handler is called (can be overridden)
 * 5. If not overridden, callback is invoked if set
 * 6. Event consumed flag is returned
 *
 * ## State Management
 *
 * The class tracks several states automatically:
 * - **Hover**: Updated based on mouse position and `is_inside()` checks
 * - **Pressed**: Tracked from mouse down to mouse up
 * - **Focus**: Managed by focus_manager (friend class)
 * - **Enabled**: Controls whether events are processed
 */

#pragma once

#include <functional>
#include <onyxui/concepts/backend.hh>
#include <onyxui/concepts/event_like.hh>

namespace onyxui {
    // Forward declarations for friend declarations
    template<UIBackend Backend> class focus_manager;
    template<UIBackend Backend> class ui_handle;

    /**
     * @class event_target
     * @brief Base class providing event handling capabilities
     *
     * This class is completely independent of ui_element or any layout concerns.
     * It simply provides a way to handle events and track state.
     *
     * @tparam Backend The backend traits type (sdl_backend, glfw_backend, etc.)
     *
     * @example Testing event_target independently
     * @code
     * // Test class that just handles events, no UI
     * class test_target : public event_target<test_backend> {
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
     * test_target target;
     * test_backend::test_mouse_event evt;
     * evt.x = 50; evt.y = 25; evt.pressed = true;
     * target.process_event_impl(evt);  // Use template method directly
     * // Or: target.process_event(evt) if evt matches event_type
     * assert(target.click_count == 1);
     * @endcode
     */
    template<UIBackend Backend>
    class event_target {
        // Allow focus_manager to access protected focus methods
        friend class focus_manager<Backend>;
        // Allow ui_handle to access protected mouse event handlers
        friend class ui_handle<Backend>;

        public:
            // Type aliases from backend
            using event_type = typename Backend::event_type;
            using keyboard_event_type = typename Backend::keyboard_event_type;
            using mouse_button_event_type = typename Backend::mouse_button_event_type;
            using mouse_motion_event_type = typename Backend::mouse_motion_event_type;

            virtual ~event_target() = default;

            // Delete copy operations
            // Rationale: Event handlers are per-instance and shouldn't be copied
            event_target(const event_target&) = delete;
            event_target& operator=(const event_target&) = delete;

            // Allow move operations
            // Rationale: Event handlers can transfer to new owner
            event_target(event_target&&) noexcept = default;
            event_target& operator=(event_target&&) noexcept = default;

            // -----------------------------------------------------------------------
            // Main Event Processing
            // -----------------------------------------------------------------------

            /**
             * @brief Virtual method for polymorphic event dispatch
             *
             * This virtual method enables layer managers and other systems to route events
             * through base pointers. It accepts the backend's concrete event type and
             * forwards it to the template process_event method.
             *
             * @param event The backend-specific event to process
             * @return true if the event was handled
             *
             * @note This method exists to solve the polymorphic dispatch problem where
             *       layer_manager needs to call process_event through a base pointer.
             * @note Default implementation forwards to the template method.
             */
            virtual bool process_event(const event_type& event) {
                // Default: forward to template method
                return process_event_impl(event);
            }

            /**
             * @brief Process any event type that has traits defined
             *
             * This template function accepts any event type that has event_traits
             * specialized for it. It extracts the necessary data using traits and
             * calls the appropriate virtual handler.
             *
             * @exception Any exception thrown by virtual handlers (handle_mouse_move, handle_key_down, etc.)
             * @exception Any exception thrown by registered callbacks
             * @note Exception safety: Basic guarantee - internal state (hover, pressed) may be modified before exception
             * @note Returns false (no exception) if target is disabled
             * @note Renamed from process_event to process_event_impl to avoid confusion with virtual method
             */
            template<typename E>
            bool process_event_impl(const E& event);

            // -----------------------------------------------------------------------
            // Callback Setters
            // -----------------------------------------------------------------------

            using mouse_callback = std::function <bool(int x, int y)>;
            using button_callback = std::function <bool(int x, int y, int button)>;
            using wheel_callback = std::function <bool(float dx, float dy)>;
            using key_callback = std::function <bool(int key, bool shift, bool ctrl, bool alt)>;
            using text_callback = std::function <bool(std::string_view text)>;
            using simple_callback = std::function <bool()>;

            /**
             * @brief Set callback for mouse enter event
             * @exception std::bad_alloc If callback allocation fails
             * @note Exception safety: Strong guarantee - old callback retained if exception thrown
             */
            void set_on_mouse_enter(simple_callback cb) { m_on_mouse_enter = std::move(cb); }

            /**
             * @brief Set callback for mouse leave event
             * @exception std::bad_alloc If callback allocation fails
             * @note Exception safety: Strong guarantee - old callback retained if exception thrown
             */
            void set_on_mouse_leave(simple_callback cb) { m_on_mouse_leave = std::move(cb); }

            /**
             * @brief Set callback for mouse move event
             * @exception std::bad_alloc If callback allocation fails
             * @note Exception safety: Strong guarantee - old callback retained if exception thrown
             */
            void set_on_mouse_move(mouse_callback cb) { m_on_mouse_move = std::move(cb); }

            /**
             * @brief Set callback for mouse button down event
             * @exception std::bad_alloc If callback allocation fails
             * @note Exception safety: Strong guarantee - old callback retained if exception thrown
             */
            void set_on_mouse_down(button_callback cb) { m_on_mouse_down = std::move(cb); }

            /**
             * @brief Set callback for mouse button up event
             * @exception std::bad_alloc If callback allocation fails
             * @note Exception safety: Strong guarantee - old callback retained if exception thrown
             */
            void set_on_mouse_up(button_callback cb) { m_on_mouse_up = std::move(cb); }

            /**
             * @brief Set callback for click event
             * @exception std::bad_alloc If callback allocation fails
             * @note Exception safety: Strong guarantee - old callback retained if exception thrown
             */
            void set_on_click(mouse_callback cb) { m_on_click = std::move(cb); }

            /**
             * @brief Set callback for mouse wheel event
             * @exception std::bad_alloc If callback allocation fails
             * @note Exception safety: Strong guarantee - old callback retained if exception thrown
             */
            void set_on_wheel(wheel_callback cb) { m_on_wheel = std::move(cb); }

            /**
             * @brief Set callback for key down event
             * @exception std::bad_alloc If callback allocation fails
             * @note Exception safety: Strong guarantee - old callback retained if exception thrown
             */
            void set_on_key_down(key_callback cb) { m_on_key_down = std::move(cb); }

            /**
             * @brief Set callback for key up event
             * @exception std::bad_alloc If callback allocation fails
             * @note Exception safety: Strong guarantee - old callback retained if exception thrown
             */
            void set_on_key_up(key_callback cb) { m_on_key_up = std::move(cb); }

            /**
             * @brief Set callback for text input event
             * @exception std::bad_alloc If callback allocation fails
             * @note Exception safety: Strong guarantee - old callback retained if exception thrown
             */
            void set_on_text_input(text_callback cb) { m_on_text_input = std::move(cb); }

            /**
             * @brief Set callback for focus gained event
             * @exception std::bad_alloc If callback allocation fails
             * @note Exception safety: Strong guarantee - old callback retained if exception thrown
             */
            void set_on_focus_gained(simple_callback cb) { m_on_focus_gained = std::move(cb); }

            /**
             * @brief Set callback for focus lost event
             * @exception std::bad_alloc If callback allocation fails
             * @note Exception safety: Strong guarantee - old callback retained if exception thrown
             */
            void set_on_focus_lost(simple_callback cb) { m_on_focus_lost = std::move(cb); }

            // -----------------------------------------------------------------------
            // State Queries
            // -----------------------------------------------------------------------

            /**
             * @brief Check if mouse is currently hovering over this target
             * @note Exception safety: No-throw guarantee (noexcept)
             */
            [[nodiscard]] bool is_hovered() const noexcept { return m_is_hovered; }

            /**
             * @brief Check if mouse button is currently pressed on this target
             * @note Exception safety: No-throw guarantee (noexcept)
             */
            [[nodiscard]] bool is_pressed() const noexcept { return m_is_pressed; }

            /**
             * @brief Check if this target currently has keyboard focus
             * @note Exception safety: No-throw guarantee (noexcept)
             */
            [[nodiscard]] bool has_focus() const noexcept { return m_has_focus; }

            /**
             * @brief Check if this target can receive keyboard focus
             * @note Exception safety: No-throw guarantee (noexcept)
             */
            [[nodiscard]] bool is_focusable() const noexcept { return m_focusable; }

            /**
             * @brief Check if this target is enabled and can process events
             * @note Exception safety: No-throw guarantee (noexcept)
             */
            [[nodiscard]] bool is_enabled() const noexcept { return m_enabled; }

            /**
             * @brief Get the tab navigation order index
             * @note Exception safety: No-throw guarantee (noexcept)
             */
            [[nodiscard]] int tab_index() const noexcept { return m_tab_index; }

            // -----------------------------------------------------------------------
            // State Setters
            // -----------------------------------------------------------------------

            /**
             * @brief Set whether this target can receive keyboard focus
             * @note Exception safety: No-throw guarantee (noexcept)
             */
            void set_focusable(bool focusable) noexcept { m_focusable = focusable; }

            /**
             * @brief Set whether this target is enabled and can process events
             * @note Exception safety: No-throw guarantee (noexcept)
             * @note When disabled, process_event() returns false immediately
             */
            void set_enabled(bool enabled) noexcept { m_enabled = enabled; }

            /**
             * @brief Set the tab navigation order index
             * @note Exception safety: No-throw guarantee (noexcept)
             * @note Lower indices receive focus first during tab navigation
             */
            void set_tab_index(int index) noexcept { m_tab_index = index; }

            // -----------------------------------------------------------------------
            // Virtual Handlers - Override these in derived classes
            // -----------------------------------------------------------------------
        protected:
            event_target() = default;
            /**
             * @brief Handle mouse enter event
             */
            virtual bool handle_mouse_enter();

            /**
             * @brief Handle mouse leave event
             */
            virtual bool handle_mouse_leave();

            /**
             * @brief Handle mouse move event
             */
            virtual bool handle_mouse_move(int x, int y);

            /**
             * @brief Handle mouse button down event
             */
            virtual bool handle_mouse_down(int x, int y, int button);

            /**
             * @brief Handle mouse button up event
             */
            virtual bool handle_mouse_up(int x, int y, int button);

            /**
             * @brief Handle click event (generated from down+up)
             */
            virtual bool handle_click(int x, int y);

            /**
             * @brief Handle mouse wheel event
             */
            virtual bool handle_wheel(float delta_x, float delta_y);

            /**
             * @brief Handle key down event
             */
            virtual bool handle_key_down(int key, bool shift, bool ctrl, bool alt);

            /**
             * @brief Handle key up event
             */
            virtual bool handle_key_up(int key, bool shift, bool ctrl, bool alt);

            /**
             * @brief Handle text input event
             */
            virtual bool handle_text_input(std::string_view text);

            /**
             * @brief Handle focus gained event
             */
            virtual bool handle_focus_gained();

            /**
             * @brief Handle focus lost event
             */
            virtual bool handle_focus_lost();

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
            [[nodiscard]] virtual bool is_inside(int x, int y) const = 0;

            /**
             * @brief Set whether Enter/Space keys trigger click events
             */
            void set_accept_keys_as_click(bool accept) noexcept {
                m_accept_keys_as_click = accept;
            }

        private:
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

            // -----------------------------------------------------------------------
            // Internal Processing (State Management)
            // -----------------------------------------------------------------------

            /**
             * @brief Process mouse movement and track hover state
             */
            bool process_mouse_move(int x, int y);

            /**
             * @brief Process mouse button and track pressed state
             */
            bool process_mouse_button(int x, int y, int button, bool pressed);

            /**
             * @brief Process keyboard input
             */
            template<typename E>
            requires KeyboardEvent<E>
            bool process_key(const E& event);

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

    // ========================================================================================
    // Implementation
    // ========================================================================================

    template<UIBackend Backend>
    template<typename E>
    bool event_target<Backend>::process_event_impl(const E& event) {
        if (!m_enabled) {
            return false;
        }

        // Handle mouse button events FIRST (more specific than position events)
        // Note: Remove 'else' to allow fall-through for events that satisfy both concepts
        if constexpr (MouseButtonEvent<E>) {
            int button = event_traits<E>::mouse_button(event);
            bool is_press = event_traits<E>::is_button_press(event);

            // Runtime check: only treat as button event if button != 0 or it's a press
            // This handles backends where the same type is used for button and move events
            if (button != 0 || is_press) {
                int x = event_traits<E>::mouse_x(event);
                int y = event_traits<E>::mouse_y(event);
                return process_mouse_button(x, y, button, is_press);
            }
            // Fall through to position event handling if not a real button event
        }
        // Handle mouse position events (motion)
        if constexpr (MousePositionEvent <E>) {
            int x = event_traits <E>::mouse_x(event);
            int y = event_traits <E>::mouse_y(event);
            return process_mouse_move(x, y);
        }
        // Handle mouse wheel events
        else if constexpr (MouseWheelEvent <E>) {
            float dx = event_traits <E>::wheel_delta_x(event);
            float dy = event_traits <E>::wheel_delta_y(event);
            return handle_wheel(dx, dy);
        }
        // Handle keyboard events
        else if constexpr (KeyboardEvent <E>) {
            return process_key(event);
        }
        // Handle text input events
        else if constexpr (TextInputEvent <E>) {
            auto text = event_traits <E>::text(event);
            return handle_text_input(text);
        }

        return false;
    }

    // -------------------------------------------------------------------------------
    template<UIBackend Backend>
    bool event_target<Backend>::handle_mouse_enter() {
        if (m_on_mouse_enter) return m_on_mouse_enter();
        return false;
    }

    template<UIBackend Backend>
    bool event_target<Backend>::handle_mouse_leave() {
        if (m_on_mouse_leave) return m_on_mouse_leave();
        return false;
    }

    template<UIBackend Backend>
    bool event_target<Backend>::handle_mouse_move(int x, int y) {
        if (m_on_mouse_move) return m_on_mouse_move(x, y);
        return false;
    }

    template<UIBackend Backend>
    bool event_target<Backend>::handle_mouse_down(int x, int y, int button) {
        if (m_on_mouse_down) return m_on_mouse_down(x, y, button);
        return false;
    }

    template<UIBackend Backend>
    bool event_target<Backend>::handle_mouse_up(int x, int y, int button) {
        if (m_on_mouse_up) return m_on_mouse_up(x, y, button);
        return false;
    }

    template<UIBackend Backend>
    bool event_target<Backend>::handle_click(int x, int y) {
        if (m_on_click) return m_on_click(x, y);
        return false;
    }

    template<UIBackend Backend>
    bool event_target<Backend>::handle_wheel(float delta_x, float delta_y) {
        if (m_on_wheel) return m_on_wheel(delta_x, delta_y);
        return false;
    }

    template<UIBackend Backend>
    bool event_target<Backend>::handle_key_down(int key, bool shift, bool ctrl, bool alt) {
        if (m_on_key_down) return m_on_key_down(key, shift, ctrl, alt);
        return false;
    }

    template<UIBackend Backend>
    bool event_target<Backend>::handle_key_up(int key, bool shift, bool ctrl, bool alt) {
        if (m_on_key_up) return m_on_key_up(key, shift, ctrl, alt);
        return false;
    }

    template<UIBackend Backend>
    bool event_target<Backend>::handle_text_input(std::string_view text) {
        if (m_on_text_input) return m_on_text_input(text);
        return false;
    }

    template<UIBackend Backend>
    bool event_target<Backend>::handle_focus_gained() {
        m_has_focus = true;
        if (m_on_focus_gained) return m_on_focus_gained();
        return false;
    }

    template<UIBackend Backend>
    bool event_target<Backend>::handle_focus_lost() {
        m_has_focus = false;
        m_is_pressed = false; // Clear pressed state when losing focus
        if (m_on_focus_lost) return m_on_focus_lost();
        return false;
    }

    template<UIBackend Backend>
    bool event_target<Backend>::process_mouse_move(int x, int y) {
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

    template<UIBackend Backend>
    bool event_target<Backend>::process_mouse_button(int x, int y, int button, bool pressed) {
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

    template<UIBackend Backend>
    template<typename E>
    requires KeyboardEvent<E>
    bool event_target<Backend>::process_key(const E& event) {
        if (!m_has_focus) {
            return false;
        }

        // Extract basic key information
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

        if (pressed) {
            // Handle Enter/Space as click for focused elements
            if (m_accept_keys_as_click && !repeat) {
                // Use event traits to identify keys instead of hardcoded values
                if (event_traits<E>::is_enter_key(event) ||
                    event_traits<E>::is_space_key(event)) {
                    // Generate a click at the center (derived class can override)
                    return handle_click(0, 0);
                }
            }
            return handle_key_down(key, shift, ctrl, alt);
        } else {
            return handle_key_up(key, shift, ctrl, alt);
        }
    }


}