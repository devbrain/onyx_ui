/**
 * @file vbox.hh
 * @brief Vertical box layout widget
 * @author igor
 * @date 16/10/2025
 *
 * @details
 * Convenience widget that arranges children vertically in a column.
 * This is a pre-configured widget with linear_layout in vertical mode.
 */

#pragma once

#include <onyxui/widgets/widget.hh>
#include <onyxui/layout/linear_layout.hh>

namespace onyxui {
    /**
     * @class vbox
     * @brief Vertical box container (column layout)
     *
     * @tparam Backend The UI backend type
     *
     * @details
     * vbox is a convenience widget that automatically uses linear_layout
     * in vertical direction. Children are arranged top-to-bottom in a column.
     *
     * ## Features
     *
     * - Automatic vertical layout
     * - Configurable spacing between children
     * - Cross-axis alignment control (horizontal alignment of children)
     * - Simple, declarative API
     *
     * @example Simple Vertical Layout
     * @code
     * auto column = std::make_unique<vbox<Backend>>(5);  // 5px spacing
     * column->add_child(create_label("Name:"));
     * column->add_child(create_text_input());
     * column->add_child(create_label("Email:"));
     * column->add_child(create_text_input());
     * @endcode
     *
     * @example Form Layout
     * @code
     * auto form = std::make_unique<vbox<Backend>>(10);
     * form->set_padding({20, 20, 20, 20});
     *
     * auto name_row = create_form_row("Name");
     * auto email_row = create_form_row("Email");
     * auto submit_btn = create_button("Submit");
     *
     * form->add_child(std::move(name_row));
     * form->add_child(std::move(email_row));
     * form->add_child(std::move(submit_btn));
     * @endcode
     */
    template<UIBackend Backend>
    class vbox : public widget<Backend> {
    public:
        using base = widget<Backend>;

        /**
         * @brief Construct a vertical box
         * @param spacing Space between children in pixels
         * @param h_align Horizontal alignment (default: stretch to fill width)
         * @param v_align Vertical alignment (default: top)
         * @param parent Parent element (nullptr for none)
         */
        explicit vbox(
            int spacing = 0,
            horizontal_alignment h_align = horizontal_alignment::stretch,
            vertical_alignment v_align = vertical_alignment::top,
            ui_element<Backend>* parent = nullptr
        )
            : base(parent)
            , m_spacing(spacing)
            , m_h_align(h_align)
            , m_v_align(v_align) {
            this->set_layout_strategy(
                std::make_unique<linear_layout<Backend>>(
                    direction::vertical, m_spacing, m_h_align, m_v_align));
            this->set_focusable(false);  // Container, not focusable
        }

        /**
         * @brief Destructor
         */
        ~vbox() override = default;

        // Rule of Five
        vbox(const vbox&) = delete;
        vbox& operator=(const vbox&) = delete;
        vbox(vbox&&) noexcept = default;
        vbox& operator=(vbox&&) noexcept = default;

        /**
         * @brief Set the spacing between children
         * @param spacing New spacing in pixels
         */
        void set_spacing(int spacing) {
            m_spacing = spacing;
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
         * @brief Get current spacing
         */
        [[nodiscard]] int spacing() const noexcept { return m_spacing; }

        /**
         * @brief Get current horizontal alignment for children
         */
        [[nodiscard]] horizontal_alignment child_h_align() const noexcept { return m_h_align; }

        /**
         * @brief Get current vertical alignment for children
         */
        [[nodiscard]] vertical_alignment child_v_align() const noexcept { return m_v_align; }

    protected:
        void do_apply_theme([[maybe_unused]] const typename base::theme_type& theme) override {
            // VBox is a layout container - children inherit via CSS-style inheritance
        }

    private:
        /**
         * @brief Recreate the layout strategy with current settings
         *
         * @exception std::bad_alloc If layout allocation fails
         * @note Exception safety: Strong guarantee - if allocation fails, old layout remains
         */
        void recreate_layout() {
            // Create new layout first (may throw)
            auto new_layout = std::make_unique<linear_layout<Backend>>(
                direction::vertical, m_spacing, m_h_align, m_v_align);

            // Only replace if creation succeeded (strong guarantee)
            this->set_layout_strategy(std::move(new_layout));
        }

        int m_spacing;
        horizontal_alignment m_h_align;
        vertical_alignment m_v_align;
    };
}
