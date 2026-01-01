/**
 * @file scroll_info.hh
 * @brief Scroll position and sizing information
 * @author OnyxUI Framework
 * @date 2025-10-28
 */

#pragma once

#include <onyxui/concepts/backend.hh>
#include <onyxui/concepts/rect_like.hh>
#include <onyxui/concepts/size_like.hh>
#include <onyxui/concepts/point_like.hh>
#include <algorithm>  // For std::max
#include <cmath>      // For std::floor

namespace onyxui {

    /**
     * @struct scroll_info
     * @brief Encapsulates scroll position, content size, and viewport size
     *
     * @details
     * This structure bundles all information needed to determine:
     * - Whether scrolling is needed (content > viewport)
     * - How far content can scroll (max scroll offset)
     * - Current scroll position
     *
     * **Logical Coordinate System:**
     * All sizes and offsets are stored as double to preserve full logical unit
     * precision. Rounding to pixels only happens at render time.
     *
     * Used by:
     * - scrollable widget (logic container)
     * - scrollbar widget (visual representation)
     * - scroll_controller (coordination)
     *
     * @tparam Backend UIBackend implementation
     */
    template<UIBackend Backend>
    struct scroll_info {
        using size_type = typename Backend::size_type;
        using point_type = typename Backend::point_type;

        double content_width = 0.0;   ///< Total content width (logical units)
        double content_height = 0.0;  ///< Total content height (logical units)
        double viewport_width = 0.0;  ///< Visible viewport width (logical units)
        double viewport_height = 0.0; ///< Visible viewport height (logical units)
        double scroll_x = 0.0;        ///< Current horizontal scroll offset (logical units)
        double scroll_y = 0.0;        ///< Current vertical scroll offset (logical units)

        /**
         * @brief Check if horizontal scrolling is needed
         * @return true if content width exceeds viewport width
         */
        [[nodiscard]] bool needs_horizontal_scroll() const noexcept {
            return content_width > viewport_width;
        }

        /**
         * @brief Check if vertical scrolling is needed
         * @return true if content height exceeds viewport height
         */
        [[nodiscard]] bool needs_vertical_scroll() const noexcept {
            return content_height > viewport_height;
        }

        /**
         * @brief Calculate maximum horizontal scroll offset
         * @return Maximum X scroll value (content_width - viewport_width), or 0 if no scroll needed
         */
        [[nodiscard]] double max_scroll_x() const noexcept {
            return std::max(0.0, content_width - viewport_width);
        }

        /**
         * @brief Calculate maximum vertical scroll offset
         * @return Maximum Y scroll value (content_height - viewport_height), or 0 if no scroll needed
         */
        [[nodiscard]] double max_scroll_y() const noexcept {
            return std::max(0.0, content_height - viewport_height);
        }

        /**
         * @brief Get current scroll X as int (for legacy APIs)
         * @return Scroll X floored to int
         */
        [[nodiscard]] int scroll_x_int() const noexcept {
            return static_cast<int>(std::floor(scroll_x));
        }

        /**
         * @brief Get current scroll Y as int (for legacy APIs)
         * @return Scroll Y floored to int
         */
        [[nodiscard]] int scroll_y_int() const noexcept {
            return static_cast<int>(std::floor(scroll_y));
        }

        /**
         * @brief Get max scroll X as int (for legacy APIs)
         * @return Max scroll X floored to int
         */
        [[nodiscard]] int max_scroll_x_int() const noexcept {
            return static_cast<int>(std::floor(max_scroll_x()));
        }

        /**
         * @brief Get max scroll Y as int (for legacy APIs)
         * @return Max scroll Y floored to int
         */
        [[nodiscard]] int max_scroll_y_int() const noexcept {
            return static_cast<int>(std::floor(max_scroll_y()));
        }

        /**
         * @brief Default constructor - zero initialized
         */
        scroll_info() = default;

        /**
         * @brief Construct with explicit sizes and offset (logical double values)
         * @param cw Content width
         * @param ch Content height
         * @param vw Viewport width
         * @param vh Viewport height
         * @param sx Scroll X offset (default: 0)
         * @param sy Scroll Y offset (default: 0)
         */
        scroll_info(double cw, double ch, double vw, double vh, double sx = 0.0, double sy = 0.0)
            : content_width(cw)
            , content_height(ch)
            , viewport_width(vw)
            , viewport_height(vh)
            , scroll_x(sx)
            , scroll_y(sy)
        {
        }

        /**
         * @brief Construct from Backend types (for backward compatibility)
         * @param content Total content size
         * @param viewport Visible viewport size
         * @param offset Current scroll offset (default: 0, 0)
         */
        scroll_info(size_type content, size_type viewport, point_type offset = {})
            : content_width(static_cast<double>(size_utils::get_width(content)))
            , content_height(static_cast<double>(size_utils::get_height(content)))
            , viewport_width(static_cast<double>(size_utils::get_width(viewport)))
            , viewport_height(static_cast<double>(size_utils::get_height(viewport)))
            , scroll_x(static_cast<double>(point_utils::get_x(offset)))
            , scroll_y(static_cast<double>(point_utils::get_y(offset)))
        {
        }

        /**
         * @brief Comparison operators for testing
         */
        [[nodiscard]] bool operator==(const scroll_info&) const noexcept = default;
        [[nodiscard]] bool operator!=(const scroll_info&) const noexcept = default;
    };

} // namespace onyxui
