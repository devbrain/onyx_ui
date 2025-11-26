/**
 * @file geometry.hh
 * @brief Logical geometry types (size, point, rect) for backend-agnostic layout
 * @author OnyxUI Framework
 * @date 2025-11-26
 */

#pragma once

#include "types.hh"
#include <algorithm>

namespace onyxui {

    /**
     * @struct logical_size
     * @brief Two-dimensional size in logical units
     */
    struct logical_size {
        logical_unit width;
        logical_unit height;

        constexpr logical_size() noexcept : width(0.0), height(0.0) {}

        constexpr logical_size(logical_unit w, logical_unit h) noexcept
            : width(w), height(h) {}

        // Comparison
        [[nodiscard]] constexpr bool operator==(const logical_size& other) const noexcept {
            return width == other.width && height == other.height;
        }

        [[nodiscard]] constexpr bool operator!=(const logical_size& other) const noexcept {
            return !(*this == other);
        }

        // Arithmetic
        [[nodiscard]] constexpr logical_size operator+(const logical_size& other) const noexcept {
            return {width + other.width, height + other.height};
        }

        [[nodiscard]] constexpr logical_size operator-(const logical_size& other) const noexcept {
            return {width - other.width, height - other.height};
        }

        [[nodiscard]] constexpr logical_size operator*(double scalar) const noexcept {
            return {width * scalar, height * scalar};
        }

        [[nodiscard]] constexpr logical_size operator/(double scalar) const noexcept {
            return {width / scalar, height / scalar};
        }

        [[nodiscard]] constexpr logical_size operator/(int divisor) const noexcept {
            return {width / divisor, height / divisor};
        }

        // Helpers
        [[nodiscard]] constexpr bool is_empty() const noexcept {
            return width.value <= 0.0 || height.value <= 0.0;
        }

        [[nodiscard]] constexpr logical_unit area() const noexcept {
            return logical_unit(width.value * height.value);
        }
    };

    // Free function scalar multiplication
    [[nodiscard]] constexpr logical_size operator*(double scalar, const logical_size& size) noexcept {
        return size * scalar;
    }

    /**
     * @struct logical_point
     * @brief Two-dimensional point in logical units
     */
    struct logical_point {
        logical_unit x;
        logical_unit y;

        constexpr logical_point() noexcept : x(0.0), y(0.0) {}

        constexpr logical_point(logical_unit x_val, logical_unit y_val) noexcept
            : x(x_val), y(y_val) {}

        // Comparison
        [[nodiscard]] constexpr bool operator==(const logical_point& other) const noexcept {
            return x == other.x && y == other.y;
        }

        [[nodiscard]] constexpr bool operator!=(const logical_point& other) const noexcept {
            return !(*this == other);
        }

        // Arithmetic (point + size = point)
        [[nodiscard]] constexpr logical_point operator+(const logical_size& size) const noexcept {
            return {x + size.width, y + size.height};
        }

        [[nodiscard]] constexpr logical_point operator-(const logical_size& size) const noexcept {
            return {x - size.width, y - size.height};
        }

        // Vector arithmetic (point - point = size/vector)
        [[nodiscard]] constexpr logical_size operator-(const logical_point& other) const noexcept {
            return {x - other.x, y - other.y};
        }

        // Offset by individual units
        [[nodiscard]] constexpr logical_point offset(logical_unit dx, logical_unit dy) const noexcept {
            return {x + dx, y + dy};
        }
    };

    /**
     * @struct logical_rect
     * @brief Two-dimensional rectangle in logical units
     *
     * Represents a rectangle with position (x, y) and size (width, height).
     * All values are in logical units for backend-agnostic layout.
     */
    struct logical_rect {
        logical_unit x;
        logical_unit y;
        logical_unit width;
        logical_unit height;

        constexpr logical_rect() noexcept
            : x(0.0), y(0.0), width(0.0), height(0.0) {}

        constexpr logical_rect(logical_unit x_val, logical_unit y_val,
                              logical_unit w, logical_unit h) noexcept
            : x(x_val), y(y_val), width(w), height(h) {}

        constexpr logical_rect(const logical_point& pos, const logical_size& size) noexcept
            : x(pos.x), y(pos.y), width(size.width), height(size.height) {}

        // Accessors
        [[nodiscard]] constexpr logical_point position() const noexcept {
            return {x, y};
        }

        [[nodiscard]] constexpr logical_size size() const noexcept {
            return {width, height};
        }

        [[nodiscard]] constexpr logical_unit left() const noexcept {
            return x;
        }

        [[nodiscard]] constexpr logical_unit top() const noexcept {
            return y;
        }

        [[nodiscard]] constexpr logical_unit right() const noexcept {
            return x + width;
        }

        [[nodiscard]] constexpr logical_unit bottom() const noexcept {
            return y + height;
        }

        [[nodiscard]] constexpr logical_point top_left() const noexcept {
            return {x, y};
        }

        [[nodiscard]] constexpr logical_point top_right() const noexcept {
            return {x + width, y};
        }

        [[nodiscard]] constexpr logical_point bottom_left() const noexcept {
            return {x, y + height};
        }

        [[nodiscard]] constexpr logical_point bottom_right() const noexcept {
            return {x + width, y + height};
        }

        [[nodiscard]] constexpr logical_point center() const noexcept {
            return {x + width / 2, y + height / 2};
        }

        // Predicates
        [[nodiscard]] constexpr bool is_empty() const noexcept {
            return width.value <= 0.0 || height.value <= 0.0;
        }

        [[nodiscard]] constexpr bool contains(const logical_point& point) const noexcept {
            return point.x >= x && point.x < (x + width) &&
                   point.y >= y && point.y < (y + height);
        }

        [[nodiscard]] constexpr bool contains(const logical_rect& other) const noexcept {
            return other.x >= x && other.right() <= right() &&
                   other.y >= y && other.bottom() <= bottom();
        }

        [[nodiscard]] constexpr bool intersects(const logical_rect& other) const noexcept {
            return !(other.right() <= x || other.x >= right() ||
                    other.bottom() <= y || other.y >= bottom());
        }

        // Transformations
        [[nodiscard]] constexpr logical_rect offset(logical_unit dx, logical_unit dy) const noexcept {
            return {x + dx, y + dy, width, height};
        }

        [[nodiscard]] constexpr logical_rect offset(const logical_size& delta) const noexcept {
            return {x + delta.width, y + delta.height, width, height};
        }

        [[nodiscard]] constexpr logical_rect inflate(logical_unit dw, logical_unit dh) const noexcept {
            return {x - dw, y - dh, width + dw * 2.0, height + dh * 2.0};
        }

        [[nodiscard]] constexpr logical_rect deflate(logical_unit dw, logical_unit dh) const noexcept {
            return inflate(-dw, -dh);
        }

        // Set operations
        [[nodiscard]] constexpr logical_rect intersect(const logical_rect& other) const noexcept {
            const auto l = max(left(), other.left());
            const auto t = max(top(), other.top());
            const auto r = min(right(), other.right());
            const auto b = min(bottom(), other.bottom());

            if (r <= l || b <= t) {
                return {}; // Empty rect
            }

            return {l, t, r - l, b - t};
        }

        [[nodiscard]] constexpr logical_rect union_with(const logical_rect& other) const noexcept {
            if (is_empty()) return other;
            if (other.is_empty()) return *this;

            const auto l = min(left(), other.left());
            const auto t = min(top(), other.top());
            const auto r = max(right(), other.right());
            const auto b = max(bottom(), other.bottom());

            return {l, t, r - l, b - t};
        }

        // Comparison
        [[nodiscard]] constexpr bool operator==(const logical_rect& other) const noexcept {
            return x == other.x && y == other.y &&
                   width == other.width && height == other.height;
        }

        [[nodiscard]] constexpr bool operator!=(const logical_rect& other) const noexcept {
            return !(*this == other);
        }
    };

    /**
     * @struct logical_thickness
     * @brief Thickness/margin/padding in logical units (left, top, right, bottom)
     */
    struct logical_thickness {
        logical_unit left;
        logical_unit top;
        logical_unit right;
        logical_unit bottom;

        constexpr logical_thickness() noexcept
            : left(0.0), top(0.0), right(0.0), bottom(0.0) {}

        constexpr logical_thickness(logical_unit all) noexcept
            : left(all), top(all), right(all), bottom(all) {}

        constexpr logical_thickness(logical_unit horizontal, logical_unit vertical) noexcept
            : left(horizontal), top(vertical), right(horizontal), bottom(vertical) {}

        constexpr logical_thickness(logical_unit l, logical_unit t, logical_unit r, logical_unit b) noexcept
            : left(l), top(t), right(r), bottom(b) {}

        [[nodiscard]] constexpr logical_unit horizontal() const noexcept {
            return left + right;
        }

        [[nodiscard]] constexpr logical_unit vertical() const noexcept {
            return top + bottom;
        }

        [[nodiscard]] constexpr logical_size total_size() const noexcept {
            return {horizontal(), vertical()};
        }

        [[nodiscard]] constexpr bool operator==(const logical_thickness& other) const noexcept {
            return left == other.left && top == other.top &&
                   right == other.right && bottom == other.bottom;
        }

        [[nodiscard]] constexpr bool operator!=(const logical_thickness& other) const noexcept {
            return !(*this == other);
        }
    };

} // namespace onyxui
