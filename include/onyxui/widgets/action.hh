/**
 * @file action.hh
 * @brief Abstract user action that can be triggered from multiple UI elements
 * @author igor
 * @date 16/10/2025
 *
 * @details
 * Provides the action system for decoupling user commands from UI presentation,
 * similar to QAction in Qt. Actions represent abstract operations (Save, Copy, etc.)
 * that can be triggered from buttons, menu items, or hotkeys.
 *
 * ## Action System
 * An action encapsulates a user command with associated state and metadata.
 *
 * **Key Features:**
 * - Multiple widgets can share one action
 * - Automatic state synchronization (enable action → all widgets disabled)
 * - Checkable actions for toggle functionality
 * - Signal-based triggering (action → application logic)
 * - Backend-agnostic design
 * - Application-owned with weak_ptr in widgets (no circular references)
 *
 * **Useful For:**
 * - Toolbar buttons + menu items sharing same command
 * - Centralized enable/disable logic
 * - Hotkey support (future)
 * - Undo/redo systems
 * - Command pattern implementation
 *
 * ## Usage Examples
 *
 * ### Basic Action
 * ```cpp
 * // Application owns the action
 * auto save_action = std::make_shared<action<Backend>>();
 * save_action->set_text("Save");
 * save_action->set_enabled(true);
 *
 * // Connect to application logic
 * save_action->triggered.connect([]() {
 *     save_document();
 * });
 *
 * // Multiple widgets share the action
 * toolbar_button->set_action(save_action);
 * menu_item->set_action(save_action);
 *
 * // Disable action → both widgets become disabled
 * save_action->set_enabled(false);
 * ```
 *
 * ### Checkable Action (Toggle)
 * ```cpp
 * auto bold_action = std::make_shared<action<Backend>>();
 * bold_action->set_text("Bold");
 * bold_action->set_checkable(true);
 *
 * bold_action->checked_changed.connect([](bool checked) {
 *     set_text_bold(checked);
 * });
 *
 * // Toggle via action or widget
 * bold_action->set_checked(true);  // Updates all connected widgets
 * ```
 *
 * ### Action Group (Mutually Exclusive)
 * ```cpp
 * auto left_align = std::make_shared<action<Backend>>();
 * auto center_align = std::make_shared<action<Backend>>();
 * auto right_align = std::make_shared<action<Backend>>();
 *
 * auto align_group = std::make_shared<action_group<Backend>>();
 * align_group->add_action(left_align);
 * align_group->add_action(center_align);
 * align_group->add_action(right_align);
 *
 * // Checking one unchecks others (radio button behavior)
 * center_align->set_checked(true);  // left_align and right_align become unchecked
 * ```
 *
 * ### Keyboard Shortcuts
 * ```cpp
 * // ASCII keys with modifiers
 * auto save_action = std::make_shared<action<Backend>>();
 * save_action->set_text("Save");
 * save_action->set_shortcut('s', key_modifier::ctrl);  // Ctrl+S
 *
 * auto quit_action = std::make_shared<action<Backend>>();
 * quit_action->set_text("Quit");
 * quit_action->set_shortcut('q', key_modifier::alt);   // Alt+Q (Borland C style)
 *
 * // Function keys
 * auto help_action = std::make_shared<action<Backend>>();
 * help_action->set_text("Help");
 * help_action->set_shortcut_f(1);  // F1
 *
 * // Combined modifiers
 * auto save_as_action = std::make_shared<action<Backend>>();
 * save_as_action->set_text("Save As...");
 * save_as_action->set_shortcut('s', key_modifier::ctrl | key_modifier::shift);  // Ctrl+Shift+S
 *
 * // Check and clear shortcuts
 * if (save_action->has_shortcut()) {
 *     auto seq = save_action->shortcut().value();
 *     // Display shortcut hint in menu
 * }
 * save_action->clear_shortcut();  // Remove shortcut
 * ```
 *
 * ## Ownership Model
 * - **Application owns actions**: `std::shared_ptr<action>`
 * - **Widgets hold weak references**: `std::weak_ptr<action>`
 * - **No circular references**: Widgets don't keep actions alive
 * - **Action groups hold weak references**: Don't extend action lifetime
 *
 * ## Exception Safety
 * - Setters: Strong guarantee (no state change on exception)
 * - Getters: No-throw guarantee (all marked noexcept)
 * - trigger(): No-throw guarantee (signal emission is noexcept)
 *
 * ## Thread Safety
 * Not thread-safe. All operations must be performed on the UI thread.
 *
 * @see action_group For mutually exclusive actions
 * @see widget::set_action For widget integration
 */

#pragma once

#include <onyxui/signal.hh>
#include <onyxui/concepts/backend.hh>
#include <onyxui/hotkeys/key_sequence.hh>
#include <string>
#include <memory>
#include <optional>

namespace onyxui {
    // Forward declaration
    template<UIBackend Backend>
    class action_group;

    /**
     * @class action
     * @brief Abstract user command that can be triggered from multiple widgets
     *
     * @details
     * Action represents an abstract user operation (like "Save" or "Copy") that
     * can be invoked from multiple UI elements. When an action is triggered,
     * it emits a signal that application code can connect to.
     *
     * ## Behavior
     * - Triggered via widget interaction or programmatic call
     * - State synchronization across all connected widgets
     * - Optional checkable state for toggle actions
     * - Text/label that widgets can display
     * - Enable/disable affects all connected widgets
     *
     * ## Signals
     * - **triggered**: Emitted when action is invoked
     * - **text_changed**: Emitted when text/label changes
     * - **enabled_changed**: Emitted when enabled state changes
     * - **checked_changed**: Emitted when checked state changes (if checkable)
     * - **shortcut_changed**: Emitted when keyboard shortcut changes
     *
     * ## Common Use Cases
     * - File operations: Save, Open, Close
     * - Edit operations: Cut, Copy, Paste, Undo, Redo
     * - View toggles: Show Toolbar, Show Status Bar
     * - Formatting: Bold, Italic, Underline
     * - Alignment: Left, Center, Right (in action group)
     *
     * ## Memory Management
     * - Application owns actions via `shared_ptr`
     * - Widgets reference via `weak_ptr`
     * - Action groups reference via `weak_ptr`
     * - Action can outlive widgets (intentional)
     * - Action should NOT outlive application scope
     *
     * ## Exception Safety
     * - Constructor provides basic exception guarantee
     * - Setters provide strong exception guarantee (atomic state change)
     * - Getters are noexcept (no-throw guarantee)
     * - trigger() is noexcept (signal emission won't throw)
     *
     * @tparam Backend The backend traits type satisfying UIBackend concept
     *
     * @see action_group For coordinating mutually exclusive actions
     * @see widget::set_action For connecting actions to widgets
     */
    template<UIBackend Backend>
    class action : public std::enable_shared_from_this<action<Backend>> {
        public:
            // Signals
            signal<> triggered;                          ///< Emitted when action is triggered
            signal<std::string_view> text_changed;       ///< Emitted when text changes
            signal<bool> enabled_changed;                ///< Emitted when enabled state changes
            signal<bool> checked_changed;                ///< Emitted when checked state changes (if checkable)
            signal<const std::optional<key_sequence>&> shortcut_changed;  ///< Emitted when keyboard shortcut changes

            /**
             * @brief Construct an action with default state
             *
             * @throws std::bad_alloc If memory allocation fails
             *
             * @details
             * Creates an action with:
             * - Empty text
             * - Enabled: true
             * - Checkable: false
             * - Checked: false
             * - No action group
             * - No keyboard shortcut
             *
             * **Exception Safety:** Basic guarantee
             */
            action()
                : m_text()
                  , m_enabled(true)
                  , m_checkable(false)
                  , m_checked(false)
                  , m_group()
                  , m_shortcut() {}

            /**
             * @brief Virtual destructor
             */
            virtual ~action() = default;

            // Disable copy, allow move
            action(const action&) = delete;
            action& operator=(const action&) = delete;
            action(action&&) noexcept = default;
            action& operator=(action&&) noexcept = default;

            /**
             * @brief Set action text/label
             *
             * @param text The text to display (e.g., "Save", "Copy")
             *
             * @throws std::bad_alloc If string allocation fails
             *
             * @details
             * Sets the text that widgets should display for this action.
             * All connected widgets will be notified via text_changed signal.
             *
             * **Exception Safety:** Strong guarantee - text unchanged on exception
             *
             * @example
             * @code
             * save_action->set_text("Save");
             * save_as_action->set_text("Save As...");
             * @endcode
             */
            void set_text(std::string text) {
                if (m_text != text) {
                    m_text = std::move(text);
                    text_changed.emit(m_text);
                }
            }

            /**
             * @brief Get action text
             *
             * @return Current action text
             *
             * @note noexcept - guaranteed not to throw
             */
            [[nodiscard]] const std::string& text() const noexcept { return m_text; }

            /**
             * @brief Set enabled state
             *
             * @param enabled True to enable, false to disable
             *
             * @details
             * When disabled, connected widgets become non-interactive.
             * This is the primary way to enable/disable multiple UI elements at once.
             *
             * **Exception Safety:** No-throw guarantee (noexcept)
             *
             * @example
             * @code
             * // Disable save action when no document is open
             * save_action->set_enabled(document != nullptr);
             * @endcode
             */
            void set_enabled(bool enabled) noexcept {
                if (m_enabled != enabled) {
                    m_enabled = enabled;
                    enabled_changed.emit(enabled);
                }
            }

            /**
             * @brief Check if action is enabled
             *
             * @return True if enabled, false if disabled
             *
             * @note noexcept - guaranteed not to throw
             */
            [[nodiscard]] bool is_enabled() const noexcept { return m_enabled; }

            /**
             * @brief Set whether action is checkable
             *
             * @param checkable True for toggle action, false for regular action
             *
             * @details
             * Checkable actions maintain a checked/unchecked state, like
             * toggle buttons or checkboxes. Non-checkable actions just trigger.
             *
             * **Exception Safety:** No-throw guarantee (noexcept)
             *
             * @example
             * @code
             * bold_action->set_checkable(true);  // Toggle bold formatting
             * save_action->set_checkable(false); // Just trigger save
             * @endcode
             */
            void set_checkable(bool checkable) noexcept {
                if (m_checkable != checkable) {
                    m_checkable = checkable;
                    if (!checkable && m_checked) {
                        // Reset checked state when no longer checkable
                        m_checked = false;
                        checked_changed.emit(false);
                    }
                }
            }

            /**
             * @brief Check if action is checkable
             *
             * @return True if action supports checked state
             *
             * @note noexcept - guaranteed not to throw
             */
            [[nodiscard]] bool is_checkable() const noexcept { return m_checkable; }

            /**
             * @brief Set checked state
             *
             * @param checked True to check, false to uncheck
             *
             * @details
             * Only has effect if action is checkable. When an action is in
             * an exclusive action group, checking it will uncheck other actions
             * in the group.
             *
             * **Exception Safety:** No-throw guarantee (noexcept)
             *
             * @example
             * @code
             * bold_action->set_checked(true);  // Turn on bold
             * left_align->set_checked(true);   // Select left alignment (unchecks others in group)
             * @endcode
             */
            void set_checked(bool checked) noexcept;  // Implemented after action_group definition

            /**
             * @brief Check if action is checked
             *
             * @return True if checked, false if unchecked (always false for non-checkable actions)
             *
             * @note noexcept - guaranteed not to throw
             */
            [[nodiscard]] bool is_checked() const noexcept { return m_checked; }

            /**
             * @brief Set keyboard shortcut from key_sequence
             *
             * @param seq The key sequence (key + modifiers)
             *
             * @details
             * Assigns a keyboard shortcut to this action. When the hotkey manager
             * detects this key combination, it will trigger the action.
             *
             * **Exception Safety:** No-throw guarantee (noexcept)
             *
             * @example
             * @code
             * save_action->set_shortcut(key_sequence{'s', key_modifier::ctrl});
             * @endcode
             */
            void set_shortcut(key_sequence seq) noexcept {
                m_shortcut = seq;
                shortcut_changed.emit(m_shortcut);
            }

            /**
             * @brief Set keyboard shortcut from ASCII key and modifiers
             *
             * @param key ASCII character (a-z, 0-9, punctuation)
             * @param mods Modifier keys (Ctrl, Alt, Shift)
             *
             * @details
             * Convenience method for setting shortcuts using ASCII keys.
             * The key will be normalized to lowercase.
             *
             * **Exception Safety:** No-throw guarantee (noexcept)
             *
             * @example
             * @code
             * save_action->set_shortcut('s', key_modifier::ctrl);     // Ctrl+S
             * quit_action->set_shortcut('q', key_modifier::alt);      // Alt+Q
             * save_as->set_shortcut('s', key_modifier::ctrl | key_modifier::shift);  // Ctrl+Shift+S
             * @endcode
             */
            void set_shortcut(char key, key_modifier mods = key_modifier::none) noexcept {
                set_shortcut(key_sequence{key, mods});
            }

            /**
             * @brief Set keyboard shortcut from F-key and modifiers
             *
             * @param f_key F-key number (1-12 for F1-F12)
             * @param mods Modifier keys (Ctrl, Alt, Shift)
             *
             * @details
             * Convenience method for setting shortcuts using function keys.
             *
             * **Exception Safety:** No-throw guarantee (noexcept)
             *
             * @example
             * @code
             * help_action->set_shortcut_f(1);           // F1
             * menu_action->set_shortcut_f(9);           // F9 (Norton Commander style)
             * close_action->set_shortcut_f(4, key_modifier::alt);  // Alt+F4
             * @endcode
             */
            void set_shortcut_f(int f_key, key_modifier mods = key_modifier::none) noexcept {
                set_shortcut(key_sequence{function_key_from_number(f_key), mods});
            }

            /**
             * @brief Clear keyboard shortcut
             *
             * @details
             * Removes any assigned keyboard shortcut from this action.
             *
             * **Exception Safety:** No-throw guarantee (noexcept)
             *
             * @example
             * @code
             * action->clear_shortcut();  // Remove shortcut
             * @endcode
             */
            void clear_shortcut() noexcept {
                m_shortcut.reset();
                shortcut_changed.emit(m_shortcut);
            }

            /**
             * @brief Get keyboard shortcut
             *
             * @return Optional key sequence (empty if no shortcut assigned)
             *
             * @note noexcept - guaranteed not to throw
             *
             * @example
             * @code
             * if (auto shortcut = action->shortcut()) {
             *     display_shortcut(*shortcut);
             * }
             * @endcode
             */
            [[nodiscard]] const std::optional<key_sequence>& shortcut() const noexcept {
                return m_shortcut;
            }

            /**
             * @brief Check if action has a keyboard shortcut
             *
             * @return True if shortcut is assigned
             *
             * @note noexcept - guaranteed not to throw
             *
             * @example
             * @code
             * if (action->has_shortcut()) {
             *     show_shortcut_hint(action->shortcut().value());
             * }
             * @endcode
             */
            [[nodiscard]] bool has_shortcut() const noexcept {
                return m_shortcut.has_value();
            }

            /**
             * @brief Trigger the action programmatically
             *
             * @details
             * Emits the triggered signal, which application code should be connected to.
             * This is called by widgets when they're activated, but can also be called
             * directly (e.g., for hotkeys or programmatic invocation).
             *
             * For checkable actions, this automatically toggles the checked state.
             *
             * **Exception Safety:** No-throw guarantee (noexcept)
             *
             * @example
             * @code
             * // Hotkey handler
             * if (key == CTRL_S) {
             *     save_action->trigger();
             * }
             * @endcode
             */
            void trigger() noexcept {
                if (!m_enabled) return;  // Don't trigger disabled actions

                // Toggle checked state for checkable actions
                if (m_checkable) {
                    set_checked(!m_checked);
                }

                triggered.emit();
            }

        private:
            friend class action_group<Backend>;

            /**
             * @brief Set the action group (called by action_group)
             *
             * @param group Weak pointer to the action group
             *
             * @details
             * Internal method used by action_group to register this action.
             * Not intended for public use.
             */
            void set_group(std::weak_ptr<action_group<Backend>> group) noexcept {
                m_group = group;
            }

            std::string m_text;                           ///< Action text/label
            bool m_enabled;                               ///< Enabled/disabled state
            bool m_checkable;                             ///< Whether action can be checked
            bool m_checked;                               ///< Checked state (if checkable)
            std::weak_ptr<action_group<Backend>> m_group; ///< Optional action group
            std::optional<key_sequence> m_shortcut;       ///< Keyboard shortcut (if assigned)
    };

} // namespace onyxui

// Include action_group to complete the action implementation
#include <onyxui/widgets/action_group.hh>

namespace onyxui {
    // Implementation of set_checked (needs action_group definition)
    template<UIBackend Backend>
    inline void action<Backend>::set_checked(bool checked) noexcept {
        if (!m_checkable) return;  // Only checkable actions can be checked

        if (m_checked != checked) {
            m_checked = checked;
            checked_changed.emit(checked);

            // Notify action group if this action is being checked
            if (checked) {
                if (auto group = m_group.lock()) {
                    group->on_action_checked(this->shared_from_this());
                }
            }
        }
    }
} // namespace onyxui
