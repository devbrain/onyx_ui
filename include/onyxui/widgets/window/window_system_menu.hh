/**
 * @file window_system_menu.hh
 * @brief System menu for window control (Restore, Move, Size, Minimize, Maximize, Close)
 * @author Claude Code
 * @date 2025-11-09
 *
 * @details
 * The system menu is a popup menu that appears when:
 * - User clicks the menu button in the title bar
 * - User presses Alt+Space
 *
 * ## Visual Appearance
 *
 * ```
 * ┌─────────────────┐
 * │ &Restore        │  (disabled if normal)
 * │ &Move           │  (disabled - keyboard move not implemented)
 * │ &Size           │  (disabled - keyboard resize not implemented)
 * │ Mi&nimize       │  (disabled if minimized)
 * │ Ma&ximize       │  (disabled if maximized)
 * ├─────────────────┤
 * │ &Close  Alt+F4  │
 * └─────────────────┘
 * ```
 *
 * ## Usage Example
 *
 * ```cpp
 * // In window constructor
 * m_system_menu = std::make_unique<window_system_menu<Backend>>(this);
 *
 * // Wire up menu button
 * title_bar->menu_clicked.connect([this]() {
 *     m_system_menu->show_at(menu_button_position);
 * });
 * ```
 *
 * @see window.hh For window widget
 * @see menu.hh For menu container
 */

#pragma once

#include <memory>
#include <onyxui/widgets/menu/menu.hh>
#include <onyxui/widgets/menu/menu_item.hh>
#include <onyxui/actions/action.hh>

namespace onyxui {

    // Forward declaration
    template<UIBackend Backend> class window;

    /**
     * @class window_system_menu
     * @brief System menu for window controls
     *
     * @tparam Backend The UI backend type
     *
     * @details
     * Creates and manages a popup menu with standard window operations:
     * - Restore (if minimized or maximized)
     * - Move (currently disabled - keyboard-driven move not yet implemented)
     * - Size (currently disabled - keyboard-driven resize not yet implemented)
     * - Minimize
     * - Maximize
     * - Close
     *
     * Menu items are automatically enabled/disabled based on window state.
     *
     * ## Lifecycle
     *
     * 1. Create system menu (owned by window)
     * 2. Show menu at button position or center
     * 3. User selects action or dismisses menu
     * 4. Menu triggers window operation
     *
     * ## Current Implementation
     *
     * - Menu with Restore, Minimize, Maximize, Close (fully functional)
     * - Move and Size items are present but disabled (keyboard control not implemented)
     * - Menu items auto-update based on window state
     */
    template<UIBackend Backend>
    class window_system_menu {
    public:
        using action_type = std::shared_ptr<action<Backend>>;

        /**
         * @brief Construct system menu for a window
         * @param owner The window that owns this menu
         *
         * @details
         * Creates menu and wires up all actions to window methods.
         */
        explicit window_system_menu(window<Backend>* owner);

        /**
         * @brief Destructor
         */
        ~window_system_menu() = default;

        // Rule of Five
        window_system_menu(const window_system_menu&) = delete;
        window_system_menu& operator=(const window_system_menu&) = delete;
        window_system_menu(window_system_menu&&) noexcept = default;
        window_system_menu& operator=(window_system_menu&&) noexcept = default;

        /**
         * @brief Update menu item states based on window state
         *
         * @details
         * Call this before showing the menu to update:
         * - Restore: Enabled if minimized or maximized
         * - Minimize: Enabled if not minimized
         * - Maximize: Enabled if not maximized
         * - Close: Always enabled
         */
        void update_menu_states();

        /**
         * @brief Get the menu widget
         * @return Pointer to menu widget
         *
         * @details
         * Returns the underlying menu widget for positioning and display.
         * The menu is owned by this system_menu instance.
         */
        [[nodiscard]] menu<Backend>* get_menu() noexcept {
            return m_menu.get();
        }

    private:
        window<Backend>* m_owner;  // Non-owning pointer to parent window

        // Menu widget (owned)
        std::unique_ptr<menu<Backend>> m_menu;

        // Menu items (owned by menu, we keep pointers for state management)
        menu_item<Backend>* m_restore_item = nullptr;
        menu_item<Backend>* m_move_item = nullptr;      // Disabled (keyboard move not implemented)
        menu_item<Backend>* m_size_item = nullptr;      // Disabled (keyboard resize not implemented)
        menu_item<Backend>* m_minimize_item = nullptr;
        menu_item<Backend>* m_maximize_item = nullptr;
        menu_item<Backend>* m_close_item = nullptr;

        // Actions
        action_type m_restore_action;
        action_type m_move_action;      // Optional
        action_type m_size_action;      // Optional
        action_type m_minimize_action;
        action_type m_maximize_action;
        action_type m_close_action;

        // Helper: Create and wire up menu items
        void create_menu_items();
    };

} // namespace onyxui

// Implementation
#include <onyxui/widgets/window/window_system_menu.inl>
