/**
 * @file transform_2d.hh
 * @brief 2D coordinate transformation for UI rendering
 * @author OnyxUI Framework
 * @date 2024-11
 */

#pragma once

#include <onyxui/concepts/point_like.hh>
#include <onyxui/concepts/rect_like.hh>

namespace onyxui {

    /**
     * @struct transform_2d
     * @brief Simple 2D transformation for coordinate translation
     *
     * @details
     * Provides lightweight 2D transformation support for the rendering pipeline.
     * Currently supports translation only, but designed for future extension
     * to include rotation and scaling if needed.
     *
     * ## Usage Example
     *
     * ```cpp
     * // Create identity transform
     * auto transform = transform_2d::identity();
     *
     * // Apply translation
     * auto child_transform = transform.translate(10, 20);
     *
     * // Transform a point
     * point_type p{5, 5};
     * auto transformed = child_transform.apply(p);  // {15, 25}
     *
     * // Chain transformations
     * auto final_transform = transform
     *     .translate(10, 0)   // Move right
     *     .translate(0, 20);  // Then move down
     * ```
     *
     * ## Design Notes
     *
     * - Kept simple and lightweight for performance
     * - Easily copyable (just two integers)
     * - Immutable operations (translate returns new transform)
     * - Ready for extension with scale/rotation if needed
     */
    struct transform_2d {
        /// Horizontal translation offset
        int dx = 0;

        /// Vertical translation offset
        int dy = 0;

        /**
         * @brief Create identity transformation (no change)
         * @return Transform that doesn't modify coordinates
         */
        [[nodiscard]] static constexpr transform_2d identity() noexcept {
            return {0, 0};
        }

        /**
         * @brief Apply transformation to a point
         * @tparam PointType Type satisfying PointLike concept
         * @param p Point to transform
         * @return Transformed point
         */
        template<PointLike PointType>
        [[nodiscard]] constexpr PointType apply(const PointType& p) const noexcept {
            // Use aggregate initialization to create transformed point
            return PointType{
                point_utils::get_x(p) + dx,
                point_utils::get_y(p) + dy
            };
        }

        /**
         * @brief Apply transformation to a rectangle
         * @tparam RectType Type satisfying RectLike concept
         * @param r Rectangle to transform
         * @return Transformed rectangle (position changed, size unchanged)
         */
        template<RectLike RectType>
        [[nodiscard]] constexpr RectType apply(const RectType& r) const noexcept {
            // Use aggregate initialization to create transformed rectangle
            return RectType{
                rect_utils::get_x(r) + dx,
                rect_utils::get_y(r) + dy,
                rect_utils::get_width(r),
                rect_utils::get_height(r)
            };
        }

        /**
         * @brief Create new transform with additional translation
         * @param x Additional horizontal offset
         * @param y Additional vertical offset
         * @return New transform with combined translation
         *
         * @details
         * Creates a new transform that combines this transform's
         * translation with the additional offset. Original transform
         * is unchanged (immutable operation).
         */
        [[nodiscard]] constexpr transform_2d translate(int x, int y) const noexcept {
            return {dx + x, dy + y};
        }

        /**
         * @brief Create new transform from point offset
         * @tparam PointType Type satisfying PointLike concept
         * @param offset Translation offset as point
         * @return New transform with combined translation
         */
        template<PointLike PointType>
        [[nodiscard]] constexpr transform_2d translate(const PointType& offset) const noexcept {
            return translate(
                point_utils::get_x(offset),
                point_utils::get_y(offset));
        }

        /**
         * @brief Check if this is an identity transform
         * @return true if transform has no effect
         */
        [[nodiscard]] constexpr bool is_identity() const noexcept {
            return dx == 0 && dy == 0;
        }

        /**
         * @brief Invert the transformation
         * @return Transform that undoes this transformation
         */
        [[nodiscard]] constexpr transform_2d inverse() const noexcept {
            return {-dx, -dy};
        }

        /**
         * @brief Compose two transformations
         * @param other Transform to apply after this one
         * @return Combined transformation
         *
         * @details
         * Creates a transform equivalent to applying this transform
         * first, then the other transform. For translations, this
         * is simply the sum of the offsets.
         */
        [[nodiscard]] constexpr transform_2d compose(const transform_2d& other) const noexcept {
            return {dx + other.dx, dy + other.dy};
        }

        /**
         * @brief Equality comparison
         */
        [[nodiscard]] constexpr bool operator==(const transform_2d& other) const noexcept = default;

        /**
         * @brief Inequality comparison
         */
        [[nodiscard]] constexpr bool operator!=(const transform_2d& other) const noexcept = default;

        // Future extension points (commented out for now):
        // float scale_x = 1.0f;  ///< Horizontal scaling factor
        // float scale_y = 1.0f;  ///< Vertical scaling factor
        // float rotation = 0.0f; ///< Rotation angle in radians
        //
        // For full affine transformations, we'd use a 3x3 matrix:
        // | scale_x * cos(r)  -scale_y * sin(r)  dx |
        // | scale_x * sin(r)   scale_y * cos(r)  dy |
        // | 0                  0                  1  |
    };

} // namespace onyxui