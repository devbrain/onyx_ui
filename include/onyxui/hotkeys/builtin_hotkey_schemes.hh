/**
 * @file builtin_hotkey_schemes.hh
 * @brief Built-in hotkey schemes (library-provided, NOT backend-specific)
 * @author Claude Code
 * @date 2025-10-26
 *
 * @details
 * Provides standard hotkey schemes that work across all backends.
 * These are LIBRARY-LEVEL, not backend-specific.
 *
 * ## Why Not Backend-Specific?
 *
 * Hotkey schemes are user preference ("look and feel"), not technical limitation:
 * - F10 works the same on DOS console, SDL window, and web canvas
 * - User muscle memory (Norton vs Windows) transcends platform
 * - Keyboard shortcuts are universal, unlike colors/rendering
 *
 * Compare to themes:
 * - Themes CAN be backend-specific (16-color DOS vs 24-bit SDL)
 * - Hotkey schemes CANNOT be backend-specific (keys are universal)
 *
 * ## Available Schemes
 *
 * 1. **Windows** (default) - F10 for menu, standard Windows conventions
 * 2. **Norton Commander** - F9 for menu, classic DOS feel
 *
 * ## Usage Example
 *
 * ```cpp
 * // Automatic registration (happens in ui_context)
 * scoped_ui_context<Backend> ctx;
 *
 * // Built-in schemes already registered!
 * auto* windows = ctx.hotkey_schemes().get_scheme("Windows");
 * auto* norton = ctx.hotkey_schemes().get_scheme("Norton Commander");
 *
 * // Switch to Norton
 * ctx.hotkey_schemes().set_current_scheme("Norton Commander");
 * // Now F9 activates menu instead of F10
 * ```
 */

#pragma once

#include <onyxui/hotkeys/hotkey_scheme.hh>

namespace onyxui::builtin_hotkey_schemes {

    /**
     * @brief Windows-style hotkey scheme
     * @return Scheme with standard Windows keyboard shortcuts
     *
     * @details
     * Uses Windows conventions familiar to modern users:
     * - **F10**: Activate menu bar
     * - **Arrow keys**: Navigate menus
     * - **Enter**: Select/activate item
     * - **Escape**: Close/cancel
     * - **Tab/Shift+Tab**: Focus navigation
     *
     * ## History
     *
     * Windows has used F10 for menu activation since Windows 1.0 (1985).
     * This convention was inherited from IBM CUA (Common User Access)
     * standards and is now ubiquitous in GUI applications.
     *
     * ## When to Use
     *
     * - Default choice for modern applications
     * - Users expect F10 for menu in GUI applications
     * - Consistent with Windows, Linux desktop environments, macOS
     */
    inline hotkey_scheme windows() {
        hotkey_scheme scheme;
        scheme.name = "Windows";
        scheme.description = "Standard Windows keyboard shortcuts (F10 for menu)";

        scheme.bindings = {
            // Menu Navigation
            {hotkey_action::activate_menu_bar, parse_key_sequence("F10")},
            {hotkey_action::menu_up, parse_key_sequence("Up")},
            {hotkey_action::menu_down, parse_key_sequence("Down")},
            {hotkey_action::menu_left, parse_key_sequence("Left")},
            {hotkey_action::menu_right, parse_key_sequence("Right")},
            {hotkey_action::menu_select, parse_key_sequence("Enter")},
            {hotkey_action::menu_cancel, parse_key_sequence("Escape")},

            // Focus Navigation
            {hotkey_action::focus_next, parse_key_sequence("Tab")},
            {hotkey_action::focus_previous, parse_key_sequence("Shift+Tab")},

            // Widget Activation
            {hotkey_action::activate_focused, parse_key_sequence("Enter")},

            // Scrolling Navigation
            {hotkey_action::scroll_up, parse_key_sequence("Up")},
            {hotkey_action::scroll_down, parse_key_sequence("Down")},
            {hotkey_action::scroll_page_up, parse_key_sequence("PageUp")},
            {hotkey_action::scroll_page_down, parse_key_sequence("PageDown")},
            {hotkey_action::scroll_home, parse_key_sequence("Home")},
            {hotkey_action::scroll_end, parse_key_sequence("End")},
        };

        return scheme;
    }

    /**
     * @brief Norton Commander-style hotkey scheme
     * @return Scheme with classic DOS Norton Commander shortcuts
     *
     * @details
     * Uses Norton Commander conventions from the DOS era:
     * - **F9**: Activate menu bar (Norton convention)
     * - **Arrow keys**: Navigate menus
     * - **Enter**: Select/activate item
     * - **Escape**: Close/cancel
     * - **Tab/Shift+Tab**: Focus navigation
     *
     * ## History
     *
     * Norton Commander (1986-1998) was the iconic DOS file manager.
     * It used F9 for menu activation, reserving F10 for quit.
     * Many DOS-era users have muscle memory for F9 = menu.
     *
     * Other programs using F9 for menu:
     * - Midnight Commander (Norton Commander clone)
     * - Volkov Commander
     * - Total Commander (early versions)
     *
     * ## When to Use
     *
     * - Nostalgia for classic DOS applications
     * - Terminal/console applications (TUI feel)
     * - Users who grew up with Norton Commander
     * - Retro-style interfaces
     *
     * ## Implementation Note
     *
     * Only the menu activation key differs from Windows scheme.
     * All other keys (arrows, Enter, Escape, Tab) are the same.
     */
    inline hotkey_scheme norton_commander() {
        hotkey_scheme scheme;
        scheme.name = "Norton Commander";
        scheme.description = "Classic DOS Norton Commander shortcuts (F9 for menu)";

        scheme.bindings = {
            // Menu Navigation
            {hotkey_action::activate_menu_bar, parse_key_sequence("F9")},  // Norton uses F9!
            {hotkey_action::menu_up, parse_key_sequence("Up")},
            {hotkey_action::menu_down, parse_key_sequence("Down")},
            {hotkey_action::menu_left, parse_key_sequence("Left")},
            {hotkey_action::menu_right, parse_key_sequence("Right")},
            {hotkey_action::menu_select, parse_key_sequence("Enter")},
            {hotkey_action::menu_cancel, parse_key_sequence("Escape")},

            // Focus Navigation
            {hotkey_action::focus_next, parse_key_sequence("Tab")},
            {hotkey_action::focus_previous, parse_key_sequence("Shift+Tab")},

            // Widget Activation
            {hotkey_action::activate_focused, parse_key_sequence("Enter")},

            // Scrolling Navigation
            {hotkey_action::scroll_up, parse_key_sequence("Up")},
            {hotkey_action::scroll_down, parse_key_sequence("Down")},
            {hotkey_action::scroll_page_up, parse_key_sequence("PageUp")},
            {hotkey_action::scroll_page_down, parse_key_sequence("PageDown")},
            {hotkey_action::scroll_home, parse_key_sequence("Home")},
            {hotkey_action::scroll_end, parse_key_sequence("End")},
        };

        return scheme;
    }

    // Future schemes (not implemented yet):
    //
    // inline hotkey_scheme emacs() { ... }
    // - Ctrl+X Ctrl+C for quit
    // - Meta (Alt) based navigation
    //
    // inline hotkey_scheme vim() { ... }
    // - j/k for up/down
    // - hjkl navigation
    // - ESC to exit modes
    //
    // inline hotkey_scheme macos() { ... }
    // - Cmd instead of Ctrl
    // - Option instead of Alt
    //
    // inline hotkey_scheme minimal() { ... }
    // - Only essential keys (Enter, Escape)
    // - Primarily mouse-driven

} // namespace onyxui::builtin_hotkey_schemes
