/**
 * @file scrollbar.hh
 * @brief Visual scrollbar widget with interactive thumb and arrow buttons
 * @author OnyxUI Framework
 * @date 2025-10-29
 */

#pragma once

#include <onyxui/widgets/core/widget.hh>
#include <onyxui/widgets/core/widget_container.hh>
#include <onyxui/widgets/containers/scroll/scroll_info.hh>
#include <onyxui/widgets/containers/scroll/scrollbar_thumb.hh>
#include <onyxui/widgets/containers/scroll/scrollbar_arrow.hh>
#include <onyxui/layout/absolute_layout.hh>
#include <onyxui/core/signal.hh>
#include <onyxui/core/orientation.hh>
#include <onyxui/ui_constants.hh>  // For default scrollbar dimensions
#include <onyxui/theming/theme.hh>
#include <onyxui/services/ui_services.hh>
#include <onyxui/events/ui_event.hh>
#include <onyxui/events/event_phase.hh>
#include <algorithm>

namespace onyxui {

    /**
     * @class scrollbar
     * @brief Visual scrollbar widget with draggable thumb and optional arrow buttons
     *
     * @details
     * The scrollbar widget provides visual feedback and interactive control
     * for scrolling. It is the visual layer of the scrolling system.
     *
     * Architecture:
     * - Logic layer (scrollable): Viewport clipping + scroll offset management
     * - Visual layer (this class): Visual representation + interaction
     * - Coordination layer (scroll_controller): Binds logic + visual
     *
     * Components:
     * - Track: Background bar showing scrollable region
     * - Thumb: Draggable indicator showing viewport position/size
     * - Arrow buttons: Optional increment/decrement buttons (style-dependent)
     *
     * Interaction:
     * - Thumb drag: Direct proportional scrolling
     * - Track click: Page up/down scrolling
     * - Arrow click: Line up/down scrolling
     * - Mouse wheel: Line scrolling (handled by scrollable)
     *
     * Measurement:
     * - Horizontal: width = content-based, height = theme.scrollbar.width
     * - Vertical: width = theme.scrollbar.width, height = content-based
     *
     * Thumb Sizing:
     * - thumb_size = max(min_thumb_size, track_size * (viewport / content))
     * - Ensures thumb is always draggable even with huge content
     *
     * Thumb Position:
     * - thumb_pos = (scroll_offset / max_scroll) * (track_size - thumb_size)
     * - Maps scroll position to thumb position
     *
     * @tparam Backend UIBackend implementation
     */
    template<UIBackend Backend>
    class scrollbar : public widget_container<Backend> {
    public:
        using base = widget_container<Backend>;
        using size_type = typename Backend::size_type;
        using point_type = typename Backend::point_type;
        using rect_type = typename Backend::rect_type;
        using color_type = typename Backend::color_type;
        using renderer_type = typename Backend::renderer_type;
        using theme_type = typename base::theme_type;

        /**
         * @brief Construct scrollbar with orientation
         * @param orient Horizontal or vertical orientation
         */
        explicit scrollbar(orientation orient = orientation::vertical)
            : base(
                std::make_unique<absolute_layout<Backend>>()  // Manually position components
              )
            , m_orientation(orient)
        {
            // Create child widgets
            auto thumb = std::make_unique<scrollbar_thumb<Backend>>();
            m_thumb = thumb.get();  // Store raw pointer before moving
            m_thumb->set_visible(true);  // CRITICAL: Make visible (widgets invisible by default)

            auto arrow_dec = std::make_unique<scrollbar_arrow<Backend>>(
                orient == orientation::vertical
                    ? arrow_direction::up
                    : arrow_direction::left
            );
            m_arrow_dec = arrow_dec.get();
            m_arrow_dec->set_visible(true);  // CRITICAL: Make visible

            auto arrow_inc = std::make_unique<scrollbar_arrow<Backend>>(
                orient == orientation::vertical
                    ? arrow_direction::down
                    : arrow_direction::right
            );
            m_arrow_inc = arrow_inc.get();
            m_arrow_inc->set_visible(true);  // CRITICAL: Make visible

            // Add as children (transfers ownership)
            this->add_child(std::move(thumb));
            this->add_child(std::move(arrow_dec));
            this->add_child(std::move(arrow_inc));
        }

        /**
         * @brief Set scroll information (content, viewport, offset)
         * @param info Scroll information from scrollable widget
         * @note Updates thumb position/size automatically
         */
        void set_scroll_info(const scroll_info<Backend>& info) {
            if (m_scroll_info == info) {
                return;  // No change
            }

            m_scroll_info = info;
            this->invalidate_arrange();  // Thumb position/size may have changed

            // CRITICAL FIX: If we've already been arranged once, immediately update child bounds
            // to avoid thumb being invisible until next layout cycle
            auto const bounds = this->bounds();
            if (bounds.width.to_int() > 0 && bounds.height.to_int() > 0) {
                // We have valid bounds, so we've been arranged - update children immediately
                update_child_arrangement();
            }
        }

        /**
         * @brief Get current scroll information
         * @return Current scroll info
         */
        [[nodiscard]] scroll_info<Backend> get_scroll_info() const noexcept {
            return m_scroll_info;
        }

        /**
         * @brief Set scrollbar orientation
         * @param orient Horizontal or vertical
         */
        void set_orientation(orientation orient) {
            if (m_orientation == orient) {
                return;
            }

            m_orientation = orient;

            // Recreate arrow widgets with correct direction
            // Remove old arrows (capture return value to avoid [[nodiscard]] warning)
            [[maybe_unused]] auto removed_dec = this->remove_child(m_arrow_dec);
            [[maybe_unused]] auto removed_inc = this->remove_child(m_arrow_inc);

            // Create new arrows with correct direction
            auto arrow_dec = std::make_unique<scrollbar_arrow<Backend>>(
                orient == orientation::vertical
                    ? arrow_direction::up
                    : arrow_direction::left
            );
            m_arrow_dec = arrow_dec.get();

            auto arrow_inc = std::make_unique<scrollbar_arrow<Backend>>(
                orient == orientation::vertical
                    ? arrow_direction::down
                    : arrow_direction::right
            );
            m_arrow_inc = arrow_inc.get();

            this->add_child(std::move(arrow_dec));
            this->add_child(std::move(arrow_inc));

            this->invalidate_measure();  // Dimensions swap
        }

        /**
         * @brief Get scrollbar orientation
         * @return Current orientation
         */
        [[nodiscard]] orientation get_orientation() const noexcept {
            return m_orientation;
        }

        /**
         * @brief Get calculated thumb bounds (for testing)
         * @return Rectangle representing thumb position and size
         * @note Only valid after arrange() has been called
         */
        [[nodiscard]] rect_type get_thumb_bounds() const noexcept {
            return m_thumb_bounds;
        }

        // Signals

        /**
         * @brief Emitted when user scrolls via scrollbar interaction
         * @param int New scroll position (x for horizontal, y for vertical)
         */
        signal<int> scroll_requested;

    protected:
        /**
         * @brief Measure scrollbar (fixed width/height based on orientation)
         * @param available_width Available width constraint
         * @param available_height Available height constraint
         * @return Desired size
         */
        logical_size do_measure(logical_unit available_width, logical_unit available_height) override {
            // CRITICAL FIX: If not visible, return zero size so grid collapses our row/column
            if (!this->is_visible()) {
                return logical_size{0_lu, 0_lu};
            }

            // Scrollbar has fixed thickness from theme
            // Length is content-based (parent determines)

            // Get theme to determine scrollbar dimensions
            auto const* themes = ui_services<Backend>::themes();
            auto const* theme = themes ? themes->get_current_theme() : nullptr;

            int thickness = theme ? theme->scrollbar.width : ui_constants::DEFAULT_SCROLLBAR_THICKNESS;
            int min_size = theme ? theme->scrollbar.min_render_size : ui_constants::DEFAULT_SCROLLBAR_MIN_RENDER_SIZE;

            // IMPORTANT: min_render_size is the minimum LENGTH (height for vertical, width for horizontal)
            // NOT the minimum thickness! Do NOT override thickness with min_size.

            // CRITICAL FIX: Don't use UNCONSTRAINED available space directly.
            // When measured with UNCONSTRAINED space (e.g., inside a scrollable that measures
            // children with infinite space), we should return min_size instead of INT_MAX.
            // This prevents scroll_view from measuring to INT_MAX which breaks nested scroll_views.
            constexpr int UNCONSTRAINED_THRESHOLD = std::numeric_limits<int>::max() / 2;

            int const avail_w = available_width.to_int();
            int const avail_h = available_height.to_int();

            logical_size result;
            if (m_orientation == orientation::horizontal) {
                // Horizontal: request width (capped if unconstrained), fixed thickness
                int const request_width = (avail_w > UNCONSTRAINED_THRESHOLD) ? min_size : std::max(avail_w, min_size);
                result = logical_size{logical_unit(static_cast<double>(request_width)), logical_unit(static_cast<double>(thickness))};
            } else {
                // Vertical: fixed thickness, request height (capped if unconstrained)
                int const request_height = (avail_h > UNCONSTRAINED_THRESHOLD) ? min_size : std::max(avail_h, min_size);
                result = logical_size{logical_unit(static_cast<double>(thickness)), logical_unit(static_cast<double>(request_height))};
            }

            return result;
        }

        /**
         * @brief Arrange scrollbar and position child widgets
         * @param final_bounds Final bounds assigned by parent
         */
        void do_arrange(const logical_rect& final_bounds) override {
            base::do_arrange(final_bounds);

            // Get theme to determine scrollbar style
            auto const* themes = ui_services<Backend>::themes();
            auto const* theme = themes ? themes->get_current_theme() : nullptr;

            if (!theme) {
                return;
            }

            scrollbar_style const style = theme->scrollbar.style;

            // Calculate component layout (returns RELATIVE logical coordinates)
            auto const layout = calculate_layout(style);

            // Calculate thumb bounds for legacy API compatibility (using old method)
            // Tests expect this to be relative to scrollbar widget (0,0), not track
            m_thumb_bounds = calculate_thumb_bounds();

            // Arrange children at calculated positions
            // Note: arrange() expects RELATIVE bounds (which calculate_layout provides)
            // Layout is now in logical_rect - can pass directly
            m_thumb->arrange(layout.thumb);

            // Only arrange arrows if they have non-zero size
            if (layout.has_arrows()) {
                m_arrow_dec->arrange(layout.arrow_decrement);
                m_arrow_inc->arrange(layout.arrow_increment);

                // Make arrows visible
                m_arrow_dec->set_visible(true);
                m_arrow_inc->set_visible(true);
            } else {
                // Hide arrows for minimal style
                m_arrow_dec->set_visible(false);
                m_arrow_inc->set_visible(false);
            }
        }

        /**
         * @brief Override event handling to intercept clicks in CAPTURE phase
         * @param evt Event to handle
         * @param phase Event routing phase
         * @return true if event was handled
         *
         * @details
         * Intercepts mouse events in CAPTURE phase to handle:
         * - Arrow button clicks for line scrolling
         * - Thumb press to start dragging (captures mouse to scrollbar)
         * - Mouse move during drag
         * - Mouse release to stop dragging
         *
         * This is necessary because child widgets (thumb, arrows) would otherwise
         * capture the mouse, preventing the scrollbar from receiving drag events.
         */
        bool handle_event(const ui_event& evt, event_phase phase) override {
            // Handle mouse events in CAPTURE phase (before children receive them)
            if (phase == event_phase::capture) {
                if (auto const* mouse = std::get_if<mouse_event>(&evt)) {
                    // Get theme for layout calculation
                    auto const* themes = ui_services<Backend>::themes();
                    auto const* theme = themes ? themes->get_current_theme() : nullptr;

                    if (theme) {
                        scrollbar_style const style = theme->scrollbar.style;
                        auto const layout = calculate_layout(style);

                        // Convert to scrollbar-relative logical coordinates
                        auto const abs_bounds = this->get_absolute_logical_bounds();
                        double const rel_x = mouse->x.value - abs_bounds.x.value;
                        double const rel_y = mouse->y.value - abs_bounds.y.value;

                        if (mouse->act == mouse_event::action::press) {
                            // Check if press is on arrow first (pass logical coordinates directly)
                            if (handle_arrow_click(mouse->x.value, mouse->y.value)) {
                                return true;
                            }

                            // Check if press is on track (including thumb area)
                            if (point_in_logical_rect(rel_x, rel_y, layout.track)) {
                                // Calculate scroll position based on click location (all in logical)
                                double const track_start = (m_orientation == orientation::vertical)
                                    ? layout.track.y.value
                                    : layout.track.x.value;
                                double const track_size = (m_orientation == orientation::vertical)
                                    ? layout.track.height.value
                                    : layout.track.width.value;
                                double const thumb_size = (m_orientation == orientation::vertical)
                                    ? layout.thumb.height.value
                                    : layout.thumb.width.value;
                                double const click_pos = (m_orientation == orientation::vertical)
                                    ? rel_y : rel_x;

                                // Content/viewport now use logical units (double)
                                double const content_size = (m_orientation == orientation::vertical)
                                    ? m_scroll_info.content_height
                                    : m_scroll_info.content_width;
                                double const viewport_size = (m_orientation == orientation::vertical)
                                    ? m_scroll_info.viewport_height
                                    : m_scroll_info.viewport_width;

                                double const max_scroll = content_size - viewport_size;
                                double const max_thumb_travel = track_size - thumb_size;

                                if (max_thumb_travel > 0.0 && max_scroll > 0.0) {
                                    // Center thumb on click position
                                    double const thumb_center_offset = click_pos - track_start - (thumb_size / 2.0);
                                    double const clamped_offset = std::clamp(thumb_center_offset, 0.0, max_thumb_travel);
                                    int const new_scroll = static_cast<int>((clamped_offset * max_scroll) / max_thumb_travel);

                                    scroll_requested.emit(new_scroll);

                                    // Start dragging from new position (use logical coordinates)
                                    m_dragging = true;
                                    m_drag_start_mouse = (m_orientation == orientation::vertical) ? mouse->y.value : mouse->x.value;
                                    m_drag_start_scroll = static_cast<double>(new_scroll);

                                    m_thumb->set_state(thumb_state::pressed);

                                    // Capture mouse to scrollbar so we receive drag events
                                    if (auto* input = ui_services<Backend>::input()) {
                                        input->set_capture(this);
                                    }
                                }
                                return true;  // Consume event
                            }
                        }
                    }
                }
            }

            // Handle drag events when we have mouse capture (TARGET phase)
            if (phase == event_phase::target && m_dragging) {
                if (auto const* mouse = std::get_if<mouse_event>(&evt)) {
                    if (mouse->act == mouse_event::action::move) {
                        // Calculate new scroll position based on mouse movement
                        auto const* themes = ui_services<Backend>::themes();
                        auto const* theme = themes ? themes->get_current_theme() : nullptr;

                        if (theme) {
                            scrollbar_style const style = theme->scrollbar.style;
                            auto const layout = calculate_layout(style);

                            // Use logical coordinates for mouse position
                            double const mouse_pos = (m_orientation == orientation::vertical)
                                ? mouse->y.value : mouse->x.value;
                            double const mouse_delta = mouse_pos - m_drag_start_mouse;

                            // Content/viewport now use logical units (double)
                            double const content_size = (m_orientation == orientation::vertical)
                                ? m_scroll_info.content_height
                                : m_scroll_info.content_width;
                            double const viewport_size = (m_orientation == orientation::vertical)
                                ? m_scroll_info.viewport_height
                                : m_scroll_info.viewport_width;

                            // Layout is now in logical coordinates
                            double const track_size = (m_orientation == orientation::vertical)
                                ? layout.track.height.value
                                : layout.track.width.value;
                            double const thumb_size = (m_orientation == orientation::vertical)
                                ? layout.thumb.height.value
                                : layout.thumb.width.value;

                            double const max_scroll = content_size - viewport_size;
                            double const max_thumb_travel = track_size - thumb_size;

                            if (max_thumb_travel > 0.0 && max_scroll > 0.0) {
                                double const scroll_delta = (mouse_delta * max_scroll) / max_thumb_travel;
                                int const new_scroll = std::clamp(
                                    static_cast<int>(m_drag_start_scroll + scroll_delta),
                                    0, static_cast<int>(max_scroll));
                                scroll_requested.emit(new_scroll);
                            }
                        }
                        return true;
                    }

                    if (mouse->act == mouse_event::action::release) {
                        m_dragging = false;
                        m_thumb->set_state(thumb_state::normal);

                        // Release mouse capture
                        if (auto* input = ui_services<Backend>::input()) {
                            input->release_capture();
                        }
                        return true;
                    }
                }
            }

            // Let base class handle other events
            return base::handle_event(evt, phase);
        }

        /**
         * @brief Internal helper to check and handle arrow clicks
         * @param x Mouse x coordinate (absolute)
         * @param y Mouse y coordinate (absolute)
         * @return true if click was on an arrow and handled
         *
         * @note Called from both handle_click (tests) and handle_event (CAPTURE phase)
         */
        bool handle_arrow_click(double x, double y) {
            // Get theme to determine scrollbar style
            auto const* themes = ui_services<Backend>::themes();
            if (!themes) {
                return false;
            }

            auto const* theme = themes->get_current_theme();
            if (!theme) {
                return false;
            }

            scrollbar_style const style = theme->scrollbar.style;

            // Calculate layout to get arrow button bounds (now in logical coordinates)
            auto const layout = calculate_layout(style);

            // Use logical bounds to get absolute position
            auto const abs_bounds = this->get_absolute_logical_bounds();

            // Convert absolute click coords to scrollbar-relative coords (in logical units)
            double const rel_x = x - abs_bounds.x.value;
            double const rel_y = y - abs_bounds.y.value;

            // Check if click is in decrement arrow
            if (layout.has_arrows() && point_in_logical_rect(rel_x, rel_y, layout.arrow_decrement)) {
                // CRITICAL FIX: scroll_requested expects ABSOLUTE position, not delta!
                // Get current scroll position and adjust it
                int const line_increment = theme->scrollbar.line_increment;
                double current_pos = m_orientation == orientation::vertical
                    ? m_scroll_info.scroll_y
                    : m_scroll_info.scroll_x;

                int new_pos = static_cast<int>(current_pos) - line_increment;  // Decrement
                scroll_requested.emit(new_pos);
                return true;
            }

            // Check if click is in increment arrow
            if (layout.has_arrows() && point_in_logical_rect(rel_x, rel_y, layout.arrow_increment)) {
                // CRITICAL FIX: scroll_requested expects ABSOLUTE position, not delta!
                // Get current scroll position and adjust it
                int const line_increment = theme->scrollbar.line_increment;
                double current_pos = m_orientation == orientation::vertical
                    ? m_scroll_info.scroll_y
                    : m_scroll_info.scroll_x;

                int new_pos = static_cast<int>(current_pos) + line_increment;  // Increment
                scroll_requested.emit(new_pos);
                return true;
            }

            // Click was not on an arrow button
            return false;
        }

        /**
         * @brief Handle mouse events (unified handler)
         * @param mouse Mouse event containing position, button, and action
         * @return true if handled
         *
         * @details
         * Handles mouse interactions with scrollbar components:
         * - Move: Updates hover states for thumb and arrows
         * - Press: Sets pressed states
         * - Release: Clears pressed states
         */
        bool handle_mouse(const mouse_event& mouse) override {
            // Track hover state before base class updates it
            bool const was_hovered = this->is_hovered();

            // Let base class handle the event first
            bool handled = base::handle_mouse(mouse);

            // Detect hover state changes
            bool const now_hovered = this->is_hovered();

            auto const* themes = ui_services<Backend>::themes();
            if (!themes) {
                return handled;
            }

            auto const* theme = themes->get_current_theme();
            if (!theme) {
                return handled;
            }

            scrollbar_style const style = theme->scrollbar.style;
            auto const layout = calculate_layout(style);

            // Convert absolute mouse coordinates to scrollbar-relative coordinates
            // Use get_absolute_logical_bounds() to preserve precision
            auto const abs_bounds = this->get_absolute_logical_bounds();
            double const abs_x = abs_bounds.x.value;
            double const abs_y = abs_bounds.y.value;
            double const mouse_x = mouse.x.value;
            double const mouse_y = mouse.y.value;
            double const rel_x = mouse_x - abs_x;
            double const rel_y = mouse_y - abs_y;

            // Dispatch based on action type
            switch (mouse.act) {
                case mouse_event::action::move:
                    // Handle thumb dragging
                    if (m_dragging) {
                        // Calculate new scroll position based on mouse movement (in logical coords)
                        double const mouse_pos = (m_orientation == orientation::vertical) ? mouse_y : mouse_x;
                        double const mouse_delta = mouse_pos - m_drag_start_mouse;

                        // Get track size and content/viewport info (now use logical units directly)
                        double const content_size = (m_orientation == orientation::vertical)
                            ? m_scroll_info.content_height
                            : m_scroll_info.content_width;
                        double const viewport_size = (m_orientation == orientation::vertical)
                            ? m_scroll_info.viewport_height
                            : m_scroll_info.viewport_width;

                        // Layout is now in logical coordinates
                        double const track_size = (m_orientation == orientation::vertical)
                            ? layout.track.height.value
                            : layout.track.width.value;
                        double const thumb_size = (m_orientation == orientation::vertical)
                            ? layout.thumb.height.value
                            : layout.thumb.width.value;

                        double const max_scroll = content_size - viewport_size;
                        double const max_thumb_travel = track_size - thumb_size;

                        if (max_thumb_travel > 0.0 && max_scroll > 0.0) {
                            // Convert mouse delta to scroll delta (mouse_delta is already double)
                            double const scroll_delta = (mouse_delta * max_scroll) / max_thumb_travel;
                            int const new_scroll = std::clamp(
                                static_cast<int>(m_drag_start_scroll + scroll_delta),
                                0, static_cast<int>(max_scroll));

                            scroll_requested.emit(new_scroll);
                        }
                        return true;  // Event handled
                    }

                    if (now_hovered) {
                        // Update thumb hover state (rel_x, rel_y are already double)
                        if (point_in_logical_rect(rel_x, rel_y, layout.thumb)) {
                            if (m_thumb->get_state() != thumb_state::pressed) {
                                m_thumb->set_state(thumb_state::hover);
                            }
                        } else if (m_thumb->get_state() == thumb_state::hover) {
                            m_thumb->set_state(thumb_state::normal);
                        }

                        // Update arrow hover states
                        if (layout.has_arrows()) {
                            bool const dec_hover = point_in_logical_rect(rel_x, rel_y, layout.arrow_decrement);
                            bool const inc_hover = point_in_logical_rect(rel_x, rel_y, layout.arrow_increment);

                            if (m_arrow_dec->get_state() != arrow_state::pressed) {
                                m_arrow_dec->set_state(dec_hover ? arrow_state::hover : arrow_state::normal);
                            }
                            if (m_arrow_inc->get_state() != arrow_state::pressed) {
                                m_arrow_inc->set_state(inc_hover ? arrow_state::hover : arrow_state::normal);
                            }
                        }
                    }
                    break;

                case mouse_event::action::press:
                    // Update thumb pressed state and start dragging (rel_x, rel_y are double)
                    if (point_in_logical_rect(rel_x, rel_y, layout.thumb)) {
                        m_thumb->set_state(thumb_state::pressed);

                        // Start thumb dragging (store logical coordinate)
                        m_dragging = true;
                        m_drag_start_mouse = (m_orientation == orientation::vertical) ? mouse_y : mouse_x;
                        m_drag_start_scroll = (m_orientation == orientation::vertical)
                            ? m_scroll_info.scroll_y
                            : m_scroll_info.scroll_x;
                    }

                    // Update arrow pressed states
                    if (layout.has_arrows()) {
                        if (point_in_logical_rect(rel_x, rel_y, layout.arrow_decrement)) {
                            m_arrow_dec->set_state(arrow_state::pressed);
                        }
                        if (point_in_logical_rect(rel_x, rel_y, layout.arrow_increment)) {
                            m_arrow_inc->set_state(arrow_state::pressed);
                        }
                    }
                    break;

                case mouse_event::action::release:
                    // Stop thumb dragging
                    m_dragging = false;

                    // Clear pressed states on all children
                    if (m_thumb->get_state() == thumb_state::pressed) {
                        m_thumb->set_state(thumb_state::normal);
                    }

                    if (m_arrow_dec->get_state() == arrow_state::pressed) {
                        m_arrow_dec->set_state(arrow_state::normal);
                    }

                    if (m_arrow_inc->get_state() == arrow_state::pressed) {
                        m_arrow_inc->set_state(arrow_state::normal);
                    }

                    // Handle arrow clicks (release inside generates click)
                    if (handled) {  // Click was generated by base class
                        handle_arrow_click(mouse_x, mouse_y);
                    }
                    break;

                case mouse_event::action::wheel_up:
                case mouse_event::action::wheel_down:
                    break;
            }

            // Handle mouse leave (hover state changed from true to false)
            if (was_hovered && !now_hovered) {
                // Stop dragging if mouse leaves while dragging
                // (Some backends may not send release events when mouse leaves)
                m_dragging = false;

                // Clear hover states on all children
                if (m_thumb->get_state() == thumb_state::hover) {
                    m_thumb->set_state(thumb_state::normal);
                }

                if (m_arrow_dec->get_state() == arrow_state::hover) {
                    m_arrow_dec->set_state(arrow_state::normal);
                }

                if (m_arrow_inc->get_state() == arrow_state::hover) {
                    m_arrow_inc->set_state(arrow_state::normal);
                }
            }

            return handled;
        }

        /**
         * @brief Render scrollbar track (children render themselves)
         * @param ctx Render context
         */
        void do_render(render_context<Backend>& ctx) const override {
            // Get theme to determine scrollbar style
            auto const* theme = ctx.theme();
            if (!theme) {
                return;
            }

            // Get physical position from render context
            int const base_x = point_utils::get_x(ctx.position());
            int const base_y = point_utils::get_y(ctx.position());

            // Get dimensions using get_final_dims pattern (handles measurement vs rendering)
            auto const logical_bounds = this->bounds();
            auto const [width, height] = ctx.get_final_dims(
                logical_bounds.width.to_int(), logical_bounds.height.to_int());

            // CRITICAL FIX: Don't render if scrollbar is too small
            // min_render_size is in logical units, compare against logical bounds
            int const logical_w = logical_bounds.width.to_int();
            int const logical_h = logical_bounds.height.to_int();
            int const min_size = theme->scrollbar.min_render_size;

            if (m_orientation == orientation::vertical && logical_h < min_size) {
                return;
            }
            if (m_orientation == orientation::horizontal && logical_w < min_size) {
                return;
            }

            scrollbar_style const style = theme->scrollbar.style;

            // Calculate track bounds in physical coordinates
            // Track is the scrollbar bounds minus arrow space (for non-minimal styles)
            rect_type abs_track;
            int const arrow_size_logical = theme->scrollbar.arrow_size;

            // Calculate scale factor from logical to physical
            double const scale_x = (logical_w > 0) ? static_cast<double>(width) / logical_w : 1.0;
            double const scale_y = (logical_h > 0) ? static_cast<double>(height) / logical_h : 1.0;
            int const arrow_size_phys = (m_orientation == orientation::horizontal)
                ? static_cast<int>(arrow_size_logical * scale_x)
                : static_cast<int>(arrow_size_logical * scale_y);

            if (m_orientation == orientation::horizontal) {
                if (style == scrollbar_style::minimal) {
                    rect_utils::set_bounds(abs_track, base_x, base_y, width, height);
                } else if (style == scrollbar_style::compact) {
                    int const track_width = std::max(0, width - arrow_size_phys);
                    rect_utils::set_bounds(abs_track, base_x, base_y, track_width, height);
                } else { // classic
                    int const track_width = std::max(0, width - 2 * arrow_size_phys);
                    rect_utils::set_bounds(abs_track, base_x + arrow_size_phys, base_y, track_width, height);
                }
            } else {
                if (style == scrollbar_style::minimal) {
                    rect_utils::set_bounds(abs_track, base_x, base_y, width, height);
                } else if (style == scrollbar_style::compact) {
                    int const track_height = std::max(0, height - arrow_size_phys);
                    rect_utils::set_bounds(abs_track, base_x, base_y, width, track_height);
                } else { // classic
                    int const track_height = std::max(0, height - 2 * arrow_size_phys);
                    rect_utils::set_bounds(abs_track, base_x, base_y + arrow_size_phys, width, track_height);
                }
            }

            // Only draw track background - children render themselves!
            ctx.draw_rect(abs_track, theme->scrollbar.track_normal.box_style);

            // Children (thumb, arrows) are rendered automatically by widget_container
        }

    protected:
        /**
         * @brief Check if a point is inside a rectangle
         * @param pt Point to test
         * @param rect Rectangle bounds
         * @return true if point is inside rectangle
         */
        [[nodiscard]] static bool point_in_rect(const point_type& pt, const rect_type& rect) noexcept {
            int const px = point_utils::get_x(pt);
            int const py = point_utils::get_y(pt);
            int const rx = rect_utils::get_x(rect);
            int const ry = rect_utils::get_y(rect);
            int const rw = rect_utils::get_width(rect);
            int const rh = rect_utils::get_height(rect);

            return px >= rx && px < rx + rw &&
                   py >= ry && py < ry + rh;
        }

        /**
         * @brief Check if a point (in logical units) is within a logical rectangle
         * @param x Logical X coordinate
         * @param y Logical Y coordinate
         * @param rect Logical rectangle to check against
         * @return true if point is inside rectangle
         */
        [[nodiscard]] static bool point_in_logical_rect(double x, double y, const logical_rect& rect) noexcept {
            return x >= rect.x.value && x < rect.x.value + rect.width.value &&
                   y >= rect.y.value && y < rect.y.value + rect.height.value;
        }

        /**
         * @struct scrollbar_layout
         * @brief Layout information for scrollbar components (in logical coordinates)
         *
         * @details
         * Holds calculated rectangles for all scrollbar components based on style:
         * - minimal: Only track and thumb (no arrows)
         * - compact: Track, thumb, and arrows at one end
         * - classic: Track, thumb, and arrows at both ends
         *
         * All rectangles are in logical coordinates (DPI-independent).
         */
        struct scrollbar_layout {
            logical_rect track{};           ///< Background track rectangle (logical)
            logical_rect thumb{};           ///< Draggable thumb rectangle (logical)
            logical_rect arrow_decrement{}; ///< Up/Left arrow button (empty for minimal)
            logical_rect arrow_increment{}; ///< Down/Right arrow button (empty for minimal)

            /**
             * @brief Check if layout has arrow buttons
             * @return true if arrows are present (non-zero size)
             */
            [[nodiscard]] bool has_arrows() const noexcept {
                return arrow_decrement.width.value > 0 ||
                       arrow_decrement.height.value > 0;
            }
        };

        /**
         * @brief Calculate complete scrollbar layout based on style
         * @param style Scrollbar visual style (minimal/compact/classic)
         * @return Layout with track, thumb, and arrow rectangles
         *
         * @details
         * Layout strategies:
         * - minimal: Track = full bounds, thumb within track, no arrows
         * - compact: Arrows at one end (bottom/right), track fills rest
         * - classic: Arrows at both ends, track in middle
         */
        [[nodiscard]] scrollbar_layout calculate_layout(scrollbar_style style) const {
            scrollbar_layout layout;

            auto const bounds = this->bounds();
            // CRITICAL FIX: Use (0,0) origin for layout - coordinates should be relative
            // to the scrollbar widget itself, NOT relative to parent grid!
            // do_render() will add ctx.position() to convert to absolute screen coords
            logical_unit const x{0.0};
            logical_unit const y{0.0};
            logical_unit const w = bounds.width;
            logical_unit const h = bounds.height;

            // Scroll info values (now stored as logical units - double)
            double const content_w = m_scroll_info.content_width;
            double const content_h = m_scroll_info.content_height;
            double const viewport_w = m_scroll_info.viewport_width;
            double const viewport_h = m_scroll_info.viewport_height;
            double const scroll_x = m_scroll_info.scroll_x;
            double const scroll_y = m_scroll_info.scroll_y;

            // Get dimensions from theme (as logical units)
            auto const* themes = ui_services<Backend>::themes();
            auto const* theme = themes ? themes->get_current_theme() : nullptr;

            logical_unit const min_thumb_size{static_cast<double>(
                theme ? theme->scrollbar.min_thumb_size : ui_constants::DEFAULT_SCROLLBAR_MIN_THUMB_SIZE)};
            logical_unit const arrow_size{static_cast<double>(
                theme ? theme->scrollbar.arrow_size : ui_constants::DEFAULT_SCROLLBAR_ARROW_SIZE)};

            if (m_orientation == orientation::horizontal) {
                // Horizontal scrollbar layout

                if (style == scrollbar_style::minimal) {
                    // No arrows - track fills entire bounds
                    layout.track = logical_rect{x, y, w, h};
                } else if (style == scrollbar_style::compact) {
                    // Arrows at right end only
                    logical_unit const track_width = (w > arrow_size) ? (w - arrow_size) : 0.0_lu;
                    layout.arrow_decrement = logical_rect{x + w - arrow_size, y, arrow_size, h};
                    layout.track = logical_rect{x, y, track_width, h};
                } else { // classic
                    // Arrows at both ends
                    logical_unit const double_arrow = arrow_size + arrow_size;
                    logical_unit const track_width = (w > double_arrow) ? (w - double_arrow) : 0.0_lu;
                    layout.arrow_decrement = logical_rect{x, y, arrow_size, h};
                    layout.arrow_increment = logical_rect{x + w - arrow_size, y, arrow_size, h};
                    layout.track = logical_rect{x + arrow_size, y, track_width, h};
                }

                // Calculate thumb within track
                logical_unit const track_x = layout.track.x;
                double const track_w = layout.track.width.value;

                if (content_w <= viewport_w || track_w <= 0.0 || viewport_w <= 0.0) {
                    // No scrolling needed - zero-sized thumb
                    layout.thumb = logical_rect{track_x, y, 0.0_lu, h};
                } else {
                    // Calculate proportional thumb size
                    double thumb_length = std::max(min_thumb_size.value,
                        (viewport_w / content_w) * track_w);
                    thumb_length = std::min(thumb_length, track_w);

                    // Calculate thumb position
                    double const max_scroll = content_w - viewport_w;
                    double const max_thumb_pos = track_w - thumb_length;
                    double thumb_pos = 0.0;

                    if (max_scroll > 0.0) {
                        double const clamped_scroll = std::clamp(scroll_x, 0.0, max_scroll);
                        thumb_pos = (clamped_scroll / max_scroll) * max_thumb_pos;
                    }

                    thumb_pos = std::clamp(thumb_pos, 0.0, max_thumb_pos);
                    layout.thumb = logical_rect{track_x + logical_unit{thumb_pos}, y, logical_unit{thumb_length}, h};
                }
            } else {
                // Vertical scrollbar layout

                if (style == scrollbar_style::minimal) {
                    // No arrows - track fills entire bounds
                    layout.track = logical_rect{x, y, w, h};
                } else if (style == scrollbar_style::compact) {
                    // Arrows at bottom end only
                    logical_unit const track_height = (h > arrow_size) ? (h - arrow_size) : 0.0_lu;
                    layout.arrow_increment = logical_rect{x, y + h - arrow_size, w, arrow_size};
                    layout.track = logical_rect{x, y, w, track_height};
                } else { // classic
                    // Arrows at both ends
                    logical_unit const double_arrow = arrow_size + arrow_size;
                    logical_unit const track_height = (h > double_arrow) ? (h - double_arrow) : 0.0_lu;
                    layout.arrow_decrement = logical_rect{x, y, w, arrow_size};
                    layout.arrow_increment = logical_rect{x, y + h - arrow_size, w, arrow_size};
                    layout.track = logical_rect{x, y + arrow_size, w, track_height};
                }

                // Calculate thumb within track
                logical_unit const track_y = layout.track.y;
                double const track_h = layout.track.height.value;

                if (content_h <= viewport_h || track_h <= 0.0 || viewport_h <= 0.0) {
                    // No scrolling needed - zero-sized thumb
                    layout.thumb = logical_rect{x, track_y, w, 0.0_lu};
                } else {
                    // Calculate proportional thumb size
                    double thumb_length = std::max(min_thumb_size.value,
                        (viewport_h / content_h) * track_h);
                    thumb_length = std::min(thumb_length, track_h);

                    // Calculate thumb position
                    double const max_scroll = content_h - viewport_h;
                    double const max_thumb_pos = track_h - thumb_length;
                    double thumb_pos = 0.0;

                    if (max_scroll > 0.0) {
                        double const clamped_scroll = std::clamp(scroll_y, 0.0, max_scroll);
                        thumb_pos = (clamped_scroll / max_scroll) * max_thumb_pos;
                    }

                    thumb_pos = std::clamp(thumb_pos, 0.0, max_thumb_pos);
                    layout.thumb = logical_rect{x, track_y + logical_unit{thumb_pos}, w, logical_unit{thumb_length}};
                }
            }

            return layout;
        }

        /**
         * @brief Calculate thumb bounds based on scroll_info
         * @return Rectangle representing thumb position/size
         * @note Legacy method - use calculate_layout() for style-aware layout
         */
        [[nodiscard]] rect_type calculate_thumb_bounds() const {
            auto const bounds = this->bounds();
            int const x = bounds.x.to_int();
            int const y = bounds.y.to_int();
            int const w = bounds.width.to_int();
            int const h = bounds.height.to_int();

            // Use double for calculations, convert to int for final rect
            double const content_w = m_scroll_info.content_width;
            double const content_h = m_scroll_info.content_height;
            double const viewport_w = m_scroll_info.viewport_width;
            double const viewport_h = m_scroll_info.viewport_height;
            double const scroll_x = m_scroll_info.scroll_x;
            double const scroll_y = m_scroll_info.scroll_y;

            // Get min thumb size from theme
            auto const* themes = ui_services<Backend>::themes();
            int const min_thumb_size = (themes && themes->get_current_theme())
                ? themes->get_current_theme()->scrollbar.min_thumb_size
                : ui_constants::DEFAULT_SCROLLBAR_MIN_THUMB_SIZE;

            if (m_orientation == orientation::horizontal) {
                // Horizontal scrollbar
                double const track_length = static_cast<double>(w);  // Full width is track

                if (content_w <= viewport_w || track_length <= 0.0 || viewport_w <= 0.0) {
                    // No scrolling needed, invalid track, or invalid viewport
                    return rect_type{x, y, 0, 0};  // Zero-sized thumb
                }

                // Calculate thumb size (proportional to viewport/content ratio)
                double thumb_length = std::max(static_cast<double>(min_thumb_size),
                    (viewport_w / content_w) * track_length);

                // Clamp thumb to track
                thumb_length = std::min(thumb_length, track_length);

                // Calculate thumb position (proportional to scroll offset)
                double const max_scroll = content_w - viewport_w;
                double const max_thumb_pos = track_length - thumb_length;

                double thumb_pos = 0.0;
                if (max_scroll > 0.0) {
                    // Clamp scroll offset to valid range
                    double const clamped_scroll = std::clamp(scroll_x, 0.0, max_scroll);
                    thumb_pos = (clamped_scroll / max_scroll) * max_thumb_pos;
                }

                // Ensure thumb position stays within track bounds
                thumb_pos = std::clamp(thumb_pos, 0.0, max_thumb_pos);

                return rect_type{x + static_cast<int>(thumb_pos), y, static_cast<int>(thumb_length), h};
            } else {
                // Vertical scrollbar
                double const track_length = static_cast<double>(h);  // Full height is track

                if (content_h <= viewport_h || track_length <= 0.0 || viewport_h <= 0.0) {
                    // No scrolling needed, invalid track, or invalid viewport
                    return rect_type{x, y, 0, 0};  // Zero-sized thumb
                }

                // Calculate thumb size (proportional to viewport/content ratio)
                double thumb_length = std::max(static_cast<double>(min_thumb_size),
                    (viewport_h / content_h) * track_length);

                // Clamp thumb to track
                thumb_length = std::min(thumb_length, track_length);

                // Calculate thumb position (proportional to scroll offset)
                double const max_scroll = content_h - viewport_h;
                double const max_thumb_pos = track_length - thumb_length;

                double thumb_pos = 0.0;
                if (max_scroll > 0.0) {
                    // Clamp scroll offset to valid range
                    double const clamped_scroll = std::clamp(scroll_y, 0.0, max_scroll);
                    thumb_pos = (clamped_scroll / max_scroll) * max_thumb_pos;
                }

                // Ensure thumb position stays within track bounds
                thumb_pos = std::clamp(thumb_pos, 0.0, max_thumb_pos);

                return rect_type{x, y + static_cast<int>(thumb_pos), w, static_cast<int>(thumb_length)};
            }
        }

        /**
         * @brief Update child widget arrangement without full re-layout
         * @note Used when scroll_info changes to immediately update thumb bounds
         */
        void update_child_arrangement() {
            // Get theme to determine scrollbar style
            auto const* themes = ui_services<Backend>::themes();
            auto const* theme = themes ? themes->get_current_theme() : nullptr;

            if (!theme) {
                return;
            }

            scrollbar_style const style = theme->scrollbar.style;

            // Calculate component layout (returns RELATIVE logical coordinates)
            auto const layout = calculate_layout(style);

            // Update thumb bounds for legacy API compatibility
            m_thumb_bounds = calculate_thumb_bounds();

            // Arrange children at calculated positions
            // Layout is now in logical_rect - can pass directly
            m_thumb->arrange(layout.thumb);

            // Only arrange arrows if they have non-zero size
            if (layout.has_arrows()) {
                m_arrow_dec->arrange(layout.arrow_decrement);
                m_arrow_inc->arrange(layout.arrow_increment);
                m_arrow_dec->set_visible(true);
                m_arrow_inc->set_visible(true);
            } else {
                m_arrow_dec->set_visible(false);
                m_arrow_inc->set_visible(false);
            }
        }

    private:
        orientation m_orientation = orientation::vertical;  ///< Horizontal or vertical
        scroll_info<Backend> m_scroll_info{};               ///< Current scroll state
        rect_type m_thumb_bounds{};                         ///< Calculated thumb bounds (legacy API)

        // Raw pointers to child widgets (ownership held by widget_container)
        scrollbar_thumb<Backend>* m_thumb = nullptr;        ///< Thumb widget
        scrollbar_arrow<Backend>* m_arrow_dec = nullptr;    ///< Decrement arrow (up/left)
        scrollbar_arrow<Backend>* m_arrow_inc = nullptr;    ///< Increment arrow (down/right)

        // Drag state for thumb dragging
        bool m_dragging = false;           ///< True while thumb is being dragged
        double m_drag_start_mouse = 0.0;   ///< Mouse position at drag start (logical, x or y based on orientation)
        double m_drag_start_scroll = 0.0;  ///< Scroll position at drag start
    };

} // namespace onyxui
