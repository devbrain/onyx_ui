/**
 * @file backend_metrics.hh
 * @brief Backend metrics for converting logical units to physical coordinates
 * @author OnyxUI Framework
 * @date 2025-11-26
 *
 * @note Updated 2026-01 to return strong physical types (physical_x, physical_y, etc.)
 */

#pragma once

#include "geometry.hh"
#include "physical_types.hh"
#include "../concepts/backend.hh"
#include "../concepts/rect_like.hh"
#include "../concepts/size_like.hh"
#include "../concepts/point_like.hh"
#include <cmath>

namespace onyxui {

    /**
     * @enum snap_mode
     * @brief Snapping mode for logical→physical conversion
     */
    enum class snap_mode : std::uint8_t {
        floor,   ///< Round down towards negative infinity (for positions)
        ceil,    ///< Round up towards positive infinity (for far edges)
        round,   ///< Round to nearest integer (for sizes computed directly)
        truncate ///< Truncate towards zero (legacy, prefer floor/ceil for consistency)
    };

    /**
     * @struct backend_metrics
     * @brief Metrics for converting logical units to physical backend coordinates
     *
     * Encapsulates scaling factors, DPI, and snapping strategies
     * for a specific backend (terminal, SDL, etc.).
     *
     * @tparam Backend The UI backend type
     */
    template<UIBackend Backend>
    struct backend_metrics {
        // Conversion factors (logical → physical)
        double logical_to_physical_x = 1.0;  ///< X-axis scale (e.g., 1.0 for conio, 8.0 for SDL)
        double logical_to_physical_y = 1.0;  ///< Y-axis scale (e.g., 1.0 for conio, 8.0 for SDL)

        // DPI scaling (for high-DPI displays)
        double dpi_scale = 1.0;  ///< DPI multiplier (1.0, 1.5, 2.0, 3.0, etc.)

        /**
         * @brief Snap logical X coordinate to physical coordinate
         * @param lu Logical unit value
         * @param mode Snapping mode
         * @return Physical X coordinate (strong type)
         */
        [[nodiscard]] constexpr physical_x snap_to_physical_x(logical_unit lu, snap_mode mode) const noexcept {
            const double physical = lu.value * logical_to_physical_x * dpi_scale;
            switch (mode) {
                case snap_mode::floor:    return physical_x(static_cast<int>(std::floor(physical)));
                case snap_mode::ceil:     return physical_x(static_cast<int>(std::ceil(physical)));
                case snap_mode::round:    return physical_x(static_cast<int>(std::round(physical)));
                case snap_mode::truncate: return physical_x(static_cast<int>(physical));
            }
            return physical_x(static_cast<int>(std::floor(physical)));  // Default to floor
        }

        /**
         * @brief Snap logical Y coordinate to physical coordinate
         * @param lu Logical unit value
         * @param mode Snapping mode
         * @return Physical Y coordinate (strong type)
         */
        [[nodiscard]] constexpr physical_y snap_to_physical_y(logical_unit lu, snap_mode mode) const noexcept {
            const double physical = lu.value * logical_to_physical_y * dpi_scale;
            switch (mode) {
                case snap_mode::floor:    return physical_y(static_cast<int>(std::floor(physical)));
                case snap_mode::ceil:     return physical_y(static_cast<int>(std::ceil(physical)));
                case snap_mode::round:    return physical_y(static_cast<int>(std::round(physical)));
                case snap_mode::truncate: return physical_y(static_cast<int>(physical));
            }
            return physical_y(static_cast<int>(std::floor(physical)));  // Default to floor
        }

        /**
         * @brief Convert logical size to physical size
         * @param size Logical size
         * @return Physical size (strong type)
         *
         * @note Uses round() for direct size conversion
         */
        [[nodiscard]] constexpr physical_size snap_size(const logical_size& size) const noexcept {
            return {
                snap_to_physical_x(size.width, snap_mode::round),
                snap_to_physical_y(size.height, snap_mode::round)
            };
        }

        /**
         * @brief Convert logical point to physical point
         * @param point Logical point
         * @return Physical point (strong type)
         *
         * @note Uses floor() for position snapping
         */
        [[nodiscard]] constexpr physical_point snap_point(const logical_point& point) const noexcept {
            return {
                snap_to_physical_x(point.x, snap_mode::floor),
                snap_to_physical_y(point.y, snap_mode::floor)
            };
        }

        /**
         * @brief Convert logical rect to physical rect (edge-based snapping)
         * @param rect Logical rect
         * @return Physical rect (strong type)
         *
         * Snapping strategy:
         * 1. Floor position (left, top)
         * 2. Ceil far edges (right, bottom)
         * 3. Compute size from edges (avoids accumulation errors)
         *
         * This prevents gaps and overlaps in tiled layouts.
         */
        [[nodiscard]] constexpr physical_rect snap_rect(const logical_rect& rect) const noexcept {
            // Snap edges
            const physical_x px_left   = snap_to_physical_x(rect.left(), snap_mode::floor);
            const physical_y px_top    = snap_to_physical_y(rect.top(), snap_mode::floor);
            const physical_x px_right  = snap_to_physical_x(rect.right(), snap_mode::ceil);
            const physical_y px_bottom = snap_to_physical_y(rect.bottom(), snap_mode::ceil);

            // Compute size from edges (not from logical size directly)
            const physical_x px_width  = px_right - px_left;
            const physical_y px_height = px_bottom - px_top;

            return {px_left, px_top, px_width, px_height};
        }

        /**
         * @brief Convert physical X coordinate to logical X coordinate
         * @param px Physical X coordinate (strong type)
         * @return Logical unit
         */
        [[nodiscard]] constexpr logical_unit physical_to_logical_x(physical_x px) const noexcept {
            return logical_unit(static_cast<double>(px.value) / (logical_to_physical_x * dpi_scale));
        }

        /**
         * @brief Convert physical Y coordinate to logical Y coordinate
         * @param py Physical Y coordinate (strong type)
         * @return Logical unit
         */
        [[nodiscard]] constexpr logical_unit physical_to_logical_y(physical_y py) const noexcept {
            return logical_unit(static_cast<double>(py.value) / (logical_to_physical_y * dpi_scale));
        }

        /**
         * @brief Convert physical size to logical size
         * @param size Physical size (strong type)
         * @return Logical size
         */
        [[nodiscard]] constexpr logical_size physical_to_logical_size(const physical_size& size) const noexcept {
            return {
                physical_to_logical_x(size.width),
                physical_to_logical_y(size.height)
            };
        }

        /**
         * @brief Convert physical point to logical point
         * @param point Physical point (strong type)
         * @return Logical point
         */
        [[nodiscard]] constexpr logical_point physical_to_logical_point(const physical_point& point) const noexcept {
            return {
                physical_to_logical_x(point.x),
                physical_to_logical_y(point.y)
            };
        }

        /**
         * @brief Convert physical rect to logical rect
         * @param rect Physical rect (strong type)
         * @return Logical rect
         */
        [[nodiscard]] constexpr logical_rect physical_to_logical_rect(const physical_rect& rect) const noexcept {
            return {
                physical_to_logical_x(rect.x),
                physical_to_logical_y(rect.y),
                physical_to_logical_x(rect.width),
                physical_to_logical_y(rect.height)
            };
        }

        // ====================================================================
        // Overloads accepting backend types (for convenience/interop)
        // ====================================================================

        /**
         * @brief Convert backend size type to logical size
         * @param size Backend-specific size type
         * @return Logical size
         */
        [[nodiscard]] constexpr logical_size physical_to_logical_size(const typename Backend::size_type& size) const noexcept {
            return {
                physical_to_logical_x(physical_x(size_utils::get_width(size))),
                physical_to_logical_y(physical_y(size_utils::get_height(size)))
            };
        }

        /**
         * @brief Convert backend point type to logical point
         * @param point Backend-specific point type
         * @return Logical point
         */
        [[nodiscard]] constexpr logical_point physical_to_logical_point(const typename Backend::point_type& point) const noexcept {
            return {
                physical_to_logical_x(physical_x(point_utils::get_x(point))),
                physical_to_logical_y(physical_y(point_utils::get_y(point)))
            };
        }

        /**
         * @brief Convert backend rect type to logical rect
         * @param rect Backend-specific rect type
         * @return Logical rect
         */
        [[nodiscard]] constexpr logical_rect physical_to_logical_rect(const typename Backend::rect_type& rect) const noexcept {
            return {
                physical_to_logical_x(physical_x(rect_utils::get_x(rect))),
                physical_to_logical_y(physical_y(rect_utils::get_y(rect))),
                physical_to_logical_x(physical_x(rect_utils::get_width(rect))),
                physical_to_logical_y(physical_y(rect_utils::get_height(rect)))
            };
        }
    };

    /**
     * @brief Create terminal backend metrics (1:1 char cell mapping)
     * @return Backend metrics for terminal/console
     */
    template<UIBackend Backend>
    [[nodiscard]] constexpr backend_metrics<Backend> make_terminal_metrics() noexcept {
        return {
            .logical_to_physical_x = 1.0,   // 1 logical unit = 1 char column
            .logical_to_physical_y = 1.0,   // 1 logical unit = 1 char row
            .dpi_scale = 1.0                // No DPI scaling for terminal
        };
    }

    /**
     * @brief Create SDL/GUI backend metrics (8 pixels per logical unit)
     * @param dpi_scale DPI multiplier (1.0, 1.5, 2.0, 3.0)
     * @return Backend metrics for SDL/pixel-based rendering
     */
    template<UIBackend Backend>
    [[nodiscard]] constexpr backend_metrics<Backend> make_gui_metrics(double dpi_scale = 1.0) noexcept {
        return {
            .logical_to_physical_x = 8.0,   // 1 logical unit = 8 pixels
            .logical_to_physical_y = 8.0,   // 1 logical unit = 8 pixels
            .dpi_scale = dpi_scale          // DPI scaling (1x, 1.5x, 2x, etc.)
        };
    }

    /**
     * @brief Convert theme padding/spacing to physical pixels (X-axis)
     * @tparam Backend The UI backend type
     * @param value Theme value (small integer like 1, 2)
     * @param metrics Pointer to backend metrics (optional)
     * @return Physical X coordinate (strong type)
     *
     * @details
     * Use this in do_render() to convert theme padding/spacing values
     * before combining with measure_text() results (which are in pixels).
     *
     * Theme padding values are in logical units. For terminal backends, 1 logical
     * unit = 1 character cell. For graphical backends, 1 logical unit = N pixels
     * (typically 8). This function converts to physical pixels using the metrics.
     *
     * When metrics are unavailable, runtime detection estimates the scale factor
     * by measuring a reference character.
     */
    template<UIBackend Backend>
    [[nodiscard]] inline physical_x to_physical_x(int value, backend_metrics<Backend> const* metrics = nullptr) noexcept {
        if (metrics) {
            return metrics->snap_to_physical_x(logical_unit(value), snap_mode::round);
        }
        // Runtime detection: measure reference character to estimate scale factor
        typename Backend::renderer_type::font default_font{};
        auto ref_size = Backend::renderer_type::measure_text("X", default_font);
        int const ref_width = size_utils::get_width(ref_size);
        // Use the reference character width as the scale factor
        // For terminal: ref_width = 1, so no scaling
        // For graphical: ref_width = ~8, so value * 8
        return physical_x(value * ref_width);
    }

    /**
     * @brief Convert theme padding/spacing to physical pixels (Y-axis)
     * @tparam Backend The UI backend type
     * @param value Theme value (small integer like 1, 2)
     * @param metrics Pointer to backend metrics (optional)
     * @return Physical Y coordinate (strong type)
     *
     * @details
     * Use this in do_render() to convert theme padding/spacing values
     * before combining with measure_text() results (which are in pixels).
     *
     * Theme padding values are in logical units. For terminal backends, 1 logical
     * unit = 1 character cell. For graphical backends, 1 logical unit = N pixels
     * (typically 8). This function converts to physical pixels using the metrics.
     *
     * When metrics are unavailable, runtime detection estimates the scale factor
     * by measuring a reference character. Note: we use character WIDTH (not height)
     * for Y-axis conversion too, to keep padding uniform (square) regardless of
     * font aspect ratio.
     */
    template<UIBackend Backend>
    [[nodiscard]] inline physical_y to_physical_y(int value, backend_metrics<Backend> const* metrics = nullptr) noexcept {
        if (metrics) {
            return metrics->snap_to_physical_y(logical_unit(value), snap_mode::round);
        }
        // Runtime detection: measure reference character to estimate scale factor
        // Use WIDTH for both X and Y to keep padding uniform (square)
        typename Backend::renderer_type::font default_font{};
        auto ref_size = Backend::renderer_type::measure_text("X", default_font);
        int const ref_width = size_utils::get_width(ref_size);
        // For terminal: ref_width = 1, so no scaling
        // For graphical: ref_width = ~8, so value * 8
        return physical_y(value * ref_width);
    }

    /**
     * @brief Convert physical pixels to logical units (Y-axis)
     * @tparam Backend The UI backend type
     * @param py Physical Y coordinate (strong type)
     * @param metrics Pointer to backend metrics (optional)
     * @return Value in logical units (rounded up to avoid clipping)
     *
     * @details
     * Use this in calculate_content_area() and similar methods when you need
     * to convert a physical measurement (like computed tab bar height) back
     * to logical units for layout calculations.
     *
     * When metrics are unavailable, runtime detection estimates the scale factor
     * by measuring a reference character. We use character WIDTH for both X and Y
     * to keep conversions consistent with to_physical_y().
     */
    template<UIBackend Backend>
    [[nodiscard]] inline int to_logical_y(physical_y py, backend_metrics<Backend> const* metrics = nullptr) noexcept {
        if (metrics) {
            return static_cast<int>(std::ceil(
                metrics->physical_to_logical_y(py).value));
        }
        // Runtime detection: measure reference character to estimate scale factor
        // Use WIDTH for both X and Y to keep conversions symmetric with to_physical_y()
        typename Backend::renderer_type::font default_font{};
        auto ref_size = Backend::renderer_type::measure_text("X", default_font);
        int const ref_width = std::max(1, size_utils::get_width(ref_size));
        // For terminal: ref_width = 1, so value / 1 = value (no change)
        // For graphical: ref_width = ~8, so value / 8 (e.g., 16 pixels -> 2 logical)
        // Round up to avoid clipping
        return (py.value + ref_width - 1) / ref_width;
    }

} // namespace onyxui
