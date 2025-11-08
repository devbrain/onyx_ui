/**
 * @file window.inl
 * @brief Implementation of window class
 */

#pragma once

#include "window_title_bar.hh"
#include "window_content_area.hh"
#include <onyxui/services/ui_services.hh>

namespace onyxui {

    template<UIBackend Backend>
    window<Backend>::window(
        std::string title,
        window_flags flags,
        ui_element<Backend>* parent
    )
        : base(parent)
        , m_title(std::move(title))
        , m_flags(flags)
    {
        // Phase 1: Create title bar and content area
        // (Drag/resize implementation comes in later phases)

        if (m_flags.has_title_bar) {
            m_title_bar = std::make_unique<window_title_bar<Backend>>(
                m_title,
                m_flags,
                this
            );
            this->add_child(m_title_bar.get());
        }

        m_content_area = std::make_unique<window_content_area<Backend>>(
            m_flags.is_scrollable,
            this
        );
        this->add_child(m_content_area.get());

        // Register with window_manager (if available)
        register_with_window_manager();
    }

    template<UIBackend Backend>
    window<Backend>::~window() {
        // Unregister from window_manager
        unregister_from_window_manager();
    }

    template<UIBackend Backend>
    void window<Backend>::minimize() {
        if (m_state == window_state::minimized) {
            return;  // Already minimized
        }

        // Save current bounds for restore
        m_before_minimize = this->bounds();

        // Change state
        auto old_state = m_state;
        m_state = window_state::minimized;

        // Hide window (Phase 1: simple implementation)
        this->set_visible(false);

        // Emit signal
        minimized_sig.emit();

        // TODO Phase 4: Notify window_manager
    }

    template<UIBackend Backend>
    void window<Backend>::maximize() {
        if (m_state == window_state::maximized) {
            return;  // Already maximized
        }

        // Save normal bounds for restore
        if (m_state == window_state::normal) {
            m_normal_bounds = this->bounds();
        }

        // Change state
        m_state = window_state::maximized;

        // TODO Phase 4: Fill parent container
        // For Phase 1: just change state, actual resizing comes later

        // Emit signal
        maximized_sig.emit();
    }

    template<UIBackend Backend>
    void window<Backend>::restore() {
        if (m_state == window_state::normal) {
            return;  // Already in normal state
        }

        auto old_state = m_state;
        m_state = window_state::normal;

        if (old_state == window_state::minimized) {
            // Restore visibility
            this->set_visible(true);

            // Restore bounds
            if (m_before_minimize.width > 0 && m_before_minimize.height > 0) {
                this->set_bounds(m_before_minimize);
            }
        } else if (old_state == window_state::maximized) {
            // Restore to normal bounds
            if (m_normal_bounds.width > 0 && m_normal_bounds.height > 0) {
                this->set_bounds(m_normal_bounds);
            }
        }

        // Emit signal
        restored_sig.emit();
    }

    template<UIBackend Backend>
    void window<Backend>::close() {
        // Emit closing signal (can be cancelled in future)
        closing.emit();

        // Hide window
        hide();

        // Emit closed signal
        closed.emit();
    }

    template<UIBackend Backend>
    void window<Backend>::set_position(int x, int y) {
        auto current_bounds = this->bounds();
        current_bounds.x = x;
        current_bounds.y = y;
        this->set_bounds(current_bounds);

        moved.emit();
    }

    template<UIBackend Backend>
    void window<Backend>::set_size(int width, int height) {
        auto current_bounds = this->bounds();
        current_bounds.width = width;
        current_bounds.height = height;
        this->set_bounds(current_bounds);
        this->invalidate_measure();

        resized_sig.emit();
    }

    template<UIBackend Backend>
    void window<Backend>::set_content(std::unique_ptr<ui_element<Backend>> content) {
        if (m_content_area) {
            m_content_area->set_content(std::move(content));
        }
    }

    template<UIBackend Backend>
    ui_element<Backend>* window<Backend>::get_content() noexcept {
        return m_content_area ? m_content_area->get_content() : nullptr;
    }

    template<UIBackend Backend>
    void window<Backend>::set_title(const std::string& title) {
        if (m_title != title) {
            m_title = title;

            if (m_title_bar) {
                m_title_bar->set_title(title);
            }
        }
    }

    template<UIBackend Backend>
    void window<Backend>::show() {
        // TODO Phase 5: Integrate with layer_manager
        // For Phase 1: just make visible
        this->set_visible(true);
    }

    template<UIBackend Backend>
    void window<Backend>::show_modal() {
        // TODO Phase 5: Integrate with layer_manager modal layer
        // For Phase 1: same as show()
        show();
    }

    template<UIBackend Backend>
    void window<Backend>::hide() {
        // TODO Phase 5: Remove from layer_manager
        // For Phase 1: just make invisible
        this->set_visible(false);
    }

    template<UIBackend Backend>
    void window<Backend>::do_render(render_context_type& ctx) const {
        if (!this->is_visible()) {
            return;
        }

        // Phase 1: Basic rendering
        // Draw window border (if window has border enabled)
        if (this->m_has_border) {
            auto style = ctx.style();
            // TODO Phase 8: Use window theme style (focused/unfocused)
            // For Phase 1: use default border style
            auto border_style = style.button.box_style;
            ctx.draw_rect(this->bounds(), border_style);
        }

        // Children (title bar and content area) render automatically via framework
    }

    template<UIBackend Backend>
    bool window<Backend>::handle_event(const ui_event& event, event_phase phase) {
        // Phase 1: No event handling yet (drag/resize comes in Phase 2-3)
        // Just pass to base class
        return base::handle_event(event, phase);
    }

    template<UIBackend Backend>
    void window<Backend>::register_with_window_manager() {
        // TODO Phase 4: Register with window_manager service
        // For Phase 1: window_manager doesn't exist yet
        /*
        auto* mgr = ui_services<Backend>::window_manager();
        if (mgr) {
            mgr->register_window(this);
        }
        */
    }

    template<UIBackend Backend>
    void window<Backend>::unregister_from_window_manager() {
        // TODO Phase 4: Unregister from window_manager service
        // For Phase 1: window_manager doesn't exist yet
        /*
        auto* mgr = ui_services<Backend>::window_manager();
        if (mgr) {
            mgr->unregister_window(this);
        }
        */
    }

} // namespace onyxui
