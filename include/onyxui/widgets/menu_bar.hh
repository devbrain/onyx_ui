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
#include <onyxui/widgets/button.hh>
#include <onyxui/widgets/mnemonic_parser.hh>
#include <onyxui/layer_manager.hh>
#include <onyxui/scoped_layer.hh>
#include <onyxui/ui_services.hh>
#include <onyxui/focus_manager.hh>

namespace onyxui {

    // Forward declaration
    template<UIBackend Backend>
    class ui_handle;

    /**
     * @struct menu_entry
     * @brief Associates a menu title button with its dropdown menu
     */
    template<UIBackend Backend>
    struct menu_entry {
        button<Backend>* title_button;  ///< Button for menu title
        menu<Backend>* dropdown_menu;   ///< Associated dropdown menu
        char mnemonic_char;              ///< Mnemonic character for Alt+key
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
            : base(0, horizontal_alignment::left, vertical_alignment::top, parent)
            , m_open_menu_index(std::nullopt) {
            this->set_focusable(true);
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
            // Create button for menu title
            auto title_button = std::make_unique<button<Backend>>("", this);

            // Parse mnemonic from title
            mnemonic_info<Backend> mnemonic = parse_mnemonic<Backend>(
                title,
                typename Backend::renderer_type::font{},  // Would use theme fonts
                typename Backend::renderer_type::font{}
            );

            title_button->set_text(strip_mnemonic(title));

            // Store button pointer before moving
            button<Backend>* button_ptr = title_button.get();

            // Add button to menu bar
            this->add_child(std::move(title_button));

            // Store menu entry
            menu_entry<Backend> entry{
                button_ptr,
                menu_ptr.get(),
                mnemonic.mnemonic_char
            };

            std::size_t index = m_menus.size();
            m_menus.push_back(entry);
            m_owned_menus.push_back(std::move(menu_ptr));

            // Connect button click to open menu
            button_ptr->clicked.connect([this, index]() {
                this->open_menu(index);
            });

            // Connect menu closing to cleanup
            entry.dropdown_menu->closing.connect([this]() {
                this->close_menu();
            });

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
                return m_menus[index].dropdown_menu;
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
            if (index < m_menus.size() && m_menus[index].title_button) {
                return m_menus[index].title_button->bounds();
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
                std::size_t next_index = (*m_open_menu_index + 1) % m_menus.size();
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
                std::size_t prev_index = *m_open_menu_index == 0
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
         * @brief Apply theme to menu bar and all owned menus
         */
        void do_apply_theme(const typename base::theme_type& theme) override {
            // Apply to base (hbox) which propagates to title buttons
            base::do_apply_theme(theme);

            // Also apply to all owned menus (not part of normal child tree)
            for (auto& menu_ptr : m_owned_menus) {
                if (menu_ptr) {
                    menu_ptr->apply_theme(theme);
                }
            }
        }

    private:
        std::vector<menu_entry<Backend>> m_menus;                    ///< Menu entries
        std::vector<std::unique_ptr<menu<Backend>>> m_owned_menus;   ///< Owned menus
        std::optional<std::size_t> m_open_menu_index;                ///< Currently open menu
        scoped_layer<Backend> m_current_menu;                        ///< Current menu popup (RAII cleanup)
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

        // Show context menu with outside click detection (Phase 1.3)
        // scoped_layer auto-closes previous menu when reassigned
        m_current_menu = entry.title_button->show_context_menu_scoped(
            entry.dropdown_menu
        );

        // Focus the menu and first item
        if (auto* focus = ui_services<Backend>::focus()) {
            focus->set_focus(entry.dropdown_menu);
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

        // RAII cleanup - scoped_layer automatically removes layer
        m_current_menu.reset();

        // Clear focus
        if (auto* focus = ui_services<Backend>::focus()) {
            focus->clear_focus();
        }

        m_open_menu_index = std::nullopt;
    }

} // namespace onyxui
