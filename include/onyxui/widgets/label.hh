/**
 * @file label.hh
 * @brief Simple text label widget
 * @author igor
 * @date 16/10/2025
 */

#pragma once

#include <cstddef>
#include <sstream>
#include <string>
#include <vector>
#include <onyxui/widgets/core/widget.hh>
#include <onyxui/actions/mnemonic_parser.hh>

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
     * - Single or multi-line text (honours embedded `\n`)
     * - Optional word-wrap to the layout-assigned width (`set_wrap_mode`)
     * - Automatic size calculation based on text
     * - Theme-aware colors
     *
     * @example
     * @code
     * auto title = std::make_unique<label<Backend>>("Hello World");
     * title->set_text("Updated Title");
     *
     * // Multi-line wrap inside a fixed-width dialog:
     * auto body = std::make_unique<label<Backend>>("a long message ...");
     * body->set_wrap_mode(label<Backend>::wrap_mode::word);
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
         * @brief Text-wrapping policy for the label.
         *
         * - `none` (default): the label asks for the natural width of its
         *   text and renders on a single line. Embedded `\n` characters
         *   still produce multi-line text via the underlying renderer.
         * - `word`: in `do_measure` the label soft-breaks the text at
         *   whitespace boundaries to fit within the available width
         *   passed by the parent layout. Embedded `\n` are honoured as
         *   hard line breaks; words longer than the available width are
         *   not broken (kept on their own line).
         *
         * Mnemonic-styled labels (`set_mnemonic_text`) ignore wrap_mode
         * and always render on a single line — wrapping mnemonic markup
         * isn't supported.
         */
        enum class wrap_mode {
            none,
            word,
        };

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
                m_wrap_cache_width = -1;     // text changed -> wrap is stale
                this->invalidate_measure();  // Size may change
            }
        }

        /**
         * @brief Switch the wrap policy. See @ref wrap_mode for semantics.
         *
         * @details
         * Changing the mode invalidates any cached wrap layout and
         * marks the widget for re-measurement; the next layout pass
         * will re-flow the text against the new policy.
         */
        void set_wrap_mode(wrap_mode mode) {
            if (m_wrap_mode != mode) {
                m_wrap_mode = mode;
                m_wrap_cache_width = -1;
                this->invalidate_measure();
            }
        }

        /**
         * @brief Get the current wrap policy. Default is `wrap_mode::none`.
         */
        [[nodiscard]] wrap_mode get_wrap_mode() const noexcept {
            return m_wrap_mode;
        }

        /**
         * @brief Get the current text
         */
        [[nodiscard]] const std::string& text() const noexcept {
            return m_text;
        }

        /**
         * @brief Set vertical alignment for text inside the label bounds.
         *
         * @details
         * This affects rendering only. The label still measures to its natural
         * text height; parents that want centered text must assign the label a
         * taller arranged height, usually via vertical_alignment::stretch.
         */
        void set_text_vertical_align(vertical_alignment align) {
            if (m_text_vertical_align != align) {
                m_text_vertical_align = align;
                this->mark_dirty();
            }
        }

        [[nodiscard]] vertical_alignment text_vertical_align() const noexcept {
            return m_text_vertical_align;
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

            // Store raw markup (will be parsed on-the-fly during render using ctx.theme())
            m_mnemonic_markup = std::string(mnemonic_text);

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
         * Uses the visitor pattern via render_context. For the default
         * `wrap_mode::none` and for mnemonic-styled labels this single
         * method handles BOTH rendering (draw_context) and measurement
         * (measure_context), eliminating the need for a separate
         * `get_content_size()` implementation.
         *
         * For `wrap_mode::word` the per-line layout is computed in
         * `do_measure()` (which has access to `available_width`); this
         * method then walks the cached lines and emits one `draw_text`
         * call per line, stacked vertically.
         */
        void do_render(render_context<Backend>& ctx) const override {
            // Use pre-resolved style from context (respects CSS inheritance!)
            auto const& fg = ctx.style().foreground_color;
            auto const& font = ctx.style().font;
            auto* theme = ctx.theme();

            // Use context position (absolute coordinates) for rendering
            // bounds() are now RELATIVE, but ctx.position() is absolute screen coords
            const auto& pos = ctx.position();
            int x = point_utils::get_x(pos);
            int y = point_utils::get_y(pos);

            // Render text (mnemonic segments if available, otherwise plain text)
            if (!m_mnemonic_markup.empty()) {
                // Parse mnemonic on-the-fly (no mutable state modification!)
                const auto mnemonic_info = parse_mnemonic<Backend>(
                    m_mnemonic_markup,
                    theme->label.font,
                    theme->label.mnemonic_font
                );

                if (!mnemonic_info.text.empty()) {
                    apply_vertical_text_offset(ctx, y, measured_text_height(m_text, font));
                    // Render styled text with mnemonic (multi-segment)
                    for (const auto& segment : mnemonic_info.text) {
                        typename Backend::point_type const segment_pos{x, y};
                        // Use segment-specific font, but inherited foreground color
                        auto text_size = ctx.draw_text(segment.text, segment_pos, segment.font, fg);
                        x += size_utils::get_width(text_size);
                    }
                } else {
                    // Fallback to plain text if parsing failed
                    apply_vertical_text_offset(ctx, y, measured_text_height(m_text, font));
                    typename Backend::point_type const text_pos{x, y};
                    (void)ctx.draw_text(m_text, text_pos, font, fg);
                }
            } else if (m_wrap_mode == wrap_mode::word && !m_text.empty()) {
                // Re-wrap if the assigned width has changed since the last
                // measure pass (defensive — under normal layout flow
                // do_measure runs first with the same width).
                int const avail = size_utils::get_width(ctx.available_size());
                if (avail > 0) {
                    rebuild_wrap_cache_if_stale(font, avail);
                }

                if (m_wrap_lines.empty()) {
                    // No measure yet, no width hint — render as single line.
                    apply_vertical_text_offset(ctx, y, measured_text_height(m_text, font));
                    typename Backend::point_type const text_pos{x, y};
                    (void)ctx.draw_text(m_text, text_pos, font, fg);
                } else {
                    apply_vertical_text_offset(ctx, y, measured_wrapped_text_height(font));
                    for (const auto& line : m_wrap_lines) {
                        typename Backend::point_type const line_pos{x, y};
                        auto text_size = ctx.draw_text(line, line_pos, font, fg);
                        y += size_utils::get_height(text_size);
                    }
                }
            } else {
                // Render plain text using pre-resolved style
                apply_vertical_text_offset(ctx, y, measured_text_height(m_text, font));
                typename Backend::point_type const text_pos{x, y};
                (void)ctx.draw_text(m_text, text_pos, font, fg);
            }
        }

        /**
         * @brief Width-aware measurement for `wrap_mode::word`.
         *
         * For the default `wrap_mode::none` (and the mnemonic path)
         * we defer to the base class, which uses the
         * `do_render(measure_context)` visitor pattern. For wrapped
         * labels we soft-break the text against `available_width`,
         * cache the resulting lines, and report the bounding size.
         */
        logical_size do_measure(logical_unit available_width,
                                logical_unit available_height) override {
            const bool wrappable =
                m_wrap_mode == wrap_mode::word
                && m_mnemonic_markup.empty()
                && !m_text.empty();

            if (!wrappable) {
                return base::do_measure(available_width, available_height);
            }

            const int avail = available_width.to_int();
            if (avail <= 0) {
                // No useful width hint — fall back to natural size.
                return base::do_measure(available_width, available_height);
            }

            auto* themes = ui_services<Backend>::themes();
            auto const* theme = themes ? themes->get_current_theme() : nullptr;
            if (!theme) {
                return base::do_measure(available_width, available_height);
            }

            rebuild_wrap_cache_if_stale(theme->label.font, avail);

            int max_w  = 0;
            int total_h = 0;
            for (const auto& line : m_wrap_lines) {
                // Empty lines (from consecutive '\n') still take a line of
                // height — reuse a single-space measurement so the gap is
                // visually consistent with the font.
                auto sz = renderer_type::measure_text(
                    line.empty() ? std::string_view(" ") : std::string_view(line),
                    theme->label.font);
                int const w = size_utils::get_width(sz);
                int const h = size_utils::get_height(sz);
                if (w > max_w) max_w = w;
                total_h += h;
            }
            return logical_size{
                logical_unit(static_cast<double>(max_w)),
                logical_unit(static_cast<double>(total_h))
            };
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
                .mnemonic_foreground = theme.label.text,  // Mnemonics same as text (non-stateful widget)
                .border_color = theme.label.text,  // Use text color for border
                .box_style = theme.panel.box_style,  // Labels use panel box style
                .font = theme.label.font,
                .opacity = 1.0F,
                .icon_style = std::optional<typename Backend::renderer_type::icon_style>{},
                .padding_horizontal = std::optional<int>{},  // Label has no padding
                .padding_vertical = std::optional<int>{},
                .mnemonic_font = std::make_optional(theme.label.mnemonic_font),  // Label has mnemonics
                .submenu_icon = std::optional<typename Backend::renderer_type::icon_style>{}  // Label has no submenu icons
            };
        }


    private:
        // -----------------------------------------------------------------
        // Wrap helpers
        // -----------------------------------------------------------------

        /// Rebuild m_wrap_lines for the supplied font + width if the cache
        /// is stale (text changed, wrap mode changed, or the available
        /// width differs from the cached one). Cheap re-entry: returns
        /// immediately when the cache is fresh.
        void rebuild_wrap_cache_if_stale(
            const typename renderer_type::font& font,
            int available_width) const {
            if (m_wrap_cache_width == available_width
                && m_wrap_cache_text_size == m_text.size()) {
                return;
            }
            m_wrap_lines = wrap_text(m_text, font, available_width);
            m_wrap_cache_width = available_width;
            m_wrap_cache_text_size = m_text.size();
        }

        /// Soft-break `text` at whitespace boundaries to fit
        /// `max_width`. Honours embedded `\n` as hard breaks. Words
        /// longer than max_width are kept whole on their own line —
        /// no mid-word breaking.
        static std::vector<std::string> wrap_text(
            const std::string& text,
            const typename renderer_type::font& font,
            int max_width) {
            std::vector<std::string> lines;
            std::string para;
            for (char c : text) {
                if (c == '\n') {
                    wrap_paragraph(para, font, max_width, lines);
                    para.clear();
                } else {
                    para.push_back(c);
                }
            }
            wrap_paragraph(para, font, max_width, lines);
            return lines;
        }

        static void wrap_paragraph(
            const std::string& para,
            const typename renderer_type::font& font,
            int max_width,
            std::vector<std::string>& out) {
            if (para.empty()) {
                out.emplace_back();   // preserve hard line break
                return;
            }
            std::string current;
            std::istringstream words(para);
            std::string word;
            while (words >> word) {
                std::string candidate =
                    current.empty() ? word : current + " " + word;
                auto sz = renderer_type::measure_text(candidate, font);
                if (size_utils::get_width(sz) > max_width && !current.empty()) {
                    out.push_back(current);
                    current = word;
                } else {
                    current = candidate;
                }
            }
            if (!current.empty()) out.push_back(current);
        }

        [[nodiscard]] static int measured_text_height(
            const std::string& text,
            const typename renderer_type::font& font) {
            auto const size = renderer_type::measure_text(
                text.empty() ? std::string_view(" ") : std::string_view(text),
                font);
            return size_utils::get_height(size);
        }

        [[nodiscard]] int measured_wrapped_text_height(
            const typename renderer_type::font& font) const {
            int total = 0;
            for (const auto& line : m_wrap_lines) {
                total += measured_text_height(line, font);
            }
            return total;
        }

        void apply_vertical_text_offset(
            const render_context<Backend>& ctx,
            int& y,
            int text_height) const {
            if (text_height <= 0) {
                return;
            }

            int const available_height = size_utils::get_height(ctx.available_size());
            if (available_height <= text_height) {
                return;
            }

            switch (m_text_vertical_align) {
                case vertical_alignment::center:
                    y += (available_height - text_height) / 2;
                    break;
                case vertical_alignment::bottom:
                    y += available_height - text_height;
                    break;
                case vertical_alignment::top:
                case vertical_alignment::stretch:
                    break;
            }
        }

        // -----------------------------------------------------------------
        // State
        // -----------------------------------------------------------------

        std::string m_text;                       ///< Plain text without markup
        std::string m_mnemonic_markup;            ///< Raw markup like "&Name:" (parsed on-the-fly during render)

        wrap_mode   m_wrap_mode = wrap_mode::none;
        vertical_alignment m_text_vertical_align = vertical_alignment::top;

        /// Cache of the per-line wrapped text from the most recent
        /// measure pass. Invalidated when text, wrap_mode, or the
        /// available width changes. Mutable because do_render is
        /// const and may need to refresh the cache when the assigned
        /// width drifts from what measure last saw.
        mutable std::vector<std::string> m_wrap_lines;
        mutable int  m_wrap_cache_width     = -1;
        mutable std::size_t m_wrap_cache_text_size = 0;
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

// Deferred implementation: widget tooltip needs label to be fully defined
#include <onyxui/widgets/core/widget_tooltip_impl.hh>
