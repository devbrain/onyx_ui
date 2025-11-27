/**
 * @file hbox.hh
 * @brief Horizontal box layout widget
 * @author igor
 * @date 16/10/2025
 *
 * @details
 * Convenience widget that arranges children horizontally in a row.
 * This is a pre-configured widget with linear_layout in horizontal mode.
 */

#pragma once

#include <onyxui/widgets/core/widget.hh>
#include <onyxui/layout/linear_layout.hh>
#include <onyxui/services/ui_services.hh>

namespace onyxui {
    /**
     * @class hbox
     * @brief Horizontal box container (row layout)
     *
     * @tparam Backend The UI backend type
     *
     * @details
     * hbox is a convenience widget that automatically uses linear_layout
     * in horizontal direction. Children are arranged left-to-right in a row.
     *
     * ## Features
     *
     * - Automatic horizontal layout
     * - Configurable spacing between children
     * - Cross-axis alignment control (vertical alignment of children)
     * - Simple, declarative API
     *
     * @example Simple Horizontal Layout
     * @code
     * auto row = std::make_unique<hbox<Backend>>(spacing::medium);
     * row->add_child(create_button("OK"));
     * row->add_child(create_button("Cancel"));
     * row->add_child(create_button("Help"));
     * @endcode
     *
     * @example Toolbar
     * @code
     * auto toolbar = std::make_unique<hbox<Backend>>(spacing::small);
     * toolbar->add_child(create_button("New"));
     * toolbar->add_child(create_button("Open"));
     * toolbar->add_child(create_button("Save"));
     * toolbar->add_child(create_spacer());  // Push next items to right
     * toolbar->add_child(create_button("Help"));
     * @endcode
     */
    template<UIBackend Backend>
    class hbox : public widget<Backend> {
    public:
        using base = widget<Backend>;

        /**
         * @brief Construct a horizontal box
         * @param spacing_value Semantic spacing between children (resolved via theme)
         * @param h_align Horizontal alignment (default: left)
         * @param v_align Vertical alignment (default: stretch to fill height)
         * @param parent Parent element (nullptr for none)
         */
        explicit hbox(
            spacing spacing_value = spacing::medium,
            horizontal_alignment h_align = horizontal_alignment::left,
            vertical_alignment v_align = vertical_alignment::stretch,
            ui_element<Backend>* parent = nullptr
        )
            : base(parent)
            , m_spacing(spacing_value)
            , m_h_align(h_align)
            , m_v_align(v_align) {
            this->set_layout_strategy(
                std::make_unique<linear_layout<Backend>>(
                    direction::horizontal, resolve_spacing(), m_h_align, m_v_align));
            this->set_focusable(false);  // Container, not focusable
        }

        /**
         * @brief Destructor
         */
        ~hbox() override = default;

        // Rule of Five
        hbox(const hbox&) = delete;
        hbox& operator=(const hbox&) = delete;
        hbox(hbox&&) noexcept = default;
        hbox& operator=(hbox&&) noexcept = default;

        /**
         * @brief Set the spacing between children
         * @param spacing_value New semantic spacing (resolved via theme)
         */
        void set_spacing(spacing spacing_value) {
            m_spacing = spacing_value;
            recreate_layout();
        }

        /**
         * @brief Set horizontal alignment for children
         * @param h_align New horizontal alignment for child elements
         */
        void set_child_h_align(horizontal_alignment h_align) {
            m_h_align = h_align;
            recreate_layout();
        }

        /**
         * @brief Set vertical alignment for children
         * @param v_align New vertical alignment for child elements
         */
        void set_child_v_align(vertical_alignment v_align) {
            m_v_align = v_align;
            recreate_layout();
        }

        /**
         * @brief Get current semantic spacing
         */
        [[nodiscard]] spacing get_spacing() const noexcept { return m_spacing; }

        /**
         * @brief Get current horizontal alignment for children
         */
        [[nodiscard]] horizontal_alignment child_h_align() const noexcept { return m_h_align; }

        /**
         * @brief Get current vertical alignment for children
         */
        [[nodiscard]] vertical_alignment child_v_align() const noexcept { return m_v_align; }

    private:
        /**
         * @brief Resolve semantic spacing to backend-specific integer via theme
         * @return Resolved spacing value in logical units
         */
        [[nodiscard]] int resolve_spacing() const {
            // Get current theme from ui_services
            auto* themes = ui_services<Backend>::themes();
            if (!themes) {
                // No theme available, use default medium spacing (1)
                return 1;
            }

            auto* theme = themes->get_current_theme();
            if (!theme) {
                // No current theme, use default medium spacing (1)
                return 1;
            }

            // Resolve spacing enum via theme
            return theme->spacing.resolve(m_spacing);
        }

        /**
         * @brief Recreate the layout strategy with current settings
         *
         * @exception std::bad_alloc If layout allocation fails
         * @note Exception safety: Strong guarantee - if allocation fails, old layout remains
         */
        void recreate_layout() {
            // Create new layout first (may throw)
            auto new_layout = std::make_unique<linear_layout<Backend>>(
                direction::horizontal, resolve_spacing(), m_h_align, m_v_align);

            // Only replace if creation succeeded (strong guarantee)
            this->set_layout_strategy(std::move(new_layout));
        }

        spacing m_spacing;
        horizontal_alignment m_h_align;
        vertical_alignment m_v_align;
    };
}
