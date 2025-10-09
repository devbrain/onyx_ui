//
// Created by igor on 08/10/2025.
//

#pragma once

#include <vector>
#include <algorithm>
#include <memory>

#include <onyxui/concepts.hh>
#include <onyxui/layout_strategy.hh>

namespace onyxui {
    // Thickness struct (margin/padding) - this one is custom
    struct thickness {
        int left, top, right, bottom;

        [[nodiscard]] int horizontal() const { return left + right; }
        [[nodiscard]] int vertical() const { return top + bottom; }
    };

    // The UI tree uses unique_ptr for owned children
    // Parent pointers are non-owning raw pointers
    template<RectLike TRect, SizeLike TSize>
    class ui_element {
        public:
            using ui_element_ptr = std::unique_ptr <ui_element>;
            using layout_strategy_ptr = std::unique_ptr <layout_strategy <TRect, TSize>>;

        public:
            explicit ui_element(ui_element* parent);
            virtual ~ui_element() = default;

            // Add child (takes ownership)
            void add_child(ui_element_ptr child);

            // Remove child (returns ownership)
            ui_element_ptr remove_child(ui_element* child);

            void invalidate_measure();
            void invalidate_arrange();

            TSize measure(int available_width, int available_height);
            void arrange(const TRect& final_bounds);

            void sort_children_by_z_index();

            // Call this after adding/removing children or changing z_index
            void update_child_order();

            ui_element* hit_test(int x, int y);

        protected:
            // Override to provide custom measurement
            virtual TSize do_measure(int available_width, int available_height);

            // Override to provide custom arrangement
            virtual void do_arrange(const TRect& final_bounds);

            // Content size (for content-sized elements)
            virtual TSize get_content_size() const {
                TSize s = {};
                size_utils::set_size(s, 0, 0);
                return s;
            }

        private:
            // Non-owning pointer to parent
            ui_element* m_parent = nullptr;

            // Owned children
            std::vector <ui_element_ptr> m_children;

            // Owned layout strategy
            layout_strategy_ptr m_layout_strategy;

            enum class layout_state {
                valid, // Layout is up to date
                dirty, // This element needs layout
                propagated // Already propagated invalidation
            };

            layout_state measure_state = layout_state::dirty; // Start dirty
            layout_state arrange_state = layout_state::dirty;

            // Cached measure results
            mutable TSize m_last_measured_size = {};
            mutable int m_last_available_width = -1;
            mutable int m_last_available_height = -1;

            // Properties
            bool m_visible = true;
            bool m_enabled = true;

            // Geometry - uses your types!
            TRect m_bounds = {};
            int z_index = 0;

            // Size constraints
            size_constraint m_width_constraint;
            size_constraint m_height_constraint;

            // Alignment within allocated space
            horizontal_alignment h_align = horizontal_alignment::stretch;
            vertical_alignment v_align = vertical_alignment::stretch;

            // Spacing
            thickness margin = {0, 0, 0, 0};
            thickness padding = {0, 0, 0, 0};
    };

    template<RectLike TRect, SizeLike TSize>
    ui_element <TRect, TSize>::ui_element(ui_element* parent)
        : m_parent(parent) {
    }

    // ===============================================================================
    // Implementation
    // ===============================================================================
    template<RectLike TRect, SizeLike TSize>
    void ui_element <TRect, TSize>::add_child(ui_element_ptr child) {
        child->m_parent = this;
        m_children.push_back(std::move(child));
        invalidate_measure();
    }
    // ------------------------------------------------------------------------------
    template<RectLike TRect, SizeLike TSize>
    ui_element <TRect, TSize>::ui_element_ptr ui_element <TRect, TSize>::remove_child(ui_element* child) {
        auto it = std::find_if(m_children.begin(), m_children.end(),
                               [child](const ui_element_ptr& ptr) {
                                   return ptr.get() == child;
                               });

        if (it != m_children.end()) {
            ui_element_ptr removed = std::move(*it);
            m_children.erase(it);
            removed->m_parent = nullptr;
            invalidate_measure();
            return removed;
        }
        return nullptr;
    }
    // -----------------------------------------------------------------------------------------------------
    template<RectLike TRect, SizeLike TSize>
    void ui_element <TRect, TSize>::invalidate_measure() {
        // If already dirty or propagated, nothing to do
        if (measure_state != layout_state::valid) {
            return;
        }

        measure_state = layout_state::dirty;
        arrange_state = layout_state::dirty;

        // Propagate up
        if (m_parent) {
            m_parent->invalidate_measure();
        }
    }
    // -----------------------------------------------------------------------------------------------------
    template<RectLike TRect, SizeLike TSize>
    void ui_element <TRect, TSize>::invalidate_arrange() {
        if (arrange_state != layout_state::valid) {
            return;
        }

        arrange_state = layout_state::dirty;

        // Propagate down
        for (auto& child : m_children) {
            child->invalidate_arrange();
        }
    }
    // -----------------------------------------------------------------------------------------------------
    template<RectLike TRect, SizeLike TSize>
    TSize ui_element <TRect, TSize>::measure(int available_width, int available_height) {
        // Check cache
        // Check cache
        if (measure_state == layout_state::valid &&
            m_last_available_width == available_width &&
            m_last_available_height == available_height) {
            return m_last_measured_size;
        }

        // Account for margin
        int content_width = available_width - margin.horizontal();
        int content_height = available_height - margin.vertical();

        // Measure content
        TSize measured = do_measure(content_width, content_height);

        // Add margin back
        int meas_w = size_utils::get_width(measured) + margin.horizontal();
        int meas_h = size_utils::get_height(measured) + margin.vertical();

        // Apply constraints
        meas_w = m_width_constraint.clamp(meas_w);
        meas_h = m_height_constraint.clamp(meas_h);

        size_utils::set_size(measured, meas_w, meas_h);

        // Cache results
        m_last_measured_size = measured;
        m_last_available_width = available_width;
        m_last_available_height = available_height;

        measure_state = layout_state::valid;
        return m_last_measured_size;
    }
    // -----------------------------------------------------------------------------------------------------
    template<RectLike TRect, SizeLike TSize>
    void ui_element <TRect, TSize>::arrange(const TRect& final_bounds) {
        m_bounds = final_bounds;

        if (arrange_state == layout_state::valid &&
            measure_state == layout_state::valid) {
            return;
        }

        // Calculate content area (exclude margin and padding)
        TRect content_area;
        int x = rect_utils::get_x(final_bounds) + margin.left + padding.left;
        int y = rect_utils::get_y(final_bounds) + margin.top + padding.top;
        int w = rect_utils::get_width(final_bounds) - margin.horizontal() - padding.horizontal();
        int h = rect_utils::get_height(final_bounds) - margin.vertical() - padding.vertical();
        rect_utils::set_bounds(content_area, x, y, w, h);

        do_arrange(content_area);
        arrange_state = layout_state::valid;
    }

    // -----------------------------------------------------------------------------------------------------
    template<RectLike TRect, SizeLike TSize>
    void ui_element<TRect, TSize>::sort_children_by_z_index() {
        std::stable_sort(m_children.begin(), m_children.end(),
                         [](const ui_element_ptr& a, const ui_element_ptr& b) {
                             return a->z_index < b->z_index;
                         });
    }

    // -----------------------------------------------------------------------------------------------------
    template<RectLike TRect, SizeLike TSize>
    void ui_element<TRect, TSize>::update_child_order() {
        sort_children_by_z_index();
        invalidate_arrange();
    }

    // -----------------------------------------------------------------------------------------------------
    template<RectLike TRect, SizeLike TSize>
    ui_element<TRect, TSize>* ui_element<TRect, TSize>::hit_test(int x, int y) {
        // Not visible or not within bounds
        if (!m_visible || !m_bounds.contains(x, y)) {
            return nullptr;
        }

        // Check children in reverse order (highest z-index first)
        for (auto it = m_children.rbegin(); it != m_children.rend(); ++it) {
            ui_element* hit = (*it)->hit_test(x, y);
            if (hit) return hit;
        }

        // No child hit, return this element
        return this;
    }

    // -----------------------------------------------------------------------------------------------------
    template<RectLike TRect, SizeLike TSize>
    TSize ui_element <TRect, TSize>::do_measure(int available_width, int available_height) {
        // Default implementation: measure using layout strategy
        if (m_layout_strategy) {
            return m_layout_strategy->measure_children(this, available_width, available_height);
        }

        // Fallback: use content size
        return get_content_size();
    }
    // -----------------------------------------------------------------------------------------------------
    template<RectLike TRect, SizeLike TSize>
    void ui_element <TRect, TSize>::do_arrange(const TRect& final_bounds) {
        // Default implementation: arrange using layout strategy
        if (m_layout_strategy) {
            m_layout_strategy->arrange_children(this, final_bounds);
        }
    }
}
