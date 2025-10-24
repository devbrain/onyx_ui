/**
 * @file focus_manager.hh
 * @brief Manages keyboard focus across UI elements using event traits architecture
 * @author igor
 * @date 10/10/2025
 *
 * This focus manager is designed to work with the event_target system and
 * event traits architecture, providing backend-agnostic focus management.
 *
 * ## Architecture Overview
 *
 * The focus management system provides keyboard navigation and focus tracking
 * for event_target elements. It's designed to work with the Backend pattern,
 * making it compatible with any backend that implements the required traits.
 *
 * ## Key Features
 *
 * - **Tab Navigation**: Automatic Tab/Shift+Tab keyboard navigation
 * - **Tab Order Control**: Elements can specify tab_index for custom order
 * - **Focus State Management**: Tracks and updates focus state across elements
 * - **Event Forwarding**: Routes events to the focused element
 * - **Hierarchical Support**: Extended version for tree-based UI structures
 *
 * ## Design Principles
 *
 * 1. **Backend Agnostic**: Uses event_traits to work with any event system
 * 2. **Non-Intrusive**: Elements opt-in to focus by setting focusable flag
 * 3. **Flexible Ordering**: Supports both explicit (tab_index) and implicit ordering
 * 4. **State Consistency**: Ensures only one element has focus at a time
 *
 * ## Integration with Backend Pattern
 *
 * The focus_manager is templated on Backend, allowing it to work with any
 * backend that provides the necessary event types and traits:
 *
 * @code
 * // SDL backend focus manager
 * focus_manager<sdl_backend> sdl_focus;
 *
 * // Test backend focus manager for unit tests
 * focus_manager<test_backend> test_focus;
 * @endcode
 */

#pragma once

#include <onyxui/event_target.hh>
#include <onyxui/concepts/event_like.hh>
#include <algorithm>
#include <vector>
#include <memory>
#include <utility>  // for std::exchange

namespace onyxui {
    /**
     * @class focus_manager
     * @brief Manages keyboard focus and tab order for event_target elements
     *
     * This class manages focus for elements that derive from event_target<Backend>.
     * It handles Tab/Shift+Tab navigation and delivers focus change notifications.
     *
     * @tparam Backend The backend traits type providing event types
     *
     * @example Using focus_manager with SDL backend
     * @code
     * // Create focus manager for SDL backend
     * focus_manager<sdl_backend> fm;
     *
     * // Create some focusable targets
     * class button : public event_target<sdl_backend> { ... };
     * button btn1, btn2, btn3;
     *
     * // Set focusable and tab order
     * btn1.set_focusable(true);
     * btn1.set_tab_index(0);
     * btn2.set_focusable(true);
     * btn2.set_tab_index(1);
     * btn3.set_focusable(true);
     * btn3.set_tab_index(2);
     *
     * // Set initial focus
     * fm.set_focus(&btn1);
     *
     * // Handle Tab navigation in event loop
     * SDL_Event event;
     * while (SDL_PollEvent(&event)) {
     *     if (event.type == SDL_KEYDOWN) {
     *         if (fm.handle_tab_navigation(event.key, root_container)) {
     *             continue; // Tab was handled
     *         }
     *     }
     *     // Forward other events to focused element
     *     if (auto* focused = fm.get_focused()) {
     *         focused->process_event(event);
     *     }
     * }
     * @endcode
     */
    template<UIBackend Backend>
    class focus_manager {
    public:
        using target_type = event_target<Backend>;
        using target_ptr = target_type*;

        /**
         * @brief Default constructor
         */
        focus_manager() = default;

        /**
         * @brief Destructor - clears focus without callbacks
         *
         * @details
         * Resets the focus pointer without calling handle_focus_lost() on the target.
         * This is necessary because the target may have already been destroyed, and
         * calling methods on a dangling pointer would cause undefined behavior.
         * The target's own destructor is responsible for cleaning up its focus state.
         *
         * @note This is noexcept - destructors must not throw
         */
        ~focus_manager() noexcept {
            // Just reset the pointer without callbacks - target may already be destroyed
            m_focused_target = nullptr;
        }

        // Delete copy operations
        // Rationale: Focus state is unique per manager
        focus_manager(const focus_manager&) = delete;
        focus_manager& operator=(const focus_manager&) = delete;

        /**
         * @brief Move constructor - transfers focus state
         */
        focus_manager(focus_manager&& other) noexcept
            : m_focused_target(std::exchange(other.m_focused_target, nullptr)) {
        }

        /**
         * @brief Move assignment - transfers focus state
         *
         * @details
         * Transfers focus from another manager without calling callbacks.
         * Similar to the destructor, we cannot safely call handle_focus_lost()
         * because we don't know if the current focused target is still alive.
         */
        focus_manager& operator=(focus_manager&& other) noexcept {
            if (this != &other) {
                // Just reset the pointer without callbacks - target may not be valid
                m_focused_target = std::exchange(other.m_focused_target, nullptr);
            }
            return *this;
        }

        /**
         * @brief Set keyboard focus to a target
         * @param target The target to focus (or nullptr to clear focus)
         * @return true if focus was changed, false if target is non-focusable, disabled, or already focused
         *
         * @exception Any exception thrown by target->handle_focus_lost()
         * @exception Any exception thrown by target->handle_focus_gained()
         * @note Exception safety: Basic guarantee - if handle_focus_gained() throws, old focus was already lost
         * @note Returns false (no exception) for non-focusable or disabled targets
         */
        bool set_focus(target_ptr target) {
            if (target && !target->is_focusable()) {
                return false;  // Cannot focus non-focusable elements
            }

            if (target && !target->is_enabled()) {
                return false;  // Cannot focus disabled elements
            }

            if (m_focused_target == target) {
                return false;  // Already focused
            }

            // Notify old focused target of focus loss
            if (m_focused_target) {
                m_focused_target->handle_focus_lost();
            }

            m_focused_target = target;

            // Notify new focused target of focus gain
            if (m_focused_target) {
                m_focused_target->handle_focus_gained();
            }

            return true;
        }

        /**
         * @brief Get currently focused target
         * @return Pointer to focused target or nullptr
         *
         * @note Exception safety: No-throw guarantee (noexcept)
         */
        [[nodiscard]] target_ptr get_focused() const noexcept {
            return m_focused_target;
        }

        /**
         * @brief Clear focus (no element focused)
         *
         * @exception Any exception thrown by set_focus()
         * @note Exception safety: Basic guarantee - delegates to set_focus(nullptr)
         */
        void clear_focus() {
            set_focus(nullptr);
        }

        /**
         * @brief Check if a specific target has focus
         * @param target The target to check
         * @return true if the target has focus
         *
         * @note Exception safety: No-throw guarantee (noexcept)
         */
        [[nodiscard]] bool has_focus(target_ptr target) const noexcept {
            return m_focused_target == target;
        }

        /**
         * @brief Move focus to next focusable target
         * @param targets Collection of all available targets
         *
         * @exception std::bad_alloc If vector allocation or sort fails
         * @exception Any exception thrown by set_focus()
         * @note Exception safety: Strong guarantee - focus unchanged if exception thrown during sort
         * @note If targets list is empty, this is a no-op
         */
        void focus_next(const std::vector<target_ptr>& targets) {
            auto focusables = collect_focusable_targets(targets);
            if (focusables.empty()) return;

            // Sort by tab index with stable tiebreaker
            std::stable_sort(focusables.begin(), focusables.end(),
                     [](target_ptr a, target_ptr b) {
                         int const a_idx = a->tab_index();
                         int const b_idx = b->tab_index();

                         // Both have explicit tab indices
                         if (a_idx >= 0 && b_idx >= 0) {
                             if (a_idx != b_idx) {
                                 return a_idx < b_idx;
                             }
                             // Equal tab indices: use pointer address as stable tiebreaker
                             return std::less<>()(a, b);
                         }

                         // Explicit indices come before implicit (-1)
                         if (a_idx >= 0) return true;
                         if (b_idx >= 0) return false;

                         // Both have implicit indices: maintain original order (stable_sort)
                         return false;
                     });

            // Find current in list
            auto it = std::find(focusables.begin(), focusables.end(), m_focused_target);

            if (it == focusables.end()) {
                // Nothing focused, focus first
                set_focus(focusables.front());
            } else {
                // Move to next element
                ++it;
                if (it == focusables.end()) {
                    // At end, wrap to beginning
                    set_focus(focusables.front());
                } else {
                    // Focus next
                    set_focus(*it);
                }
            }
        }

        /**
         * @brief Move focus to previous focusable target
         * @param targets Collection of all available targets
         *
         * @exception std::bad_alloc If vector allocation or sort fails
         * @exception Any exception thrown by set_focus()
         * @note Exception safety: Strong guarantee - focus unchanged if exception thrown during sort
         * @note If targets list is empty, this is a no-op
         */
        void focus_previous(const std::vector<target_ptr>& targets) {
            auto focusables = collect_focusable_targets(targets);
            if (focusables.empty()) return;

            // Sort by tab index with stable tiebreaker (same as focus_next)
            std::stable_sort(focusables.begin(), focusables.end(),
                     [](target_ptr a, target_ptr b) {
                         int const a_idx = a->tab_index();
                         int const b_idx = b->tab_index();

                         // Both have explicit tab indices
                         if (a_idx >= 0 && b_idx >= 0) {
                             if (a_idx != b_idx) {
                                 return a_idx < b_idx;
                             }
                             // Equal tab indices: use pointer address as stable tiebreaker
                             return std::less<>()(a, b);
                         }

                         // Explicit indices come before implicit (-1)
                         if (a_idx >= 0) return true;
                         if (b_idx >= 0) return false;

                         // Both have implicit indices: maintain original order (stable_sort)
                         return false;
                     });

            // Find current in list
            auto it = std::find(focusables.begin(), focusables.end(), m_focused_target);

            if (it == focusables.end()) {
                // Nothing focused, focus last
                set_focus(focusables.back());
            } else if (it == focusables.begin()) {
                // At beginning, wrap to end
                set_focus(focusables.back());
            } else {
                // Focus previous
                --it;
                set_focus(*it);
            }
        }

        /**
         * @brief Handle Tab key for focus navigation
         * @tparam E The specific event type (must satisfy KeyboardEvent concept)
         * @param event The keyboard event
         * @param targets Collection of all available targets
         * @return true if Tab was handled, false if not a Tab key press
         *
         * @exception Any exception thrown by focus_next() or focus_previous()
         * @note Exception safety: Strong guarantee - focus unchanged if exception thrown
         * @note Returns false (no exception) for non-Tab keys or key release events
         *
         * @example
         * @code
         * if (event.type == SDL_KEYDOWN) {
         *     if (focus_mgr.handle_tab_navigation(event.key, all_targets)) {
         *         // Tab was handled, don't process further
         *         continue;
         *     }
         * }
         * @endcode
         */
        template<typename E>
        requires KeyboardEvent<E>
        bool handle_tab_navigation(const E& event, const std::vector<target_ptr>& targets) {
            // Check if this is a key press (not release)
            if (!event_traits<E>::is_key_press(event)) {
                return false;
            }

            // Check if it's a repeat (we typically want to handle repeats for Tab)
            // Most apps do handle Tab repeats for continuous navigation

            // Check if it's Tab using event traits
            if (!event_traits<E>::is_tab_key(event)) {
                return false;
            }

            // Check modifiers if available
            bool shift = false;
            if constexpr (ModifierState<E>) {
                shift = event_traits<E>::shift_pressed(event);
            }

            // Navigate focus
            if (shift) {
                focus_previous(targets);
            } else {
                focus_next(targets);
            }

            return true;
        }

        /**
         * @brief Process any keyboard event and handle Tab navigation
         * @param event Any event (will check if it's a keyboard event)
         * @param targets Collection of all available targets
         * @return true if the event was handled (Tab navigation occurred)
         *
         * @exception Any exception thrown by handle_tab_navigation() if event is a keyboard event
         * @note Exception safety: Strong guarantee - delegates to handle_tab_navigation()
         * @note Returns false (no exception) for non-keyboard events
         */
        template<typename E>
        bool process_navigation_event(const E& event, const std::vector<target_ptr>& targets) {
            if constexpr (KeyboardEvent<E>) {
                return handle_tab_navigation(event, targets);
            }
            return false;
        }

        /**
         * @brief Forward an event to the focused target
         * @param event The event to forward
         * @return true if the focused target handled the event, false if no target focused
         *
         * @exception Any exception thrown by target->process_event()
         * @note Exception safety: Depends on focused target's process_event() implementation
         * @note Returns false (no exception) if no target currently has focus
         */
        template<typename E>
        bool forward_to_focused(const E& event) {
            if (m_focused_target) {
                return m_focused_target->process_event(event);
            }
            return false;
        }

    private:
        target_ptr m_focused_target = nullptr;

        /**
         * @brief Collect all focusable targets from a list
         * @param targets The list of all targets
         * @return Vector of focusable and enabled targets
         *
         * @details
         * Filters the input list to include only targets that are both focusable
         * and enabled. This method relies on RVO (Return Value Optimization) to
         * avoid copying the result vector. Modern C++ compilers (C++11 and later)
         * guarantee copy elision for return values, making this pattern efficient.
         *
         * @note Performance: O(n) where n = number of targets in input list
         * @note RVO Optimization: The returned vector is constructed directly at
         *       the call site without copying or moving, thanks to guaranteed copy
         *       elision in C++17 and earlier RVO in C++11/14.
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

    /**
     * @class hierarchical_focus_manager
     * @brief Extended focus manager that works with tree hierarchies
     *
     * This version can traverse a tree of elements (like ui_element hierarchy)
     * to find focusable targets. It requires a way to get children from elements.
     *
     * @tparam Backend The backend traits type providing event types
     * @tparam ElementType The element type (must have a way to get children)
     */
    template<UIBackend Backend, typename ElementType>
    class hierarchical_focus_manager : public focus_manager<Backend> {
    public:
        using base_type = focus_manager<Backend>;
        using element_ptr = ElementType*;
        using target_ptr = typename base_type::target_ptr;

        /**
         * @brief Move focus to next element in tree
         * @param root The root element to search from
         */
        void focus_next_in_tree(element_ptr root) {
            auto targets = collect_all_targets(root);
            this->focus_next(targets);
        }

        /**
         * @brief Move focus to previous element in tree
         * @param root The root element to search from
         */
        void focus_previous_in_tree(element_ptr root) {
            auto targets = collect_all_targets(root);
            this->focus_previous(targets);
        }

        /**
         * @brief Handle Tab navigation within a tree
         * @param event The keyboard event
         * @param root The root element
         * @return true if Tab was handled
         */
        template<typename E>
        requires KeyboardEvent<E>
        bool handle_tab_navigation_in_tree(const E& event, element_ptr root) {
            auto targets = collect_all_targets(root);
            return this->handle_tab_navigation(event, targets);
        }

    private:
        /**
         * @brief Recursively collect all event targets from element tree
         * @param element The element to start from
         * @return Vector of all event_target pointers in tree
         *
         * @details
         * Traverses the element tree depth-first, collecting all targets that
         * can be focused. This method assumes ElementType either:
         * 1. Is or inherits from event_target<EventType>
         * 2. Has a method to get its event_target
         *
         * @note Performance: O(n) where n = total number of elements in tree
         * @note RVO Optimization: Return value benefits from guaranteed copy elision,
         *       avoiding vector copy overhead
         */
        std::vector<target_ptr> collect_all_targets(element_ptr element) {
            std::vector<target_ptr> result;
            if (!element) return result;

            collect_targets_recursive(element, result);
            return result;
        }

        void collect_targets_recursive(element_ptr element, std::vector<target_ptr>& out) {
            if (!element) return;

            // Try to cast element to event_target
            // This assumes ElementType inherits from event_target<EventType>
            if (auto* target = dynamic_cast<target_ptr>(element)) {
                // Only add if it's visible (assuming element has this method)
                if constexpr (requires { element->is_visible(); }) {
                    if (element->is_visible()) {
                        out.push_back(target);
                    } else {
                        return; // Skip invisible branches
                    }
                } else {
                    out.push_back(target);
                }
            }

            // Recurse to children
            // This requires ElementType to have a way to iterate children
            // Different approaches depending on the element interface:

            // Approach 1: If element has a children() method returning a range
            if constexpr (requires { element->children(); }) {
                for (auto& child : element->children()) {
                    if constexpr (std::is_pointer_v<std::decay_t<decltype(child)>>) {
                        collect_targets_recursive(child, out);
                    } else if constexpr (requires { child.get(); }) {
                        collect_targets_recursive(child.get(), out);
                    } else if constexpr (requires { &child; }) {
                        collect_targets_recursive(&child, out);
                    }
                }
            }
            // Approach 2: If element has begin()/end() iterators
            else if constexpr (requires { element->begin(); element->end(); }) {
                for (auto it = element->begin(); it != element->end(); ++it) {
                    if constexpr (std::is_pointer_v<std::decay_t<decltype(*it)>>) {
                        collect_targets_recursive(*it, out);
                    } else if constexpr (requires { (*it).get(); }) {
                        collect_targets_recursive((*it).get(), out);
                    } else {
                        collect_targets_recursive(&(*it), out);
                    }
                }
            }
            // Approach 3: Manual child access (you'd implement this based on your element)
            // else { ... }
        }
    };
}