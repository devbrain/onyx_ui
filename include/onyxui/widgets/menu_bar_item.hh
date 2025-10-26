/**
 * @file menu_bar_item.hh
 * @brief Menu bar item widget (top-level menu trigger)
 * @author Claude Code
 * @date 2025-10-25
 *
 * @details
 * Dedicated widget for menu bar items (File, Edit, View, etc.).
 * This is NOT a button - it's a specialized widget for menu bar triggers.
 *
 * Key differences from button:
 * - Simpler rendering (text with minimal padding, no border)
 * - Designed specifically for horizontal menu bar layout
 * - Theme uses menu bar colors, not button colors
 */

#pragma once

#include <string>
#include <onyxui/widgets/stateful_widget.hh>
#include <onyxui/widgets/mnemonic_parser.hh>

namespace onyxui {
    /**
     * @class menu_bar_item
     * @brief Top-level menu trigger in a menu bar
     *
     * @tparam Backend The UI backend type
     *
     * @details
     * A menu_bar_item displays text (typically "File", "Edit", etc.) and
     * triggers a dropdown menu when clicked. It's visually distinct from
     * regular buttons - no border, compact padding, menu bar colors.
     *
     * ## Visual Appearance
     *
     * ```
     * File  Edit  View  Help
     * ^^^^  ^^^^  ^^^^  ^^^^
     * Each is a menu_bar_item (no borders, compact)
     * ```
     *
     * ## Signals
     *
     * - clicked: Emitted when item is clicked (inherited from stateful_widget)
     *
     * @example
     * @code
     * auto file_item = std::make_unique<menu_bar_item<Backend>>("File");
     * file_item->clicked.connect([&]() {
     *     open_file_menu();
     * });
     * @endcode
     */
    template<UIBackend Backend>
    class menu_bar_item : public stateful_widget<Backend> {
    public:
        using base = stateful_widget<Backend>;
        using renderer_type = typename Backend::renderer_type;
        using size_type = typename Backend::size_type;
        using theme_type = typename base::theme_type;

        /**
         * @brief Construct a menu bar item with text
         * @param text Item label text (e.g., "File", "Edit")
         * @param parent Parent element (typically menu_bar)
         */
        explicit menu_bar_item(std::string text = "", ui_element<Backend>* parent = nullptr)
            : base(parent), m_text(std::move(text)) {
            this->set_focusable(true);  // Menu bar items are focusable
            this->set_accept_keys_as_click(true);  // Enter/Space triggers click
        }

        /**
         * @brief Destructor
         */
        ~menu_bar_item() override = default;

        // Rule of Five
        menu_bar_item(const menu_bar_item&) = delete;
        menu_bar_item& operator=(const menu_bar_item&) = delete;
        menu_bar_item(menu_bar_item&&) noexcept = default;
        menu_bar_item& operator=(menu_bar_item&&) noexcept = default;

        /**
         * @brief Set the item text
         * @param text New text to display
         */
        void set_text(const std::string& text) {
            if (m_text != text) {
                m_text = text;
                this->invalidate_measure();  // Size may change
            }
        }

        /**
         * @brief Get the current text
         */
        [[nodiscard]] const std::string& text() const noexcept {
            return m_text;
        }

        /**
         * @brief Get the mnemonic character
         * @return Mnemonic character (lowercase), or '\0' if none
         */
        [[nodiscard]] char get_mnemonic_char() const noexcept {
            return m_has_mnemonic ? m_mnemonic_info.mnemonic_char : '\0';
        }

        /**
         * @brief Check if item has a mnemonic
         */
        [[nodiscard]] bool has_mnemonic() const noexcept {
            return m_has_mnemonic && m_mnemonic_info.mnemonic_char != '\0';
        }

        /**
         * @brief Set whether this item's menu is open
         * @param is_open True if menu is open, false otherwise
         *
         * @details
         * Called by menu_bar when opening/closing menus.
         * Affects visual state (open state uses different colors).
         */
        void set_menu_open(bool is_open) {
            if (m_is_menu_open != is_open) {
                m_is_menu_open = is_open;
                this->invalidate_arrange();  // Trigger redraw
            }
        }

        /**
         * @brief Check if this item's menu is open
         */
        [[nodiscard]] bool is_menu_open() const noexcept {
            return m_is_menu_open;
        }

    protected:
        /**
         * @brief Render the menu bar item
         *
         * @details
         * Menu bar items are rendered WITHOUT borders.
         * They use menu bar theme colors and compact padding.
         */
        void do_render(render_context<Backend>& ctx) const override {
            auto* theme = this->get_theme();
            if (!theme) return;

            // DEBUG: Log menu bar item state
            std::cerr << "[menu_bar_item::do_render] text=\"" << m_text << "\" "
                      << "hover=" << this->is_hovered() << " "
                      << "open=" << m_is_menu_open << std::endl;

            // Get padding from theme (configurable!)
            int const horizontal_padding = theme->menu_bar.item_padding_horizontal;
            int const vertical_padding = theme->menu_bar.item_padding_vertical;

            // Get position from context
            auto const& pos = ctx.position();
            int const x = point_utils::get_x(pos);
            int const y = point_utils::get_y(pos);

            // Use pre-resolved style from context (includes state-dependent font!)
            auto const& text_font = ctx.style().font;
            auto const& fg = ctx.style().foreground_color;

            // DEBUG: Log resolved colors
            std::cerr << "[menu_bar_item::do_render] ctx.style() fg=("
                      << static_cast<int>(fg.r) << "," << static_cast<int>(fg.g) << "," << static_cast<int>(fg.b) << ")" << std::endl;

            // Menu bar items render ONLY text (no background, no border)
            // The menu bar itself provides the background

            // Draw text with theme-configured padding
            int const text_x = x + horizontal_padding;
            int const text_y = y + vertical_padding;
            typename Backend::point_type const text_pos{text_x, text_y};
            ctx.draw_text(m_text, text_pos, text_font, fg);
        }

        /**
         * @brief Get theme-specific foreground color
         * @return Menu bar item foreground color from theme (state-dependent)
         * @details Uses menu_bar_item-specific states (normal/hover/open)
         */
        [[nodiscard]] typename Backend::color_type get_theme_foreground_color(const theme_type& theme) const override {
            // Map menu_bar_item states to visual_state
            if (m_is_menu_open) {
                return theme.menu_bar_item.open.foreground;
            }
            if (this->is_hovered()) {
                return theme.menu_bar_item.hover.foreground;
            }
            return theme.menu_bar_item.normal.foreground;
        }

        /**
         * @brief Get theme-specific background color
         * @return Menu bar item background color from theme (state-dependent)
         * @details Uses menu_bar_item-specific states (normal/hover/open)
         */
        [[nodiscard]] typename Backend::color_type get_theme_background_color(const theme_type& theme) const override {
            // Map menu_bar_item states to visual_state
            if (m_is_menu_open) {
                return theme.menu_bar_item.open.background;
            }
            if (this->is_hovered()) {
                return theme.menu_bar_item.hover.background;
            }
            return theme.menu_bar_item.normal.background;
        }

        /**
         * @brief Get theme-specific box style
         * @return ALWAYS returns no-border style for menu bar items
         *
         * @details
         * Menu bar items NEVER have borders. This overrides any inheritance.
         */
        [[nodiscard]] typename Backend::renderer_type::box_style get_theme_box_style([[maybe_unused]] const theme_type& theme) const override {
            // Menu bar items NEVER have borders!
            // Return default-constructed box_style (no border)
            return typename Backend::renderer_type::box_style{};
        }

        /**
         * @brief Get theme-specific font
         * @return Menu bar item font from theme
         */
        [[nodiscard]] typename Backend::renderer_type::font get_theme_font(const theme_type& theme) const override {
            return theme.menu_bar_item.normal.font;
        }

        /**
         * @brief Apply theme to menu bar item
         */
        void do_apply_theme([[maybe_unused]] const theme_type& theme) override {
            this->invalidate_arrange();  // Redraw with new theme
        }

    private:
        std::string m_text;                       ///< Plain item text
        mnemonic_info<Backend> m_mnemonic_info;   ///< Parsed mnemonic (unused for now)
        bool m_has_mnemonic = false;              ///< Whether mnemonic is active
        bool m_is_menu_open = false;              ///< Whether this item's menu is open
    };

} // namespace onyxui
