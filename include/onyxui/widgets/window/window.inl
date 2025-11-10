/**
 * @file window.inl
 * @brief Implementation of window class
 */

#pragma once

#include <iostream>
#include "onyxui/hotkeys/hotkey_action.hh"
#include <onyxui/widgets/window/window_title_bar.hh>
#include <onyxui/widgets/window/window_content_area.hh>
#include <onyxui/widgets/window/window_system_menu.hh>
#include <onyxui/services/ui_services.hh>

namespace onyxui {

    template<UIBackend Backend>
    window<Backend>::window(
        std::string title,
        window_flags flags,
        ui_element<Backend>* parent
    )
        : base(
            std::make_unique<linear_layout<Backend>>(
                direction::vertical,
                0  // No spacing between title bar and content
            ),
            parent
          )
        , m_title(std::move(title))
        , m_flags(flags)
    {

        // Phase 1: Create title bar and content area
        // (Drag/resize implementation comes in later phases)

        if (m_flags.has_title_bar) {
            auto title_bar = std::make_unique<window_title_bar<Backend>>(
                m_title,
                m_flags,
                this
            );
            m_title_bar = title_bar.get();  // Store raw pointer before moving

            // Phase 2: Connect drag signals (if movable)
            if (m_flags.is_movable && m_title_bar) {
                m_title_bar->drag_started.connect([this]() {
                    // Save initial position when drag starts
                    m_drag_initial_bounds = this->bounds();
                });

                m_title_bar->dragging.connect([this](int delta_x, int delta_y) {
                    // Update window position during drag
                    auto new_bounds = m_drag_initial_bounds;
                    new_bounds.x += delta_x;
                    new_bounds.y += delta_y;
                    this->arrange(new_bounds);
                    moved.emit();
                });
            }

            this->add_child(std::move(title_bar));
        }

        // Phase 7: Create system menu (if menu button enabled)
        if (m_flags.has_menu_button && m_title_bar) {
            m_system_menu = std::make_unique<window_system_menu<Backend>>(this);

            // Wire up menu button to show system menu
            m_title_bar->menu_clicked.connect([this]() {
                if (m_system_menu) {
                    // Update menu states before showing
                    m_system_menu->update_menu_states();

                    // Phase 7: Position menu at icon location
                    auto* menu_icon = m_title_bar->get_menu_icon();
                    auto* menu = m_system_menu->get_menu();

                    if (menu_icon && menu) {
                        // Get icon's absolute bounds
                        auto icon_bounds = menu_icon->bounds();

                        // Position menu below icon (at icon's bottom-left corner)
                        // Note: Using relative coordinates - menu will be positioned relative to window
                        int menu_x = rect_utils::get_x(icon_bounds);
                        int menu_y = rect_utils::get_y(icon_bounds) + rect_utils::get_height(icon_bounds);

                        // Measure menu to get its size
                        auto menu_size = menu->measure(200, 300);  // Reasonable max size for menu

                        // Set menu bounds
                        typename Backend::rect_type menu_bounds;
                        rect_utils::set_bounds(
                            menu_bounds,
                            menu_x,
                            menu_y,
                            size_utils::get_width(menu_size),
                            size_utils::get_height(menu_size)
                        );

                        menu->arrange(menu_bounds);
                        menu->set_visible(true);

                        // Activate menu: reset states and focus first item
                        menu->reset_item_states();
                        menu->focus_first();
                    }
                }
            });
        }

        auto content_area = std::make_unique<window_content_area<Backend>>(
            m_flags.is_scrollable,
            this
        );
        m_content_area = content_area.get();  // Store raw pointer before moving
        this->add_child(std::move(content_area));

        // Windows always have borders (for the window frame)
        this->m_has_border = true;

        // Register with window_manager (if available)
        register_with_window_manager();
    }

    template<UIBackend Backend>
    window<Backend>::~window() {
        // Phase 5: Remove from layer_manager
        hide();

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
        m_state = window_state::minimized;

        // Hide window (Phase 1: simple implementation)
        this->set_visible(false);

        // Emit signal
        minimized_sig.emit();

        // Notify window_manager (Phase 4)
        auto* mgr = ui_services<Backend>::windows();
        if (mgr) {
            mgr->handle_minimize(this);
        }
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

        // Phase 4: Fill parent container
        // Get parent element and maximize to fill its bounds
        auto* parent_elem = this->parent();
        if (parent_elem) {
            // Get parent's bounds to fill entire parent area
            auto parent_bounds = parent_elem->bounds();

            // Window fills entire parent (positioned at 0,0 relative to parent)
            // Note: bounds are relative coordinates, so (0,0) means top-left of parent
            typename Backend::rect_type maximized_bounds;
            rect_utils::set_bounds(
                maximized_bounds,
                0,  // x: relative to parent (top-left corner)
                0,  // y: relative to parent (top-left corner)
                rect_utils::get_width(parent_bounds),
                rect_utils::get_height(parent_bounds)
            );

            // Measure and arrange to new size
            int const width = rect_utils::get_width(maximized_bounds);
            int const height = rect_utils::get_height(maximized_bounds);

            if (!this->children().empty() && ui_services<Backend>::themes()) {
                [[maybe_unused]] auto measured_size = this->measure(width, height);
            }
            this->arrange(maximized_bounds);
        }

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
            if (m_before_minimize.w > 0 && m_before_minimize.h > 0) {
                this->arrange(m_before_minimize);
            }
        } else if (old_state == window_state::maximized) {
            // Restore to normal bounds
            if (m_normal_bounds.w > 0 && m_normal_bounds.h > 0) {
                this->arrange(m_normal_bounds);
            }
        }

        // Emit signal
        restored_sig.emit();

        // Notify window_manager (Phase 4)
        auto* mgr = ui_services<Backend>::windows();
        if (mgr) {
            mgr->handle_restore(this);
        }
    }

    template<UIBackend Backend>
    void window<Backend>::close() {
        // Emit closing signal (can be cancelled in future)
        closing.emit();

        // Phase 5: Call hook for subclasses (e.g., dialog result handling)
        on_close();

        // Phase 5: Restore focus if this was a modal window
        if (m_flags.is_modal && m_previous_active_window) {
            auto* win_mgr = ui_services<Backend>::windows();
            if (win_mgr) {
                win_mgr->set_active_window(m_previous_active_window);
            }

            auto* focus_mgr = ui_services<Backend>::input();
            if (focus_mgr) {
                focus_mgr->set_focus(m_previous_active_window);
            }

            m_previous_active_window = nullptr;
        }

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
        this->arrange(current_bounds);

        moved.emit();
    }

    template<UIBackend Backend>
    void window<Backend>::set_size(int width, int height) {
        auto current_bounds = this->bounds();
        current_bounds.w = width;
        current_bounds.h = height;

        // Only measure if needed (widget has children and theme is available)
        if (!this->children().empty() && ui_services<Backend>::themes()) {
            [[maybe_unused]] auto measured_size = this->measure(width, height);
        }

        this->arrange(current_bounds);
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
        std::cerr << "[DEBUG] window::show() called for window: " << m_title << "\n";

        // Phase 5: Integrate with layer_manager
        auto* layers = ui_services<Backend>::layers();
        std::cerr << "[DEBUG] layer_manager pointer: " << static_cast<const void*>(layers) << "\n";

        if (layers) {
            // Remove existing layer if already shown
            if (m_layer_id.is_valid()) {
                std::cerr << "[DEBUG] Removing existing layer: " << m_layer_id.value << "\n";
                layers->remove_layer(m_layer_id);
            }

            // Create non-owning shared_ptr (window manages its own lifetime)
            // Store in m_layer_handle to keep weak_ptr in layer_manager alive
            m_layer_handle = std::shared_ptr<ui_element<Backend>>(
                static_cast<ui_element<Backend>*>(this),
                [](ui_element<Backend>*) {}  // No-op deleter
            );
            std::cerr << "[DEBUG] Created non-owning shared_ptr, use_count=" << m_layer_handle.use_count() << "\n";

            // Show as non-modal dialog layer
            m_layer_id = layers->add_layer(layer_type::dialog, m_layer_handle);
            std::cerr << "[DEBUG] Added to layer_manager, layer_id=" << m_layer_id.value << "\n";

            // Set layer bounds to window's position/size
            // Window uses absolute coordinates (layer and window have same bounds)
            auto window_bounds = this->bounds();
            std::cerr << "[DEBUG] Setting layer bounds to: (" << window_bounds.x << "," << window_bounds.y << ","
                      << window_bounds.w << "," << window_bounds.h << ")\n";
            layers->set_layer_bounds(m_layer_id, window_bounds);
            std::cerr << "[DEBUG] Layer bounds set\n";
        } else {
            std::cerr << "[DEBUG] ERROR: layer_manager is nullptr!\n";
        }

        this->set_visible(true);
        std::cerr << "[DEBUG] window::set_visible(true) called\n";
    }

    template<UIBackend Backend>
    void window<Backend>::show_modal() {
        // Phase 5: Integrate with layer_manager modal layer
        auto* layers = ui_services<Backend>::layers();
        if (layers) {
            // Remove existing layer if already shown
            if (m_layer_id.is_valid()) {
                layers->remove_layer(m_layer_id);
            }

            // Show as modal dialog (blocks underlying UI)
            m_layer_id = layers->show_modal_dialog(this, dialog_position::center);
        }

        this->set_visible(true);

        // Phase 5: Auto-focus modal window
        // Save previous active window for restoration when modal closes
        auto* win_mgr = ui_services<Backend>::windows();
        if (win_mgr) {
            m_previous_active_window = win_mgr->get_active_window();
            win_mgr->set_active_window(this);
        }

        // Request focus for modal
        auto* focus_mgr = ui_services<Backend>::input();
        if (focus_mgr) {
            focus_mgr->set_focus(this);
        }
    }

    template<UIBackend Backend>
    void window<Backend>::hide() {
        // Phase 5: Remove from layer_manager
        auto* layers = ui_services<Backend>::layers();
        if (layers && m_layer_id.is_valid()) {
            layers->remove_layer(m_layer_id);
            m_layer_id = layer_id::invalid();
        }

        this->set_visible(false);
    }

    template<UIBackend Backend>
    void window<Backend>::bring_to_front() {
        // Bring layer to front in layer_manager
        auto* layers = ui_services<Backend>::layers();
        if (layers && m_layer_id.is_valid()) {
            layers->bring_to_front(m_layer_id);
        }

        // Request keyboard focus from input_manager
        auto* input = ui_services<Backend>::input();
        if (input) {
            input->set_focus(this);
        }

        // Set as active window in window_manager
        auto* wm = ui_services<Backend>::windows();
        if (wm) {
            wm->set_active_window(this);
        }

        // Update window focus state and emit signal
        set_window_focus(true);
    }

    template<UIBackend Backend>
    void window<Backend>::do_render(render_context_type& ctx) const {
        std::cerr << "[DEBUG] window::do_render() called for window: " << m_title << "\n";
        std::cerr << "[DEBUG] is_visible=" << this->is_visible() << ", m_has_border=" << this->m_has_border << "\n";
        std::cerr << "[DEBUG] bounds=(" << this->bounds().x << "," << this->bounds().y << ","
                  << this->bounds().w << "," << this->bounds().h << ")\n";
        std::cerr << "[DEBUG] children count=" << this->children().size() << "\n";

        if (!this->is_visible()) {
            std::cerr << "[DEBUG] Window not visible, returning\n";
            return;
        }

        // Call base class to draw border with proper coordinate translation
        // widget_container::do_render() translates relative coords to absolute
        std::cerr << "[DEBUG] Calling base::do_render() to draw border\n";
        base::do_render(ctx);

        std::cerr << "[DEBUG] Children will render automatically via framework\n";
        // Children (title bar and content area) render automatically via framework
    }

    template<UIBackend Backend>
    bool window<Backend>::handle_event(const ui_event& event, event_phase phase) {
        // Only handle events in bubble phase (after children)
        if (phase != event_phase::bubble) {
            return base::handle_event(event, phase);
        }

        // Check if this is a mouse event
        auto* mouse_evt = std::get_if<mouse_event>(&event);
        if (!mouse_evt) {
            return base::handle_event(event, phase);
        }

        // Phase 3: Handle window resizing
        if (m_flags.is_resizable) {
            if (mouse_evt->btn == mouse_event::button::left && mouse_evt->act == mouse_event::action::press) {
                // Check if press is on a resize handle
                auto handle = get_resize_handle_at(mouse_evt->x, mouse_evt->y);
                if (handle != resize_handle::none) {
                    // Start resizing
                    m_is_resizing = true;
                    m_resize_handle = handle;
                    m_resize_initial_bounds = this->bounds();
                    m_resize_start_x = mouse_evt->x;
                    m_resize_start_y = mouse_evt->y;
                    return true;  // Event handled
                }
            }

            if (m_is_resizing && mouse_evt->act == mouse_event::action::move) {
                // Continue resizing
                int delta_x = mouse_evt->x - m_resize_start_x;
                int delta_y = mouse_evt->y - m_resize_start_y;

                auto new_bounds = m_resize_initial_bounds;

                // Apply deltas based on which handle is being dragged
                switch (m_resize_handle) {
                    case resize_handle::north:
                        new_bounds.y += delta_y;
                        new_bounds.h -= delta_y;
                        break;
                    case resize_handle::south:
                        new_bounds.h += delta_y;
                        break;
                    case resize_handle::east:
                        new_bounds.w += delta_x;
                        break;
                    case resize_handle::west:
                        new_bounds.x += delta_x;
                        new_bounds.w -= delta_x;
                        break;
                    case resize_handle::north_east:
                        new_bounds.y += delta_y;
                        new_bounds.h -= delta_y;
                        new_bounds.w += delta_x;
                        break;
                    case resize_handle::north_west:
                        new_bounds.x += delta_x;
                        new_bounds.y += delta_y;
                        new_bounds.w -= delta_x;
                        new_bounds.h -= delta_y;
                        break;
                    case resize_handle::south_east:
                        new_bounds.w += delta_x;
                        new_bounds.h += delta_y;
                        break;
                    case resize_handle::south_west:
                        new_bounds.x += delta_x;
                        new_bounds.w -= delta_x;
                        new_bounds.h += delta_y;
                        break;
                    case resize_handle::none:
                        break;
                }

                // Apply size constraints
                apply_size_constraints(new_bounds);

                // Update window bounds
                this->arrange(new_bounds);
                this->invalidate_measure();
                resized_sig.emit();

                return true;  // Event handled
            }

            if (m_is_resizing && mouse_evt->btn == mouse_event::button::left && mouse_evt->act == mouse_event::action::release) {
                // End resizing
                m_is_resizing = false;
                m_resize_handle = resize_handle::none;
                return true;  // Event handled
            }
        }

        // Not handled, pass to base
        return base::handle_event(event, phase);
    }

    template<UIBackend Backend>
    void window<Backend>::register_with_window_manager() {
        auto* mgr = ui_services<Backend>::windows();
        if (mgr) {
            mgr->register_window(this);
        }
    }

    template<UIBackend Backend>
    void window<Backend>::unregister_from_window_manager() {
        if (auto* mgr = ui_services<Backend>::windows()) {
            mgr->unregister_window(this);
        }
    }

    template<UIBackend Backend>
    typename window<Backend>::resize_handle window<Backend>::get_resize_handle_at(int x, int y) const {
        if (!m_flags.is_resizable) {
            return resize_handle::none;
        }

        auto bounds = this->bounds();
        int border = m_flags.resize_border_width;

        bool on_left = (x >= bounds.x && x < bounds.x + border);
        bool on_right = (x > bounds.x + bounds.w - border && x <= bounds.x + bounds.w);
        bool on_top = (y >= bounds.y && y < bounds.y + border);
        bool on_bottom = (y > bounds.y + bounds.h - border && y <= bounds.y + bounds.h);

        // Corners have priority
        if (on_top && on_left) return resize_handle::north_west;
        if (on_top && on_right) return resize_handle::north_east;
        if (on_bottom && on_left) return resize_handle::south_west;
        if (on_bottom && on_right) return resize_handle::south_east;

        // Edges
        if (on_top) return resize_handle::north;
        if (on_bottom) return resize_handle::south;
        if (on_left) return resize_handle::west;
        if (on_right) return resize_handle::east;

        return resize_handle::none;
    }

    template<UIBackend Backend>
    void window<Backend>::apply_size_constraints(rect_type& bounds) const {
        // Apply minimum size
        if (bounds.w < m_flags.min_width) {
            bounds.w = m_flags.min_width;
        }
        if (bounds.h < m_flags.min_height) {
            bounds.h = m_flags.min_height;
        }

        // Apply maximum size (if set)
        if (m_flags.max_width > 0 && bounds.w > m_flags.max_width) {
            bounds.w = m_flags.max_width;
        }
        if (m_flags.max_height > 0 && bounds.h > m_flags.max_height) {
            bounds.h = m_flags.max_height;
        }
    }

    template<UIBackend Backend>
    void window<Backend>::set_window_focus(bool focused) {
        if (m_has_focus != focused) {
            m_has_focus = focused;

            // Invalidate measure/layout when focus changes (forces re-render)
            this->invalidate_measure();

            // Emit focus signals
            if (m_has_focus) {
                focus_gained.emit();
            } else {
                focus_lost.emit();
            }
        }
    }

    template<UIBackend Backend>
    resolved_style<Backend> window<Backend>::get_theme_style(const theme_type& theme) const {
        // Use focused or unfocused border colors based on focus state
        return resolved_style<Backend>{
            .background_color = theme.window.content_background,
            .foreground_color = theme.text_fg,
            .mnemonic_foreground = theme.text_fg,
            .border_color = m_has_focus ? theme.window.border_color_focused : theme.window.border_color_unfocused,
            .box_style = m_has_focus ? theme.window.border_focused : theme.window.border_unfocused,
            .font = theme.label.font,
            .opacity = 1.0f,
            .icon_style = std::optional<typename Backend::renderer_type::icon_style>{},
            .padding_horizontal = std::optional<int>{},
            .padding_vertical = std::optional<int>{},
            .mnemonic_font = std::optional<typename Backend::renderer_type::font>{},
            .submenu_icon = std::optional<typename Backend::renderer_type::icon_style>{}
        };
    }

    // ========================================================================
    // window_manager Implementation (requires window<Backend> fully defined)
    // ========================================================================

    template<UIBackend Backend>
    std::vector<window<Backend>*> window_manager<Backend>::get_visible_windows() const {
        std::vector<window<Backend>*> visible;
        visible.reserve(m_windows.size());

        for (auto* win : m_windows) {
            if (win && win->is_visible() && win->get_state() != window<Backend>::window_state::minimized) {
                visible.push_back(win);
            }
        }

        return visible;
    }

    template<UIBackend Backend>
    std::vector<window<Backend>*> window_manager<Backend>::get_minimized_windows() const {
        std::vector<window<Backend>*> minimized;
        minimized.reserve(m_windows.size());

        for (auto* win : m_windows) {
            if (win && win->get_state() == window<Backend>::window_state::minimized) {
                minimized.push_back(win);
            }
        }

        return minimized;
    }

    template<UIBackend Backend>
    std::vector<window<Backend>*> window_manager<Backend>::get_modal_windows() const {
        std::vector<window<Backend>*> modal;
        modal.reserve(m_windows.size());

        for (auto* win : m_windows) {
            // Phase 5: Check if window is modal
            if (win && win->is_modal()) {
                modal.push_back(win);
            }
        }

        return modal;
    }

    template<UIBackend Backend>
    void window_manager<Backend>::show_window_list() {
        // Note: This is a basic implementation for Phase 4
        // Full implementation with proper modal support comes in Phase 5

        // For now, we can't easily create the dialog because it would require
        // dynamic allocation and lifecycle management.
        // This will be properly implemented when dialog lifecycle is established.

        // Placeholder: In a real implementation, this would:
        // 1. Create window_list_dialog
        // 2. Add all windows from m_windows
        // 3. Connect window_selected signal to restore/focus
        // 4. Show as modal dialog

        // TODO Phase 5: Implement with proper dialog lifecycle
    }

    template<UIBackend Backend>
    void window_manager<Backend>::cycle_next_window() {
        auto visible = get_visible_windows();
        if (visible.empty()) {
            m_active_window = nullptr;
            return;
        }

        // Find current active window in visible list
        auto it = std::find(visible.begin(), visible.end(), m_active_window);

        if (it == visible.end()) {
            // No active window or active window not visible - activate first
            m_active_window = visible[0];
        } else {
            // Cycle to next window (wrap around)
            ++it;
            if (it == visible.end()) {
                m_active_window = visible[0];  // Wrap to first
            } else {
                m_active_window = *it;
            }
        }

        // Bring window to front and focus
        if (m_active_window) {
            m_active_window->bring_to_front();
        }
    }

    template<UIBackend Backend>
    void window_manager<Backend>::cycle_previous_window() {
        auto visible = get_visible_windows();
        if (visible.empty()) {
            m_active_window = nullptr;
            return;
        }

        // Find current active window in visible list
        auto it = std::find(visible.begin(), visible.end(), m_active_window);

        if (it == visible.end() || it == visible.begin()) {
            // No active window, not visible, or at first - go to last
            m_active_window = visible.back();
        } else {
            // Cycle to previous window
            --it;
            m_active_window = *it;
        }

        // Bring window to front and focus
        if (m_active_window) {
            m_active_window->bring_to_front();
        }
    }

    template<UIBackend Backend>
    void window_manager<Backend>::register_hotkeys() {
        auto* hotkeys = ui_services<Backend>::hotkeys();
        if (!hotkeys) return;

        // Ctrl+W - Show window list (Turbo Vision style)
        hotkeys->register_semantic_action(
            hotkey_action::show_window_list,
            [this]() { this->show_window_list(); }
        );

        // Ctrl+Tab / Ctrl+F6 - Cycle to next window
        hotkeys->register_semantic_action(
            hotkey_action::next_window,
            [this]() { this->cycle_next_window(); }
        );

        // Ctrl+Shift+Tab / Ctrl+Shift+F6 - Cycle to previous window
        hotkeys->register_semantic_action(
            hotkey_action::previous_window,
            [this]() { this->cycle_previous_window(); }
        );
    }

} // namespace onyxui
