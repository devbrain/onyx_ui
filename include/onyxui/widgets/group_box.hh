/**
 * @file group_box.hh
 * @brief Group box widget - bordered container with inset title
 * @author igor
 * @date 16/10/2025
 *
 * @details
 * A group box (also called frame or fieldset) is a container widget that
 * draws a border around its children with an optional title inset into
 * the top border.
 *
 * ## Visual Appearance
 *
 * ```
 * ┌─ Title ──────────────┐
 * │                      │
 * │   Child widgets      │
 * │   go here            │
 * │                      │
 * └──────────────────────┘
 * ```
 *
 * ## Features
 *
 * - **Border Styles**: Supports single-line, double-line, or custom borders
 * - **Inset Title**: Title text breaks the top border
 * - **Container**: Can hold any child widgets
 * - **Padding**: Internal padding around children
 * - **Themeable**: Border and title colors from theme
 *
 * ## Usage Example
 *
 * ```cpp
 * auto group = std::make_unique<group_box<Backend>>();
 * group->set_title("Registered To");
 *
 * auto user_label = std::make_unique<label<Backend>>();
 * user_label->set_text("John Doe");
 * group->add_child(std::move(user_label));
 *
 * auto company_label = std::make_unique<label<Backend>>();
 * company_label->set_text("Acme Corp");
 * group->add_child(std::move(company_label));
 * ```
 *
 * ## Typical Use Cases
 *
 * - Grouping related form fields
 * - Visually organizing sections of a dialog
 * - Creating panels with headers
 * - Norton Utilities-style information blocks
 *
 * @see panel.hh For basic container
 * @see label.hh For text display in group boxes
 */

#pragma once

#include <onyxui/widgets/panel.hh>
#include <onyxui/layout/linear_layout.hh>
#include <string>

namespace onyxui {

    /**
     * @enum group_box_border
     * @brief Border appearance for group boxes (high-level choice)
     *
     * @details This is separate from renderer's box_style (which is inherited
     *          from themeable). This enum selects the visual appearance,
     *          while box_style is the low-level renderer instruction.
     */
    enum class group_box_border {
        none,      ///< No border
        single,    ///< Single-line border (├─┤)
        double_,   ///< Double-line border (╠═╣)
        rounded,   ///< Rounded corners (╭─╮)
        thick      ///< Thick border
    };

    /**
     * @class group_box
     * @brief Container widget with border and inset title
     *
     * @tparam Backend The UI backend type
     *
     * @details
     * Group box extends panel to add visual grouping through borders
     * and an optional title. The title is positioned at the top-left
     * of the border, breaking the top border line.
     *
     * ## Layout
     *
     * The group box reserves space for:
     * - Border: 1 character on each side (2 total width/height)
     * - Padding: Internal spacing around children
     * - Title: Height of 1 line at top
     *
     * Children are laid out inside the bordered area using the
     * inherited layout strategy from panel.
     *
     * ## Title Positioning
     *
     * Title text is placed 2 characters from the left edge:
     * ```
     * ┌─ Title ───────
     * ```
     * Not at edge:
     * ```
     * ┌Title──────────  (wrong)
     * ```
     */
    template<UIBackend Backend>
    class group_box : public panel<Backend> {
    public:
        using base = panel<Backend>;
        using size_type = typename Backend::size_type;
        using theme_type = typename base::theme_type;

        /**
         * @brief Construct an empty group box
         * @param parent Parent element
         */
        explicit group_box(ui_element<Backend>* parent = nullptr)
            : base(parent)
            , m_title()
            , m_border(group_box_border::single)
            , m_title_position(2) {  // 2 characters from left
            // Group box always has a border - use panel's border mechanism
            this->set_has_border(true);

            // Set default vertical layout for children
            this->set_layout_strategy(
                std::make_unique<linear_layout<Backend>>(direction::vertical, 0)
            );
        }

        /**
         * @brief Destructor
         */
        ~group_box() override = default;

        // Rule of Five
        group_box(const group_box&) = delete;
        group_box& operator=(const group_box&) = delete;
        group_box(group_box&&) noexcept = default;
        group_box& operator=(group_box&&) noexcept = default;

        /**
         * @brief Set the title text
         * @param title Title to display in border
         *
         * @details
         * Title is inserted into the top border. If empty, no title is shown
         * and the top border is continuous.
         *
         * @example
         * @code
         * group->set_title("System Information");
         * group->set_title("");  // Remove title
         * @endcode
         */
        void set_title(std::string_view title) {
            if (m_title != title) {
                m_title = title;
                this->invalidate_arrange();
            }
        }

        /**
         * @brief Get the title text
         * @return Current title
         */
        [[nodiscard]] const std::string& get_title() const noexcept {
            return m_title;
        }

        /**
         * @brief Set border appearance (high-level)
         * @param border Border appearance choice
         *
         * @details This is separate from themeable::set_box_style() which
         *          sets the renderer's box_style (inheritable).
         *          This method chooses the visual appearance: single/double/rounded.
         *
         * @note Use themeable::set_box_style() for renderer-level styling.
         *       Use this method for group-box-specific appearance choice.
         */
        void set_border(group_box_border border) {
            if (m_border != border) {
                m_border = border;
                this->invalidate_arrange();
            }
        }

        /**
         * @brief Get border appearance
         * @return Current border appearance
         */
        [[nodiscard]] group_box_border get_border() const noexcept {
            return m_border;
        }

        /**
         * @brief Set title position offset from left
         * @param offset Number of characters from left edge
         *
         * @details
         * Controls where the title appears in the top border.
         * Default is 2 characters from the left.
         */
        void set_title_position(int offset) {
            if (m_title_position != offset) {
                m_title_position = offset;
                this->invalidate_arrange();
            }
        }

        /**
         * @brief Check if group box has a title
         * @return True if title is not empty
         */
        [[nodiscard]] bool has_title() const noexcept {
            return !m_title.empty();
        }

    protected:
        /**
         * @brief Measure group box including border space
         *
         * @details
         * Measures children using layout strategy, then adds border space:
         * - Width: +2 for left and right borders
         * - Height: +2 for top and bottom borders
         */
        size_type do_measure(int available_width, int available_height) override {
            // Measure using base class (widget_container handles border addition)
            // Base class chain: panel -> widget_container -> widget
            // widget_container::do_measure() already adds border size
            return base::do_measure(available_width, available_height);
        }

        /**
         * @brief Calculate content size including border
         *
         * @details
         * Adds border and title space to the content size:
         * - Width: +2 for left and right borders
         * - Height: +2 for top and bottom borders
         *
         * @note This is only used when there's no layout strategy.
         */

        /**
         * @brief Render the group box
         *
         * @details
         * Renders in this order:
         * 1. Border around the bounds
         * 2. Title inset into top border (if present)
         * 3. Children (rendered by base class)
         *
         * @note Rendering implementation stub - currently empty.
         * Override in backend-specific subclass if needed.
         */
        void do_render(render_context<Backend>& ctx) const override {
            // Call base panel to draw the border
            base::do_render(ctx);

            // TODO: Draw title inset into top border (if present)
            // This would require drawing the title text and breaking the top border
            // For now, title is just set but not rendered visually
        }

        /**
         * @brief Apply theme to group box
         */
        void do_apply_theme([[maybe_unused]] const theme_type& theme) override {
            // Apply theme to base panel
            base::do_apply_theme(theme);

            // Group box specific theming would go here
            // e.g., border_color = theme.group_box.border
            this->invalidate_arrange();
        }

    private:
        std::string m_title;              ///< Title text
        group_box_border m_border;        ///< Border appearance
        int m_title_position;             ///< Offset from left for title
    };

} // namespace onyxui
