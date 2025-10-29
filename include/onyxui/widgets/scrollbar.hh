/**
 * @file scrollbar.hh
 * @brief Visual scrollbar widget with interactive thumb and arrow buttons
 * @author OnyxUI Framework
 * @date 2025-10-29
 */

#pragma once

#include <onyxui/widgets/widget.hh>
#include <onyxui/widgets/scroll_info.hh>
#include <onyxui/signal.hh>
#include <algorithm>

namespace onyxui {

    /**
     * @enum orientation
     * @brief Scrollbar orientation
     */
    enum class orientation : std::uint8_t {
        horizontal,   ///< Horizontal scrollbar (bottom of viewport)
        vertical      ///< Vertical scrollbar (right side of viewport)
    };

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

            int const thickness = 16;  // TODO: Get from theme

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
         * @brief Render scrollbar (track, thumb, arrows)
         * @param ctx Render context
         */
        void do_render(render_context<Backend>& ctx) const override {
            auto const bounds = this->bounds();

            // TODO Phase 3: Render track
            // TODO Phase 3: Render thumb
            // TODO Phase 3: Render arrow buttons (if style != minimal)

            // For now, just render a placeholder rect
            ctx.draw_rect(bounds);
        }

    private:
        /**
         * @brief Calculate thumb bounds based on scroll_info
         * @return Rectangle representing thumb position/size
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

            int const min_thumb_size = 20;  // TODO: Get from theme

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

        orientation m_orientation = orientation::vertical;  ///< Horizontal or vertical
        scroll_info<Backend> m_scroll_info{};               ///< Current scroll state
        rect_type m_thumb_bounds{};                         ///< Calculated thumb bounds
    };

} // namespace onyxui
