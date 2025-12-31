/**
 * @file geometry.hh
 * @brief Pixel-based geometric types for SDL++ backend
 *
 * Note: lib_sdlpp provides its own geometry types (sdlpp::point, sdlpp::rect,
 * sdlpp::size) that can be used directly. However, we define OnyxUI-specific
 * types to ensure concept compliance and maintain consistency with other
 * backends.
 *
 * lib_sdlpp's concept-based API accepts any types satisfying point_like,
 * rect_like, etc., so these types work seamlessly with lib_sdlpp functions.
 */

#pragma once

#include <cstdint>

namespace onyxui::sdlpp {

/**
 * @struct rect
 * @brief Rectangle with pixel coordinates
 *
 * Satisfies both OnyxUI RectLike and lib_sdlpp rect_like concepts.
 */
struct rect {
    int x = 0;      ///< X coordinate (pixels from left)
    int y = 0;      ///< Y coordinate (pixels from top)
    int w = 0;      ///< Width in pixels
    int h = 0;      ///< Height in pixels

    constexpr bool operator==(const rect&) const noexcept = default;
};

/**
 * @struct size
 * @brief Size with pixel dimensions
 *
 * Satisfies both OnyxUI SizeLike and lib_sdlpp size_like concepts.
 */
struct size {
    int w = 0;      ///< Width in pixels
    int h = 0;      ///< Height in pixels

    constexpr bool operator==(const size&) const noexcept = default;
};

/**
 * @struct point
 * @brief 2D point with pixel coordinates
 *
 * Satisfies both OnyxUI PointLike and lib_sdlpp point_like concepts.
 */
struct point {
    int x = 0;      ///< X coordinate
    int y = 0;      ///< Y coordinate

    constexpr bool operator==(const point&) const noexcept = default;
};

} // namespace onyxui::sdlpp
