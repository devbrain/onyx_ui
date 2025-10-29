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
         * Menu bar items draw background when hovered or when menu is open.
         * They use menu bar theme colors and compact padding.
         */
        void do_render(render_context<Backend>& ctx) const override {
            // Get padding from resolved style (with defaults)
            int const horizontal_padding = ctx.style().padding_horizontal.value.value_or(2);
            int const vertical_padding = ctx.style().padding_vertical.value.value_or(0);

            // Get position from context
            auto const& pos = ctx.position();
            int const x = point_utils::get_x(pos);
            int const y = point_utils::get_y(pos);

            // Use pre-resolved style from context (includes state-dependent font!)
            auto const& text_font = ctx.style().font;
            auto const& fg = ctx.style().foreground_color;

            // Measure text to determine item size
            auto text_size = renderer_type::measure_text(m_text, text_font);
            int const text_width = size_utils::get_width(text_size);
            int const text_height = size_utils::get_height(text_size);

            // Calculate full item bounds including padding
            int const total_width = text_width + 2 * horizontal_padding;
            int const total_height = text_height + 2 * vertical_padding;

            // Draw background if hovered or menu is open (for highlighting)
            if (this->is_hovered() || m_is_menu_open) {
                typename Backend::rect_type bg_rect;
                rect_utils::set_bounds(bg_rect, x, y, total_width, total_height);
                ctx.fill_rect(bg_rect);
            }

            // Draw text with theme-configured padding
            int const text_x = x + horizontal_padding;
            int const text_y = y + vertical_padding;
            typename Backend::point_type const text_pos{text_x, text_y};
            ctx.draw_text(m_text, text_pos, text_font, fg);
        }

        /**
         * @brief Stateful widget - does NOT inherit colors from parent
         * @return false - menu bar items manage their own state-based colors
         *
         * @details
         * Menu bar items have state-dependent colors (normal/hover/open) that
         * must NOT be overridden by parent CSS inheritance.
         */
        [[nodiscard]] bool should_inherit_colors() const override {
            return false;  // Stateful widget - use theme colors, not parent colors
        }

        /**
         * @brief Get complete widget style from theme
         * @param theme Theme to extract properties from
         * @return Resolved style with menu_bar_item-specific theme values
         * @details Uses menu_bar_item-specific states (normal/hover/open)
         */
        [[nodiscard]] resolved_style<Backend> get_theme_style(const theme_type& theme) const override {
            // Determine which menu_bar_item state to use
            typename Backend::color_type bg, fg;
            typename Backend::renderer_type::font font;

            if (m_is_menu_open) {
                bg = theme.menu_bar_item.open.background;
                fg = theme.menu_bar_item.open.foreground;
                font = theme.menu_bar_item.open.font;
            } else if (this->is_hovered()) {
                bg = theme.menu_bar_item.hover.background;
                fg = theme.menu_bar_item.hover.foreground;
                font = theme.menu_bar_item.hover.font;
            } else {
                bg = theme.menu_bar_item.normal.background;
                fg = theme.menu_bar_item.normal.foreground;
                font = theme.menu_bar_item.normal.font;
            }

            return resolved_style<Backend>{
                .background_color = bg,
                .foreground_color = fg,
                .border_color = theme.border_color,
                .box_style = typename Backend::renderer_type::box_style{},  // Menu bar items NEVER have borders!
                .font = font,
                .opacity = 1.0f,
                .icon_style = std::optional<typename Backend::renderer_type::icon_style>{},
                .padding_horizontal = std::make_optional(theme.menu_bar.item_padding_horizontal),  // Menu bar item has padding
                .padding_vertical = std::make_optional(theme.menu_bar.item_padding_vertical),
                .mnemonic_font = std::optional<typename Backend::renderer_type::font>{}  // Menu bar item has no mnemonics (unused)
            };
        }


    private:
        std::string m_text;                       ///< Plain item text
        mnemonic_info<Backend> m_mnemonic_info;   ///< Parsed mnemonic (unused for now)
        bool m_has_mnemonic = false;              ///< Whether mnemonic is active
        bool m_is_menu_open = false;              ///< Whether this item's menu is open
    };

} // namespace onyxui
