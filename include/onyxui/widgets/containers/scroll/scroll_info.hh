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

        size_type content_size;      ///< Total content size (unconstrained measurement)
        size_type viewport_size;     ///< Visible area size (constrained by parent)
        point_type scroll_offset;    ///< Current scroll position (top-left offset)

        /**
         * @brief Check if horizontal scrolling is needed
         * @return true if content width exceeds viewport width
         */
        [[nodiscard]] bool needs_horizontal_scroll() const noexcept {
            int const content_w = size_utils::get_width(content_size);
            int const viewport_w = size_utils::get_width(viewport_size);
            return content_w > viewport_w;
        }

        /**
         * @brief Check if vertical scrolling is needed
         * @return true if content height exceeds viewport height
         */
        [[nodiscard]] bool needs_vertical_scroll() const noexcept {
            int const content_h = size_utils::get_height(content_size);
            int const viewport_h = size_utils::get_height(viewport_size);
            return content_h > viewport_h;
        }

        /**
         * @brief Calculate maximum horizontal scroll offset
         * @return Maximum X scroll value (content_width - viewport_width), or 0 if no scroll needed
         */
        [[nodiscard]] int max_scroll_x() const noexcept {
            int const content_w = size_utils::get_width(content_size);
            int const viewport_w = size_utils::get_width(viewport_size);
            return std::max(0, content_w - viewport_w);
        }

        /**
         * @brief Calculate maximum vertical scroll offset
         * @return Maximum Y scroll value (content_height - viewport_height), or 0 if no scroll needed
         */
        [[nodiscard]] int max_scroll_y() const noexcept {
            int const content_h = size_utils::get_height(content_size);
            int const viewport_h = size_utils::get_height(viewport_size);
            return std::max(0, content_h - viewport_h);
        }

        /**
         * @brief Default constructor - zero initialized
         */
        scroll_info() = default;

        /**
         * @brief Construct with explicit sizes and offset
         * @param content Total content size
         * @param viewport Visible viewport size
         * @param offset Current scroll offset (default: 0, 0)
         */
        scroll_info(size_type content, size_type viewport, point_type offset = {})
            : content_size(content)
            , viewport_size(viewport)
            , scroll_offset(offset)
        {
        }

        /**
         * @brief Comparison operators for testing
         */
        [[nodiscard]] bool operator==(const scroll_info&) const noexcept = default;
        [[nodiscard]] bool operator!=(const scroll_info&) const noexcept = default;
    };

} // namespace onyxui
