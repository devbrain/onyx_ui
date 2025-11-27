/**
 * @file absolute_layout.hh
 * @brief Absolute positioning layout strategy with fixed coordinates
 * @author igor
 * @date 09/10/2025
 *
 * @details
 * Absolute layout positions children at explicit (x, y) coordinates, optionally
 * with explicit sizes. This is the most direct positioning method, similar to
 * CSS position:absolute, and is useful for custom-positioned overlays, tooltips,
 * or when precise control is needed.
 *
 * ## Features
 * - Precise positioning with explicit coordinates
 * - Optional size overrides for each child
 * - Free-form overlapping support
 * - Coordinates relative to parent's content area
 * - Automatic bounding box calculation
 * - Default positioning at origin (0, 0)
 * - Visibility-aware layout (hidden children ignored)
 *
 * ## Performance Characteristics
 * - Time Complexity: O(n) for measure and arrange where n = number of children
 * - Space Complexity: O(n) for position mapping storage
 * - No iterative calculations or constraint solving
 * - Optimal for static layouts with known positions
 *
 * ## Common Use Cases
 * - Drag-and-drop interfaces with user-positioned elements
 * - Custom tooltips positioned near cursor or target
 * - Overlays and floating panels at specific locations
 * - Node-based editors with connected elements
 * - Game UI with precise element placement
 * - Prototyping and mockup implementations
 *
 * ## Comparison with Other Layouts
 * - vs anchor_layout: Absolute uses exact coordinates, anchor uses relative positions
 * - vs grid_layout: Absolute allows overlapping, grid enforces cell structure
 * - vs linear_layout: Absolute has no automatic flow, linear stacks children
 *
 * @see layout_strategy Base class for layout strategies
 * @see anchor_layout For relative positioning to parent edges
 * @see grid_layout For structured tabular layouts
 */

#pragma once

#include <unordered_map>
#include <onyxui/layout/layout_strategy.hh>

namespace onyxui {
    /**
     * @class absolute_layout
     * @brief Layout strategy that positions children at explicit coordinates
     *
     * @details
     * Absolute layout gives complete control over child positioning. Each child
     * can be assigned a specific (x, y) position and optionally a fixed size.
     * Children without explicit positions default to (0, 0). This layout is ideal
     * when you need precise, precise control over element placement.
     *
     * ## Key Features
     *
     * - **Explicit positioning**: Set exact (x, y) coordinates for each child
     * - **Optional sizing**: Override child's measured size with fixed dimensions
     * - **No automatic flow**: Children can overlap freely
     * - **Relative to content area**: Coordinates are relative to parent's content area
     * - **Bounding box calculation**: Parent size encompasses all children
     *
     * ## Positioning Algorithm
     *
     * 1. Each child is measured with available space
     * 2. Position is retrieved from mapping (default: 0, 0)
     * 3. Size is either override value or measured size
     * 4. Child is positioned at content_area origin + specified offset
     * 5. Parent's size is calculated as bounding box of all children
     *
     * ## Edge Cases
     *
     * - **Negative coordinates**: Allowed, positions children outside parent
     * - **Size override with -1**: Uses child's measured size
     * - **Unmapped children**: Default to position (0, 0) with measured size
     * - **Overlapping children**: Render order follows child addition order
     * - **Empty parent**: Returns (0, 0) size
     * - **Hidden children**: Ignored in both measurement and arrangement
     *
     * ## Thread Safety
     *
     * - **Mutable mapping**: Position mapping is mutable for lazy configuration
     * - **Not thread-safe**: Don't modify positions during layout operations
     * - **Stateless calculation**: Position calculation has no side effects
     *
     * ## Best Practices
     *
     * - Use for static layouts with known positions
     * - Consider z-ordering for overlapping elements
     * - Test with different content sizes when using auto-sizing
     * - Use negative coordinates carefully (may clip)
     * - Prefer anchor_layout for responsive positioning
     * - Cache positions when implementing drag-and-drop
     *
     * @tparam Backend The backend traits type providing rect and size types
     *
     * @example Tooltip Positioning
     * @code
     * auto layout = std::make_unique<absolute_layout<MyBackend>>();
     * overlay->set_layout_strategy(std::move(layout));
     *
     * // Position tooltip near mouse cursor
     * auto tooltip = create_tooltip("Click to continue");
     * int mouse_x, mouse_y;
     * get_mouse_position(&mouse_x, &mouse_y);
     *
     * // Position tooltip offset from cursor
     * layout->set_position(tooltip.get(), mouse_x + 10, mouse_y - 30);
     * overlay->add_child(std::move(tooltip));
     * @endcode
     *
     * @example Drag and Drop Interface
     * @code
     * auto layout = std::make_unique<absolute_layout<MyBackend>>();
     * canvas->set_layout_strategy(std::move(layout));
     *
     * // Position draggable items
     * for (auto& item : draggable_items) {
     *     layout->set_position(item.get(), item->drag_x, item->drag_y);
     * }
     *
     * // Update position during drag
     * void on_drag(DraggableItem* item, int new_x, int new_y) {
     *     layout->set_position(item, new_x, new_y);
     *     canvas->invalidate_layout();  // Trigger re-layout
     * }
     * @endcode
     *
     * @example Custom Dialog Layout
     * @code
     * auto layout = std::make_unique<absolute_layout<MyBackend>>();
     * dialog->set_layout_strategy(std::move(layout));
     *
     * // Title at top
     * layout->set_position(title.get(), 20, 10, 260, 30);
     *
     * // Message in middle
     * layout->set_position(message.get(), 20, 50, 260, 80);
     *
     * // Buttons at bottom
     * layout->set_position(ok_btn.get(), 50, 150, 80, 30);
     * layout->set_position(cancel_btn.get(), 170, 150, 80, 30);
     *
     * // Fixed dialog size
     * dialog->set_size(300, 200);
     * @endcode
     *
     * @example Node Editor
     * @code
     * auto layout = std::make_unique<absolute_layout<MyBackend>>();
     * node_canvas->set_layout_strategy(std::move(layout));
     *
     * // Position nodes based on graph structure
     * for (auto& node : graph.nodes()) {
     *     // Fixed size for consistent appearance
     *     layout->set_position(node->widget.get(),
     *                         node->x, node->y,
     *                         NODE_WIDTH, NODE_HEIGHT);
     * }
     *
     * // Connections drawn separately (not part of layout)
     * @endcode
     */
    template<UIBackend Backend>
    class absolute_layout : public layout_strategy<Backend> {
        public:
            using elt_t = ui_element<Backend>;
            using rect_type = typename Backend::rect_type;
            using size_type = typename Backend::size_type;

            /**
             * @brief Default constructor
             */
            absolute_layout() = default;

            /**
             * @brief Destructor
             */
            ~absolute_layout() override = default;

            // Rule of Five
            absolute_layout(const absolute_layout&) = delete;
            absolute_layout& operator=(const absolute_layout&) = delete;
            absolute_layout(absolute_layout&&) noexcept = default;
            absolute_layout& operator=(absolute_layout&&) noexcept = default;

            /**
             * @brief Set the position (and optionally size) for a child element
             *
             * @param child Pointer to the child element to position
             * @param x X coordinate in logical units relative to parent's content area
             * @param y Y coordinate in logical units relative to parent's content area
             * @param width Width override in logical units (-1 to use measured width, 0 to hide)
             * @param height Height override in logical units (-1 to use measured height, 0 to hide)
             *
             * @details
             * Configures the exact position and optional size for a child element.
             * Coordinates are relative to the parent's content area origin.
             * Size overrides bypass the child's natural size preferences.
             *
             * @note
             * - Null children are silently ignored
             * - Negative coordinates are allowed (positions outside parent)
             * - Width/height of 0 effectively hides the element
             * - Previous position settings are replaced
             * - Changes take effect on next layout pass
             *
             * @warning Size overrides ignore the child's min/max constraints.
             *          Use with caution to avoid breaking child layouts.
             */
            void set_position(elt_t* child, int x, int y,
                              int width = -1, int height = -1) {
                if (!child) return;
                m_position_mapping[child] = {x, y, width, height};
            }

        protected:
            /**
             * @brief Calculate bounding box that contains all positioned children
             *
             * @param parent Parent element whose children to measure
             * @param available_width Maximum width available for layout
             * @param available_height Maximum height available for layout
             * @return Size that encompasses all positioned children
             *
             * @details
             * Override of layout_strategy::measure_children. Calculates the
             * minimum rectangle that contains all children by:
             * 1. Measuring each visible child
             * 2. Calculating each child's bottom-right corner (x+width, y+height)
             * 3. Finding the maximum extents
             * 4. Returning the bounding box dimensions
             *
             * @note Children at negative coordinates don't affect the bounding box.
             *       The parent's size is always at least (0, 0).
             */
            logical_size measure_children(const elt_t* parent,
                                   logical_unit available_width,
                                   logical_unit available_height) const override;

            /**
             * @brief Position children at their configured coordinates
             *
             * @param parent Parent element whose children to arrange
             * @param content_area Rectangle defining the area for positioning
             *
             * @details
             * Override of layout_strategy::arrange_children. For each visible child:
             * 1. Retrieves position configuration (defaults to 0,0 if not set)
             * 2. Calculates absolute position by adding content area offset
             * 3. Applies size override or uses measured size
             * 4. Positions child at calculated bounds
             *
             * @note Children may be positioned outside content_area bounds.
             *       Clipping behavior depends on the parent's rendering settings.
             */
            void arrange_children(elt_t* parent, const logical_rect& content_area) override;

            /**
             * @brief Clean up position mapping when child is removed
             *
             * @param child The child being removed from parent
             *
             * @details
             * Override of layout_strategy::on_child_removed. Removes the child's
             * position configuration to prevent memory leaks.
             */
            void on_child_removed(elt_t* child) noexcept override {
                m_position_mapping.erase(child);
            }

            /**
             * @brief Clear all position mappings when all children are removed
             *
             * @details
             * Override of layout_strategy::on_children_cleared. Clears the entire
             * position mapping when the parent's children collection is cleared.
             */
            void on_children_cleared() noexcept override {
                m_position_mapping.clear();
            }

        private:
            /**
             * @struct position_info
             * @brief Internal storage for a child's position and size configuration
             *
             * @details
             * Stores the complete positioning information for a child element,
             * including absolute coordinates and optional size overrides.
             * Values of -1 for width/height indicate auto-sizing.
             */
            struct position_info {
                int x = 0; ///< X coordinate relative to parent's content area origin
                int y = 0; ///< Y coordinate relative to parent's content area origin
                int width = -1; ///< Width override in logical units (-1 for measured width)
                int height = -1; ///< Height override in logical units (-1 for measured height)
            };

            /// Maps each child to its position configuration (mutable for lazy updates)
            mutable std::unordered_map <const elt_t*, position_info> m_position_mapping;
    };

    // ===================================================================================================
    // Implementation
    // ===================================================================================================

    // -----------------------------------------------------------------------------------------------------
    template<UIBackend Backend>
    logical_size absolute_layout<Backend>::measure_children(const elt_t* parent, logical_unit available_width,
                                                           logical_unit available_height) const {
        logical_unit max_width = logical_unit(0.0);
        logical_unit max_height = logical_unit(0.0);

        // Measure all children
        for (const auto& child : this->get_children(parent)) {
            if (!child->is_visible()) continue;

            logical_size const measured = child->measure(available_width, available_height);
            logical_unit const meas_w = measured.width;
            logical_unit const meas_h = measured.height;

            auto it = m_position_mapping.find(child.get());
            if (it != m_position_mapping.end()) {
                const position_info& pos = it->second;

                logical_unit const child_width = (pos.width > 0) ? logical_unit(static_cast<double>(pos.width)) : meas_w;
                logical_unit const child_height = (pos.height > 0) ? logical_unit(static_cast<double>(pos.height)) : meas_h;

                max_width = max(max_width, logical_unit(static_cast<double>(pos.x)) + child_width);
                max_height = max(max_height, logical_unit(static_cast<double>(pos.y)) + child_height);
            } else {
                max_width = max(max_width, meas_w);
                max_height = max(max_height, meas_h);
            }
        }

        return logical_size{max_width, max_height};
    }

    // -----------------------------------------------------------------------------------------------------
    template<UIBackend Backend>
    void absolute_layout<Backend>::arrange_children(elt_t* parent, const logical_rect& content_area) {
        // RELATIVE coordinates - positions are relative to content area (0,0)
        // pos.x and pos.y are already relative offsets
        (void)content_area;  // Unused in relative coordinate system (only dimensions matter, from measure phase)

        for (auto& child : this->get_mutable_children(parent)) {
            if (!child->is_visible()) continue;

            auto it = m_position_mapping.find(child.get());
            const position_info pos = (it != m_position_mapping.end())
                                    ? it->second
                                    : position_info{0, 0, -1, -1};

            const logical_size& measured = this->get_last_measured_size(child.get());
            logical_unit const meas_w = measured.width;
            logical_unit const meas_h = measured.height;

            // Position is relative to content area
            logical_unit const child_x = logical_unit(static_cast<double>(pos.x));
            logical_unit const child_y = logical_unit(static_cast<double>(pos.y));
            logical_unit const child_width = (pos.width > 0) ? logical_unit(static_cast<double>(pos.width)) : meas_w;
            logical_unit const child_height = (pos.height > 0) ? logical_unit(static_cast<double>(pos.height)) : meas_h;

            logical_rect child_bounds{child_x, child_y, child_width, child_height};
            child->arrange(child_bounds);
        }
    }
}
