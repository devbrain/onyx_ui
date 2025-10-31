/**
 * @file action_impl.hh
 * @brief Implementation details for action and action_group interaction
 * @author igor
 * @date 2025-10-31
 *
 * @details
 * This file contains implementations that require both action and action_group
 * to be fully defined. This breaks the circular dependency between the two headers.
 *
 * ## Why This File Exists
 *
 * - action.hh forward-declares action_group (doesn't need full definition)
 * - action_group.hh includes action.hh (needs full action definition)
 * - action::set_checked() needs to call action_group::on_action_checked()
 * - This creates a circular dependency
 *
 * ## Solution
 *
 * - action.hh declares action with forward-declared action_group
 * - action_group.hh defines action_group (includes action.hh)
 * - action_impl.hh provides implementations needing both (includes both headers)
 *
 * ## Usage
 *
 * **If you only need action:**
 * ```cpp
 * #include <onyxui/actions/action.hh>
 * auto save_action = std::make_shared<action<Backend>>();
 * save_action->trigger();  // Works fine
 * ```
 *
 * **If you need action with action_group:**
 * ```cpp
 * #include <onyxui/actions/action_impl.hh>  // Includes both + implementations
 * auto left = std::make_shared<action<Backend>>();
 * auto group = std::make_shared<action_group<Backend>>();
 * group->add_action(left);
 * left->set_checked(true);  // Notifies group - implementation provided here
 * ```
 *
 * **If you only need action_group:**
 * ```cpp
 * #include <onyxui/actions/action_group.hh>  // Already includes action.hh
 * auto group = std::make_shared<action_group<Backend>>();
 * ```
 *
 * @see action For action class declaration
 * @see action_group For action_group class declaration
 */

#pragma once

#include <onyxui/actions/action.hh>
#include <onyxui/actions/action_group.hh>

namespace onyxui {
    /**
     * @brief Implementation of set_checked with action_group notification
     *
     * @tparam Backend The backend type
     * @param checked New checked state
     *
     * @details
     * This implementation requires the full definition of action_group because
     * it calls group->on_action_checked(). Without this implementation, you can
     * still use actions, but checking an action won't notify its group.
     *
     * **Exception Safety:** No-throw guarantee (noexcept)
     *
     * @note This is defined out-of-line to avoid circular dependencies between
     * action.hh and action_group.hh
     */
    template<UIBackend Backend>
    inline void action<Backend>::set_checked(bool checked) noexcept {
        if (!m_checkable) return;  // Only checkable actions can be checked

        if (m_checked != checked) {
            m_checked = checked;
            checked_changed.emit(checked);

            // Notify action group if this action is being checked
            // This requires the full definition of action_group (on_action_checked method)
            if (checked) {
                if (auto group = m_group.lock()) {
                    group->on_action_checked(this->shared_from_this());
                }
            }
        }
    }
} // namespace onyxui
