/**
 * @file panel.hh
 * @brief Container widget with optional border
 * @author igor
 * @date 16/10/2025
 */

#pragma once

#include <onyxui/widgets/widget_container.hh>

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
    class panel : public widget_container<Backend> {
    public:
        using base = widget_container<Backend>;
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
            if (this->m_has_border != has_border) {
                this->m_has_border = has_border;
                // Border affects content area, so invalidate layout
                this->invalidate_measure();
            }
        }

        /**
         * @brief Check if border is enabled
         */
        [[nodiscard]] bool has_border() const noexcept {
            return this->m_has_border;
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
         * @brief Apply theme to panel
         */
        void do_apply_theme([[maybe_unused]] const theme_type& theme) override {
            // Would cache theme panel style here
            // Note: Children without explicit theme will inherit via CSS-style inheritance
        }

    private:
        box_style_type m_border_style{};  // m_has_border is in widget_container base class
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
