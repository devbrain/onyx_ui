/**
 * @file widget_container.hh
 * @brief Base class for container widgets with optional borders
 * @author Assistant
 * @date 2025-10-25
 *
 * @details
 * Provides common functionality for container widgets that:
 * - Contain child widgets via layout strategy
 * - Optionally have a border frame
 * - Need to account for border in measurement and layout
 *
 * This base class handles the measurement and content area logic
 * so derived classes (panel, group_box, menu) don't duplicate it.
 */

#pragma once

#include <onyxui/widgets/core/widget.hh>
#include <onyxui/core/rendering/render_context.hh>
#include <iostream>

namespace onyxui {

    /**
     * @class widget_container
     * @brief Base class for container widgets with optional border
     *
     * @tparam Backend The UI backend type
     *
     * @details
     * widget_container handles the common pattern for container widgets:
     *
     * **Measurement:**
     * 1. Layout strategy measures children
     * 2. Border size added to result (if enabled)
     *
     * **Content Area:**
     * - Base content area (handles padding)
     * - Shrunk by border thickness (if enabled)
     *
     * **Rendering:**
     * - Draws border rectangle (if enabled)
     * - Children rendered by framework
     *
     * ## Usage
     *
     * Derived classes just need to:
     * - Set `m_has_border` flag
     * - Optionally override `do_render()` for custom appearance
     *
     * @example
     * @code
     * template<UIBackend Backend>
     * class panel : public widget_container<Backend> {
     * public:
     *     void set_has_border(bool border) {
     *         if (this->m_has_border != border) {
     *             this->m_has_border = border;
     *             this->invalidate_measure();
     *         }
     *     }
     * };
     * @endcode
     */
    template<UIBackend Backend>
    class widget_container : public widget<Backend> {
    public:
        using base = widget<Backend>;
        using size_type = typename Backend::size_type;
        using rect_type = typename Backend::rect_type;
        using render_context_type = render_context<Backend>;

    protected:
        bool m_has_border = false;  ///< Whether to draw border frame

        /**
         * @brief Construct a widget container
         * @param parent Parent element (nullptr for none)
         */
        explicit widget_container(ui_element<Backend>* parent = nullptr)
            : base(parent) {
        }

        /**
         * @brief Destructor
         */
        ~widget_container() override = default;

        // Rule of Five
        widget_container(const widget_container&) = delete;
        widget_container& operator=(const widget_container&) = delete;
        widget_container(widget_container&&) noexcept = default;
        widget_container& operator=(widget_container&&) noexcept = default;

        /**
         * @brief Measure container including border
         *
         * @details
         * Lets base class (widget/ui_element) measure children via layout strategy,
         * then adds border size to the result.
         */
        size_type do_measure(int available_width, int available_height) override {
            // Let base class measure children via layout strategy
            auto measured = base::do_measure(available_width, available_height);

            // Add border frame size (1px on each side = +2 total)
            if (m_has_border) {
                int const w = size_utils::get_width(measured) + 2;
                int const h = size_utils::get_height(measured) + 2;
                size_utils::set_size(measured, w, h);
            }

            return measured;
        }

        /**
         * @brief Get content area for children (inside border + padding)
         *
         * @details
         * Border is drawn around the edge, so content area is shrunk
         * by 1px on each side when border is enabled.
         */
        rect_type get_content_area() const noexcept override {
            // Start with base implementation (handles margin and padding)
            auto content_area = base::get_content_area();

            // Shrink for border (1px on each side, inside padding)
            if (m_has_border) {
                int const x = rect_utils::get_x(content_area) + 1;
                int const y = rect_utils::get_y(content_area) + 1;
                int const w = std::max(0, rect_utils::get_width(content_area) - 2);
                int const h = std::max(0, rect_utils::get_height(content_area) - 2);
                rect_utils::set_bounds(content_area, x, y, w, h);
            }

            return content_area;
        }

        /**
         * @brief Render container border
         *
         * @details
         * Draws border rectangle if enabled. Context handles measurement/rendering.
         * Derived classes can override to customize appearance.
         */
        void do_render(render_context_type& ctx) const override {
            if (m_has_border) {
                // RELATIVE COORDINATES: Reconstruct absolute bounds from context position
                // bounds() returns RELATIVE coordinates after coordinate system refactoring
                typename Backend::rect_type absolute_bounds;
                rect_utils::make_absolute_bounds(absolute_bounds, ctx.position(), this->bounds());

                // Create box_style with border enabled
                // Extract value from wrapper (make copy), modify, then use
                typename Backend::renderer_type::box_style border_style = ctx.style().box_style.value;

                // Enable border drawing based on backend type
                // Test backends use bool draw_border, conio uses border_style enum
                if constexpr (requires { border_style.draw_border; }) {
                    border_style.draw_border = true;
                } else if constexpr (requires { border_style.style; }) {
                    // conio backend: set border style to single_line if currently none
                    using border_style_enum = decltype(border_style.style);
                    if (border_style.style == border_style_enum::none) {
                        border_style.style = border_style_enum::single_line;
                    }
                }

                // Draw border frame - context handles measurement/rendering
                ctx.draw_rect(absolute_bounds, border_style);
            }
        }
    };

} // namespace onyxui
