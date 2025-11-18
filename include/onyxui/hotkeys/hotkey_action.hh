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
     * Actions are organized by functional area (menu navigation, focus, window management, etc.).
     *
     * **Current scope:**
     * - Menu navigation
     * - Focus management
     * - Scrolling
     * - Window management (keyboard-based movement and state changes)
     *
     * **Future extensions:**
     * - File operations (new, open, save, etc.)
     * - Edit operations (cut, copy, paste, undo, redo)
     * - Application control (quit, settings, help)
     * - Custom user-defined actions
     *
     * **Note on Window Movement:**
     * Mouse dragging is handled via direct event processing, not semantic actions.
     * Semantic actions are for keyboard-based window management (accessibility).
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

        // Widget Activation
        activate_focused,   ///< Activate currently focused widget (Enter/Space in most schemes)

        // Scrolling Navigation
        scroll_up,          ///< Scroll content up one line (Up arrow in scroll contexts)
        scroll_down,        ///< Scroll content down one line (Down arrow in scroll contexts)
        scroll_page_up,     ///< Scroll content up one page (Page Up)
        scroll_page_down,   ///< Scroll content down one page (Page Down)
        scroll_home,        ///< Scroll to top of content (Home)
        scroll_end,         ///< Scroll to end of content (End)

        // Window Management - Turbo Vision style (Ctrl+W, Ctrl+Tab, etc.)
        enter_window_move_mode,   ///< Start keyboard window moving (Alt+F7 Windows, Alt+F8 Norton)
        enter_window_resize_mode, ///< Start keyboard window resizing (Alt+F8 Windows)
        minimize_window,          ///< Minimize active window (Alt+F9 Norton)
        maximize_window,          ///< Maximize/restore active window (Alt+F5 Norton, F5 Windows)
        close_window,             ///< Close active window (Alt+F3 Norton, Alt+F4 Windows)
        window_menu,              ///< Open window system menu (Alt+Space Windows, Alt+0 Norton)
        next_window,              ///< Switch to next window (Ctrl+F6 Windows, Ctrl+Tab)
        previous_window,          ///< Switch to previous window (Ctrl+Shift+F6, Ctrl+Shift+Tab)
        show_window_list,         ///< Show window list dialog (Ctrl+W Turbo Vision style)

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
            case hotkey_action::activate_focused: return "activate_focused";
            case hotkey_action::scroll_up: return "scroll_up";
            case hotkey_action::scroll_down: return "scroll_down";
            case hotkey_action::scroll_page_up: return "scroll_page_up";
            case hotkey_action::scroll_page_down: return "scroll_page_down";
            case hotkey_action::scroll_home: return "scroll_home";
            case hotkey_action::scroll_end: return "scroll_end";
            case hotkey_action::enter_window_move_mode: return "enter_window_move_mode";
            case hotkey_action::enter_window_resize_mode: return "enter_window_resize_mode";
            case hotkey_action::minimize_window: return "minimize_window";
            case hotkey_action::maximize_window: return "maximize_window";
            case hotkey_action::close_window: return "close_window";
            case hotkey_action::window_menu: return "window_menu";
            case hotkey_action::next_window: return "next_window";
            case hotkey_action::previous_window: return "previous_window";
            case hotkey_action::show_window_list: return "show_window_list";
            case hotkey_action::action_count: return "action_count";
        }
        return "unknown";
    }

} // namespace onyxui
