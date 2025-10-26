/**
 * @file hotkey_action.hh
 * @brief Semantic actions for customizable hotkey schemes
 * @author Claude Code
 * @date 2025-10-26
 *
 * @details
 * Provides semantic action enumeration for hotkey binding. Actions represent
 * user intent (e.g., "activate menu"), while key sequences represent physical
 * keys (e.g., "F10"). This separation allows for customizable hotkey schemes
 * similar to the theme system.
 *
 * ## Design Philosophy
 *
 * **Separation of Concerns:**
 * - **hotkey_action**: Semantic intent (what the user wants to do)
 * - **key_sequence**: Physical keys (how to invoke the action)
 * - **hotkey_scheme**: Mapping between actions and keys
 *
 * This allows different UIs to use different key bindings:
 * - Windows-style: F10 activates menu
 * - Norton Commander: F9 activates menu
 * - Custom: Any key combination
 *
 * ## Usage Example
 *
 * ```cpp
 * // Create a scheme
 * hotkey_scheme windows_scheme;
 * windows_scheme.set_binding(
 *     hotkey_action::activate_menu_bar,
 *     parse_key_sequence("F10")
 * );
 *
 * // Query bindings
 * auto seq = windows_scheme.get_binding(hotkey_action::activate_menu_bar);
 * if (seq) {
 *     std::cout << "Menu activates with: " << format_key_sequence(*seq) << "\n";
 * }
 * ```
 */

#pragma once

#include <cstdint>

namespace onyxui {

    /**
     * @enum hotkey_action
     * @brief Semantic actions for keyboard shortcuts
     *
     * @details
     * Represents user intent independent of physical key bindings.
     * Actions are organized by functional area (menu navigation, focus, etc.).
     *
     * **Current scope (Phase 1):**
     * - Menu navigation only
     *
     * **Future extensions:**
     * - File operations (new, open, save, etc.)
     * - Edit operations (cut, copy, paste, undo, redo)
     * - Application control (quit, settings, help)
     * - Custom user-defined actions
     */
    enum class hotkey_action : std::uint8_t {
        // Menu Navigation
        activate_menu_bar,  ///< Activate the menu bar (F10 in Windows, F9 in Norton)
        menu_up,            ///< Navigate to previous menu item (Up arrow)
        menu_down,          ///< Navigate to next menu item (Down arrow)
        menu_left,          ///< Navigate to previous menu or close submenu (Left arrow)
        menu_right,         ///< Navigate to next menu or open submenu (Right arrow)
        menu_select,        ///< Select/activate current menu item (Enter)
        menu_cancel,        ///< Close menu without selection (Escape)

        // Focus Navigation
        focus_next,         ///< Move focus to next widget (Tab)
        focus_previous,     ///< Move focus to previous widget (Shift+Tab)

        // Sentinel value for iteration/validation
        action_count        ///< Total number of actions (not a valid action)
    };

    /**
     * @brief Get human-readable name for an action
     * @param action The hotkey action
     * @return String representation (e.g., "activate_menu_bar")
     *
     * @details
     * Useful for debugging, logging, and UI display. Returns the exact
     * enum name as a string.
     */
    constexpr const char* to_string(hotkey_action action) noexcept {
        switch (action) {
            case hotkey_action::activate_menu_bar: return "activate_menu_bar";
            case hotkey_action::menu_up: return "menu_up";
            case hotkey_action::menu_down: return "menu_down";
            case hotkey_action::menu_left: return "menu_left";
            case hotkey_action::menu_right: return "menu_right";
            case hotkey_action::menu_select: return "menu_select";
            case hotkey_action::menu_cancel: return "menu_cancel";
            case hotkey_action::focus_next: return "focus_next";
            case hotkey_action::focus_previous: return "focus_previous";
            case hotkey_action::action_count: return "action_count";
        }
        return "unknown";
    }

} // namespace onyxui
