/**
 * @file label.hh
 * @brief Simple text label widget
 * @author igor
 * @date 16/10/2025
 */

#pragma once

#include <string>
#include <iostream>
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

            // Store raw markup for lazy parsing during render
            m_mnemonic_markup = std::string(mnemonic_text);

            // Invalidate cache (will be parsed on next render using ctx.theme())
            m_cached_theme_ptr = nullptr;

            this->invalidate_measure();  // Size may change
        }

        /**
         * @brief Get the mnemonic character
         * @return Mnemonic character (lowercase), or '\0' if none
         */
        [[nodiscard]] char get_mnemonic_char() const noexcept {
            return extract_mnemonic_char_from_markup(m_mnemonic_markup);
        }

        /**
         * @brief Check if label has a mnemonic
         * @return True if mnemonic is set
         */
        [[nodiscard]] bool has_mnemonic() const noexcept {
            return get_mnemonic_char() != '\0';
        }

    private:
        /**
         * @brief Extract mnemonic character from markup text
         * @param markup Markup string like "&Name:" or "&Password:"
         * @return Mnemonic character (lowercase), or '\0' if none
         *
         * @details
         * Finds the first non-escaped '&' and returns the next character.
         * - "&Name:" -> 'n'
         * - "&Password:" -> 'p'
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
         * @brief Render the label using render context
         *
         * @details
         * Uses the visitor pattern via render_context. This single method
         * handles BOTH rendering (draw_context) and measurement (measure_context),
         * eliminating the need for a separate get_content_size() implementation.
         */
        void do_render(render_context<Backend>& ctx) const override {
            // Use pre-resolved style from context (respects CSS inheritance!)
            auto const& fg = ctx.style().foreground_color;
            auto const& font = ctx.style().font;
            auto* theme = ctx.theme();

            // Use bounds for positioning (measure_context tracks bounding box, ignores position)
            const auto& bounds = this->bounds();
            int x = rect_utils::get_x(bounds);
            int y = rect_utils::get_y(bounds);

            // Parse mnemonic on-demand if needed (lazy initialization with cache)
            if (!m_mnemonic_markup.empty() && theme && m_cached_theme_ptr != theme) {
                // Theme changed or first render - parse mnemonic with current theme fonts
                m_mnemonic_info = parse_mnemonic<Backend>(
                    m_mnemonic_markup,
                    theme->label.font,
                    theme->label.mnemonic_font
                );
                m_cached_theme_ptr = theme;
            }

            // Render text (mnemonic segments if available, otherwise plain text)
            if (!m_mnemonic_markup.empty() && !m_mnemonic_info.text.empty()) {
                // Render styled text with mnemonic (multi-segment)
                for (const auto& segment : m_mnemonic_info.text) {
                    typename Backend::point_type const pos{x, y};
                    // Use segment-specific font, but inherited foreground color
                    auto text_size = ctx.draw_text(segment.text, pos, segment.font, fg);
                    x += size_utils::get_width(text_size);
                }
            } else {
                // Render plain text using pre-resolved style
                typename Backend::point_type const pos{x, y};
                ctx.draw_text(m_text, pos, font, fg);
            }
        }


        /**
         * @brief Get complete widget style from theme
         * @param theme Theme to extract properties from
         * @return Resolved style with label-specific theme values
         */
        [[nodiscard]] resolved_style<Backend> get_theme_style(const theme_type& theme) const override {
            return resolved_style<Backend>{
                .background_color = theme.label.background,
                .foreground_color = theme.label.text,
                .border_color = theme.label.text,  // Use text color for border
                .box_style = theme.panel.box_style,  // Labels use panel box style
                .font = theme.label.font,
                .opacity = 1.0F,
                .icon_style = std::optional<typename Backend::renderer_type::icon_style>{},
                .padding_horizontal = std::optional<int>{},  // Label has no padding
                .padding_vertical = std::optional<int>{},
                .mnemonic_font = std::make_optional(theme.label.mnemonic_font)  // Label has mnemonics
            };
        }


    private:
        std::string m_text;                       ///< Plain text without markup
        std::string m_mnemonic_markup;            ///< Raw markup like "&Name:" (empty if no mnemonic)
        mutable mnemonic_info<Backend> m_mnemonic_info;  ///< Cached parsed segments (lazy init)
        mutable const ui_theme<Backend>* m_cached_theme_ptr = nullptr;  ///< Track theme for cache invalidation
    };

    // =========================================================================
    // Helper Functions
    // =========================================================================

    /**
     * @brief Add a label widget to a parent
     * @tparam Parent Parent widget type (backend deduced automatically)
     * @tparam Args Constructor argument types (forwarded to label constructor)
     * @param parent Parent widget to add label to
     * @param args Arguments forwarded to label constructor
     * @return Pointer to the created label
     *
     * @details
     * Convenience helper that creates a label and adds it to the parent in one call.
     * The backend type is automatically deduced from the parent.
     * All arguments are forwarded to the label constructor.
     *
     * @example
     * @code
     * auto root = std::make_unique<panel<Backend>>();
     * auto* title = add_label(*root, "Hello World");
     * auto* label2 = add_label(*root, "Text", nullptr);  // With explicit parent
     * @endcode
     */
    template<typename Parent, typename... Args>
    auto* add_label(Parent& parent, Args&&... args) {
       // using Backend = typename Parent::backend_type;
        return parent.template emplace_child<label>(std::forward<Args>(args)...);
    }
}
