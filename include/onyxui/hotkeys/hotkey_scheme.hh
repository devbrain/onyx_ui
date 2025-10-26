/**
 * @file hotkey_scheme.hh
 * @brief Hotkey scheme structure (parallel to ui_theme)
 * @author Claude Code
 * @date 2025-10-26
 *
 * @details
 * Defines customizable key bindings for framework semantic actions.
 * This is user preference ("look and feel"), NOT backend-specific.
 *
 * ## Design Philosophy
 *
 * Hotkey schemes work similarly to themes, but for keyboard shortcuts:
 * - **Theme**: Visual appearance (colors, borders, fonts)
 * - **Hotkey Scheme**: Keyboard behavior (which keys do what)
 *
 * Both are user preferences that work across all backends.
 *
 * ## Usage Example
 *
 * ```cpp
 * // Create a scheme
 * hotkey_scheme scheme;
 * scheme.name = "My Custom Scheme";
 * scheme.set_binding(hotkey_action::activate_menu_bar, parse_key_sequence("F10"));
 * scheme.set_binding(hotkey_action::menu_down, parse_key_sequence("Down"));
 *
 * // Query bindings
 * auto key = scheme.get_binding(hotkey_action::activate_menu_bar);
 * if (key) {
 *     std::cout << "Menu opens with: " << format_key_sequence(*key) << "\n";
 * } else {
 *     std::cout << "No hotkey for menu (use mouse)\n";
 * }
 *
 * // Find action for a key
 * auto action = scheme.find_action_for_key(parse_key_sequence("F10"));
 * if (action) {
 *     std::cout << "F10 triggers: " << to_string(*action) << "\n";
 * }
 * ```
 */

#pragma once

#include <string>
#include <map>
#include <optional>
#include <onyxui/hotkeys/hotkey_action.hh>
#include <onyxui/hotkeys/key_sequence.hh>

namespace onyxui {

    /**
     * @struct hotkey_scheme
     * @brief Collection of keyboard shortcuts (parallel to ui_theme)
     *
     * @details
     * Maps semantic actions (what to do) to key sequences (how to trigger).
     * Different schemes allow different user preferences:
     * - "Windows" scheme: F10 for menu activation
     * - "Norton Commander" scheme: F9 for menu activation
     * - "Minimal" scheme: Only essential keys (or none for mouse-only)
     *
     * ## Graceful Degradation
     *
     * If an action has no binding, widgets fall back to mouse operation.
     * This allows:
     * - Minimal schemes (only essential keys)
     * - Mouse-only schemes (no keyboard bindings at all)
     * - Progressive enhancement (start minimal, add keys as needed)
     *
     * ## Key Uniqueness
     *
     * A key sequence can trigger only ONE action, but an action can have
     * MULTIPLE keys (not currently supported, but possible future extension).
     *
     * ## Comparison to Themes
     *
     * | Aspect | Theme | Hotkey Scheme |
     * |--------|-------|---------------|
     * | Controls | Visual appearance | Keyboard behavior |
     * | Examples | Colors, fonts, borders | F10 vs F9 for menu |
     * | Backend-specific? | Partially (colors, rendering) | No (universal keys) |
     * | User preference | Yes | Yes |
     * | Registered where | Backend (platform capabilities) | Library (universal) |
     */
    struct hotkey_scheme {
        std::string name;                                      ///< Scheme name (e.g., "Windows", "Norton Commander")
        std::string description;                               ///< Human-readable description
        std::map<hotkey_action, key_sequence> bindings;        ///< Action → key mapping

        /**
         * @brief Get key binding for an action
         * @param action The semantic action to query
         * @return Key sequence if bound, nullopt otherwise
         *
         * @details
         * Returns nullopt if action has no binding - this is NOT an error!
         * Widgets should gracefully fall back to mouse operation.
         *
         * @example
         * @code
         * auto key = scheme.get_binding(hotkey_action::activate_menu_bar);
         * if (key) {
         *     // Register hotkey handler
         *     hotkeys->register_semantic_action(activate_menu_bar, handler);
         * } else {
         *     // No keyboard shortcut - mouse-only is fine
         * }
         * @endcode
         */
        [[nodiscard]] std::optional<key_sequence> get_binding(hotkey_action action) const {
            auto it = bindings.find(action);
            return (it != bindings.end()) ? std::optional{it->second} : std::nullopt;
        }

        /**
         * @brief Find action triggered by a key
         * @param seq Key sequence to match
         * @return Action if found, nullopt otherwise
         *
         * @details
         * Searches through all bindings to find which action (if any)
         * is triggered by the given key sequence.
         *
         * Used by hotkey_manager to dispatch key events:
         * 1. Convert event to key_sequence
         * 2. Ask scheme: which action does this key trigger?
         * 3. Find registered handler for that action
         * 4. Execute handler
         *
         * @example
         * @code
         * auto action = scheme.find_action_for_key(parse_key_sequence("F10"));
         * if (action) {
         *     std::cout << "F10 triggers: " << to_string(*action) << "\n";
         * }
         * @endcode
         */
        [[nodiscard]] std::optional<hotkey_action> find_action_for_key(const key_sequence& seq) const {
            for (const auto& [action, binding] : bindings) {
                if (binding == seq) {
                    return action;
                }
            }
            return std::nullopt;
        }

        /**
         * @brief Check if action has a binding
         * @param action The semantic action
         * @return true if action is bound to a key
         */
        [[nodiscard]] bool has_binding(hotkey_action action) const {
            return bindings.contains(action);
        }

        /**
         * @brief Set binding for an action
         * @param action The semantic action
         * @param seq The key sequence to bind
         *
         * @details
         * Overwrites any existing binding for this action.
         * If seq is already bound to another action, that binding is NOT removed.
         * (Future: Could add conflict detection)
         */
        void set_binding(hotkey_action action, key_sequence seq) {
            bindings[action] = seq;
        }

        /**
         * @brief Remove binding for an action
         * @param action The semantic action to unbind
         *
         * @details
         * After removal, get_binding(action) will return nullopt.
         * Safe to call even if action has no binding.
         */
        void remove_binding(hotkey_action action) {
            bindings.erase(action);
        }

        /**
         * @brief Clear all bindings
         *
         * @details
         * Creates an empty scheme (mouse-only).
         * Useful for testing or creating minimal schemes.
         */
        void clear() {
            bindings.clear();
        }

        /**
         * @brief Get number of bindings
         * @return Number of actions with key bindings
         */
        [[nodiscard]] size_t binding_count() const noexcept {
            return bindings.size();
        }
    };

} // namespace onyxui
