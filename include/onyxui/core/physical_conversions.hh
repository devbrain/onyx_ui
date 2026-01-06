/**
 * @file physical_conversions.hh
 * @brief Conversion functions between physical types and backend types
 * @author OnyxUI Framework
 * @date 2026-01
 *
 * @details
 * These functions bridge the gap between strong physical types (physical_rect, etc.)
 * and backend-specific types (e.g., conio::rect, sdlpp::rect). Backend types remain
 * unchanged as external contracts; physical types provide compile-time safety.
 */

#pragma once

#include <onyxui/core/physical_types.hh>
#include <onyxui/concepts/backend.hh>
#include <onyxui/concepts/rect_like.hh>
#include <onyxui/concepts/size_like.hh>
#include <onyxui/concepts/point_like.hh>

namespace onyxui {

// ============================================================================
// Physical types -> Backend types
// ============================================================================

/**
 * @brief Convert physical_rect to backend rect type
 *
 * Creates a backend-specific rectangle from strong physical types.
 * Uses rect_utils::set_bounds for backend compatibility.
 *
 * @tparam Backend The UI backend type
 * @param rect Physical rectangle to convert
 * @return Backend-specific rectangle type
 */
template<UIBackend Backend>
[[nodiscard]] constexpr typename Backend::rect_type
to_backend_rect(const physical_rect& rect) noexcept {
    typename Backend::rect_type result{};
    rect_utils::set_bounds(result,
                          rect.x.value, rect.y.value,
                          rect.width.value, rect.height.value);
    return result;
}

/**
 * @brief Convert physical_size to backend size type
 *
 * @tparam Backend The UI backend type
 * @param size Physical size to convert
 * @return Backend-specific size type
 */
template<UIBackend Backend>
[[nodiscard]] constexpr typename Backend::size_type
to_backend_size(const physical_size& size) noexcept {
    typename Backend::size_type result{};
    size_utils::set_size(result, size.width.value, size.height.value);
    return result;
}

/**
 * @brief Convert physical_point to backend point type
 *
 * @tparam Backend The UI backend type
 * @param point Physical point to convert
 * @return Backend-specific point type
 */
template<UIBackend Backend>
[[nodiscard]] constexpr typename Backend::point_type
to_backend_point(const physical_point& point) noexcept {
    return point_utils::make<typename Backend::point_type>(point.x.value, point.y.value);
}

// ============================================================================
// Backend types -> Physical types
// ============================================================================

/**
 * @brief Convert backend rect type to physical_rect
 *
 * @tparam Backend The UI backend type
 * @param rect Backend-specific rectangle
 * @return Physical rectangle with strong types
 */
template<UIBackend Backend>
[[nodiscard]] constexpr physical_rect
from_backend_rect(const typename Backend::rect_type& rect) noexcept {
    return {
        physical_x(rect_utils::get_x(rect)),
        physical_y(rect_utils::get_y(rect)),
        physical_x(rect_utils::get_width(rect)),
        physical_y(rect_utils::get_height(rect))
    };
}

/**
 * @brief Convert backend size type to physical_size
 *
 * @tparam Backend The UI backend type
 * @param size Backend-specific size
 * @return Physical size with strong types
 */
template<UIBackend Backend>
[[nodiscard]] constexpr physical_size
from_backend_size(const typename Backend::size_type& size) noexcept {
    return {
        physical_x(size_utils::get_width(size)),
        physical_y(size_utils::get_height(size))
    };
}

/**
 * @brief Convert backend point type to physical_point
 *
 * @tparam Backend The UI backend type
 * @param point Backend-specific point
 * @return Physical point with strong types
 */
template<UIBackend Backend>
[[nodiscard]] constexpr physical_point
from_backend_point(const typename Backend::point_type& point) noexcept {
    return {
        physical_x(point_utils::get_x(point)),
        physical_y(point_utils::get_y(point))
    };
}

// ============================================================================
// Convenience overloads for generic RectLike/SizeLike/PointLike types
// ============================================================================

/**
 * @brief Convert any RectLike type to physical_rect
 */
template<RectLike R>
[[nodiscard]] constexpr physical_rect
to_physical_rect(const R& rect) noexcept {
    return {
        physical_x(rect_utils::get_x(rect)),
        physical_y(rect_utils::get_y(rect)),
        physical_x(rect_utils::get_width(rect)),
        physical_y(rect_utils::get_height(rect))
    };
}

/**
 * @brief Convert any SizeLike type to physical_size
 */
template<SizeLike S>
[[nodiscard]] constexpr physical_size
to_physical_size(const S& size) noexcept {
    return {
        physical_x(size_utils::get_width(size)),
        physical_y(size_utils::get_height(size))
    };
}

/**
 * @brief Convert any PointLike type to physical_point
 */
template<PointLike P>
[[nodiscard]] constexpr physical_point
to_physical_point(const P& point) noexcept {
    return {
        physical_x(point_utils::get_x(point)),
        physical_y(point_utils::get_y(point))
    };
}

} // namespace onyxui
