/**
 * @file menu.hh
 * @brief Vertical menu container with keyboard navigation
 * @author igor
 * @date 16/10/2025
 *
 * @details
 * A menu is a vertical list of menu_item widgets that supports:
 * - Keyboard navigation (Up/Down arrows)
 * - Mnemonic activation (Alt+key)
 * - Enter to activate focused item
 * - Escape to close menu
 * - Mouse hover and click
 *
 * ## Visual Appearance
 *
 * ```
 * ┌────────────────────┐
 * │ &New        Ctrl+N │
 * │ &Open       Ctrl+O │
 * │ &Save       Ctrl+S │
 * ├────────────────────┤  ← Separator
 * │ E&xit      Alt+F4  │
 * └────────────────────┘
 * ```
 *
 * ## Keyboard Navigation
 *
 * - **Up/Down**: Move focus between items
 * - **Enter**: Activate focused item
 * - **Escape**: Close menu
 * - **Alt+mnemonic**: Activate item with matching mnemonic
 * - **First letter**: Jump to item starting with that letter
 *
 * ## Usage Example
 *
 * ```cpp
 * auto menu = std::make_unique<menu<Backend>>();
 *
 * auto new_item = std::make_unique<menu_item<Backend>>();
 * new_item->set_mnemonic_text("&New");
 * new_item->set_action(new_action);
 * menu->add_item(std::move(new_item));
 *
 * menu->add_separator();
 *
 * auto exit_item = std::make_unique<menu_item<Backend>>();
 * exit_item->set_mnemonic_text("E&xit");
 * exit_item->set_action(exit_action);
 * menu->add_item(std::move(exit_item));
 * ```
 *
 * @see menu_item.hh For individual menu items
 * @see menu_bar.hh For horizontal menu bar
 */

#pragma once

#include <vector>
#include <memory>
#include <iostream>  // For debug output
#include <onyxui/widgets/widget_container.hh>
#include <onyxui/widgets/menu_item.hh>
#include <onyxui/widgets/separator.hh>
#include <onyxui/ui_services.hh>
#include <onyxui/focus_manager.hh>
#include <onyxui/layout/linear_layout.hh>

namespace onyxui {

    /**
     * @class menu
     * @brief Vertical list of menu items with keyboard navigation
     *
     * @tparam Backend The UI backend type
     *
     * @details
     * A menu uses vertical box layout to stack menu items.
     * It provides keyboard navigation and mnemonic-based activation.
     *
     * ## Lifecycle
     *
     * Menus are typically shown as popups/overlays:
     * 1. Create menu
     * 2. Add items
     * 3. Show menu at position
     * 4. Handle activation or close
     * 5. Destroy menu
     *
     * ## Focus Management
     *
     * When opened, menu automatically focuses first non-separator item.
     * Arrow keys navigate between focusable items (skipping separators).
     */
    template<UIBackend Backend>
    class menu : public widget_container<Backend> {
    public:
        using base = widget_container<Backend>;
        using renderer_type = typename Backend::renderer_type;
        using rect_type = typename Backend::rect_type;
        using size_type = typename Backend::size_type;

        /**
         * @brief Construct an empty menu
         * @param parent Parent element
         */
        explicit menu(ui_element<Backend>* parent = nullptr)
            : base(parent) {
            this->m_has_border = true;  // Menus always have borders
            this->set_focusable(true);
            // Use vertical layout (like vbox)
            this->set_layout_strategy(
                std::make_unique<linear_layout<Backend>>(
                    direction::vertical, 0,
                    horizontal_alignment::left,
                    vertical_alignment::top));

            // NOTE: Semantic actions are registered when menu opens, not in constructor
            // This prevents multiple menus from conflicting with each other
        }

        /**
         * @brief Destructor
         */
        ~menu() override = default;

        // Rule of Five
        menu(const menu&) = delete;
        menu& operator=(const menu&) = delete;
        menu(menu&&) noexcept = default;
        menu& operator=(menu&&) noexcept = default;

        /**
         * @brief Add a menu item
         * @param item The menu item to add
         * @return Pointer to added item (for chaining)
         *
         * @details
         * Takes ownership of the menu item and adds it to the menu.
         *
         * @example
         * @code
         * auto item = std::make_unique<menu_item<Backend>>();
         * item->set_mnemonic_text("&Save");
         * menu->add_item(std::move(item));
         * @endcode
         */
        menu_item<Backend>* add_item(std::unique_ptr<menu_item<Backend>> item) {
            menu_item<Backend>* ptr = item.get();

            // Connect item activation to menu close
            item->clicked.connect([this]() {
                this->on_item_activated();
            });

            this->add_child(std::move(item));
            return ptr;
        }

        /**
         * @brief Add a separator line
         * @return Pointer to separator widget
         *
         * @details
         * Convenience method to add a horizontal divider.
         *
         * @example
         * @code
         * menu->add_item(new_item);
         * menu->add_separator();
         * menu->add_item(exit_item);
         * @endcode
         */
        separator<Backend>* add_separator() {
            auto sep = std::make_unique<separator<Backend>>(orientation::horizontal, this);
            separator<Backend>* ptr = sep.get();
            this->add_child(std::move(sep));
            return ptr;
        }

        /**
         * @brief Find menu item by mnemonic character
         * @param mnemonic_char Character to search for (lowercase)
         * @return Pointer to item, or nullptr if not found
         *
         * @details
         * Searches for a menu item with the given mnemonic character.
         * Used for Alt+key navigation.
         *
         * @example
         * @code
         * // Find item with Alt+F
         * if (auto* item = menu->find_by_mnemonic('f')) {
         *     item->trigger();
         * }
         * @endcode
         */
        [[nodiscard]] menu_item<Backend>* find_by_mnemonic(char mnemonic_char) {
            for (auto& child : this->children()) {
                if (auto* item = dynamic_cast<menu_item<Backend>*>(child.get())) {
                    if (item->get_mnemonic_char() == mnemonic_char) {
                        return item;
                    }
                }
            }
            return nullptr;
        }

        /**
         * @brief Get all menu items (excluding non-items)
         * @return Vector of menu item pointers
         */
        [[nodiscard]] std::vector<menu_item<Backend>*> items() const {
            std::vector<menu_item<Backend>*> result;
            for (auto& child : this->children()) {
                if (auto* item = dynamic_cast<menu_item<Backend>*>(child.get())) {
                    result.push_back(item);
                }
            }
            return result;
        }

        /**
         * @brief Get focused menu item
         * @return Pointer to focused item, or nullptr
         */
        [[nodiscard]] menu_item<Backend>* focused_item() const {
            for (auto& child : this->children()) {
                if (auto* item = dynamic_cast<menu_item<Backend>*>(child.get())) {
                    if (item->has_focus()) {
                        return item;
                    }
                }
            }
            return nullptr;
        }

        /**
         * @brief Focus first focusable item
         *
         * @details
         * Focuses the first enabled, focusable child.
         * Called automatically when menu is opened.
         */
        void focus_first() {
            // Focus first focusable item using global focus manager
            auto* focus_mgr = ui_services<Backend>::input();
            if (!focus_mgr) return;

            for (auto& child : this->children()) {
                if (child->is_enabled() && child->is_focusable()) {
                    focus_mgr->set_focus(child.get());
                    return;
                }
            }
        }

        /**
         * @brief Focus next item in menu
         *
         * @details
         * Moves focus to next focusable item (wraps to start).
         * Skips non-focusable children (like separators) and disabled items.
         */
        void focus_next() {
            auto* focus_mgr = ui_services<Backend>::input();
            if (!focus_mgr) return;

            auto menu_items = items();
            if (menu_items.empty()) return;

            auto* current = focused_item();
            auto it = std::find(menu_items.begin(), menu_items.end(), current);

            // Find next focusable item
            for (std::size_t i = 0; i < menu_items.size(); ++i) {
                if (it != menu_items.end()) {
                    ++it;
                    if (it == menu_items.end()) {
                        it = menu_items.begin();  // Wrap around
                    }
                } else {
                    it = menu_items.begin();
                }

                if ((*it)->is_enabled() && (*it)->is_focusable()) {
                    focus_mgr->set_focus(*it);
                    return;
                }
            }
        }

        /**
         * @brief Focus previous item in menu
         *
         * @details
         * Moves focus to previous focusable item (wraps to end).
         * Skips non-focusable children (like separators) and disabled items.
         */
        void focus_previous() {
            auto* focus_mgr = ui_services<Backend>::input();
            if (!focus_mgr) return;

            auto menu_items = items();
            if (menu_items.empty()) return;

            auto* current = focused_item();
            auto it = std::find(menu_items.begin(), menu_items.end(), current);

            // Find previous focusable item
            for (std::size_t i = 0; i < menu_items.size(); ++i) {
                if (it != menu_items.end()) {
                    if (it == menu_items.begin()) {
                        it = menu_items.end();  // Wrap around
                    }
                    --it;
                } else {
                    it = std::prev(menu_items.end());
                }

                if ((*it)->is_enabled() && (*it)->is_focusable()) {
                    focus_mgr->set_focus(*it);
                    return;
                }
            }
        }

        /**
         * @brief Activate currently focused item
         *
         * @details
         * Triggers the action of the focused menu item.
         * Called when Enter is pressed.
         */
        void activate_focused() {
            auto* item = focused_item();
            if (item && item->is_enabled()) {
                // Trigger the item's action
                if (auto action_ptr = item->get_action()) {
                    action_ptr->trigger();
                }
                on_item_activated();
            }
        }

        /**
         * @brief Register keyboard navigation hotkeys
         *
         * @details
         * Registers semantic actions for menu keyboard navigation:
         * - menu_down: Move to next item
         * - menu_up: Move to previous item
         * - menu_select: Activate focused item
         * - menu_cancel: Close menu
         *
         * Should be called when the menu is opened/shown.
         * Must be paired with unregister_navigation_hotkeys() when menu closes.
         */
        void register_navigation_hotkeys() {
            auto* hotkeys = ui_services<Backend>::hotkeys();
            if (!hotkeys) return;

            // Down arrow → focus next item
            hotkeys->register_semantic_action(
                hotkey_action::menu_down,
                [this]() { this->focus_next(); }
            );

            // Up arrow → focus previous item
            hotkeys->register_semantic_action(
                hotkey_action::menu_up,
                [this]() { this->focus_previous(); }
            );

            // Enter → activate focused item
            hotkeys->register_semantic_action(
                hotkey_action::menu_select,
                [this]() { this->activate_focused(); }
            );

            // Escape → close menu
            hotkeys->register_semantic_action(
                hotkey_action::menu_cancel,
                [this]() { this->closing.emit(); }
            );
        }

        /**
         * @brief Unregister keyboard navigation hotkeys
         *
         * @details
         * Unregisters semantic actions for menu keyboard navigation.
         * Should be called when the menu is closed/hidden.
         * Must be paired with register_navigation_hotkeys().
         */
        void unregister_navigation_hotkeys() {
            auto* hotkeys = ui_services<Backend>::hotkeys();
            if (!hotkeys) return;

            hotkeys->unregister_semantic_action(hotkey_action::menu_down);
            hotkeys->unregister_semantic_action(hotkey_action::menu_up);
            hotkeys->unregister_semantic_action(hotkey_action::menu_select);
            hotkeys->unregister_semantic_action(hotkey_action::menu_cancel);
        }

        /**
         * @brief Signal emitted when menu should close
         *
         * @details
         * Emitted when:
         * - An item is activated
         * - Escape is pressed
         * - Click outside menu
         *
         * Owner of menu should listen to this signal and hide/destroy menu.
         */
        signal<> closing;

    protected:
        /**
         * @brief Override click to forward to children
         */
        bool handle_click(int x, int y) override {
            // Forward click to children
            for (auto& child : this->children()) {
                if (!child || !child->is_visible()) continue;

                // Check if click is inside child bounds
                auto child_bounds = child->bounds();
                if (rect_utils::contains(child_bounds, x, y)) {
                    // Trigger the child's click handler directly
                    if (auto* item = dynamic_cast<menu_item<Backend>*>(child.get())) {
                        // Emit the clicked signal which will trigger the action
                        item->clicked.emit();
                        return true;
                    }
                }
            }

            // Don't call base implementation - we don't want the menu itself to handle clicks
            return false;
        }

        /**
         * @brief Handle item activation
         *
         * @details
         * Called when a menu item is activated.
         * Emits closing signal so menu can be hidden.
         */
        virtual void on_item_activated() {
            closing.emit();
        }
    };

} // namespace onyxui
