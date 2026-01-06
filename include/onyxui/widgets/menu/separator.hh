/**
 * @file separator.hh
 * @brief Separator widget for visual division in menus and layouts
 * @author Claude Code
 * @date 2025-10-26
 *
 * @details
 * Provides horizontal and vertical separator lines using the renderer's
 * line drawing capabilities. Commonly used in menus, toolbars, and panels.
 *
 * ## Visual Appearance
 *
 * Horizontal separator:
 * ```
 * Item 1
 * Item 2
 * ──────────  ← separator (horizontal line)
 * Item 3
 * Item 4
 * ```
 *
 * Vertical separator:
 * ```
 * Panel A │ Panel B
 *         ↑ separator (vertical line)
 * ```
 *
 * ## Usage Example
 *
 * ```cpp
 * // Menu with separators
 * auto menu = std::make_unique<menu<Backend>>();
 * menu->add_item(create_menu_item("New"));
 * menu->add_item(create_menu_item("Open"));
 * menu->add_separator();  // Horizontal line
 * menu->add_item(create_menu_item("Exit"));
 *
 * // Vertical separator in toolbar
 * auto toolbar = std::make_unique<hbox<Backend>>();
 * toolbar->add_child(create_button("Cut"));
 * toolbar->add_child(create_button("Copy"));
 * toolbar->add_child(std::make_unique<separator<Backend>>(orientation::vertical));
 * toolbar->add_child(create_button("Paste"));
 * ```
 */

#pragma once

#include <onyxui/widgets/core/widget.hh>
#include <onyxui/layout/layout_strategy.hh>
#include <onyxui/core/rendering/resolved_style.hh>
#include <onyxui/core/orientation.hh>

namespace onyxui {

    /**
     * @class separator
     * @brief Visual separator line (horizontal or vertical)
     *
     * @tparam Backend The UI backend type
     *
     * @details
     * Separator renders a thin line using the renderer's line drawing capabilities.
     * It automatically sizes itself based on orientation:
     * - **Horizontal**: height=1, width fills available space
     * - **Vertical**: width=1, height fills available space
     *
     * Line style comes from theme (configurable per-theme).
     */
    template<UIBackend Backend>
    class separator : public widget<Backend> {
    public:
        using base = widget<Backend>;
        using renderer_type = typename Backend::renderer_type;
        using line_style_type = typename renderer_type::line_style;
        using theme_type = typename base::theme_type;

        /**
         * @brief Construct a separator
         * @param orient Orientation (horizontal or vertical)
         * @param parent Parent element
         */
        explicit separator(orientation orient = orientation::horizontal, ui_element<Backend>* parent = nullptr)
            : base(parent), m_orientation(orient) {
            // Horizontal separators have fixed height, vertical separators have fixed width
            // Thickness comes from theme (with fallback for early initialization)
            apply_constraints();
        }

        /**
         * @brief Destructor
         */
        ~separator() override = default;

        // Rule of Five
        separator(const separator&) = delete;
        separator& operator=(const separator&) = delete;
        separator(separator&&) noexcept = default;
        separator& operator=(separator&&) noexcept = default;

        /**
         * @brief Set orientation
         * @param orient New orientation
         */
        void set_orientation(orientation orient) {
            if (m_orientation != orient) {
                m_orientation = orient;
                // Update size constraints using theme thickness
                apply_constraints();
                this->invalidate_measure();
            }
        }

        /**
         * @brief Get orientation
         */
        [[nodiscard]] orientation get_orientation() const noexcept {
            return m_orientation;
        }

    protected:
        /**
         * @brief Separator does NOT inherit colors from parent
         * @return false - separator has its own color from theme
         */
        [[nodiscard]] bool should_inherit_colors() const override {
            return false;  // Use separator-specific color from theme
        }

        /**
         * @brief Get separator style from theme
         * @param theme Theme to extract properties from
         * @return Resolved style with separator foreground color
         */
        [[nodiscard]] resolved_style<Backend> get_theme_style(const theme_type& theme) const override {
            return resolved_style<Backend>{
                .background_color = theme.menu.background,  // Same as menu background
                .foreground_color = theme.separator.foreground,  // Separator-specific color (black in NU8)
                .mnemonic_foreground = theme.text_fg,  // Mnemonics same as text (separator has no mnemonics)
                .border_color = theme.border_color,
                .box_style = typename Backend::renderer_type::box_style{},
                .font = theme.label.font,
                .opacity = 1.0f,
                .icon_style = std::optional<typename Backend::renderer_type::icon_style>{},
                .padding_horizontal = std::optional<int>{},
                .padding_vertical = std::optional<int>{},
                .mnemonic_font = std::optional<typename Backend::renderer_type::font>{},
                .submenu_icon = std::optional<typename Backend::renderer_type::icon_style>{}
            };
        }

        /**
         * @brief Render the separator line
         */
        void do_render(render_context<Backend>& ctx) const override {
            // Use context position (already DPI-scaled physical coordinates)
            auto const& pos = ctx.position();
            int const x = point_utils::get_x(pos);
            int const y = point_utils::get_y(pos);

            // Get dimensions using get_final_dims pattern (handles measurement vs rendering)
            auto logical_bounds = this->bounds();
            auto const [width, height] = ctx.get_final_dims(
                logical_bounds.width.to_int(), logical_bounds.height.to_int());

            typename Backend::rect_type line_rect;
            rect_utils::set_bounds(line_rect, x, y, width, height);

            // Get line style from theme (rare property accessed via ctx.theme())
            line_style_type line_style{};
            if (auto* theme = ctx.theme()) {
                line_style = theme->separator.line_style;
            }

            // Draw line based on orientation
            // Colors come from resolved style (foreground = separator color, background = menu background)
            if (m_orientation == orientation::horizontal) {
                ctx.draw_horizontal_line(line_rect, line_style);
            } else {
                ctx.draw_vertical_line(line_rect, line_style);
            }
        }


    private:
        orientation m_orientation;  ///< Line orientation

        /**
         * @brief Get separator thickness from theme
         * @return Thickness in logical units (1.0 default during construction)
         */
        [[nodiscard]] logical_unit get_thickness() const {
            auto* themes = ui_services<Backend>::themes();
            if (themes) {
                if (auto* theme = themes->get_current_theme()) {
                    return logical_unit(theme->separator.thickness);
                }
            }
            return logical_unit(1.0);  // Default during construction
        }

        /**
         * @brief Apply size constraints based on orientation and thickness
         */
        void apply_constraints() {
            auto thickness = get_thickness();
            if (m_orientation == orientation::horizontal) {
                size_constraint height_constraint;
                height_constraint.policy = size_policy::fixed;
                height_constraint.preferred_size = thickness;
                height_constraint.min_size = thickness;
                height_constraint.max_size = thickness;
                this->set_height_constraint(height_constraint);

                size_constraint width_constraint;
                width_constraint.policy = size_policy::expand;
                this->set_width_constraint(width_constraint);
            } else {
                size_constraint width_constraint;
                width_constraint.policy = size_policy::fixed;
                width_constraint.preferred_size = thickness;
                width_constraint.min_size = thickness;
                width_constraint.max_size = thickness;
                this->set_width_constraint(width_constraint);

                size_constraint height_constraint;
                height_constraint.policy = size_policy::expand;
                this->set_height_constraint(height_constraint);
            }
        }
    };

} // namespace onyxui
