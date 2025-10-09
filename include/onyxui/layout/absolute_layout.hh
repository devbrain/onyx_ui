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
    class absolute_layout : layout_strategy <TRect, TSize> {
        using elt_t = ui_element <TRect, TSize>;

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
        std::unordered_map <elt_t*, position_info> position_mapping;

        /**
         * @brief Set the position (and optionally size) for a child element
         * @param child Pointer to the child element
         * @param x X coordinate in pixels
         * @param y Y coordinate in pixels
         * @param width Width override (-1 to use measured width)
         * @param height Height override (-1 to use measured height)
         */
        void set_position(elt_t* child, int x, int y,
                          int width = -1, int height = -1);

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

    template<RectLike TRect, SizeLike TSize>
    void absolute_layout<TRect, TSize>::set_position(elt_t* child, int x, int y, int width, int height) {
        position_mapping[child] = {x, y, width, height};
    }
    // -----------------------------------------------------------------------------------------------------
    template<RectLike TRect, SizeLike TSize>
    TSize absolute_layout<TRect, TSize>::measure_children(elt_t* parent, int available_width, int available_height) {
        int max_width = 0;
        int max_height = 0;

        // Measure all children
        for (auto& child : parent->children) {
            if (!child->visible) continue;

            auto measured = child->measure(available_width, available_height);

            auto it = position_mapping.find(child.get());
            if (it != position_mapping.end()) {
                const position_info& pos = it->second;

                int child_width = (pos.width > 0) ? pos.width : measured.width;
                int child_height = (pos.height > 0) ? pos.height : measured.height;

                max_width = std::max(max_width, pos.x + child_width);
                max_height = std::max(max_height, pos.y + child_height);
            } else {
                max_width = std::max(max_width, measured.width);
                max_height = std::max(max_height, measured.height);
            }
        }

        return {max_width, max_height};
    }
    // -----------------------------------------------------------------------------------------------------
    template<RectLike TRect, SizeLike TSize>
    void absolute_layout<TRect, TSize>::arrange_children(elt_t* parent, const TRect& content_area) {
        for (auto& child : parent->children) {
            if (!child->visible) continue;

            auto it = position_mapping.find(child.get());
            position_info pos = (it != position_mapping.end())
                                    ? it->second
                                    : position_info{0, 0, -1, -1};

            auto measured = child->last_measured_size;

            int child_x = content_area.x + pos.x;
            int child_y = content_area.y + pos.y;
            int child_width = (pos.width > 0) ? pos.width : measured.width;
            int child_height = (pos.height > 0) ? pos.height : measured.height;

            child->arrange({child_x, child_y, child_width, child_height});
        }
    }
}
