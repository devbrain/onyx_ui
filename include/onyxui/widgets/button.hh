/**
 * @file button.hh
 * @brief Clickable button widget with text
 * @author igor
 * @date 16/10/2025
 */

#pragma once

#include <string>
#include <onyxui/widgets/stateful_widget.hh>
#include <onyxui/widgets/action.hh>
#include <onyxui/widgets/mnemonic_parser.hh>
#include <onyxui/layout_strategy.hh>  // For horizontal_alignment

namespace onyxui {
    /**
     * @class button
     * @brief Interactive push button widget
     *
     * @tparam Backend The UI backend type
     *
     * @details
     * A button is a focusable, clickable widget that displays text and
     * responds to user interaction with visual feedback.
     *
     * ## States
     *
     * - Normal: Default appearance
     * - Hovered: Mouse is over the button
     * - Pressed: Mouse button is held down
     * - Focused: Button has keyboard focus
     * - Disabled: Button is not interactive
     *
     * ## Signals
     *
     * - clicked: Emitted when button is clicked (press + release)
     * - All base widget signals
     *
     * @example
     * @code
     * auto save_btn = std::make_unique<button<Backend>>("Save");
     * save_btn->clicked.connect([&]() {
     *     save_document();
     * });
     * @endcode
     */
    template<UIBackend Backend>
    class button : public stateful_widget<Backend> {
    public:
        using base = stateful_widget<Backend>;
        using renderer_type = typename Backend::renderer_type;
        using size_type = typename Backend::size_type;
        using theme_type = typename base::theme_type;

        /**
         * @brief Construct a button with text
         * @param text Button label text
         * @param parent Parent element (nullptr for none)
         */
        explicit button(std::string text = "", ui_element<Backend>* parent = nullptr)
            : base(parent), m_text(std::move(text)) {
            this->set_focusable(true);  // Buttons are focusable
            this->set_accept_keys_as_click(true);  // Enter/Space triggers click
        }

        /**
         * @brief Destructor
         */
        ~button() override = default;

        // Rule of Five
        button(const button&) = delete;
        button& operator=(const button&) = delete;
        button(button&&) noexcept = default;
        button& operator=(button&&) noexcept = default;

        /**
         * @brief Set the button text
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
         * @brief Set button text with mnemonic syntax
         * @param mnemonic_text Text with "&" mnemonic markers (e.g., "&Save")
         *
         * @details
         * Parses mnemonic syntax and stores styled text for rendering.
         * The mnemonic character will be underlined/emphasized.
         *
         * **Requires:** Theme must be applied first (needs mnemonic_font)
         *
         * **Example:**
         * @code
         * button->set_mnemonic_text("&Save");   // 'S' underlined, mnemonic = 's'
         * button->set_mnemonic_text("E&xit");   // 'x' underlined, mnemonic = 'x'
         * button->set_mnemonic_text("Save && Exit");  // Literal "&", no mnemonic
         * @endcode
         */
        void set_mnemonic_text(std::string_view mnemonic_text) {
            // Store plain text for backwards compatibility
            m_text = strip_mnemonic(mnemonic_text);

            // Parse mnemonic if theme is available
            if (auto* theme = this->get_theme()) {
                m_mnemonic_info = parse_mnemonic<Backend>(
                    mnemonic_text,
                    theme->button.normal.font,
                    theme->button.mnemonic_font
                );
                m_has_mnemonic = true;
            } else {
                // No theme yet - just store plain text
                m_has_mnemonic = false;
            }

            this->invalidate_measure();  // Size may change
        }

        /**
         * @brief Get the mnemonic character
         * @return Mnemonic character (lowercase), or '\0' if none
         *
         * @details
         * Returns the keyboard shortcut character for Alt+key navigation.
         * Returns '\0' if no mnemonic was set or text has no mnemonic.
         */
        [[nodiscard]] char get_mnemonic_char() const noexcept {
            return m_has_mnemonic ? m_mnemonic_info.mnemonic_char : '\0';
        }

        /**
         * @brief Check if button has a mnemonic
         * @return True if mnemonic is set
         */
        [[nodiscard]] bool has_mnemonic() const noexcept {
            return m_has_mnemonic && m_mnemonic_info.mnemonic_char != '\0';
        }

    protected:
        /**
         * @brief Render button using render context (visitor pattern)
         *
         * @details
         * Uses the visitor pattern via render_context. This single method handles
         * BOTH rendering (draw_context) and measurement (measure_context).
         *
         * Position and size come from context, not from this->bounds(), which
         * makes the widget stateless and upholds the pure visitor pattern.
         *
         * - **Measurement**: ctx.available_size() = {0,0} → calculate natural size
         * - **Rendering**: ctx.available_size() = bounds.size → use assigned size
         */
        void do_render(render_context<Backend>& ctx) const override {
            auto* theme = this->get_theme();

            // Use default values if no theme
            int const padding_horizontal = theme ? theme->button.padding_horizontal : 2;
            int const padding_vertical = theme ? theme->button.padding_vertical : 1;
            int const border = theme ? renderer_type::get_border_thickness(theme->button.box_style) : 1;

            // Measure text size
            typename renderer_type::font const default_font{};
            auto text_size = renderer_type::measure_text(m_text, default_font);
            int const text_width = size_utils::get_width(text_size);
            int const text_height = size_utils::get_height(text_size);

            // Calculate natural size
            int const natural_width = text_width + (padding_horizontal * 2) + (border * 2);
            int const natural_height = text_height + (padding_vertical * 2) + (border * 2);

            // Get available size from context (0,0 during measurement, bounds.size during rendering)
            auto const& avail_size = ctx.available_size();
            int const avail_width = size_utils::get_width(avail_size);
            int const avail_height = size_utils::get_height(avail_size);

            // Use available size if provided, otherwise natural size
            int const final_width = (avail_width > 0) ? avail_width : natural_width;
            int const final_height = (avail_height > 0) ? avail_height : natural_height;

            // Get position from context (decoupled from element state!)
            auto const& pos = ctx.position();
            int const x = point_utils::get_x(pos);
            int const y = point_utils::get_y(pos);

            // Create button rectangle at context position with determined size
            typename Backend::rect_type button_rect;
            rect_utils::set_bounds(button_rect, x, y, final_width, final_height);

            // Use pre-resolved style from context (state already resolved during CSS phase)
            auto fg = ctx.style().foreground_color;

            // Draw button box/border using resolved style
            ctx.draw_rect(button_rect, ctx.style().box_style);

            // If no theme, skip text rendering
            if (!theme) return;

            // Calculate available space for text within button
            int const available_width = std::max(0, final_width - ((border + padding_horizontal) * 2));

            // Calculate horizontal alignment offset
            int align_offset = 0;
            switch (theme->button.text_align) {
                case horizontal_alignment::left:
                    align_offset = 0;
                    break;
                case horizontal_alignment::center:
                    align_offset = (available_width - text_width) / 2;
                    break;
                case horizontal_alignment::right:
                    align_offset = available_width - text_width;
                    break;
                case horizontal_alignment::stretch:
                    align_offset = 0;
                    break;
            }

            // Calculate text position (offset by border + padding + alignment)
            int const text_x = x + border + padding_horizontal + align_offset;
            int const text_y = y + border + padding_vertical;

            if (m_has_mnemonic && !m_mnemonic_info.text.empty()) {
                // Render styled text with multiple fonts (multi-segment)
                int segment_x = text_x;
                for (const auto& segment : m_mnemonic_info.text) {
                    typename Backend::point_type const text_pos{segment_x, text_y};
                    auto seg_size = ctx.draw_text(segment.text, text_pos, segment.font, fg);
                    segment_x += size_utils::get_width(seg_size);
                }
            } else {
                // Render plain text
                typename Backend::point_type const text_pos{text_x, text_y};
                ctx.draw_text(m_text, text_pos, this->get_state_font(theme->button), fg);
            }
        }

        /**
         * @brief Get theme-specific background color
         * @return Button background color from theme (state-dependent)
         */
        [[nodiscard]] typename Backend::color_type get_theme_background_color(const theme_type& theme) const override {
            return this->get_state_background(theme.button);
        }

        /**
         * @brief Get theme-specific foreground color
         * @return Button foreground color from theme (state-dependent)
         */
        [[nodiscard]] typename Backend::color_type get_theme_foreground_color(const theme_type& theme) const override {
            return this->get_state_foreground(theme.button);
        }

    public:
        /**
         * @brief Resolve style with state-dependent colors (NO parent color inheritance)
         *
         * @details
         * Buttons use STATE-DEPENDENT colors that should NOT be inherited from parents.
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
         * @brief Get theme-specific box style
         * @return Button box style from theme
         */
        [[nodiscard]] typename Backend::renderer_type::box_style get_theme_box_style(const theme_type& theme) const override {
            return theme.button.box_style;
        }

        /**
         * @brief Get theme-specific font
         * @return Button font from theme (normal state)
         */
        [[nodiscard]] typename Backend::renderer_type::font get_theme_font(const theme_type& theme) const override {
            return theme.button.normal.font;
        }

        /**
         * @brief Apply theme to button
         */
        void do_apply_theme([[maybe_unused]] const theme_type& theme) override {
            // Reparse mnemonic text with new theme fonts if we have mnemonic text
            // We need to reconstruct the mnemonic_info because fonts changed
            if (m_has_mnemonic) {
                // Reconstruct original mnemonic text from stored data
                // For now, if we have a mnemonic, just invalidate
                // In a real implementation, we'd store the original mnemonic text
                // or rebuild it from m_text and m_mnemonic_info.mnemonic_char
                m_has_mnemonic = false;  // Clear until set_mnemonic_text is called again
            }

            this->invalidate_arrange();  // Redraw with new theme
        }

        /**
         * @brief Handle action association
         *
         * @param action_ptr The action to associate with this button
         *
         * @details
         * Integrates the button with an action by:
         * - Syncing button text with action text
         * - Triggering action when button is clicked
         * - Updating button when action is triggered externally (e.g., hotkey)
         * - Syncing enabled state (handled by base class)
         *
         * @example
         * @code
         * auto save_action = std::make_shared<action<Backend>>();
         * save_action->set_text("Save");
         * save_action->triggered.connect([]() { save_document(); });
         *
         * auto button = std::make_unique<button<Backend>>();
         * button->set_action(save_action);  // Button text becomes "Save"
         * // Click button → triggers action → saves document
         * @endcode
         */
        void on_action_changed(std::shared_ptr<action<Backend>> action_ptr) override {
            // Call base to sync enabled state
            base::on_action_changed(action_ptr);

            if (action_ptr) {
                // Sync button text with action text
                set_text(action_ptr->text());

                // Connect action text changes to button
                this->m_action_connections.emplace_back(
                    action_ptr->text_changed,
                    [this](std::string_view new_text) {
                        this->set_text(std::string(new_text));
                    }
                );

                // Connect button click to action trigger
                this->m_action_connections.emplace_back(
                    this->clicked,
                    [weak_action = std::weak_ptr(action_ptr)]() {
                        if (auto action = weak_action.lock()) {
                            action->trigger();
                        }
                    }
                );

                // Connect action trigger to button (for visual updates when triggered externally)
                this->m_action_connections.emplace_back(
                    action_ptr->triggered,
                    [this]() {
                        // Visual feedback for external trigger (e.g., hotkey)
                        this->invalidate_arrange();
                    }
                );
            }
        }

    private:
        std::string m_text;
        mnemonic_info<Backend> m_mnemonic_info;  ///< Parsed mnemonic text with styling
        bool m_has_mnemonic = false;              ///< Whether mnemonic is active
    };

    // =========================================================================
    // Helper Functions
    // =========================================================================

    /**
     * @brief Add a button widget to a parent
     * @tparam Parent Parent widget type (backend deduced automatically)
     * @tparam Args Constructor argument types (forwarded to button constructor)
     * @param parent Parent widget to add button to
     * @param args Arguments forwarded to button constructor
     * @return Pointer to the created button
     *
     * @details
     * Convenience helper that creates a button and adds it to the parent in one call.
     * The backend type is automatically deduced from the parent.
     * All arguments are forwarded to the button constructor.
     *
     * @example
     * @code
     * auto root = std::make_unique<panel<Backend>>();
     * auto* ok_btn = add_button(*root, "OK");
     * ok_btn->clicked.connect([]() { handle_click(); });
     * auto* btn2 = add_button(*root, "Cancel", nullptr);  // With explicit parent
     * @endcode
     */
    template<typename Parent, typename... Args>
    auto* add_button(Parent& parent, Args&&... args) {
       // using Backend = typename Parent::backend_type;
        return parent.template emplace_child<button>(std::forward<Args>(args)...);
    }
}
