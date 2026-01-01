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
         * @brief Construct a widget container WITH a layout strategy (REQUIRED)
         * @param layout_strategy The layout strategy to use for positioning children
         * @param parent Parent element (nullptr for none)
         *
         * @details
         * **COMPILE-TIME ENFORCEMENT:** All widget_container subclasses MUST provide
         * a layout_strategy at construction. This prevents the "missing layout strategy"
         * bug where containers measure to {0,0} and don't render.
         *
         * @example Fixed-layout container (window)
         * @code
         * window<Backend>::window()
         *     : widget_container<Backend>(
         *         std::make_unique<linear_layout<Backend>>(direction::vertical, 0),
         *         nullptr
         *       )
         * { }
         * @endcode
         */
        explicit widget_container(
            std::unique_ptr<layout_strategy<Backend>> layout_strategy,
            ui_element<Backend>* parent = nullptr
        )
            : base(parent)
        {
            // Set layout strategy immediately - containers can NEVER exist without one
            this->set_layout_strategy(std::move(layout_strategy));
        }

        /**
         * @brief Default constructor DELETED - containers MUST have layout strategy
         *
         * @details
         * COMPILE-TIME ENFORCEMENT: Attempting to use the default constructor
         * will cause a compiler error. All containers must provide a layout_strategy.
         *
         * Without layout: containers measure to {0,0} and render nothing.
         * This deletion catches the bug at compile time instead of runtime.
         */
        widget_container() = delete;

    public:
        /**
         * @brief Destructor (public and virtual for proper polymorphic deletion)
         */
        ~widget_container() override = default;

        // Rule of Five (deleted constructors remain protected to prevent direct instantiation)
    protected:
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
        logical_size do_measure(logical_unit available_width, logical_unit available_height) override {
            // Let base class measure children via layout strategy
            auto measured = base::do_measure(available_width, available_height);

            // Add border frame size (1px on each side = +2 total)
            if (m_has_border) {
                measured.width = measured.width + logical_unit(2.0);
                measured.height = measured.height + logical_unit(2.0);
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
        logical_rect get_content_area() const noexcept override {
            // Start with base implementation (handles margin and padding)
            auto content_area = base::get_content_area();

            // Shrink for border (1px on each side, inside padding)
            if (m_has_border) {
                content_area.x = content_area.x + logical_unit(1.0);
                content_area.y = content_area.y + logical_unit(1.0);
                content_area.width = max(logical_unit(0.0), content_area.width - logical_unit(2.0));
                content_area.height = max(logical_unit(0.0), content_area.height - logical_unit(2.0));
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
                // Use context position (already DPI-scaled physical coordinates)
                // The render() method in element.hh already converts logical bounds to physical
                // via metrics.snap_rect(), so ctx.position() is physical
                auto const& pos = ctx.position();

                // Get dimensions using get_final_dims pattern (handles measurement vs rendering)
                // During measurement: available_size is {0,0}, use logical bounds as fallback
                // During rendering: available_size has physical dimensions
                auto logical_bounds = this->bounds();
                auto const [final_width, final_height] = ctx.get_final_dims(
                    logical_bounds.width.to_int(), logical_bounds.height.to_int());

                typename Backend::rect_type absolute_bounds;
                rect_utils::set_bounds(absolute_bounds,
                    point_utils::get_x(pos), point_utils::get_y(pos),
                    final_width, final_height);

                // Create box_style with border enabled
                // Extract value from wrapper (make copy), modify, then use
                typename Backend::renderer_type::box_style border_style = ctx.style().box_style.value;

                // Enable border drawing based on backend type
                // Test backends use bool draw_border, other backends use border_style enum
                if constexpr (requires { border_style.draw_border; }) {
                    border_style.draw_border = true;
                } else if constexpr (requires { border_style.style; }) {
                    // Set border style to a visible style if currently none
                    using border_style_enum = decltype(border_style.style);
                    if (border_style.style == border_style_enum::none) {
                        // Use single_line for conio, flat for graphical backends
                        if constexpr (requires { border_style_enum::single_line; }) {
                            border_style.style = border_style_enum::single_line;
                        } else if constexpr (requires { border_style_enum::flat; }) {
                            border_style.style = border_style_enum::flat;
                        }
                    }
                }

                // Draw border frame - context handles measurement/rendering
                ctx.draw_rect(absolute_bounds, border_style);
            }
        }
    };

} // namespace onyxui
