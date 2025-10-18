/**
 * @file button.hh
 * @brief Clickable button widget with text
 * @author igor
 * @date 16/10/2025
 */

#pragma once

#include <string>
#include <onyxui/widgets/widget.hh>
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
    class button : public widget<Backend> {
    public:
        using base = widget<Backend>;
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
            if (auto* theme = this->m_theme) {
                m_mnemonic_info = parse_mnemonic<Backend>(
                    mnemonic_text,
                    theme->button.font,
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
         * @brief Render the button with current state
         */
        void do_render(renderer_type& renderer) override {
            auto* theme = this->m_theme;
            if (!theme) return;

            const auto& bounds = this->bounds();

            // Select colors based on state
            typename Backend::color_type fg, bg;
            if (!this->is_enabled()) {
                fg = theme->button.fg_disabled;
                bg = theme->button.bg_disabled;
            } else if (this->is_pressed()) {
                fg = theme->button.fg_pressed;
                bg = theme->button.bg_pressed;
            } else if (this->is_hovered() || this->has_focus()) {
                fg = theme->button.fg_hover;
                bg = theme->button.bg_hover;
            } else {
                fg = theme->button.fg_normal;
                bg = theme->button.bg_normal;
            }

            // Set colors
            renderer.set_foreground(fg);
            renderer.set_background(bg);

            // Draw button box/border
            renderer.draw_box(bounds, theme->button.box_style);

            // Calculate border thickness to offset text correctly
            int border = renderer_type::get_border_thickness(theme->button.box_style);

            // Calculate available space for text
            int available_width = rect_utils::get_width(bounds) - ((border + theme->button.padding_horizontal) * 2);
            int available_height = rect_utils::get_height(bounds) - ((border + theme->button.padding_vertical) * 2);

            // Measure text to calculate alignment offset
            typename renderer_type::font default_font{};
            auto text_size = renderer_type::measure_text(m_text, default_font);
            int text_width = size_utils::get_width(text_size);

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
                    // Stretch doesn't apply to text, treat as left
                    align_offset = 0;
                    break;
            }

            // Render text (offset by border + padding + alignment)
            int text_x = rect_utils::get_x(bounds) + border + theme->button.padding_horizontal + align_offset;
            int text_y = rect_utils::get_y(bounds) + border + theme->button.padding_vertical;

            if (m_has_mnemonic && !m_mnemonic_info.text.empty()) {
                // Render styled text with multiple fonts (multi-segment)
                int x = text_x;
                for (const auto& segment : m_mnemonic_info.text) {
                    auto seg_size = renderer_type::measure_text(segment.text, segment.font);
                    typename Backend::rect_type text_rect;
                    rect_utils::set_bounds(text_rect, x, text_y,
                                          size_utils::get_width(seg_size),
                                          size_utils::get_height(seg_size));
                    renderer.draw_text(text_rect, segment.text, segment.font);
                    x += size_utils::get_width(seg_size);
                }
            } else {
                // Render plain text
                typename Backend::rect_type text_rect;
                rect_utils::set_bounds(text_rect, text_x, text_y, available_width, available_height);
                renderer.draw_text(text_rect, m_text, theme->button.font);
            }
        }

        /**
         * @brief Calculate content size based on text + padding + border
         *
         * @details
         * Uses renderer's static measure_text() method to correctly measure text,
         * then adds space for button padding and border based on the theme's box_style.
         *
         * The renderer is the correct abstraction level for text measurement
         * since it's the component that actually draws text!
         *
         * Typical button layouts:
         * ```
         * With border (box_style != none):
         * ┌─────────┐
         * │  Save  │  ← Text with padding
         * └─────────┘
         *
         * Without border (box_style == none):
         *   Save    ← Just text with padding
         * ```
         */
        size_type get_content_size() const override {
            // Measure text correctly using renderer's static method
            typename renderer_type::font default_font{};
            auto text_size = renderer_type::measure_text(m_text, default_font);

            // Get padding and border from theme
            // Note: All measurements are in "renderer units" (characters for TUI, pixels for GUI)
            int padding_horizontal = 0;  // Horizontal padding per side (e.g., 2 = 2 chars left + 2 chars right)
            int padding_vertical = 0;    // Vertical padding per side (e.g., 1 = 1 line top + 1 line bottom)
            int border = 0;              // Border thickness per side (e.g., 1 = 1 char on each side)

            if (auto* theme = this->m_theme) {
                padding_horizontal = theme->button.padding_horizontal;  // From theme (e.g., default is 4 in theme)
                padding_vertical = theme->button.padding_vertical;      // From theme (e.g., default is 4 in theme)
                // Use renderer's static method to get border thickness for this box_style
                border = renderer_type::get_border_thickness(theme->button.box_style);
            }

            // Calculate total size: text + padding on both sides + border on both sides
            // Example: "Save" (4 chars) + padding_h=2 (2+2=4) + border=1 (1+1=2) = 10 width
            //          "Save" (1 line) + padding_v=0 (0+0=0) + border=1 (1+1=2) = 3 height
            int width = size_utils::get_width(text_size) + (padding_horizontal * 2) + (border * 2);
            int height = size_utils::get_height(text_size) + (padding_vertical * 2) + (border * 2);

            size_type size{};
            size_utils::set_size(size, width, height);
            return size;
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
        using Backend = typename Parent::backend_type;
        return parent.template emplace_child<button>(std::forward<Args>(args)...);
    }
}
