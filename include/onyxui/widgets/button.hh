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
        void do_render(renderer_type& /*renderer*/) override {
            // Rendering implementation would go here
            // Select colors based on state

            // Example (pseudo-code):
            // auto* theme = this->m_theme;
            // if (!theme) return;
            //
            // color fg, bg;
            // if (!this->is_enabled()) {
            //     fg = theme->button.fg_disabled;
            //     bg = theme->button.bg_disabled;
            // } else if (this->is_pressed()) {
            //     fg = theme->button.fg_pressed;
            //     bg = theme->button.bg_pressed;
            // } else if (this->is_hovered() || this->is_focused()) {
            //     fg = theme->button.fg_hover;
            //     bg = theme->button.bg_hover;
            // } else {
            //     fg = theme->button.fg_normal;
            //     bg = theme->button.bg_normal;
            // }
            //
            // Draw button background
            // renderer.draw_box(this->bounds(), theme->button.box_style);
            //
            // Render text (with mnemonics if enabled)
            // if (m_has_mnemonic) {
            //     // Render styled text with multiple fonts (multi-segment)
            //     int x = this->bounds().x + theme->button.padding;
            //     int y = this->bounds().y + theme->button.padding;
            //     for (const auto& segment : m_mnemonic_info.text) {
            //         // CORRECT: Use static renderer_type::measure_text()
            //         auto text_size = renderer_type::measure_text(segment.text, segment.font);
            //         rect text_rect{x, y, text_size.w, text_size.h};
            //         renderer.draw_text(text_rect, segment.text, segment.font);
            //         x += text_size.w;  // ✓ Correct advance (not segment.text.length()!)
            //     }
            // } else {
            //     // Render plain text
            //     auto text_size = renderer_type::measure_text(m_text, theme->button.font);
            //     rect text_rect{this->bounds().x + theme->button.padding,
            //                     this->bounds().y + theme->button.padding,
            //                     text_size.w, text_size.h};
            //     renderer.draw_text(text_rect, m_text, theme->button.font);
            // }
        }

        /**
         * @brief Calculate content size based on text + padding
         *
         * @details
         * Uses renderer's static measure_text() method to correctly measure text,
         * then adds space for button padding and border.
         *
         * The renderer is the correct abstraction level for text measurement
         * since it's the component that actually draws text!
         *
         * Typical button layout:
         * ```
         * ┌─────────┐
         * │  Save  │  ← Text with padding
         * └─────────┘
         * ```
         */
        size_type get_content_size() const override {
            // Measure text correctly using renderer's static method
            typename renderer_type::font default_font{};
            auto text_size = renderer_type::measure_text(m_text, default_font);

            // Button needs space for text plus padding and border
            int width = size_utils::get_width(text_size) + 4;  // +4 for padding
            int height = 3;  // Includes border

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
}
