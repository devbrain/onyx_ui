/**
 * @file semantic_action_guard.hh
 * @brief RAII guard for semantic action registration
 * @author Assistant
 * @date 2025-10-28
 *
 * @details
 * Provides RAII-based semantic action registration to prevent resource leaks
 * and ensure automatic cleanup when guards go out of scope.
 */

#pragma once

#include <onyxui/concepts/backend.hh>
#include <onyxui/hotkeys/hotkey_action.hh>
#include <onyxui/hotkeys/hotkey_manager.hh>
#include <functional>

namespace onyxui {

    /**
     * @brief RAII guard for semantic action registration
     *
     * @tparam Backend The UI backend type
     *
     * @details
     * Automatically unregisters a semantic action when destroyed.
     * Provides move semantics for transfer of ownership.
     *
     * Similar to std::unique_ptr for semantic action lifecycle.
     *
     * ## Benefits
     *
     * - **Automatic cleanup**: Destructor unregisters action
     * - **Exception-safe**: RAII guarantees cleanup even on exceptions
     * - **Move semantics**: Transfer ownership between guards
     * - **Single responsibility**: Guard owns the registration lifecycle
     *
     * ## Usage Example
     *
     * @code
     * auto* hotkeys = ui_services<Backend>::hotkeys();
     *
     * // Create guard - registers action
     * semantic_action_guard<Backend> guard(
     *     hotkeys,
     *     hotkey_action::menu_down,
     *     []() { handle_menu_down(); }
     * );
     *
     * // Action is registered and active
     * // ...
     *
     * // Guard destroyed - action automatically unregistered
     * @endcode
     *
     * ## Move Semantics
     *
     * @code
     * std::vector<semantic_action_guard<Backend>> guards;
     *
     * // Move guard into vector - ownership transferred
     * guards.emplace_back(hotkeys, hotkey_action::menu_down, handler);
     *
     * // Clear vector - all guards destroyed, actions unregistered
     * guards.clear();
     * @endcode
     *
     * ## Thread Safety
     *
     * Not thread-safe. All operations must occur on the UI thread.
     *
     * ## Lifetime Requirements
     *
     * **CRITICAL**: The `hotkey_manager` must outlive all `semantic_action_guard`
     * instances that reference it. The guard stores a raw (non-owning) pointer and
     * will dereference it in the destructor. If the manager is destroyed while guards
     * still exist, the destructor will access a dangling pointer (undefined behavior).
     *
     * Safe usage patterns:
     * - Store guards as members of widgets that are children of the application root
     * - Store guards in scope that ends before application shutdown
     * - Use `release()` before destroying the manager if guards may outlive it
     *
     * Unsafe patterns to avoid:
     * - Static/global guards (may outlive the UI context)
     * - Guards stored in objects with unclear ownership relative to hotkey_manager
     */
    template<UIBackend Backend>
    class semantic_action_guard {
    public:
        using hotkey_manager_type = hotkey_manager<Backend>;

        /**
         * @brief Default constructor - creates invalid guard
         *
         * @details
         * Creates a guard that owns no registration.
         * Useful for deferred initialization.
         */
        semantic_action_guard() noexcept = default;

        /**
         * @brief Construct and register semantic action
         *
         * @param manager Hotkey manager (non-owning pointer). **Must outlive this guard.**
         * @param action Semantic action to register
         * @param handler Action handler function
         *
         * @details
         * Registers the semantic action immediately.
         * If manager is nullptr, guard is invalid but safe to destroy.
         *
         * @warning The manager must outlive this guard. See class documentation
         *          for lifetime requirements.
         */
        semantic_action_guard(
            hotkey_manager_type* manager,
            hotkey_action action,
            std::function<void()> handler
        ) : m_manager(manager), m_action(action) {
            if (m_manager) {
                m_manager->register_semantic_action(m_action, std::move(handler));
            }
        }

        /**
         * @brief Destructor - automatically unregisters
         *
         * @details
         * Unregisters the semantic action if guard is valid.
         * Safe to call even if guard is invalid (m_manager == nullptr).
         */
        ~semantic_action_guard() {
            if (m_manager) {
                m_manager->unregister_semantic_action(m_action);
            }
        }

        /**
         * @brief Move constructor
         *
         * @param other Guard to move from
         *
         * @details
         * Transfers ownership of the registration.
         * After move, `other` is invalid and will not unregister on destruction.
         */
        semantic_action_guard(semantic_action_guard&& other) noexcept
            : m_manager(other.m_manager)
            , m_action(other.m_action) {
            other.m_manager = nullptr;  // Transfer ownership
        }

        /**
         * @brief Move assignment operator
         *
         * @param other Guard to move from
         * @return Reference to this guard
         *
         * @details
         * Unregisters current action (if any), then transfers ownership
         * from `other`. After move, `other` is invalid.
         */
        semantic_action_guard& operator=(semantic_action_guard&& other) noexcept {
            if (this != &other) {
                // Unregister current (if any)
                if (m_manager) {
                    m_manager->unregister_semantic_action(m_action);
                }

                // Transfer ownership
                m_manager = other.m_manager;
                m_action = other.m_action;
                other.m_manager = nullptr;
            }
            return *this;
        }

        /**
         * @brief Check if guard is valid (owns a registration)
         *
         * @return true if guard owns a registration, false otherwise
         *
         * @details
         * A guard is valid if it was constructed with a non-null manager
         * and has not been moved from.
         */
        [[nodiscard]] bool is_valid() const noexcept {
            return m_manager != nullptr;
        }

        /**
         * @brief Explicitly release ownership without unregistering
         *
         * @details
         * After calling release(), the guard no longer owns the registration
         * and will not unregister on destruction.
         *
         * **Use with caution**: This can lead to leaked registrations if not
         * manually unregistered elsewhere.
         */
        void release() noexcept {
            m_manager = nullptr;
        }

        /**
         * @brief Get the hotkey action managed by this guard
         *
         * @return The hotkey action
         *
         * @details
         * Valid even for invalid guards (m_manager == nullptr).
         */
        [[nodiscard]] hotkey_action action() const noexcept {
            return m_action;
        }

        // Non-copyable
        semantic_action_guard(const semantic_action_guard&) = delete;
        semantic_action_guard& operator=(const semantic_action_guard&) = delete;

    private:
        hotkey_manager_type* m_manager = nullptr;  ///< Non-owning pointer to hotkey manager (must outlive guard)
        hotkey_action m_action = hotkey_action::activate_menu_bar;  ///< Semantic action being guarded (default value not used)
    };

} // namespace onyxui
