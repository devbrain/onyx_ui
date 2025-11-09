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
        : base(
            std::make_unique<linear_layout<Backend>>(
                direction::horizontal,
                0  // No spacing - title and buttons are adjacent
            ),
            parent
          )
        , m_title(std::move(title))
    {

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

        // Phase 8: Draw title bar background (uses theme.window.title_focused/unfocused)
        // Rendering order: do_render() is called BEFORE children by framework (element.hh:651)
        // 1. This fill_rect() draws background
        // 2. Children (label + buttons) render on top automatically
        //
        // This layering works in real renderers (conio) where text draws over fills.
        // Note: test_canvas_backend may show artifacts due to simplified rendering.
        ctx.fill_rect(this->bounds());

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

    template<UIBackend Backend>
    bool window_title_bar<Backend>::handle_event(const ui_event& event, event_phase phase) {
        // Only handle events in target phase (after children have had a chance)
        if (phase != event_phase::target) {
            return base::handle_event(event, phase);
        }

        // Check if this is a mouse event
        auto* mouse_evt = std::get_if<mouse_event>(&event);
        if (!mouse_evt) {
            return base::handle_event(event, phase);
        }

        // Phase 2: Handle mouse dragging
        if (mouse_evt->btn == mouse_event::button::left && mouse_evt->act == mouse_event::action::press) {
            // Start dragging
            m_is_dragging = true;
            m_drag_start_x = mouse_evt->x;
            m_drag_start_y = mouse_evt->y;
            drag_started.emit();
            return true;  // Event handled
        }

        if (m_is_dragging && mouse_evt->act == mouse_event::action::move) {
            // Continue dragging - emit delta from start position
            int delta_x = mouse_evt->x - m_drag_start_x;
            int delta_y = mouse_evt->y - m_drag_start_y;
            dragging.emit(delta_x, delta_y);
            return true;  // Event handled
        }

        if (m_is_dragging && mouse_evt->btn == mouse_event::button::left && mouse_evt->act == mouse_event::action::release) {
            // End dragging
            m_is_dragging = false;
            drag_ended.emit();
            return true;  // Event handled
        }

        // Not handled, pass to base
        return base::handle_event(event, phase);
    }

    template<UIBackend Backend>
    resolved_style<Backend> window_title_bar<Backend>::get_theme_style(const theme_type& theme) const {
        // Check if parent is a window to determine focus state
        bool parent_has_focus = false;
        if (auto* parent_window = dynamic_cast<const window<Backend>*>(this->parent())) {
            parent_has_focus = parent_window->has_focus();
        }

        // Use focused or unfocused title bar style based on parent window focus
        const auto& title_state = parent_has_focus ? theme.window.title_focused : theme.window.title_unfocused;

        return resolved_style<Backend>{
            .background_color = title_state.background,
            .foreground_color = title_state.foreground,
            .mnemonic_foreground = title_state.mnemonic_foreground,
            .border_color = theme.border_color,
            .box_style = typename Backend::renderer_type::box_style{},  // Title bar has no border
            .font = title_state.font,
            .opacity = 1.0f,
            .icon_style = std::optional<typename Backend::renderer_type::icon_style>{},
            .padding_horizontal = std::optional<int>{},
            .padding_vertical = std::optional<int>{},
            .mnemonic_font = std::optional<typename Backend::renderer_type::font>{},
            .submenu_icon = std::optional<typename Backend::renderer_type::icon_style>{}
        };
    }

} // namespace onyxui
