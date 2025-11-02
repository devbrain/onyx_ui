/**
 * @file scrollbar.hh
 * @brief Visual scrollbar widget with interactive thumb and arrow buttons
 * @author OnyxUI Framework
 * @date 2025-10-29
 */

#pragma once

#include <onyxui/widgets/core/widget.hh>
#include <onyxui/widgets/containers/scroll/scroll_info.hh>
#include <onyxui/core/signal.hh>
#include <onyxui/core/orientation.hh>
#include <onyxui/ui_constants.hh>  // For default scrollbar dimensions
#include <onyxui/theming/theme.hh>
#include <onyxui/services/ui_services.hh>
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
    class scrollbar : public widget<Backend> {
    public:
        using base = widget<Backend>;
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
            : m_orientation(orient)
        {
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
            (void)available_width;
            (void)available_height;

            // Scrollbar has fixed thickness from theme
            // Length is content-based (parent determines)

            // Get theme to determine scrollbar width
            auto const* themes = ui_services<Backend>::themes();
            int const thickness = (themes && themes->get_current_theme())
                ? themes->get_current_theme()->scrollbar.width
                : ui_constants::DEFAULT_SCROLLBAR_THICKNESS;

            if (m_orientation == orientation::horizontal) {
                // Horizontal: flexible width, fixed height
                return size_type{100, thickness};  // Width is placeholder
            } else {
                // Vertical: fixed width, flexible height
                return size_type{thickness, 100};  // Height is placeholder
            }
        }

        /**
         * @brief Arrange scrollbar and calculate thumb bounds
         * @param final_bounds Final bounds assigned by parent
         */
        void do_arrange(const rect_type& final_bounds) override {
            base::do_arrange(final_bounds);

            // Calculate thumb position and size based on scroll_info
            m_thumb_bounds = calculate_thumb_bounds();
        }

        /**
         * @brief Handle mouse click for arrow button interaction
         * @param x Mouse x coordinate
         * @param y Mouse y coordinate
         * @return true if click was handled
         */
        bool handle_click(int x, int y) override {
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

            // Check if click is in decrement arrow
            if (layout.has_arrows() && point_in_rect({x, y}, layout.arrow_decrement)) {
                // Scroll up/left by one line
                int const line_increment = theme->scrollbar.line_increment;
                if (m_orientation == orientation::vertical) {
                    scroll_requested.emit(-line_increment);  // Negative = scroll up
                } else {
                    scroll_requested.emit(-line_increment);  // Negative = scroll left
                }
                return true;
            }

            // Check if click is in increment arrow
            if (layout.has_arrows() && point_in_rect({x, y}, layout.arrow_increment)) {
                // Scroll down/right by one line
                int const line_increment = theme->scrollbar.line_increment;
                if (m_orientation == orientation::vertical) {
                    scroll_requested.emit(line_increment);  // Positive = scroll down
                } else {
                    scroll_requested.emit(line_increment);  // Positive = scroll right
                }
                return true;
            }

            // Click was not on an arrow button
            return false;
        }

        /**
         * @brief Handle mouse movement for hover states
         * @param x Mouse x coordinate
         * @param y Mouse y coordinate
         * @return true if handled
         */
        bool handle_mouse_move(int x, int y) override {
            auto const* themes = ui_services<Backend>::themes();
            if (!themes) {
                return false;
            }

            auto const* theme = themes->get_current_theme();
            if (!theme) {
                return false;
            }

            scrollbar_style const style = theme->scrollbar.style;
            auto const layout = calculate_layout(style);

            // Update hover states
            bool const old_thumb_hovered = m_thumb_hovered;
            bool const old_arrow_hovered = m_arrow_hovered;

            m_thumb_hovered = point_in_rect({x, y}, layout.thumb);
            m_arrow_hovered = (layout.has_arrows() &&
                              (point_in_rect({x, y}, layout.arrow_decrement) ||
                               point_in_rect({x, y}, layout.arrow_increment)));

            // Mark dirty if hover state changed
            if (old_thumb_hovered != m_thumb_hovered || old_arrow_hovered != m_arrow_hovered) {
                this->mark_dirty();
            }

            return base::handle_mouse_move(x, y);
        }

        /**
         * @brief Handle mouse button press
         * @param x Mouse x coordinate
         * @param y Mouse y coordinate
         * @param button Mouse button number
         * @return true if handled
         */
        bool handle_mouse_down(int x, int y, int button) override {
            auto const* themes = ui_services<Backend>::themes();
            if (!themes) {
                return false;
            }

            auto const* theme = themes->get_current_theme();
            if (!theme) {
                return false;
            }

            scrollbar_style const style = theme->scrollbar.style;
            auto const layout = calculate_layout(style);

            // Update pressed states
            bool const old_thumb_pressed = m_thumb_pressed;
            bool const old_arrow_pressed = m_arrow_pressed;

            m_thumb_pressed = point_in_rect({x, y}, layout.thumb);
            m_arrow_pressed = (layout.has_arrows() &&
                              (point_in_rect({x, y}, layout.arrow_decrement) ||
                               point_in_rect({x, y}, layout.arrow_increment)));

            // Mark dirty if pressed state changed
            if (old_thumb_pressed != m_thumb_pressed || old_arrow_pressed != m_arrow_pressed) {
                this->mark_dirty();
            }

            return base::handle_mouse_down(x, y, button);
        }

        /**
         * @brief Handle mouse button release
         * @param x Mouse x coordinate
         * @param y Mouse y coordinate
         * @param button Mouse button number
         * @return true if handled
         */
        bool handle_mouse_up(int x, int y, int button) override {
            bool const old_thumb_pressed = m_thumb_pressed;
            bool const old_arrow_pressed = m_arrow_pressed;

            // Clear pressed states
            m_thumb_pressed = false;
            m_arrow_pressed = false;

            // Mark dirty if pressed state changed
            if (old_thumb_pressed || old_arrow_pressed) {
                this->mark_dirty();
            }

            return base::handle_mouse_up(x, y, button);
        }

        /**
         * @brief Handle mouse leaving widget
         * @return true if handled
         */
        bool handle_mouse_leave() override {
            bool const old_thumb_hovered = m_thumb_hovered;
            bool const old_arrow_hovered = m_arrow_hovered;

            // Clear hover states
            m_thumb_hovered = false;
            m_arrow_hovered = false;

            // Mark dirty if hover state changed
            if (old_thumb_hovered || old_arrow_hovered) {
                this->mark_dirty();
            }

            return base::handle_mouse_leave();
        }

        /**
         * @brief Render scrollbar (track, thumb, arrows)
         * @param ctx Render context
         */
        void do_render(render_context<Backend>& ctx) const override {
            // Get theme to determine scrollbar style
            auto const* theme = ctx.theme();
            if (!theme) {
                return;  // No theme available
            }

            scrollbar_style const style = theme->scrollbar.style;

            // Calculate layout based on current style
            auto const layout = calculate_layout(style);

            // Determine thumb state based on interaction
            auto const& thumb_style = get_thumb_style(*theme);

            // Determine arrow state based on interaction
            auto const& arrow_style = get_arrow_style(*theme);

            // Render track (background) - always normal state
            ctx.draw_rect(layout.track, theme->scrollbar.track_normal.box_style);

            // Render thumb (only if visible) with state-based styling
            if (rect_utils::get_width(layout.thumb) > 0 && rect_utils::get_height(layout.thumb) > 0) {
                ctx.draw_rect(layout.thumb, thumb_style.box_style);
            }

            // Render arrow buttons (if style has them) with state-based styling
            if (layout.has_arrows()) {
                // Render decrement arrow (up/left) if present
                if (rect_utils::get_width(layout.arrow_decrement) > 0 &&
                    rect_utils::get_height(layout.arrow_decrement) > 0) {
                    ctx.draw_rect(layout.arrow_decrement, arrow_style.box_style);

                    // Render arrow glyph (centered in button)
                    auto icon_size = renderer_type::get_icon_size(theme->scrollbar.arrow_decrement_icon);
                    int const icon_x = rect_utils::get_x(layout.arrow_decrement) +
                                       (rect_utils::get_width(layout.arrow_decrement) - size_utils::get_width(icon_size)) / 2;
                    int const icon_y = rect_utils::get_y(layout.arrow_decrement) +
                                       (rect_utils::get_height(layout.arrow_decrement) - size_utils::get_height(icon_size)) / 2;
                    point_type const icon_pos{icon_x, icon_y};
                    ctx.draw_icon(theme->scrollbar.arrow_decrement_icon, icon_pos);
                }

                // Render increment arrow (down/right) if present
                if (rect_utils::get_width(layout.arrow_increment) > 0 &&
                    rect_utils::get_height(layout.arrow_increment) > 0) {
                    ctx.draw_rect(layout.arrow_increment, arrow_style.box_style);

                    // Render arrow glyph (centered in button)
                    auto icon_size = renderer_type::get_icon_size(theme->scrollbar.arrow_increment_icon);
                    int const icon_x = rect_utils::get_x(layout.arrow_increment) +
                                       (rect_utils::get_width(layout.arrow_increment) - size_utils::get_width(icon_size)) / 2;
                    int const icon_y = rect_utils::get_y(layout.arrow_increment) +
                                       (rect_utils::get_height(layout.arrow_increment) - size_utils::get_height(icon_size)) / 2;
                    point_type const icon_pos{icon_x, icon_y};
                    ctx.draw_icon(theme->scrollbar.arrow_increment_icon, icon_pos);
                }
            }
        }

    protected:
        /**
         * @brief Get appropriate thumb style based on current state
         * @param theme Theme containing style definitions
         * @return Component style for current thumb state
         */
        [[nodiscard]] auto const& get_thumb_style(const theme_type& theme) const noexcept {
            if (!this->is_enabled()) {
                return theme.scrollbar.thumb_disabled;
            }
            if (m_thumb_pressed) {
                return theme.scrollbar.thumb_pressed;
            }
            if (m_thumb_hovered) {
                return theme.scrollbar.thumb_hover;
            }
            return theme.scrollbar.thumb_normal;
        }

        /**
         * @brief Get appropriate arrow style based on current state
         * @param theme Theme containing style definitions
         * @return Component style for current arrow state
         */
        [[nodiscard]] auto const& get_arrow_style(const theme_type& theme) const noexcept {
            // For now, arrows don't track individual hover/pressed state
            // Could be enhanced to track decrement vs increment arrow states
            if (m_arrow_pressed) {
                return theme.scrollbar.arrow_pressed;
            }
            if (m_arrow_hovered) {
                return theme.scrollbar.arrow_hover;
            }
            return theme.scrollbar.arrow_normal;
        }

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
                // Horizontal scrollbar layout
                int const thickness = h;  // Arrow button size = scrollbar thickness

                if (style == scrollbar_style::minimal) {
                    // No arrows - track fills entire bounds
                    rect_utils::set_bounds(layout.track, x, y, w, h);
                } else if (style == scrollbar_style::compact) {
                    // Arrows at right end only
                    rect_utils::set_bounds(layout.arrow_decrement, x + w - thickness, y, thickness, h);
                    rect_utils::set_bounds(layout.track, x, y, w - thickness, h);
                } else { // classic
                    // Arrows at both ends
                    rect_utils::set_bounds(layout.arrow_decrement, x, y, thickness, h);
                    rect_utils::set_bounds(layout.arrow_increment, x + w - thickness, y, thickness, h);
                    rect_utils::set_bounds(layout.track, x + thickness, y, w - 2 * thickness, h);
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
                int const thickness = w;  // Arrow button size = scrollbar thickness

                if (style == scrollbar_style::minimal) {
                    // No arrows - track fills entire bounds
                    rect_utils::set_bounds(layout.track, x, y, w, h);
                } else if (style == scrollbar_style::compact) {
                    // Arrows at bottom end only
                    rect_utils::set_bounds(layout.arrow_increment, x, y + h - thickness, w, thickness);
                    rect_utils::set_bounds(layout.track, x, y, w, h - thickness);
                } else { // classic
                    // Arrows at both ends
                    rect_utils::set_bounds(layout.arrow_decrement, x, y, w, thickness);
                    rect_utils::set_bounds(layout.arrow_increment, x, y + h - thickness, w, thickness);
                    rect_utils::set_bounds(layout.track, x, y + thickness, w, h - 2 * thickness);
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

    private:
        orientation m_orientation = orientation::vertical;  ///< Horizontal or vertical
        scroll_info<Backend> m_scroll_info{};               ///< Current scroll state
        rect_type m_thumb_bounds{};                         ///< Calculated thumb bounds

        // State tracking for themed rendering
        bool m_thumb_hovered = false;   ///< True if mouse is hovering over thumb
        bool m_thumb_pressed = false;   ///< True if thumb is being pressed
        bool m_arrow_hovered = false;   ///< True if mouse is hovering over arrow
        bool m_arrow_pressed = false;   ///< True if arrow is being pressed
    };

} // namespace onyxui
