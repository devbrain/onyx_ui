/**
 * @file window_title_bar.inl
 * @brief Implementation of window_title_bar class
 */

#pragma once

#include <onyxui/widgets/label.hh>
#include <onyxui/widgets/icon.hh>
#include <onyxui/widgets/layout/spring.hh>
#include <onyxui/layout/linear_layout.hh>
#include <onyxui/layout/layout_strategy.hh>  // For size_constraint, horizontal_alignment
#include <onyxui/services/ui_services.hh>     // For theme access
#include <iostream>
#include <string>

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
        using icon_style = typename Backend::renderer_type::icon_style;

        // Get title alignment from current theme (defaults to left if no theme set)
        horizontal_alignment alignment = horizontal_alignment::left;
        if (auto* theme_registry = ui_services<Backend>::themes()) {
            if (auto* theme = theme_registry->get_current_theme()) {
                alignment = theme->window.title_alignment;
            }
        }

        // Layout order varies by alignment:
        // LEFT:   [menu] [title] [spring]         [controls]
        // CENTER: [menu] [spring] [title] [spring] [controls]
        // RIGHT:  [menu] [spring] [title]          [controls]

        // 1. Menu icon (left edge, optional)
        if (flags.has_menu_button) {
            m_menu_icon = this->template emplace_child<icon>(icon_style::menu);
        }

        // 2. Add spring BEFORE title for center/right alignment
        if (alignment == horizontal_alignment::center || alignment == horizontal_alignment::right) {
            this->template emplace_child<spring>();
        }

        // 3. Create title label (always content-sized)
        m_title_label = this->template emplace_child<label>(m_title);
        size_constraint width_constraint;
        width_constraint.policy = size_policy::content;
        m_title_label->set_width_constraint(width_constraint);

        // 4. Add spring AFTER title for left/center alignment
        if (alignment == horizontal_alignment::left || alignment == horizontal_alignment::center) {
            this->template emplace_child<spring>();
        }

        // 5. Create control icons (right edge)
        create_control_icons(flags);
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
        std::cerr << "[DEBUG] window_title_bar::do_render() called\n";
        std::cerr << "[DEBUG] is_visible=" << this->is_visible() << "\n";
        std::cerr << "[DEBUG] bounds=(" << this->bounds().x << "," << this->bounds().y << ","
                  << this->bounds().w << "," << this->bounds().h << ")\n";
        std::cerr << "[DEBUG] children count=" << this->children().size() << "\n";

        if (!this->is_visible()) {
            std::cerr << "[DEBUG] Title bar not visible, returning\n";
            return;
        }

        // Phase 8: Draw title bar background (uses theme.window.title_focused/unfocused)
        // Rendering order: do_render() is called BEFORE children by framework (element.hh:651)
        // 1. This fill_rect() draws background
        // 2. Children (label + buttons) render on top automatically
        std::cerr << "[DEBUG] Drawing title bar background\n";

        // ctx.position() already contains this widget's absolute screen position
        // bounds() contains relative bounds (for width/height)
        // make_absolute_bounds uses position for x,y and bounds for w,h
        typename Backend::rect_type absolute_bounds;
        rect_utils::make_absolute_bounds(absolute_bounds, ctx.position(), this->bounds());

        std::cerr << "[DEBUG] Title bar absolute fill rect: (" << absolute_bounds.x << "," << absolute_bounds.y << ","
                  << absolute_bounds.w << "," << absolute_bounds.h << ")\n";

        ctx.fill_rect(absolute_bounds);

        // Debug: Print bounds of each child
        std::cerr << "[DEBUG] Child widget bounds:\n";
        int child_idx = 0;
        for (const auto& child : this->children()) {
            auto child_bounds = child->bounds();
            std::string type_name = "unknown";

            // Identify child by pointer comparison (works regardless of order)
            if (child.get() == m_menu_icon) type_name = "menu_icon";
            else if (dynamic_cast<label<Backend>*>(child.get()) == m_title_label) type_name = "title_label";
            else if (dynamic_cast<spring<Backend>*>(child.get())) type_name = "spring";
            else if (child.get() == m_minimize_icon) type_name = "minimize_icon";
            else if (child.get() == m_maximize_icon) type_name = "maximize_icon";
            else if (child.get() == m_close_icon) type_name = "close_icon";

            std::cerr << "  Child[" << child_idx << "] (" << type_name << ") at ("
                      << child_bounds.x << "," << child_bounds.y
                      << ") size (" << child_bounds.w << "x" << child_bounds.h << ")"
                      << " visible=" << child->is_visible();

            // For icons, show edge positions (menu on left, controls on right)
            if (type_name.find("icon") != std::string::npos) {
                if (type_name == "menu_icon") {
                    std::cerr << " [left_edge=" << child_bounds.x << "]";
                } else {
                    int right_edge = child_bounds.x + child_bounds.w;
                    std::cerr << " [right_edge=" << right_edge << "]";
                }
            }
            std::cerr << "\n";
            child_idx++;
        }

        std::cerr << "[DEBUG] Title bar children will render automatically\n";
        // Children (label and buttons) render automatically via framework
    }

    template<UIBackend Backend>
    void window_title_bar<Backend>::create_control_icons(const window_flags& flags) {
        using icon_style = typename Backend::renderer_type::icon_style;

        // Create control icon widgets (right side of title bar)
        // Icons are NOT clickable by themselves - click handling is in handle_event()

        // Minimize icon
        if (flags.has_minimize_button) {
            m_minimize_icon = this->template emplace_child<icon>(icon_style::minimize);
        }

        // Maximize icon (starts with maximize, can toggle to restore)
        if (flags.has_maximize_button) {
            m_maximize_icon = this->template emplace_child<icon>(icon_style::maximize);
        }

        // Close icon
        if (flags.has_close_button) {
            m_close_icon = this->template emplace_child<icon>(icon_style::close_x);
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

        // Handle icon clicks (use hit_test to find which icon was clicked)
        if (mouse_evt->btn == mouse_event::button::left && mouse_evt->act == mouse_event::action::release) {
            // Check each icon widget using hit_test
            if (m_menu_icon && m_menu_icon->hit_test(mouse_evt->x, mouse_evt->y) == m_menu_icon) {
                menu_clicked.emit();
                return true;
            }
            if (m_minimize_icon && m_minimize_icon->hit_test(mouse_evt->x, mouse_evt->y) == m_minimize_icon) {
                minimize_clicked.emit();
                return true;
            }
            if (m_maximize_icon && m_maximize_icon->hit_test(mouse_evt->x, mouse_evt->y) == m_maximize_icon) {
                maximize_clicked.emit();
                return true;
            }
            if (m_close_icon && m_close_icon->hit_test(mouse_evt->x, mouse_evt->y) == m_close_icon) {
                close_clicked.emit();
                return true;
            }
        }

        // Phase 2: Handle mouse dragging (only if not clicking an icon)
        if (mouse_evt->btn == mouse_event::button::left && mouse_evt->act == mouse_event::action::press) {
            // Start dragging (unless clicking on an icon)
            bool clicking_icon = (m_menu_icon && m_menu_icon->hit_test(mouse_evt->x, mouse_evt->y) == m_menu_icon) ||
                                 (m_minimize_icon && m_minimize_icon->hit_test(mouse_evt->x, mouse_evt->y) == m_minimize_icon) ||
                                 (m_maximize_icon && m_maximize_icon->hit_test(mouse_evt->x, mouse_evt->y) == m_maximize_icon) ||
                                 (m_close_icon && m_close_icon->hit_test(mouse_evt->x, mouse_evt->y) == m_close_icon);

            if (!clicking_icon) {
                m_is_dragging = true;
                m_drag_start_x = mouse_evt->x;
                m_drag_start_y = mouse_evt->y;
                drag_started.emit();
                return true;  // Event handled
            }
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
            parent_has_focus = parent_window->has_window_focus();
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

    template<UIBackend Backend>
    void window_title_bar<Backend>::show_maximize_icon() {
        if (m_maximize_icon) {
            using icon_style = typename Backend::renderer_type::icon_style;
            m_maximize_icon->set_icon(icon_style::maximize);
        }
    }

    template<UIBackend Backend>
    void window_title_bar<Backend>::show_restore_icon() {
        if (m_maximize_icon) {
            using icon_style = typename Backend::renderer_type::icon_style;
            m_maximize_icon->set_icon(icon_style::restore);
        }
    }

} // namespace onyxui
