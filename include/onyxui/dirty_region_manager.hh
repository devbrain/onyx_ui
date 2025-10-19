/**
 * @file dirty_region_manager.hh
 * @brief Generic dirty rectangle tracking for efficient rendering
 * @author Assistant
 * @date 2024
 *
 * @details
 * Tracks regions of the screen that need to be redrawn, merging overlapping
 * rectangles for efficiency. This is a backend-agnostic implementation that
 * works with any rectangle type satisfying the RectLike concept.
 *
 * ## Algorithm
 *
 * The manager tracks "dirty" regions that need redrawing. When multiple
 * regions overlap, they're merged into larger regions to minimize the
 * number of clear/redraw operations.
 *
 * ## Usage
 *
 * ```cpp
 * dirty_region_manager<SDL_Rect> dirty_mgr;
 *
 * // Mark regions as dirty when things change
 * dirty_mgr.mark_dirty({10, 10, 50, 50});
 * dirty_mgr.mark_dirty({40, 40, 30, 30});  // Overlaps, will be merged
 *
 * // Get merged regions for rendering
 * auto regions = dirty_mgr.get_merged_regions();
 * for (auto& region : regions) {
 *     renderer.clear_region(region);
 *     render_widgets_in_region(region);
 * }
 *
 * dirty_mgr.clear();  // Reset for next frame
 * ```
 */

#pragma once

#include <vector>
#include <algorithm>
#include <onyxui/concepts/rect_like.hh>

namespace onyxui {

    /**
     * @class dirty_region_manager
     * @brief Tracks and merges dirty regions for efficient rendering
     *
     * @tparam Rect Rectangle type (must satisfy RectLike concept)
     */
    template<RectLike Rect>
    class dirty_region_manager {
    public:
        dirty_region_manager() = default;

        /**
         * @brief Mark a region as dirty (needs redraw)
         * @param region The rectangular region that needs redrawing
         *
         * @details
         * The region will be merged with existing dirty regions if they
         * overlap or are adjacent.
         */
        void mark_dirty(const Rect& region) {
            // Skip empty rectangles
            if (rect_utils::get_width(region) <= 0 ||
                rect_utils::get_height(region) <= 0) {
                return;
            }

            // If we have no regions yet, just add it
            if (m_dirty_regions.empty()) {
                m_dirty_regions.push_back(region);
                return;
            }

            // Try to merge with existing regions
            bool merged = false;
            for (auto& existing : m_dirty_regions) {
                if (should_merge(existing, region)) {
                    existing = merge_rects(existing, region);
                    merged = true;
                    break;
                }
            }

            // If couldn't merge, add as new region
            if (!merged) {
                m_dirty_regions.push_back(region);
            }

            // Periodically consolidate regions to prevent fragmentation
            if (m_dirty_regions.size() > 10) {
                consolidate();
            }
        }

        /**
         * @brief Mark entire screen as dirty
         * @param viewport The full viewport rectangle
         */
        void mark_all_dirty(const Rect& viewport) {
            m_dirty_regions.clear();
            m_dirty_regions.push_back(viewport);
        }

        /**
         * @brief Get the list of dirty regions (after merging)
         * @return Vector of rectangles that need redrawing
         */
        [[nodiscard]] std::vector<Rect> get_merged_regions() const {
            if (m_dirty_regions.empty()) {
                return {};
            }

            // Make a copy to merge
            std::vector<Rect> result = m_dirty_regions;

            // Keep merging until no more merges possible
            bool changed = true;
            while (changed) {
                changed = false;
                for (size_t i = 0; i < result.size(); ++i) {
                    for (size_t j = i + 1; j < result.size(); ++j) {
                        if (should_merge(result[i], result[j])) {
                            result[i] = merge_rects(result[i], result[j]);
                            result.erase(result.begin() + j);
                            changed = true;
                            break;
                        }
                    }
                    if (changed) break;
                }
            }

            return result;
        }

        /**
         * @brief Check if there are any dirty regions
         */
        [[nodiscard]] bool has_dirty_regions() const noexcept {
            return !m_dirty_regions.empty();
        }

        /**
         * @brief Clear all dirty regions (call after rendering)
         */
        void clear() noexcept {
            m_dirty_regions.clear();
        }

        /**
         * @brief Get the bounding box of all dirty regions
         * @return Rectangle containing all dirty regions
         */
        [[nodiscard]] Rect get_bounding_box() const {
            if (m_dirty_regions.empty()) {
                Rect empty{};
                rect_utils::set_bounds(empty, 0, 0, 0, 0);
                return empty;
            }

            auto bbox = m_dirty_regions[0];
            for (size_t i = 1; i < m_dirty_regions.size(); ++i) {
                bbox = merge_rects(bbox, m_dirty_regions[i]);
            }
            return bbox;
        }

    private:
        std::vector<Rect> m_dirty_regions;

        /**
         * @brief Check if two rectangles should be merged
         *
         * @details
         * Rectangles are merged if they overlap or are adjacent
         * (within a small threshold to avoid fragmentation).
         */
        [[nodiscard]] static bool should_merge(const Rect& a, const Rect& b) {
            // Check if rectangles overlap or are adjacent
            constexpr int threshold = 2;  // Merge if within 2 pixels

            int a_left = rect_utils::get_x(a);
            int a_top = rect_utils::get_y(a);
            int a_right = a_left + rect_utils::get_width(a);
            int a_bottom = a_top + rect_utils::get_height(a);

            int b_left = rect_utils::get_x(b);
            int b_top = rect_utils::get_y(b);
            int b_right = b_left + rect_utils::get_width(b);
            int b_bottom = b_top + rect_utils::get_height(b);

            // Check if they overlap or are adjacent
            return !(a_right + threshold < b_left ||
                    b_right + threshold < a_left ||
                    a_bottom + threshold < b_top ||
                    b_bottom + threshold < a_top);
        }

        /**
         * @brief Merge two rectangles into their bounding box
         */
        [[nodiscard]] static Rect merge_rects(const Rect& a, const Rect& b) {
            int a_left = rect_utils::get_x(a);
            int a_top = rect_utils::get_y(a);
            int a_right = a_left + rect_utils::get_width(a);
            int a_bottom = a_top + rect_utils::get_height(a);

            int b_left = rect_utils::get_x(b);
            int b_top = rect_utils::get_y(b);
            int b_right = b_left + rect_utils::get_width(b);
            int b_bottom = b_top + rect_utils::get_height(b);

            int left = std::min(a_left, b_left);
            int top = std::min(a_top, b_top);
            int right = std::max(a_right, b_right);
            int bottom = std::max(a_bottom, b_bottom);

            Rect result;
            rect_utils::set_bounds(result, left, top, right - left, bottom - top);
            return result;
        }

        /**
         * @brief Consolidate regions by merging all mergeable pairs
         */
        void consolidate() {
            auto merged = get_merged_regions();
            m_dirty_regions = std::move(merged);
        }
    };

} // namespace onyxui