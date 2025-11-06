/**
 * @file menu_system.hh
 * @brief Centralized coordinator for menu navigation and state (Phase 4)
 * @author Assistant
 * @date 2025-10-28
 *
 * @details
 * Provides a composite menu system that:
 * - Manages menu hierarchy stack (for cascading submenus)
 * - Implements context-dependent navigation (left/right arrows)
 * - Centralizes hotkey registration (registered once, not per menu switch)
 * - Coordinates between menu_bar and individual menus
 *
 * ## Architecture
 *
 * Before Phase 4 (Phase 2 approach):
 * - menu_bar re-registers handlers every time menu switches
 * - Left/Right always switch menu bar items
 * - No submenu support
 *
 * After Phase 4:
 * - menu_system registers handlers ONCE in constructor
 * - Handlers check menu depth dynamically
 * - Left/Right behavior context-dependent:
 *   - Top-level: Switch menu bar items
 *   - Submenu: Close submenu (left) or open child submenu (right)
 * - Menu stack enables arbitrary nesting
 *
 * ## Usage Example
 *
 * ```cpp
 * class menu_bar {
 *     menu_system<Backend> m_menu_system{this};
 *
 *     void open_menu(size_t index) {
 *         m_menu_system.open_top_level_menu(menu_ptr);
 *         // Hotkeys already registered!
 *     }
 * };
 * ```
 *
 * @see menu_bar.hh For integration
 * @see menu.hh For menu implementation
 */

#pragma once

#include <onyxui/concepts/backend.hh>
#include <onyxui/hotkeys/semantic_action_guard.hh>
#include <onyxui/services/ui_services.hh>
#include <onyxui/core/raii/scoped_layer.hh>
#include <onyxui/core/signal.hh>  // For scoped_connection
#include <vector>

namespace onyxui {

    // Forward declarations
    template<UIBackend Backend> class menu;
    template<UIBackend Backend> class menu_bar;
    template<UIBackend Backend> class menu_item;

    /**
     * @class menu_system
     * @brief Centralized coordinator for hierarchical menu navigation
     *
     * @tparam Backend UI backend type
     *
     * @details
     * Manages menu hierarchy as a stack:
     * - Stack depth 1: Top-level menu (File, Edit, etc.)
     * - Stack depth 2+: Submenus (File → Recent, File → Recent → Project1)
     *
     * Navigation is context-dependent based on stack depth:
     * - **Up/Down**: Always navigate within current menu
     * - **Left**: Submenu → close submenu | Top-level → previous menu bar item
     * - **Right**: On submenu item → open | Top-level → next menu bar item
     * - **Enter**: Activate focused item
     * - **Escape**: Submenu → close submenu | Top-level → close entire menu
     *
     * ## Ownership
     *
     * - menu_system does NOT own menus (non-owning pointers)
     * - menu_bar owns the menus and menu_system instance
     * - menu_system owns semantic_action_guards (RAII cleanup)
     *
     * ## Thread Safety
     *
     * Not thread-safe. All operations must occur on UI thread.
     */
    template<UIBackend Backend>
    class menu_system {
    public:
        using menu_type = menu<Backend>;
        using menu_bar_type = menu_bar<Backend>;

        /**
         * @brief Construct menu system
         * @param menu_bar Parent menu bar (non-owning pointer)
         *
         * @details
         * Navigation hotkeys are registered on-demand when a menu opens.
         * This prevents ESC from being consumed when no menu is open.
         */
        explicit menu_system(menu_bar_type* menu_bar)
            : m_menu_bar(menu_bar) {
            // Don't register hotkeys here - they'll be registered when menu opens
        }

        /**
         * @brief Destructor
         *
         * @details
         * RAII guards automatically unregister hotkeys.
         */
        ~menu_system() = default;

        // Non-copyable, non-movable (contains non-owning pointer to parent)
        menu_system(const menu_system&) = delete;
        menu_system& operator=(const menu_system&) = delete;
        menu_system(menu_system&&) = delete;
        menu_system& operator=(menu_system&&) = delete;

        /**
         * @brief Open top-level menu
         * @param menu Menu to open (non-owning pointer)
         *
         * @details
         * Resets menu stack to single menu.
         * Called by menu_bar when switching top-level menus.
         *
         * Phase 5: Clears any open submenus when switching top-level menus
         * Registers navigation hotkeys on first menu open.
         */
        void open_top_level_menu(menu_type* menu) {
            m_submenu_layers.clear();  // Phase 5: Close all submenus when switching
            m_menu_stack = {menu};  // Reset to single menu

            // Register navigation hotkeys if not already registered
            // This ensures ESC and other keys are only active when a menu is open
            if (m_nav_guards.empty()) {
                register_navigation_hotkeys();
            }

            // Connect mouse click handlers for submenu items
            connect_submenu_click_handlers(menu);
        }

        /**
         * @brief Open submenu (push onto stack)
         * @param submenu Submenu to open
         *
         * @details
         * Adds submenu to top of stack and shows it as a popup layer.
         * Navigation handlers will target this submenu until closed.
         *
         * Phase 5: Visual rendering with layer management
         */
        void open_submenu(menu_type* submenu) {
            if (!submenu) return;

            // Get parent menu (current top of stack before pushing)
            auto* parent_menu = current_menu();
            if (!parent_menu) return;

            // Get focused item in parent menu (anchor for submenu position)
            auto* focused_item = parent_menu->focused_item();
            if (!focused_item) return;

            // Push submenu onto stack FIRST
            m_menu_stack.push_back(submenu);

            // Show submenu as popup to the right of focused item (Phase 5)
            auto submenu_layer = focused_item->show_popup_scoped(
                submenu,
                popup_placement::right  // Show to the right of parent item
            );

            // Store scoped_layer (RAII auto-cleanup on pop)
            m_submenu_layers.push_back(std::move(submenu_layer));

            // Connect mouse click handlers for submenu items
            connect_submenu_click_handlers(submenu);

            // Set focus to submenu and its first item
            if (auto* focus = ui_services<Backend>::input()) {
                focus->set_focus(submenu);
                if (submenu) {
                    submenu->focus_first();
                }
            }
        }

        /**
         * @brief Close current submenu (pop from stack)
         * @return True if submenu was closed, false if at top level
         *
         * @details
         * Removes top menu from stack if depth > 1.
         * If at top level, returns false and does nothing.
         *
         * Phase 5: Removes visual layer and restores focus to parent
         */
        bool close_current_submenu() {
            if (m_menu_stack.size() <= 1) {
                return false;  // At top-level, can't close
            }

            // Pop submenu layer (RAII auto-removes from layer_manager) (Phase 5)
            if (!m_submenu_layers.empty()) {
                m_submenu_layers.pop_back();
            }

            // Pop from menu stack
            m_menu_stack.pop_back();

            // Restore focus to parent menu (Phase 5)
            if (auto* parent_menu = current_menu()) {
                if (auto* focus = ui_services<Backend>::input()) {
                    focus->set_focus(parent_menu);
                }
            }

            return true;
        }

        /**
         * @brief Close all menus (clear stack)
         *
         * @details
         * Removes all menus from stack.
         * Used when closing entire menu bar.
         *
         * Phase 5: Clears all submenu layers (RAII auto-cleanup)
         * Unregisters navigation hotkeys to allow ESC to propagate to app.
         */
        void close_all_menus() {
            m_submenu_layers.clear();  // Phase 5: RAII removes all submenu layers
            m_menu_stack.clear();

            // Clear submenu click connections
            m_submenu_click_connections.clear();

            // Unregister navigation hotkeys when no menu is open
            // This allows ESC and other keys to propagate to application
            m_nav_guards.clear();
        }

        /**
         * @brief Get currently active menu (top of stack)
         * @return Pointer to current menu, or nullptr if stack is empty
         */
        [[nodiscard]] menu_type* current_menu() const noexcept {
            return m_menu_stack.empty() ? nullptr : m_menu_stack.back();
        }

        /**
         * @brief Get menu hierarchy depth
         * @return 1 = top-level menu, 2+ = submenus
         */
        [[nodiscard]] std::size_t menu_depth() const noexcept {
            return m_menu_stack.size();
        }

        /**
         * @brief Check if at top-level menu
         * @return true if depth == 1, false otherwise
         */
        [[nodiscard]] bool is_top_level() const noexcept {
            return m_menu_stack.size() == 1;
        }

        /**
         * @brief Check if menu system has active menu
         * @return true if stack is not empty
         */
        [[nodiscard]] bool has_active_menu() const noexcept {
            return !m_menu_stack.empty();
        }

    private:
        /**
         * @brief Register context-dependent navigation hotkeys
         *
         * @details
         * Registers handlers ONCE for all semantic actions.
         * Handlers check menu depth dynamically at runtime.
         * RAII guards ensure automatic cleanup on destruction.
         */
        void register_navigation_hotkeys() {
            auto* hotkeys = ui_services<Backend>::hotkeys();
            if (!hotkeys) return;

            m_nav_guards.clear();

            // Register context-dependent handlers
            m_nav_guards.emplace_back(hotkeys, hotkey_action::menu_down,
                [this]() { this->handle_menu_down(); });
            m_nav_guards.emplace_back(hotkeys, hotkey_action::menu_up,
                [this]() { this->handle_menu_up(); });
            m_nav_guards.emplace_back(hotkeys, hotkey_action::menu_left,
                [this]() { this->handle_menu_left(); });
            m_nav_guards.emplace_back(hotkeys, hotkey_action::menu_right,
                [this]() { this->handle_menu_right(); });
            m_nav_guards.emplace_back(hotkeys, hotkey_action::menu_select,
                [this]() { this->handle_menu_select(); });
            m_nav_guards.emplace_back(hotkeys, hotkey_action::menu_cancel,
                [this]() { this->handle_menu_cancel(); });
        }

        /**
         * @brief Handle menu_down action
         * @details Navigates to next item in current menu
         */
        void handle_menu_down() {
            if (auto* menu = current_menu()) {
                menu->focus_next();
            }
        }

        /**
         * @brief Handle menu_up action
         * @details Navigates to previous item in current menu
         */
        void handle_menu_up() {
            if (auto* menu = current_menu()) {
                menu->focus_previous();
            }
        }

        /**
         * @brief Handle menu_left action (context-dependent)
         *
         * @details
         * Behavior depends on menu depth:
         * - **Top-level menu**: Switch to previous menu bar item
         * - **Submenu**: Close submenu, return to parent
         */
        void handle_menu_left() {
            if (is_top_level()) {
                // Top-level menu - switch to previous menu bar item
                if (m_menu_bar) {
                    m_menu_bar->navigate_previous();
                }
            } else {
                // Submenu - close and return to parent
                close_current_submenu();
            }
        }

        /**
         * @brief Handle menu_right action (context-dependent)
         *
         * @details
         * Behavior depends on focused item and menu depth:
         * - **Focused item has submenu**: Open the submenu
         * - **Top-level menu, regular item**: Switch to next menu bar item
         * - **Submenu, regular item**: Do nothing
         */
        void handle_menu_right() {
            auto* menu = current_menu();
            if (!menu) return;

            auto* focused_item = menu->focused_item();

            if (focused_item && focused_item->has_submenu()) {
                // Open submenu (Phase 5 will add visual display)
                open_submenu(focused_item->submenu());

                // Focus first item in submenu
                if (auto* submenu = focused_item->submenu()) {
                    submenu->focus_first();
                }
            } else if (is_top_level()) {
                // Top-level menu on regular item - switch to next menu bar item
                if (m_menu_bar) {
                    m_menu_bar->navigate_next();
                }
            }
            // Else: In submenu on regular item - do nothing
        }

        /**
         * @brief Handle menu_select action (context-dependent)
         *
         * @details
         * Behavior depends on focused item:
         * - **Item has submenu**: Open the submenu
         * - **Regular item**: Activate the item (trigger action)
         */
        void handle_menu_select() {
            auto* menu = current_menu();
            if (!menu) return;

            auto* focused_item = menu->focused_item();
            if (!focused_item) return;

            if (focused_item->has_submenu()) {
                // Enter on submenu item - open submenu (like Right arrow)
                open_submenu(focused_item->submenu());

                // Focus first item in submenu
                if (auto* submenu = focused_item->submenu()) {
                    submenu->focus_first();
                }
            } else {
                // Enter on regular item - activate it
                menu->activate_focused();
            }
        }

        /**
         * @brief Handle menu_cancel action (context-dependent)
         *
         * @details
         * Behavior depends on menu depth:
         * - **Submenu**: Close current submenu, return to parent
         * - **Top-level**: Close entire menu bar
         */
        void handle_menu_cancel() {
            if (!close_current_submenu()) {
                // At top-level - close entire menu bar
                close_all_menus();
                if (m_menu_bar) {
                    m_menu_bar->close_menu();
                }
            }
        }

        /**
         * @brief Connect mouse click handlers for submenu items in a menu
         * @param menu Menu to process
         *
         * @details
         * Iterates through all items in the menu and connects their clicked
         * signal to open_submenu() if they have a submenu.
         * This enables mouse-driven submenu navigation.
         */
        void connect_submenu_click_handlers(menu_type* menu) {
            if (!menu) return;

            // Iterate through all children to find menu items
            for (auto& child : menu->children()) {
                // Try to cast to menu_item
                if (auto* item = dynamic_cast<menu_item<Backend>*>(child.get())) {
                    if (item->has_submenu()) {
                        // Connect clicked signal to open submenu
                        m_submenu_click_connections.emplace_back(
                            item->clicked,
                            [this, item]() {
                                this->open_submenu(item->submenu());
                                // Focus first item in opened submenu
                                if (auto* submenu = item->submenu()) {
                                    submenu->focus_first();
                                }
                            }
                        );
                    }
                }
            }
        }

        menu_bar_type* m_menu_bar = nullptr;       ///< Non-owning pointer to parent menu bar
        std::vector<menu_type*> m_menu_stack;      ///< Menu hierarchy (back = current)
        std::vector<semantic_action_guard<Backend>> m_nav_guards;  ///< RAII hotkey guards
        std::vector<scoped_layer<Backend>> m_submenu_layers;  ///< Submenu popup layers (Phase 5)
        std::vector<scoped_connection> m_submenu_click_connections;  ///< Mouse click connections for submenu items
    };

} // namespace onyxui
