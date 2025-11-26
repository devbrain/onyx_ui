/**
 * @file types.hh
 * @brief Core logical unit types for backend-agnostic coordinates
 * @author OnyxUI Framework
 * @date 2025-11-26
 */

#pragma once

#include <cmath>
#include <cstdint>
#include <type_traits>

namespace onyxui {

    /**
     * @class logical_unit
     * @brief Backend-agnostic coordinate value using double precision
     *
     * Logical units represent abstract coordinates that scale appropriately
     * per backend (e.g., 1 character cell on terminal, 8 pixels on SDL).
     *
     * Uses double precision to preserve accuracy during layout calculations.
     * Explicit construction prevents accidental mixing with physical units.
     *
     * @note Conversion to physical units happens at render time via backend_metrics
     */
    class logical_unit {
    public:
        double value;

        // Explicit construction to prevent implicit conversions
        constexpr explicit logical_unit(double v) noexcept : value(v) {}
        constexpr explicit logical_unit(int v) noexcept : value(static_cast<double>(v)) {}

        // Default constructor (zero)
        constexpr logical_unit() noexcept : value(0.0) {}

        // Arithmetic operators (logical_unit)
        [[nodiscard]] constexpr logical_unit operator+(logical_unit other) const noexcept {
            return logical_unit(value + other.value);
        }

        [[nodiscard]] constexpr logical_unit operator-(logical_unit other) const noexcept {
            return logical_unit(value - other.value);
        }

        [[nodiscard]] constexpr logical_unit operator*(double scalar) const noexcept {
            return logical_unit(value * scalar);
        }

        [[nodiscard]] constexpr logical_unit operator/(double scalar) const noexcept {
            return logical_unit(value / scalar);
        }

        // Division by integer (more natural than multiplication by 0.5)
        [[nodiscard]] constexpr logical_unit operator/(int divisor) const noexcept {
            return logical_unit(value / static_cast<double>(divisor));
        }

        // Unary minus
        [[nodiscard]] constexpr logical_unit operator-() const noexcept {
            return logical_unit(-value);
        }

        // Compound assignment
        constexpr logical_unit& operator+=(logical_unit other) noexcept {
            value += other.value;
            return *this;
        }

        constexpr logical_unit& operator-=(logical_unit other) noexcept {
            value -= other.value;
            return *this;
        }

        constexpr logical_unit& operator*=(double scalar) noexcept {
            value *= scalar;
            return *this;
        }

        constexpr logical_unit& operator/=(double scalar) noexcept {
            value /= scalar;
            return *this;
        }

        constexpr logical_unit& operator/=(int divisor) noexcept {
            value /= static_cast<double>(divisor);
            return *this;
        }

        // Comparison operators (epsilon-based for floating-point)
        [[nodiscard]] constexpr bool operator==(logical_unit other) const noexcept {
            return std::abs(value - other.value) < 1e-9;
        }

        [[nodiscard]] constexpr bool operator!=(logical_unit other) const noexcept {
            return !(*this == other);
        }

        [[nodiscard]] constexpr bool operator<(logical_unit other) const noexcept {
            return value < other.value;
        }

        [[nodiscard]] constexpr bool operator<=(logical_unit other) const noexcept {
            return value <= other.value;
        }

        [[nodiscard]] constexpr bool operator>(logical_unit other) const noexcept {
            return value > other.value;
        }

        [[nodiscard]] constexpr bool operator>=(logical_unit other) const noexcept {
            return value >= other.value;
        }

        // Conversion to double (explicit)
        [[nodiscard]] constexpr explicit operator double() const noexcept {
            return value;
        }

        // Math helpers
        [[nodiscard]] constexpr logical_unit abs() const noexcept {
            return logical_unit(std::abs(value));
        }

        [[nodiscard]] constexpr logical_unit floor() const noexcept {
            return logical_unit(std::floor(value));
        }

        [[nodiscard]] constexpr logical_unit ceil() const noexcept {
            return logical_unit(std::ceil(value));
        }

        [[nodiscard]] constexpr logical_unit round() const noexcept {
            return logical_unit(std::round(value));
        }
    };

    // Free function operators (scalar * logical_unit)
    [[nodiscard]] constexpr logical_unit operator*(double scalar, logical_unit lu) noexcept {
        return lu * scalar;
    }

    // User-defined literals
    [[nodiscard]] constexpr logical_unit operator""_lu(long double v) noexcept {
        return logical_unit(static_cast<double>(v));
    }

    [[nodiscard]] constexpr logical_unit operator""_lu(unsigned long long v) noexcept {
        return logical_unit(static_cast<double>(v));
    }

    // Math functions (free functions for consistency with std::)
    [[nodiscard]] constexpr logical_unit abs(logical_unit lu) noexcept {
        return lu.abs();
    }

    [[nodiscard]] constexpr logical_unit floor(logical_unit lu) noexcept {
        return lu.floor();
    }

    [[nodiscard]] constexpr logical_unit ceil(logical_unit lu) noexcept {
        return lu.ceil();
    }

    [[nodiscard]] constexpr logical_unit round(logical_unit lu) noexcept {
        return lu.round();
    }

    [[nodiscard]] constexpr logical_unit min(logical_unit a, logical_unit b) noexcept {
        return a < b ? a : b;
    }

    [[nodiscard]] constexpr logical_unit max(logical_unit a, logical_unit b) noexcept {
        return a > b ? a : b;
    }

    [[nodiscard]] constexpr logical_unit clamp(logical_unit value, logical_unit min_val, logical_unit max_val) noexcept {
        return min(max(value, min_val), max_val);
    }

} // namespace onyxui
