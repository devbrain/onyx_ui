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
            if (rect_utils::get_width(bounds) > 0 && rect_utils::get_height(bounds) > 0) {
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
        size_type do_measure(int available_width, int available_height) override {
            // CRITICAL FIX: If not visible, return zero size so grid collapses our row/column
            if (!this->is_visible()) {
                return size_type{0, 0};
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

            size_type result;
            if (m_orientation == orientation::horizontal) {
                // Horizontal: request full width (at least min_render_size), fixed thickness
                result = size_type{std::max(available_width, min_size), thickness};
            } else {
                // Vertical: fixed thickness, request full height (at least min_render_size)
                result = size_type{thickness, std::max(available_height, min_size)};
            }

            return result;
        }

        /**
         * @brief Arrange scrollbar and position child widgets
         * @param final_bounds Final bounds assigned by parent
         */
        void do_arrange(const rect_type& final_bounds) override {
            base::do_arrange(final_bounds);

            // Get theme to determine scrollbar style
            auto const* themes = ui_services<Backend>::themes();
            auto const* theme = themes ? themes->get_current_theme() : nullptr;

            if (!theme) {
                return;
            }

            scrollbar_style const style = theme->scrollbar.style;

            // Calculate component layout (returns RELATIVE coordinates)
            auto const layout = calculate_layout(style);

            // Calculate thumb bounds for legacy API compatibility (using old method)
            // Tests expect this to be relative to scrollbar widget (0,0), not track
            m_thumb_bounds = calculate_thumb_bounds();

            // Arrange children at calculated positions
            // Note: arrange() expects RELATIVE bounds (which calculate_layout provides)
            m_thumb->arrange(geometry::relative_rect<Backend>{layout.thumb});

            // Only arrange arrows if they have non-zero size
            if (layout.has_arrows()) {
                m_arrow_dec->arrange(geometry::relative_rect<Backend>{layout.arrow_decrement});
                m_arrow_inc->arrange(geometry::relative_rect<Backend>{layout.arrow_increment});

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
         * @brief Override event handling to intercept arrow clicks in CAPTURE phase
         * @param evt Event to handle
         * @param phase Event routing phase
         * @return true if event was handled
         *
         * @details
         * Since arrow buttons are child widgets, with the fixed hit_test they now
         * receive events directly. We need to intercept mouse button press events
         * in CAPTURE phase (before they reach the arrow children) to handle scrolling.
         */
        bool handle_event(const ui_event& evt, event_phase phase) override {
            // Only intercept in CAPTURE phase (before children receive events)
            if (phase == event_phase::capture) {
                // Check if this is a mouse button press event
                if (auto const* mouse = std::get_if<mouse_event>(&evt)) {
                    if (mouse->act == mouse_event::action::press) {
                        // Try to handle as arrow click
                        if (handle_arrow_click(mouse->x, mouse->y)) {
                            return true;  // Stop propagation - we handled it
                        }
                    }
                }
            }

            // Let base class handle the event (will route to children if not handled)
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
        bool handle_arrow_click(int x, int y) {
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

            // Calculate layout to get arrow button bounds
            auto const layout = calculate_layout(style);

            // Use systematic API to get absolute bounds
            rect_type const abs_bounds = this->get_absolute_bounds().get();
            int const abs_x = rect_utils::get_x(abs_bounds);
            int const abs_y = rect_utils::get_y(abs_bounds);

            // Convert absolute click coords to scrollbar-relative coords
            int const rel_x = x - abs_x;
            int const rel_y = y - abs_y;
            point_type const rel_point{rel_x, rel_y};

            // Check if click is in decrement arrow
            if (layout.has_arrows() && point_in_rect(rel_point, layout.arrow_decrement)) {
                // CRITICAL FIX: scroll_requested expects ABSOLUTE position, not delta!
                // Get current scroll position and adjust it
                int const line_increment = theme->scrollbar.line_increment;
                int current_pos = m_orientation == orientation::vertical
                    ? point_utils::get_y(m_scroll_info.scroll_offset)
                    : point_utils::get_x(m_scroll_info.scroll_offset);

                int new_pos = current_pos - line_increment;  // Decrement
                scroll_requested.emit(new_pos);
                return true;
            }

            // Check if click is in increment arrow
            if (layout.has_arrows() && point_in_rect(rel_point, layout.arrow_increment)) {
                // CRITICAL FIX: scroll_requested expects ABSOLUTE position, not delta!
                // Get current scroll position and adjust it
                int const line_increment = theme->scrollbar.line_increment;
                int current_pos = m_orientation == orientation::vertical
                    ? point_utils::get_y(m_scroll_info.scroll_offset)
                    : point_utils::get_x(m_scroll_info.scroll_offset);

                int new_pos = current_pos + line_increment;  // Increment
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

            // Convert to relative coordinates
            auto const bounds = this->bounds();
            int const rel_x = mouse.x - rect_utils::get_x(bounds);
            int const rel_y = mouse.y - rect_utils::get_y(bounds);
            point_type const rel_point{rel_x, rel_y};

            // Dispatch based on action type
            switch (mouse.act) {
                case mouse_event::action::move:
                    if (now_hovered) {
                        // Update thumb hover state
                        if (point_in_rect(rel_point, layout.thumb)) {
                            if (m_thumb->get_state() != thumb_state::pressed) {
                                m_thumb->set_state(thumb_state::hover);
                            }
                        } else if (m_thumb->get_state() == thumb_state::hover) {
                            m_thumb->set_state(thumb_state::normal);
                        }

                        // Update arrow hover states
                        if (layout.has_arrows()) {
                            bool const dec_hover = point_in_rect(rel_point, layout.arrow_decrement);
                            bool const inc_hover = point_in_rect(rel_point, layout.arrow_increment);

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
                    // Update thumb pressed state
                    if (point_in_rect(rel_point, layout.thumb)) {
                        m_thumb->set_state(thumb_state::pressed);
                    }

                    // Update arrow pressed states
                    if (layout.has_arrows()) {
                        if (point_in_rect(rel_point, layout.arrow_decrement)) {
                            m_arrow_dec->set_state(arrow_state::pressed);
                        }
                        if (point_in_rect(rel_point, layout.arrow_increment)) {
                            m_arrow_inc->set_state(arrow_state::pressed);
                        }
                    }
                    break;

                case mouse_event::action::release:
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
                        handle_arrow_click(mouse.x, mouse.y);
                    }
                    break;

                case mouse_event::action::wheel_up:
                case mouse_event::action::wheel_down:
                    break;
            }

            // Handle mouse leave (hover state changed from true to false)
            if (was_hovered && !now_hovered) {
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

            // CRITICAL FIX: Don't render if scrollbar is too small
            auto const bounds = this->bounds();
            int const width = rect_utils::get_width(bounds);
            int const height = rect_utils::get_height(bounds);
            int const min_size = theme->scrollbar.min_render_size;

            if (m_orientation == orientation::vertical && height < min_size) {
                return;
            }
            if (m_orientation == orientation::horizontal && width < min_size) {
                return;
            }

            scrollbar_style const style = theme->scrollbar.style;
            auto const layout = calculate_layout(style);

            // Convert track from relative to absolute coordinates
            rect_type abs_track;
            int const base_x = point_utils::get_x(ctx.position());
            int const base_y = point_utils::get_y(ctx.position());

            rect_utils::set_bounds(abs_track,
                base_x + rect_utils::get_x(layout.track),
                base_y + rect_utils::get_y(layout.track),
                rect_utils::get_width(layout.track),
                rect_utils::get_height(layout.track));

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
         * @struct scrollbar_layout
         * @brief Layout information for scrollbar components
         *
         * @details
         * Holds calculated rectangles for all scrollbar components based on style:
         * - minimal: Only track and thumb (no arrows)
         * - compact: Track, thumb, and arrows at one end
         * - classic: Track, thumb, and arrows at both ends
         */
        struct scrollbar_layout {
            rect_type track{};           ///< Background track rectangle
            rect_type thumb{};           ///< Draggable thumb rectangle
            rect_type arrow_decrement{}; ///< Up/Left arrow button (empty for minimal)
            rect_type arrow_increment{}; ///< Down/Right arrow button (empty for minimal)

            /**
             * @brief Check if layout has arrow buttons
             * @return true if arrows are present (non-zero size)
             */
            [[nodiscard]] bool has_arrows() const noexcept {
                return rect_utils::get_width(arrow_decrement) > 0 ||
                       rect_utils::get_height(arrow_decrement) > 0;
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
            int const x = 0;
            int const y = 0;
            int const w = rect_utils::get_width(bounds);
            int const h = rect_utils::get_height(bounds);

            int const content_w = size_utils::get_width(m_scroll_info.content_size);
            int const content_h = size_utils::get_height(m_scroll_info.content_size);
            int const viewport_w = size_utils::get_width(m_scroll_info.viewport_size);
            int const viewport_h = size_utils::get_height(m_scroll_info.viewport_size);
            int const scroll_x = point_utils::get_x(m_scroll_info.scroll_offset);
            int const scroll_y = point_utils::get_y(m_scroll_info.scroll_offset);

            // Get dimensions from theme
            auto const* themes = ui_services<Backend>::themes();
            auto const* theme = themes ? themes->get_current_theme() : nullptr;

            int const min_thumb_size = theme ? theme->scrollbar.min_thumb_size : ui_constants::DEFAULT_SCROLLBAR_MIN_THUMB_SIZE;
            int const arrow_size = theme ? theme->scrollbar.arrow_size : ui_constants::DEFAULT_SCROLLBAR_ARROW_SIZE;

            if (m_orientation == orientation::horizontal) {
                // Horizontal scrollbar layout

                if (style == scrollbar_style::minimal) {
                    // No arrows - track fills entire bounds
                    rect_utils::set_bounds(layout.track, x, y, w, h);
                } else if (style == scrollbar_style::compact) {
                    // Arrows at right end only
                    int const track_width = std::max(0, w - arrow_size);
                    rect_utils::set_bounds(layout.arrow_decrement, x + w - arrow_size, y, arrow_size, h);
                    rect_utils::set_bounds(layout.track, x, y, track_width, h);
                } else { // classic
                    // Arrows at both ends
                    int const track_width = std::max(0, w - 2 * arrow_size);
                    rect_utils::set_bounds(layout.arrow_decrement, x, y, arrow_size, h);
                    rect_utils::set_bounds(layout.arrow_increment, x + w - arrow_size, y, arrow_size, h);
                    rect_utils::set_bounds(layout.track, x + arrow_size, y, track_width, h);
                }

                // Calculate thumb within track
                int const track_x = rect_utils::get_x(layout.track);
                int const track_w = rect_utils::get_width(layout.track);

                if (content_w <= viewport_w || track_w == 0 || viewport_w == 0) {
                    // No scrolling needed - zero-sized thumb
                    rect_utils::set_bounds(layout.thumb, track_x, y, 0, h);
                } else {
                    // Calculate proportional thumb size
                    int thumb_length = std::max(min_thumb_size,
                        static_cast<int>((static_cast<float>(viewport_w) / static_cast<float>(content_w)) * static_cast<float>(track_w)));
                    thumb_length = std::min(thumb_length, track_w);

                    // Calculate thumb position
                    int const max_scroll = content_w - viewport_w;
                    int const max_thumb_pos = track_w - thumb_length;
                    int thumb_pos = 0;

                    if (max_scroll > 0) {
                        int const clamped_scroll = std::clamp(scroll_x, 0, max_scroll);
                        thumb_pos = static_cast<int>((static_cast<float>(clamped_scroll) / static_cast<float>(max_scroll)) * static_cast<float>(max_thumb_pos));
                    }

                    thumb_pos = std::clamp(thumb_pos, 0, max_thumb_pos);
                    rect_utils::set_bounds(layout.thumb, track_x + thumb_pos, y, thumb_length, h);
                }
            } else {
                // Vertical scrollbar layout
                // arrow_size already obtained from theme above

                if (style == scrollbar_style::minimal) {
                    // No arrows - track fills entire bounds
                    rect_utils::set_bounds(layout.track, x, y, w, h);
                } else if (style == scrollbar_style::compact) {
                    // Arrows at bottom end only
                    int const track_height = std::max(0, h - arrow_size);
                    rect_utils::set_bounds(layout.arrow_increment, x, y + h - arrow_size, w, arrow_size);
                    rect_utils::set_bounds(layout.track, x, y, w, track_height);
                } else { // classic
                    // Arrows at both ends
                    int const track_height = std::max(0, h - 2 * arrow_size);
                    rect_utils::set_bounds(layout.arrow_decrement, x, y, w, arrow_size);
                    rect_utils::set_bounds(layout.arrow_increment, x, y + h - arrow_size, w, arrow_size);
                    rect_utils::set_bounds(layout.track, x, y + arrow_size, w, track_height);
                }

                // Calculate thumb within track
                int const track_y = rect_utils::get_y(layout.track);
                int const track_h = rect_utils::get_height(layout.track);

                if (content_h <= viewport_h || track_h == 0 || viewport_h == 0) {
                    // No scrolling needed - zero-sized thumb
                    rect_utils::set_bounds(layout.thumb, x, track_y, w, 0);
                } else {
                    // Calculate proportional thumb size
                    int thumb_length = std::max(min_thumb_size,
                        static_cast<int>((static_cast<float>(viewport_h) / static_cast<float>(content_h)) * static_cast<float>(track_h)));
                    thumb_length = std::min(thumb_length, track_h);

                    // Calculate thumb position
                    int const max_scroll = content_h - viewport_h;
                    int const max_thumb_pos = track_h - thumb_length;
                    int thumb_pos = 0;

                    if (max_scroll > 0) {
                        int const clamped_scroll = std::clamp(scroll_y, 0, max_scroll);
                        thumb_pos = static_cast<int>((static_cast<float>(clamped_scroll) / static_cast<float>(max_scroll)) * static_cast<float>(max_thumb_pos));
                    }

                    thumb_pos = std::clamp(thumb_pos, 0, max_thumb_pos);
                    rect_utils::set_bounds(layout.thumb, x, track_y + thumb_pos, w, thumb_length);
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
            int const x = rect_utils::get_x(bounds);
            int const y = rect_utils::get_y(bounds);
            int const w = rect_utils::get_width(bounds);
            int const h = rect_utils::get_height(bounds);

            int const content_w = size_utils::get_width(m_scroll_info.content_size);
            int const content_h = size_utils::get_height(m_scroll_info.content_size);
            int const viewport_w = size_utils::get_width(m_scroll_info.viewport_size);
            int const viewport_h = size_utils::get_height(m_scroll_info.viewport_size);
            int const scroll_x = point_utils::get_x(m_scroll_info.scroll_offset);
            int const scroll_y = point_utils::get_y(m_scroll_info.scroll_offset);

            // Get min thumb size from theme
            auto const* themes = ui_services<Backend>::themes();
            int const min_thumb_size = (themes && themes->get_current_theme())
                ? themes->get_current_theme()->scrollbar.min_thumb_size
                : ui_constants::DEFAULT_SCROLLBAR_MIN_THUMB_SIZE;

            if (m_orientation == orientation::horizontal) {
                // Horizontal scrollbar
                int const track_length = w;  // Full width is track

                if (content_w <= viewport_w || track_length == 0 || viewport_w == 0) {
                    // No scrolling needed, invalid track, or invalid viewport
                    return rect_type{x, y, 0, 0};  // Zero-sized thumb
                }

                // Calculate thumb size (proportional to viewport/content ratio)
                int thumb_length = std::max(min_thumb_size,
                    static_cast<int>((static_cast<float>(viewport_w) / static_cast<float>(content_w)) * static_cast<float>(track_length)));

                // Clamp thumb to track
                thumb_length = std::min(thumb_length, track_length);

                // Calculate thumb position (proportional to scroll offset)
                int const max_scroll = content_w - viewport_w;
                int const max_thumb_pos = track_length - thumb_length;

                int thumb_pos = 0;
                if (max_scroll > 0) {
                    // Clamp scroll offset to valid range
                    int const clamped_scroll = std::clamp(scroll_x, 0, max_scroll);
                    thumb_pos = static_cast<int>((static_cast<float>(clamped_scroll) / static_cast<float>(max_scroll)) * static_cast<float>(max_thumb_pos));
                }

                // Ensure thumb position stays within track bounds
                thumb_pos = std::clamp(thumb_pos, 0, max_thumb_pos);

                return rect_type{x + thumb_pos, y, thumb_length, h};
            } else {
                // Vertical scrollbar
                int const track_length = h;  // Full height is track

                if (content_h <= viewport_h || track_length == 0 || viewport_h == 0) {
                    // No scrolling needed, invalid track, or invalid viewport
                    return rect_type{x, y, 0, 0};  // Zero-sized thumb
                }

                // Calculate thumb size (proportional to viewport/content ratio)
                int thumb_length = std::max(min_thumb_size,
                    static_cast<int>((static_cast<float>(viewport_h) / static_cast<float>(content_h)) * static_cast<float>(track_length)));

                // Clamp thumb to track
                thumb_length = std::min(thumb_length, track_length);

                // Calculate thumb position (proportional to scroll offset)
                int const max_scroll = content_h - viewport_h;
                int const max_thumb_pos = track_length - thumb_length;

                int thumb_pos = 0;
                if (max_scroll > 0) {
                    // Clamp scroll offset to valid range
                    int const clamped_scroll = std::clamp(scroll_y, 0, max_scroll);
                    thumb_pos = static_cast<int>((static_cast<float>(clamped_scroll) / static_cast<float>(max_scroll)) * static_cast<float>(max_thumb_pos));
                }

                // Ensure thumb position stays within track bounds
                thumb_pos = std::clamp(thumb_pos, 0, max_thumb_pos);

                return rect_type{x, y + thumb_pos, w, thumb_length};
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

            // Calculate component layout (returns RELATIVE coordinates)
            auto const layout = calculate_layout(style);

            // Update thumb bounds for legacy API compatibility
            m_thumb_bounds = calculate_thumb_bounds();

            // Arrange children at calculated positions
            m_thumb->arrange(geometry::relative_rect<Backend>{layout.thumb});

            // Only arrange arrows if they have non-zero size
            if (layout.has_arrows()) {
                m_arrow_dec->arrange(geometry::relative_rect<Backend>{layout.arrow_decrement});
                m_arrow_inc->arrange(geometry::relative_rect<Backend>{layout.arrow_increment});
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
    };

} // namespace onyxui
