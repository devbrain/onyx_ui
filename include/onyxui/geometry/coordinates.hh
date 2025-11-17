/**
 * @file coordinates.hh
 * @brief Strong types for coordinate systems (absolute vs relative)
 * @author OnyxUI Architecture Team
 * @date 2025-11-11
 *
 * @details
 * Provides compile-time safety for coordinate systems by wrapping backend types
 * with phantom type tags. Prevents accidental mixing of absolute (screen-space)
 * and relative (widget-local) coordinates.
 *
 * ## Key Features
 * - **Zero-cost abstraction**: Wrappers optimize away at compile time
 * - **Backend integration**: Wraps Backend::rect_type and Backend::point_type
 * - **Type safety**: Deleted constructors prevent implicit conversions
 * - **Explicit conversions**: to_absolute() and to_relative() functions
 *
 * ## Usage Example
 * @code
 * // Get relative bounds (widget-local coordinates)
 * relative_rect<Backend> rel = widget->bounds();
 *
 * // Convert to absolute (screen coordinates)
 * absolute_rect<Backend> abs = to_absolute(rel, widget);
 *
 * // Cannot mix coordinate systems (compiler error!)
 * // if (abs == rel) { }  // ERROR: types don't match!
 * @endcode
 *
 * @see docs/STRONG_COORDINATE_TYPES_DESIGN.md for full design document
 */

#pragma once

#include <onyxui/concepts/backend.hh>
#include <onyxui/concepts/rect_like.hh>
#include <onyxui/concepts/point_like.hh>

namespace onyxui {

// Forward declarations
template<UIBackend Backend>
class ui_element;

namespace geometry {

// ====================
// PHANTOM TYPE TAGS
// ====================

/**
 * @brief Tag type for absolute (screen-space) coordinates
 * @details Used as phantom type parameter to distinguish coordinate systems
 */
struct absolute_tag {};

/**
 * @brief Tag type for relative (widget-local) coordinates
 * @details Used as phantom type parameter to distinguish coordinate systems
 */
struct relative_tag {};

// ====================
// STRONG TYPE WRAPPERS
// ====================

/**
 * @brief Strong type wrapper for points (wraps Backend::point_type)
 * @tparam Backend The UI backend providing point_type
 * @tparam CoordTag Phantom type tag (absolute_tag or relative_tag)
 *
 * @details
 * Provides compile-time coordinate system safety by wrapping backend-specific
 * point types with phantom type tags. No implicit conversions between coordinate
 * systems are allowed.
 *
 * @example
 * @code
 * // Create absolute point
 * Backend::point_type backend_pt{10, 20};
 * absolute_point<Backend> abs_pt{backend_pt};
 *
 * // Cannot mix coordinate systems
 * relative_point<Backend> rel_pt{backend_pt};
 * // abs_pt = rel_pt;  // Compiler error! ✅
 * @endcode
 */
template<UIBackend Backend, typename CoordTag>
struct strong_point {
    using underlying_type = typename Backend::point_type;
    underlying_type value;

    // Explicit construction only (no implicit conversions)
    explicit constexpr strong_point(underlying_type v) : value(v) {}

    // Access underlying backend value
    [[nodiscard]] constexpr const underlying_type& get() const { return value; }
    [[nodiscard]] constexpr underlying_type& get() { return value; }

    // No implicit conversions between coordinate systems!
    template<typename OtherTag>
    strong_point(const strong_point<Backend, OtherTag>&) = delete;

    // Equality comparison (same coordinate system only)
    constexpr bool operator==(const strong_point&) const = default;

    // Helper accessors using point_utils
    [[nodiscard]] int x() const { return point_utils::get_x(value); }
    [[nodiscard]] int y() const { return point_utils::get_y(value); }
};

/**
 * @brief Strong type wrapper for rectangles (wraps Backend::rect_type)
 * @tparam Backend The UI backend providing rect_type
 * @tparam CoordTag Phantom type tag (absolute_tag or relative_tag)
 *
 * @details
 * Provides compile-time coordinate system safety by wrapping backend-specific
 * rect types with phantom type tags. Use rect_utils for backend-agnostic
 * manipulation of the underlying rect.
 */
template<UIBackend Backend, typename CoordTag>
struct strong_rect {
    using underlying_type = typename Backend::rect_type;
    underlying_type value;

    explicit constexpr strong_rect(underlying_type v) : value(v) {}

    [[nodiscard]] constexpr const underlying_type& get() const { return value; }
    [[nodiscard]] constexpr underlying_type& get() { return value; }

    // No implicit conversions between coordinate systems!
    template<typename OtherTag>
    strong_rect(const strong_rect<Backend, OtherTag>&) = delete;

    constexpr bool operator==(const strong_rect&) const = default;

    // Helper: Check if point is within rect (uses rect_utils)
    [[nodiscard]] bool contains(const strong_point<Backend, CoordTag>& pt) const {
        return rect_utils::contains(value, pt.x(), pt.y());
    }

    // Helper: Get dimensions using rect_utils
    [[nodiscard]] int width() const { return rect_utils::get_width(value); }
    [[nodiscard]] int height() const { return rect_utils::get_height(value); }
    [[nodiscard]] int x() const { return rect_utils::get_x(value); }
    [[nodiscard]] int y() const { return rect_utils::get_y(value); }
};

// ====================
// TYPE ALIASES
// ====================

/**
 * @brief Point in absolute screen coordinates
 * @details Origin at root element's top-left (0, 0)
 */
template<UIBackend Backend>
using absolute_point = strong_point<Backend, absolute_tag>;

/**
 * @brief Point in relative coordinates (parent's content area)
 * @details Origin at parent's content area top-left (0, 0)
 */
template<UIBackend Backend>
using relative_point = strong_point<Backend, relative_tag>;

/**
 * @brief Rectangle in absolute screen coordinates
 */
template<UIBackend Backend>
using absolute_rect = strong_rect<Backend, absolute_tag>;

/**
 * @brief Rectangle in relative coordinates
 */
template<UIBackend Backend>
using relative_rect = strong_rect<Backend, relative_tag>;

// ====================
// COORDINATE CONVERSIONS
// ====================

/**
 * @brief Convert relative rect to absolute screen coordinates
 * @param rel Rectangle in element's local coordinates
 * @param element Element that owns the coordinate space
 * @return Rectangle in screen coordinates
 *
 * @details
 * Walks parent chain to accumulate offsets, using rect_utils for
 * backend-agnostic rect manipulation. This is the primary conversion
 * function used throughout the framework.
 *
 * @example
 * @code
 * // Child at (10, 5) relative to parent at (100, 50)
 * relative_rect<Backend> child_rel = child->bounds();
 * absolute_rect<Backend> child_abs = to_absolute(child_rel, child);
 * // child_abs is at (110, 55) in screen coordinates
 * @endcode
 */
template<UIBackend Backend>
[[nodiscard]] absolute_rect<Backend> to_absolute(
    const relative_rect<Backend>& rel,
    const ui_element<Backend>* element);

/**
 * @brief Convert absolute screen coordinates to relative coordinates
 * @param abs Rectangle in screen coordinates
 * @param parent Parent element defining the coordinate space
 * @return Rectangle relative to parent's content area
 *
 * @details
 * Subtracts parent's absolute position from absolute position.
 * Used when converting global mouse coordinates to widget-local coordinates.
 */
template<UIBackend Backend>
[[nodiscard]] relative_rect<Backend> to_relative(
    const absolute_rect<Backend>& abs,
    const ui_element<Backend>* parent);

/**
 * @brief Convert relative point to absolute screen coordinates
 * @details Point-specific conversion (uses same logic as rect conversion)
 */
template<UIBackend Backend>
[[nodiscard]] absolute_point<Backend> to_absolute(
    const relative_point<Backend>& rel_pt,
    const ui_element<Backend>* element);

/**
 * @brief Convert absolute point to relative coordinates
 */
template<UIBackend Backend>
[[nodiscard]] relative_point<Backend> to_relative(
    const absolute_point<Backend>& abs_pt,
    const ui_element<Backend>* parent);

} // namespace geometry
} // namespace onyxui

// Include implementation (conversion functions need ui_element definition)
#include <onyxui/geometry/coordinates.inl>
