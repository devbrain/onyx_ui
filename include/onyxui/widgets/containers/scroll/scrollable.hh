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
                m_content_size,
                m_viewport_size,
                m_scroll_offset
            };
        }

        /**
         * @brief Get current scroll offset
         * @return Current scroll position (top-left offset)
         */
        [[nodiscard]] point_type get_scroll_offset() const noexcept {
            return m_scroll_offset;
        }

        /**
         * @brief Set scroll offset with bounds checking
         * @param offset New scroll position
         * @note Automatically clamps to valid range [0, max_scroll]
         */
        void set_scroll_offset(point_type offset) {
            scroll_to(point_utils::get_x(offset), point_utils::get_y(offset));
        }

        /**
         * @brief Scroll to absolute position with bounds checking
         * @param x Horizontal scroll position (clamped to [0, max_scroll_x])
         * @param y Vertical scroll position (clamped to [0, max_scroll_y])
         * @note Emits scroll_changed signal if position changes
         */
        void scroll_to(int x, int y) {
            auto info = get_scroll_info();

            // Clamp to valid range
            int const new_x = std::clamp(x, 0, info.max_scroll_x());
            int const new_y = std::clamp(y, 0, info.max_scroll_y());

            int const old_x = point_utils::get_x(m_scroll_offset);
            int const old_y = point_utils::get_y(m_scroll_offset);

            if (new_x != old_x || new_y != old_y) {
                m_scroll_offset = point_type{new_x, new_y};
                this->invalidate_arrange();  // Need to re-arrange children at new offset
                scroll_changed.emit(m_scroll_offset);
            }
        }

        /**
         * @brief Scroll by relative delta with bounds checking
         * @param dx Horizontal scroll delta (positive = scroll right)
         * @param dy Vertical scroll delta (positive = scroll down)
         * @note Emits scroll_changed signal if position changes
         */
        void scroll_by(int dx, int dy) {
            int const current_x = point_utils::get_x(m_scroll_offset);
            int const current_y = point_utils::get_y(m_scroll_offset);
            scroll_to(current_x + dx, current_y + dy);
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

            int const viewport_x = viewport_bounds.x.to_int();
            int const viewport_y = viewport_bounds.y.to_int();
            int const viewport_w = viewport_bounds.width.to_int();
            int const viewport_h = viewport_bounds.height.to_int();

            int const child_x = child_bounds.x.to_int();
            int const child_y = child_bounds.y.to_int();
            int const child_w = child_bounds.width.to_int();
            int const child_h = child_bounds.height.to_int();

            int const scroll_x = point_utils::get_x(m_scroll_offset);
            int const scroll_y = point_utils::get_y(m_scroll_offset);

            // Calculate child position relative to content (not viewport)
            int const child_content_x = child_x + scroll_x - viewport_x;
            int const child_content_y = child_y + scroll_y - viewport_y;

            int new_scroll_x = scroll_x;
            int new_scroll_y = scroll_y;

            // Horizontal scrolling
            if (child_content_x < scroll_x) {
                // Child is left of viewport, scroll left
                new_scroll_x = child_content_x;
            } else if (child_content_x + child_w > scroll_x + viewport_w) {
                // Child is right of viewport, scroll right
                new_scroll_x = child_content_x + child_w - viewport_w;
            }

            // Vertical scrolling
            if (child_content_y < scroll_y) {
                // Child is above viewport, scroll up
                new_scroll_y = child_content_y;
            } else if (child_content_y + child_h > scroll_y + viewport_h) {
                // Child is below viewport, scroll down
                new_scroll_y = child_content_y + child_h - viewport_h;
            }

            if (new_scroll_x != scroll_x || new_scroll_y != scroll_y) {
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
                size_utils::set_size(m_content_size, 0, 0);
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

            // Detect content size change
            int const old_w = size_utils::get_width(m_content_size);
            int const old_h = size_utils::get_height(m_content_size);
            int const new_w = max_size.width.to_int();
            int const new_h = max_size.height.to_int();

            if (old_w != new_w || old_h != new_h) {
                // Cache old scrollbar visibility
                bool const old_h_visible = should_show_horizontal_scrollbar();
                bool const old_v_visible = should_show_vertical_scrollbar();

                size_utils::set_size(m_content_size, new_w, new_h);

                // Content size changed - may need to adjust scroll offset
                auto info = get_scroll_info();
                int const scroll_x = point_utils::get_x(m_scroll_offset);
                int const scroll_y = point_utils::get_y(m_scroll_offset);

                // Clamp scroll offset if content shrank
                if (scroll_x > info.max_scroll_x() || scroll_y > info.max_scroll_y()) {
                    const_cast<scrollable*>(this)->scroll_to(scroll_x, scroll_y);
                }

                const_cast<scrollable*>(this)->content_size_changed.emit(m_content_size);

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
            size_type const old_viewport_size = m_viewport_size;

            // Store viewport size from content area
            auto content_area = this->get_content_area();
            size_utils::set_size(m_viewport_size,
                content_area.width.to_int(),
                content_area.height.to_int());

            // Check if scrollbar visibility changed (for auto_hide policy)
            bool const new_h_visible = should_show_horizontal_scrollbar();
            bool const new_v_visible = should_show_vertical_scrollbar();

            // Check if viewport size changed
            bool const viewport_changed = (size_utils::get_width(old_viewport_size) != size_utils::get_width(m_viewport_size)) ||
                                          (size_utils::get_height(old_viewport_size) != size_utils::get_height(m_viewport_size));

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
            int const scroll_x = point_utils::get_x(m_scroll_offset);
            int const scroll_y = point_utils::get_y(m_scroll_offset);

            // Child position = content area origin (0,0) - scroll offset
            // In relative coords, base is always (0,0) - padding is handled elsewhere
            logical_unit const child_x = logical_unit(static_cast<double>(0 - scroll_x));
            logical_unit const child_y = logical_unit(static_cast<double>(0 - scroll_y));

            // Arrange children ONCE with scroll offset baked in
            // This is the ONLY arrangement - no double-arrangement
            for (auto& child : this->children()) {
                int const content_w = size_utils::get_width(m_content_size);
                int const content_h = size_utils::get_height(m_content_size);

                logical_rect child_bounds{child_x, child_y,
                    logical_unit(static_cast<double>(content_w)),
                    logical_unit(static_cast<double>(content_h))};
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
        mutable size_type m_content_size{0, 0};   ///< Total content size (unconstrained)
        mutable size_type m_viewport_size{0, 0};  ///< Visible viewport size
        point_type m_scroll_offset{0, 0};         ///< Current scroll position
        scrollbar_visibility_policy m_visibility_policy{};  ///< Scrollbar visibility policy (default: auto_hide)
    };

} // namespace onyxui
