/**
 * @file button.hh
 * @brief Clickable button widget with text
 * @author igor
 * @date 16/10/2025
 */

#pragma once


#include <algorithm>
#include <optional>
#include <string>
#include <onyxui/widgets/core/stateful_widget.hh>
#include <onyxui/actions/action.hh>
#include <onyxui/actions/mnemonic_parser.hh>
#include <onyxui/layout/layout_strategy.hh>
#include <onyxui/ui_constants.hh>  // For default padding values
#include <onyxui/services/ui_services.hh>  // For focus management
#include "onyxui/core/rendering/render_context.hh"

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

            // Store raw markup (will be parsed on-the-fly during render using ctx.theme())
            m_mnemonic_markup = std::string(mnemonic_text);

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
            return extract_mnemonic_char_from_markup(m_mnemonic_markup);
        }

        /**
         * @brief Check if button has a mnemonic
         * @return True if mnemonic is set
         */
        [[nodiscard]] bool has_mnemonic() const noexcept {
            return get_mnemonic_char() != '\0';
        }

        /**
         * @brief Override handle_event to request focus on mouse click
         * @param event The event to handle
         * @param phase The event routing phase
         * @return false (let base class handle the event)
         *
         * @details
         * Buttons need to request focus when clicked so that Enter key works.
         * Uses capture phase to request focus before event reaches target phase.
         */
        bool handle_event(const ui_event& event, event_phase phase) override {
            // Request focus on mouse press (capture phase, before target)
            if (auto* mouse_evt = std::get_if<mouse_event>(&event)) {
                if (mouse_evt->act == mouse_event::action::press) {
                    if (phase == event_phase::capture) {
                        auto* input = ui_services<Backend>::input();
                        if (input && this->is_focusable()) {
                            input->set_focus(this);
                        }
                        // Don't consume - let event continue to target phase for click handling
                        return false;
                    }
                }
            }

            // Let base class handle all other events
            return base::handle_event(event, phase);
        }

    private:
        /**
         * @brief Extract mnemonic character from markup text
         * @param markup Markup string like "&Save" or "E&xit"
         * @return Mnemonic character (lowercase), or '\0' if none
         *
         * @details
         * Finds the first non-escaped '&' and returns the next character.
         * - "&Save" -> 's'
         * - "E&xit" -> 'x'
         * - "Save && Exit" -> '\0' (escaped ampersand)
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
            auto* theme = ctx.theme();

            // Get padding from resolved style (with defaults)
            int const padding_horizontal = ctx.style().padding_horizontal.value
                .value_or(ui_constants::DEFAULT_BUTTON_PADDING_HORIZONTAL);
            int const padding_vertical = ctx.style().padding_vertical.value
                .value_or(ui_constants::DEFAULT_BUTTON_PADDING_VERTICAL);
            int const border = renderer_type::get_border_thickness(ctx.style().box_style);

            // Measure text size
            typename renderer_type::font const default_font{};
            auto text_size = renderer_type::measure_text(m_text, default_font);
            int const text_width = size_utils::get_width(text_size);
            int const text_height = size_utils::get_height(text_size);

            // Calculate natural size
            int const natural_width = text_width + (padding_horizontal * 2) + (border * 2);
            int const natural_height = text_height + (padding_vertical * 2) + (border * 2);

            // Get final dimensions (context returns assigned size during rendering, natural size during measurement)
            auto const [final_width, final_height] = ctx.get_final_dims(natural_width, natural_height);

            // Get position from context (decoupled from element state!)
            auto const& pos = ctx.position();
            int const x = point_utils::get_x(pos);
            int const y = point_utils::get_y(pos);

            // Create button rectangle at context position with determined size
            typename Backend::rect_type button_rect;
            rect_utils::set_bounds(button_rect, x, y, final_width, final_height);

            // Use pre-resolved style from context (state already resolved during CSS phase)
            auto fg = ctx.style().foreground_color;
            auto mnemonic_fg = ctx.style().mnemonic_foreground;

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

            // Render text (mnemonic segments if available, otherwise plain text)
            if (!m_mnemonic_markup.empty()) {
                // Parse mnemonic on-the-fly (no mutable state modification!)
                const auto mnemonic_info = parse_mnemonic<Backend>(
                    m_mnemonic_markup,
                    theme->button.normal.font,
                    theme->button.mnemonic_font
                );

                if (!mnemonic_info.text.empty()) {
                    // Render styled text with multiple fonts and colors (multi-segment)
                    int segment_x = text_x;
                    for (const auto& segment : mnemonic_info.text) {
                        typename Backend::point_type const text_pos{segment_x, text_y};
                        // Use mnemonic color for mnemonic segments, normal color for others
                        auto segment_color = segment.is_mnemonic ? mnemonic_fg.value : fg.value;
                        auto seg_size = ctx.draw_text(segment.text, text_pos, segment.font, segment_color);
                        segment_x += size_utils::get_width(seg_size);
                    }
                } else {
                    // Fallback to plain text if parsing failed
                    typename Backend::point_type const text_pos{text_x, text_y};
                    ctx.draw_text(m_text, text_pos, this->get_state_font(theme->button), fg);
                }
            } else {
                // Render plain text
                typename Backend::point_type const text_pos{text_x, text_y};
                ctx.draw_text(m_text, text_pos, this->get_state_font(theme->button), fg);
            }

            // Draw 3D effects AFTER button content (like menu shadows)
            // Only draw when button is NOT pressed (creates raised depth effect)
            if (theme->button.shadow.enabled) {
                auto current_state = this->get_effective_state();
                if (current_state != base::interaction_state::pressed) {
                    // Draw shadow on right and bottom edges (darkens background OUTSIDE button)
                    ctx.draw_shadow(
                        button_rect,
                        theme->button.shadow.offset_x,
                        theme->button.shadow.offset_y
                    );

                    // Draw highlight on left and top edges (brightens button border area)
                    ctx.draw_highlight(
                        button_rect,
                        theme->button.shadow.offset_x,
                        theme->button.shadow.offset_y
                    );
                }
            }
        }

        /**
         * @brief Stateful widget - does NOT inherit colors from parent
         * @return false - buttons manage their own state-based colors
         *
         * @details
         * Buttons have state-dependent colors (normal/hover/pressed/disabled) that
         * must NOT be overridden by parent CSS inheritance.
         */
        [[nodiscard]] bool should_inherit_colors() const override {
            return false;  // Stateful widget - use theme colors, not parent colors
        }

        /**
         * @brief Get complete widget style from theme
         * @param theme Theme to extract properties from
         * @return Resolved style with button-specific theme values
         * @details Uses state-aware colors (normal/hover/pressed/disabled)
         */
        [[nodiscard]] resolved_style<Backend> get_theme_style(const theme_type& theme) const override {
            return resolved_style<Backend>{
                .background_color = this->get_state_background(theme.button),
                .foreground_color = this->get_state_foreground(theme.button),
                .mnemonic_foreground = this->get_state_mnemonic_foreground(theme.button),
                .border_color = theme.border_color,
                .box_style = theme.button.box_style,
                .font = theme.button.normal.font,
                .opacity = 1.0f,
                .icon_style = std::optional<typename Backend::renderer_type::icon_style>{},
                .padding_horizontal = std::make_optional(theme.button.padding_horizontal),  // Button has padding
                .padding_vertical = std::make_optional(theme.button.padding_vertical),
                .mnemonic_font = std::make_optional(theme.button.mnemonic_font),  // Button has mnemonics
                .submenu_icon = std::optional<typename Backend::renderer_type::icon_style>{}  // Button has no submenu icons
            };
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
        std::string m_text;                       ///< Plain text without markup
        std::string m_mnemonic_markup;            ///< Raw markup like "&Save" (parsed on-the-fly during render)
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
