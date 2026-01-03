/**
 * @file scrollable.hh
 * @brief Logic container widget with viewport clipping and scroll offset management
 * @author OnyxUI Framework
 * @date 2025-10-29
 */

#pragma once

#include <onyxui/widgets/containers/panel.hh>
#include <onyxui/widgets/containers/scroll/scroll_info.hh>
#include <onyxui/widgets/containers/scroll/scrollbar_visibility.hh>
#include <onyxui/core/signal.hh>
#include <onyxui/core/raii/scoped_clip.hh>
#include <onyxui/core/rendering/draw_context.hh>
#include <onyxui/theming/theme.hh>
#include <algorithm>
#include <cmath>
#include <iostream>

namespace onyxui {

    /**
     * @class scrollable
     * @brief Container widget with viewport clipping and scroll management
     *
     * @details
     * The scrollable widget is the logic layer of the scrolling system. It:
     * - Measures children with unconstrained size (infinite space)
     * - Arranges children at negative scroll offset
     * - Clips rendering to viewport bounds
     * - Manages scroll position with bounds checking
     * - Emits signals on scroll/content changes
     * - Handles mouse wheel events
     *
     * Architecture:
     * - Logic layer (this class): Viewport clipping + scroll offset
     * - Visual layer (scrollbar): Visual representation of scroll state
     * - Coordination layer (scroll_controller): Binds logic + visual
     *
     * Measurement:
     * - Children measure with infinite available space
     * - Container's desired size = child's content size (unconstrained)
     * - Viewport size determined by parent during arrange
     *
     * Arrangement:
     * - Children positioned at: base_position - scroll_offset
     * - Negative offset scrolls content upward/leftward
     * - Scroll offset clamped to [0, max_scroll]
     *
     * Rendering:
     * - Calls renderer.push_clip(viewport_bounds) before rendering children
     * - Content outside viewport is clipped by renderer
     * - Calls renderer.pop_clip() after rendering children
     *
     * @tparam Backend UIBackend implementation
     */
    template<UIBackend Backend>
    class scrollable : public panel<Backend> {
    public:
        using base = panel<Backend>;
        using element_type = ui_element<Backend>;
        using size_type = typename Backend::size_type;
        using point_type = typename Backend::point_type;
        using rect_type = typename Backend::rect_type;
        using renderer_type = typename Backend::renderer_type;
        using event_type = typename Backend::event_type;
        using theme_type = ui_theme<Backend>;
        using render_context_type = render_context<Backend>;

        /**
         * @brief Construct empty scrollable container
         */
        scrollable() = default;

        /**
         * @brief Construct with single child
         * @param child Child widget to scroll (takes ownership)
         */
        explicit scrollable(std::unique_ptr<element_type> child) {
            if (child) {
                this->add_child(std::move(child));
            }
        }

        /**
         * @brief Get current scroll information
         * @return scroll_info structure with content, viewport, and offset
         */
        [[nodiscard]] scroll_info<Backend> get_scroll_info() const noexcept {
            return scroll_info<Backend>{
                m_content_width,
                m_content_height,
                m_viewport_width,
                m_viewport_height,
                m_scroll_x,
                m_scroll_y
            };
        }

        /**
         * @brief Get current scroll offset as Backend point_type (for legacy APIs)
         * @return Current scroll position as int-based point_type
         */
        [[nodiscard]] point_type get_scroll_offset() const noexcept {
            return point_type{static_cast<int>(std::floor(m_scroll_x)),
                              static_cast<int>(std::floor(m_scroll_y))};
        }

        /**
         * @brief Get current scroll X offset (logical units)
         * @return Current horizontal scroll position
         */
        [[nodiscard]] double get_scroll_x() const noexcept {
            return m_scroll_x;
        }

        /**
         * @brief Get current scroll Y offset (logical units)
         * @return Current vertical scroll position
         */
        [[nodiscard]] double get_scroll_y() const noexcept {
            return m_scroll_y;
        }

        /**
         * @brief Set scroll offset with bounds checking
         * @param offset New scroll position
         * @note Automatically clamps to valid range [0, max_scroll]
         */
        void set_scroll_offset(point_type offset) {
            scroll_to(static_cast<double>(point_utils::get_x(offset)),
                      static_cast<double>(point_utils::get_y(offset)));
        }

        /**
         * @brief Scroll to absolute position with bounds checking (double precision)
         * @param x Horizontal scroll position (clamped to [0, max_scroll_x])
         * @param y Vertical scroll position (clamped to [0, max_scroll_y])
         * @note Emits scroll_changed signal if position changes
         */
        void scroll_to(double x, double y) {
            auto info = get_scroll_info();

            // Clamp to valid range
            double const new_x = std::clamp(x, 0.0, info.max_scroll_x());
            double const new_y = std::clamp(y, 0.0, info.max_scroll_y());

            // Use a small epsilon for floating-point comparison
            constexpr double EPSILON = 0.001;
            if (std::abs(new_x - m_scroll_x) > EPSILON || std::abs(new_y - m_scroll_y) > EPSILON) {
                m_scroll_x = new_x;
                m_scroll_y = new_y;
                this->invalidate_arrange();  // Need to re-arrange children at new offset
                scroll_changed.emit(get_scroll_offset());  // Emit legacy int-based offset
            }
        }

        /**
         * @brief Scroll to absolute position with bounds checking (int version for legacy APIs)
         * @param x Horizontal scroll position (clamped to [0, max_scroll_x])
         * @param y Vertical scroll position (clamped to [0, max_scroll_y])
         * @note Emits scroll_changed signal if position changes
         */
        void scroll_to(int x, int y) {
            scroll_to(static_cast<double>(x), static_cast<double>(y));
        }

        /**
         * @brief Scroll by relative delta with bounds checking
         * @param dx Horizontal scroll delta (positive = scroll right)
         * @param dy Vertical scroll delta (positive = scroll down)
         * @note Emits scroll_changed signal if position changes
         */
        void scroll_by(int dx, int dy) {
            scroll_to(m_scroll_x + static_cast<double>(dx), m_scroll_y + static_cast<double>(dy));
        }

        /**
         * @brief Scroll by relative delta with bounds checking (double precision)
         * @param dx Horizontal scroll delta (positive = scroll right)
         * @param dy Vertical scroll delta (positive = scroll down)
         * @note Emits scroll_changed signal if position changes
         */
        void scroll_by(double dx, double dy) {
            scroll_to(m_scroll_x + dx, m_scroll_y + dy);
        }

        /**
         * @brief Scroll to bring child element into view
         * @param child Pointer to child element
         * @note Scrolls minimum distance to make child visible
         * @note No-op if child is nullptr or not a direct child
         */
        void scroll_into_view(const element_type* child) {
            if (!child) return;

            // Verify child is a direct child of this scrollable
            auto it = std::find_if(this->children().begin(), this->children().end(),
                [child](const auto& c) { return c.get() == child; });
            if (it == this->children().end()) return;

            auto const child_bounds = child->bounds();
            auto const viewport_bounds = this->get_content_area();

            // Use logical units (double) for precision
            double const viewport_x = viewport_bounds.x.value;
            double const viewport_y = viewport_bounds.y.value;
            double const viewport_w = viewport_bounds.width.value;
            double const viewport_h = viewport_bounds.height.value;

            double const child_x = child_bounds.x.value;
            double const child_y = child_bounds.y.value;
            double const child_w = child_bounds.width.value;
            double const child_h = child_bounds.height.value;

            // Calculate child position relative to content (not viewport)
            double const child_content_x = child_x + m_scroll_x - viewport_x;
            double const child_content_y = child_y + m_scroll_y - viewport_y;

            double new_scroll_x = m_scroll_x;
            double new_scroll_y = m_scroll_y;

            // Horizontal scrolling
            if (child_content_x < m_scroll_x) {
                // Child is left of viewport, scroll left
                new_scroll_x = child_content_x;
            } else if (child_content_x + child_w > m_scroll_x + viewport_w) {
                // Child is right of viewport, scroll right
                new_scroll_x = child_content_x + child_w - viewport_w;
            }

            // Vertical scrolling
            if (child_content_y < m_scroll_y) {
                // Child is above viewport, scroll up
                new_scroll_y = child_content_y;
            } else if (child_content_y + child_h > m_scroll_y + viewport_h) {
                // Child is below viewport, scroll down
                new_scroll_y = child_content_y + child_h - viewport_h;
            }

            constexpr double EPSILON = 0.001;
            if (std::abs(new_scroll_x - m_scroll_x) > EPSILON || std::abs(new_scroll_y - m_scroll_y) > EPSILON) {
                scroll_to(new_scroll_x, new_scroll_y);
            }
        }

        // Signals

        /**
         * @brief Emitted when scroll position changes
         * @param point_type New scroll offset
         */
        signal<point_type> scroll_changed;

        /**
         * @brief Emitted when content size changes (child resized)
         * @param size_type New content size
         */
        signal<size_type> content_size_changed;

        /**
         * @brief Emitted when scrollbar visibility changes
         * @param bool horizontal_visible New horizontal scrollbar visibility
         * @param bool vertical_visible New vertical scrollbar visibility
         */
        signal<bool, bool> scrollbar_visibility_changed;

        // Visibility Policy Management

        /**
         * @brief Set visibility policy for both scrollbars
         * @param policy Visibility policy to apply to both axes
         */
        void set_scrollbar_visibility(scrollbar_visibility policy) {
            set_scrollbar_visibility_policy(scrollbar_visibility_policy{policy});
        }

        /**
         * @brief Set independent visibility policies for each axis
         * @param horizontal Horizontal scrollbar visibility policy
         * @param vertical Vertical scrollbar visibility policy
         */
        void set_scrollbar_visibility(scrollbar_visibility horizontal, scrollbar_visibility vertical) {
            set_scrollbar_visibility_policy(scrollbar_visibility_policy{horizontal, vertical});
        }

        /**
         * @brief Set complete visibility policy
         * @param policy Visibility policy with independent horizontal/vertical settings
         */
        void set_scrollbar_visibility_policy(scrollbar_visibility_policy policy) {
            if (m_visibility_policy == policy) {
                return;  // No change
            }

            // Cache old visibility state
            bool const old_h_visible = should_show_horizontal_scrollbar();
            bool const old_v_visible = should_show_vertical_scrollbar();

            // Update policy
            m_visibility_policy = policy;

            // Check if visibility changed
            bool const new_h_visible = should_show_horizontal_scrollbar();
            bool const new_v_visible = should_show_vertical_scrollbar();

            if (old_h_visible != new_h_visible || old_v_visible != new_v_visible) {
                scrollbar_visibility_changed.emit(new_h_visible, new_v_visible);
            }
        }

        /**
         * @brief Get current visibility policy
         * @return Current scrollbar visibility policy
         */
        [[nodiscard]] scrollbar_visibility_policy get_scrollbar_visibility_policy() const noexcept {
            return m_visibility_policy;
        }

        /**
         * @brief Check if horizontal scrollbar should be visible
         * @return true if horizontal scrollbar should be shown
         *
         * @details
         * Determines visibility based on:
         * - Visibility policy (always/auto/hidden)
         * - Content size vs viewport size (for auto policy)
         */
        [[nodiscard]] bool should_show_horizontal_scrollbar() const noexcept {
            switch (m_visibility_policy.horizontal) {
                case scrollbar_visibility::always:
                    return true;
                case scrollbar_visibility::hidden:
                    return false;
                case scrollbar_visibility::auto_hide: {
                    auto info = get_scroll_info();
                    return info.needs_horizontal_scroll();
                }
            }
            return false;  // Unreachable, but satisfies compiler
        }

        /**
         * @brief Check if vertical scrollbar should be visible
         * @return true if vertical scrollbar should be shown
         *
         * @details
         * Determines visibility based on:
         * - Visibility policy (always/auto/hidden)
         * - Content size vs viewport size (for auto policy)
         */
        [[nodiscard]] bool should_show_vertical_scrollbar() const noexcept {
            switch (m_visibility_policy.vertical) {
                case scrollbar_visibility::always:
                    return true;
                case scrollbar_visibility::hidden:
                    return false;
                case scrollbar_visibility::auto_hide: {
                    auto info = get_scroll_info();
                    return info.needs_vertical_scroll();
                }
            }
            return false;  // Unreachable, but satisfies compiler
        }

    protected:
        /**
         * @brief Measure children with unconstrained size (infinite space)
         * @param available_width Available width (ignored - we measure unconstrained)
         * @param available_height Available height (ignored - we measure unconstrained)
         * @return Content size (unconstrained child size)
         * @note Children measure as if they have infinite space available
         */
        logical_size do_measure(logical_unit available_width, logical_unit available_height) override {
            // Ignore available size - measure children with unconstrained size
            (void)available_width;
            (void)available_height;

            constexpr logical_unit UNCONSTRAINED = logical_unit(static_cast<double>(std::numeric_limits<int>::max() - 1));

            if (this->children().empty()) {
                m_content_width = 0.0;
                m_content_height = 0.0;
                // Return small size for empty container
                return logical_size{logical_unit(0.0), logical_unit(0.0)};
            }

            // Measure all children with unconstrained size
            // This allows them to report their natural/preferred size
            logical_size max_size{logical_unit(0.0), logical_unit(0.0)};

            for (const auto& child : this->children()) {
                auto const child_size = child->measure(UNCONSTRAINED, UNCONSTRAINED);

                // Take maximum size across all children
                max_size.width = max(max_size.width, child_size.width);
                max_size.height = max(max_size.height, child_size.height);
            }

            // Detect content size change (using small epsilon for double comparison)
            constexpr double EPSILON = 0.001;
            double const new_w = max_size.width.value;
            double const new_h = max_size.height.value;

            if (std::abs(m_content_width - new_w) > EPSILON || std::abs(m_content_height - new_h) > EPSILON) {
                // Cache old scrollbar visibility
                bool const old_h_visible = should_show_horizontal_scrollbar();
                bool const old_v_visible = should_show_vertical_scrollbar();

                m_content_width = new_w;
                m_content_height = new_h;

                // Content size changed - may need to adjust scroll offset
                auto info = get_scroll_info();

                // Clamp scroll offset if content shrank
                if (m_scroll_x > info.max_scroll_x() || m_scroll_y > info.max_scroll_y()) {
                    const_cast<scrollable*>(this)->scroll_to(m_scroll_x, m_scroll_y);
                }

                // Emit content_size_changed with legacy size_type (for backward compatibility)
                size_type content_size_legacy;
                size_utils::set_size(content_size_legacy,
                    static_cast<int>(std::floor(m_content_width)),
                    static_cast<int>(std::floor(m_content_height)));
                const_cast<scrollable*>(this)->content_size_changed.emit(content_size_legacy);

                // Check if scrollbar visibility changed (for auto_hide policy)
                bool const new_h_visible = should_show_horizontal_scrollbar();
                bool const new_v_visible = should_show_vertical_scrollbar();

                if (old_h_visible != new_h_visible || old_v_visible != new_v_visible) {
                    const_cast<scrollable*>(this)->scrollbar_visibility_changed.emit(new_h_visible, new_v_visible);
                }
            }

            // Return the unconstrained content size
            // This is what the scrollable wants to be (its natural size)
            return max_size;
        }

        /**
         * @brief Arrange widget and children with scroll offset
         * @param final_bounds Final bounds assigned by parent
         * @note Overrides do_arrange to position children with negative scroll offset
         */
        void do_arrange(const logical_rect& final_bounds) override {
            // SINGLE-PASS ARRANGEMENT (Solution 1 from SCROLLABLE_ARCHITECTURE_ISSUE.md)
            // Arrange children ONCE with scroll offset baked in
            // DON'T call base::do_arrange() - prevents double-arrangement
            (void)final_bounds;  // Bounds already set by arrange() before calling do_arrange()

            // Cache old scrollbar visibility AND viewport size
            bool const old_h_visible = should_show_horizontal_scrollbar();
            bool const old_v_visible = should_show_vertical_scrollbar();
            double const old_viewport_width = m_viewport_width;
            double const old_viewport_height = m_viewport_height;

            // Store viewport size from content area (preserve logical precision)
            auto content_area = this->get_content_area();
            m_viewport_width = content_area.width.value;
            m_viewport_height = content_area.height.value;

            // Check if scrollbar visibility changed (for auto_hide policy)
            bool const new_h_visible = should_show_horizontal_scrollbar();
            bool const new_v_visible = should_show_vertical_scrollbar();

            // Check if viewport size changed (using epsilon for double comparison)
            constexpr double EPSILON = 0.001;
            bool const viewport_changed = (std::abs(old_viewport_width - m_viewport_width) > EPSILON) ||
                                          (std::abs(old_viewport_height - m_viewport_height) > EPSILON);

            // CRITICAL FIX: Emit signal if visibility OR viewport size changed
            // This ensures scroll_controller updates scrollbar info when viewport changes,
            // even if visibility policy is "always" (like in text_view)
            if (old_h_visible != new_h_visible || old_v_visible != new_v_visible || viewport_changed) {
                scrollbar_visibility_changed.emit(new_h_visible, new_v_visible);
            }

            if (this->children().empty()) {
                return;
            }

            // Calculate child position with scroll offset applied
            // Children are arranged in VIEWPORT space, not virtual space
            // RELATIVE COORDINATES: Children are positioned relative to content area (0,0)
            // content_area's x,y offset is already accounted for during rendering
            // Child position = content area origin (0,0) - scroll offset
            // In relative coords, base is always (0,0) - padding is handled elsewhere
            // ROUND to integers for terminal backends where mouse coords are integers
            logical_unit const child_x = logical_unit(std::round(-m_scroll_x));
            logical_unit const child_y = logical_unit(std::round(-m_scroll_y));

            // Arrange children ONCE with scroll offset baked in
            // This is the ONLY arrangement - no double-arrangement
            for (auto& child : this->children()) {
                logical_rect child_bounds{child_x, child_y,
                    logical_unit(m_content_width),
                    logical_unit(m_content_height)};
                child->arrange(child_bounds);
            }
        }

        /**
         * @brief Render this widget itself (not children)
         * @details Scrollable has no visual representation (no border/background).
         *          Clipping is handled by base class via get_content_area().
         */
        void do_render(render_context_type& ctx) const override {
            // Nothing to render for scrollable itself
            // Base class will handle clipping via get_content_area()
            (void)ctx;
        }

    private:
        mutable double m_content_width = 0.0;     ///< Total content width (logical units)
        mutable double m_content_height = 0.0;    ///< Total content height (logical units)
        mutable double m_viewport_width = 0.0;    ///< Visible viewport width (logical units)
        mutable double m_viewport_height = 0.0;   ///< Visible viewport height (logical units)
        double m_scroll_x = 0.0;                  ///< Current horizontal scroll position (logical units)
        double m_scroll_y = 0.0;                  ///< Current vertical scroll position (logical units)
        scrollbar_visibility_policy m_visibility_policy{};  ///< Scrollbar visibility policy (default: auto_hide)
    };

} // namespace onyxui
