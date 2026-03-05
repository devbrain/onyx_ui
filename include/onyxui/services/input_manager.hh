/**
 * @file input_manager.hh
 * @brief Unified manager for keyboard focus, mouse capture, and hover state
 * @author igor
 * @date 2025-10-25
 *
 * @details
 * The input_manager consolidates all interactive input state management:
 * - **Keyboard focus**: Which widget receives keyboard events
 * - **Mouse capture**: Which widget receives mouse events during drag operations
 * - **Hover state**: Which widget the mouse cursor is currently over
 *
 * ## Why Unified Management?
 *
 * These states are tightly coupled and must coordinate:
 * - Mouse down on widget → Sets BOTH focus (if focusable) and capture
 * - Focus changes via Tab → Releases mouse capture (prevents stuck state)
 * - Widget disabled/destroyed → Clears ALL states it holds
 *
 * ## Design Principles
 *
 * 1. **Single Source of Truth**: All input state in one manager
 * 2. **State Consistency**: Coordinated updates prevent desync bugs
 * 3. **Backend Agnostic**: Works with any backend via event_target
 * 4. **Thread Safety**: Can be made thread-safe at manager level
 *
 * ## State Coordination Rules
 *
 * - `set_capture(widget)` → Also sets focus (if widget is focusable)
 * - `set_focus(widget)` → Releases capture if different widget
 * - `release_capture()` → Only releases capture, preserves focus
 * - `clear_focus()` → Releases focus, also releases capture
 * - Widget destruction → Must call `release_widget()` to clear all states
 *
 * @example Basic Usage
 * @code
 * input_manager<Backend> input;
 *
 * // Mouse down on button
 * input.set_capture(button);  // Also sets focus if focusable
 *
 * // Mouse up
 * input.release_capture();    // Button keeps focus
 *
 * // Tab to next widget
 * input.set_focus(next_button);  // Releases any capture
 * @endcode
 */

#pragma once

#include <onyxui/core/event_target.hh>
#include <onyxui/core/element.hh>  // for ui_element::destroying signal
#include <onyxui/core/signal.hh>   // for scoped_connection
#include <onyxui/concepts/backend.hh>
#include <onyxui/concepts/event_like.hh>
#include <utility>   // for std::exchange
#include <vector>    // for std::vector
#include <algorithm> // for std::find

namespace onyxui {

    /**
     * @class input_manager
     * @brief Manages keyboard focus, mouse capture, and hover state
     *
     * @tparam Backend The UI backend type
     *
     * @details
     * Consolidates focus_manager functionality with mouse state management.
     * Ensures consistent state updates when switching between widgets.
     */
    template<UIBackend Backend>
    class input_manager {
    public:
        using target_type = event_target<Backend>;
        using target_ptr = target_type*;

        /**
         * @brief Default constructor
         */
        input_manager() = default;

        /**
         * @brief Destructor - clears all state without callbacks
         *
         * @details
         * Resets all pointers without calling callbacks on targets.
         * Targets may have already been destroyed, so callbacks would be unsafe.
         */
        ~input_manager() noexcept {
            m_focused = nullptr;
            m_captured = nullptr;
            m_hovered = nullptr;
        }

        // Delete copy operations (state is unique per manager)
        input_manager(const input_manager&) = delete;
        input_manager& operator=(const input_manager&) = delete;

        /**
         * @brief Move constructor - transfers all state and connections
         */
        input_manager(input_manager&& other) noexcept
            : m_focused(std::exchange(other.m_focused, nullptr))
            , m_captured(std::exchange(other.m_captured, nullptr))
            , m_hovered(std::exchange(other.m_hovered, nullptr))
            , m_focused_conn(std::move(other.m_focused_conn))
            , m_captured_conn(std::move(other.m_captured_conn))
            , m_hovered_conn(std::move(other.m_hovered_conn)) {
        }

        /**
         * @brief Move assignment - transfers all state and connections
         */
        input_manager& operator=(input_manager&& other) noexcept {
            if (this != &other) {
                m_focused = std::exchange(other.m_focused, nullptr);
                m_captured = std::exchange(other.m_captured, nullptr);
                m_hovered = std::exchange(other.m_hovered, nullptr);
                m_focused_conn = std::move(other.m_focused_conn);
                m_captured_conn = std::move(other.m_captured_conn);
                m_hovered_conn = std::move(other.m_hovered_conn);
            }
            return *this;
        }

        // ====================================================================
        // Focus Management
        // ====================================================================

        /**
         * @brief Set keyboard focus to a widget
         * @param target The widget to focus (or nullptr to clear focus)
         * @return true if focus changed, false if already focused or non-focusable
         *
         * @details
         * **Coordination**: If focus changes to a different widget, releases mouse capture.
         * This prevents stuck capture when user Tabs away during a drag.
         */
        bool set_focus(target_ptr target) {
            // Validate target is focusable
            if (target && !target->is_focusable()) {
                return false;
            }
            if (target && !target->is_enabled()) {
                return false;
            }

            // Already focused?
            if (m_focused == target) {
                return false;
            }

            // Release capture if focus is changing to a different widget
            // (Prevents stuck capture when Tab-ing away during drag)
            if (m_captured && m_captured != target) {
                release_capture();
            }

            // Notify old focused widget
            if (m_focused) {
                m_focused->set_focus(false);
            }

            // Update focus and connect to destroying signal
            m_focused = target;
            connect_destroying(target, m_focused_conn, m_focused);

            // Notify new focused widget
            if (m_focused) {
                m_focused->set_focus(true);
            }

            return true;
        }

        /**
         * @brief Clear keyboard focus
         *
         * @details
         * Also releases mouse capture for consistency.
         */
        void clear_focus() {
            set_focus(nullptr);
        }

        /**
         * @brief Get currently focused widget
         * @return Pointer to focused widget, or nullptr if none
         */
        [[nodiscard]] target_ptr get_focused() const noexcept {
            return m_focused;
        }

        /**
         * @brief Check if a widget has focus
         * @param target Widget to check
         * @return true if target has focus
         */
        [[nodiscard]] bool has_focus(target_ptr target) const noexcept {
            return m_focused == target;
        }

        // ====================================================================
        // Mouse Capture Management
        // ====================================================================

        /**
         * @brief Capture mouse input to a widget
         * @param target Widget to capture mouse (or nullptr to release)
         * @return true if capture changed, false if already captured
         *
         * @details
         * **Coordination**: Capturing mouse also sets focus (if widget is focusable).
         * This ensures pressed buttons receive keyboard events.
         *
         * Typical usage:
         * - Mouse down → set_capture(button)
         * - Mouse up → release_capture()
         */
        bool set_capture(target_ptr target) {
            if (m_captured == target) {
                return false;  // Already captured
            }

            // Update capture and connect to destroying signal
            m_captured = target;
            connect_destroying(target, m_captured_conn, m_captured);

            // Capture implies focus (if focusable)
            if (target && target->is_focusable() && target->is_enabled()) {
                set_focus(target);
            }

            return true;
        }

        /**
         * @brief Release mouse capture
         *
         * @details
         * Does NOT clear focus - widget keeps focus after mouse release.
         */
        void release_capture() {
            m_captured = nullptr;
        }

        /**
         * @brief Handle capture transfer when pressing on a different target
         *
         * @param new_target The widget being pressed (may be nullptr)
         * @param synthesize_release_fn Callback to send synthetic release to old capture
         * @return true if a synthetic release was sent, false otherwise
         *
         * @details
         * ## WORKAROUND FOR TERMINAL BACKENDS
         *
         * Some backends (like termbox2) don't send mouse release events due to
         * terminal limitations. This is expected behavior, not a bug.
         *
         * When pressing on a different widget than what's captured, we must:
         * 1. Send a synthetic release to the old captured widget
         * 2. Release the capture
         * 3. Allow normal press handling to proceed
         *
         * This ensures widgets can clean up their pressed state properly.
         *
         * ## Usage
         *
         * @code
         * // In event handler:
         * if (mouse_evt->act == mouse_event::action::press) {
         *     input.handle_capture_transfer_on_press(
         *         target_widget,
         *         [&](target_ptr old_capture) {
         *             // Send synthetic release to old capture
         *             mouse_event release{...};
         *             old_capture->handle_event(release, event_phase::target);
         *         }
         *     );
         * }
         * @endcode
         *
         * ## Why a callback?
         *
         * The input_manager doesn't know about mouse_event or handle_event().
         * The callback lets callers construct the proper event type for their context.
         */
        template<typename ReleaseFn>
        bool handle_capture_transfer_on_press(target_ptr new_target, ReleaseFn&& synthesize_release_fn) {
            auto* old_capture = m_captured;

            // No capture or same target - nothing to do
            if (!old_capture || old_capture == new_target) {
                return false;
            }

            // Different target while something is captured - synthesize release
            synthesize_release_fn(old_capture);
            release_capture();
            return true;
        }

        /**
         * @brief Get currently captured widget
         * @return Pointer to captured widget, or nullptr if none
         */
        [[nodiscard]] target_ptr get_captured() const noexcept {
            return m_captured;
        }

        /**
         * @brief Check if a widget has mouse capture
         * @param target Widget to check
         * @return true if target has capture
         */
        [[nodiscard]] bool has_capture(target_ptr target) const noexcept {
            return m_captured == target;
        }

        // ====================================================================
        // Hover State Management
        // ====================================================================

        /**
         * @brief Set hovered widget (widget under mouse cursor)
         * @param target Widget currently under mouse (or nullptr if none)
         * @return true if hover changed, false if already hovered
         *
         * @details
         * Hover state is independent of focus and capture.
         * Used for visual feedback (highlights, tooltips).
         */
        bool set_hover(target_ptr target) {
            if (m_hovered == target) {
                return false;  // Already hovered
            }

            // CRITICAL: Clear hover state on previously hovered widget
            // This fixes the issue where widgets stay highlighted after mouse moves away,
            // particularly visible with sdlpp backend's continuous mouse move events.
            // The old widget won't receive a mouse event (it's not in the hit test path),
            // so we must explicitly clear its state here.
            // Note: Safe to call - we're changing hover, not responding to destruction
            if (m_hovered) {
                m_hovered->reset_hover_and_press_state();
            }

            // Update hover and connect to destroying signal
            m_hovered = target;
            connect_destroying(target, m_hovered_conn, m_hovered);

            return true;
        }

        /**
         * @brief Clear hover state
         */
        void clear_hover() {
            set_hover(nullptr);
        }

        /**
         * @brief Get currently hovered widget
         * @return Pointer to hovered widget, or nullptr if none
         */
        [[nodiscard]] target_ptr get_hovered() const noexcept {
            return m_hovered;
        }

        /**
         * @brief Check if a widget is hovered
         * @param target Widget to check
         * @return true if target is hovered
         */
        [[nodiscard]] bool has_hover(target_ptr target) const noexcept {
            return m_hovered == target;
        }

        // ====================================================================
        // Widget Lifecycle Management
        // ====================================================================

        /**
         * @brief Release all states for a widget being destroyed
         * @param target Widget being destroyed
         *
         * @details
         * Clears focus, capture, and hover if widget holds any of these states.
         * Does NOT call callbacks - widget is being destroyed.
         *
         * **Note**: For ui_element targets, this is called automatically via the
         * `destroying` signal - no manual call needed. Only required for non-ui_element
         * event_target implementations (rare).
         */
        void release_widget(target_ptr target) noexcept {
            if (m_focused == target) {
                m_focused = nullptr;
            }
            if (m_captured == target) {
                m_captured = nullptr;
            }
            if (m_hovered == target) {
                m_hovered = nullptr;
            }
        }

    private:
        using element_type = ui_element<Backend>;

        /**
         * @brief Connect to an element's destroying signal
         * @param element Element to track (may be nullptr)
         * @param conn Connection to update
         * @param ptr_to_clear Pointer to set to nullptr when element is destroyed
         *
         * @details
         * Uses the RTTI-free as_ui_element() virtual method to safely check if
         * the target is a ui_element. If not (rare case of non-ui_element
         * event_target), the connection is skipped and release_widget() must
         * be called manually on destruction.
         */
        void connect_destroying(target_ptr element, scoped_connection& conn, target_ptr& ptr_to_clear) {
            if (!element) {
                conn = scoped_connection{};  // Disconnect any existing
                return;
            }

            // Use RTTI-free as_ui_element() to check if target is a ui_element
            // Non-ui_element targets (rare) fall back to manual release_widget() on destruction
            auto* ui_elem = element->as_ui_element();
            if (ui_elem) {
                conn = scoped_connection(ui_elem->destroying, [&ptr_to_clear](element_type*) {
                    // Element is being destroyed - clear our pointer WITHOUT calling methods
                    ptr_to_clear = nullptr;
                });
            } else {
                conn = scoped_connection{};  // No signal available
            }
        }

        target_ptr m_focused = nullptr;   ///< Widget with keyboard focus
        target_ptr m_captured = nullptr;  ///< Widget that captured mouse during drag
        target_ptr m_hovered = nullptr;   ///< Widget under mouse cursor

        // Connections to destroying signals - automatically cleared when element dies
        scoped_connection m_focused_conn;   ///< Connection to focused element's destroying signal
        scoped_connection m_captured_conn;  ///< Connection to captured element's destroying signal
        scoped_connection m_hovered_conn;   ///< Connection to hovered element's destroying signal
    };

    /**
     * @class hierarchical_input_manager
     * @brief Extended input manager that works with tree hierarchies
     *
     * @tparam Backend The UI backend type
     * @tparam ElementType The element type (must have children() method)
     *
     * @details
     * Adds tree traversal for Tab navigation. Useful for ui_element hierarchies.
     * Inherits all focus/capture/hover functionality from input_manager.
     *
     * @example
     * @code
     * hierarchical_input_manager<Backend, ui_element<Backend>> input;
     * input.focus_next_in_tree(root);  // Tab to next widget in tree
     * @endcode
     */
    template<UIBackend Backend, typename ElementType>
    class hierarchical_input_manager : public input_manager<Backend> {
    public:
        using base_type = input_manager<Backend>;
        using element_ptr = ElementType*;
        using target_ptr = typename base_type::target_ptr;

        /**
         * @brief Move focus to next element in tree (Tab key)
         * @param root The root element to search from
         */
        void focus_next_in_tree(element_ptr root) {
            auto targets = collect_all_targets(root);
            auto focusable = collect_focusable_targets(targets);

            if (focusable.empty()) return;

            // Find current focused target in list
            auto current = this->get_focused();
            auto it = std::find(focusable.begin(), focusable.end(), current);

            // Move to next (wrap around if at end)
            if (it == focusable.end()) {
                // Not found, start from beginning
                it = focusable.begin();
            } else {
                // Move to next
                ++it;
                // Wrap around if at end
                if (it == focusable.end()) {
                    it = focusable.begin();
                }
            }

            this->set_focus(*it);
        }

        /**
         * @brief Move focus to previous element in tree (Shift+Tab key)
         * @param root The root element to search from
         */
        void focus_previous_in_tree(element_ptr root) {
            auto targets = collect_all_targets(root);
            auto focusable = collect_focusable_targets(targets);

            if (focusable.empty()) return;

            // Find current focused target in list
            auto current = this->get_focused();
            auto it = std::find(focusable.begin(), focusable.end(), current);

            // Move to previous (wrap around if at beginning)
            if (it == focusable.end() || it == focusable.begin()) {
                it = focusable.end();
            }
            --it;

            this->set_focus(*it);
        }

        /**
         * @brief Handle Tab navigation key events
         * @param event The keyboard event
         * @param root The root element to search from
         * @return true if Tab key was handled, false otherwise
         *
         * @details
         * Checks if the event is Tab or Shift+Tab and navigates focus accordingly.
         * Returns false if the event is not a Tab key (allowing other handlers to process it).
         */
        template<typename EventType>
        bool handle_tab_navigation_in_tree(const EventType& event, element_ptr root) {
            if constexpr (KeyboardEvent<EventType>) {
                // Check if this is a Tab key
                if (!event_traits<EventType>::is_tab_key(event)) {
                    return false;  // Not a Tab key
                }

                // Check for Shift modifier (reverse direction)
                bool shift = false;
                if constexpr (ModifierState<EventType>) {
                    shift = event_traits<EventType>::shift_pressed(event);
                }

                if (shift) {
                    focus_previous_in_tree(root);
                } else {
                    focus_next_in_tree(root);
                }
                return true;  // Tab handled
            }
            return false;  // Not a keyboard event
        }

    private:
        /**
         * @brief Collect all targets from tree (depth-first)
         * @param root Root element to traverse
         * @return Vector of all targets in tree
         */
        std::vector<target_ptr> collect_all_targets(element_ptr root) {
            std::vector<target_ptr> result;

            if (!root) return result;

            // Add root
            result.push_back(static_cast<target_ptr>(root));

            // Recursively add children
            for (auto& child : root->children()) {
                auto child_targets = collect_all_targets(child.get());
                result.insert(result.end(), child_targets.begin(), child_targets.end());
            }

            return result;
        }

        /**
         * @brief Filter targets to only focusable ones
         * @param targets All targets
         * @return Focusable targets only
         */
        std::vector<target_ptr> collect_focusable_targets(const std::vector<target_ptr>& targets) {
            std::vector<target_ptr> result;

            for (auto* target : targets) {
                if (target && target->is_focusable() && target->is_enabled()) {
                    result.push_back(target);
                }
            }

            return result;
        }
    };

} // namespace onyxui
