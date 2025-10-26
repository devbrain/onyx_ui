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

#include <onyxui/event_target.hh>
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
         * @brief Move constructor - transfers all state
         */
        input_manager(input_manager&& other) noexcept
            : m_focused(std::exchange(other.m_focused, nullptr))
            , m_captured(std::exchange(other.m_captured, nullptr))
            , m_hovered(std::exchange(other.m_hovered, nullptr)) {
        }

        /**
         * @brief Move assignment - transfers all state
         */
        input_manager& operator=(input_manager&& other) noexcept {
            if (this != &other) {
                m_focused = std::exchange(other.m_focused, nullptr);
                m_captured = std::exchange(other.m_captured, nullptr);
                m_hovered = std::exchange(other.m_hovered, nullptr);
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
                m_focused->handle_focus_lost();
            }

            // Update focus
            m_focused = target;

            // Notify new focused widget
            if (m_focused) {
                m_focused->handle_focus_gained();
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

            m_captured = target;

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

            // Notify old hovered widget
            if (m_hovered) {
                m_hovered->handle_mouse_leave();
            }

            // Update hover
            m_hovered = target;

            // Notify new hovered widget
            if (m_hovered) {
                m_hovered->handle_mouse_enter();
            }

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
         * **CRITICAL**: Call this in widget destructor to prevent dangling pointers.
         * Clears focus, capture, and hover if widget holds any of these states.
         *
         * Does NOT call callbacks - widget is being destroyed.
         *
         * @example
         * @code
         * ~my_widget() {
         *     if (auto* input = ui_services<Backend>::input()) {
         *         input->release_widget(this);
         *     }
         * }
         * @endcode
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
        target_ptr m_focused = nullptr;   ///< Widget with keyboard focus
        target_ptr m_captured = nullptr;  ///< Widget that captured mouse during drag
        target_ptr m_hovered = nullptr;   ///< Widget under mouse cursor
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
            if (it == focusable.end() || ++it == focusable.end()) {
                it = focusable.begin();
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
