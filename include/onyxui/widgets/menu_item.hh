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
#include <onyxui/widgets/stateful_widget.hh>
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
    class menu_item : public stateful_widget<Backend> {
    public:
        using base = stateful_widget<Backend>;
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

            if (auto* theme = this->get_theme()) {
                m_mnemonic_info = parse_mnemonic<Backend>(
                    mnemonic_text,
                    theme->menu_item.normal.font,
                    theme->menu_item.mnemonic_font
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
                const auto& shortcut_opt = action_ptr->shortcut();
                if (shortcut_opt.has_value()) {
                    // Format shortcut for display
                    const auto& seq = *shortcut_opt;
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
            auto* theme = this->get_theme();
            if (!theme) return;

            // DEBUG: Log menu item state
            std::cerr << "[menu_item::do_render] text=\"" << m_text << "\" "
                      << "hover=" << this->is_hovered() << " "
                      << "focus=" << this->has_focus() << " "
                      << "enabled=" << this->is_enabled() << std::endl;

            // Use resolved style from context (includes state-dependent font!)
            auto const& text_font = ctx.style().font;
            auto const& fg = ctx.style().foreground_color;

            // DEBUG: Log resolved colors
            std::cerr << "[menu_item::do_render] ctx.style() bg=("
                      << static_cast<int>(ctx.style().background_color.r) << ","
                      << static_cast<int>(ctx.style().background_color.g) << ","
                      << static_cast<int>(ctx.style().background_color.b) << ") fg=("
                      << static_cast<int>(fg.r) << "," << static_cast<int>(fg.g) << "," << static_cast<int>(fg.b) << ")" << std::endl;

            // Calculate sizes (needed for both measurement and rendering)
            std::string const shortcut = get_shortcut_text();
            int shortcut_width = 0;
            if (!shortcut.empty()) {
                typename renderer_type::font const shortcut_font{};
                auto shortcut_size = renderer_type::measure_text(shortcut, shortcut_font);
                shortcut_width = size_utils::get_width(shortcut_size);
            }

            // Get text size for measurement
            auto text_size = renderer_type::measure_text(m_text, text_font);
            int text_width = size_utils::get_width(text_size);

            // Define padding constants
            constexpr int LEFT_PADDING = 2;
            constexpr int RIGHT_PADDING = 2;
            constexpr int SHORTCUT_SPACING = 2;  // Space between text and shortcut

            // Use bounds for positioning during rendering
            // During measurement, bounds will be zero, which is fine
            const auto& item_bounds = this->bounds();
            int const base_x = rect_utils::get_x(item_bounds);
            int const base_y = rect_utils::get_y(item_bounds);
            int const item_width = rect_utils::get_width(item_bounds);

            // Separator has fixed size
            if (is_separator()) {
                int const min_width = 20;
                // During measurement (item_width == 0), use minimum width
                // During rendering, use actual width
                int const width = item_width > 0 ? std::max(min_width, item_width) : min_width;
                std::string const sep_line(static_cast<size_t>(width), '-');
                typename Backend::point_type const pos{base_x, base_y};
                ctx.draw_text(sep_line, pos, typename renderer_type::font{},
                             theme->label.text);
                return;
            }

            // Calculate minimum width needed
            int const min_width = LEFT_PADDING + text_width +
                                  (shortcut.empty() ? 0 : SHORTCUT_SPACING + shortcut_width) +
                                  RIGHT_PADDING;

            // Use actual width if available (rendering), otherwise use minimum (measurement)
            // During measurement, item_width is 0, so we use min_width
            int const effective_width = (item_width > 0) ? item_width : min_width;

            // Get text height for background rectangle
            int const text_height = size_utils::get_height(text_size);

            // Draw background rectangle using resolved style
            // Background color is state-dependent (normal/highlighted/disabled)
            typename Backend::rect_type bg_rect;
            rect_utils::set_bounds(bg_rect, base_x, base_y, effective_width, text_height);
            ctx.fill_rect(bg_rect);

            // Ensure measurement includes left padding by drawing marker at leftmost edge
            // During measurement (base_x = 0), this ensures bounding box starts at 0
            // During rendering, this is harmless (just a space at the left edge)
            typename Backend::point_type const left_marker{base_x, base_y};
            ctx.draw_text(" ", left_marker, text_font, fg);

            // Draw text with left padding
            int const text_x = base_x + LEFT_PADDING;
            typename Backend::point_type const text_pos{text_x, base_y};
            ctx.draw_text(m_text, text_pos, text_font, fg);

            // Draw shortcut if present (right-aligned within effective width)
            if (!shortcut.empty()) {
                int const shortcut_x = base_x + effective_width - shortcut_width - RIGHT_PADDING;
                typename Backend::point_type const shortcut_pos{shortcut_x, base_y};
                ctx.draw_text(shortcut, shortcut_pos, typename renderer_type::font{}, fg);
            }

            // Ensure measurement includes right padding by drawing marker at rightmost edge
            // During measurement this ensures the bounding box extends to include right padding
            // During rendering this is harmless (just a space at the right edge)
            int const right_edge = base_x + effective_width - 1;
            typename Backend::point_type const right_marker{right_edge, base_y};
            ctx.draw_text(" ", right_marker, text_font, fg);
        }

    public:
        /**
         * @brief Resolve style with state-dependent colors (NO parent color inheritance)
         *
         * @details
         * Menu items use STATE-DEPENDENT colors that should NOT be inherited from parents.
         * We override resolve_style() to use our own state colors directly instead of
         * inheriting parent colors.
         *
         * Priority: explicit override → state-dependent theme color → default
         */
        [[nodiscard]] resolved_style<Backend> resolve_style() const override {
            resolved_style<Backend> style;

            // Resolve background color: explicit override OR state-dependent theme color
            if (this->m_background_override) {
                style.background_color = *this->m_background_override;
            } else if (auto* theme = this->get_theme()) {
                style.background_color = get_theme_background_color(*theme);
                std::cerr << "[menu_item::resolve_style] Using state-dependent bg" << std::endl;
            } else {
                style.background_color = typename Backend::color_type{};
            }

            // Resolve foreground color: explicit override OR state-dependent theme color
            if (this->m_foreground_override) {
                style.foreground_color = *this->m_foreground_override;
            } else if (auto* theme = this->get_theme()) {
                style.foreground_color = get_theme_foreground_color(*theme);
            } else {
                style.foreground_color = typename Backend::color_type{};
            }

            // Resolve other properties normally (these can inherit from parent)
            style.box_style = this->get_effective_box_style();
            style.font = this->get_effective_font();
            style.opacity = this->get_effective_opacity();
            style.icon_style = this->get_effective_icon_style();
            style.border_color = style.foreground_color;

            return style;
        }

        /**
         * @brief Get theme-specific foreground color
         * @return Menu item foreground color from theme (state-dependent)
         * @details Uses menu_item-specific states (normal/highlighted/disabled)
         */
        [[nodiscard]] typename Backend::color_type get_theme_foreground_color(const theme_type& theme) const override {
            // Map menu_item states to visual_state
            if (!this->is_enabled()) {
                return theme.menu_item.disabled.foreground;
            }
            if (this->is_hovered() || this->has_focus()) {
                return theme.menu_item.highlighted.foreground;
            }
            return theme.menu_item.normal.foreground;
        }

        /**
         * @brief Get theme-specific background color
         * @return Menu item background color from theme (state-dependent)
         * @details Uses menu_item-specific states (normal/highlighted/disabled)
         */
        [[nodiscard]] typename Backend::color_type get_theme_background_color(const theme_type& theme) const override {
            // Map menu_item states to visual_state
            typename Backend::color_type bg;
            if (!this->is_enabled()) {
                bg = theme.menu_item.disabled.background;
                std::cerr << "[menu_item::get_theme_background_color] DISABLED bg=("
                          << static_cast<int>(bg.r) << "," << static_cast<int>(bg.g) << "," << static_cast<int>(bg.b) << ")" << std::endl;
            } else if (this->is_hovered() || this->has_focus()) {
                bg = theme.menu_item.highlighted.background;
                std::cerr << "[menu_item::get_theme_background_color] HIGHLIGHTED bg=("
                          << static_cast<int>(bg.r) << "," << static_cast<int>(bg.g) << "," << static_cast<int>(bg.b) << ")" << std::endl;
            } else {
                bg = theme.menu_item.normal.background;
                std::cerr << "[menu_item::get_theme_background_color] NORMAL bg=("
                          << static_cast<int>(bg.r) << "," << static_cast<int>(bg.g) << "," << static_cast<int>(bg.b) << ")" << std::endl;
            }
            return bg;
        }

        /**
         * @brief Apply theme to menu item
         */
        void do_apply_theme([[maybe_unused]] const theme_type& theme) override {
            // Reparse mnemonic if we have one
            if (m_has_mnemonic) {
                m_has_mnemonic = false;  // Clear until set_mnemonic_text is called again
            }
            // Invalidate measurement because font changes affect text size
            this->invalidate_measure();
        }

        /**
         * @brief Handle mouse enter (hover)
         *
         * @details
         * Mouse hover sets focus (DOS-style menu behavior).
         * This ensures keyboard navigation works from the hovered position.
         */
        bool handle_mouse_enter() override {
            // Mouse hover also sets focus (DOS menu behavior)
            auto* input = ui_services<Backend>::input();
            if (input && this->is_focusable() && this->is_enabled() && !is_separator()) {
                input->set_focus(this);
            }

            return base::handle_mouse_enter();  // Sets hover state
        }

        /**
         * @brief Handle mouse down (click)
         *
         * @details
         * Clicking a menu item triggers its action.
         * Disabled items and separators don't respond to clicks.
         */
        bool handle_mouse_down(int x, int y, int button) override {
            // Call base implementation first (sets pressed state and emits clicked signal)
            return base::handle_mouse_down(x, y, button);
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
