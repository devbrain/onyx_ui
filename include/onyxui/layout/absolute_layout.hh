//
// Created by igor on 09/10/2025.
//

#pragma once

#include <unordered_map>
#include <onyxui/layout_strategy.hh>

namespace onyxui {
    template<RectLike TRect, SizeLike TSize>
    class absolute_layout : layout_strategy <TRect, TSize> {
        using elt_t = ui_element <TRect, TSize>;

        struct position_info {
            int x = 0;
            int y = 0;
            int width = -1; // -1 means use measured width
            int height = -1; // -1 means use measured height
        };

        std::unordered_map <elt_t*, position_info> position_mapping;

        void set_position(elt_t* child, int x, int y,
                          int width = -1, int height = -1);

        TSize measure_children(elt_t* parent,
                               int available_width,
                               int available_height) override;

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
