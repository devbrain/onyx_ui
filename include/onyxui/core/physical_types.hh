/**
 * @file physical_types.hh
 * @brief Strong types for physical coordinates (pixels/character cells)
 * @author OnyxUI Framework
 * @date 2026-01
 *
 * @details
 * Physical units represent actual screen coordinates (pixels for SDL, character
 * cells for terminal). Separate types for X and Y prevent accidental coordinate
 * swaps at compile time, as backends may have different X and Y scale factors.
 *
 * Physical types are int-based since screen coordinates are always integers.
 * Conversion from logical_unit happens via backend_metrics at render time.
 */

#pragma once

#include <algorithm>
#include <cstdint>

namespace onyxui {

// Forward declarations
class physical_x;
class physical_y;
struct physical_size;
struct physical_point;
struct physical_rect;

/**
 * @class physical_x
 * @brief Physical X-axis coordinate (pixels or character columns)
 *
 * Strong type preventing accidental mixing with Y coordinates or plain integers.
 * Explicit construction required to prevent implicit conversions.
 */
class physical_x {
public:
    int value;

    // Explicit construction to prevent implicit conversions
    constexpr explicit physical_x(int v) noexcept : value(v) {}

    // Default constructor (zero)
    constexpr physical_x() noexcept : value(0) {}

    // Arithmetic operators (physical_x only)
    [[nodiscard]] constexpr physical_x operator+(physical_x other) const noexcept {
        return physical_x(value + other.value);
    }

    [[nodiscard]] constexpr physical_x operator-(physical_x other) const noexcept {
        return physical_x(value - other.value);
    }

    [[nodiscard]] constexpr physical_x operator*(int scalar) const noexcept {
        return physical_x(value * scalar);
    }

    [[nodiscard]] constexpr physical_x operator/(int divisor) const noexcept {
        return physical_x(value / divisor);
    }

    // Unary minus
    [[nodiscard]] constexpr physical_x operator-() const noexcept {
        return physical_x(-value);
    }

    // Compound assignment
    constexpr physical_x& operator+=(physical_x other) noexcept {
        value += other.value;
        return *this;
    }

    constexpr physical_x& operator-=(physical_x other) noexcept {
        value -= other.value;
        return *this;
    }

    constexpr physical_x& operator*=(int scalar) noexcept {
        value *= scalar;
        return *this;
    }

    constexpr physical_x& operator/=(int divisor) noexcept {
        value /= divisor;
        return *this;
    }

    // Comparison operators
    [[nodiscard]] constexpr bool operator==(physical_x other) const noexcept {
        return value == other.value;
    }

    [[nodiscard]] constexpr bool operator!=(physical_x other) const noexcept {
        return value != other.value;
    }

    [[nodiscard]] constexpr bool operator<(physical_x other) const noexcept {
        return value < other.value;
    }

    [[nodiscard]] constexpr bool operator<=(physical_x other) const noexcept {
        return value <= other.value;
    }

    [[nodiscard]] constexpr bool operator>(physical_x other) const noexcept {
        return value > other.value;
    }

    [[nodiscard]] constexpr bool operator>=(physical_x other) const noexcept {
        return value >= other.value;
    }

    // Explicit conversion to int (for renderer calls)
    [[nodiscard]] constexpr explicit operator int() const noexcept {
        return value;
    }

    // Math helpers
    [[nodiscard]] constexpr physical_x abs() const noexcept {
        return physical_x(value < 0 ? -value : value);
    }
};

/**
 * @class physical_y
 * @brief Physical Y-axis coordinate (pixels or character rows)
 *
 * Strong type preventing accidental mixing with X coordinates or plain integers.
 * Explicit construction required to prevent implicit conversions.
 */
class physical_y {
public:
    int value;

    // Explicit construction to prevent implicit conversions
    constexpr explicit physical_y(int v) noexcept : value(v) {}

    // Default constructor (zero)
    constexpr physical_y() noexcept : value(0) {}

    // Arithmetic operators (physical_y only)
    [[nodiscard]] constexpr physical_y operator+(physical_y other) const noexcept {
        return physical_y(value + other.value);
    }

    [[nodiscard]] constexpr physical_y operator-(physical_y other) const noexcept {
        return physical_y(value - other.value);
    }

    [[nodiscard]] constexpr physical_y operator*(int scalar) const noexcept {
        return physical_y(value * scalar);
    }

    [[nodiscard]] constexpr physical_y operator/(int divisor) const noexcept {
        return physical_y(value / divisor);
    }

    // Unary minus
    [[nodiscard]] constexpr physical_y operator-() const noexcept {
        return physical_y(-value);
    }

    // Compound assignment
    constexpr physical_y& operator+=(physical_y other) noexcept {
        value += other.value;
        return *this;
    }

    constexpr physical_y& operator-=(physical_y other) noexcept {
        value -= other.value;
        return *this;
    }

    constexpr physical_y& operator*=(int scalar) noexcept {
        value *= scalar;
        return *this;
    }

    constexpr physical_y& operator/=(int divisor) noexcept {
        value /= divisor;
        return *this;
    }

    // Comparison operators
    [[nodiscard]] constexpr bool operator==(physical_y other) const noexcept {
        return value == other.value;
    }

    [[nodiscard]] constexpr bool operator!=(physical_y other) const noexcept {
        return value != other.value;
    }

    [[nodiscard]] constexpr bool operator<(physical_y other) const noexcept {
        return value < other.value;
    }

    [[nodiscard]] constexpr bool operator<=(physical_y other) const noexcept {
        return value <= other.value;
    }

    [[nodiscard]] constexpr bool operator>(physical_y other) const noexcept {
        return value > other.value;
    }

    [[nodiscard]] constexpr bool operator>=(physical_y other) const noexcept {
        return value >= other.value;
    }

    // Explicit conversion to int (for renderer calls)
    [[nodiscard]] constexpr explicit operator int() const noexcept {
        return value;
    }

    // Math helpers
    [[nodiscard]] constexpr physical_y abs() const noexcept {
        return physical_y(value < 0 ? -value : value);
    }
};

// Free function operators (scalar * physical_x/y)
[[nodiscard]] constexpr physical_x operator*(int scalar, physical_x px) noexcept {
    return px * scalar;
}

[[nodiscard]] constexpr physical_y operator*(int scalar, physical_y py) noexcept {
    return py * scalar;
}

// User-defined literals
[[nodiscard]] constexpr physical_x operator""_px(unsigned long long v) noexcept {
    return physical_x(static_cast<int>(v));
}

[[nodiscard]] constexpr physical_y operator""_py(unsigned long long v) noexcept {
    return physical_y(static_cast<int>(v));
}

// Math functions (free functions)
[[nodiscard]] constexpr physical_x abs(physical_x px) noexcept {
    return px.abs();
}

[[nodiscard]] constexpr physical_y abs(physical_y py) noexcept {
    return py.abs();
}

[[nodiscard]] constexpr physical_x min(physical_x a, physical_x b) noexcept {
    return a < b ? a : b;
}

[[nodiscard]] constexpr physical_x max(physical_x a, physical_x b) noexcept {
    return a > b ? a : b;
}

[[nodiscard]] constexpr physical_y min(physical_y a, physical_y b) noexcept {
    return a < b ? a : b;
}

[[nodiscard]] constexpr physical_y max(physical_y a, physical_y b) noexcept {
    return a > b ? a : b;
}

[[nodiscard]] constexpr physical_x clamp(physical_x value, physical_x min_val, physical_x max_val) noexcept {
    return min(max(value, min_val), max_val);
}

[[nodiscard]] constexpr physical_y clamp(physical_y value, physical_y min_val, physical_y max_val) noexcept {
    return min(max(value, min_val), max_val);
}

/**
 * @struct physical_size
 * @brief 2D size in physical units (width: physical_x, height: physical_y)
 */
struct physical_size {
    physical_x width;
    physical_y height;

    constexpr physical_size() noexcept : width(), height() {}
    constexpr physical_size(physical_x w, physical_y h) noexcept : width(w), height(h) {}

    [[nodiscard]] constexpr bool is_empty() const noexcept {
        return width.value <= 0 || height.value <= 0;
    }

    [[nodiscard]] constexpr bool operator==(const physical_size& other) const noexcept {
        return width == other.width && height == other.height;
    }

    [[nodiscard]] constexpr bool operator!=(const physical_size& other) const noexcept {
        return !(*this == other);
    }

    // Arithmetic
    [[nodiscard]] constexpr physical_size operator+(const physical_size& other) const noexcept {
        return {width + other.width, height + other.height};
    }

    [[nodiscard]] constexpr physical_size operator-(const physical_size& other) const noexcept {
        return {width - other.width, height - other.height};
    }
};

/**
 * @struct physical_point
 * @brief 2D point in physical units (x: physical_x, y: physical_y)
 */
struct physical_point {
    physical_x x;
    physical_y y;

    constexpr physical_point() noexcept : x(), y() {}
    constexpr physical_point(physical_x x_, physical_y y_) noexcept : x(x_), y(y_) {}

    [[nodiscard]] constexpr bool operator==(const physical_point& other) const noexcept {
        return x == other.x && y == other.y;
    }

    [[nodiscard]] constexpr bool operator!=(const physical_point& other) const noexcept {
        return !(*this == other);
    }

    // Vector arithmetic
    [[nodiscard]] constexpr physical_point operator+(const physical_size& size) const noexcept {
        return {x + size.width, y + size.height};
    }

    [[nodiscard]] constexpr physical_point operator-(const physical_size& size) const noexcept {
        return {x - size.width, y - size.height};
    }

    [[nodiscard]] constexpr physical_size operator-(const physical_point& other) const noexcept {
        return {x - other.x, y - other.y};
    }
};

/**
 * @struct physical_rect
 * @brief Rectangle in physical units
 *
 * Stores position (x, y) and dimensions (width, height) using strong physical types.
 */
struct physical_rect {
    physical_x x;
    physical_y y;
    physical_x width;
    physical_y height;

    constexpr physical_rect() noexcept : x(), y(), width(), height() {}

    constexpr physical_rect(physical_x x_, physical_y y_, physical_x w, physical_y h) noexcept
        : x(x_), y(y_), width(w), height(h) {}

    constexpr physical_rect(physical_point pos, physical_size size) noexcept
        : x(pos.x), y(pos.y), width(size.width), height(size.height) {}

    // Edge accessors
    [[nodiscard]] constexpr physical_x left() const noexcept { return x; }
    [[nodiscard]] constexpr physical_y top() const noexcept { return y; }
    [[nodiscard]] constexpr physical_x right() const noexcept { return x + width; }
    [[nodiscard]] constexpr physical_y bottom() const noexcept { return y + height; }

    // Position and size
    [[nodiscard]] constexpr physical_point position() const noexcept { return {x, y}; }
    [[nodiscard]] constexpr physical_size size() const noexcept { return {width, height}; }

    [[nodiscard]] constexpr bool is_empty() const noexcept {
        return width.value <= 0 || height.value <= 0;
    }

    [[nodiscard]] constexpr bool operator==(const physical_rect& other) const noexcept {
        return x == other.x && y == other.y && width == other.width && height == other.height;
    }

    [[nodiscard]] constexpr bool operator!=(const physical_rect& other) const noexcept {
        return !(*this == other);
    }

    // Point containment
    [[nodiscard]] constexpr bool contains(const physical_point& pt) const noexcept {
        return pt.x >= x && pt.x < right() && pt.y >= y && pt.y < bottom();
    }

    [[nodiscard]] constexpr bool contains(physical_x px, physical_y py) const noexcept {
        return px >= x && px < right() && py >= y && py < bottom();
    }

    // Rectangle intersection
    [[nodiscard]] constexpr bool intersects(const physical_rect& other) const noexcept {
        return x < other.right() && right() > other.x &&
               y < other.bottom() && bottom() > other.y;
    }

    // Translate by offset
    [[nodiscard]] constexpr physical_rect translated(physical_x dx, physical_y dy) const noexcept {
        return {x + dx, y + dy, width, height};
    }

    [[nodiscard]] constexpr physical_rect translated(const physical_point& offset) const noexcept {
        return {x + offset.x, y + offset.y, width, height};
    }
};

} // namespace onyxui
