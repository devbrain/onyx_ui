/**
 * @file window_title_bar.inl
 * @brief Implementation of window_title_bar class
 */

#pragma once

#include <onyxui/widgets/label.hh>
#include <onyxui/widgets/button.hh>
#include <onyxui/layout/linear_layout.hh>

namespace onyxui {

    template<UIBackend Backend>
    window_title_bar<Backend>::window_title_bar(
        std::string title,
        const window_flags& flags,
        ui_element<Backend>* parent
    )
        : base(parent)
        , m_title(std::move(title))
    {
        // Use horizontal linear layout for title bar
        this->set_layout_strategy(
            std::make_unique<linear_layout<Backend>>(
                linear_layout_direction::horizontal,
                5  // 5px spacing between elements
            )
        );

        // Create title label (expands to fill available space)
        m_title_label = this->template emplace_child<label>(m_title);

        // Title label should expand
        // TODO: Add flex/stretch property when available in layout system

        // Create buttons based on flags
        create_buttons(flags);
    }

    template<UIBackend Backend>
    void window_title_bar<Backend>::set_title(const std::string& title) {
        if (m_title != title) {
            m_title = title;

            if (m_title_label) {
                m_title_label->set_text(m_title);
            }
        }
    }

    template<UIBackend Backend>
    void window_title_bar<Backend>::do_render(render_context_type& ctx) const {
        if (!this->is_visible()) {
            return;
        }

        // Phase 1: Draw title bar background
        auto style = ctx.style();
        // TODO Phase 8: Use window theme style (focused/unfocused)
        // For Phase 1: use button style as placeholder
        auto bg_color = style.button.bg_normal;
        ctx.fill_rect(this->bounds(), bg_color);

        // Children (label and buttons) render automatically via framework
    }

    template<UIBackend Backend>
    void window_title_bar<Backend>::create_buttons(const window_flags& flags) {
        // Menu button (left side, optional)
        if (flags.has_menu_button) {
            m_menu_button = this->template emplace_child<button>("[≡]");
            m_menu_button->clicked.connect([this]() {
                menu_clicked.emit();
            });
        }

        // Control buttons (right side)

        // Minimize button
        if (flags.has_minimize_button) {
            m_minimize_button = this->template emplace_child<button>("[_]");
            m_minimize_button->clicked.connect([this]() {
                minimize_clicked.emit();
            });
        }

        // Maximize button
        if (flags.has_maximize_button) {
            m_maximize_button = this->template emplace_child<button>("[□]");
            m_maximize_button->clicked.connect([this]() {
                maximize_clicked.emit();
            });
        }

        // Close button
        if (flags.has_close_button) {
            m_close_button = this->template emplace_child<button>("[X]");
            m_close_button->clicked.connect([this]() {
                close_clicked.emit();
            });
        }
    }

} // namespace onyxui
