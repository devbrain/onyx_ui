/**
 * @file layout_strategy.hh
 * @brief Layout strategy interface and related enums for UI element positioning
 * @author igor
 * @date 08/10/2025
 *
 * This file defines the abstract base class for layout strategies and all
 * supporting types (alignment, size policies, constraints). Layout strategies
 * implement the Strategy pattern to provide pluggable layout algorithms.
 */

#pragma once

#include <algorithm>
#include <numeric>
#include <onyxui/concepts.hh>

namespace onyxui {
    /**
     * @enum horizontal_alignment
     * @brief Horizontal alignment options for UI elements
     *
     * Controls how an element is positioned horizontally within its allocated space.
     */
    enum class horizontal_alignment {
        left,     ///< Align to the left edge
        center,   ///< Center horizontally
        right,    ///< Align to the right edge
        stretch   ///< Stretch to fill the entire width
    };

    /**
     * @enum vertical_alignment
     * @brief Vertical alignment options for UI elements
     *
     * Controls how an element is positioned vertically within its allocated space.
     */
    enum class vertical_alignment {
        top,      ///< Align to the top edge
        center,   ///< Center vertically
        bottom,   ///< Align to the bottom edge
        stretch   ///< Stretch to fill the entire height
    };

    /**
     * @enum direction
     * @brief Layout flow direction for linear layouts
     */
    enum class direction {
        horizontal,  ///< Stack elements left-to-right
        vertical     ///< Stack elements top-to-bottom
    };

    /**
     * @enum size_policy
     * @brief Sizing behavior policies for UI elements
     *
     * Determines how an element calculates its size during the measure phase.
     * Different policies enable flexible layouts similar to CSS flexbox or grid.
     */
    enum class size_policy {
        fixed,        ///< Use preferred_size exactly (ignore content)
        content,      ///< Size based on content (wrap content, default)
        expand,       ///< Grow to fill remaining available space equally
        fill_parent,  ///< Match parent's content area completely
        percentage,   ///< Take a percentage of parent's space
        weighted      ///< Proportional distribution based on weight (like flex-grow)
    };

    /**
     * @struct size_constraint
     * @brief Complete sizing specification for one dimension (width or height)
     *
     * Combines a size policy with bounds and parameters. Each ui_element has
     * separate constraints for width and height.
     *
     * @example
     * @code
     * size_constraint width;
     * width.policy = size_policy::fixed;
     * width.preferred_size = 200;
     * width.min_size = 100;
     * width.max_size = 400;
     * @endcode
     */
    struct size_constraint {
        size_policy policy = size_policy::content;  ///< Sizing behavior

        // Core size values
        int preferred_size = 0;  ///< Desired size for 'fixed' policy (pixels)
        int min_size = 0;        ///< Minimum allowed size (pixels)
        int max_size = std::numeric_limits<int>::max();  ///< Maximum allowed size (pixels)

        // For weighted/percentage policies
        float weight = 1.0f;      ///< Weight for proportional distribution (flex-grow style)
        float percentage = 1.0f;  ///< Percentage of parent (0.0 to 1.0)

        /**
         * @brief Clamp a size value to the min/max constraints
         * @param value The unconstrained size value
         * @return The value clamped to [min_size, max_size]
         */
        [[nodiscard]] int clamp(int value) const {
            return std::max(min_size, std::min(max_size, value));
        }

        /**
         * @brief Compare two size constraints for equality
         * @param other The other constraint to compare
         * @return true if all fields are equal
         */
        bool operator==(const size_constraint& other) const {
            return policy == other.policy &&
                   preferred_size == other.preferred_size &&
                   min_size == other.min_size &&
                   max_size == other.max_size &&
                   weight == other.weight &&
                   percentage == other.percentage;
        }
    };

    // Forward declaration
    template<RectLike TRect, SizeLike TSize>
    class ui_element;

    /**
     * @class layout_strategy
     * @brief Abstract base class for layout algorithms (Strategy pattern)
     *
     * Layout strategies implement how children are measured and positioned
     * within a parent element. The two-phase algorithm ensures optimal layout:
     *
     * 1. **Measure Phase**: Calculate desired sizes bottom-up
     * 2. **Arrange Phase**: Assign final positions top-down
     *
     * Concrete implementations include linear_layout, grid_layout, anchor_layout,
     * and absolute_layout.
     *
     * @tparam TRect Rectangle type satisfying RectLike concept
     * @tparam TSize Size type satisfying SizeLike concept
     *
     * @example
     * @code
     * // Create a vertical linear layout
     * auto strategy = std::make_unique<linear_layout<SDL_Rect, SDL_Size>>();
     * strategy->layout_direction = direction::vertical;
     * strategy->spacing = 10;
     * element->set_layout_strategy(std::move(strategy));
     * @endcode
     */
    template<RectLike TRect, SizeLike TSize>
    class layout_strategy {
        public:
            virtual ~layout_strategy() = default;

            /**
             * @brief Measure phase: calculate the total size needed for all children
             *
             * Called during the bottom-up measure pass. Implementations should:
             * 1. Call measure() on each child with appropriate available space
             * 2. Combine child sizes according to layout algorithm
             * 3. Return the total size needed
             *
             * Results are cached in the parent element, so this may not be called
             * every frame if nothing has changed.
             *
             * @param parent The parent element whose children to measure
             * @param available_width Maximum width available (pixels)
             * @param available_height Maximum height available (pixels)
             * @return The total size needed to accommodate all children
             */
            virtual TSize measure_children(
                ui_element <TRect, TSize>* parent,
                int available_width,
                int available_height) = 0;

            /**
             * @brief Arrange phase: assign final positions to all children
             *
             * Called during the top-down arrange pass. Implementations should:
             * 1. Calculate each child's final bounds within content_area
             * 2. Call arrange() on each child with those bounds
             *
             * This is where alignment, spacing, and size policies are applied
             * to determine the exact position and size of each child.
             *
             * @param parent The parent element whose children to arrange
             * @param content_area The area available for children (excludes padding/margin)
             */
            virtual void arrange_children(
                ui_element <TRect, TSize>* parent,
                const TRect& content_area) = 0;
    };
}
