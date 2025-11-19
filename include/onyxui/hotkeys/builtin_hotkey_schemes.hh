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

            // Window Management (Phase 2)
            {hotkey_action::enter_window_move_mode, parse_key_sequence("Alt+F7")},
            {hotkey_action::enter_window_resize_mode, parse_key_sequence("Alt+F8")},
            {hotkey_action::minimize_window, parse_key_sequence("Alt+F9")},
            {hotkey_action::maximize_window, parse_key_sequence("F5")},
            {hotkey_action::close_window, parse_key_sequence("Alt+F4")},
            {hotkey_action::window_menu, parse_key_sequence("Alt+Space")},
            {hotkey_action::next_window, parse_key_sequence("Ctrl+Tab")},
            {hotkey_action::previous_window, parse_key_sequence("Ctrl+Shift+Tab")},
            {hotkey_action::show_window_list, parse_key_sequence("Ctrl+W")},

            // Text Editing - Cursor Movement
            {hotkey_action::cursor_move_left, parse_key_sequence("Left")},
            {hotkey_action::cursor_move_right, parse_key_sequence("Right")},
            {hotkey_action::cursor_move_word_left, parse_key_sequence("Ctrl+Left")},
            {hotkey_action::cursor_move_word_right, parse_key_sequence("Ctrl+Right")},
            {hotkey_action::cursor_move_home, parse_key_sequence("Home")},
            {hotkey_action::cursor_move_end, parse_key_sequence("End")},

            // Text Editing - Selection
            {hotkey_action::cursor_select_left, parse_key_sequence("Shift+Left")},
            {hotkey_action::cursor_select_right, parse_key_sequence("Shift+Right")},
            {hotkey_action::cursor_select_word_left, parse_key_sequence("Ctrl+Shift+Left")},
            {hotkey_action::cursor_select_word_right, parse_key_sequence("Ctrl+Shift+Right")},
            {hotkey_action::cursor_select_home, parse_key_sequence("Shift+Home")},
            {hotkey_action::cursor_select_end, parse_key_sequence("Shift+End")},
            {hotkey_action::cursor_select_all, parse_key_sequence("Ctrl+A")},

            // Text Editing - Deletion
            {hotkey_action::text_delete_char, parse_key_sequence("Delete")},
            {hotkey_action::text_backspace, parse_key_sequence("Backspace")},
            {hotkey_action::text_delete_word, parse_key_sequence("Ctrl+Delete")},
            {hotkey_action::text_backspace_word, parse_key_sequence("Ctrl+Backspace")},

            // Text Editing - Clipboard
            {hotkey_action::text_copy, parse_key_sequence("Ctrl+C")},
            {hotkey_action::text_cut, parse_key_sequence("Ctrl+X")},
            {hotkey_action::text_paste, parse_key_sequence("Ctrl+V")},

            // Text Editing - Mode Toggle
            {hotkey_action::text_toggle_overwrite, parse_key_sequence("Insert")},
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

            // Window Management (Phase 2) - Norton Commander style
            {hotkey_action::enter_window_move_mode, parse_key_sequence("Alt+F8")},  // Norton: Alt+F8
            {hotkey_action::enter_window_resize_mode, parse_key_sequence("Ctrl+F8")},
            {hotkey_action::minimize_window, parse_key_sequence("Alt+F9")},
            {hotkey_action::maximize_window, parse_key_sequence("Alt+F5")},
            {hotkey_action::close_window, parse_key_sequence("Alt+F3")},  // Norton: Alt+F3
            {hotkey_action::window_menu, parse_key_sequence("Alt+0")},    // Norton: Alt+0
            {hotkey_action::next_window, parse_key_sequence("Ctrl+F6")},
            {hotkey_action::previous_window, parse_key_sequence("Ctrl+Shift+F6")},
            {hotkey_action::show_window_list, parse_key_sequence("Ctrl+W")},

            // Text Editing - Cursor Movement (same as Windows)
            {hotkey_action::cursor_move_left, parse_key_sequence("Left")},
            {hotkey_action::cursor_move_right, parse_key_sequence("Right")},
            {hotkey_action::cursor_move_word_left, parse_key_sequence("Ctrl+Left")},
            {hotkey_action::cursor_move_word_right, parse_key_sequence("Ctrl+Right")},
            {hotkey_action::cursor_move_home, parse_key_sequence("Home")},
            {hotkey_action::cursor_move_end, parse_key_sequence("End")},

            // Text Editing - Selection (same as Windows)
            {hotkey_action::cursor_select_left, parse_key_sequence("Shift+Left")},
            {hotkey_action::cursor_select_right, parse_key_sequence("Shift+Right")},
            {hotkey_action::cursor_select_word_left, parse_key_sequence("Ctrl+Shift+Left")},
            {hotkey_action::cursor_select_word_right, parse_key_sequence("Ctrl+Shift+Right")},
            {hotkey_action::cursor_select_home, parse_key_sequence("Shift+Home")},
            {hotkey_action::cursor_select_end, parse_key_sequence("Shift+End")},
            {hotkey_action::cursor_select_all, parse_key_sequence("Ctrl+A")},

            // Text Editing - Deletion (same as Windows)
            {hotkey_action::text_delete_char, parse_key_sequence("Delete")},
            {hotkey_action::text_backspace, parse_key_sequence("Backspace")},
            {hotkey_action::text_delete_word, parse_key_sequence("Ctrl+Delete")},
            {hotkey_action::text_backspace_word, parse_key_sequence("Ctrl+Backspace")},

            // Text Editing - Clipboard (same as Windows)
            {hotkey_action::text_copy, parse_key_sequence("Ctrl+C")},
            {hotkey_action::text_cut, parse_key_sequence("Ctrl+X")},
            {hotkey_action::text_paste, parse_key_sequence("Ctrl+V")},

            // Text Editing - Mode Toggle (same as Windows)
            {hotkey_action::text_toggle_overwrite, parse_key_sequence("Insert")},
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
