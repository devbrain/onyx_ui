/**
 * @file hotkey_manager.hh
 * @brief Central manager for keyboard shortcuts and hotkey dispatch
 * @author igor
 * @date 16/10/2025
 *
 * @details
 * Provides centralized hotkey management for UI applications, handling
 * keyboard shortcut registration, conflict detection, and event dispatch.
 *
 * ## Design Philosophy
 *
 * **Centralized Management:**
 * All hotkeys are registered with a single hotkey_manager instance.
 * This allows conflict detection and provides a single source of truth.
 *
 * **Action-Based:**
 * Hotkeys are registered for actions, not callbacks. This integrates
 * seamlessly with the existing action system.
 *
 * **Scope-Aware:**
 * Hotkeys can be global, window-scoped, or element-scoped:
 * - **Global**: Work anywhere in the application (Ctrl+S for Save)
 * - **Window**: Work when specific window has focus
 * - **Element**: Work when specific element or its children have focus
 *
 * **Conflict Detection:**
 * Detects when multiple hotkeys use the same key sequence within
 * the same scope and provides warnings or errors.
 *
 * ## Usage Examples
 *
 * ### Basic Global Hotkeys
 * ```cpp
 * hotkey_manager<Backend> manager;
 *
 * auto save_action = std::make_shared<action<Backend>>();
 * save_action->set_text("Save");
 * save_action->set_shortcut('s', key_modifier::ctrl);
 * save_action->triggered.connect([]() { save_document(); });
 *
 * // Register with manager
 * manager.register_action(save_action);
 *
 * // Handle keyboard event
 * if (event.key == 's' && event.ctrl) {
 *     if (manager.handle_key_event(event)) {
 *         // Hotkey was handled, don't propagate
 *     }
 * }
 * ```
 *
 * ### Scoped Hotkeys (Context-Sensitive)
 * ```cpp
 * // Text editor hotkeys (only active when editor has focus)
 * auto editor = std::make_unique<text_editor<Backend>>();
 *
 * auto bold_action = std::make_shared<action<Backend>>();
 * bold_action->set_shortcut('b', key_modifier::ctrl);
 *
 * manager.register_action(bold_action, editor.get());
 * // Ctrl+B only works when editor or its children have focus
 * ```
 *
 * ### Conflict Detection
 * ```cpp
 * auto action1 = std::make_shared<action<Backend>>();
 * action1->set_shortcut('s', key_modifier::ctrl);
 *
 * auto action2 = std::make_shared<action<Backend>>();
 * action2->set_shortcut('s', key_modifier::ctrl);  // Conflict!
 *
 * manager.register_action(action1);
 * manager.register_action(action2);  // Returns false, conflict detected
 * ```
 *
 * ## Integration with Focus System
 *
 * The hotkey manager uses the focus system to determine which
 * scoped hotkeys are active. When an element has focus:
 * 1. Element-scoped hotkeys are checked first
 * 2. Parent element hotkeys are checked (bubbling up)
 * 3. Global hotkeys are checked last
 *
 * @see action.hh For action shortcuts
 * @see key_sequence.hh For key representation
 * @see focus_manager.hh For focus tracking
 */

#pragma once

#include <onyxui/hotkeys/key_sequence.hh>
#include <onyxui/hotkeys/key_chord.hh>
#include <onyxui/hotkeys/hotkey_action.hh>
#include <onyxui/hotkeys/hotkey_scheme_registry.hh>
#include <onyxui/concepts/backend.hh>
#include <onyxui/concepts/event_like.hh>
#include <onyxui/actions/action.hh>
#include <onyxui/core/element.hh>
#include <onyxui/events/ui_event.hh>
// #include <failsafe/logger.hh>  // Not needed for dirty rectangle testing
#include <map>
#include <memory>
#include <vector>
#include <optional>
#include <string>
#include <functional>

namespace onyxui {

    /**
     * @enum hotkey_scope
     * @brief Defines the activation scope for a hotkey
     *
     * @details
     * Controls when and where a hotkey is active:
     *
     * - **global**: Always active, regardless of focus
     *   Example: Ctrl+S for Save, Ctrl+Q for Quit
     *
     * - **window**: Active when a specific window has focus
     *   Example: Window-specific commands
     *
     * - **element**: Active when element or its children have focus
     *   Example: Text editor formatting shortcuts
     *
     * **Priority Order:**
     * Element-scoped hotkeys have highest priority, followed by
     * window-scoped, then global. This allows context-specific
     * overrides of global hotkeys.
     */
    enum class hotkey_scope : std::uint8_t {
        global,   ///< Active anywhere in application
        window,   ///< Active when window has focus
        element   ///< Active when element or children have focus
    };

    /**
     * @struct hotkey_registration
     * @brief Internal registration info for a hotkey
     *
     * @details
     * Stores all information needed to dispatch a hotkey:
     * - The action to trigger
     * - The key sequence
     * - The scope (global, window, element)
     * - The scope target (element or window)
     *
     * @tparam Backend The backend traits type satisfying UIBackend concept
     */
    template<UIBackend Backend>
    struct hotkey_registration {
        std::weak_ptr<action<Backend>> action_ptr;  ///< Action to trigger
        key_sequence sequence;                       ///< Key combination
        hotkey_scope scope = hotkey_scope::global;   ///< Activation scope
        ui_element<Backend>* scope_target = nullptr; ///< Target element/window (null for global)

        /**
         * @brief Check if registration is still valid
         * @return True if action still exists
         */
        [[nodiscard]] bool is_valid() const noexcept {
            return !action_ptr.expired();
        }

        /**
         * @brief Get the action if still alive
         * @return Shared pointer to action, or nullptr
         */
        [[nodiscard]] std::shared_ptr<action<Backend>> get_action() const noexcept {
            return action_ptr.lock();
        }
    };

    /**
     * @class hotkey_manager
     * @brief Manages keyboard shortcuts and dispatches hotkey events
     *
     * @details
     * Central registry for all keyboard shortcuts in the application.
     * Handles registration, conflict detection, scope resolution,
     * and event dispatching.
     *
     * ## Lifecycle
     *
     * The hotkey_manager should be owned by the application and live
     * for the entire application lifetime. Actions register themselves
     * with the manager.
     *
     * ## Thread Safety
     *
     * Not thread-safe. All operations must be performed on the UI thread.
     *
     * @tparam Backend The backend traits type satisfying UIBackend concept
     *
     * @see action For action shortcuts
     * @see key_sequence For key representation
     */
    template<UIBackend Backend>
    class hotkey_manager {
    public:
        /**
         * @brief Construct a hotkey manager
         * @param scheme_registry Optional pointer to hotkey scheme registry
         *
         * @details
         * If scheme_registry is provided, the manager will support framework
         * semantic actions (menu navigation, focus, etc.) based on the current
         * hotkey scheme. Semantic actions have priority over application actions.
         *
         * If scheme_registry is nullptr, only application actions are supported.
         */
        explicit hotkey_manager(hotkey_scheme_registry* scheme_registry = nullptr)
            : m_registrations()
            , m_conflict_policy(conflict_policy::warn)
            , m_scheme_registry(scheme_registry) {}

        /**
         * @brief Destructor
         */
        ~hotkey_manager() = default;

        // Disable copy, allow move
        hotkey_manager(const hotkey_manager&) = delete;
        hotkey_manager& operator=(const hotkey_manager&) = delete;
        hotkey_manager(hotkey_manager&&) noexcept = default;
        hotkey_manager& operator=(hotkey_manager&&) noexcept = default;

        /**
         * @enum conflict_policy
         * @brief How to handle hotkey conflicts
         */
        enum class conflict_policy : std::uint8_t {
            allow,    ///< Allow conflicts (last registered wins)
            warn,     ///< Allow but warn (default)
            error     ///< Reject conflicting registrations
        };

        /**
         * @brief Register an action's shortcut as a global hotkey
         *
         * @param action_ptr The action to register
         * @return True if registered successfully, false if conflict
         *
         * @details
         * Registers the action's shortcut (if it has one) as a global hotkey.
         * Global hotkeys are active anywhere in the application.
         *
         * **Returns false if:**
         * - Action has no shortcut
         * - Conflict detected (depending on conflict_policy)
         *
         * @example
         * @code
         * auto save_action = std::make_shared<action<Backend>>();
         * save_action->set_shortcut('s', key_modifier::ctrl);
         *
         * if (!manager.register_action(save_action)) {
         *     // Conflict or no shortcut
         * }
         * @endcode
         */
        bool register_action(std::shared_ptr<action<Backend>> action_ptr) {
            return register_action(action_ptr, hotkey_scope::global, nullptr);
        }

        /**
         * @brief Register an action's shortcut scoped to an element
         *
         * @param action_ptr The action to register
         * @param scope_element Element that must have focus
         * @return True if registered successfully
         *
         * @details
         * Registers a scoped hotkey that only activates when the given
         * element or its children have focus.
         *
         * @example
         * @code
         * auto bold_action = std::make_shared<action<Backend>>();
         * bold_action->set_shortcut('b', key_modifier::ctrl);
         *
         * manager.register_action(bold_action, text_editor.get());
         * // Ctrl+B only works when text_editor has focus
         * @endcode
         */
        bool register_action(
            std::shared_ptr<action<Backend>> action_ptr,
            ui_element<Backend>* scope_element
        ) {
            return register_action(action_ptr, hotkey_scope::element, scope_element);
        }

        /**
         * @brief Unregister an action's hotkey
         *
         * @param action_ptr The action to unregister
         *
         * @details
         * Removes all hotkey registrations for the given action.
         * Safe to call even if action isn't registered.
         */
        void unregister_action(std::shared_ptr<action<Backend>> action_ptr) {
            if (!action_ptr) return;

            // Remove all registrations for this action
            for (auto it = m_registrations.begin(); it != m_registrations.end();) {
                bool should_remove = false;

                for (auto& reg : it->second) {
                    if (auto reg_action = reg.get_action()) {
                        if (reg_action == action_ptr) {
                            should_remove = true;
                            break;
                        }
                    }
                }

                if (should_remove) {
                    it->second.erase(
                        std::remove_if(it->second.begin(), it->second.end(),
                            [&](const hotkey_registration<Backend>& reg) {
                                auto reg_action = reg.get_action();
                                return reg_action == action_ptr;
                            }),
                        it->second.end()
                    );

                    if (it->second.empty()) {
                        it = m_registrations.erase(it);
                    } else {
                        ++it;
                    }
                } else {
                    ++it;
                }
            }
        }

        /**
         * @brief Handle a keyboard event
         *
         * @param event The keyboard event (must satisfy HotkeyCapable concept)
         * @param focused_element Currently focused element (nullptr if none)
         * @return True if hotkey was found and triggered
         *
         * @details
         * Converts the keyboard event to a key_sequence and searches for
         * matching registered hotkeys.
         *
         * **Priority Order:**
         * 1. Framework semantic actions (from current hotkey scheme)
         * 2. Element-scoped application actions (focused element and ancestors)
         * 3. Global application actions
         *
         * This priority ensures framework-level shortcuts (F10 for menu)
         * take precedence over application shortcuts.
         *
         * **Example:**
         * @code
         * if (manager.handle_key_event(key_event, focused_widget)) {
         *     return true;  // Event handled, don't propagate
         * }
         * @endcode
         *
         * @tparam KeyEvent Type satisfying HotkeyCapable concept
         */
        template<HotkeyCapable KeyEvent>
        bool handle_key_event(
            const KeyEvent& event,
            ui_element<Backend>* focused_element = nullptr
        ) {
            using traits = event_traits<KeyEvent>;

            // Only handle key presses (not releases)
            if (!traits::is_key_press(event)) {
                return false;
            }

            // PRIORITY 0: Check for modifier-only activation (QBasic-style)
            if (handle_modifier_event(event)) {
                return true;  // Modifier-only action triggered
            }

            // Build key_sequence from event
            std::optional<key_sequence> seq = event_to_sequence(event);
            if (!seq) {
                return false;  // Not a hotkey candidate
            }

            // PRIORITY 0.5: Check for multi-key chords (Emacs-style)
            if (process_chord(*seq)) {
                return true;  // Chord completed or partial match
            }

            // PRIORITY 1: Framework semantic actions (from current scheme)
            if (m_scheme_registry) {
                if (try_semantic_action(*seq)) {
                    return true;
                }
            }

            // PRIORITY 2: Element-scoped application actions
            if (focused_element) {
                if (try_scoped_hotkeys(*seq, focused_element)) {
                    return true;
                }
            }

            // PRIORITY 3: Global application actions
            return try_global_hotkeys(*seq);
        }

        /**
         * @brief Handle framework-level ui_event (Phase 2 API)
         * @param evt Framework-level event (from backend's create_event())
         * @param focused_element Currently focused element (for scoped hotkeys)
         * @return True if event was handled as a hotkey
         *
         * @details
         * New event system API that accepts ui_event variant directly.
         * Only processes keyboard events - mouse and resize events are ignored.
         *
         * This is the preferred API for new code using the unified event system.
         * The old template-based handle_key_event() is maintained for backward
         * compatibility with event_traits-based code.
         *
         * ## Priority Order (same as old API)
         *
         * 1. **Framework semantic actions** (menu_down, menu_up, etc.)
         * 2. **Element-scoped application actions** (walk up tree)
         * 3. **Global application actions**
         * 4. **Widget keyboard handlers** (if not handled above)
         *
         * @example
         * @code
         * // Backend converts native event to ui_event
         * std::optional<ui_event> evt = Backend::create_event(native_event);
         * if (!evt) return false;
         *
         * // Process with hotkey manager
         * if (hotkey_mgr.handle_ui_event(evt.value(), focused_widget)) {
         *     return true;  // Handled as hotkey
         * }
         * @endcode
         *
         * @see handle_key_event For old event_traits-based API
         */
        bool handle_ui_event(
            const ui_event& evt,
            ui_element<Backend>* focused_element = nullptr
        ) {
            // Only process keyboard events
            auto* kbd = std::get_if<keyboard_event>(&evt);
            if (!kbd) {
                return false;  // Not a keyboard event
            }

            // Convert keyboard_event to key_sequence
            if (kbd->key == key_code::none) {
                return false;  // Not a hotkey candidate
            }
            key_sequence seq{kbd->key, kbd->modifiers};

            // PRIORITY 0.5: Check for multi-key chords (Emacs-style)
            if (process_chord(seq)) {
                return true;  // Chord completed or partial match
            }

            // PRIORITY 1: Framework semantic actions (from current scheme)
            if (m_scheme_registry) {
                if (try_semantic_action(seq)) {
                    return true;
                }
            }

            // PRIORITY 2: Element-scoped application actions
            if (focused_element) {
                if (try_scoped_hotkeys(seq, focused_element)) {
                    return true;
                }
            }

            // PRIORITY 3: Global application actions
            return try_global_hotkeys(seq);
        }

        /**
         * @brief Set conflict handling policy
         *
         * @param policy The new policy
         *
         * @details
         * Controls what happens when registering conflicting hotkeys:
         * - allow: Accept conflicts, last registered wins
         * - warn: Accept but log warning (default)
         * - error: Reject conflicting registrations
         */
        void set_conflict_policy(conflict_policy policy) noexcept {
            m_conflict_policy = policy;
        }

        /**
         * @brief Get conflict handling policy
         * @return Current policy
         */
        [[nodiscard]] conflict_policy get_conflict_policy() const noexcept {
            return m_conflict_policy;
        }

        /**
         * @brief Set hotkey scheme registry
         * @param registry Pointer to scheme registry (non-owning)
         *
         * @details
         * Sets or updates the scheme registry used for framework semantic actions.
         * Can be set after construction for lazy initialization.
         *
         * Setting to nullptr disables semantic action support.
         */
        void set_scheme_registry(hotkey_scheme_registry* registry) noexcept {
            m_scheme_registry = registry;
        }

        /**
         * @brief Get current scheme registry
         * @return Pointer to scheme registry, or nullptr if not set
         */
        [[nodiscard]] hotkey_scheme_registry* get_scheme_registry() const noexcept {
            return m_scheme_registry;
        }

        /**
         * @brief Register a handler for a framework semantic action
         * @param action The semantic action to handle
         * @param handler Callback to execute when action is triggered
         *
         * @details
         * Registers a callback for a framework semantic action (menu navigation,
         * focus, etc.). The handler will be invoked when the user presses the
         * key bound to this action in the current hotkey scheme.
         *
         * **Priority:**
         * Semantic actions are checked FIRST, before application actions.
         * This allows framework-level shortcuts (F10 for menu) to take priority.
         *
         * **Graceful Fallback:**
         * If a semantic action has no handler registered, no action is taken.
         * This allows minimal implementations.
         *
         * @example
         * @code
         * hotkey_manager<Backend> manager(&scheme_registry);
         *
         * // Register menu activation handler
         * manager.register_semantic_action(
         *     hotkey_action::activate_menu_bar,
         *     [&menu]() { menu->show(); }
         * );
         *
         * // Now F10 (Windows scheme) or F9 (Norton scheme) will show menu
         * @endcode
         */
        void register_semantic_action(
            hotkey_action action,
            std::function<void()> handler
        ) {
            m_semantic_handlers[action] = std::move(handler);
        }

        /**
         * @brief Unregister a semantic action handler
         * @param action The semantic action to unregister
         *
         * @details
         * Removes the handler for the given semantic action.
         * Safe to call even if no handler is registered.
         */
        void unregister_semantic_action(hotkey_action action) {
            m_semantic_handlers.erase(action);
        }

        /**
         * @brief Check if a semantic action has a registered handler
         * @param action The semantic action to check
         * @return True if handler is registered
         */
        [[nodiscard]] bool has_semantic_handler(hotkey_action action) const {
            return m_semantic_handlers.contains(action);
        }

        /**
         * @brief Check if a key sequence is registered
         *
         * @param seq The key sequence to check
         * @param scope_element Element scope (nullptr for global only)
         * @return True if registered
         */
        [[nodiscard]] bool is_registered(
            const key_sequence& seq,
            ui_element<Backend>* scope_element = nullptr
        ) const {
            auto it = m_registrations.find(seq);
            if (it == m_registrations.end()) {
                return false;
            }

            // Check if any registration matches the scope
            for (const auto& reg : it->second) {
                if (!reg.is_valid()) continue;

                if (scope_element == nullptr && reg.scope == hotkey_scope::global) {
                    return true;
                }

                if (scope_element != nullptr && reg.scope_target == scope_element) {
                    return true;
                }
            }

            return false;
        }

        /**
         * @brief Clean up expired registrations
         *
         * @details
         * Removes registrations for actions that no longer exist.
         * Called automatically during event handling, but can be
         * called explicitly for cleanup.
         */
        void cleanup() {
            for (auto it = m_registrations.begin(); it != m_registrations.end();) {
                // Remove expired registrations
                it->second.erase(
                    std::remove_if(it->second.begin(), it->second.end(),
                        [](const hotkey_registration<Backend>& reg) {
                            return !reg.is_valid();
                        }),
                    it->second.end()
                );

                // Remove empty entries
                if (it->second.empty()) {
                    it = m_registrations.erase(it);
                } else {
                    ++it;
                }
            }
        }

    private:
        /**
         * @brief Register action with explicit scope
         */
        bool register_action(
            std::shared_ptr<action<Backend>> action_ptr,
            hotkey_scope scope,
            ui_element<Backend>* scope_target
        ) {
            if (!action_ptr) {
                return false;
            }

            const auto& shortcut_opt = action_ptr->shortcut();
            if (!shortcut_opt.has_value()) {
                return false;
            }

            const auto& seq = *shortcut_opt;

            // Check for conflicts
            if (m_conflict_policy != conflict_policy::allow) {
                if (is_registered(seq, scope_target)) {
                    // Build conflict message (commented out with logging)
                    // std::string const shortcut_str = format_key_sequence(seq);
                    // std::string const scope_str = format_scope(scope, scope_target);
                    // std::string const action_name = action_ptr->text().empty()
                    //     ? "<unnamed>"
                    //     : action_ptr->text();

                    if (m_conflict_policy == conflict_policy::warn) {
                        // LOG_CAT_WARN("Hotkeys",
                        //     "Hotkey conflict detected: ", shortcut_str,
                        //     " is already registered in ", scope_str,
                        //     ". Action '", action_name, "' will override existing registration.");
                    } else if (m_conflict_policy == conflict_policy::error) {
                        // LOG_CAT_ERROR("Hotkeys",
                        //     "Hotkey registration rejected: ", shortcut_str,
                        //     " is already registered in ", scope_str,
                        //     ". Action '", action_name, "' cannot be registered.");
                        return false;
                    }
                }
            }

            // Add registration
            hotkey_registration<Backend> reg;
            reg.action_ptr = action_ptr;
            reg.sequence = seq;
            reg.scope = scope;
            reg.scope_target = scope_target;

            m_registrations[seq].push_back(reg);
            return true;
        }

        /**
         * @brief Convert keyboard event to key_sequence
         */
        template<HotkeyCapable KeyEvent>
        std::optional<key_sequence> event_to_sequence(const KeyEvent& event) {
            using traits = event_traits<KeyEvent>;

            // Build modifier flags
            key_modifier mods = key_modifier::none;
            if (traits::ctrl_pressed(event)) mods |= key_modifier::ctrl;
            if (traits::alt_pressed(event)) mods |= key_modifier::alt;
            if (traits::shift_pressed(event)) mods |= key_modifier::shift;

            // Try ASCII key (includes control characters like Escape, Enter, Tab)
            char const ascii = traits::to_ascii(event);
            if (ascii != '\0') {
                return key_sequence{ascii, mods};
            }

            // Try F-key (1-12 → key_code::f1-f12)
            int const f_key_num = traits::to_f_key(event);
            if (f_key_num != 0) {
                key_code code = function_key_from_number(f_key_num);
                if (code != key_code::none) {
                    return key_sequence{code, mods};
                }
            }

            // Try special key (arrow keys, etc.)
            int const special = traits::to_special_key(event);
            switch (special) {
                case -1: return key_sequence{key_code::arrow_up, mods};
                case -2: return key_sequence{key_code::arrow_down, mods};
                case -3: return key_sequence{key_code::arrow_left, mods};
                case -4: return key_sequence{key_code::arrow_right, mods};
                case -5: return key_sequence{key_code::home, mods};
                case -6: return key_sequence{key_code::end, mods};
                case -7: return key_sequence{key_code::page_up, mods};
                case -8: return key_sequence{key_code::page_down, mods};
                case -9: return key_sequence{key_code::insert, mods};
                case -10: return key_sequence{key_code::delete_key, mods};
                default: break;
            }

            return std::nullopt;
        }

        /**
         * @brief Convert keyboard_event to key_sequence (trivial specialization)
         *
         * @details
         * Since keyboard_event now uses key_code directly, conversion is trivial.
         * This specialization takes precedence over the generic template for keyboard_event.
         */
        std::optional<key_sequence> event_to_sequence(const keyboard_event& event) {
            if (event.key == key_code::none) {
                return std::nullopt;
            }
            return key_sequence{event.key, event.modifiers};
        }

        /**
         * @brief Try to trigger framework semantic action
         * @return True if semantic action was found and handler executed
         */
        bool try_semantic_action(const key_sequence& seq) {
            if (!m_scheme_registry) {
                return false;
            }

            // Get current hotkey scheme
            const auto* scheme = m_scheme_registry->get_current_scheme();
            if (!scheme) {
                return false;
            }

            // Find semantic action for this key
            auto action_opt = scheme->find_action_for_key(seq);
            if (!action_opt) {
                return false;  // Key not bound to any semantic action
            }

            // Check if we have a handler for this action
            auto handler_it = m_semantic_handlers.find(*action_opt);
            if (handler_it == m_semantic_handlers.end()) {
                return false;  // No handler registered (graceful fallback)
            }

            // Execute handler
            handler_it->second();
            return true;
        }

        /**
         * @brief Try to trigger scoped hotkeys
         */
        bool try_scoped_hotkeys(
            const key_sequence& seq,
            ui_element<Backend>* element
        ) {
            // Walk up the element tree checking for scoped hotkeys
            ui_element<Backend>* current = element;
            while (current != nullptr) {
                if (try_hotkeys_for_scope(seq, current)) {
                    return true;
                }
                current = current->parent();
            }
            return false;
        }

        /**
         * @brief Try to trigger global hotkeys
         */
        bool try_global_hotkeys(const key_sequence& seq) {
            return try_hotkeys_for_scope(seq, nullptr);
        }

        /**
         * @brief Try hotkeys for specific scope
         */
        bool try_hotkeys_for_scope(
            const key_sequence& seq,
            ui_element<Backend>* scope_target
        ) {
            auto it = m_registrations.find(seq);
            if (it == m_registrations.end()) {
                return false;
            }

            // Find matching registration
            for (const auto& reg : it->second) {
                if (!reg.is_valid()) continue;

                // Check scope match
                if (scope_target == nullptr && reg.scope != hotkey_scope::global) {
                    continue;
                }
                if (scope_target != nullptr && reg.scope_target != scope_target) {
                    continue;
                }

                // Trigger action
                if (auto act = reg.get_action()) {
                    if (act->is_enabled()) {
                        act->trigger();
                        return true;
                    }
                }
            }

            return false;
        }

        // NOTE: format_key_sequence() is now in key_sequence.hh
        // (removed duplicate to avoid inconsistency)

        /**
         * @brief Format scope information for logging
         */
        [[nodiscard]] static std::string format_scope(
            hotkey_scope scope,
            ui_element<Backend>* scope_target
        ) {
            switch (scope) {
                case hotkey_scope::global:
                    return "global scope";
                case hotkey_scope::window:
                    return scope_target
                        ? "window scope (element: " + std::to_string(reinterpret_cast<std::uintptr_t>(scope_target)) + ")"
                        : "window scope";
                case hotkey_scope::element:
                    return scope_target
                        ? "element scope (element: " + std::to_string(reinterpret_cast<std::uintptr_t>(scope_target)) + ")"
                        : "element scope";
                default:
                    return "unknown scope";
            }
        }

        std::map<key_sequence, std::vector<hotkey_registration<Backend>>> m_registrations;
        conflict_policy m_conflict_policy;

        // Scheme-aware semantic action support
        hotkey_scheme_registry* m_scheme_registry;                    ///< Non-owning pointer to scheme registry
        std::map<hotkey_action, std::function<void()>> m_semantic_handlers;  ///< Semantic action callbacks

        // Multi-key sequence support
        chord_matcher m_chord_matcher;                                ///< State machine for multi-key sequences
        std::map<key_chord, std::function<void()>> m_chord_handlers; ///< Handlers for multi-key chords

        // Modifier-only activation support (QBasic-style)
        std::map<key_modifier, std::function<void()>> m_modifier_handlers;  ///< Handlers for modifier-only keys
        key_modifier m_last_modifier_state = key_modifier::none;    ///< Track modifier state for press/release

    public:
        /**
         * @brief Register a multi-key chord (Emacs-style)
         * @param chord The key chord to register
         * @param handler Callback when chord is completed
         *
         * @example
         * @code
         * // Register Ctrl+X, Ctrl+C to exit
         * manager.register_chord(
         *     make_emacs_chord({
         *         {'x', key_modifier::ctrl},
         *         {'c', key_modifier::ctrl}
         *     }),
         *     []() { std::exit(0); }
         * );
         * @endcode
         */
        void register_chord(const key_chord& chord, std::function<void()> handler) {
            m_chord_handlers[chord] = std::move(handler);
        }

        /**
         * @brief Register modifier-only activation (QBasic-style)
         * @param mod Modifier key that triggers action
         * @param handler Callback when modifier is pressed alone
         *
         * @example
         * @code
         * // Alt key alone activates menu (like MS-DOS apps)
         * manager.register_modifier_activation(
         *     key_modifier::alt,
         *     [&menu]() { menu->activate(); }
         * );
         * @endcode
         */
        void register_modifier_activation(key_modifier mod, std::function<void()> handler) {
            m_modifier_handlers[mod] = std::move(handler);
        }

        /**
         * @brief Handle modifier key events (for single-key activation)
         * @param event The keyboard event
         * @return True if modifier-only action was triggered
         *
         * @details
         * Tracks modifier key press/release to detect single-key activation.
         * Must be called for both key press and release events.
         */
        template<KeyboardEvent KeyEvent>
        bool handle_modifier_event(const KeyEvent& event) {
            using traits = event_traits<KeyEvent>;

            // Get current modifier state
            key_modifier current_mods = key_modifier::none;
            if (traits::ctrl_pressed(event)) current_mods |= key_modifier::ctrl;
            if (traits::alt_pressed(event)) current_mods |= key_modifier::alt;
            if (traits::shift_pressed(event)) current_mods |= key_modifier::shift;

            // Check if this is a modifier-only press (no character key)
            char ascii = '\0';
            int f_key = 0;
            if constexpr (HotkeyCapable<KeyEvent>) {
                ascii = traits::to_ascii(event);
                f_key = traits::to_f_key(event);
            }

            bool is_modifier_only = (ascii == '\0' && f_key == 0);

            // On key press with just modifiers
            if (traits::is_key_press(event) && is_modifier_only) {
                // Check if we have a handler for this modifier
                auto it = m_modifier_handlers.find(current_mods);
                if (it != m_modifier_handlers.end()) {
                    it->second();  // Execute handler
                    return true;
                }
            }

            // Track modifier state for release detection
            m_last_modifier_state = current_mods;
            return false;
        }

        /**
         * @brief Process key event for chord matching
         * @param seq The key sequence from current event
         * @return True if a chord was completed and handled
         */
        bool process_chord(const key_sequence& seq) {
            // Check all registered chords
            for (auto& [chord, handler] : m_chord_handlers) {
                if (m_chord_matcher.process_key(seq, chord)) {
                    handler();  // Execute chord handler
                    return true;
                }
            }

            // Check if we have a partial match
            return m_chord_matcher.has_partial_match();
        }
    };

} // namespace onyxui
