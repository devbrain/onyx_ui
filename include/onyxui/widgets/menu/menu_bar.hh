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
#include <onyxui/widgets/core/widget_container.hh>
#include <onyxui/layout/linear_layout.hh>
#include <onyxui/layout/layout_strategy.hh>
#include <onyxui/widgets/menu/menu.hh>
#include <onyxui/widgets/menu/menu_bar_item.hh>
#include <onyxui/actions/mnemonic_parser.hh>
#include <onyxui/core/raii/scoped_layer.hh>
#include <onyxui/core/rendering/resolved_style.hh>
#include <onyxui/services/ui_services.hh>
#include <onyxui/hotkeys/hotkey_action.hh>
#include <onyxui/widgets/menu/menu_system.hh>

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
     * Inherits from widget_container and uses horizontal linear_layout.
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
    class menu_bar : public widget_container<Backend> {
    public:
        using base = widget_container<Backend>;
        using renderer_type = typename Backend::renderer_type;
        using rect_type = typename Backend::rect_type;
        using theme_type = typename base::theme_type;

        /**
         * @brief Construct an empty menu bar
         * @param parent Parent element
         */
        explicit menu_bar(ui_element<Backend>* parent = nullptr)
            : base(parent)
            , m_spacing(0)
            , m_h_align(horizontal_alignment::left)
            , m_v_align(vertical_alignment::top)
            , m_open_menu_index(std::nullopt)
            , m_menu_system(this) {  // Phase 4: Initialize menu_system with this pointer

            // Set up horizontal layout strategy
            this->set_layout_strategy(
                std::make_unique<linear_layout<Backend>>(
                    direction::horizontal, m_spacing, m_h_align, m_v_align));

            // Menu bar should expand to fill parent width (creates continuous stripe)
            this->set_width_constraint({size_policy::expand});

            // NOTE: Menu bar should NOT be in Tab order
            // It's activated via F10/F9 semantic action, not Tab navigation
            this->set_focusable(false);
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

        /**
         * @brief Set spacing between menu items
         * @param spacing New spacing in pixels
         */
        void set_spacing(int spacing) {
            if (m_spacing != spacing) {
                m_spacing = spacing;
                recreate_layout();
            }
        }

        /**
         * @brief Get current spacing between menu items
         * @return Spacing in pixels
         */
        [[nodiscard]] int spacing() const noexcept { return m_spacing; }

    protected:
        /**
         * @brief Menu bar does NOT inherit colors from parent
         * @return false - menu bar has its own distinct background color
         *
         * @details
         * Menu bar must use its own theme colors (white stripe) and not inherit
         * the parent's background color (which is typically black window background).
         */
        [[nodiscard]] bool should_inherit_colors() const override {
            return false;  // Menu bar has distinct background (white stripe in NU8)
        }

        /**
         * @brief Get complete widget style from theme
         * @param theme Theme to extract properties from
         * @return Resolved style with menu_bar background (same as menu_bar_item normal)
         */
        [[nodiscard]] resolved_style<Backend> get_theme_style(const theme_type& theme) const override {
            // Menu bar uses the same background as normal menu bar items
            // This creates the continuous stripe effect (Norton Utilities 8 style)
            return resolved_style<Backend>{
                .background_color = theme.menu_bar_item.normal.background,
                .foreground_color = theme.text_fg,
                .mnemonic_foreground = theme.text_fg,  // Mnemonics same as text (menu bar has no mnemonics)
                .border_color = theme.border_color,
                .box_style = typename Backend::renderer_type::box_style{},
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
         * - activate_menu_bar: Open/close menu bar (F10/F9)
         *
         * Phase 4: All menu navigation (left/right/up/down/select/cancel) is now
         * handled by menu_system, which provides context-dependent navigation.
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
        }

        /**
         * @brief Render menu bar, updating spacing from theme if needed
         * @param ctx Render context (provides access to current theme)
         */
        void do_render(render_context<Backend>& ctx) const override {
            // Update spacing from theme before rendering
            if (auto* theme = ctx.theme()) {
                int theme_spacing = theme->menu_bar.item_spacing;
                if (theme_spacing != m_spacing) {
                    // Need to update spacing - cast away const (spacing is mutable layout state)
                    const_cast<menu_bar*>(this)->set_spacing(theme_spacing);
                }
            }

            // Draw continuous background stripe (Norton Utilities 8 style)
            // This fills the entire menu bar width, creating a continuous colored bar
            ctx.fill_rect(this->bounds());

            // Render children on top of background
            base::do_render(ctx);
        }

    private:
        /**
         * @brief Recreate the layout strategy with current settings
         */
        void recreate_layout() {
            auto new_layout = std::make_unique<linear_layout<Backend>>(
                direction::horizontal, m_spacing, m_h_align, m_v_align);
            this->set_layout_strategy(std::move(new_layout));
        }

        int m_spacing;                                 ///< Spacing between menu items (from theme)
        horizontal_alignment m_h_align;               ///< Horizontal alignment for items
        vertical_alignment m_v_align;                 ///< Vertical alignment for items
        std::vector<menu_entry<Backend>> m_menus;     ///< Menu entries with owned dropdown menus
        std::optional<std::size_t> m_open_menu_index; ///< Currently open menu
        scoped_layer<Backend> m_current_menu;         ///< Current menu popup (RAII cleanup)
        menu_system<Backend> m_menu_system;           ///< Centralized menu navigation coordinator (Phase 4)
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

        // Clear menu_open state on ALL items, then set only the current one
        // This prevents stale highlighting on other menu items
        for (std::size_t i = 0; i < m_menus.size(); ++i) {
            if (m_menus[i].title_item) {
                m_menus[i].title_item->set_menu_open(i == index);
            }
        }

        auto& entry = m_menus[index];

        // Show context menu with outside click detection (Phase 1.3)
        // scoped_layer auto-closes previous menu when reassigned
        m_current_menu = entry.title_item->show_context_menu_scoped(
            entry.dropdown_menu.get()  // Pass raw pointer, unique_ptr retains ownership
        );

        // Phase 4: Notify menu_system of open menu (registers handlers once, not per switch)
        m_menu_system.open_top_level_menu(entry.dropdown_menu.get());

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

        // Clear menu_open state on ALL items (defensive - ensures no stale highlighting)
        for (auto& entry : m_menus) {
            if (entry.title_item) {
                entry.title_item->set_menu_open(false);
            }
        }

        // Phase 4: Notify menu_system to close all menus (unregisters handlers via RAII)
        m_menu_system.close_all_menus();

        // RAII cleanup - scoped_layer automatically removes layer
        m_current_menu.reset();

        // Clear focus
        if (auto* focus = ui_services<Backend>::input()) {
            focus->clear_focus();
        }

        m_open_menu_index = std::nullopt;
    }

} // namespace onyxui
