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
#include <onyxui/widgets/vbox.hh>
#include <onyxui/widgets/menu_item.hh>
#include <onyxui/focus_manager.hh>

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
    class menu : public vbox<Backend> {
    public:
        using base = vbox<Backend>;
        using renderer_type = typename Backend::renderer_type;
        using rect_type = typename Backend::rect_type;

        /**
         * @brief Construct an empty menu
         * @param parent Parent element
         */
        explicit menu(ui_element<Backend>* parent = nullptr)
            : base(0, horizontal_alignment::left, vertical_alignment::top, parent)
            , m_focus_mgr() {
            this->set_focusable(true);
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
         * @return Pointer to separator item
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
        menu_item<Backend>* add_separator() {
            auto separator = menu_item<Backend>::make_separator(this);
            menu_item<Backend>* ptr = separator.get();
            this->add_child(std::move(separator));
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
         * Focuses the first non-separator, enabled item.
         * Called automatically when menu is opened.
         */
        void focus_first() {
            // Focus first focusable item using focus manager
            for (auto& child : this->children()) {
                if (auto* item = dynamic_cast<menu_item<Backend>*>(child.get())) {
                    if (!item->is_separator() && item->is_enabled() && item->is_focusable()) {
                        m_focus_mgr.set_focus(item);
                        return;
                    }
                }
            }
        }

        /**
         * @brief Focus next item in menu
         *
         * @details
         * Moves focus to next focusable item (wraps to start).
         * Skips separators and disabled items.
         */
        void focus_next() {
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

                if (!(*it)->is_separator() && (*it)->is_enabled() && (*it)->is_focusable()) {
                    m_focus_mgr.set_focus(*it);
                    return;
                }
            }
        }

        /**
         * @brief Focus previous item in menu
         *
         * @details
         * Moves focus to previous focusable item (wraps to end).
         * Skips separators and disabled items.
         */
        void focus_previous() {
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

                if (!(*it)->is_separator() && (*it)->is_enabled() && (*it)->is_focusable()) {
                    m_focus_mgr.set_focus(*it);
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
            if (auto* item = focused_item()) {
                if (!item->is_separator() && item->is_enabled()) {
                    // Trigger the item's action
                    if (auto action_ptr = item->get_action()) {
                        action_ptr->trigger();
                    }
                    on_item_activated();
                }
            }
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
         * @brief Handle item activation
         *
         * @details
         * Called when a menu item is activated.
         * Emits closing signal so menu can be hidden.
         */
        virtual void on_item_activated() {
            closing.emit();
        }

    private:
        focus_manager<Backend> m_focus_mgr;  ///< Focus manager for menu items
    };

} // namespace onyxui
