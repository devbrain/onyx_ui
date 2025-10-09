/**
 * @file absolute_layout.hh
 * @brief Absolute positioning layout strategy with fixed coordinates
 * @author igor
 * @date 09/10/2025
 *
 * Absolute layout positions children at explicit (x, y) coordinates, optionally
 * with explicit sizes. This is the most direct positioning method, similar to
 * CSS position:absolute, and is useful for custom-positioned overlays, tooltips,
 * or when precise control is needed.
 */

#pragma once

#include <unordered_map>
#include <onyxui/layout_strategy.hh>

namespace onyxui {
    /**
     * @class absolute_layout
     * @brief Layout strategy that positions children at explicit coordinates
     *
     * Absolute layout gives complete control over child positioning. Each child
     * can be assigned a specific (x, y) position and optionally a fixed size.
     * Children without explicit positions default to (0, 0).
     *
     * ## Key Features
     *
     * - **Explicit positioning**: Set exact (x, y) coordinates for each child
     * - **Optional sizing**: Optionally override child's measured size
     * - **No automatic flow**: Children can overlap freely
     * - **Relative to content area**: Coordinates are relative to parent's content area
     *
     * ## Use Cases
     *
     * - Custom positioned overlays or popups
     * - Tooltips positioned near their targets
     * - Manual layout for complex custom controls
     * - Drag-and-drop interfaces
     *
     * @tparam TRect Rectangle type satisfying RectLike concept
     * @tparam TSize Size type satisfying SizeLike concept
     *
     * @example
     * @code
     * auto layout = std::make_unique<absolute_layout<SDL_Rect, SDL_Size>>();
     * panel->set_layout_strategy(std::move(layout));
     *
     * // Position a child at (10, 20) with auto size
     * layout->set_position(child.get(), 10, 20);
     *
     * // Position with explicit size
     * layout->set_position(other_child.get(), 50, 50, 100, 30);
     * @endcode
     */
    template<RectLike TRect, SizeLike TSize>
    class absolute_layout : public layout_strategy <TRect, TSize> {
    public:
        using elt_t = ui_element <TRect, TSize>;

        /**
         * @brief Set the position (and optionally size) for a child element
         * @param child Pointer to the child element
         * @param x X coordinate in pixels
         * @param y Y coordinate in pixels
         * @param width Width override (-1 to use measured width)
         * @param height Height override (-1 to use measured height)
         */
        void set_position(elt_t* child, int x, int y,
                          int width = -1, int height = -1) {
            if (!child) return;
            m_position_mapping[child] = {x, y, width, height};
        }

    private:
        /**
         * @struct position_info
         * @brief Position and optional size override for a child element
         */
        struct position_info {
            int x = 0;        ///< X coordinate relative to content area
            int y = 0;        ///< Y coordinate relative to content area
            int width = -1;   ///< Width override (-1 means use measured width)
            int height = -1;  ///< Height override (-1 means use measured height)
        };

        /// Maps children to their explicit positions
        std::unordered_map <elt_t*, position_info> m_position_mapping;

        /**
         * @brief Calculate bounding box that contains all children
         * @param parent Parent element
         * @param available_width Available width
         * @param available_height Available height
         * @return Size that encompasses all positioned children
         */
        TSize measure_children(elt_t* parent,
                               int available_width,
                               int available_height) override;

        /**
         * @brief Position children at their explicit coordinates
         * @param parent Parent element
         * @param content_area Area for positioning children
         */
        void arrange_children(elt_t* parent, const TRect& content_area) override;
    };

    // ===================================================================================================
    // Implementation
    // ===================================================================================================

    // -----------------------------------------------------------------------------------------------------
    template<RectLike TRect, SizeLike TSize>
    TSize absolute_layout<TRect, TSize>::measure_children(elt_t* parent, int available_width, int available_height) {
        int max_width = 0;
        int max_height = 0;

        // Measure all children
        for (auto& child : parent->children()) {
            if (!child->is_visible()) continue;

            TSize measured = child->measure(available_width, available_height);
            int meas_w = size_utils::get_width(measured);
            int meas_h = size_utils::get_height(measured);

            auto it = m_position_mapping.find(child.get());
            if (it != m_position_mapping.end()) {
                const position_info& pos = it->second;

                int child_width = (pos.width > 0) ? pos.width : meas_w;
                int child_height = (pos.height > 0) ? pos.height : meas_h;

                max_width = std::max(max_width, pos.x + child_width);
                max_height = std::max(max_height, pos.y + child_height);
            } else {
                max_width = std::max(max_width, meas_w);
                max_height = std::max(max_height, meas_h);
            }
        }

        TSize result = {};
        size_utils::set_size(result, max_width, max_height);
        return result;
    }
    // -----------------------------------------------------------------------------------------------------
    template<RectLike TRect, SizeLike TSize>
    void absolute_layout<TRect, TSize>::arrange_children(elt_t* parent, const TRect& content_area) {
        int content_x = rect_utils::get_x(content_area);
        int content_y = rect_utils::get_y(content_area);

        for (auto& child : parent->children()) {
            if (!child->is_visible()) continue;

            auto it = m_position_mapping.find(child.get());
            position_info pos = (it != m_position_mapping.end())
                                    ? it->second
                                    : position_info{0, 0, -1, -1};

            const TSize& measured = child->last_measured_size();
            int meas_w = size_utils::get_width(measured);
            int meas_h = size_utils::get_height(measured);

            int child_x = content_x + pos.x;
            int child_y = content_y + pos.y;
            int child_width = (pos.width > 0) ? pos.width : meas_w;
            int child_height = (pos.height > 0) ? pos.height : meas_h;

            TRect child_bounds;
            rect_utils::set_bounds(child_bounds, child_x, child_y, child_width, child_height);
            child->arrange(child_bounds);
        }
    }
}
