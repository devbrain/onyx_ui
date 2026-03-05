/**
 * @file window.inl
 * @brief Implementation of window class
 */

#pragma once

#include "onyxui/concepts/backend.hh"
#include "onyxui/hotkeys/hotkey_action.hh"
#include <onyxui/widgets/window/window_title_bar.hh>
#include <onyxui/widgets/window/window_content_area.hh>
#include <onyxui/widgets/window/window_system_menu.hh>
#include <onyxui/services/ui_services.hh>
#include <failsafe/logger.hh>
#include <vector>

namespace onyxui {
    template<UIBackend Backend>
    window <Backend>::window(
        std::string title,
        window_flags flags,
        ui_element <Backend>* parent
    )
        : base(
              std::make_unique <linear_layout <Backend>>(
                  direction::vertical,
                  0 // No spacing between title bar and content
              ),
              parent
          )
          , m_title(std::move(title))
          , m_flags(flags) {
        // Create title bar and content area
        // (Drag/resize implementation comes in later phases)

        if (m_flags.has_title_bar) {
            auto title_bar = std::make_unique <window_title_bar <Backend>>(
                m_title,
                m_flags,
                this
            );
            m_title_bar = title_bar.get(); // Store raw pointer before moving

            // Connect drag signals for window movement (if movable)
            if (m_flags.is_movable && m_title_bar) {
                m_title_bar->drag_started.connect([this]() {
                    // Save initial position when drag starts (already logical)
                    m_drag_initial_bounds = this->bounds();
                });

                m_title_bar->dragging.connect([this](int delta_x, int delta_y) {
                    // Update window position during drag
                    // Delta values are in logical units (from mouse coordinates)
                    logical_rect new_bounds = m_drag_initial_bounds;
                    new_bounds.x = new_bounds.x + logical_unit(static_cast<double>(delta_x));
                    new_bounds.y = new_bounds.y + logical_unit(static_cast<double>(delta_y));
                    this->arrange(new_bounds);

                    // Update layer bounds so the window visually moves
                    if (m_layer_id.is_valid()) {
                        auto* layers = ui_services<Backend>::layers();
                        if (layers) {
                            layers->set_layer_bounds(m_layer_id, new_bounds);
                        }
                    }

                    moved.emit();
                });
            }

            this->add_child(std::move(title_bar));
        }

        // Create system menu with window controls (if menu button enabled)
        if (m_flags.has_menu_button && m_title_bar) {
            m_system_menu = std::make_unique <window_system_menu <Backend>>(this);

            // Wire up menu button to show system menu
            m_title_bar->menu_clicked.connect([this]() {
                if (!m_system_menu) {
                    return;
                }

                m_system_menu->update_menu_states();

                // Get menu and icon
                auto* menu_icon = m_title_bar->get_menu_icon();
                auto* menu = m_system_menu->get_menu();

                if (!menu_icon || !menu) {
                    return;
                }

                auto* layers = ui_services <Backend>::layers();
                if (!layers) {
                    return;
                }

                // Get icon's absolute screen bounds for menu positioning
                // Get absolute logical bounds for popup positioning
                auto icon_abs_bounds = menu_icon->get_absolute_logical_bounds();

                // Use layer_manager's show_popup() to automatically position menu below icon
                // Layer manager now works with logical coordinates directly
                layer_id id = layers->show_popup(
                    menu,
                    icon_abs_bounds,
                    popup_placement::below,
                    [menu]() {
                        menu->closing.emit();
                    }
                );

                // Wrap in scoped_layer for RAII cleanup
                m_system_menu_layer = scoped_layer <Backend>(layers, id);

                // Connect to menu's closing signal to dismiss the popup
                // When a menu item is clicked, the menu emits closing signal
                m_system_menu_closing_connection = scoped_connection(menu->closing, [this]() {
                    m_system_menu_layer.reset();
                });

                // Activate menu: reset states and focus first item
                menu->reset_item_states();
                menu->focus_first();

                // Set focus to menu
                if (auto* focus = ui_services <Backend>::input()) {
                    focus->set_focus(menu);
                }
            });
        }

        // Connect window control button signals (minimize/maximize/close)
        if (m_title_bar) {
            // Minimize button
            if (m_flags.has_minimize_button) {
                m_title_bar->minimize_clicked.connect([this]() {
                    minimize();
                });
            }

            // Maximize button
            if (m_flags.has_maximize_button) {
                m_title_bar->maximize_clicked.connect([this]() {
                    if (m_state == window_state::maximized) {
                        restore();
                    } else {
                        maximize();
                    }
                });
            }

            // Close button
            if (m_flags.has_close_button) {
                m_title_bar->close_clicked.connect([this]() {
                    close();
                });
            }
        }

        auto content_area = std::make_unique <window_content_area <Backend>>(
            m_flags.is_scrollable,
            this
        );

        // Window should draw the border (not the content area)
        this->m_has_border = true;
        this->invalidate_measure();
        content_area->set_has_border(false);

        // Content area should expand to fill remaining vertical space in window
        size_constraint height_constraint;
        height_constraint.policy = size_policy::expand;
        content_area->set_height_constraint(height_constraint);

        m_content_area = content_area.get(); // Store raw pointer before moving
        this->add_child(std::move(content_area));

        // Register with window_manager (if available)
        register_with_window_manager();
    }

    template<UIBackend Backend>
    window <Backend>::~window() {
        // Remove from layer_manager (cleanup on window destruction)
        hide();

        // Unregister from window_manager
        unregister_from_window_manager();
    }

    template<UIBackend Backend>
    void window <Backend>::minimize() {
        if (m_state == window_state::minimized) {
            return; // Already minimized
        }

        // Save current bounds for restore (already in logical coordinates)
        m_before_minimize = this->bounds();

        // Change state
        m_state = window_state::minimized;

        // Hide window by removing from layer_manager
        this->set_visible(false);

        // Emit signal
        minimized_sig.emit();

        // Notify window_manager of window state change
        auto* mgr = ui_services <Backend>::windows();
        if (mgr) {
            mgr->handle_minimize(this);
        }
    }

    template<UIBackend Backend>
    void window <Backend>::maximize() {
        if (m_state == window_state::maximized) {
            return; // Already maximized
        }

        // Save normal bounds for restore (already in logical coordinates)
        if (m_state == window_state::normal) {
            m_normal_bounds = this->bounds();
        }

        // Change state
        m_state = window_state::maximized;

        // Phase 4: Fill parent container or screen (all in logical coordinates)
        logical_rect maximized_bounds;

        // Determine maximize bounds (priority order: parent > workspace > viewport)
        auto* parent_elem = this->parent();
        if (parent_elem) {
            // Case 1: Window has parent (MDI) - maximize to fill parent
            auto parent_bounds = parent_elem->bounds();
            // Window fills entire parent (positioned at 0,0 relative to parent)
            maximized_bounds = logical_rect{0.0_lu, 0.0_lu, parent_bounds.width, parent_bounds.height};
        } else if (m_workspace) {
            // Case 2: Floating window with workspace - maximize to fill workspace
            // Use workspace's full bounds (already in logical coordinates)
            maximized_bounds = m_workspace->bounds();
        } else {
            // Case 3: Floating window without workspace - maximize to fill viewport
            auto* layers = ui_services <Backend>::layers();
            auto* metrics = ui_services <Backend>::metrics();
            if (layers && metrics) {
                auto viewport = layers->get_viewport();
                // Check if viewport is valid (non-zero size)
                if (rect_utils::get_width(viewport) > 0 && rect_utils::get_height(viewport) > 0) {
                    // Convert physical viewport to logical
                    maximized_bounds = metrics->physical_to_logical_rect(viewport);
                } else {
                    // Viewport not set - use theme defaults
                    auto* theme = ui_services<Backend>::themes()->get_current_theme();
                    maximized_bounds = logical_rect{0.0_lu, 0.0_lu,
                        logical_unit(theme->window.default_width),
                        logical_unit(theme->window.default_height)};
                }
            }
        }

        // Measure and arrange to new size with stabilization loop
        constexpr int MAX_LAYOUT_PASSES = 10;
        int pass_count = 0;

        do {
            if (!this->children().empty() && ui_services <Backend>::themes()) {
                [[maybe_unused]] const auto measured_size = this->measure(maximized_bounds.width, maximized_bounds.height);
            }
            this->arrange(maximized_bounds);
            ++pass_count;
        } while ((this->needs_measure() || this->needs_arrange()) &&
                 pass_count < MAX_LAYOUT_PASSES &&
                 !this->children().empty() &&
                 ui_services <Backend>::themes());

        // Update layer bounds if this window is shown as a layer
        if (m_layer_id.is_valid()) {
            auto* layers = ui_services <Backend>::layers();
            if (layers) {
                layers->set_layer_bounds(m_layer_id, maximized_bounds);
            }
        }

        // Update title bar icons (show restore icon instead of maximize)
        if (m_title_bar) {
            m_title_bar->show_restore_icon();
        }

        // CRITICAL FIX: Restore focus to window after maximize
        auto* input = ui_services <Backend>::input();
        if (input) {
            input->set_focus(this);
            input->release_capture();
        }

        // CRITICAL FIX: Final stabilization pass after layer/focus updates
        if ((this->needs_measure() || this->needs_arrange()) &&
            !this->children().empty() &&
            ui_services <Backend>::themes()) {
            [[maybe_unused]] const auto measured_size = this->measure(maximized_bounds.width, maximized_bounds.height);
            this->arrange(maximized_bounds);
            ++pass_count;
        }

        // Emit signal
        maximized_sig.emit();
    }

    template<UIBackend Backend>
    void window <Backend>::restore() {
        if (m_state == window_state::normal) {
            return; // Already in normal state
        }

        auto old_state = m_state;
        m_state = window_state::normal;

        if (old_state == window_state::minimized) {
            // Restore visibility
            this->set_visible(true);

            // Restore bounds (already in logical coordinates)
            if (m_before_minimize.width.value > 0 && m_before_minimize.height.value > 0) {
                this->arrange(m_before_minimize);

                // Update layer bounds
                if (m_layer_id.is_valid()) {
                    auto* layers = ui_services <Backend>::layers();
                    if (layers) {
                        layers->set_layer_bounds(m_layer_id, m_before_minimize);
                    }
                }
            }
        } else if (old_state == window_state::maximized) {
            // Restore to normal bounds (already in logical coordinates)
            if (m_normal_bounds.width.value > 0 && m_normal_bounds.height.value > 0) {
                this->arrange(m_normal_bounds);

                // Update layer bounds
                if (m_layer_id.is_valid()) {
                    auto* layers = ui_services <Backend>::layers();
                    if (layers) {
                        layers->set_layer_bounds(m_layer_id, m_normal_bounds);
                    }
                }
            }

            // Update title bar icons (show maximize icon instead of restore)
            if (m_title_bar) {
                m_title_bar->show_maximize_icon();
            }
        }

        // CRITICAL FIX: Release mouse capture after restore
        // The restore button or other widgets may have captured the mouse during
        // the restore button click, preventing subsequent clicks from working correctly
        auto* input = ui_services <Backend>::input();
        if (input) {
            input->release_capture();
        }

        // Emit signal
        restored_sig.emit();

        // Notify window_manager of window state change
        auto* mgr = ui_services <Backend>::windows();
        if (mgr) {
            mgr->handle_restore(this);
        }
    }

    template<UIBackend Backend>
    void window <Backend>::close() {
        // Emit closing signal (can be cancelled in future)
        closing.emit();

        // Phase 5: Call hook for subclasses (e.g., dialog result handling)
        on_close();

        // Phase 5: Restore focus if this was a modal window
        if (m_flags.is_modal && m_previous_active_window) {
            auto* win_mgr = ui_services <Backend>::windows();
            if (win_mgr) {
                win_mgr->set_active_window(m_previous_active_window);
            }

            auto* focus_mgr = ui_services <Backend>::input();
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
    void window <Backend>::set_position(int x, int y) {
        // Work entirely in logical coordinates - no physical roundtrip
        auto logical_bounds = this->bounds();
        logical_bounds.x = logical_unit(static_cast<double>(x));
        logical_bounds.y = logical_unit(static_cast<double>(y));
        this->arrange(logical_bounds);

        moved.emit();
    }

    template<UIBackend Backend>
    void window <Backend>::set_size(int width, int height) {
        // Work entirely in logical coordinates - no physical roundtrip
        auto logical_bounds = this->bounds();
        logical_unit const new_width{static_cast<double>(width)};
        logical_unit const new_height{static_cast<double>(height)};

        // Only measure if needed (widget has children and theme is available)
        if (!this->children().empty() && ui_services <Backend>::themes()) {
            (void)this->measure(new_width, new_height);
        }

        logical_bounds.width = new_width;
        logical_bounds.height = new_height;
        this->arrange(logical_bounds);

        // CRITICAL FIX: Handle layout invalidation during arrange (e.g., scrollbar visibility changes)
        // When scrollbars appear/disappear during arrange, they invalidate the grid's layout.
        // Check if layout became dirty and do ONE additional measure/arrange pass if needed.
        // This is a standard pattern for handling layout dependency cycles in UI frameworks.
        if ((this->needs_measure() || this->needs_arrange()) && !this->children().empty() && ui_services <
                Backend>::themes()) {
            (void)this->measure(new_width, new_height);
            this->arrange(logical_bounds);
        }

        this->invalidate_measure();

        resized_sig.emit();
    }

    template<UIBackend Backend>
    void window<Backend>::fit_content() {
        // Ensure we have content to measure
        if (!m_content_area) {
            return;
        }

        auto* content = m_content_area->get_content();
        if (!content) {
            return;
        }

        // Measure content at a large available size to get its natural size
        const auto LARGE_SIZE = logical_unit(10000);
        const auto content_size = content->measure(LARGE_SIZE, LARGE_SIZE);

        int content_width = size_utils::get_width(content_size);
        int content_height = size_utils::get_height(content_size);

        // Add space for title bar if present
        int total_height = content_height;
        if (m_title_bar) {
            const auto title_bar_size = m_title_bar->measure(LARGE_SIZE, LARGE_SIZE);
            total_height += size_utils::get_height(title_bar_size);
        }

        int total_width = content_width;

        // Apply size constraints from window_flags
        if (m_flags.min_width > 0) {
            total_width = std::max(total_width, m_flags.min_width);
        }
        if (m_flags.min_height > 0) {
            total_height = std::max(total_height, m_flags.min_height);
        }
        if (m_flags.max_width > 0) {
            total_width = std::min(total_width, m_flags.max_width);
        }
        if (m_flags.max_height > 0) {
            total_height = std::min(total_height, m_flags.max_height);
        }

        // Update window size
        set_size(total_width, total_height);
    }

    template<UIBackend Backend>
    void window <Backend>::set_content(std::unique_ptr <ui_element <Backend>> content) {
        if (m_content_area) {
            m_content_area->set_content(std::move(content));

            // Invalidate layout since content changed
            this->invalidate_measure();

            // If window already has bounds (was sized), trigger re-layout
            // Work entirely in logical coordinates - no physical roundtrip
            auto const logical_bounds = this->bounds();
            if (logical_bounds.width.value > 0.0 && logical_bounds.height.value > 0.0) {
                // Re-measure and re-arrange window with current size
                [[maybe_unused]] const auto measured_size = this->measure(
                    logical_bounds.width, logical_bounds.height);
                this->arrange(logical_bounds);

                // Force content_area to re-arrange its children
                // (window::arrange may skip if bounds unchanged due to caching)
                auto ca_bounds = m_content_area->bounds();
                [[maybe_unused]] auto ca_measured = m_content_area->measure(
                    ca_bounds.width,
                    ca_bounds.height
                );
                m_content_area->arrange(ca_bounds);
            }
        }
    }

    template<UIBackend Backend>
    ui_element <Backend>* window <Backend>::get_content() noexcept {
        return m_content_area ? m_content_area->get_content() : nullptr;
    }

    template<UIBackend Backend>
    void window <Backend>::set_title(const std::string& title) {
        if (m_title != title) {
            m_title = title;

            if (m_title_bar) {
                m_title_bar->set_title(title);
            }
        }
    }

    template<UIBackend Backend>
    void window <Backend>::show() {
        // Phase 5: Integrate with layer_manager
        auto* layers = ui_services <Backend>::layers();

        if (layers) {
            // Remove existing layer if already shown
            if (m_layer_id.is_valid()) {
                layers->remove_layer(m_layer_id);
            }

            // Create non-owning shared_ptr (window manages its own lifetime)
            // Store in m_layer_handle to keep weak_ptr in layer_manager alive
            m_layer_handle = std::shared_ptr <ui_element <Backend>>(
                static_cast <ui_element <Backend>*>(this),
                [](ui_element <Backend>*) {
                } // No-op deleter
            );

            // Show as window layer (z=150, below popups z=200)
            m_layer_id = layers->add_layer(layer_type::window, m_layer_handle);

            // Set layer bounds to window's position/size
            // Window uses absolute coordinates (layer and window have same bounds)
            // Layer manager now works with logical coordinates directly
            layers->set_layer_bounds(m_layer_id, this->bounds());
        }

        // Clear modal flag (show() is non-modal, use show_modal() for modal)
        m_flags.is_modal = false;

        this->set_visible(true);

        // Set window focus so title bar shows as focused (blue)
        set_window_focus(true);
    }

    template<UIBackend Backend>
    void window <Backend>::show_modal() {
        // Integrate with layer_manager modal layer
        if (auto* layers = ui_services <Backend>::layers()) {
            // Remove existing layer if already shown
            if (m_layer_id.is_valid()) {
                layers->remove_layer(m_layer_id);
            }

            // Show as modal dialog (blocks underlying UI)
            m_layer_id = layers->show_modal_dialog(this, dialog_position::center);
        }

        // Set modal flag so is_modal() returns true
        m_flags.is_modal = true;

        this->set_visible(true);

        // Auto-focus modal window
        // Save previous active window for restoration when modal closes
        if (auto* win_mgr = ui_services <Backend>::windows()) {
            m_previous_active_window = win_mgr->get_active_window();
            win_mgr->set_active_window(this);
        }

        // Request focus for modal
        if (auto* focus_mgr = ui_services <Backend>::input()) {
            // Clear any stale hover/capture from the underlying UI before modal input
            focus_mgr->release_capture();
            focus_mgr->clear_hover();
            focus_mgr->set_focus(this);
        }

        // Set window focus so title bar shows as focused (blue)
        set_window_focus(true);
    }

    template<UIBackend Backend>
    void window <Backend>::hide() {
        // Remove from layer_manager (cleanup on window destruction)
        auto* layers = ui_services <Backend>::layers();
        if (layers && m_layer_id.is_valid()) {
            layers->remove_layer(m_layer_id);
            m_layer_id = layer_id::invalid();
        }

        this->set_visible(false);
    }

    template<UIBackend Backend>
    void window <Backend>::bring_to_front() {
        // Bring layer to front in layer_manager
        auto* layers = ui_services <Backend>::layers();
        if (layers && m_layer_id.is_valid()) {
            layers->bring_to_front(m_layer_id);
        }

        // Request keyboard focus from input_manager
        auto* input = ui_services <Backend>::input();
        if (input) {
            input->set_focus(this);
        }

        // Set as active window in window_manager
        auto* wm = ui_services <Backend>::windows();
        if (wm) {
            wm->set_active_window(this);
        }

        // Update window focus state and emit signal
        set_window_focus(true);
    }

    template<UIBackend Backend>
    void window <Backend>::do_render(render_context_type& ctx) const {
        if (!this->is_visible()) {
            return;
        }

        // Draw window border (from widget_container) and let children render normally.
        base::do_render(ctx);

        // For 1-row title bars (conio), cover the top border line with title background
        // so the title bar doesn't leave isolated corner glyphs.
        if (m_title_bar && this->m_has_border) {
            if (auto* theme = ctx.theme()) {
                if (theme->window.title_bar_height == 1) {
                    auto logical_bounds = this->bounds();
                    auto const [final_width, final_height] = ctx.get_final_dims(
                        logical_bounds.width.to_int(), logical_bounds.height.to_int());
                    (void)final_height;

                    typename Backend::rect_type top_row;
                    rect_utils::set_bounds(top_row,
                        point_utils::get_x(ctx.position()) + 1,
                        point_utils::get_y(ctx.position()),
                        std::max(0, final_width - 2), 1);

                    auto const& title_state = m_has_focus
                        ? theme->window.title_focused
                        : theme->window.title_unfocused;
                    ctx.fill_rect(top_row, title_state.background);
                }
            }
        }
    }

    template<UIBackend Backend>
    logical_rect window<Backend>::get_content_area() const noexcept {
        // Start from ui_element base content area (margin + padding only).
        logical_rect content = ui_element<Backend>::get_content_area();

        if (this->m_has_border) {
            // Shrink for left/right borders.
            content.x = content.x + logical_unit(1.0);
            content.width = max(logical_unit(0.0), content.width - logical_unit(2.0));

            // Shrink for bottom border only (top border is reserved for title bar overlap).
            content.height = max(logical_unit(0.0), content.height - logical_unit(1.0));
        }

        return content;
    }

    template<UIBackend Backend>
    bool window <Backend>::handle_event(const ui_event& event, event_phase phase) {
        // Only handle events in bubble phase (after children)
        if (phase != event_phase::bubble) {
            return base::handle_event(event, phase);
        }

        // Check if this is a mouse event
        auto* mouse_evt = std::get_if <mouse_event>(&event);
        if (!mouse_evt) {
            return base::handle_event(event, phase);
        }

        // Phase 3: Handle window resizing (operates entirely in logical coordinates)
        if (m_flags.is_resizable) {
            if (mouse_evt->btn == mouse_event::button::left && mouse_evt->act == mouse_event::action::press) {
                // Check if press is on a resize handle (use logical coordinates directly)
                auto handle = get_resize_handle_at(mouse_evt->x.value, mouse_evt->y.value);
                if (handle != resize_handle::none) {
                    // Start resizing - store initial bounds in logical coordinates
                    m_is_resizing = true;
                    m_resize_handle = handle;
                    m_resize_initial_bounds = this->bounds();  // Already logical
                    m_resize_start_x = mouse_evt->x;  // Logical coordinate
                    m_resize_start_y = mouse_evt->y;  // Logical coordinate
                    return true; // Event handled
                }
            }

            if (m_is_resizing && mouse_evt->act == mouse_event::action::move) {
                // Continue resizing - all calculations in logical coordinates
                logical_unit const delta_x = mouse_evt->x - m_resize_start_x;
                logical_unit const delta_y = mouse_evt->y - m_resize_start_y;

                logical_rect new_bounds = m_resize_initial_bounds;

                // Apply deltas based on which handle is being dragged
                switch (m_resize_handle) {
                    case resize_handle::north:
                        new_bounds.y = new_bounds.y + delta_y;
                        new_bounds.height = new_bounds.height - delta_y;
                        break;
                    case resize_handle::south:
                        new_bounds.height = new_bounds.height + delta_y;
                        break;
                    case resize_handle::east:
                        new_bounds.width = new_bounds.width + delta_x;
                        break;
                    case resize_handle::west:
                        new_bounds.x = new_bounds.x + delta_x;
                        new_bounds.width = new_bounds.width - delta_x;
                        break;
                    case resize_handle::north_east:
                        new_bounds.y = new_bounds.y + delta_y;
                        new_bounds.height = new_bounds.height - delta_y;
                        new_bounds.width = new_bounds.width + delta_x;
                        break;
                    case resize_handle::north_west:
                        new_bounds.x = new_bounds.x + delta_x;
                        new_bounds.y = new_bounds.y + delta_y;
                        new_bounds.width = new_bounds.width - delta_x;
                        new_bounds.height = new_bounds.height - delta_y;
                        break;
                    case resize_handle::south_east:
                        new_bounds.width = new_bounds.width + delta_x;
                        new_bounds.height = new_bounds.height + delta_y;
                        break;
                    case resize_handle::south_west:
                        new_bounds.x = new_bounds.x + delta_x;
                        new_bounds.width = new_bounds.width - delta_x;
                        new_bounds.height = new_bounds.height + delta_y;
                        break;
                    case resize_handle::none:
                        break;
                }

                // Apply size constraints (operates on logical_rect)
                apply_size_constraints(new_bounds);

                // Update window bounds - already in logical coordinates
                this->arrange(new_bounds);
                this->invalidate_measure();
                resized_sig.emit();

                return true; // Event handled
            }

            if (m_is_resizing && mouse_evt->btn == mouse_event::button::left && mouse_evt->act ==
                mouse_event::action::release) {
                // End resizing
                m_is_resizing = false;
                m_resize_handle = resize_handle::none;
                return true; // Event handled
            }
        }

        // Not handled, pass to base
        return base::handle_event(event, phase);
    }

    template<UIBackend Backend>
    void window <Backend>::register_with_window_manager() {
        auto* mgr = ui_services <Backend>::windows();
        if (mgr) {
            mgr->register_window(this);
        }
    }

    template<UIBackend Backend>
    void window <Backend>::unregister_from_window_manager() {
        if (auto* mgr = ui_services <Backend>::windows()) {
            mgr->unregister_window(this);
        }
    }

    template<UIBackend Backend>
    typename window <Backend>::resize_handle window <Backend>::get_resize_handle_at(double x, double y) const {
        if (!m_flags.is_resizable) {
            return resize_handle::none;
        }

        auto bounds = this->bounds();
        double border = static_cast<double>(m_flags.resize_border_width);

        // Use logical coordinates (double precision)
        double bounds_x = bounds.x.value;
        double bounds_y = bounds.y.value;
        double bounds_width = bounds.width.value;
        double bounds_height = bounds.height.value;

        bool on_left = (x >= bounds_x && x < bounds_x + border);
        bool on_right = (x > bounds_x + bounds_width - border && x <= bounds_x + bounds_width);
        bool on_top = (y >= bounds_y && y < bounds_y + border);
        bool on_bottom = (y > bounds_y + bounds_height - border && y <= bounds_y + bounds_height);

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
    void window <Backend>::apply_size_constraints(logical_rect& bounds) const {
        // Convert constraint values to logical units for comparison
        logical_unit const min_width{static_cast<double>(m_flags.min_width)};
        logical_unit const min_height{static_cast<double>(m_flags.min_height)};
        logical_unit const max_width{static_cast<double>(m_flags.max_width)};
        logical_unit const max_height{static_cast<double>(m_flags.max_height)};

        // Apply minimum size
        if (bounds.width < min_width) {
            bounds.width = min_width;
        }
        if (bounds.height < min_height) {
            bounds.height = min_height;
        }

        // Apply maximum size (if set - 0 means no limit)
        if (m_flags.max_width > 0 && bounds.width > max_width) {
            bounds.width = max_width;
        }
        if (m_flags.max_height > 0 && bounds.height > max_height) {
            bounds.height = max_height;
        }
    }

    template<UIBackend Backend>
    void window <Backend>::set_window_focus(bool focused) {
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
    resolved_style <Backend> window <Backend>::get_theme_style(const theme_type& theme) const {
        // Use focused or unfocused border colors based on focus state
        return resolved_style <Backend>{
            .background_color = theme.window.content_background,
            .foreground_color = theme.text_fg,
            .mnemonic_foreground = theme.text_fg,
            .border_color = m_has_focus ? theme.window.border_color_focused : theme.window.border_color_unfocused,
            .box_style = m_has_focus ? theme.window.border_focused : theme.window.border_unfocused,
            .font = theme.label.font,
            .opacity = 1.0f,
            .icon_style = std::optional <typename Backend::renderer_type::icon_style>{},
            .padding_horizontal = std::optional <int>{},
            .padding_vertical = std::optional <int>{},
            .mnemonic_font = std::optional <typename Backend::renderer_type::font>{},
            .submenu_icon = std::optional <typename Backend::renderer_type::icon_style>{}
        };
    }

    // ========================================================================
    // window_manager Implementation (requires window<Backend> fully defined)
    // ========================================================================

    template<UIBackend Backend>
    std::vector <window <Backend>*> window_manager <Backend>::get_visible_windows() const {
        std::vector <window <Backend>*> visible;
        visible.reserve(m_windows.size());

        for (auto* win : m_windows) {
            if (win && win->is_visible() && win->get_state() != window <Backend>::window_state::minimized) {
                visible.push_back(win);
            }
        }

        return visible;
    }

    template<UIBackend Backend>
    std::vector <window <Backend>*> window_manager <Backend>::get_minimized_windows() const {
        std::vector <window <Backend>*> minimized;
        minimized.reserve(m_windows.size());

        for (auto* win : m_windows) {
            if (win && win->get_state() == window <Backend>::window_state::minimized) {
                minimized.push_back(win);
            }
        }

        return minimized;
    }

    template<UIBackend Backend>
    std::vector <window <Backend>*> window_manager <Backend>::get_modal_windows() const {
        std::vector <window <Backend>*> modal;
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
    void window_manager <Backend>::show_window_list() {
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
    void window_manager <Backend>::cycle_next_window() {
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
                m_active_window = visible[0]; // Wrap to first
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
    void window_manager <Backend>::cycle_previous_window() {
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
    void window_manager <Backend>::register_hotkeys() {
        auto* hotkeys = ui_services <Backend>::hotkeys();
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
