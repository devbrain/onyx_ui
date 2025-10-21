/**
 * @file panel.hh
 * @brief Container widget with optional border
 * @author igor
 * @date 16/10/2025
 */

#pragma once

#include <onyxui/widgets/widget.hh>

namespace onyxui {
    /**
     * @class panel
     * @brief Container widget for grouping other widgets
     *
     * @tparam Backend The UI backend type
     *
     * @details
     * A panel is a container widget that can display a background and
     * optional border. It's primarily used for visual grouping and layout.
     *
     * ## Features
     *
     * - Optional border drawing
     * - Background fill
     * - Automatic size based on children (via layout strategy)
     * - Can be used with any layout strategy
     *
     * @example Simple Panel
     * @code
     * auto container = std::make_unique<panel<Backend>>();
     * container->set_has_border(true);
     * container->set_layout_strategy(
     *     std::make_unique<linear_layout<Backend>>(direction::vertical, 5));
     * container->add_child(create_label("Item 1"));
     * container->add_child(create_label("Item 2"));
     * @endcode
     *
     * @example Panel as Group Box
     * @code
     * auto group = std::make_unique<panel<Backend>>();
     * group->set_has_border(true);
     * group->set_padding({10, 10, 10, 10});
     * // Add children...
     * @endcode
     */
    template<UIBackend Backend>
    class panel : public widget<Backend> {
    public:
        using base = widget<Backend>;
        using renderer_type = typename Backend::renderer_type;
        using size_type = typename Backend::size_type;
        using box_style_type = typename renderer_type::box_style;
        using theme_type = typename base::theme_type;

        /**
         * @brief Construct a panel
         * @param parent Parent element (nullptr for none)
         */
        explicit panel(ui_element<Backend>* parent = nullptr)
            : base(parent) {
            this->set_focusable(false);  // Panels aren't focusable
        }

        /**
         * @brief Destructor
         */
        ~panel() override = default;

        // Rule of Five
        panel(const panel&) = delete;
        panel& operator=(const panel&) = delete;
        panel(panel&&) noexcept = default;
        panel& operator=(panel&&) noexcept = default;

        /**
         * @brief Set whether to draw a border
         * @param has_border True to draw border, false for borderless
         */
        void set_has_border(bool has_border) {
            if (m_has_border != has_border) {
                m_has_border = has_border;
                // Border affects content area, so invalidate layout
                this->invalidate_measure();
            }
        }

        /**
         * @brief Check if border is enabled
         */
        [[nodiscard]] bool has_border() const noexcept {
            return m_has_border;
        }

        /**
         * @brief Set the border style
         * @param style Box style from renderer
         */
        void set_border_style(box_style_type style) {
            m_border_style = style;
            // Just visual change, no layout impact
            this->invalidate_arrange();
        }

        /**
         * @brief Get the current border style
         */
        [[nodiscard]] const box_style_type& border_style() const noexcept {
            return m_border_style;
        }

    protected:
        /**
         * @brief Render the panel background and border
         *
         * @details
         * Uses render_context for both measurement and rendering.
         * During measurement: tracks border size if enabled
         * During rendering: draws the border box with theme colors
         */
        void do_render(render_context<Backend>& ctx) const override {
            auto* theme = this->get_theme();
            if (!theme) return;

            // Only draw border during rendering, not during measurement
            // (Measurement of border size is handled by get_content_size() which adds +2)
            if (m_has_border && ctx.is_rendering()) {
                const auto& bounds = this->bounds();
                ctx.draw_rect(bounds, theme->panel.box_style);
            }

            // Note: Background color is handled by the renderer when drawing the border
            // Children will be drawn on top with proper clipping
        }

        /**
         * @brief Calculate content size
         *
         * @details
         * Panels size themselves based on their children (via layout strategy)
         * plus any border space needed.
         */
        size_type get_content_size() const override {
            auto size = base::get_content_size();

            // Add space for border if present
            if (m_has_border) {
                int w = size_utils::get_width(size) + 2;  // 1px each side
                int h = size_utils::get_height(size) + 2;
                size_utils::set_size(size, w, h);
            }

            return size;
        }

        /**
         * @brief Get content area for children
         *
         * @details
         * Overrides base implementation to account for border.
         * The border is drawn around the edges, so we need to shrink
         * the content area by 1 pixel on each side if border is enabled.
         */
        typename Backend::rect_type get_content_area() const noexcept override {
            // Start with base implementation (handles margin and padding)
            auto content_area = base::get_content_area();

            // Adjust for border if present (1px on each side)
            if (m_has_border) {
                int x = rect_utils::get_x(content_area) + 1;
                int y = rect_utils::get_y(content_area) + 1;
                int w = std::max(0, rect_utils::get_width(content_area) - 2);
                int h = std::max(0, rect_utils::get_height(content_area) - 2);
                rect_utils::set_bounds(content_area, x, y, w, h);
            }

            return content_area;
        }

        /**
         * @brief Measure panel including border space
         */
        typename Backend::size_type do_measure(int available_width, int available_height) override {
            // Subtract border from available space before calling base
            // (base will subtract padding, and call layout strategy)
            int content_width = available_width;
            int content_height = available_height;

            if (m_has_border) {
                content_width = std::max(0, content_width - 2);   // 1px on each side
                content_height = std::max(0, content_height - 2);
            }

            // Call base to handle padding and layout strategy
            auto measured = base::do_measure(content_width, content_height);

            // Add border back to measured size
            if (m_has_border) {
                int w = size_utils::get_width(measured) + 2;
                int h = size_utils::get_height(measured) + 2;
                size_utils::set_size(measured, w, h);
            }

            return measured;
        }

        /**
         * @brief Apply theme to panel
         */
        void do_apply_theme([[maybe_unused]] const theme_type& theme) override {
            // Would cache theme panel style here
            // Note: Children without explicit theme will inherit via CSS-style inheritance
        }

    private:
        bool m_has_border = false;
        box_style_type m_border_style{};
    };

    // =========================================================================
    // Helper Functions
    // =========================================================================

    /**
     * @brief Add a panel widget to a parent
     * @tparam Parent Parent widget type (backend deduced automatically)
     * @tparam Args Constructor argument types (forwarded to panel constructor)
     * @param parent Parent widget to add panel to
     * @param args Arguments forwarded to panel constructor
     * @return Pointer to the created panel
     *
     * @details
     * Convenience helper that creates a panel and adds it to the parent in one call.
     * The backend type is automatically deduced from the parent.
     * All arguments are forwarded to the panel constructor.
     *
     * @example
     * @code
     * auto root = std::make_unique<panel<Backend>>();
     * auto* group = add_panel(*root);
     * group->set_has_border(true);
     * group->set_vbox_layout(5);
     * @endcode
     */
    template<typename Parent, typename... Args>
    auto* add_panel(Parent& parent, Args&&... args) {
        //using Backend = typename Parent::backend_type;
        return parent.template emplace_child<panel>(std::forward<Args>(args)...);
    }
}
