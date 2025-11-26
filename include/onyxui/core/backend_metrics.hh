/**
 * @file backend_metrics.hh
 * @brief Backend metrics for converting logical units to physical coordinates
 * @author OnyxUI Framework
 * @date 2025-11-26
 */

#pragma once

#include "geometry.hh"
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
        floor,   ///< Round down (for positions)
        ceil,    ///< Round up (for far edges)
        round,   ///< Round to nearest (for sizes computed directly)
        none     ///< No snapping (preserve fractional for intermediate calculations)
    };

    /**
     * @struct backend_metrics
     * @brief Metrics for converting logical units to physical backend coordinates
     *
     * Encapsulates scaling factors, DPI, aspect ratio, and snapping strategies
     * for a specific backend (terminal, SDL, etc.).
     *
     * @tparam Backend The UI backend type
     */
    template<UIBackend Backend>
    struct backend_metrics {
        // Conversion factors (logical → physical)
        double logical_to_physical_x = 1.0;  ///< X-axis scale (e.g., 1.0 for conio, 8.0 for SDL)
        double logical_to_physical_y = 1.0;  ///< Y-axis scale (e.g., 1.0 for conio, 8.0 for SDL)

        // Aspect ratio compensation (for non-square units like char cells)
        double aspect_ratio = 1.0;  ///< Width/height ratio (e.g., 0.5 for 8×16 chars, 1.0 for square pixels)

        // DPI scaling (for high-DPI displays)
        double dpi_scale = 1.0;  ///< DPI multiplier (1.0, 1.5, 2.0, 3.0, etc.)

        /**
         * @brief Snap logical X coordinate to physical coordinate
         * @param lu Logical unit value
         * @param mode Snapping mode
         * @return Physical coordinate (int)
         */
        [[nodiscard]] constexpr int snap_to_physical_x(logical_unit lu, snap_mode mode) const noexcept {
            const double physical = lu.value * logical_to_physical_x * dpi_scale;
            switch (mode) {
                case snap_mode::floor: return static_cast<int>(std::floor(physical));
                case snap_mode::ceil:  return static_cast<int>(std::ceil(physical));
                case snap_mode::round: return static_cast<int>(std::round(physical));
                case snap_mode::none:  return static_cast<int>(physical);
            }
            return static_cast<int>(physical);
        }

        /**
         * @brief Snap logical Y coordinate to physical coordinate
         * @param lu Logical unit value
         * @param mode Snapping mode
         * @return Physical coordinate (int)
         */
        [[nodiscard]] constexpr int snap_to_physical_y(logical_unit lu, snap_mode mode) const noexcept {
            const double physical = lu.value * logical_to_physical_y * dpi_scale;
            switch (mode) {
                case snap_mode::floor: return static_cast<int>(std::floor(physical));
                case snap_mode::ceil:  return static_cast<int>(std::ceil(physical));
                case snap_mode::round: return static_cast<int>(std::round(physical));
                case snap_mode::none:  return static_cast<int>(physical);
            }
            return static_cast<int>(physical);
        }

        /**
         * @brief Convert logical size to physical size
         * @param size Logical size
         * @return Physical size
         *
         * @note Uses round() for direct size conversion
         */
        [[nodiscard]] constexpr typename Backend::size_type snap_size(const logical_size& size) const noexcept {
            typename Backend::size_type result{};
            const int w = snap_to_physical_x(size.width, snap_mode::round);
            const int h = snap_to_physical_y(size.height, snap_mode::round);
            size_utils::set_size(result, w, h);
            return result;
        }

        /**
         * @brief Convert logical point to physical point
         * @param point Logical point
         * @return Physical point
         *
         * @note Uses floor() for position snapping
         */
        [[nodiscard]] constexpr typename Backend::point_type snap_point(const logical_point& point) const noexcept {
            const int x = snap_to_physical_x(point.x, snap_mode::floor);
            const int y = snap_to_physical_y(point.y, snap_mode::floor);
            return {x, y};  // Aggregate initialization
        }

        /**
         * @brief Convert logical rect to physical rect (edge-based snapping)
         * @param rect Logical rect
         * @return Physical rect
         *
         * Snapping strategy:
         * 1. Floor position (left, top)
         * 2. Ceil far edges (right, bottom)
         * 3. Compute size from edges (avoids accumulation errors)
         *
         * This prevents gaps and overlaps in tiled layouts.
         */
        [[nodiscard]] constexpr typename Backend::rect_type snap_rect(const logical_rect& rect) const noexcept {
            // Snap edges
            const int px_left   = snap_to_physical_x(rect.left(), snap_mode::floor);
            const int px_top    = snap_to_physical_y(rect.top(), snap_mode::floor);
            const int px_right  = snap_to_physical_x(rect.right(), snap_mode::ceil);
            const int px_bottom = snap_to_physical_y(rect.bottom(), snap_mode::ceil);

            // Compute size from edges (not from logical size directly)
            const int px_width  = px_right - px_left;
            const int px_height = px_bottom - px_top;

            typename Backend::rect_type result{};
            rect_utils::set_bounds(result, px_left, px_top, px_width, px_height);
            return result;
        }

        /**
         * @brief Convert physical coordinate to logical X coordinate
         * @param physical Physical coordinate
         * @return Logical unit
         */
        [[nodiscard]] constexpr logical_unit physical_to_logical_x(int physical) const noexcept {
            return logical_unit(static_cast<double>(physical) / (logical_to_physical_x * dpi_scale));
        }

        /**
         * @brief Convert physical coordinate to logical Y coordinate
         * @param physical Physical coordinate
         * @return Logical unit
         */
        [[nodiscard]] constexpr logical_unit physical_to_logical_y(int physical) const noexcept {
            return logical_unit(static_cast<double>(physical) / (logical_to_physical_y * dpi_scale));
        }

        /**
         * @brief Convert physical size to logical size
         * @param size Physical size
         * @return Logical size
         */
        [[nodiscard]] constexpr logical_size physical_to_logical_size(const typename Backend::size_type& size) const noexcept {
            return {
                physical_to_logical_x(size_utils::get_width(size)),
                physical_to_logical_y(size_utils::get_height(size))
            };
        }

        /**
         * @brief Convert physical point to logical point
         * @param point Physical point
         * @return Logical point
         */
        [[nodiscard]] constexpr logical_point physical_to_logical_point(const typename Backend::point_type& point) const noexcept {
            return {
                physical_to_logical_x(point_utils::get_x(point)),
                physical_to_logical_y(point_utils::get_y(point))
            };
        }

        /**
         * @brief Convert physical rect to logical rect
         * @param rect Physical rect
         * @return Logical rect
         */
        [[nodiscard]] constexpr logical_rect physical_to_logical_rect(const typename Backend::rect_type& rect) const noexcept {
            return {
                physical_to_logical_x(rect_utils::get_x(rect)),
                physical_to_logical_y(rect_utils::get_y(rect)),
                physical_to_logical_x(rect_utils::get_width(rect)),
                physical_to_logical_y(rect_utils::get_height(rect))
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
            .aspect_ratio = 0.5,            // Char cells ~8×16 (width/height)
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
            .aspect_ratio = 1.0,            // Square pixels
            .dpi_scale = dpi_scale          // DPI scaling (1x, 1.5x, 2x, etc.)
        };
    }

} // namespace onyxui
