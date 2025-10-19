/**
 * @file menu_item.hh
 * @brief Menu item widget with mnemonic and shortcut display
 * @author igor
 * @date 16/10/2025
 *
 * @details
 * Represents a single item in a menu, supporting:
 * - Text with mnemonic (Alt+key navigation)
 * - Action integration (triggers on activation)
 * - Shortcut display (shows Ctrl+S, F9, etc.)
 * - Separator style
 * - Submenu indicators
 * - Enabled/disabled state
 *
 * ## Menu Item Types
 *
 * **Normal Item:**
 * Text with optional mnemonic and shortcut
 * ```
 * &Save          Ctrl+S
 * ```
 *
 * **Separator:**
 * Visual divider between menu sections
 * ```
 * ─────────────────────
 * ```
 *
 * **Submenu:**
 * Opens another menu
 * ```
 * &Recent Files    ▶
 * ```
 *
 * ## Usage Example
 *
 * ```cpp
 * auto save_action = std::make_shared<action<Backend>>();
 * save_action->set_text("Save");
 * save_action->set_shortcut('s', key_modifier::ctrl);
 * save_action->triggered.connect([]() { save_document(); });
 *
 * auto item = std::make_unique<menu_item<Backend>>();
 * item->set_action(save_action);
 * item->set_mnemonic_text("&Save");  // Alt+S to activate
 * // Renders: "Save          Ctrl+S"
 * ```
 *
 * @see menu.hh For menu container
 * @see action.hh For action shortcuts
 * @see mnemonic_parser.hh For mnemonic rendering
 */

#pragma once

#include <string>
#include <onyxui/widgets/widget.hh>
#include <onyxui/widgets/action.hh>
#include <onyxui/widgets/mnemonic_parser.hh>

namespace onyxui {

    /**
     * @enum menu_item_type
     * @brief Type of menu item
     */
    enum class menu_item_type {
        normal,     ///< Regular menu item with text
        separator,  ///< Horizontal divider
        submenu     ///< Opens a submenu
    };

    /**
     * @class menu_item
     * @brief Single item in a menu
     *
     * @tparam Backend The UI backend type
     *
     * @details
     * Menu items are typically contained in a menu widget.
     * They display text with optional mnemonic and shortcut hint.
     *
     * ## Visual Layout
     *
     * ```
     * [Icon] Text with mnemonic     Shortcut  [▶]
     * ```
     *
     * - Icon: Optional visual indicator
     * - Text: Main label (with mnemonic underlined)
     * - Shortcut: Optional keyboard shortcut (from action)
     * - ▶: Submenu indicator
     *
     * ## Interaction
     *
     * - Click: Activates item (triggers action)
     * - Enter: Activates focused item
     * - Alt+mnemonic: Activates item with matching mnemonic
     * - Hover: Visual highlight
     */
    template<UIBackend Backend>
    class menu_item : public widget<Backend> {
    public:
        using base = widget<Backend>;
        using renderer_type = typename Backend::renderer_type;
        using size_type = typename Backend::size_type;
        using rect_type = typename Backend::rect_type;
        using theme_type = typename base::theme_type;

        /**
         * @brief Construct a menu item
         * @param text Item text (can include mnemonic syntax)
         * @param parent Parent element
         */
        explicit menu_item(std::string text = "", ui_element<Backend>* parent = nullptr)
            : base(parent)
            , m_text(std::move(text))
            , m_has_mnemonic(false)
            , m_item_type(menu_item_type::normal) {
            this->set_focusable(true);  // Menu items are focusable
        }

        /**
         * @brief Construct a separator
         * @param parent Parent element
         * @return menu_item configured as separator
         */
        static std::unique_ptr<menu_item> make_separator(ui_element<Backend>* parent = nullptr) {
            auto item = std::make_unique<menu_item>("", parent);
            item->m_item_type = menu_item_type::separator;
            item->set_focusable(false);  // Separators are not focusable
            return item;
        }

        /**
         * @brief Destructor
         */
        ~menu_item() override = default;

        // Rule of Five
        menu_item(const menu_item&) = delete;
        menu_item& operator=(const menu_item&) = delete;
        menu_item(menu_item&&) noexcept = default;
        menu_item& operator=(menu_item&&) noexcept = default;

        /**
         * @brief Set the item text
         * @param text Plain text (no mnemonic syntax)
         */
        void set_text(const std::string& text) {
            if (m_text != text) {
                m_text = text;
                m_has_mnemonic = false;
                this->invalidate_measure();
            }
        }

        /**
         * @brief Get the current text
         */
        [[nodiscard]] const std::string& text() const noexcept {
            return m_text;
        }

        /**
         * @brief Set item text with mnemonic syntax
         * @param mnemonic_text Text with "&" markers (e.g., "&Open")
         *
         * @details
         * Parses mnemonic syntax for rendering and navigation.
         * Requires theme to be applied first.
         *
         * @example
         * @code
         * item->set_mnemonic_text("&File");    // Alt+F
         * item->set_mnemonic_text("E&xit");    // Alt+X
         * item->set_mnemonic_text("Save && Close");  // Literal "&"
         * @endcode
         */
        void set_mnemonic_text(std::string_view mnemonic_text) {
            m_text = strip_mnemonic(mnemonic_text);

            if (auto* theme = this->m_theme) {
                m_mnemonic_info = parse_mnemonic<Backend>(
                    mnemonic_text,
                    theme->button.font,  // Use button fonts as fallback for menu items
                    theme->button.mnemonic_font
                );
                m_has_mnemonic = true;
            } else {
                m_has_mnemonic = false;
            }

            this->invalidate_measure();
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
         * @brief Get the item type
         */
        [[nodiscard]] menu_item_type type() const noexcept {
            return m_item_type;
        }

        /**
         * @brief Check if this is a separator
         */
        [[nodiscard]] bool is_separator() const noexcept {
            return m_item_type == menu_item_type::separator;
        }

        /**
         * @brief Get shortcut text from associated action
         * @return Shortcut string (e.g., "Ctrl+S"), or empty if none
         */
        [[nodiscard]] std::string get_shortcut_text() const {
            if (auto action_ptr = this->get_action()) {
                if (action_ptr->has_shortcut()) {
                    // Format shortcut for display
                    const auto& seq = action_ptr->shortcut().value();
                    std::string result;
                    if ((seq.modifiers & key_modifier::ctrl) != key_modifier::none) result += "Ctrl+";
                    if ((seq.modifiers & key_modifier::alt) != key_modifier::none) result += "Alt+";
                    if ((seq.modifiers & key_modifier::shift) != key_modifier::none) result += "Shift+";
                    if (seq.f_key != 0) {
                        result += "F" + std::to_string(seq.f_key);
                    } else {
                        result += static_cast<char>(std::toupper(static_cast<unsigned char>(seq.key)));
                    }
                    return result;
                }
            }
            return "";
        }

    protected:
        /**
         * @brief Render the menu item
         *
         * @details
         * Uses render_context for both measurement and rendering.
         * During measurement: calculates text + shortcut width
         * During rendering: draws background, text, and shortcut with state colors
         */
        void do_render(render_context<Backend>& ctx) const override {
            auto* theme = this->m_theme;
            if (!theme) return;

            // Calculate sizes (needed for both measurement and rendering)
            typename renderer_type::font text_font = theme->button.font;
            auto text_size = renderer_type::measure_text(m_text, text_font);
            int text_width = size_utils::get_width(text_size);
            int text_height = size_utils::get_height(text_size);

            std::string shortcut = get_shortcut_text();
            int shortcut_width = 0;
            if (!shortcut.empty()) {
                typename renderer_type::font shortcut_font{};
                auto shortcut_size = renderer_type::measure_text(shortcut, shortcut_font);
                shortcut_width = size_utils::get_width(shortcut_size);
            }

            // Separator has fixed size
            if (is_separator()) {
                if (ctx.is_rendering()) {
                    const auto& item_bounds = this->bounds();
                    int width = rect_utils::get_width(item_bounds);
                    std::string sep_line(static_cast<size_t>(width), '-');
                    typename Backend::point_type pos{rect_utils::get_x(item_bounds),
                                                     rect_utils::get_y(item_bounds)};
                    ctx.draw_text(sep_line, pos, typename renderer_type::font{},
                                 theme->label.text);
                } else {
                    // MEASUREMENT: Separator is fixed 20x1
                    typename Backend::rect_type sep_rect;
                    rect_utils::set_bounds(sep_rect, 0, 0, 20, 1);
                    ctx.draw_rect(sep_rect, typename renderer_type::box_style{});
                }
                return;
            }

            // Calculate total width: padding + text + gap + shortcut + padding
            int total_width = 2 + text_width + (shortcut_width > 0 ? 4 + shortcut_width : 0) + 2;
            int total_height = text_height;

            if (ctx.is_rendering()) {
                // RENDERING PATH: Draw with state colors
                const auto& item_bounds = this->bounds();

                bool is_focused = this->has_focus();
                bool is_hovered = this->is_hovered();
                bool is_highlighted = is_focused || is_hovered;

                // Select colors based on state
                typename Backend::color_type fg;
                if (!this->is_enabled()) {
                    fg = theme->button.fg_disabled;
                } else if (is_highlighted) {
                    fg = theme->button.fg_hover;
                } else {
                    fg = theme->button.fg_normal;
                }

                // Draw item text (left side, with padding)
                int text_x = rect_utils::get_x(item_bounds) + 2;  // 2 char padding
                int text_y = rect_utils::get_y(item_bounds);
                typename Backend::point_type text_pos{text_x, text_y};
                ctx.draw_text(m_text, text_pos, text_font, fg);

                // Draw shortcut (right side)
                if (!shortcut.empty()) {
                    int width = rect_utils::get_width(item_bounds);
                    int shortcut_x = rect_utils::get_x(item_bounds) + width - shortcut_width - 2;
                    typename Backend::point_type shortcut_pos{shortcut_x, text_y};
                    ctx.draw_text(shortcut, shortcut_pos, typename renderer_type::font{}, fg);
                }
            } else {
                // MEASUREMENT PATH: Track the size
                typename Backend::rect_type item_rect;
                rect_utils::set_bounds(item_rect, 0, 0, total_width, total_height);
                ctx.draw_rect(item_rect, typename renderer_type::box_style{});
            }
        }

        /**
         * @brief Apply theme to menu item
         */
        void do_apply_theme([[maybe_unused]] const theme_type& theme) override {
            // Reparse mnemonic if we have one
            if (m_has_mnemonic) {
                m_has_mnemonic = false;  // Clear until set_mnemonic_text is called again
            }
            this->invalidate_arrange();
        }

        /**
         * @brief Handle action association
         *
         * @details
         * Syncs menu item with action:
         * - Text from action
         * - Triggers action on activation
         * - Shows shortcut from action
         * - Syncs enabled state
         */
        void on_action_changed(std::shared_ptr<action<Backend>> action_ptr) override {
            base::on_action_changed(action_ptr);

            if (action_ptr) {
                // Sync text with action
                set_text(action_ptr->text());

                // Connect action text changes
                this->m_action_connections.emplace_back(
                    action_ptr->text_changed,
                    [this](std::string_view new_text) {
                        this->set_text(std::string(new_text));
                    }
                );

                // Connect click to action trigger
                this->m_action_connections.emplace_back(
                    this->clicked,
                    [weak_action = std::weak_ptr(action_ptr)]() {
                        if (auto action = weak_action.lock()) {
                            action->trigger();
                        }
                    }
                );

                // Connect shortcut changes (invalidate to redraw)
                this->m_action_connections.emplace_back(
                    action_ptr->shortcut_changed,
                    [this]([[maybe_unused]] const std::optional<key_sequence>& shortcut) {
                        this->invalidate_measure();  // Width may change
                    }
                );
            }
        }

    private:
        std::string m_text;                       ///< Plain item text
        mnemonic_info<Backend> m_mnemonic_info;   ///< Parsed mnemonic with styling
        bool m_has_mnemonic;                      ///< Whether mnemonic is active
        menu_item_type m_item_type;               ///< Type of item
    };

} // namespace onyxui
