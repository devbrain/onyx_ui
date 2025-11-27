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
#include <onyxui/widgets/core/widget_container.hh>
#include <onyxui/widgets/menu/menu_item.hh>
#include <onyxui/widgets/menu/separator.hh>
#include <onyxui/services/ui_services.hh>
#include <onyxui/layout/linear_layout.hh>
#include <onyxui/core/rendering/resolved_style.hh>

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
        using theme_type = typename base::theme_type;

        /**
         * @brief Construct an empty menu
         * @param parent Parent element
         */
        explicit menu(ui_element<Backend>* parent = nullptr)
            : base(
                std::make_unique<linear_layout<Backend>>(
                    direction::vertical,
                    0,  // No spacing between menu items
                    horizontal_alignment::left,
                    vertical_alignment::top
                ),
                parent
              )
        {
            this->m_has_border = true;  // Menus always have borders
            this->set_focusable(true);

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
            // IMPORTANT: Only close menu if item doesn't have a submenu
            // Submenu items should keep the menu open for navigation
            item->clicked.connect([this, ptr]() {
                if (!ptr->has_submenu()) {
                    this->on_item_activated();
                }
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
         * @brief Reset all menu item interaction states to normal
         *
         * @details
         * Resets all menu items to normal state (clears hover/pressed).
         * Called when menu is opened to prevent stale highlighting
         * from previous open/close cycles.
         */
        void reset_item_states() {
            for (auto& child : this->children()) {
                if (auto* item = dynamic_cast<menu_item<Backend>*>(child.get())) {
                    item->reset_state();
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
         * @brief Menu does NOT inherit colors from parent
         * @return false - menu has its own distinct background and border colors
         *
         * @details
         * Menus must use their own theme colors (white in NU8) and not inherit
         * the parent window's background color. This ensures the menu popup
         * has the correct background and border styling.
         */
        [[nodiscard]] bool should_inherit_colors() const override {
            return false;  // Menu has distinct background (white in NU8)
        }

        /**
         * @brief Get complete widget style from theme
         * @param theme Theme to extract properties from
         * @return Resolved style with menu-specific theme values
         *
         * @details
         * Uses menu.background for the dropdown background and menu.border_color
         * for the box drawing border (black in NU8 theme).
         * Separators get their color directly from theme.separator.foreground.
         */
        [[nodiscard]] resolved_style<Backend> get_theme_style(const theme_type& theme) const override {
            return resolved_style<Backend>{
                .background_color = theme.menu.background,
                .foreground_color = theme.menu.border_color,  // Used by box border
                .mnemonic_foreground = theme.text_fg,  // Mnemonics same as text (menu container has no mnemonics)
                .border_color = theme.menu.border_color,
                .box_style = theme.menu.box_style,
                .font = theme.label.font,
                .opacity = 1.0f,
                .icon_style = std::optional<typename Backend::renderer_type::icon_style>{},
                .padding_horizontal = std::optional<int>{},
                .padding_vertical = std::optional<int>{},
                .mnemonic_font = std::optional<typename Backend::renderer_type::font>{},
                .submenu_icon = std::optional<typename Backend::renderer_type::icon_style>{}
            };
        }

        /**
         * @brief Render menu with shadow effect
         * @param ctx Render context
         *
         * @details
         * Draws menu using base implementation, then adds drop shadow
         * if enabled in theme.
         */
        void do_render(render_context<Backend>& ctx) const override {
            // Draw menu normally
            base::do_render(ctx);

            // Draw shadow if enabled in theme
            if (auto* theme = ctx.theme()) {
                if (theme->menu.shadow.enabled) {
                    // RELATIVE COORDINATES: Reconstruct absolute bounds from context position
                    // bounds() returns logical_rect, need to convert to physical for rendering
                    typename Backend::rect_type absolute_bounds;
                    auto logical_bounds = this->bounds();
                    rect_utils::make_absolute_bounds(absolute_bounds, ctx.position(),
                        typename Backend::rect_type{logical_bounds.x.to_int(), logical_bounds.y.to_int(),
                                                    logical_bounds.width.to_int(), logical_bounds.height.to_int()});

                    ctx.draw_shadow(
                        absolute_bounds,
                        theme->menu.shadow.offset_x,
                        theme->menu.shadow.offset_y
                    );
                }
            }
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
