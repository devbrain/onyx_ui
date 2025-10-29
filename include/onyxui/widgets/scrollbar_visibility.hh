/**
 * @file scrollbar_visibility.hh
 * @brief Scrollbar visibility policy enumeration
 * @author OnyxUI Framework
 * @date 2025-10-29
 */

#pragma once

#include <cstdint>

namespace onyxui {

    /**
     * @enum scrollbar_visibility
     * @brief Defines when scrollbars should be visible
     *
     * @details
     * Controls the visibility behavior of scrollbars in scrolling containers.
     * This policy determines whether scrollbars are shown, hidden, or
     * automatically managed based on content size.
     *
     * Usage:
     * - `always`: Scrollbar is always visible, even when content fits
     * - `auto_hide`: Scrollbar visible only when content exceeds viewport
     * - `hidden`: Scrollbar is never visible (scrolling via other means)
     *
     * Independent Policies:
     * Horizontal and vertical scrollbars can have different policies:
     * - Horizontal: auto (show when content is wide)
     * - Vertical: always (always show for navigation clarity)
     */
    enum class scrollbar_visibility : std::uint8_t {
        /**
         * @brief Scrollbar always visible regardless of content size
         *
         * Use when:
         * - User should always see scrollbar for navigation awareness
         * - Consistent UI layout is important (no jumping when content changes)
         * - Scrollbar provides visual indication even when disabled
         */
        always,

        /**
         * @brief Scrollbar visible only when content exceeds viewport
         *
         * Use when:
         * - Space efficiency is important
         * - Content size varies frequently
         * - Standard modern UI behavior is desired
         *
         * Behavior:
         * - Hidden when: content_size <= viewport_size
         * - Visible when: content_size > viewport_size
         */
        auto_hide,

        /**
         * @brief Scrollbar never visible
         *
         * Use when:
         * - Scrolling controlled by other means (keyboard, mouse wheel, touch)
         * - Custom scroll indicators are provided
         * - Minimal UI is required
         *
         * Note: Content still scrollable, just no visual scrollbar
         */
        hidden
    };

    /**
     * @struct scrollbar_visibility_policy
     * @brief Visibility policies for horizontal and vertical scrollbars
     *
     * @details
     * Allows independent control of horizontal and vertical scrollbar visibility.
     * This is useful for scenarios like:
     * - Always show vertical scrollbar, auto-hide horizontal
     * - Hide both scrollbars for touch interfaces
     * - Always show both for consistent layout
     */
    struct scrollbar_visibility_policy {
        scrollbar_visibility horizontal = scrollbar_visibility::auto_hide;
        scrollbar_visibility vertical = scrollbar_visibility::auto_hide;

        /**
         * @brief Construct with default auto_hide for both axes
         */
        scrollbar_visibility_policy() = default;

        /**
         * @brief Construct with same policy for both axes
         * @param both Visibility policy for horizontal and vertical
         */
        explicit scrollbar_visibility_policy(scrollbar_visibility both)
            : horizontal(both)
            , vertical(both)
        {
        }

        /**
         * @brief Construct with independent policies
         * @param h Horizontal scrollbar visibility policy
         * @param v Vertical scrollbar visibility policy
         */
        scrollbar_visibility_policy(scrollbar_visibility h, scrollbar_visibility v)
            : horizontal(h)
            , vertical(v)
        {
        }

        /**
         * @brief Comparison operators for testing
         */
        [[nodiscard]] bool operator==(const scrollbar_visibility_policy&) const noexcept = default;
        [[nodiscard]] bool operator!=(const scrollbar_visibility_policy&) const noexcept = default;
    };

} // namespace onyxui
