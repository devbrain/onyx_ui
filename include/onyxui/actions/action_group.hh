/**
 * @file action_group.hh
 * @brief Coordinate mutually exclusive actions (radio button behavior)
 * @author igor
 * @date 16/10/2025
 *
 * @details
 * Provides action groups for coordinating related actions, particularly
 * for implementing radio button-style behavior where only one action
 * in a group can be checked at a time.
 *
 * ## Action Group
 * Groups actions together to coordinate their checked states.
 *
 * **Key Features:**
 * - Exclusive mode: Only one action checked at a time (default)
 * - Non-exclusive mode: Multiple actions can be checked
 * - Automatic state synchronization
 * - Weak references to actions (no ownership)
 * - Application-owned (shared_ptr)
 *
 * **Useful For:**
 * - Alignment buttons (left, center, right)
 * - Tool selection (select, pen, eraser)
 * - View modes (list, grid, icons)
 * - Radio button groups
 * - Tab-like selection
 *
 * ## Usage Examples
 *
 * ### Exclusive Group (Radio Buttons)
 * ```cpp
 * auto left = std::make_shared<action<Backend>>();
 * auto center = std::make_shared<action<Backend>>();
 * auto right = std::make_shared<action<Backend>>();
 *
 * auto align_group = std::make_shared<action_group<Backend>>();
 * align_group->add_action(left);
 * align_group->add_action(center);
 * align_group->add_action(right);
 *
 * // Checking one unchecks others
 * center->set_checked(true);  // left and right become unchecked
 * ```
 *
 * ### Non-Exclusive Group
 * ```cpp
 * auto bold = std::make_shared<action<Backend>>();
 * auto italic = std::make_shared<action<Backend>>();
 * auto underline = std::make_shared<action<Backend>>();
 *
 * auto format_group = std::make_shared<action_group<Backend>>(false);  // non-exclusive
 * format_group->add_action(bold);
 * format_group->add_action(italic);
 * format_group->add_action(underline);
 *
 * // Multiple can be checked simultaneously
 * bold->set_checked(true);
 * italic->set_checked(true);  // bold stays checked
 * ```
 *
 * ## Ownership Model
 * - **Application owns group**: `std::shared_ptr<action_group>`
 * - **Group holds weak references to actions**: No ownership
 * - **Actions hold weak reference to group**: No circular references
 * - **Group doesn't keep actions alive**
 *
 * ## Exception Safety
 * - Constructor: No-throw guarantee
 * - add_action(): Basic guarantee
 * - remove_action(): No-throw guarantee
 * - Getters: No-throw guarantee
 *
 * ## Thread Safety
 * Not thread-safe. All operations must be performed on the UI thread.
 *
 * @see action For individual actions
 */

#pragma once

#include <onyxui/actions/action.hh>
#include <vector>
#include <algorithm>

namespace onyxui {
    /**
     * @class action_group
     * @brief Coordinate mutually exclusive or related actions
     *
     * @details
     * Action groups manage a collection of actions, optionally enforcing
     * mutual exclusivity (radio button behavior). When in exclusive mode,
     * checking one action automatically unchecks all others in the group.
     *
     * ## Behavior
     * - **Exclusive mode (default)**: Only one action can be checked
     * - **Non-exclusive mode**: Multiple actions can be checked
     * - Automatic cleanup of destroyed actions (weak_ptr)
     * - Group doesn't own actions (no lifetime extension)
     *
     * ## Exclusive Mode Details
     * When an action is checked:
     * 1. Action calls set_checked(true)
     * 2. Action notifies its group
     * 3. Group unchecks all other actions
     * 4. Result: Only the newly checked action is checked
     *
     * ## Non-Exclusive Mode
     * - Actions behave independently
     * - Multiple can be checked
     * - Group serves organizational purpose
     * - Useful for enabling/disabling related actions together (future)
     *
     * ## Memory Management
     * - Group uses `weak_ptr<action>` to avoid extending action lifetime
     * - Actions automatically removed when destroyed
     * - Group can safely outlive actions
     * - Actions can safely outlive group
     *
     * ## Exception Safety
     * - Constructor is noexcept
     * - add_action() provides basic guarantee
     * - State changes are atomic (strong guarantee)
     *
     * @tparam Backend The backend traits type satisfying UIBackend concept
     *
     * @see action For individual actions
     */
    template<UIBackend Backend>
    class action_group : public std::enable_shared_from_this<action_group<Backend>> {
        public:
            /**
             * @brief Construct an action group
             *
             * @param exclusive True for mutually exclusive (radio button), false for independent
             *
             * @details
             * Creates an action group with the specified exclusivity mode.
             * - exclusive=true: Only one action can be checked (default)
             * - exclusive=false: Multiple actions can be checked
             *
             * **Exception Safety:** No-throw guarantee (noexcept)
             *
             * @example Exclusive (radio buttons)
             * @code
             * auto align_group = std::make_shared<action_group<Backend>>();
             * @endcode
             *
             * @example Non-exclusive (independent toggles)
             * @code
             * auto format_group = std::make_shared<action_group<Backend>>(false);
             * @endcode
             */
            explicit action_group(bool exclusive = true) noexcept
                : m_exclusive(exclusive) {}

            /**
             * @brief Virtual destructor
             */
            virtual ~action_group() = default;

            // Disable copy, allow move
            action_group(const action_group&) = delete;
            action_group& operator=(const action_group&) = delete;
            action_group(action_group&&) noexcept = default;
            action_group& operator=(action_group&&) noexcept = default;

            /**
             * @brief Add an action to the group
             *
             * @param action_ptr Shared pointer to the action to add
             *
             * @throws std::bad_alloc If vector resize fails
             *
             * @details
             * Adds an action to this group. The action will be automatically
             * made checkable. In exclusive mode, if this is the first action
             * or no action is checked, this action can be checked.
             *
             * The group stores a weak_ptr, so it doesn't keep the action alive.
             * The action stores a weak_ptr to the group, so no circular references.
             *
             * **Exception Safety:** Basic guarantee - action may be partially added
             *
             * @example
             * @code
             * auto left = std::make_shared<action<Backend>>();
             * auto center = std::make_shared<action<Backend>>();
             *
             * auto group = std::make_shared<action_group<Backend>>();
             * group->add_action(left);
             * group->add_action(center);
             * @endcode
             */
            void add_action(std::shared_ptr<action<Backend>> action_ptr) {
                if (!action_ptr) return;

                // Make action checkable (required for group coordination)
                action_ptr->set_checkable(true);

                // Register group with action
                action_ptr->set_group(this->weak_from_this());

                // Add to group's action list
                m_actions.push_back(action_ptr);
            }

            /**
             * @brief Remove an action from the group
             *
             * @param action_ptr Shared pointer to the action to remove
             *
             * @details
             * Removes an action from this group. The action's group reference
             * is cleared. If the action is destroyed, it's automatically removed
             * (weak_ptr).
             *
             * **Exception Safety:** No-throw guarantee (noexcept)
             *
             * @example
             * @code
             * group->remove_action(center_align);
             * @endcode
             */
            void remove_action(std::shared_ptr<action<Backend>> action_ptr) noexcept {
                if (!action_ptr) return;

                // Clear action's group reference
                action_ptr->set_group(std::weak_ptr<action_group<Backend>>());

                // Remove from list
                m_actions.erase(
                    std::remove_if(m_actions.begin(), m_actions.end(),
                        [&action_ptr](const std::weak_ptr<action<Backend>>& weak) {
                            auto ptr = weak.lock();
                            return !ptr || ptr == action_ptr;
                        }),
                    m_actions.end()
                );
            }

            /**
             * @brief Get all actions in the group
             *
             * @return Vector of shared pointers to alive actions
             *
             * @details
             * Returns all actions that are still alive. Dead actions (expired weak_ptr)
             * are automatically filtered out.
             *
             * **Exception Safety:** Basic guarantee (vector allocation may throw)
             *
             * @example
             * @code
             * auto actions = group->actions();
             * for (auto& action : actions) {
             *     std::cout << action->text() << std::endl;
             * }
             * @endcode
             */
            [[nodiscard]] std::vector<std::shared_ptr<action<Backend>>> actions() const {
                std::vector<std::shared_ptr<action<Backend>>> result;
                result.reserve(m_actions.size());

                for (const auto& weak : m_actions) {
                    if (auto action = weak.lock()) {
                        result.push_back(action);
                    }
                }

                return result;
            }

            /**
             * @brief Check if group is in exclusive mode
             *
             * @return True if exclusive (radio button), false if non-exclusive
             *
             * @note noexcept - guaranteed not to throw
             */
            [[nodiscard]] bool is_exclusive() const noexcept { return m_exclusive; }

            /**
             * @brief Set exclusive mode
             *
             * @param exclusive True for mutually exclusive, false for independent
             *
             * @details
             * Changes the exclusivity mode of the group. If switching to exclusive
             * mode and multiple actions are checked, only the first checked action
             * will remain checked.
             *
             * **Exception Safety:** No-throw guarantee (noexcept)
             *
             * @example
             * @code
             * group->set_exclusive(false);  // Allow multiple checked actions
             * @endcode
             */
            void set_exclusive(bool exclusive) noexcept {
                if (m_exclusive == exclusive) return;

                m_exclusive = exclusive;

                // If switching to exclusive, ensure only one is checked
                if (exclusive) {
                    bool found_checked = false;
                    for (auto& weak : m_actions) {
                        if (auto action = weak.lock()) {
                            if (action->is_checked()) {
                                if (found_checked) {
                                    // Already have a checked action, uncheck this one
                                    action->set_checked(false);
                                } else {
                                    found_checked = true;
                                }
                            }
                        }
                    }
                }
            }

            /**
             * @brief Get the currently checked action (exclusive mode only)
             *
             * @return Shared pointer to checked action, or nullptr if none checked
             *
             * @details
             * In exclusive mode, returns the one checked action.
             * In non-exclusive mode, returns the first checked action found.
             *
             * **Exception Safety:** No-throw guarantee (noexcept)
             *
             * @example
             * @code
             * if (auto checked = align_group->checked_action()) {
             *     std::cout << "Current alignment: " << checked->text() << std::endl;
             * }
             * @endcode
             */
            [[nodiscard]] std::shared_ptr<action<Backend>> checked_action() const noexcept {
                for (const auto& weak : m_actions) {
                    if (auto action = weak.lock()) {
                        if (action->is_checked()) {
                            return action;
                        }
                    }
                }
                return nullptr;
            }

        private:
            friend class action<Backend>;

            /**
             * @brief Called by action when it's checked (internal)
             *
             * @param checked_action The action that was just checked
             *
             * @details
             * Internal callback used by actions to notify the group.
             * In exclusive mode, unchecks all other actions.
             *
             * **Exception Safety:** No-throw guarantee (noexcept)
             */
            void on_action_checked(std::shared_ptr<action<Backend>> checked_action) noexcept {
                if (!m_exclusive) return;  // Non-exclusive mode: nothing to do

                // Uncheck all other actions
                for (auto& weak : m_actions) {
                    if (auto action = weak.lock()) {
                        if (action != checked_action && action->is_checked()) {
                            action->set_checked(false);
                        }
                    }
                }

                // Clean up dead actions while we're here
                m_actions.erase(
                    std::remove_if(m_actions.begin(), m_actions.end(),
                        [](const std::weak_ptr<action<Backend>>& weak) {
                            return weak.expired();
                        }),
                    m_actions.end()
                );
            }

            bool m_exclusive;                                       ///< Exclusive mode flag
            std::vector<std::weak_ptr<action<Backend>>> m_actions; ///< Weak references to actions
    };

} // namespace onyxui
