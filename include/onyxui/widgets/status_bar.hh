/**
 * @file status_bar.hh
 * @brief Status bar widget for displaying information at bottom of window
 * @author igor
 * @date 16/10/2025
 *
 * @details
 * A status bar is a horizontal bar typically docked to the bottom of a window
 * that displays status information, help text, or other contextual information.
 *
 * ## Visual Appearance
 *
 * ```
 * ┌────────────────────────────────────────────────────────┐
 * │ F1 Help   Alt-X Exit           2025-10-16 14:30:00    │
 * └────────────────────────────────────────────────────────┘
 *   ^                                 ^
 *   Left text                         Right text
 * ```
 *
 * ## Features
 *
 * - **Left Text**: Typically for help text or status messages
 * - **Right Text**: Typically for clock, position info, etc.
 * - **Auto-sizing**: Expands to fill width, single line height
 * - **Themeable**: Uses status bar colors from theme
 *
 * ## Usage Example
 *
 * ```cpp
 * auto status = std::make_unique<status_bar<Backend>>();
 * status->set_left_text("F1 Help   Alt-X Exit");
 * status->set_right_text("Ready");
 *
 * // Update dynamically (e.g., in update loop)
 * status->set_right_text(get_current_time());
 * ```
 *
 * ## Typical Placement
 *
 * Status bars are usually placed at the bottom of the window using
 * anchor layout or as the last child in a vertical box layout.
 *
 * @see panel.hh For container widgets
 * @see label.hh For simple text display
 */

#pragma once

#include <onyxui/widgets/core/widget.hh>
#include <string>

namespace onyxui {

    /**
     * @class status_bar
     * @brief Status bar widget with left and right text sections
     *
     * @tparam Backend The UI backend type
     *
     * @details
     * Status bar displays two text sections: left-aligned and right-aligned.
     * It automatically fills the available width and has a fixed height of 1.
     *
     * ## Layout
     *
     * The status bar uses fill-width sizing policy and single-line height.
     * Text is truncated if it exceeds available space.
     *
     * ## Styling
     *
     * Status bars use theme colors:
     * - `theme->status_bar.text_fg` - Text foreground color
     * - `theme->status_bar.bg` - Background color
     */
    template<UIBackend Backend>
    class status_bar : public widget<Backend> {
    public:
        using base = widget<Backend>;
        using size_type = typename Backend::size_type;
        using theme_type = typename base::theme_type;

        /**
         * @brief Construct a status bar
         * @param parent Parent element
         */
        explicit status_bar(ui_element<Backend>* parent = nullptr)
            : base(parent)
            , m_left_text()
            , m_right_text() {
        }

        /**
         * @brief Destructor
         */
        ~status_bar() override = default;

        // Rule of Five
        status_bar(const status_bar&) = delete;
        status_bar& operator=(const status_bar&) = delete;
        status_bar(status_bar&&) noexcept = default;
        status_bar& operator=(status_bar&&) noexcept = default;

        /**
         * @brief Set the left text
         * @param text Text to display on the left side
         *
         * @details
         * Left text is typically used for help text, status messages,
         * or shortcut hints (e.g., "F1 Help  Alt-X Exit").
         */
        void set_left_text(std::string_view text) {
            if (m_left_text != text) {
                m_left_text = text;
                this->invalidate_arrange();  // Only visual change
            }
        }

        /**
         * @brief Get the left text
         * @return Current left text
         */
        [[nodiscard]] const std::string& get_left_text() const noexcept {
            return m_left_text;
        }

        /**
         * @brief Set the right text
         * @param text Text to display on the right side
         *
         * @details
         * Right text is typically used for time, position info,
         * or status indicators (e.g., "Ready", "Line 10:25").
         */
        void set_right_text(std::string_view text) {
            if (m_right_text != text) {
                m_right_text = text;
                this->invalidate_arrange();  // Only visual change
            }
        }

        /**
         * @brief Get the right text
         * @return Current right text
         */
        [[nodiscard]] const std::string& get_right_text() const noexcept {
            return m_right_text;
        }

        /**
         * @brief Clear both text sections
         */
        void clear() {
            m_left_text.clear();
            m_right_text.clear();
            this->invalidate_arrange();
        }

    protected:
        /**
         * @brief Calculate content size
         *
         * @details
         * Status bar always returns:
         * - Width: Fill available width (expands)
         * - Height: 1 (single line)
         */
        size_type get_content_size() const override {
            size_type size{};
            // Status bar fills width, single line height
            size_utils::set_size(size, 0, 1);  // 0 width = expand to fill
            return size;
        }

        /**
         * @brief Render the status bar
         *
         * @details
         * Renders left text on the left side and right text on the right side.
         * Background fills the entire width.
         *
         * @note Rendering implementation stub - currently empty.
         * Override in backend-specific subclass if needed.
         */
        void do_render([[maybe_unused]] render_context<Backend>& ctx) const override {
            // Rendering implementation would go here
            //
            // Pseudocode:
            // 1. Fill background with status bar color
            // 2. Draw left text at left edge using ctx.draw_text()
            // 3. Draw right text at right edge (right-aligned)
        }


    private:
        std::string m_left_text;   ///< Text displayed on left side
        std::string m_right_text;  ///< Text displayed on right side
    };

} // namespace onyxui
