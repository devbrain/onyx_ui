/**
 * @file coordinates.inl
 * @brief Implementation of coordinate conversion functions
 * @author OnyxUI Architecture Team
 * @date 2025-11-11
 *
 * @details
 * Contains the implementation of conversion functions that require access to
 * ui_element for walking the parent chain. This file is included at the end
 * of coordinates.hh to avoid circular dependencies.
 */

#pragma once

// This file is included from coordinates.hh and requires ui_element to be defined
// If ui_element is not yet defined, the conversion functions will fail to compile

namespace onyxui::geometry {

template<UIBackend Backend>
[[nodiscard]] absolute_rect<Backend> to_absolute(
    const relative_rect<Backend>& rel,
    const ui_element<Backend>* element)
{
    auto result = rel.get();  // Start with relative rect (backend type)

    // Walk parent chain to accumulate offsets
    const ui_element<Backend>* current = element->parent();
    while (current) {
        auto parent_bounds = current->bounds().get();  // Get underlying Backend::rect_type
        auto parent_content = current->get_content_area();  // Get parent's content area

        // Children are positioned relative to parent's content area
        // So we need to add: parent's position + content area offset
        const int px = rect_utils::get_x(parent_bounds) + rect_utils::get_x(parent_content);
        const int py = rect_utils::get_y(parent_bounds) + rect_utils::get_y(parent_content);

        // Offset the result rectangle
        rect_utils::set_bounds(result,
                              rect_utils::get_x(result) + px,
                              rect_utils::get_y(result) + py,
                              rect_utils::get_width(result),
                              rect_utils::get_height(result));

        current = current->parent();
    }

    return absolute_rect<Backend>{result};  // Wrap as absolute
}

template<UIBackend Backend>
[[nodiscard]] relative_rect<Backend> to_relative(
    const absolute_rect<Backend>& abs,
    const ui_element<Backend>* parent)
{
    if (!parent) {
        return relative_rect<Backend>{abs.get()};  // Root element
    }

    auto result = abs.get();  // Start with absolute rect

    // Get parent's absolute position
    auto parent_abs = parent->get_absolute_bounds().get();  // Get underlying Backend::rect_type
    const int px = rect_utils::get_x(parent_abs);
    const int py = rect_utils::get_y(parent_abs);

    // Subtract parent's position using rect_utils
    rect_utils::set_bounds(result,
                          rect_utils::get_x(result) - px,
                          rect_utils::get_y(result) - py,
                          rect_utils::get_width(result),
                          rect_utils::get_height(result));

    return relative_rect<Backend>{result};  // Wrap as relative
}

template<UIBackend Backend>
[[nodiscard]] absolute_point<Backend> to_absolute(
    const relative_point<Backend>& rel_pt,
    const ui_element<Backend>* element)
{
    // Start with the point's coordinates relative to element
    int abs_x = rel_pt.x();
    int abs_y = rel_pt.y();

    // Add element's own position
    auto element_bounds = element->bounds().get();  // Get underlying Backend::rect_type
    abs_x += rect_utils::get_x(element_bounds);
    abs_y += rect_utils::get_y(element_bounds);

    // Walk parent chain to accumulate offsets
    const ui_element<Backend>* current = element->parent();
    while (current) {
        auto parent_bounds = current->bounds().get();  // Get underlying Backend::rect_type
        auto parent_content = current->get_content_area();  // Get parent's content area

        // Children are positioned relative to parent's content area
        abs_x += rect_utils::get_x(parent_bounds) + rect_utils::get_x(parent_content);
        abs_y += rect_utils::get_y(parent_bounds) + rect_utils::get_y(parent_content);
        current = current->parent();
    }

    // Create backend point with absolute coordinates
    typename Backend::point_type result_pt;
    if constexpr (detail::has_member_x<typename Backend::point_type> &&
                  detail::has_member_y<typename Backend::point_type>) {
        result_pt.x = abs_x;
        result_pt.y = abs_y;
    } else {
        result_pt = typename Backend::point_type{abs_x, abs_y};
    }

    return absolute_point<Backend>{result_pt};
}

template<UIBackend Backend>
[[nodiscard]] relative_point<Backend> to_relative(
    const absolute_point<Backend>& abs_pt,
    const ui_element<Backend>* parent)
{
    // Create temporary 1x1 rect at point, convert, extract point
    typename Backend::rect_type temp_rect;
    rect_utils::set_bounds(temp_rect, abs_pt.x(), abs_pt.y(), 1, 1);

    auto rel_rect = to_relative(absolute_rect<Backend>{temp_rect}, parent);

    // Extract point from converted rectangle
    typename Backend::point_type result_pt;
    if constexpr (detail::has_member_x<typename Backend::point_type> &&
                  detail::has_member_y<typename Backend::point_type>) {
        result_pt.x = rel_rect.x();
        result_pt.y = rel_rect.y();
    } else {
        // If point type doesn't have direct member access, we need a different approach
        // For now, assume point types are aggregate-initializable
        result_pt = typename Backend::point_type{rel_rect.x(), rel_rect.y()};
    }

    return relative_point<Backend>{result_pt};
}

} // namespace onyxui::geometry
