/**
 * @file label.hh
 * @brief Simple text label widget
 * @author igor
 * @date 16/10/2025
 */

#pragma once

#include <string>
#include <onyxui/widgets/widget.hh>
#include <onyxui/widgets/mnemonic_parser.hh>

namespace onyxui {
    /**
     * @class label
     * @brief Non-interactive text display widget
     *
     * @tparam Backend The UI backend type
     *
     * @details
     * A label displays static or dynamic text. It's non-focusable by default
     * and doesn't respond to clicks (though it can be made to do so).
     *
     * ## Features
     *
     * - Single or multi-line text
     * - Automatic size calculation based on text
     * - Theme-aware colors
     * - Optional text wrapping (future)
     *
     * @example
     * @code
     * auto title = std::make_unique<label<Backend>>("Hello World");
     * title->set_text("Updated Title");
     * @endcode
     */
    template<UIBackend Backend>
    class label : public widget<Backend> {
    public:
        using base = widget<Backend>;
        using renderer_type = typename Backend::renderer_type;
        using size_type = typename Backend::size_type;
        using theme_type = typename base::theme_type;

        /**
         * @brief Construct a label with text
         * @param text The text to display
         * @param parent Parent element (nullptr for none)
         */
        explicit label(std::string text = "", ui_element<Backend>* parent = nullptr)
            : base(parent), m_text(std::move(text)) {
            this->set_focusable(false);  // Labels aren't focusable by default
        }

        /**
         * @brief Destructor
         */
        ~label() override = default;

        // Rule of Five
        label(const label&) = delete;
        label& operator=(const label&) = delete;
        label(label&&) noexcept = default;
        label& operator=(label&&) noexcept = default;

        /**
         * @brief Set the label text
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
         * @brief Set label text with mnemonic syntax
         * @param mnemonic_text Text with "&" mnemonic markers (e.g., "&Name:")
         *
         * @details
         * Parses mnemonic syntax and stores styled text for rendering.
         * The mnemonic character will be underlined/emphasized.
         *
         * **Typical Usage:** Labels for form fields
         *
         * **Requires:** Theme must be applied first (needs mnemonic_font)
         *
         * @example
         * @code
         * label->set_mnemonic_text("&Name:");     // 'N' underlined, mnemonic = 'n'
         * label->set_mnemonic_text("&Password:"); // 'P' underlined, mnemonic = 'p'
         * @endcode
         */
        void set_mnemonic_text(std::string_view mnemonic_text) {
            // Store plain text for backwards compatibility
            m_text = strip_mnemonic(mnemonic_text);

            // Parse mnemonic if theme is available
            if (auto* theme = this->m_theme) {
                m_mnemonic_info = parse_mnemonic<Backend>(
                    mnemonic_text,
                    theme->label.font,
                    theme->label.mnemonic_font
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
         */
        [[nodiscard]] char get_mnemonic_char() const noexcept {
            return m_has_mnemonic ? m_mnemonic_info.mnemonic_char : '\0';
        }

        /**
         * @brief Check if label has a mnemonic
         * @return True if mnemonic is set
         */
        [[nodiscard]] bool has_mnemonic() const noexcept {
            return m_has_mnemonic && m_mnemonic_info.mnemonic_char != '\0';
        }

    protected:
        /**
         * @brief Render the label
         */
        void do_render(renderer_type& /*renderer*/) override {
            // Rendering implementation would go here
            // In practice, would use renderer.draw_text() with theme colors

            // Example (correct pseudo-code using static measure_text):
            // auto* theme = this->m_theme;
            // if (!theme) return;
            //
            // if (m_has_mnemonic) {
            //     // Render styled text with mnemonic (multi-segment)
            //     int x = this->bounds().x;
            //     int y = this->bounds().y;
            //     for (const auto& segment : m_mnemonic_info.text) {
            //         // CORRECT: Use static renderer_type::measure_text()
            //         auto text_size = renderer_type::measure_text(segment.text, segment.font);
            //         rect text_rect{x, y, text_size.w, text_size.h};
            //         renderer.draw_text(text_rect, segment.text, segment.font);
            //         x += text_size.w;  // ✓ Correct advance (not segment.text.length()!)
            //     }
            // } else {
            //     // Render plain text
            //     renderer.draw_text(this->bounds(), m_text,
            //                       theme->label.text, theme->label.background);
            // }
        }

        /**
         * @brief Calculate content size based on text
         *
         * @details
         * Uses renderer's static measure_text() method to correctly measure text
         * without requiring a renderer instance. This handles:
         * - ✅ UTF-8 multi-byte characters correctly (→, ═, Cyrillic, emoji)
         * - ✅ Proportional fonts (when backend implements real font metrics)
         * - ✅ Accurate width calculation for layout
         *
         * The renderer is the correct abstraction level for text measurement
         * since it's the component that actually draws text!
         *
         * @note Uses default font type. Real implementations may want to
         *       store font preference or get from theme.
         */
        size_type get_content_size() const override {
            // Use static renderer::measure_text() - no renderer instance needed!
            typename renderer_type::font default_font{};
            return renderer_type::measure_text(m_text, default_font);
        }

        /**
         * @brief Apply theme to label
         */
        void do_apply_theme([[maybe_unused]] const theme_type& theme) override {
            // Reparse mnemonic if we have one (fonts may have changed)
            if (m_has_mnemonic) {
                m_has_mnemonic = false;  // Clear until set_mnemonic_text is called again
            }

            this->invalidate_arrange();  // Redraw with new colors
        }

    private:
        std::string m_text;
        mnemonic_info<Backend> m_mnemonic_info;  ///< Parsed mnemonic text with styling
        bool m_has_mnemonic = false;              ///< Whether mnemonic is active
    };
}
