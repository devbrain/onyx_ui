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
#include <algorithm>

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

            int const viewport_x = rect_utils::get_x(viewport_bounds);
            int const viewport_y = rect_utils::get_y(viewport_bounds);
            int const viewport_w = rect_utils::get_width(viewport_bounds);
            int const viewport_h = rect_utils::get_height(viewport_bounds);

            int const child_x = rect_utils::get_x(child_bounds);
            int const child_y = rect_utils::get_y(child_bounds);
            int const child_w = rect_utils::get_width(child_bounds);
            int const child_h = rect_utils::get_height(child_bounds);

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
        size_type do_measure(int available_width, int available_height) override {
            // Ignore available size - measure children with unconstrained size
            (void)available_width;
            (void)available_height;

            constexpr int UNCONSTRAINED = std::numeric_limits<int>::max() - 1;

            if (this->children().empty()) {
                m_content_size = size_type{0, 0};
                // Return small size for empty container
                return m_content_size;
            }

            // Measure all children with unconstrained size
            // This allows them to report their natural/preferred size
            size_type max_size{0, 0};

            for (const auto& child : this->children()) {
                auto const child_size = child->measure(UNCONSTRAINED, UNCONSTRAINED);

                int const child_w = size_utils::get_width(child_size);
                int const child_h = size_utils::get_height(child_size);
                int const max_w = size_utils::get_width(max_size);
                int const max_h = size_utils::get_height(max_size);

                // Take maximum size across all children
                size_utils::set_size(max_size,
                    std::max(max_w, child_w),
                    std::max(max_h, child_h));
            }

            // Detect content size change
            int const old_w = size_utils::get_width(m_content_size);
            int const old_h = size_utils::get_height(m_content_size);
            int const new_w = size_utils::get_width(max_size);
            int const new_h = size_utils::get_height(max_size);

            if (old_w != new_w || old_h != new_h) {
                // Cache old scrollbar visibility
                bool const old_h_visible = should_show_horizontal_scrollbar();
                bool const old_v_visible = should_show_vertical_scrollbar();

                m_content_size = max_size;

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
            return m_content_size;
        }

        /**
         * @brief Arrange widget and children with scroll offset
         * @param final_bounds Final bounds assigned by parent
         * @note Overrides do_arrange to position children with negative scroll offset
         */
        void do_arrange(const rect_type& final_bounds) override {
            // Call base implementation
            base::do_arrange(final_bounds);

            // Cache old scrollbar visibility
            bool const old_h_visible = should_show_horizontal_scrollbar();
            bool const old_v_visible = should_show_vertical_scrollbar();

            // Store viewport size from content area
            auto content_area = this->get_content_area();
            m_viewport_size = size_type{
                rect_utils::get_width(content_area),
                rect_utils::get_height(content_area)
            };

            // Check if scrollbar visibility changed (for auto_hide policy)
            bool const new_h_visible = should_show_horizontal_scrollbar();
            bool const new_v_visible = should_show_vertical_scrollbar();

            if (old_h_visible != new_h_visible || old_v_visible != new_v_visible) {
                scrollbar_visibility_changed.emit(new_h_visible, new_v_visible);
            }

            if (this->children().empty()) {
                return;
            }

            // Check if we're at relative coordinates (nested inside another container)
            // If our bounds start at (0,0) or near it, we're likely nested
            bool const is_nested = (rect_utils::get_x(this->bounds()) < 2 &&
                                    rect_utils::get_y(this->bounds()) < 2);

            int child_x, child_y;
            if (is_nested) {
                // We're nested - use relative coordinates
                int const scroll_x = point_utils::get_x(m_scroll_offset);
                int const scroll_y = point_utils::get_y(m_scroll_offset);
                child_x = 0 - scroll_x;
                child_y = 0 - scroll_y;
            } else {
                // We're at top level - use absolute coordinates (framework limitation)
                int const base_x = rect_utils::get_x(content_area);
                int const base_y = rect_utils::get_y(content_area);
                int const scroll_x = point_utils::get_x(m_scroll_offset);
                int const scroll_y = point_utils::get_y(m_scroll_offset);
                child_x = base_x - scroll_x;
                child_y = base_y - scroll_y;
            }

            // Arrange children at scrolled position
            for (auto& child : this->children()) {
                int const content_w = size_utils::get_width(m_content_size);
                int const content_h = size_utils::get_height(m_content_size);

                rect_type child_bounds{child_x, child_y, content_w, content_h};
                child->arrange(child_bounds);
            }
        }

        /**
         * @brief Render content with viewport clipping
         * @param ctx Render context with style information
         *
         * @details
         * Clips rendering to the content area (viewport bounds).
         * Children that are positioned outside the viewport (due to scroll offset)
         * are automatically clipped by the renderer.
         *
         * The clipping ensures that:
         * - Content outside the viewport is not drawn
         * - Scrolled content is properly clipped at viewport edges
         * - Nested scrollables clip correctly (clip regions stack)
         *
         * @note Children render at scrolled positions (negative offset)
         */
        void do_render(render_context<Backend>& ctx) const override {
            // Only clip during actual rendering (not measurement)
            if (!ctx.is_rendering()) {
                base::do_render(ctx);
                return;
            }

            // Get viewport bounds (content area excluding padding/borders)
            auto const viewport_bounds = this->get_content_area();

            // RAII guard for viewport clipping - automatically pops on scope exit
            scoped_clip clip(ctx, viewport_bounds);

            // Render children with clipping active
            // Children positioned outside viewport are clipped automatically
            base::do_render(ctx);

            // Clip automatically popped when 'clip' guard is destroyed
        }

    private:
        mutable size_type m_content_size{0, 0};   ///< Total content size (unconstrained)
        mutable size_type m_viewport_size{0, 0};  ///< Visible viewport size
        point_type m_scroll_offset{0, 0};         ///< Current scroll position
        scrollbar_visibility_policy m_visibility_policy{};  ///< Scrollbar visibility policy (default: auto_hide)
    };

} // namespace onyxui
