/**
 * @file anchor_layout.hh
 * @brief Anchor-based layout strategy for positioning at predefined points
 * @author igor
 * @date 09/10/2025
 *
 * Anchor layout positions children relative to predefined anchor points within
 * the parent (e.g., top-left, center, bottom-right). This is useful for UI
 * elements that should stay at specific positions regardless of parent size,
 * similar to CSS anchor positioning or Unity's anchor system.
 */

#pragma once

#include <unordered_map>
#include <onyxui/layout_strategy.hh>

namespace onyxui {
    /**
     * @enum anchor_point
     * @brief Predefined anchor points within a parent's content area
     *
     * Defines 9 standard positions: corners, edge midpoints, and center.
     */
    enum class anchor_point {
        top_left,       ///< Anchor to top-left corner
        top_center,     ///< Anchor to top edge center
        top_right,      ///< Anchor to top-right corner
        center_left,    ///< Anchor to left edge center
        center,         ///< Anchor to absolute center
        center_right,   ///< Anchor to right edge center
        bottom_left,    ///< Anchor to bottom-left corner
        bottom_center,  ///< Anchor to bottom edge center
        bottom_right    ///< Anchor to bottom-right corner
    };

    /**
     * @class anchor_layout
     * @brief Layout strategy that positions children at anchor points
     *
     * Anchor layout provides a simple way to position children at standard
     * locations within the parent. Each child can be anchored to one of nine
     * predefined points, with optional pixel offsets.
     *
     * ## Key Features
     *
     * - **9 standard anchor points**: Corners, edge centers, and absolute center
     * - **Pixel offsets**: Fine-tune position relative to anchor
     * - **Size-responsive**: Children stay at their anchor as parent resizes
     * - **Overlay-friendly**: Children can overlap (use z-index for ordering)
     *
     * ## Use Cases
     *
     * - Title bars with centered text and corner buttons
     * - Floating action buttons (e.g., bottom-right)
     * - Status indicators anchored to edges
     * - Centered modals or dialogs
     *
     * @tparam TRect Rectangle type satisfying RectLike concept
     * @tparam TSize Size type satisfying SizeLike concept
     *
     * @example
     * @code
     * auto layout = std::make_unique<anchor_layout<SDL_Rect, SDL_Size>>();
     * panel->set_layout_strategy(std::move(layout));
     *
     * // Center a dialog
     * layout->set_anchor(dialog.get(), anchor_point::center);
     *
     * // Close button in top-right with 10px margin
     * layout->set_anchor(close_btn.get(), anchor_point::top_right, -10, 10);
     * @endcode
     */
    template<RectLike TRect, SizeLike TSize>
    class anchor_layout : layout_strategy <TRect, TSize> {
        using elt_t = ui_element <TRect, TSize>;

        /**
         * @struct anchor_info
         * @brief Anchor configuration for a child element
         */
        struct anchor_info {
            anchor_point point = anchor_point::top_left;  ///< Which anchor point to use
            int offset_x = 0;                              ///< Horizontal offset from anchor (pixels)
            int offset_y = 0;                              ///< Vertical offset from anchor (pixels)
        };

        /// Maps children to their anchor configurations
        std::unordered_map <elt_t*, anchor_info> anchor_mapping;

        /**
         * @brief Set the anchor point and offset for a child element
         * @param child Pointer to the child element
         * @param point Which anchor point to use
         * @param offset_x Horizontal offset in pixels (default 0)
         * @param offset_y Vertical offset in pixels (default 0)
         */
        void set_anchor(elt_t* child, anchor_point point,
                        int offset_x = 0, int offset_y = 0);

        /**
         * @brief Measure all children (anchor layout uses full available space)
         * @param parent Parent element
         * @param available_width Available width
         * @param available_height Available height
         * @return Full available size
         */
        TSize measure_children(elt_t* parent,
                               int available_width,
                               int available_height) override;

        /**
         * @brief Position children at their anchor points
         * @param parent Parent element
         * @param content_area Area for positioning children
         */
        void arrange_children(elt_t* parent, const TRect& content_area) override;

        /**
         * @brief Calculate final position from anchor point and offsets
         * @param content_area Parent's content area
         * @param child_size Child's measured size
         * @param info Anchor configuration
         * @param out_x Output: calculated x position
         * @param out_y Output: calculated y position
         */
        static void calculate_anchor_position(const TRect& content_area,
                                              const TSize& child_size,
                                              const anchor_info& info,
                                              int& out_x, int& out_y);
    };

    // ====================================================================================================
    // Implementation
    // ====================================================================================================
    template<RectLike TRect, SizeLike TSize>
    void anchor_layout <TRect, TSize>::set_anchor(elt_t* child, anchor_point point, int offset_x, int offset_y) {
        anchor_mapping[child] = {point, offset_x, offset_y};
    }

    // --------------------------------------------------------------------------------------------------------------
    template<RectLike TRect, SizeLike TSize>
    TSize anchor_layout <TRect, TSize>::measure_children(elt_t* parent, int available_width, int available_height) {
        // Measure all children with available space
        for (auto& child : parent->children) {
            if (child->visible) {
                child->measure(available_width, available_height);
            }
        }

        // Anchor layout uses all available space
        return {available_width, available_height};
    }

    // -------------------------------------------------------------------------------------------------------------
    template<RectLike TRect, SizeLike TSize>
    void anchor_layout <TRect, TSize>::arrange_children(elt_t* parent, const TRect& content_area) {
        for (auto& child : parent->children) {
            if (!child->visible) continue;

            auto it = anchor_mapping.find(child.get());
            anchor_info info = (it != anchor_mapping.end())
                                   ? it->second
                                   : anchor_info{anchor_point::top_left, 0, 0};

            auto measured = child->last_measured_size;

            int child_x, child_y;
            calculate_anchor_position(content_area, measured, info,
                                      child_x, child_y);

            child->arrange({child_x, child_y, measured.width, measured.height});
        }
    }
    // ---------------------------------------------------------------------------------------------------------------
    template<RectLike TRect, SizeLike TSize>
    void anchor_layout <TRect, TSize>::calculate_anchor_position(const TRect& content_area, const TSize& child_size,
                                                                 const anchor_info& info, int& out_x, int& out_y) {
        switch (info.point) {
            case anchor_point::top_left:
                out_x = content_area.x;
                out_y = content_area.y;
                break;

            case anchor_point::top_center:
                out_x = content_area.x + (content_area.w - child_size.width) / 2;
                out_y = content_area.y;
                break;

            case anchor_point::top_right:
                out_x = content_area.x + content_area.w - child_size.width;
                out_y = content_area.y;
                break;

            case anchor_point::center_left:
                out_x = content_area.x;
                out_y = content_area.y + (content_area.h - child_size.height) / 2;
                break;

            case anchor_point::center:
                out_x = content_area.x + (content_area.w - child_size.width) / 2;
                out_y = content_area.y + (content_area.h - child_size.height) / 2;
                break;

            case anchor_point::center_right:
                out_x = content_area.x + content_area.w - child_size.width;
                out_y = content_area.y + (content_area.h - child_size.height) / 2;
                break;

            case anchor_point::bottom_left:
                out_x = content_area.x;
                out_y = content_area.y + content_area.h - child_size.height;
                break;

            case anchor_point::bottom_center:
                out_x = content_area.x + (content_area.w - child_size.width) / 2;
                out_y = content_area.y + content_area.h - child_size.height;
                break;

            case anchor_point::bottom_right:
                out_x = content_area.x + content_area.w - child_size.width;
                out_y = content_area.y + content_area.h - child_size.height;
                break;
        }

        out_x += info.offset_x;
        out_y += info.offset_y;
    }
}
