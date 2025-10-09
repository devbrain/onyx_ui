//
// Created by igor on 08/10/2025.
//

#pragma once

#include <algorithm>
#include <numeric>
#include <onyxui/concepts.hh>

namespace onyxui {
    enum class horizontal_alignment {
        left,
        center,
        right,
        stretch
    };

    enum class vertical_alignment {
        top,
        center,
        bottom,
        stretch
    };

    enum class direction {
        horizontal,
        vertical
    };

    enum class size_policy {
        fixed, // Use preferred_size exactly
        content, // Size based on content (wrap)
        expand, // Grow to fill available space
        fill_parent, // Match parent's content area
        percentage, // Percentage of parent
        weighted // Proportional distribution (flex-grow style)
    };

    struct size_constraint {
        size_policy policy = size_policy::content;

        // Core size values
        int preferred_size = 0; // Desired size for 'fixed' policy
        int min_size = 0; // Minimum allowed size
        int max_size = std::numeric_limits <int>::max(); // Maximum allowed size

        // For weighted/percentage policies
        float weight = 1.0f; // Weight for distribution (flex-grow)
        float percentage = 1.0f; // Percentage of parent (0.0 to 1.0)

        // Clamp a size to constraints
        [[nodiscard]] int clamp(int value) const {
            return std::max(min_size, std::min(max_size, value));
        }

        bool operator==(const size_constraint& other) const {
            return policy == other.policy &&
                   preferred_size == other.preferred_size &&
                   min_size == other.min_size &&
                   max_size == other.max_size &&
                   weight == other.weight &&
                   percentage == other.percentage;
        }
    };

    template<RectLike TRect, SizeLike TSize>
    class ui_element;

    template<RectLike TRect, SizeLike TSize>
    class layout_strategy {
        public:
            virtual ~layout_strategy() = default;

            // Measure phase: determine desired size
            virtual TSize measure_children(
                ui_element <TRect, TSize>* parent,
                int available_width,
                int available_height) = 0;

            // Arrange phase: position children
            virtual void arrange_children(
                ui_element <TRect, TSize>* parent,
                const TRect& content_area) = 0;
    };
}
