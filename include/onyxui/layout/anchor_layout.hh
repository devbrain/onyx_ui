//
// Created by igor on 09/10/2025.
//

#pragma once

#include <unordered_map>
#include <onyxui/layout_strategy.hh>

namespace onyxui {
    enum class anchor_point {
        top_left,
        top_center,
        top_right,
        center_left,
        center,
        center_right,
        bottom_left,
        bottom_center,
        bottom_right
    };

    template<RectLike TRect, SizeLike TSize>
    class anchor_layout : layout_strategy <TRect, TSize> {
        using elt_t = ui_element <TRect, TSize>;

        struct anchor_info {
            anchor_point point = anchor_point::top_left;
            int offset_x = 0;
            int offset_y = 0;
        };

        std::unordered_map <elt_t*, anchor_info> anchor_mapping;

        void set_anchor(elt_t* child, anchor_point point,
                        int offset_x = 0, int offset_y = 0);

        TSize measure_children(elt_t* parent,
                               int available_width,
                               int available_height) override;

        void arrange_children(elt_t* parent, const TRect& content_area) override;

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
