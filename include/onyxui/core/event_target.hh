/**
 * @file event_target.hh
 * @brief Base class for three-phase event handling
 * @author igor
 * @date 10/10/2025
 *
 * This file defines event_target, which provides three-phase event handling
 * (CAPTURE → TARGET → BUBBLE) independent of any UI or layout concerns.
 *
 * ## Architecture
 *
 * The event_target class provides clean, single-responsibility event handling:
 *
 * **Three-Phase Event Routing:**
 * - CAPTURE: Event travels down from root to target (parent-first)
 * - TARGET: Event delivered to target element
 * - BUBBLE: Event travels up from target to root (child-first)
 *
 * **Virtual Handler Layer:**
 * - `handle_event()` - Main entry point, dispatches by phase
 * - `handle_keyboard()` - Override to handle keyboard events
 * - `handle_mouse()` - Override to handle mouse events
 * - `handle_resize()` - Override to handle resize events
 *
 * ## Event Flow
 *
 * 1. Event arrives via `handle_event(ui_event, event_phase)`
 * 2. Base class checks phase and enabled state
 * 3. Event dispatched to type-specific handler (keyboard/mouse/resize)
 * 4. Derived class override handles event
 * 5. Event consumed flag returned
 *
 * ## Automatic State Tracking
 *
 * The base class tracks states automatically in handle_mouse():
 * - **Hover**: Updated based on `is_inside()` checks
 * - **Pressed**: Tracked from press to release
 * - **Click**: Generated when press and release both inside
 * - **Focus**: Managed by focus_manager (friend class)
 * - **Enabled**: Controls whether events are processed
 */

#pragma once

#include <functional>
#include <variant>
#include <onyxui/concepts/backend.hh>
#include <onyxui/concepts/event_like.hh>
#include <onyxui/events/ui_event.hh>
#include <onyxui/events/event_phase.hh>
#include <onyxui/hotkeys/hotkey_action.hh>

namespace onyxui {
    // Forward declarations for friend declarations
    template<UIBackend Backend> class focus_manager;
    template<UIBackend Backend> class input_manager;
    template<UIBackend Backend> class ui_handle;
    template<UIBackend Backend> class ui_context;

    /**
     * @class event_target
     * @brief Base class providing three-phase event handling capabilities
     *
     * This class is completely independent of ui_element or any layout concerns.
     * It provides three-phase event routing (CAPTURE → TARGET → BUBBLE) with
     * automatic state tracking for hover, pressed, and focus states.
     *
     * @tparam Backend The backend traits type (sdl_backend, glfw_backend, etc.)
     *
     * @example Basic event handling
     * @code
     * class my_widget : public event_target<test_backend> {
     * public:
     *     bool handle_mouse(const mouse_event& evt) override {
     *         if (evt.act == mouse_event::action::press) {
     *             std::cout << "Clicked at " << evt.x << ", " << evt.y << "\n";
     *             return true;  // Consumed
     *         }
     *         return false;
     *     }
     *
     *     bool is_inside(int x, int y) const override {
     *         return x >= 0 && x < 100 && y >= 0 && y < 50;
     *     }
     * };
     * @endcode
     */
    template<UIBackend Backend>
    class event_target {
        // Allow input_manager to access protected focus and mouse methods
        friend class input_manager<Backend>;
        // Allow focus_manager to access protected focus methods (backward compat)
        friend class focus_manager<Backend>;
        // Allow ui_handle to access protected mouse event handlers
        friend class ui_handle<Backend>;
        // Allow ui_context to access protected handle_click for semantic actions
        friend class ui_context<Backend>;

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
             * @brief Process backend event (transitional stub for compatibility)
             * @param event Backend-specific event
             * @return true if handled
             *
             * @details
             * Transitional method for backward compatibility with code that hasn't
             * migrated to ui_event yet. Always returns false - backends should use
             * the new handle_event(ui_event, event_phase) API instead.
             *
             * @deprecated Use handle_event(ui_event, event_phase) instead
             */
            virtual bool process_event([[maybe_unused]] const event_type& event) {
                // Stub: old API no longer supported
                // Layer manager and ui_handle should convert to ui_event first
                return false;
            }

            /**
             * @brief Handle event with phase information (capture/target/bubble)
             * @param evt The event to handle
             * @param phase Which phase of event propagation this is
             * @return true if event was handled/consumed
             *
             * @details
             * Three-phase event routing following DOM/WPF model:
             * - CAPTURE: Event traveling down from root to target (parent first)
             * - TARGET: Event delivered to target element
             * - BUBBLE: Event traveling up from target to root (child first)
             *
             * Default implementation only handles events in TARGET phase by
             * dispatching to type-specific handlers via handle_keyboard(),
             * handle_mouse(), and handle_resize(). Override to handle
             * capture/bubble phases for advanced scenarios.
             *
             * ## Type Dispatch (TARGET phase only in base implementation)
             *
             * Uses std::visit to dispatch to the appropriate handler:
             * - keyboard_event: Extracted and passed to handle_keyboard()
             * - mouse_event: Coordinates/button/action passed to handle_mouse_*()
             * - resize_event: Dimensions passed to handle_resize()
             *
             * @example
             * @code
             * // Override for capture phase handling (e.g., focus on click)
             * bool handle_event(const ui_event& evt, event_phase phase) override {
             *     if (phase == event_phase::capture) {
             *         if (auto* mouse = std::get_if<mouse_event>(&evt)) {
             *             if (mouse->act == mouse_event::action::press) {
             *                 ui_services<Backend>::input()->set_focus(this);
             *                 return false;  // Continue to children
             *             }
             *         }
             *     }
             *     return base::handle_event(evt, phase);
             * }
             * @endcode
             *
             * @see event_phase For phase documentation
             * @see handle_keyboard For keyboard event handling
             * @see handle_mouse For mouse event handling
             * @see handle_resize For resize event handling
             */
            virtual bool handle_event(const ui_event& evt, event_phase phase);

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
             * @brief Check if this target accepts keyboard keys as clicks
             * @return True if Enter/Space (or keys bound to activate_focused) trigger clicks
             * @note Exception safety: No-throw guarantee (noexcept)
             */
            [[nodiscard]] bool accepts_keys_as_click() const noexcept { return m_accept_keys_as_click; }

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

            /**
             * @brief Handle semantic action event
             * @param action The semantic action to handle
             * @return true if event was consumed
             *
             * @details
             * Override this method to handle semantic actions (menu navigation, scrolling, etc.).
             * Default implementation returns false (not handled).
             *
             * Semantic actions are dispatched by hotkey_manager to the focused widget.
             * Each widget handles the actions it understands and ignores the rest.
             *
             * This enables context-dependent behavior where the same key can trigger
             * different actions based on which widget has focus:
             * - Up arrow in text_view → scroll_up
             * - Up arrow in menu → menu_up
             *
             * @example
             * @code
             * bool handle_semantic_action(hotkey_action action) override {
             *     if (action == hotkey_action::scroll_up) {
             *         scroll_by(0, -1);
             *         return true;
             *     }
             *     return base::handle_semantic_action(action); // Let parent try
             * }
             * @endcode
             */
            virtual bool handle_semantic_action(hotkey_action action);

            // -----------------------------------------------------------------------
            // Virtual Event Handlers - Override these in derived classes
            // -----------------------------------------------------------------------
        protected:
            event_target() = default;

            /**
             * @brief Handle keyboard event
             * @param kbd Framework-level keyboard event
             * @return true if event was consumed
             *
             * @details
             * Override this method to handle keyboard events in derived classes.
             * Default implementation handles Enter/Space as click if m_accept_keys_as_click is true.
             *
             * @example
             * @code
             * bool handle_keyboard(const keyboard_event& kbd) override {
             *     if (kbd.key == key::escape) {
             *         close();
             *         return true;
             *     }
             *     return base::handle_keyboard(kbd);
             * }
             * @endcode
             */
            virtual bool handle_keyboard(const keyboard_event& kbd);

            /**
             * @brief Handle mouse event
             * @param mouse Framework-level mouse event
             * @return true if event was consumed
             *
             * @details
             * Override this method to handle mouse events in derived classes.
             * Default implementation tracks hover/pressed state and generates click events.
             */
            virtual bool handle_mouse(const mouse_event& mouse);

            /**
             * @brief Handle resize event
             * @param resize Framework-level resize event
             * @return true if event was consumed
             *
             * @details
             * Override this method to handle resize events in derived classes.
             * Default implementation returns false (most widgets don't care about resize).
             */
            virtual bool handle_resize(const resize_event& resize);

            /**
             * @brief Called when focus state changes
             * @param gained True if focus gained, false if lost
             *
             * @details
             * Override this method to respond to focus changes (e.g., emit signals, mark dirty).
             * Default implementation does nothing.
             *
             * @note This is NOT an event - it's a state change notification from focus_manager.
             */
            virtual void on_focus_changed(bool gained) {
                (void)gained;  // Base implementation does nothing
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
            [[nodiscard]] virtual bool is_inside(int x, int y) const = 0;

            /**
             * @brief Set whether Enter/Space keys trigger click events
             */
            void set_accept_keys_as_click(bool accept) noexcept {
                m_accept_keys_as_click = accept;
            }

            /**
             * @brief Manually set focus state
             *
             * This should typically be called by a focus manager, not directly.
             * Made protected to allow derived test classes to set focus for testing.
             */
            void set_focus(bool focus);

        private:

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
    void event_target<Backend>::set_focus(bool focus) {
        if (focus && !m_has_focus) {
            m_has_focus = true;
            on_focus_changed(true);  // Notify derived class
        } else if (!focus && m_has_focus) {
            m_has_focus = false;
            m_is_pressed = false;  // Clear pressed state when losing focus
            on_focus_changed(false);  // Notify derived class
        }
    }

    template<UIBackend Backend>
    bool event_target<Backend>::handle_semantic_action([[maybe_unused]] hotkey_action action) {
        // Base implementation does nothing
        // Derived classes override to handle actions they understand
        return false;
    }

    // ===============================================================================
    // Phase-Aware Event Handling
    // ===============================================================================

    template<UIBackend Backend>
    bool event_target<Backend>::handle_event(const ui_event& evt, event_phase phase) {
        // Default: only handle in target phase
        // Derived classes override to handle capture/bubble phases
        if (phase != event_phase::target) {
            return false;  // Ignore capture/bubble by default
        }

        if (!m_enabled) {
            return false;
        }

        // Dispatch based on event type using std::visit
        return std::visit([this](auto&& e) -> bool {
            using T = std::decay_t<decltype(e)>;
            if constexpr (std::is_same_v<T, keyboard_event>) {
                return this->handle_keyboard(e);
            } else if constexpr (std::is_same_v<T, mouse_event>) {
                return this->handle_mouse(e);
            } else if constexpr (std::is_same_v<T, resize_event>) {
                return this->handle_resize(e);
            }
            return false;  // Unknown event type
        }, evt);
    }

    template<UIBackend Backend>
    bool event_target<Backend>::handle_keyboard(const keyboard_event& kbd) {
        if (!m_has_focus) {
            return false;
        }

        // Handle Enter/Space as click for focused elements (e.g., buttons)
        // Note: Derived classes (widget) override this to emit the clicked signal
        if (m_accept_keys_as_click) {
            if (kbd.key == key_code::enter || kbd.key == key_code::space) {
                if (kbd.pressed) {
                    // Key down: Set pressed state for visual feedback
                    m_is_pressed = true;
                    return true;  // Handled
                } else {
                    // Key up: Clear pressed state
                    // Derived classes should override to emit clicked signal here
                    m_is_pressed = false;
                    return true;  // Handled
                }
            }
        }

        return false;  // Base class doesn't handle any other keys
    }

    template<UIBackend Backend>
    bool event_target<Backend>::handle_mouse(const mouse_event& mouse) {
        int const x = mouse.x;
        int const y = mouse.y;
        bool const in_bounds = is_inside(x, y);

        // Track hover state for visual feedback
        m_is_hovered = in_bounds;

        // Dispatch based on action type
        switch (mouse.act) {
            case mouse_event::action::press:
                // Track press state for click generation
                m_is_pressed = in_bounds;
                m_press_started_inside = in_bounds;
                return in_bounds;  // Consume if inside

            case mouse_event::action::release: {
                bool const was_pressed = m_is_pressed;
                m_is_pressed = false;

                // Generate click if pressed and released inside (CRITICAL!)
                // Derived classes should override to handle the actual click
                bool handled = (was_pressed && m_press_started_inside && in_bounds);

                m_press_started_inside = false;
                return handled;
            }

            case mouse_event::action::move:
                // Generate enter/leave events through hover state changes
                // Derived classes can override to handle enter/leave/move
                return false;  // Base class doesn't consume move events

            case mouse_event::action::wheel_up:
            case mouse_event::action::wheel_down:
                return false;  // Base class doesn't handle wheel
        }

        return false;
    }

    template<UIBackend Backend>
    bool event_target<Backend>::handle_resize([[maybe_unused]] const resize_event& resize) {
        // Most widgets don't care about resize events
        // Root elements and containers can override this
        return false;
    }

}