/**
 * @file anchor_layout.hh
 * @brief Anchor-based layout strategy for positioning at predefined points
 * @author igor
 * @date 09/10/2025
 *
 * @details
 * Anchor layout positions children relative to predefined anchor points within
 * the parent (e.g., top-left, center, bottom-right). This is useful for UI
 * elements that should stay at specific positions regardless of parent size,
 * similar to CSS anchor positioning or Unity's anchor system.
 *
 * ## Features
 * - Nine predefined anchor points (corners, edges, center)
 * - Pixel-perfect offset adjustment from anchor points
 * - Size-responsive positioning (maintains relative position on resize)
 * - Overlapping support for layered UI elements
 * - Automatic cleanup on child removal
 * - Default top-left anchoring for unspecified children
 *
 * ## Performance Characteristics
 * - Time Complexity: O(n) for both measure and arrange where n = number of children
 * - Space Complexity: O(n) for anchor mapping storage
 * - No iterative calculations, direct position computation
 * - Suitable for any number of children (no performance degradation)
 *
 * ## Common Use Cases
 * - Floating action buttons (FAB) anchored to corners
 * - Title bars with edge-aligned buttons and centered text
 * - Status indicators at screen edges
 * - Centered modal dialogs and popups
 * - HUD elements in games (health bars, score displays)
 * - Watermarks or logos in corners
 *
 * ## Integration Notes
 * - Works well with z-index for layering overlapping elements
 * - Combines effectively with other layouts (nested layouts)
 * - Respects child visibility (hidden children aren't positioned)
 * - Compatible with animations and transitions
 *
 * @see layout_strategy Base class for layout strategies
 * @see absolute_layout For pixel-perfect absolute positioning
 * @see linear_layout For sequential arrangements
 */

#pragma once

#include <unordered_map>
#include <onyxui/layout/layout_strategy.hh>

namespace onyxui {
    /**
     * @enum anchor_point
     * @brief Predefined anchor points within a parent's content area
     *
     * @details
     * Defines 9 standard positions: corners, edge midpoints, and center.
     * These points remain relative to the parent's bounds, so elements
     * maintain their relative position when the parent resizes.
     *
     * @note Anchor points define where the TOP-LEFT corner of the child
     *       will be positioned (before applying offsets). For example,
     *       bottom_right anchor places the child's top-left at the parent's
     *       bottom-right, effectively pushing the child outside unless
     *       negative offsets are used.
     */
    enum class anchor_point {
        top_left, ///< (0, 0) - Top-left corner of parent
        top_center, ///< (width/2 - child_width/2, 0) - Horizontally centered at top
        top_right, ///< (width - child_width, 0) - Top-right corner
        center_left, ///< (0, height/2 - child_height/2) - Vertically centered at left
        center, ///< (width/2 - child_width/2, height/2 - child_height/2) - Absolute center
        center_right, ///< (width - child_width, height/2 - child_height/2) - Vertically centered at right
        bottom_left, ///< (0, height - child_height) - Bottom-left corner
        bottom_center, ///< (width/2 - child_width/2, height - child_height) - Horizontally centered at bottom
        bottom_right ///< (width - child_width, height - child_height) - Bottom-right corner
    };

    /**
     * @class anchor_layout
     * @brief Layout strategy that positions children at anchor points
     *
     * @details
     * Anchor layout provides a simple way to position children at standard
     * locations within the parent. Each child can be anchored to one of nine
     * predefined points, with optional pixel offsets. Children maintain their
     * relative positions when the parent resizes.
     *
     * ## Key Features
     *
     * - **9 standard anchor points**: Corners, edge centers, and absolute center
     * - **Pixel offsets**: Fine-tune position relative to anchor
     * - **Size-responsive**: Children stay at their anchor as parent resizes
     * - **Overlay-friendly**: Children can overlap (use z-index for ordering)
     * - **Default positioning**: Unanchored children default to top-left
     *
     * ## Positioning Algorithm
     *
     * 1. Calculate anchor point position based on parent dimensions
     * 2. Adjust for child dimensions (ensures child fits within bounds)
     * 3. Apply user-specified pixel offsets
     * 4. Position child at final calculated coordinates
     *
     * ## Edge Cases
     *
     * - **Child larger than parent**: May extend outside parent bounds
     * - **Negative offsets**: Can position children outside parent
     * - **Unmapped children**: Default to top-left (0,0) position
     * - **Hidden children**: Skipped during arrangement
     * - **Empty parent**: All anchors resolve to (0,0)
     *
     * ## Thread Safety
     *
     * - **Mutable state**: Anchor mapping is mutable for lazy configuration
     * - **Not thread-safe**: Don't modify anchors during layout operations
     * - **Stateless positioning**: Position calculation is pure function
     *
     * ## Best Practices
     *
     * - Use negative offsets for margins from edges (e.g., -10px from right)
     * - Combine with z-index for predictable layering
     * - Consider child dimensions when choosing anchor points
     * - Use center anchor for modal dialogs and popups
     * - Test with different parent sizes to ensure responsive behavior
     *
     * @tparam Backend The backend traits type providing rect and size types
     *
     * @example Title Bar Layout
     * @code
     * auto layout = std::make_unique<anchor_layout<MyBackend>>();
     * title_bar->set_layout_strategy(std::move(layout));
     *
     * // Menu button at left edge
     * layout->set_anchor(menu_btn.get(), anchor_point::center_left, 10, 0);
     *
     * // Title text in center
     * layout->set_anchor(title.get(), anchor_point::center);
     *
     * // Close button at right edge with margin
     * layout->set_anchor(close_btn.get(), anchor_point::center_right, -40, 0);
     * @endcode
     *
     * @example Floating Action Button
     * @code
     * // FAB in bottom-right corner with margins
     * auto layout = std::make_unique<anchor_layout<MyBackend>>();
     * screen->set_layout_strategy(std::move(layout));
     *
     * auto fab = create_fab_button();
     * layout->set_anchor(fab.get(), anchor_point::bottom_right, -16, -16);
     * screen->add_child(std::move(fab));
     * @endcode
     *
     * @example Game HUD
     * @code
     * auto layout = std::make_unique<anchor_layout<MyBackend>>();
     * hud->set_layout_strategy(std::move(layout));
     *
     * // Health bar at top-left
     * layout->set_anchor(health_bar.get(), anchor_point::top_left, 20, 20);
     *
     * // Score at top-right
     * layout->set_anchor(score.get(), anchor_point::top_right, -100, 20);
     *
     * // Mini-map at bottom-right
     * layout->set_anchor(minimap.get(), anchor_point::bottom_right, -150, -150);
     *
     * // Inventory at bottom-center
     * layout->set_anchor(inventory.get(), anchor_point::bottom_center, 0, -100);
     * @endcode
     */
    template<UIBackend Backend>
    class anchor_layout : public layout_strategy<Backend> {
        public:
            using elt_t = ui_element<Backend>;
            using rect_type = typename Backend::rect_type;
            using size_type = typename Backend::size_type;

            /**
             * @brief Default constructor
             */
            anchor_layout() = default;

            /**
             * @brief Destructor
             */
            ~anchor_layout() override = default;

            // Rule of Five
            anchor_layout(const anchor_layout&) = delete;
            anchor_layout& operator=(const anchor_layout&) = delete;
            anchor_layout(anchor_layout&&) noexcept = default;
            anchor_layout& operator=(anchor_layout&&) noexcept = default;

            /**
             * @brief Set the anchor point and offset for a child element
             *
             * @param child Pointer to the child element to anchor
             * @param point Which anchor point to use for positioning
             * @param offset_x Horizontal offset in pixels (positive = right, negative = left)
             * @param offset_y Vertical offset in pixels (positive = down, negative = up)
             *
             * @details
             * Configures how a child element should be positioned relative to its parent.
             * The anchor point determines the base position, and offsets provide fine-tuning.
             * This can be called at any time, even after the child is added to the parent.
             *
             * @note
             * - Null children are silently ignored
             * - Offsets can position children outside parent bounds
             * - Previous anchor settings for the child are replaced
             * - Changes take effect on next layout pass
             *
             * @warning Negative offsets from edge anchors (e.g., bottom_right) are often
             *          needed to keep children visible within parent bounds.
             */
            void set_anchor(elt_t* child, anchor_point point,
                            int offset_x = 0, int offset_y = 0) {
                if (!child) return;
                m_anchor_mapping[child] = {point, offset_x, offset_y};
            }

        protected:
            /**
             * @brief Measure all children with full available space
             *
             * @param parent Parent element containing children to measure
             * @param available_width Maximum width available for layout
             * @param available_height Maximum height available for layout
             * @return The full available size (anchor layout always uses all space)
             *
             * @details
             * Override of layout_strategy::measure_children. Anchor layout measures
             * all visible children with the full available space, then returns the
             * full available dimensions. This ensures the parent expands to fill its
             * container, providing maximum space for anchor positioning.
             */
            size_type measure_children(const elt_t* parent,
                                   int available_width,
                                   int available_height) const override;

            /**
             * @brief Arrange children at their configured anchor points
             *
             * @param parent Parent element whose children to arrange
             * @param content_area Rectangle defining the area for child positioning
             *
             * @details
             * Override of layout_strategy::arrange_children. For each visible child:
             * 1. Retrieves anchor configuration (defaults to top_left if not set)
             * 2. Calculates position based on anchor point and child size
             * 3. Applies pixel offsets
             * 4. Positions child at final coordinates
             *
             * @note Children may overlap or extend outside content_area depending
             *       on their anchor configuration and offsets.
             */
            void arrange_children(elt_t* parent, const rect_type& content_area) override;

            /**
             * @brief Clean up anchor mapping when child is removed
             *
             * @param child The child being removed from parent
             *
             * @details
             * Override of layout_strategy::on_child_removed. Removes the child's
             * anchor configuration from the internal mapping to prevent memory leaks.
             */
            void on_child_removed(elt_t* child) noexcept override {
                m_anchor_mapping.erase(child);
            }

            /**
             * @brief Clear all anchor mappings when all children are removed
             *
             * @details
             * Override of layout_strategy::on_children_cleared. Clears the entire
             * anchor mapping when the parent's children collection is cleared.
             */
            void on_children_cleared() noexcept override {
                m_anchor_mapping.clear();
            }

        private:
            /**
             * @struct anchor_info
             * @brief Internal storage for a child's anchor configuration
             *
             * @details
             * Stores the complete positioning information for a child element,
             * including the anchor point and pixel offsets. Used internally by
             * the layout to remember each child's positioning preferences.
             */
            struct anchor_info {
                anchor_point point = anchor_point::top_left; ///< Base anchor point for positioning
                int offset_x = 0; ///< Horizontal pixel offset from anchor
                int offset_y = 0; ///< Vertical pixel offset from anchor
            };

            /// Maps each child to its anchor configuration (mutable for lazy updates)
            mutable std::unordered_map <const elt_t*, anchor_info> m_anchor_mapping;

            /**
             * @brief Calculate exact position for a child based on anchor configuration
             *
             * @param content_area Parent's content rectangle
             * @param child_size Child's measured dimensions
             * @param info Anchor configuration (point and offsets)
             * @param[out] out_x Calculated x coordinate
             * @param[out] out_y Calculated y coordinate
             *
             * @details
             * Pure function that computes the final position for a child element.
             * The calculation:
             * 1. Determines base position from anchor point
             * 2. Adjusts for child dimensions (e.g., centering)
             * 3. Applies pixel offsets
             *
             * @note This is a static helper function with no side effects.
             *       The child may be positioned outside content_area bounds.
             */
            static void calculate_anchor_position(const rect_type& content_area,
                                                  const size_type& child_size,
                                                  const anchor_info& info,
                                                  int& out_x, int& out_y);
    };

    // ====================================================================================================
    // Implementation
    // ====================================================================================================

    // --------------------------------------------------------------------------------------------------------------
    template<UIBackend Backend>
    typename Backend::size_type anchor_layout<Backend>::measure_children(const elt_t* parent, int available_width,
                                                         int available_height) const {
        // Measure all children with available space
        for (const auto& child : this->get_children(parent)) {
            if (child->is_visible()) {
                [[maybe_unused]] auto size = child->measure(available_width, available_height);
            }
        }

        // Anchor layout uses all available space
        size_type result = {};
        size_utils::set_size(result, available_width, available_height);
        return result;
    }

    // -------------------------------------------------------------------------------------------------------------
    template<UIBackend Backend>
    void anchor_layout<Backend>::arrange_children(elt_t* parent, const rect_type& content_area) {
        for (auto& child : this->get_mutable_children(parent)) {
            if (!child->is_visible()) continue;

            auto it = m_anchor_mapping.find(child.get());
            const anchor_info info = (it != m_anchor_mapping.end())
                                   ? it->second
                                   : anchor_info{anchor_point::top_left, 0, 0};

            const size_type& measured = this->get_last_measured_size(child.get());

            // Calculate child size (handle percentage policy)
            int child_w = size_utils::get_width(measured);
            int child_h = size_utils::get_height(measured);

            int const content_w = rect_utils::get_width(content_area);
            int const content_h = rect_utils::get_height(content_area);

            if (child->w_constraint().policy == size_policy::percentage) {
                int const percentage_w = static_cast<int>(static_cast<float>(content_w) * child->w_constraint().percentage);
                child_w = child->w_constraint().clamp(percentage_w);
            }

            if (child->h_constraint().policy == size_policy::percentage) {
                int const percentage_h = static_cast<int>(static_cast<float>(content_h) * child->h_constraint().percentage);
                child_h = child->h_constraint().clamp(percentage_h);
            }

            // Calculate position based on anchor point (using potentially adjusted child size)
            size_type child_size;
            size_utils::set_size(child_size, child_w, child_h);

            int child_x, child_y;
            calculate_anchor_position(content_area, child_size, info,
                                      child_x, child_y);

            rect_type child_bounds;
            rect_utils::set_bounds(child_bounds, child_x, child_y, child_w, child_h);
            child->arrange(geometry::relative_rect<Backend>{child_bounds});
        }
    }

    // ---------------------------------------------------------------------------------------------------------------
    template<UIBackend Backend>
    void anchor_layout<Backend>::calculate_anchor_position(const rect_type& content_area, const size_type& child_size,
                                                                 const anchor_info& info, int& out_x, int& out_y) {
        // RELATIVE coordinates - 0,0 = top-left of content area
        const int content_x = 0;
        const int content_y = 0;
        const int content_w = rect_utils::get_width(content_area);
        const int content_h = rect_utils::get_height(content_area);
        const int child_w = size_utils::get_width(child_size);
        const int child_h = size_utils::get_height(child_size);

        switch (info.point) {
            case anchor_point::top_left:
                out_x = content_x;
                out_y = content_y;
                break;

            case anchor_point::top_center:
                out_x = content_x + (content_w - child_w) / 2;
                out_y = content_y;
                break;

            case anchor_point::top_right:
                out_x = content_x + content_w - child_w;
                out_y = content_y;
                break;

            case anchor_point::center_left:
                out_x = content_x;
                out_y = content_y + (content_h - child_h) / 2;
                break;

            case anchor_point::center:
                out_x = content_x + (content_w - child_w) / 2;
                out_y = content_y + (content_h - child_h) / 2;
                break;

            case anchor_point::center_right:
                out_x = content_x + content_w - child_w;
                out_y = content_y + (content_h - child_h) / 2;
                break;

            case anchor_point::bottom_left:
                out_x = content_x;
                out_y = content_y + content_h - child_h;
                break;

            case anchor_point::bottom_center:
                out_x = content_x + (content_w - child_w) / 2;
                out_y = content_y + content_h - child_h;
                break;

            case anchor_point::bottom_right:
                out_x = content_x + content_w - child_w;
                out_y = content_y + content_h - child_h;
                break;
        }

        out_x += info.offset_x;
        out_y += info.offset_y;
    }
}
