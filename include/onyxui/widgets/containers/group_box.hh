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

#include <onyxui/widgets/containers/panel.hh>
#include <onyxui/layout/linear_layout.hh>
#include <cmath>
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
         * @brief Get title offset in physical pixels for rendering
         *
         * @details
         * Returns half the title height in physical pixels.
         * Used during rendering to offset the border down so title can be centered on it.
         *
         * For text backends (conio), text height is 1, so offset is 0.
         */
        [[nodiscard]] int get_title_offset_pixels() const {
            if (m_title.empty()) {
                return 0;
            }
            // Measure title text height in physical pixels
            typename Backend::renderer_type::font default_font{};
            std::string const formatted_title = " " + m_title + " ";
            auto text_size = Backend::renderer_type::measure_text(formatted_title, default_font);
            int const text_height = size_utils::get_height(text_size);
            // For text backends where height is 1, offset is 0
            // For graphical backends, offset is half the text height
            return (text_height > 1) ? (text_height / 2) : 0;
        }

        /**
         * @brief Get title offset in logical units for layout
         *
         * @details
         * Returns the title offset in logical units for use in measurement
         * and content area calculation. For text backends, returns 0.
         * For graphical backends, derives logical units from actual pixel offset.
         */
        [[nodiscard]] logical_unit get_title_offset_logical() const {
            if (m_title.empty()) {
                return logical_unit(0.0);
            }
            // Measure text height to detect backend type
            typename Backend::renderer_type::font default_font{};
            auto text_size = Backend::renderer_type::measure_text("X", default_font);
            int const text_height_px = size_utils::get_height(text_size);

            // For text backends where height is 1 physical pixel, offset is 0
            if (text_height_px <= 1) {
                return logical_unit(0.0);
            }

            // For graphical backends, derive logical units from pixel offset
            // text_height_px is roughly 1 logical unit (the font height)
            // title_offset_pixels is half the text height
            // So title_offset_logical = 0.5 logical units approximately
            // Use ceil to ensure we never underestimate the space needed
            int const title_offset_px = get_title_offset_pixels();
            double const logical_per_pixel = 1.0 / static_cast<double>(text_height_px);
            return logical_unit(std::ceil(static_cast<double>(title_offset_px) * logical_per_pixel));
        }

        /**
         * @brief Measure group box including border and title space
         *
         * @details
         * Measures children using layout strategy, then adds:
         * - Border space (handled by base class)
         * - Title offset space (for graphical backends where title is centered on border)
         */
        logical_size do_measure(logical_unit available_width, logical_unit available_height) override {
            // Measure using base class (widget_container handles border addition)
            auto measured = base::do_measure(available_width, available_height);

            // Add title offset for graphical backends (in logical units)
            logical_unit const title_offset = get_title_offset_logical();
            if (title_offset > logical_unit(0.0)) {
                measured.height = measured.height + title_offset;
            }

            return measured;
        }

        /**
         * @brief Get content area accounting for title offset
         *
         * @details
         * Returns the area for children, which starts below both the title
         * offset and the border. Height is reduced by the same offset so
         * children remain inside the bottom border.
         */
        logical_rect get_content_area() const noexcept override {
            auto content = base::get_content_area();

            // Adjust y position for title offset in logical units
            logical_unit const title_offset = get_title_offset_logical();
            if (title_offset > logical_unit(0.0)) {
                content.y = content.y + title_offset;
                content.height = max(logical_unit(0.0), content.height - title_offset);
            }

            return content;
        }

        /**
         * @brief Render the group box
         *
         * @details
         * Renders in this order:
         * 1. Border around the bounds (offset down for graphical backends)
         * 2. Title at top of widget bounds (centered on border line)
         * 3. Children (rendered by framework)
         *
         * For graphical backends (SDL++), the border is offset down by half the
         * title height so the title can be centered on the top border line while
         * staying within the widget's allocated bounds.
         */
        void do_render(render_context<Backend>& ctx) const override {
            // Get position and dimensions
            auto const& pos = ctx.position();
            int const x = point_utils::get_x(pos);
            int const y = point_utils::get_y(pos);

            auto logical_bounds = this->bounds();
            auto const [final_width, final_height] = ctx.get_final_dims(
                logical_bounds.width.to_int(), logical_bounds.height.to_int());

            // Calculate title offset in physical pixels for graphical backends
            int const title_offset = get_title_offset_pixels();

            // Draw border - offset down by title_offset for graphical backends
            // so that the title can be centered on the top border line
            if (this->m_has_border) {
                typename Backend::rect_type border_bounds;
                rect_utils::set_bounds(border_bounds,
                    x, y + title_offset,
                    final_width, final_height - title_offset);

                // Get border style from base class
                typename Backend::renderer_type::box_style border_style = ctx.style().box_style.value;

                // Enable border drawing
                if constexpr (requires { border_style.draw_border; }) {
                    border_style.draw_border = true;
                } else if constexpr (requires { border_style.style; }) {
                    using border_style_enum = decltype(border_style.style);
                    if (border_style.style == border_style_enum::none) {
                        if constexpr (requires { border_style_enum::single_line; }) {
                            border_style.style = border_style_enum::single_line;
                        } else if constexpr (requires { border_style_enum::flat; }) {
                            border_style.style = border_style_enum::flat;
                        }
                    }
                }

                ctx.draw_rect(border_bounds, border_style);
            }

            // Draw title inset into top border (if present)
            if (!m_title.empty()) {
                std::string const formatted_title = " " + m_title + " ";

                // Measure text to get its size
                auto const& font = ctx.style().font;
                auto text_size = Backend::renderer_type::measure_text(formatted_title, font);
                int const text_height = size_utils::get_height(text_size);
                int const text_width = size_utils::get_width(text_size);

                // Position title at the top of widget bounds
                // x + 1 (skip border corner) + m_title_position
                int const title_x = x + 1 + m_title_position;
                int const title_y = y;  // Title starts at widget top

                // Clear background behind title to "break" the border
                typename Backend::rect_type clear_rect;
                rect_utils::set_bounds(clear_rect, title_x, title_y, text_width, text_height);
                ctx.fill_rect(clear_rect);

                // Draw title text
                typename Backend::point_type const title_pos{title_x, title_y};
                (void)ctx.draw_text(formatted_title, title_pos);
            }
        }


    private:
        std::string m_title;              ///< Title text
        group_box_border m_border;        ///< Border appearance
        int m_title_position;             ///< Offset from left for title
    };

} // namespace onyxui
