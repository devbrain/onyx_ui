/**
 * @file linear_layout.hh
 * @brief Linear layout strategy for stacking elements horizontally or vertically
 * @author igor
 * @date 09/10/2025
 *
 * Linear layout arranges children in a single row (horizontal) or column (vertical),
 * similar to CSS flexbox with flex-direction. Supports spacing between items,
 * alignment, and various size policies (fixed, expand, weighted, etc.).
 */

#pragma once

#include <vector>
#include <onyxui/layout_strategy.hh>

namespace onyxui {
    /**
     * @class linear_layout
     * @brief Layout strategy that stacks children in a line (horizontal or vertical)
     *
     * Linear layout is one of the most commonly used layout strategies. It arranges
     * children sequentially in either horizontal or vertical direction, with support for:
     *
     * - **Spacing**: Gap between adjacent children
     * - **Alignment**: How children align perpendicular to layout direction
     * - **Size policies**: Fixed, content, expand, weighted distribution
     * - **Min/max constraints**: Enforced on each child
     *
     * ## Measurement Algorithm
     *
     * For vertical layout:
     * - Width = max(child widths)
     * - Height = sum(child heights) + spacing
     *
     * For horizontal layout:
     * - Width = sum(child widths) + spacing
     * - Height = max(child heights)
     *
     * ## Arrangement Algorithm
     *
     * 1. Calculate fixed-size children space
     * 2. Distribute remaining space among expand/weighted children
     * 3. Position each child sequentially
     * 4. Apply alignment perpendicular to flow direction
     *
     * @tparam TRect Rectangle type satisfying RectLike concept
     * @tparam TSize Size type satisfying SizeLike concept
     *
     * @example
     * @code
     * // Create vertical list with buttons
     * auto layout = std::make_unique<linear_layout<SDL_Rect, SDL_Size>>();
     * layout->layout_direction = direction::vertical;
     * layout->spacing = 10;
     * layout->h_align = horizontal_alignment::stretch;
     *
     * panel->set_layout_strategy(std::move(layout));
     * @endcode
     */
    template<RectLike TRect, SizeLike TSize>
    class linear_layout : public layout_strategy <TRect, TSize> {  // PUBLIC inheritance
    public:
        using elt_t = ui_element <TRect, TSize>;

        /**
         * @brief Construct linear layout with immutable configuration
         * @param dir Stack direction (vertical or horizontal)
         * @param spacing Gap between children in pixels
         * @param h_align Horizontal alignment for children
         * @param v_align Vertical alignment for children
         */
        explicit linear_layout(
            direction dir = direction::vertical,
            int spacing = 0,
            horizontal_alignment h_align = horizontal_alignment::stretch,
            vertical_alignment v_align = vertical_alignment::stretch
        )
            : m_layout_direction(dir)
            , m_spacing(spacing)
            , m_h_align(h_align)
            , m_v_align(v_align)
        {}

        /**
         * @brief Get layout direction
         * @return Direction (vertical or horizontal)
         */
        direction layout_direction() const noexcept { return m_layout_direction; }

        /**
         * @brief Get spacing between children
         * @return Spacing in pixels
         */
        int spacing() const noexcept { return m_spacing; }

        /**
         * @brief Get horizontal alignment
         * @return Horizontal alignment
         */
        horizontal_alignment h_align() const noexcept { return m_h_align; }

        /**
         * @brief Get vertical alignment
         * @return Vertical alignment
         */
        vertical_alignment v_align() const noexcept { return m_v_align; }

    private:
        // Immutable configuration
        const direction m_layout_direction;
        const int m_spacing;
        const horizontal_alignment m_h_align;
        const vertical_alignment m_v_align;

        // Private override methods (Strategy pattern)
        TSize measure_children(elt_t* parent,
                               int available_width,
                               int available_height) override;

        void arrange_children(elt_t* parent,
                              const TRect& content_area) override;

        void arrange_vertical(std::vector <elt_t*>& children,
                              const TRect& content_area);

        void arrange_horizontal(std::vector <elt_t*>& children,
                                const TRect& content_area);
    };

    // ==========================================================================================
    // Implementation
    // ==========================================================================================
    template<RectLike TRect, SizeLike TSize>
    TSize linear_layout <TRect, TSize>::measure_children(elt_t* parent, int available_width, int available_height) {
        const auto& children = parent->children();
        if (children.empty()) {
            TSize result = {};
            size_utils::set_size(result, 0, 0);
            return result;
        }

        int total_width = 0;
        int total_height = 0;
        int total_spacing = m_spacing * (children.size() - 1);

        if (m_layout_direction == direction::vertical) {
            // Vertical layout: stack children vertically
            for (auto& child : children) {
                if (!child->is_visible()) continue;

                // Measure child with full width, remaining height
                int remaining_height = available_height - total_height;
                TSize child_size = child->measure(available_width, remaining_height);

                int child_w = size_utils::get_width(child_size);
                int child_h = size_utils::get_height(child_size);

                total_width = std::max(total_width, child_w);
                total_height += child_h;
            }

            total_height += total_spacing;
        } else {
            // Horizontal layout: stack children horizontally
            for (auto& child : children) {
                if (!child->is_visible()) continue;

                // Measure child with remaining width, full height
                int remaining_width = available_width - total_width;
                TSize child_size = child->measure(remaining_width, available_height);

                int child_w = size_utils::get_width(child_size);
                int child_h = size_utils::get_height(child_size);

                total_width += child_w;
                total_height = std::max(total_height, child_h);
            }

            total_width += total_spacing;
        }

        TSize result = {};
        size_utils::set_size(result, total_width, total_height);
        return result;
    }

    // ------------------------------------------------------------------------------------------------------
    template<RectLike TRect, SizeLike TSize>
    void linear_layout <TRect, TSize>::arrange_children(elt_t* parent, const TRect& content_area) {
        const auto& children = parent->children();
        if (children.empty()) return;

        // Filter visible children
        std::vector <ui_element <TRect, TSize>*> visible_children;
        for (auto& child : children) {
            if (child->is_visible()) {
                visible_children.push_back(child.get());
            }
        }

        if (visible_children.empty()) return;

        if (m_layout_direction == direction::vertical) {
            arrange_vertical(visible_children, content_area);
        } else {
            arrange_horizontal(visible_children, content_area);
        }
    }

    // ---------------------------------------------------------------------------------------
    template<RectLike TRect, SizeLike TSize>
    void linear_layout <TRect, TSize>::arrange_vertical(std::vector <elt_t*>& children, const TRect& content_area) {
        int content_h = rect_utils::get_height(content_area);

        // Calculate total desired height and identify expanding children
        int total_fixed_height = 0;
        float total_weight = 0.0f;
        int num_expanding = 0;

        for (auto* child : children) {
            const TSize& measured = child->last_measured_size();
            int meas_h = size_utils::get_height(measured);

            if (child->h_constraint().policy == size_policy::expand) {
                num_expanding++;
            } else if (child->h_constraint().policy == size_policy::weighted) {
                total_weight += child->h_constraint().weight;
            } else {
                total_fixed_height += meas_h;
            }
        }

        // Calculate spacing
        int total_spacing = m_spacing * (children.size() - 1);
        int available_height = content_h - total_fixed_height - total_spacing;

        // Distribute remaining space
        int expand_height = (num_expanding > 0 && available_height > 0)
                                ? available_height / num_expanding
                                : 0;

        // Position children
        int current_y = rect_utils::get_y(content_area);
        int content_x = rect_utils::get_x(content_area);
        int content_w = rect_utils::get_width(content_area);

        for (auto* child : children) {
            const TSize& measured = child->last_measured_size();
            int meas_w = size_utils::get_width(measured);
            int meas_h = size_utils::get_height(measured);

            // Determine child height
            int child_height = meas_h;
            if (child->h_constraint().policy == size_policy::expand) {
                child_height = expand_height;
            } else if (child->h_constraint().policy == size_policy::weighted) {
                if (total_weight > 0.0f) {
                    child_height = (int)(available_height * (child->h_constraint().weight / total_weight));
                }
            } else if (child->h_constraint().policy == size_policy::fill_parent) {
                child_height = content_h;
            }

            // Apply height constraints
            child_height = child->h_constraint().clamp(child_height);

            // Determine child width based on horizontal alignment
            int child_width = meas_w;
            int child_x = content_x;

            if (child->h_align() == horizontal_alignment::stretch) {
                child_width = content_w;
            } else {
                child_width = std::min(meas_w, content_w);

                if (child->h_align() == horizontal_alignment::center) {
                    child_x = content_x + (content_w - child_width) / 2;
                } else if (child->h_align() == horizontal_alignment::right) {
                    child_x = content_x + content_w - child_width;
                }
            }

            // Arrange child
            TRect child_bounds;
            rect_utils::set_bounds(child_bounds, child_x, current_y,
                                   child_width, child_height);
            child->arrange(child_bounds);

            current_y += child_height + m_spacing;
        }
    }

    // ----------------------------------------------------------------------------------------
    template<RectLike TRect, SizeLike TSize>
    void linear_layout <TRect, TSize>::arrange_horizontal(std::vector <elt_t*>& children, const TRect& content_area) {
        int content_w = rect_utils::get_width(content_area);

        // Calculate total desired width and identify expanding children
        int total_fixed_width = 0;
        float total_weight = 0.0f;
        int num_expanding = 0;

        for (auto* child : children) {
            const TSize& measured = child->last_measured_size();
            int meas_w = size_utils::get_width(measured);

            if (child->w_constraint().policy == size_policy::expand) {
                num_expanding++;
            } else if (child->w_constraint().policy == size_policy::weighted) {
                total_weight += child->w_constraint().weight;
            } else {
                total_fixed_width += meas_w;
            }
        }

        // Calculate spacing
        int total_spacing = m_spacing * (children.size() - 1);
        int available_width = content_w - total_fixed_width - total_spacing;

        // Distribute remaining space
        int expand_width = (num_expanding > 0 && available_width > 0)
                               ? available_width / num_expanding
                               : 0;

        // Position children
        int current_x = rect_utils::get_x(content_area);
        int content_y = rect_utils::get_y(content_area);
        int content_h = rect_utils::get_height(content_area);

        for (auto* child : children) {
            const TSize& measured = child->last_measured_size();
            int meas_w = size_utils::get_width(measured);
            int meas_h = size_utils::get_height(measured);

            // Determine child width
            int child_width = meas_w;
            if (child->w_constraint().policy == size_policy::expand) {
                child_width = expand_width;
            } else if (child->w_constraint().policy == size_policy::weighted) {
                if (total_weight > 0.0f) {
                    child_width = (int)(available_width * (child->w_constraint().weight / total_weight));
                }
            } else if (child->w_constraint().policy == size_policy::fill_parent) {
                child_width = content_w;
            }

            // Apply width constraints
            child_width = child->w_constraint().clamp(child_width);

            // Determine child height based on vertical alignment
            int child_height = meas_h;
            int child_y = content_y;

            if (child->v_align() == vertical_alignment::stretch) {
                child_height = content_h;
            } else {
                child_height = std::min(meas_h, content_h);

                if (child->v_align() == vertical_alignment::center) {
                    child_y = content_y + (content_h - child_height) / 2;
                } else if (child->v_align() == vertical_alignment::bottom) {
                    child_y = content_y + content_h - child_height;
                }
            }

            // Arrange child
            TRect child_bounds;
            rect_utils::set_bounds(child_bounds, current_x, child_y,
                                   child_width, child_height);
            child->arrange(child_bounds);

            current_x += child_width + m_spacing;
        }
    }
}
