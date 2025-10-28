/**
 * @file menu_bar.hh
 * @brief Horizontal menu bar with dropdown menus
 * @author igor
 * @date 16/10/2025
 *
 * @details
 * A menu bar displays top-level menu titles horizontally and shows
 * dropdown menus when clicked or activated via mnemonic.
 *
 * ## Visual Appearance
 *
 * ```
 * &File  &Edit  &View  &Help
 * ```
 *
 * When activated:
 * ```
 * &File  &Edit  &View  &Help
 * ┌────────────────────┐
 * │ &New        Ctrl+N │
 * │ &Open       Ctrl+O │
 * │ &Save       Ctrl+S │
 * ├────────────────────┤
 * │ E&xit      Alt+F4  │
 * └────────────────────┘
 * ```
 *
 * ## Keyboard Navigation
 *
 * - **Alt**: Activate menu bar (focus first menu)
 * - **Alt+mnemonic**: Open specific menu (e.g., Alt+F for File)
 * - **Left/Right**: Navigate between menus
 * - **Escape**: Close menu bar
 * - **Down**: Open focused menu
 *
 * ## Usage Example
 *
 * ```cpp
 * auto menu_bar = std::make_unique<menu_bar<Backend>>();
 *
 * // File menu
 * auto file_menu = std::make_unique<menu<Backend>>();
 * auto new_item = std::make_unique<menu_item<Backend>>();
 * new_item->set_mnemonic_text("&New");
 * new_item->set_action(new_action);
 * file_menu->add_item(std::move(new_item));
 *
 * menu_bar->add_menu("&File", std::move(file_menu));
 *
 * // Edit menu
 * auto edit_menu = std::make_unique<menu<Backend>>();
 * menu_bar->add_menu("&Edit", std::move(edit_menu));
 * ```
 *
 * @see menu.hh For dropdown menus
 * @see menu_item.hh For menu items
 */

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <optional>
#include <onyxui/widgets/hbox.hh>
#include <onyxui/widgets/menu.hh>
#include <onyxui/widgets/menu_bar_item.hh>
#include <onyxui/widgets/mnemonic_parser.hh>
#include <onyxui/layer_manager.hh>
#include <onyxui/scoped_layer.hh>
#include <onyxui/ui_services.hh>
#include <onyxui/focus_manager.hh>
#include <onyxui/hotkeys/hotkey_action.hh>
#include <onyxui/hotkeys/hotkey_manager.hh>
#include <onyxui/hotkeys/semantic_action_guard.hh>

namespace onyxui {

    // Forward declaration
    template<UIBackend Backend>
    class ui_handle;

    /**
     * @struct menu_entry
     * @brief Associates a menu title item with its dropdown menu
     */
    template<UIBackend Backend>
    struct menu_entry {
        menu_bar_item<Backend>* title_item;           ///< Menu bar item for menu title
        std::unique_ptr<menu<Backend>> dropdown_menu; ///< Owned dropdown menu (not a child - shown as popup)
        char mnemonic_char;                           ///< Mnemonic character for Alt+key
    };

    /**
     * @class menu_bar
     * @brief Horizontal bar of menus with dropdown functionality
     *
     * @tparam Backend The UI backend type
     *
     * @details
     * Menu bar manages top-level menu titles and their associated dropdowns.
     * It handles:
     * - Horizontal layout of menu titles
     * - Opening/closing dropdown menus
     * - Keyboard navigation between menus
     * - Mnemonic activation (Alt+F, Alt+E, etc.)
     *
     * ## Architecture
     *
     * ```
     * menu_bar (hbox)
     * ├── [File button] → file_menu (popup)
     * ├── [Edit button] → edit_menu (popup)
     * └── [View button] → view_menu (popup)
     * ```
     *
     * Dropdown menus are not children - they're shown as overlays/popups.
     */
    template<UIBackend Backend>
    class menu_bar : public hbox<Backend> {
    public:
        using base = hbox<Backend>;
        using renderer_type = typename Backend::renderer_type;
        using rect_type = typename Backend::rect_type;

        /**
         * @brief Construct an empty menu bar
         * @param parent Parent element
         */
        explicit menu_bar(ui_element<Backend>* parent = nullptr)
            : base(1, horizontal_alignment::left, vertical_alignment::top, parent)  // 1px spacing between menu items
            , m_open_menu_index(std::nullopt) {
            this->set_focusable(true);
            initialize_hotkeys();
        }

        /**
         * @brief Destructor
         */
        ~menu_bar() override = default;

        // Rule of Five
        menu_bar(const menu_bar&) = delete;
        menu_bar& operator=(const menu_bar&) = delete;
        menu_bar(menu_bar&&) noexcept = default;
        menu_bar& operator=(menu_bar&&) noexcept = default;


        /**
         * @brief Add a menu to the menu bar
         * @param title Menu title with optional mnemonic (e.g., "&File")
         * @param menu_ptr The dropdown menu
         * @return Index of added menu
         *
         * @details
         * Creates a button for the title and associates it with the menu.
         * Parses mnemonic from title for Alt+key activation.
         *
         * @example
         * @code
         * auto file_menu = std::make_unique<menu<Backend>>();
         * // ... add items to file_menu
         * menu_bar->add_menu("&File", std::move(file_menu));
         * @endcode
         */
        std::size_t add_menu(std::string_view title, std::unique_ptr<menu<Backend>> menu_ptr) {
            // Create menu_bar_item for menu title (NOT a button!)
            auto title_item = std::make_unique<menu_bar_item<Backend>>("", this);

            // Parse mnemonic from title
            mnemonic_info<Backend> const mnemonic = parse_mnemonic<Backend>(
                title,
                typename Backend::renderer_type::font{},  // Would use theme fonts
                typename Backend::renderer_type::font{}
            );

            title_item->set_text(strip_mnemonic(title));

            // Store item pointer before moving
            menu_bar_item<Backend>* item_ptr = title_item.get();

            // Add item to menu bar
            this->add_child(std::move(title_item));

            // Store menu entry with owned dropdown menu
            std::size_t const index = m_menus.size();

            menu_entry<Backend> entry{
                item_ptr,
                std::move(menu_ptr),  // Transfer ownership into entry
                mnemonic.mnemonic_char
            };

            // Connect item click to open menu
            item_ptr->clicked.connect([this, index]() {
                this->open_menu(index);
            });

            // Connect menu closing to cleanup
            entry.dropdown_menu->closing.connect([this]() {
                this->close_menu();
            });

            m_menus.push_back(std::move(entry));

            return index;
        }

        /**
         * @brief Find menu by mnemonic character
         * @param mnemonic_char Character to search for (lowercase)
         * @return Index of menu, or nullopt if not found
         *
         * @details
         * Used for Alt+key activation of menus.
         *
         * @example
         * @code
         * if (auto index = menu_bar->find_by_mnemonic('f')) {
         *     menu_bar->open_menu(*index);  // Open File menu
         * }
         * @endcode
         */
        [[nodiscard]] std::optional<std::size_t> find_by_mnemonic(char mnemonic_char) const {
            for (std::size_t i = 0; i < m_menus.size(); ++i) {
                if (m_menus[i].mnemonic_char == mnemonic_char) {
                    return i;
                }
            }
            return std::nullopt;
        }

        /**
         * @brief Open a menu by index
         * @param index Index of menu to open
         *
         * @details
         * Closes currently open menu (if any) and opens the specified menu.
         * Shows dropdown menu as popup below the title button using layer manager.
         */
        void open_menu(std::size_t index);

        /**
         * @brief Close currently open menu
         */
        void close_menu();

        /**
         * @brief Get currently open menu index
         * @return Index of open menu, or nullopt if none
         */
        [[nodiscard]] std::optional<std::size_t> open_menu_index() const noexcept {
            return m_open_menu_index;
        }

        /**
         * @brief Check if a menu is open
         */
        [[nodiscard]] bool has_open_menu() const noexcept {
            return m_open_menu_index.has_value();
        }

        /**
         * @brief Get menu by index
         * @param index Menu index
         * @return Pointer to menu, or nullptr if invalid index
         */
        [[nodiscard]] menu<Backend>* get_menu(std::size_t index) {
            if (index < m_menus.size()) {
                return m_menus[index].dropdown_menu.get();
            }
            return nullptr;
        }

        /**
         * @brief Get menu count
         */
        [[nodiscard]] std::size_t menu_count() const noexcept {
            return m_menus.size();
        }

        /**
         * @brief Get the bounds of a menu button
         * @param index Menu index
         * @return Button bounds, or default rect if invalid index
         *
         * @details
         * Returns the actual rendered bounds of the menu title button.
         * Used for positioning dropdown menus below their buttons.
         */
        [[nodiscard]] rect_type get_menu_button_bounds(std::size_t index) const {
            if (index < m_menus.size() && m_menus[index].title_item) {
                return m_menus[index].title_item->bounds();
            }
            return rect_type{};
        }

        /**
         * @brief Navigate to next menu
         *
         * @details
         * If a menu is open, closes it and opens the next one.
         * Otherwise, focuses next menu title button.
         */
        void navigate_next() {
            if (m_menus.empty()) return;

            if (m_open_menu_index) {
                std::size_t const next_index = (*m_open_menu_index + 1) % m_menus.size();
                open_menu(next_index);
            } else {
                // Focus next title button
                // (Implementation depends on focus system)
            }
        }

        /**
         * @brief Navigate to previous menu
         *
         * @details
         * If a menu is open, closes it and opens the previous one.
         * Otherwise, focuses previous menu title button.
         */
        void navigate_previous() {
            if (m_menus.empty()) return;

            if (m_open_menu_index) {
                std::size_t const prev_index = *m_open_menu_index == 0
                    ? m_menus.size() - 1
                    : *m_open_menu_index - 1;
                open_menu(prev_index);
            } else {
                // Focus previous title button
                // (Implementation depends on focus system)
            }
        }


    protected:
        /**
         * @brief Handle keyboard events
         *
         * @details
         * Implements menu bar navigation:
         * - Left/Right: Navigate menus
         * - Down: Open focused menu
         * - Escape: Close menu
         * - Alt+mnemonic: Open specific menu
         */
        // Note: Keyboard handling would be implemented in derived classes
        // that know the specific event type from Backend


        /**
         * @brief Initialize semantic action handlers for menu bar
         *
         * @details
         * Registers handlers for:
         * - activate_menu_bar: Open first menu (F10/F9)
         * - menu_left/menu_right: Navigate between menus
         * - menu_up/menu_down: Navigate within dropdown
         * - menu_select: Activate focused item
         * - menu_cancel: Close menu
         */
        void initialize_hotkeys() {
            auto* hotkeys = ui_services<Backend>::hotkeys();
            if (!hotkeys) return;

            // Activate menu bar (F10/F9 based on scheme)
            hotkeys->register_semantic_action(
                hotkey_action::activate_menu_bar,
                [this]() {
                    if (m_open_menu_index.has_value()) {
                        // Menu already open - close it
                        close_menu();
                    } else if (!m_menus.empty()) {
                        // Open first menu
                        open_menu(0);
                    }
                }
            );

            // Navigate left between menus
            hotkeys->register_semantic_action(
                hotkey_action::menu_left,
                [this]() {
                    if (m_open_menu_index.has_value()) {
                        navigate_previous();
                    }
                }
            );

            // Navigate right between menus
            hotkeys->register_semantic_action(
                hotkey_action::menu_right,
                [this]() {
                    if (m_open_menu_index.has_value()) {
                        navigate_next();
                    }
                }
            );

            // NOTE: menu_down, menu_up, menu_select, menu_cancel are registered
            // per-menu using RAII guards in open_menu() (Phase 1).
            // This prevents conflicts when multiple menus exist.
        }

    private:
        std::vector<menu_entry<Backend>> m_menus;  ///< Menu entries with owned dropdown menus
        std::optional<std::size_t> m_open_menu_index;  ///< Currently open menu
        scoped_layer<Backend> m_current_menu;      ///< Current menu popup (RAII cleanup)
        std::vector<semantic_action_guard<Backend>> m_menu_nav_guards;  ///< RAII guards for menu navigation (Phase 1)
    };

} // namespace onyxui


namespace onyxui {

    // Implementation of methods using ui_services
    template<UIBackend Backend>
    void menu_bar<Backend>::open_menu(std::size_t index) {
        if (index >= m_menus.size()) return;

        // Close current menu if different
        if (m_open_menu_index && *m_open_menu_index != index) {
            close_menu();
        }

        m_open_menu_index = index;
        auto& entry = m_menus[index];

        // Update visual state of menu bar item
        if (entry.title_item) {
            entry.title_item->set_menu_open(true);
        }

        // Show context menu with outside click detection (Phase 1.3)
        // scoped_layer auto-closes previous menu when reassigned
        m_current_menu = entry.title_item->show_context_menu_scoped(
            entry.dropdown_menu.get()  // Pass raw pointer, unique_ptr retains ownership
        );

        // Register navigation hotkeys for THIS menu using RAII guards (Phase 1)
        // Guards automatically unregister when menu closes or switches
        auto* hotkeys = ui_services<Backend>::hotkeys();
        if (hotkeys && entry.dropdown_menu) {
            auto* menu = entry.dropdown_menu.get();

            // Clear previous guards (auto-unregister previous menu's handlers)
            m_menu_nav_guards.clear();

            // Register handlers for currently open menu
            m_menu_nav_guards.emplace_back(hotkeys, hotkey_action::menu_down,
                [menu]() { menu->focus_next(); });
            m_menu_nav_guards.emplace_back(hotkeys, hotkey_action::menu_up,
                [menu]() { menu->focus_previous(); });
            m_menu_nav_guards.emplace_back(hotkeys, hotkey_action::menu_select,
                [menu]() { menu->activate_focused(); });
            m_menu_nav_guards.emplace_back(hotkeys, hotkey_action::menu_cancel,
                [this]() { this->close_menu(); });
        }

        // Focus the menu and first item
        if (auto* focus = ui_services<Backend>::input()) {
            focus->set_focus(entry.dropdown_menu.get());  // Pass raw pointer
            if (entry.dropdown_menu) {
                entry.dropdown_menu->focus_first();
            }
        }
    }

    template<UIBackend Backend>
    void menu_bar<Backend>::close_menu() {
        if (!m_open_menu_index) {
            return;
        }

        // Update visual state of menu bar item
        auto& entry = m_menus[*m_open_menu_index];
        if (entry.title_item) {
            entry.title_item->set_menu_open(false);
        }

        // Unregister navigation hotkeys using RAII guards (Phase 1)
        // Guards automatically unregister when cleared
        m_menu_nav_guards.clear();

        // RAII cleanup - scoped_layer automatically removes layer
        m_current_menu.reset();

        // Clear focus
        if (auto* focus = ui_services<Backend>::input()) {
            focus->clear_focus();
        }

        m_open_menu_index = std::nullopt;
    }

} // namespace onyxui
