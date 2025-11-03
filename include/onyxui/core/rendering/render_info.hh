/**
 * @file render_info.hh
 * @brief Aggregated rendering parameters for clean API
 * @author OnyxUI Framework
 * @date 2024-11
 */

#pragma once

#include <onyxui/core/rendering/transform_2d.hh>
#include <onyxui/core/rendering/resolved_style.hh>
#include <onyxui/theming/theme.hh>
#include <onyxui/concepts/rect_like.hh>
#include <onyxui/concepts/backend.hh>
#include <vector>

namespace onyxui {

    /**
     * @struct render_info
     * @brief Aggregates all rendering parameters into a single structure
     *
     * @tparam Backend The UI backend type
     *
     * @details
     * Simplifies the render method signature by aggregating all parameters
     * into a single, lightweight structure. Uses references for required
     * data and pointers for optional data to clearly communicate the API
     * contract.
     *
     * ## Design Philosophy
     *
     * - **References** for always-required data (theme, parent_style)
     * - **Pointers** for optional data (dirty_regions)
     * - **Values** for small copyable data (transform)
     *
     * ## Usage Example
     *
     * ```cpp
     * // Root element rendering
     * auto info = render_info<Backend>::create_root(dirty_regions, theme);
     * root->render(renderer, info);
     *
     * // Parent-child rendering
     * void panel::render(renderer_type& renderer, const render_info<Backend>& info) {
     *     // Render self...
     *
     *     // Create child info with accumulated transform
     *     auto content = get_content_area();
     *     auto child_style = resolve_style(&info.theme, info.parent_style);
     *     auto child_info = info.for_child(child_style, content);
     *
     *     // Render children
     *     for (auto& child : m_children) {
     *         child->render(renderer, child_info);
     *     }
     * }
     * ```
     *
     * ## Benefits
     *
     * - Clean API (2 params instead of 5+)
     * - Extensible without breaking changes
     * - Type-safe (references prevent null)
     * - Efficient (mostly references, minimal copying)
     * - Self-documenting parameter names
     */
    template<UIBackend Backend>
    struct render_info {
        using theme_type = ui_theme<Backend>;
        using style_type = resolved_style<Backend>;
        using rect_type = typename Backend::rect_type;
        using point_type = typename Backend::point_type;

        // ===== Data Members =====

        /// Dirty regions to optimize rendering (null = render everything)
        const std::vector<rect_type>* dirty_regions;

        /// Theme for visual styling (always required)
        const theme_type& theme;

        /// Parent's resolved style for inheritance (always required)
        const style_type& parent_style;

        /// Cumulative coordinate transformation
        transform_2d transform;

        // ===== Construction =====

        /**
         * @brief Construct render info with all parameters
         * @param dirty Optional dirty regions for optimization
         * @param theme_ref Required theme reference
         * @param style_ref Required parent style reference
         * @param trans Coordinate transformation (defaults to identity)
         */
        render_info(const std::vector<rect_type>* dirty,
                    const theme_type& theme_ref,
                    const style_type& style_ref,
                    transform_2d trans = transform_2d::identity())
            : dirty_regions(dirty)
            , theme(theme_ref)
            , parent_style(style_ref)
            , transform(trans) {}

        /**
         * @brief Factory method for root element rendering
         * @param dirty Dirty regions for optimization
         * @param theme_ref Theme to use for rendering
         * @return render_info configured for root element
         *
         * @details
         * Creates render info for the root element with identity transform
         * and default style resolved from theme.
         */
        [[nodiscard]] static render_info create_root(
            const std::vector<rect_type>& dirty,
            const theme_type& theme_ref) {

            // Create root style from theme
            // Note: In real implementation, this might need to be cached
            // to avoid recreating on every render
            static thread_local style_type root_style =
                style_type::from_theme(theme_ref);

            // Update root style if theme changed
            // (Simple approach - could be optimized with theme versioning)
            root_style = style_type::from_theme(theme_ref);

            return render_info{
                &dirty,
                theme_ref,
                root_style,
                transform_2d::identity()
            };
        }

        /**
         * @brief Factory method for root element without dirty regions
         * @param theme_ref Theme to use for rendering
         * @return render_info configured for full rendering
         */
        [[nodiscard]] static render_info create_root(const theme_type& theme_ref) {
            static thread_local style_type root_style =
                style_type::from_theme(theme_ref);

            root_style = style_type::from_theme(theme_ref);

            return render_info{
                nullptr,  // No dirty regions = render everything
                theme_ref,
                root_style,
                transform_2d::identity()
            };
        }

        // ===== Child Context Creation =====

        /**
         * @brief Create render info for child element
         * @param child_style Resolved style for the child
         * @param content_area Parent's content area for transform
         * @return New render_info with accumulated transformation
         *
         * @details
         * Creates a new render_info for rendering a child element.
         * The transformation is accumulated by translating to the
         * content area's position.
         */
        [[nodiscard]] render_info for_child(
            const style_type& child_style,
            const rect_type& content_area) const {

            return render_info{
                dirty_regions,
                theme,
                child_style,
                transform.translate(
                    rect_utils::get_x(content_area),
                    rect_utils::get_y(content_area))
            };
        }

        /**
         * @brief Create render info for child with custom offset
         * @param child_style Resolved style for the child
         * @param offset_x Horizontal offset
         * @param offset_y Vertical offset
         * @return New render_info with accumulated transformation
         */
        [[nodiscard]] render_info for_child_offset(
            const style_type& child_style,
            int offset_x,
            int offset_y) const {

            return render_info{
                dirty_regions,
                theme,
                child_style,
                transform.translate(offset_x, offset_y)
            };
        }

        // ===== Utility Methods =====

        /**
         * @brief Check if element should be rendered
         * @param bounds Element bounds to check
         * @return true if element intersects with dirty regions
         *
         * @details
         * Optimization helper that checks if an element needs rendering
         * based on dirty region intersection. If no dirty regions are
         * specified (nullptr), always returns true.
         */
        [[nodiscard]] bool should_render(const rect_type& bounds) const {
            // No dirty regions means render everything
            if (!dirty_regions || dirty_regions->empty()) {
                return true;
            }

            // Transform bounds to absolute coordinates
            auto absolute_bounds = transform.apply(bounds);

            // Check intersection with any dirty region
            for (const auto& dirty : *dirty_regions) {
                // Simple AABB intersection test
                if (rect_utils::get_x(absolute_bounds) < rect_utils::get_x(dirty) + rect_utils::get_width(dirty) &&
                    rect_utils::get_x(absolute_bounds) + rect_utils::get_width(absolute_bounds) > rect_utils::get_x(dirty) &&
                    rect_utils::get_y(absolute_bounds) < rect_utils::get_y(dirty) + rect_utils::get_height(dirty) &&
                    rect_utils::get_y(absolute_bounds) + rect_utils::get_height(absolute_bounds) > rect_utils::get_y(dirty)) {
                    return true;
                }
            }

            return false;
        }

        /**
         * @brief Apply transformation to a point
         * @param p Point in local coordinates
         * @return Point in absolute coordinates
         */
        [[nodiscard]] point_type transform_point(const point_type& p) const {
            return transform.apply(p);
        }

        /**
         * @brief Apply transformation to a rectangle
         * @param r Rectangle in local coordinates
         * @return Rectangle in absolute coordinates
         */
        [[nodiscard]] rect_type transform_rect(const rect_type& r) const {
            return transform.apply(r);
        }

        /**
         * @brief Create a copy with modified transformation
         * @param new_transform New transformation to use
         * @return Copy of this render_info with new transform
         */
        [[nodiscard]] render_info with_transform(transform_2d new_transform) const {
            return render_info{
                dirty_regions,
                this->theme,
                this->parent_style,
                new_transform
            };
        }

        /**
         * @brief Create a copy with additional translation
         * @param dx Additional horizontal offset
         * @param dy Additional vertical offset
         * @return Copy with accumulated translation
         */
        [[nodiscard]] render_info with_offset(int dx, int dy) const {
            return render_info{
                dirty_regions,
                theme,
                parent_style,
                transform.translate(dx, dy)
            };
        }
    };

} // namespace onyxui