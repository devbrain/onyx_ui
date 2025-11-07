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
#include <memory>
#include <onyxui/widgets/core/stateful_widget.hh>
#include <onyxui/actions/action.hh>
#include <onyxui/actions/mnemonic_parser.hh>
#include <onyxui/services/ui_services.hh>

namespace onyxui {

    // Forward declaration to avoid circular dependency (Phase 3)
    template<UIBackend Backend>
    class menu;

    /**
     * @enum menu_item_type
     * @brief Type of menu item
     */
    enum class menu_item_type {
        normal,     ///< Regular menu item with text
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
            , m_item_type(menu_item_type::normal) {
            this->set_focusable(true);  // Menu items are focusable
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
                m_mnemonic_markup.clear();  // Clear mnemonic markup when setting plain text
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

            // Store raw markup (will be parsed on-the-fly during render using ctx.theme())
            m_mnemonic_markup = std::string(mnemonic_text);

            this->invalidate_measure();
        }

        /**
         * @brief Get the mnemonic character
         * @return Mnemonic character (lowercase), or '\0' if none
         */
        [[nodiscard]] char get_mnemonic_char() const noexcept {
            return extract_mnemonic_char_from_markup(m_mnemonic_markup);
        }

        /**
         * @brief Check if item has a mnemonic
         */
        [[nodiscard]] bool has_mnemonic() const noexcept {
            return get_mnemonic_char() != '\0';
        }

        /**
         * @brief Reset interaction state to normal
         *
         * @details
         * Clears hover/pressed state AND focus. Used when menu is opened
         * to prevent stale highlighting from previous sessions.
         */
        void reset_state() {
            if (this->is_enabled()) {
                // Clear hover and pressed state (event_target flags)
                this->reset_hover_and_press_state();

                // Reset interaction state to normal (stateful_widget state)
                this->set_interaction_state(base::interaction_state::normal);

                // Also clear focus - highlighting depends on both interaction_state AND focus
                // menu_item.hh line 458: if (this->is_hovered() || this->has_focus())
                if (this->has_focus()) {
                    if (auto* input = ui_services<Backend>::input()) {
                        input->clear_focus();
                    }
                }
            }
        }

        /**
         * @brief Get the item type
         */
        [[nodiscard]] menu_item_type type() const noexcept {
            return m_item_type;
        }

        /**
         * @brief Get shortcut text from associated action
         * @return Shortcut string (e.g., "Ctrl+S"), or empty if none
         */
        [[nodiscard]] std::string get_shortcut_text() const {
            if (auto action_ptr = this->get_action()) {
                const auto& shortcut_opt = action_ptr->shortcut();
                if (shortcut_opt.has_value()) {
                    return format_key_sequence(shortcut_opt.value());
                }
            }
            return "";
        }

        // ===================================================================
        // Submenu Management (Phase 3)
        // ===================================================================

        /**
         * @brief Set submenu for this item
         * @param submenu Submenu to attach (nullptr to remove)
         *
         * @details
         * Item takes ownership of submenu.
         * Automatically updates item type to submenu.
         *
         * @example
         * @code
         * auto submenu = std::make_unique<menu<Backend>>();
         * submenu->add_item(std::make_unique<menu_item<Backend>>("Recent 1"));
         * item->set_submenu(std::move(submenu));
         * @endcode
         */
        void set_submenu(std::unique_ptr<menu<Backend>> submenu) {
            m_submenu = std::move(submenu);
            m_item_type = m_submenu ? menu_item_type::submenu : menu_item_type::normal;
            this->invalidate_measure();  // Need to redraw with submenu indicator
        }

        /**
         * @brief Check if item has submenu
         * @return true if submenu attached, false otherwise
         */
        [[nodiscard]] bool has_submenu() const noexcept {
            return m_submenu != nullptr;
        }

        /**
         * @brief Get submenu pointer
         * @return Pointer to submenu, or nullptr if none
         */
        [[nodiscard]] menu<Backend>* submenu() const noexcept {
            return m_submenu.get();
        }

        /**
         * @brief Release submenu ownership
         * @return Submenu unique_ptr
         *
         * @details
         * Transfers ownership to caller.
         * Item type automatically reset to normal.
         */
        [[nodiscard]] std::unique_ptr<menu<Backend>> release_submenu() noexcept {
            m_item_type = menu_item_type::normal;
            this->invalidate_measure();
            return std::move(m_submenu);
        }

    private:
        /**
         * @brief Extract mnemonic character from markup text
         * @param markup Markup string like "&File" or "E&xit"
         * @return Mnemonic character (lowercase), or '\0' if none
         *
         * @details
         * Finds the first non-escaped '&' and returns the next character.
         * - "&File" -> 'f'
         * - "E&xit" -> 'x'
         * - "Save && Close" -> '\0' (escaped ampersand)
         */
        static char extract_mnemonic_char_from_markup(std::string_view markup) noexcept {
            for (size_t i = 0; i < markup.length(); ++i) {
                if (markup[i] == '&') {
                    if (i + 1 < markup.length()) {
                        if (markup[i + 1] == '&') {
                            // Escaped ampersand "&&" - skip both
                            ++i;
                        } else {
                            // Found mnemonic - return lowercase character
                            return static_cast<char>(std::tolower(static_cast<unsigned char>(markup[i + 1])));
                        }
                    }
                }
            }
            return '\0';
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
            // Use context position (absolute screen coords) for rendering
            const auto& pos = ctx.position();

            // Use resolved style from context (includes state-dependent font!)
            auto const& text_font = ctx.style().font;
            auto const& fg = ctx.style().foreground_color;
            auto const& mnemonic_fg = ctx.style().mnemonic_foreground;

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

            // Submenu indicator width (backend-specific icon from theme)
            int submenu_indicator_width = 0;
            if (has_submenu() && ctx.theme() && ctx.style().submenu_icon.value.has_value()) {
                auto indicator_size = renderer_type::get_icon_size(ctx.style().submenu_icon.value.value());
                submenu_indicator_width = size_utils::get_width(indicator_size) + 1;  // +1 for spacing
            }

            // Define padding constants
            constexpr int LEFT_PADDING = 2;
            constexpr int RIGHT_PADDING = 2;
            constexpr int SHORTCUT_SPACING = 2;  // Space between text and shortcut

            // Use absolute screen position from context
            int const base_x = point_utils::get_x(pos);
            int const base_y = point_utils::get_y(pos);

            // Calculate minimum width needed (includes submenu indicator if present)
            int const min_width = LEFT_PADDING + text_width +
                                  (shortcut.empty() ? 0 : SHORTCUT_SPACING + shortcut_width) +
                                  submenu_indicator_width +
                                  RIGHT_PADDING;

            // Get final width (context returns assigned width during rendering, natural width during measurement)
            int const effective_width = ctx.get_final_width(min_width);

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

            // Draw text with left padding (with mnemonic support)
            int const text_x = base_x + LEFT_PADDING;
            if (!m_mnemonic_markup.empty() && ctx.theme()) {
                // Parse mnemonic on-the-fly (no mutable state modification!)
                const auto mnemonic_info = parse_mnemonic<Backend>(
                    m_mnemonic_markup,
                    text_font,  // Normal font from resolved style
                    ctx.style().mnemonic_font.value.value_or(text_font)  // Mnemonic font from theme
                );

                if (!mnemonic_info.text.empty()) {
                    // Render styled text with mnemonics (multi-segment)
                    int segment_x = text_x;
                    for (const auto& segment : mnemonic_info.text) {
                        typename Backend::point_type const seg_pos{segment_x, base_y};
                        // Use mnemonic color for mnemonic segments, normal color for others
                        auto segment_color = segment.is_mnemonic ? mnemonic_fg.value : fg.value;
                        auto seg_size = ctx.draw_text(segment.text, seg_pos, segment.font, segment_color);
                        segment_x += size_utils::get_width(seg_size);
                    }
                } else {
                    // Fallback to plain text if parsing failed
                    typename Backend::point_type const text_pos{text_x, base_y};
                    ctx.draw_text(m_text, text_pos, text_font, fg);
                }
            } else {
                // Render plain text (no mnemonics)
                typename Backend::point_type const text_pos{text_x, base_y};
                ctx.draw_text(m_text, text_pos, text_font, fg);
            }

            // Draw submenu indicator if item has submenu (backend-specific icon from theme)
            if (has_submenu() && ctx.theme() && ctx.style().submenu_icon.value.has_value()) {
                // Position indicator to the right of shortcut (or text if no shortcut)
                int indicator_x;
                if (!shortcut.empty()) {
                    // After shortcut
                    indicator_x = base_x + effective_width - shortcut_width - RIGHT_PADDING - 2;
                } else {
                    // After text
                    indicator_x = base_x + effective_width - RIGHT_PADDING - 1;
                }
                // Draw icon at calculated position
                typename Backend::point_type const icon_pos{indicator_x, base_y};
                ctx.draw_icon(ctx.style().submenu_icon.value.value(), icon_pos);
            }

            // Draw shortcut if present (right-aligned within effective width)
            if (!shortcut.empty() && ctx.theme()) {
                int const shortcut_x = base_x + effective_width - shortcut_width - RIGHT_PADDING;
                typename Backend::point_type const shortcut_pos{shortcut_x, base_y};
                // Use shortcut color from theme (shortcuts use a dimmed color for subtlety)
                auto shortcut_color = ctx.theme()->menu_item.shortcut.foreground;
                ctx.draw_text(shortcut, shortcut_pos, typename renderer_type::font{}, shortcut_color);
            }

            // Ensure measurement includes right padding by drawing marker at rightmost edge
            // During measurement this ensures the bounding box extends to include right padding
            // During rendering this is harmless (just a space at the right edge)
            int const right_edge = base_x + effective_width - 1;
            typename Backend::point_type const right_marker{right_edge, base_y};
            ctx.draw_text(" ", right_marker, text_font, fg);
        }

        /**
         * @brief Stateful widget - does NOT inherit colors from parent
         * @return false - menu items manage their own state-based colors
         *
         * @details
         * Menu items have state-dependent colors (normal/highlighted/disabled) that
         * must NOT be overridden by parent CSS inheritance. If we inherit colors from
         * the menu container, the highlighting won't work.
         */
        [[nodiscard]] bool should_inherit_colors() const override {
            return false;  // Stateful widget - use theme colors, not parent colors
        }

        /**
         * @brief Get complete widget style from theme
         * @param theme Theme to extract properties from
         * @return Resolved style with menu_item-specific theme values
         * @details Uses menu_item-specific states (normal/highlighted/disabled)
         */
        [[nodiscard]] resolved_style<Backend> get_theme_style(const theme_type& theme) const override {
            // Determine which menu_item state to use
            typename Backend::color_type bg, fg, mnemonic_fg;
            typename Backend::renderer_type::font font;

            if (!this->is_enabled()) {
                bg = theme.menu_item.disabled.background;
                fg = theme.menu_item.disabled.foreground;
                mnemonic_fg = theme.menu_item.disabled.mnemonic_foreground;
                font = theme.menu_item.disabled.font;
            } else if (this->is_hovered() || this->has_focus()) {
                bg = theme.menu_item.highlighted.background;
                fg = theme.menu_item.highlighted.foreground;
                mnemonic_fg = theme.menu_item.highlighted.mnemonic_foreground;
                font = theme.menu_item.highlighted.font;
            } else {
                bg = theme.menu_item.normal.background;
                fg = theme.menu_item.normal.foreground;
                mnemonic_fg = theme.menu_item.normal.mnemonic_foreground;
                font = theme.menu_item.normal.font;
            }

            return resolved_style<Backend>{
                .background_color = bg,
                .foreground_color = fg,
                .mnemonic_foreground = mnemonic_fg,
                .border_color = theme.border_color,
                .box_style = theme.menu.box_style,  // Use menu's box_style
                .font = font,
                .opacity = 1.0f,
                .icon_style = std::optional<typename Backend::renderer_type::icon_style>{},
                .padding_horizontal = std::make_optional(theme.menu_item.padding_horizontal),  // Menu item has padding
                .padding_vertical = std::make_optional(theme.menu_item.padding_vertical),
                .mnemonic_font = std::make_optional(theme.menu_item.mnemonic_font),  // Menu item has mnemonics
                .submenu_icon = std::make_optional(theme.menu_item.submenu_icon)  // Menu item submenu indicator
            };
        }


        /**
         * @brief Handle mouse events (NEW Phase 4 API)
         * @param mouse Mouse event containing action, position, button, and modifiers
         * @return true if event was handled
         *
         * @details
         * Menu item uses base stateful_widget::handle_mouse() for state management.
         * The base implementation handles:
         * - Press: Sets pressed state
         * - Release: Returns to hover or normal
         *
         * Additionally, mouse hover sets focus (DOS-style menu behavior).
         * This ensures keyboard navigation works from the hovered position.
         *
         * Menu item actions are triggered via the base widget's clicked signal,
         * which is emitted automatically on press+release.
         */
        bool handle_mouse(const mouse_event& mouse) override {
            // Track hover state changes
            bool const was_hovered = this->is_hovered();

            // Use base implementation (stateful_widget handles state management)
            bool handled = base::handle_mouse(mouse);

            // Detect hover enter and set focus (DOS menu behavior)
            bool const now_hovered = this->is_hovered();
            if (now_hovered && !was_hovered) {
                auto* input = ui_services<Backend>::input();
                if (input && this->is_focusable() && this->is_enabled()) {
                    input->set_focus(this);
                }
            }

            return handled;
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
        std::string m_mnemonic_markup;            ///< Raw markup like "&Save" (parsed on-the-fly during render)
        menu_item_type m_item_type;               ///< Type of item
        std::unique_ptr<menu<Backend>> m_submenu; ///< Submenu (Phase 3) - nullptr if no submenu
    };

} // namespace onyxui
