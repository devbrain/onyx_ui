/**
 * @file udim.hh
 * @brief Unified Dimension (UDim) for mixed relative+absolute sizing
 * @author OnyxUI Framework
 * @date 2025-11-26
 *
 * @details
 * UDim combines relative (percentage) and absolute (logical units) sizing
 * in a single value, inspired by CEGUI's UDim system.
 *
 * Examples:
 * - UDim(0.5, 0) = 50% of parent
 * - UDim(0.0, 10) = 10 logical units
 * - UDim(1.0, -20) = 100% of parent minus 20 logical units
 * - UDim(0.5, 5) = 50% of parent plus 5 logical units
 */

#pragma once

#include "types.hh"
#include "geometry.hh"

namespace onyxui {

    /**
     * @class udim
     * @brief Unified dimension combining relative scale and absolute offset
     *
     * Formula: final_value = (parent_size * scale) + offset
     *
     * This allows expressing sizes like:
     * - "50% of parent" → udim(0.5, 0)
     * - "100% minus 20 units" → udim(1.0, -20)
     * - "50% plus 10 units" → udim(0.5, 10)
     */
    class udim {
    public:
        double scale;              ///< Relative scale (0.0 to 1.0 typically)
        logical_unit offset;       ///< Absolute offset in logical units

        // Construction
        constexpr udim() noexcept : scale(0.0), offset(0.0) {}

        constexpr udim(double s, logical_unit off) noexcept
            : scale(s), offset(off) {}

        constexpr udim(double s, double off) noexcept
            : scale(s), offset(logical_unit(off)) {}

        // Resolve to absolute value given parent size
        [[nodiscard]] constexpr logical_unit resolve(logical_unit parent_size) const noexcept {
            return logical_unit(parent_size.value * scale) + offset;
        }

        // Arithmetic operators
        [[nodiscard]] constexpr udim operator+(const udim& other) const noexcept {
            return {scale + other.scale, offset + other.offset};
        }

        [[nodiscard]] constexpr udim operator-(const udim& other) const noexcept {
            return {scale - other.scale, offset - other.offset};
        }

        [[nodiscard]] constexpr udim operator*(double scalar) const noexcept {
            return {scale * scalar, offset * scalar};
        }

        [[nodiscard]] constexpr udim operator/(double divisor) const noexcept {
            return {scale / divisor, offset / divisor};
        }

        [[nodiscard]] constexpr udim operator/(int divisor) const noexcept {
            return {scale / static_cast<double>(divisor), offset / divisor};
        }

        // Unary minus
        [[nodiscard]] constexpr udim operator-() const noexcept {
            return {-scale, -offset};
        }

        // Comparison
        [[nodiscard]] constexpr bool operator==(const udim& other) const noexcept {
            return std::abs(scale - other.scale) < 1e-9 && offset == other.offset;
        }

        [[nodiscard]] constexpr bool operator!=(const udim& other) const noexcept {
            return !(*this == other);
        }

        // Compound assignment
        constexpr udim& operator+=(const udim& other) noexcept {
            scale += other.scale;
            offset += other.offset;
            return *this;
        }

        constexpr udim& operator-=(const udim& other) noexcept {
            scale -= other.scale;
            offset -= other.offset;
            return *this;
        }

        constexpr udim& operator*=(double scalar) noexcept {
            scale *= scalar;
            offset *= scalar;
            return *this;
        }

        constexpr udim& operator/=(double divisor) noexcept {
            scale /= divisor;
            offset /= divisor;
            return *this;
        }

        constexpr udim& operator/=(int divisor) noexcept {
            scale /= static_cast<double>(divisor);
            offset /= divisor;
            return *this;
        }
    };

    // Free function scalar multiplication
    [[nodiscard]] constexpr udim operator*(double scalar, const udim& u) noexcept {
        return u * scalar;
    }

    /**
     * @struct udim2
     * @brief Two-dimensional unified dimensions (width, height)
     */
    struct udim2 {
        udim width;
        udim height;

        constexpr udim2() noexcept : width(), height() {}

        constexpr udim2(const udim& w, const udim& h) noexcept
            : width(w), height(h) {}

        constexpr udim2(double scale_w, logical_unit offset_w,
                       double scale_h, logical_unit offset_h) noexcept
            : width(scale_w, offset_w), height(scale_h, offset_h) {}

        // Resolve to absolute size given parent size
        [[nodiscard]] constexpr logical_size resolve(const logical_size& parent_size) const noexcept {
            return {
                width.resolve(parent_size.width),
                height.resolve(parent_size.height)
            };
        }

        // Comparison
        [[nodiscard]] constexpr bool operator==(const udim2& other) const noexcept {
            return width == other.width && height == other.height;
        }

        [[nodiscard]] constexpr bool operator!=(const udim2& other) const noexcept {
            return !(*this == other);
        }

        // Arithmetic
        [[nodiscard]] constexpr udim2 operator+(const udim2& other) const noexcept {
            return {width + other.width, height + other.height};
        }

        [[nodiscard]] constexpr udim2 operator-(const udim2& other) const noexcept {
            return {width - other.width, height - other.height};
        }

        [[nodiscard]] constexpr udim2 operator*(double scalar) const noexcept {
            return {width * scalar, height * scalar};
        }

        [[nodiscard]] constexpr udim2 operator/(double divisor) const noexcept {
            return {width / divisor, height / divisor};
        }

        [[nodiscard]] constexpr udim2 operator/(int divisor) const noexcept {
            return {width / divisor, height / divisor};
        }
    };

    // Free function scalar multiplication
    [[nodiscard]] constexpr udim2 operator*(double scalar, const udim2& u) noexcept {
        return u * scalar;
    }

    /**
     * @struct udim_rect
     * @brief Rectangle with unified dimensions (position and size)
     */
    struct udim_rect {
        udim x;
        udim y;
        udim width;
        udim height;

        constexpr udim_rect() noexcept : x(), y(), width(), height() {}

        constexpr udim_rect(const udim& x_val, const udim& y_val,
                           const udim& w, const udim& h) noexcept
            : x(x_val), y(y_val), width(w), height(h) {}

        // Resolve to absolute rect given parent size
        [[nodiscard]] constexpr logical_rect resolve(const logical_size& parent_size) const noexcept {
            return {
                x.resolve(parent_size.width),
                y.resolve(parent_size.height),
                width.resolve(parent_size.width),
                height.resolve(parent_size.height)
            };
        }

        // Comparison
        [[nodiscard]] constexpr bool operator==(const udim_rect& other) const noexcept {
            return x == other.x && y == other.y &&
                   width == other.width && height == other.height;
        }

        [[nodiscard]] constexpr bool operator!=(const udim_rect& other) const noexcept {
            return !(*this == other);
        }
    };

    // ========================================================================
    // Helper functions for common patterns
    // ========================================================================

    /**
     * @brief Create UDim for percentage of parent
     * @param percent Percentage (0.0 to 1.0)
     * @return UDim with scale and zero offset
     */
    [[nodiscard]] constexpr udim percent(double percent) noexcept {
        return udim(percent, 0.0);
    }

    /**
     * @brief Create UDim for absolute logical units
     * @param units Logical units
     * @return UDim with zero scale and offset
     */
    [[nodiscard]] constexpr udim absolute(logical_unit units) noexcept {
        return udim(0.0, units);
    }

    /**
     * @brief Create UDim for full parent size (100%)
     * @return UDim(1.0, 0)
     */
    [[nodiscard]] constexpr udim full() noexcept {
        return udim(1.0, 0.0);
    }

    /**
     * @brief Create UDim for half parent size (50%)
     * @return UDim(0.5, 0)
     */
    [[nodiscard]] constexpr udim half() noexcept {
        return udim(0.5, 0.0);
    }

    /**
     * @brief Create UDim2 for percentage of parent (both dimensions)
     * @param pct Percentage (0.0 to 1.0)
     * @return UDim2 with scale and zero offset
     */
    [[nodiscard]] constexpr udim2 percent2(double pct) noexcept {
        return udim2(percent(pct), percent(pct));
    }

    /**
     * @brief Create UDim2 for absolute size
     * @param width Width in logical units
     * @param height Height in logical units
     * @return UDim2 with zero scale
     */
    [[nodiscard]] constexpr udim2 absolute2(logical_unit width, logical_unit height) noexcept {
        return udim2(absolute(width), absolute(height));
    }

    /**
     * @brief Create UDim2 for full parent size (100% both dimensions)
     * @return UDim2(1.0, 0)
     */
    [[nodiscard]] constexpr udim2 full2() noexcept {
        return udim2(full(), full());
    }

} // namespace onyxui
