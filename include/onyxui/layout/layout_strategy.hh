/**
 * @file layout_strategy.hh
 * @brief Layout strategy interface and related enums for UI element positioning
 * @author igor
 * @date 08/10/2025
 *
 * @details
 * This file defines the abstract base class for layout strategies and all
 * supporting types (alignment, size policies, constraints). Layout strategies
 * implement the Strategy pattern to provide pluggable layout algorithms.
 *
 * ## Architecture Overview
 *
 * The layout system uses a two-phase algorithm inspired by modern UI frameworks:
 * 1. **Measure Phase** (bottom-up): Calculate desired sizes
 * 2. **Arrange Phase** (top-down): Assign final positions
 *
 * This separation ensures efficient layout calculation with minimal redundant work.
 *
 * ## Core Components
 *
 * - **layout_strategy**: Abstract base class for all layout algorithms
 * - **size_constraint**: Defines how an element sizes itself
 * - **size_policy**: Enumeration of sizing behaviors
 * - **Alignment enums**: Control positioning within allocated space
 *
 * ## Design Patterns
 *
 * - **Strategy Pattern**: Pluggable layout algorithms
 * - **Template Method**: Two-phase layout protocol
 * - **Friend Class**: Controlled access to ui_element internals
 *
 * ## Layout Composition (Nested Layouts)
 *
 * One of the most powerful features of this system is the ability to compose
 * layouts hierarchically. Each ui_element can have its own layout strategy,
 * enabling complex UI structures through simple building blocks.
 *
 * ### Why Composition?
 * - **Modularity**: Break complex layouts into manageable pieces
 * - **Reusability**: Create reusable components with internal layouts
 * - **Flexibility**: Mix different layout strategies at each level
 * - **Clarity**: Each container focuses on one layout responsibility
 *
 * ### Common Composition Patterns
 *
 * #### 1. Dashboard Layout (Vertical → Horizontal)
 * ```cpp
 * // Root: Vertical layout for rows (using Backend pattern)
 * auto dashboard = std::make_unique<ui_element<MyBackend>>(nullptr);
 * dashboard->set_layout_strategy(
 *     std::make_unique<linear_layout<MyBackend>>(direction::vertical, 10));
 *
 * // Metrics row: Horizontal layout for cards
 * auto metrics_row = std::make_unique<ui_element<MyBackend>>(nullptr);
 * metrics_row->set_layout_strategy(
 *     std::make_unique<linear_layout<MyBackend>>(direction::horizontal, 15));
 * metrics_row->add_child(create_metric_card("Users", "1,234"));
 * metrics_row->add_child(create_metric_card("Revenue", "$5,678"));
 * metrics_row->add_child(create_metric_card("Growth", "+12%"));
 * dashboard->add_child(std::move(metrics_row));
 *
 * // Charts grid: Grid layout for visualizations
 * auto charts = std::make_unique<ui_element<MyBackend>>(nullptr);
 * charts->set_layout_strategy(
 *     std::make_unique<grid_layout<MyBackend>>(2, 2));  // 2x2 grid
 * charts->add_child(create_line_chart());
 * charts->add_child(create_bar_chart());
 * charts->add_child(create_pie_chart());
 * charts->add_child(create_area_chart());
 * dashboard->add_child(std::move(charts));
 * ```
 *
 * #### 2. Application Window (Anchor → Linear)
 * ```cpp
 * // Root: Anchor layout for major regions
 * auto window = std::make_unique<ui_element<Rect, Size>>(nullptr);
 * auto anchor_strategy = std::make_unique<anchor_layout<Rect, Size>>();
 *
 * // Toolbar: Horizontal linear layout for buttons
 * auto toolbar = std::make_unique<ui_element<Rect, Size>>(nullptr);
 * toolbar->set_layout_strategy(
 *     std::make_unique<linear_layout<Rect, Size>>(direction::horizontal, 5));
 * toolbar->set_padding({8, 8, 8, 8});
 * toolbar->add_child(create_icon_button("new"));
 * toolbar->add_child(create_icon_button("open"));
 * toolbar->add_child(create_icon_button("save"));
 * anchor_strategy->set_anchor(toolbar.get(), anchor_point::top_center);
 * window->add_child(std::move(toolbar));
 *
 * // Sidebar: Vertical linear layout for menu items
 * auto sidebar = std::make_unique<ui_element<Rect, Size>>(nullptr);
 * sidebar->set_layout_strategy(
 *     std::make_unique<linear_layout<Rect, Size>>(direction::vertical, 2));
 * sidebar->set_width_constraint({size_policy::fixed, 200, 200});
 * sidebar->add_child(create_menu_item("File"));
 * sidebar->add_child(create_menu_item("Edit"));
 * sidebar->add_child(create_menu_item("View"));
 * anchor_strategy->set_anchor(sidebar.get(), anchor_point::center_left);
 * window->add_child(std::move(sidebar));
 *
 * window->set_layout_strategy(std::move(anchor_strategy));
 * ```
 *
 * #### 3. Form Layout (Vertical → Horizontal)
 * ```cpp
 * // Root: Vertical layout for form rows
 * auto form = std::make_unique<ui_element<Rect, Size>>(nullptr);
 * form->set_layout_strategy(
 *     std::make_unique<linear_layout<Rect, Size>>(direction::vertical, 12));
 *
 * // Each row: Horizontal layout for label + input
 * auto create_form_row = [](const char* label) {
 *     auto row = std::make_unique<ui_element<Rect, Size>>(nullptr);
 *     row->set_layout_strategy(
 *         std::make_unique<linear_layout<Rect, Size>>(direction::horizontal, 8));
 *
 *     auto label_elem = create_label(label);
 *     label_elem->set_width_constraint({size_policy::fixed, 120, 120});
 *     label_elem->set_vertical_align(vertical_alignment::center);
 *     row->add_child(std::move(label_elem));
 *
 *     auto input = create_text_input();
 *     input->set_width_constraint({size_policy::expand});
 *     row->add_child(std::move(input));
 *
 *     return row;
 * };
 *
 * form->add_child(create_form_row("Username:"));
 * form->add_child(create_form_row("Email:"));
 * form->add_child(create_form_row("Password:"));
 * ```
 *
 * #### 4. Complex Card (Grid → Vertical → Horizontal)
 * ```cpp
 * // Root: Grid for card layout
 * auto card = std::make_unique<ui_element<Rect, Size>>(nullptr);
 * auto grid_strategy = std::make_unique<grid_layout<Rect, Size>>(2, 3);
 *
 * // Header with icon and title (horizontal)
 * auto header = std::make_unique<ui_element<Rect, Size>>(nullptr);
 * header->set_layout_strategy(
 *     std::make_unique<linear_layout<Rect, Size>>(direction::horizontal, 8));
 * header->add_child(create_icon("card"));
 * header->add_child(create_title("Card Title"));
 * grid_strategy->set_cell(header.get(), 0, 0, 2, 1);  // Span 2 columns
 * card->add_child(std::move(header));
 *
 * // Content sections (vertical lists)
 * auto section1 = std::make_unique<ui_element<Rect, Size>>(nullptr);
 * section1->set_layout_strategy(
 *     std::make_unique<linear_layout<Rect, Size>>(direction::vertical, 4));
 * section1->add_child(create_text("Item 1"));
 * section1->add_child(create_text("Item 2"));
 * section1->add_child(create_text("Item 3"));
 * grid_strategy->set_cell(section1.get(), 0, 1);
 * card->add_child(std::move(section1));
 *
 * card->set_layout_strategy(std::move(grid_strategy));
 * ```
 *
 * ### Best Practices for Composition
 *
 * 1. **Single Responsibility**: Each container should handle one layout concern
 * 2. **Avoid Deep Nesting**: More than 3-4 levels can impact performance and clarity
 * 3. **Use Appropriate Strategy**: Pick the simplest layout that solves the problem
 * 4. **Set Constraints Early**: Define size constraints before adding to parent
 * 5. **Consider Reusability**: Extract common patterns into factory functions
 * 6. **Test Incrementally**: Build and test each level before adding more complexity
 *
 * ### Performance Impact
 *
 * - **Measure Phase**: O(n) where n = total elements in tree (each measured once)
 * - **Arrange Phase**: O(n) where n = total elements in tree (each arranged once)
 * - **Caching**: Nested layouts benefit from caching at every level
 * - **Invalidation**: Changes only propagate through affected branches
 *
 * ## Performance Considerations
 *
 * - Layout calculations are cached when possible
 * - Invalidation propagates only when necessary
 * - Constraints are evaluated once per measure phase
 * - Arrangement is skipped for unchanged bounds
 *
 * @see ui_element The primary consumer of layout strategies
 * @see linear_layout Most commonly used concrete implementation
 * @see grid_layout For tabular arrangements
 */

#pragma once

#include <algorithm>
#include <cassert>
#include <cmath>
#include <concepts>
#include <cstdint>
#include <numeric>
#include <onyxui/core/types.hh>
#include <onyxui/core/geometry.hh>
#include <onyxui/concepts/backend.hh>


namespace onyxui {
    /**
     * @enum horizontal_alignment
     * @brief Horizontal alignment options for UI elements
     *
     * @details
     * Controls how an element is positioned horizontally within its allocated space
     * when the element's width is less than the available width. This enum works
     * in conjunction with the layout strategy to determine final positioning.
     *
     * ## Behavior by Alignment Type
     *
     * - **left**: Element is positioned at the start (left edge) of allocated space
     * - **center**: Element is centered with equal space on both sides
     * - **right**: Element is positioned at the end (right edge) of allocated space
     * - **stretch**: Element expands to fill the entire allocated width
     *
     * ## Interaction with Size Policies
     *
     * - With `size_policy::fixed`: Alignment applies when fixed size < allocated space
     * - With `size_policy::expand`: Typically fills space, alignment rarely applies
     * - With `size_policy::content`: Alignment applies when content < allocated space
     *
     * @note Stretch alignment may override the element's preferred width but
     *       respects min/max constraints defined in size_constraint.
     *
     * @see vertical_alignment For vertical positioning options
     * @see size_constraint For width constraints that interact with alignment
     */
    enum class horizontal_alignment : std::uint8_t {
        left, ///< Align to the left edge of allocated space
        center, ///< Center horizontally within allocated space
        right, ///< Align to the right edge of allocated space
        stretch ///< Stretch to fill the entire allocated width
    };

    /**
     * @enum vertical_alignment
     * @brief Vertical alignment options for UI elements
     *
     * @details
     * Controls how an element is positioned vertically within its allocated space
     * when the element's height is less than the available height. Works with
     * layout strategies to determine final vertical positioning.
     *
     * ## Behavior by Alignment Type
     *
     * - **top**: Element is positioned at the start (top edge) of allocated space
     * - **center**: Element is centered with equal space above and below
     * - **bottom**: Element is positioned at the end (bottom edge) of allocated space
     * - **stretch**: Element expands to fill the entire allocated height
     *
     * ## Common Use Cases
     *
     * - Text baselines: Align text elements along their baselines
     * - Icon alignment: Center icons vertically with adjacent text
     * - Form layouts: Top-align labels with their input fields
     * - Flexible containers: Stretch to create equal-height columns
     *
     * @note Stretch alignment may override the element's preferred height but
     *       respects min/max constraints defined in size_constraint.
     *
     * @see horizontal_alignment For horizontal positioning options
     * @see size_constraint For height constraints that interact with alignment
     */
    enum class vertical_alignment : std::uint8_t {
        top, ///< Align to the top edge of allocated space
        center, ///< Center vertically within allocated space
        bottom, ///< Align to the bottom edge of allocated space
        stretch ///< Stretch to fill the entire allocated height
    };

    /**
     * @enum direction
     * @brief Layout flow direction for linear layouts
     *
     * @details
     * Specifies the primary axis along which children are arranged in linear layouts.
     * This determines whether elements stack horizontally (in a row) or vertically
     * (in a column). The perpendicular axis becomes the "cross axis" for alignment.
     *
     * ## Layout Behavior
     *
     * - **horizontal**: Children arranged left-to-right, cross axis is vertical
     * - **vertical**: Children arranged top-to-bottom, cross axis is horizontal
     *
     * ## Cross-Axis Alignment
     *
     * When direction is horizontal:
     * - vertical_alignment controls positioning in cross axis
     * - Children can have different heights
     *
     * When direction is vertical:
     * - horizontal_alignment controls positioning in cross axis
     * - Children can have different widths
     *
     * @note This enum is primarily used by linear_layout but may be referenced
     *       by other layouts for consistency.
     *
     * @see linear_layout The primary consumer of this enum
     */
    enum class direction : std::uint8_t {
        horizontal, ///< Stack elements left-to-right in a row
        vertical ///< Stack elements top-to-bottom in a column
    };

    /**
     * @enum size_policy
     * @brief Sizing behavior policies for UI elements
     *
     * @details
     * Determines how an element calculates its size during the measure phase.
     * Different policies enable flexible layouts similar to CSS flexbox or grid.
     * Each policy defines a specific strategy for size calculation.
     *
     * ## Policy Descriptions
     *
     * ### fixed
     * Uses the exact preferred_size value, ignoring content dimensions.
     * Best for buttons, icons, or elements with known dimensions.
     *
     * ### content
     * Sizes based on the element's natural content (text, children, etc.).
     * This is the default policy, equivalent to CSS "wrap-content".
     *
     * ### expand
     * Grows to fill remaining space after fixed/content elements are placed.
     * Multiple expand children share available space equally.
     * Similar to CSS flex-grow: 1 with all children having equal weight.
     *
     * ### fill_parent
     * Matches the parent's entire content area dimension.
     * Useful for backgrounds or full-width/height containers.
     *
     * ### percentage
     * Takes a specified percentage (0.0-1.0) of the parent's dimension.
     * Allows proportional sizing like "50% of parent width".
     *
     * ### weighted
     * Distributes available space proportionally based on weight values.
     * Like CSS flex-grow with custom weight ratios (1:2:1, etc.).
     *
     * ## Priority and Interaction
     *
     * Layout strategies typically process policies in this order:
     * 1. fixed - Allocate exact sizes first
     * 2. content - Measure and allocate natural sizes
     * 3. percentage - Calculate from parent size
     * 4. expand/weighted - Distribute remaining space
     * 5. fill_parent - Use full parent dimension
     *
     * @note Min/max constraints in size_constraint always apply regardless
     *       of the policy chosen.
     *
     * @see size_constraint For complete sizing configuration including bounds
     */
    enum class size_policy : std::uint8_t {
        fixed, ///< Use preferred_size exactly, ignoring content
        content, ///< Size based on natural content dimensions (default)
        expand, ///< Grow to fill remaining space equally with other expand children
        fill_parent, ///< Match parent's entire content area dimension
        percentage, ///< Take a percentage (0.0-1.0) of parent's dimension
        weighted ///< Proportional distribution based on weight (like flex-grow)
    };

    /**
     * @enum spacing
     * @brief Semantic spacing levels for backend-agnostic layouts
     *
     * @details
     * Type-safe spacing values resolved by theme at layout time.
     * Prevents hardcoded values in application code.
     * Each backend's theme defines the actual logical unit values.
     *
     * ## Purpose
     *
     * The spacing enum allows semantic spacing that scales appropriately
     * across different backends while maintaining visual consistency.
     *
     * Using semantic spacing values allows the same application code to
     * produce visually consistent layouts across different backends.
     *
     * ## Usage
     *
     * ### Container Spacing
     * ```cpp
     * auto container = std::make_unique<vbox<Backend>>(spacing::medium);
     * ```
     *
     * ### Widget Padding/Margins
     * ```cpp
     * widget->set_padding(spacing::small);
     * widget->set_margin(spacing::large);
     * ```
     *
     * ## Theme Resolution
     *
     * At layout time, spacing values are resolved to logical unit values
     * defined in the current theme (e.g., spacing::medium -> theme-defined value).
     *
     * ## Recommended Usage by Context
     *
     * - **none**: No spacing, direct adjacency (borders, separators)
     * - **tiny**: Minimal separation (icon+text, inline elements)
     * - **small**: Tight spacing (dense forms, toolbars)
     * - **medium**: Standard spacing (default for most UIs)
     * - **large**: Loose spacing (visual separation between sections)
     * - **xlarge**: Very loose (major section boundaries, top-level layout)
     *
     * @note The actual logical unit values are defined in each theme's YAML file
     *
     * @see theme::spacing_values For theme-side resolution
     * @see vbox, hbox, grid For container constructors accepting spacing
     */
    enum class spacing : std::uint8_t {
        none   = 0,   ///< No spacing (0)
        tiny   = 1,   ///< Minimal separation
        small  = 2,   ///< Tight spacing (default for dense UIs)
        medium = 3,   ///< Standard spacing (default for most cases)
        large  = 4,   ///< Loose spacing (for visual separation)
        xlarge = 5    ///< Very loose (for major sections)
    };

    /**
     * @brief Approximate equality comparison for floating-point values
     *
     * @tparam T Floating-point type (float or double)
     * @param a First value
     * @param b Second value
     * @param epsilon Tolerance for comparison (default: reasonable for float)
     * @return true if values are approximately equal within tolerance
     *
     * @details
     * Uses relative epsilon comparison which scales with the magnitude of values.
     * This handles both small and large values correctly.
     *
     * Formula: |a - b| <= epsilon * max(|a|, |b|)
     *
     * @note For exact zero comparisons, falls back to absolute epsilon check
     */
    template<std::floating_point T>
    [[nodiscard]] constexpr bool approx_equal(T a, T b, T epsilon = T(0.0001)) noexcept {
        // Handle exact equality (including both zero)
        if (a == b) {
            return true;
        }

        // Relative epsilon comparison
        const T abs_a = std::abs(a);
        const T abs_b = std::abs(b);
        const T diff = std::abs(a - b);

        // For values near zero, use absolute epsilon
        if (abs_a < epsilon || abs_b < epsilon) {
            return diff <= epsilon;
        }

        // For larger values, use relative epsilon
        return diff <= epsilon * std::max(abs_a, abs_b);
    }

    /**
     * @struct size_constraint
     * @brief Complete sizing specification for one dimension (width or height)
     *
     * @details
     * Combines a size policy with bounds and parameters to fully define how
     * an element should be sized in one dimension. Each ui_element has separate
     * constraints for width and height, allowing independent control over each axis.
     *
     * ## Core Concepts
     *
     * A size_constraint encapsulates:
     * - **Policy**: The strategy for calculating size (fixed, content, expand, etc.)
     * - **Bounds**: Minimum and maximum size limits that always apply
     * - **Parameters**: Policy-specific values (preferred size, weight, percentage)
     *
     * ## Size Calculation Flow
     *
     * 1. Initial size is calculated based on the policy
     * 2. Result is clamped to [min_size, max_size]
     * 3. Layout strategy may further adjust based on available space
     * 4. Final size respects all constraints
     *
     * ## Common Patterns
     *
     * ### Fixed Size Button
     * ```cpp
     * size_constraint width;
     * width.policy = size_policy::fixed;
     * width.preferred_size = 120;
     * width.min_size = 80;   // Never smaller than 80px
     * width.max_size = 200;  // Never larger than 200px
     * ```
     *
     * ### Flexible Content
     * ```cpp
     * size_constraint height;
     * height.policy = size_policy::content;
     * height.min_size = 30;   // At least 30px tall
     * height.max_size = 500;  // Cap at 500px
     * ```
     *
     * ### Proportional Column
     * ```cpp
     * size_constraint width;
     * width.policy = size_policy::weighted;
     * width.weight = 2.0f;    // Takes 2x space vs weight=1.0 siblings
     * width.min_size = 100;   // But never less than 100px
     * ```
     *
     * ## Field Usage by Policy
     *
     * - **fixed**: Uses preferred_size, ignores weight/percentage
     * - **content**: Uses natural size, all fields except preferred_size apply
     * - **expand**: Distributes space equally, ignores weight/percentage
     * - **weighted**: Uses weight for proportional distribution
     * - **percentage**: Uses percentage field (0.0-1.0)
     * - **fill_parent**: Matches parent dimension, only min/max apply
     *
     * @note Default values create a content-sized element with no restrictions,
     *       which is appropriate for most use cases.
     *
     * @see size_policy For detailed policy descriptions
     * @see ui_element::set_width_constraint() To apply constraints
     */
    struct size_constraint {
        size_policy policy = size_policy::content; ///< Sizing behavior strategy

        // Core size values
        logical_unit preferred_size = logical_unit(0.0); ///< Desired size for 'fixed' policy
        logical_unit min_size = logical_unit(0.0); ///< Minimum allowed size, always enforced
        logical_unit max_size = logical_unit(std::numeric_limits<double>::max()); ///< Maximum allowed size, always enforced

        // Policy-specific parameters
        float weight = 1.0F; ///< Weight for 'weighted' policy (like CSS flex-grow)
        float percentage = 1.0F; ///< Percentage for 'percentage' policy (0.0 to 1.0)

        /**
         * @brief Check if the constraint is in a valid state
         *
         * @return true if min_size <= max_size, false otherwise
         *
         * @details
         * A constraint is valid when the minimum size does not exceed the maximum.
         * Invalid constraints can lead to undefined behavior in layout calculations.
         * Use this method to validate constraints before applying them.
         *
         * @note Exception safety: No-throw guarantee (noexcept)
         *
         * @example Validation Example
         * @code
         * size_constraint c;
         * c.min_size = 100;
         * c.max_size = 50;
         * if (!c.is_valid()) {
         *     // Handle error: min_size > max_size
         * }
         * @endcode
         */
        [[nodiscard]] constexpr bool is_valid() const noexcept {
            return min_size <= max_size;
        }

        /**
         * @brief Set min and max size with auto-correction
         *
         * @param min The minimum size (logical units)
         * @param max The maximum size (logical units)
         *
         * @details
         * Sets both min_size and max_size atomically. If min > max, the values
         * are automatically swapped to maintain the invariant min_size <= max_size.
         * This auto-correction approach is more robust than assertions and prevents
         * undefined behavior in release builds.
         *
         * @note Auto-Correction: If min > max, values are swapped automatically
         * @note Exception safety: No-throw guarantee (noexcept)
         * @note This method always maintains the invariant: min_size <= max_size
         *
         * @example Safe Range Setting
         * @code
         * size_constraint width;
         * width.set_range(80, 200);  // Min: 80px, Max: 200px
         *
         * // Auto-correction: swaps values if inverted
         * width.set_range(200, 80);  // Becomes: Min: 80px, Max: 200px
         * @endcode
         */
        constexpr void set_range(logical_unit min, logical_unit max) noexcept {
            // Auto-correct: swap if min > max to maintain invariant
            if (min > max) {
                min_size = max;
                max_size = min;
            } else {
                min_size = min;
                max_size = max;
            }
        }

        /**
         * @brief Clamp a size value to the min/max constraints with auto-correction
         *
         * @param value The unconstrained size value to clamp
         * @return The value clamped to [min_size, max_size]
         *
         * @details
         * Ensures any calculated size respects the minimum and maximum bounds.
         * This is called by layout strategies after calculating initial sizes
         * to enforce the constraints. If the constraint is invalid (min > max),
         * the method auto-corrects by using min as both bounds.
         *
         * @note This is a const noexcept function suitable for high-frequency calls
         * @note Auto-Correction: If min_size > max_size, uses min_size as the clamped value
         * @note Exception safety: No-throw guarantee (noexcept)
         *
         * @example
         * @code
         * size_constraint c;
         * c.min_size = 100;
         * c.max_size = 200;
         * int size = c.clamp(150);  // Returns 150
         * int size2 = c.clamp(50);  // Returns 100 (clamped to min)
         * int size3 = c.clamp(300); // Returns 200 (clamped to max)
         * @endcode
         */
        [[nodiscard]] logical_unit clamp(logical_unit value) const noexcept {
            // Auto-correct: if invalid constraint, use min as both bounds
            if (min_size > max_size) {
                return min_size;
            }
            return max(min_size, min(max_size, value));
        }

        /**
         * @brief Compare two size constraints for equality
         *
         * @param other The other constraint to compare against
         * @return true if all fields are approximately equal, false otherwise
         *
         * @details
         * Performs comparison of all fields with epsilon-based comparison for
         * floating-point values (weight and percentage). This is used for change
         * detection and caching validation.
         *
         * ## Float Comparison Behavior
         *
         * The weight and percentage fields now use **epsilon-based comparison**
         * via the approx_equal() function. This handles floating-point rounding
         * errors correctly.
         *
         * **Why Epsilon Comparison:**
         * - Handles rounding errors from floating-point arithmetic
         * - Works correctly for computed values (e.g., 2.0f / 3.0f * 3.0f)
         * - Still works for user-set values (e.g., weight = 2.0f)
         * - Default epsilon (0.0001) is appropriate for typical UI values
         *
         * **Epsilon Value:**
         * - Default: 0.0001 (1/10000)
         * - Relative epsilon: scales with magnitude of values
         * - Handles both small and large values correctly
         *
         * **Examples:**
         * @code
         * size_constraint a, b;
         * a.weight = 2.0f;
         * b.weight = 2.0f;
         * assert(a == b);  // Equal
         *
         * a.weight = 2.0f / 3.0f * 3.0f;  // May have rounding error
         * b.weight = 2.0f;
         * assert(a == b);  // Still equal (within epsilon)
         *
         * a.weight = 2.0f;
         * b.weight = 2.1f;
         * assert(a != b);  // Different (exceeds epsilon)
         * @endcode
         *
         * @note Exception safety: No-throw guarantee (noexcept)
         * @note Integer fields (preferred_size, min_size, max_size) use exact comparison
         */
        bool operator==(const size_constraint& other) const noexcept {
            return policy == other.policy &&
                   preferred_size == other.preferred_size &&
                   min_size == other.min_size &&
                   max_size == other.max_size &&
                   approx_equal(weight, other.weight) &&
                   approx_equal(percentage, other.percentage);
        }
    };

    // Forward declaration
    template<UIBackend Backend>
    class ui_element;

    /**
     * @class layout_strategy
     * @brief Abstract base class for layout algorithms (Strategy pattern)
     *
     * @details
     * Layout strategies implement how children are measured and positioned
     * within a parent element. The two-phase algorithm ensures optimal layout
     * with minimal redundant calculations and clear separation of concerns.
     *
     * ## Two-Phase Layout Algorithm
     *
     * ### Phase 1: Measure (Bottom-Up)
     * - Starts at leaf nodes and propagates up
     * - Each element calculates its desired size
     * - Parents aggregate child sizes according to their strategy
     * - Results are cached to avoid redundant calculations
     *
     * ### Phase 2: Arrange (Top-Down)
     * - Starts at root with final allocated bounds
     * - Parents divide space among children per strategy rules
     * - Each child receives its final position and size
     * - Children recursively arrange their own children
     *
     * ## Concrete Implementations
     *
     * - **linear_layout**: Sequential arrangement (row/column)
     * - **grid_layout**: Tabular arrangement with rows and columns
     * - **anchor_layout**: Position at predefined anchor points
     * - **absolute_layout**: Explicit coordinate positioning
     * - **stack_layout**: Overlapping children (z-ordered)
     * - **wrap_layout**: Flow layout with automatic wrapping
     *
     * ## Implementation Guide
     *
     * ### Creating a Custom Layout
     *
     * 1. Inherit from layout_strategy
     * 2. Override measure_children() to calculate total size
     * 3. Override arrange_children() to position children
     * 4. Optionally override cleanup methods for state management
     * 5. Use protected helper methods to access ui_element internals
     *
     * ### Example Custom Layout
     * ```cpp
     * template<RectLike TRect, SizeLike TSize>
     * class circle_layout : public layout_strategy<TRect, TSize> {
     *     TSize measure_children(const ui_element<TRect, TSize>* parent,
     *                           int available_width, int available_height) const override {
     *         // Measure all children and calculate circle diameter
     *         int max_child_dim = 0;
     *         for (const auto& child : get_children(parent)) {
     *             TSize size = child->measure(100, 100);
     *             max_child_dim = std::max(max_child_dim,
     *                 std::max(size_utils::get_width(size),
     *                         size_utils::get_height(size)));
     *         }
     *         int diameter = max_child_dim * 3; // Example calculation
     *         TSize result;
     *         size_utils::set_size(result, diameter, diameter);
     *         return result;
     *     }
     *
     *     void arrange_children(ui_element<TRect, TSize>* parent,
     *                          const TRect& content_area) override {
     *         // Position children in a circle
     *         auto& children = get_mutable_children(parent);
     *         int n = children.size();
     *         for (int i = 0; i < n; ++i) {
     *             double angle = (2 * M_PI * i) / n;
     *             // Calculate position on circle...
     *             // child->arrange(bounds);
     *         }
     *     }
     * };
     * ```
     *
     * ## Performance Considerations
     *
     * - **Caching**: Measure results are cached, implement efficient change detection
     * - **Invalidation**: Only invalidate what changed, avoid full tree traversal
     * - **Allocation**: Avoid dynamic allocations in measure/arrange methods
     * - **Complexity**: Keep O(n) complexity where n = number of children
     *
     * ## Thread Safety
     *
     * Layout strategies are not thread-safe. All layout operations should
     * occur on the UI thread. If implementing stateful strategies, ensure
     * proper synchronization or use immutable state patterns.
     *
     * ## Best Practices
     *
     * - Respect child visibility (skip hidden children)
     * - Honor size constraints (min/max bounds)
     * - Support all size policies where reasonable
     * - Provide sensible defaults for missing configuration
     * - Document strategy-specific behavior clearly
     * - Test with edge cases (0 children, 1 child, many children)
     *
     * @tparam Backend The backend traits type providing rect and size types
     *
     * @see ui_element The primary consumer of layout strategies
     * @see linear_layout Most commonly used concrete implementation
     * @see grid_layout For structured tabular layouts
     */
    template<typename Backend>
    class layout_strategy {
            static_assert(UIBackend<Backend>,
                          "Template parameter must satisfy UIBackend concept for layout strategies");
            using rect_type = typename Backend::rect_type;
            using size_type = typename Backend::size_type;
        public:
            virtual ~layout_strategy() noexcept = default;

            // Delete copy operations (polymorphic base - prevent slicing)
            layout_strategy(const layout_strategy&) = delete;
            layout_strategy& operator=(const layout_strategy&) = delete;

            // Delete move operations (polymorphic base - prevent slicing)
            layout_strategy(layout_strategy&&) = delete;
            layout_strategy& operator=(layout_strategy&&) = delete;

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
             * @param available_width Maximum width available (logical units)
             * @param available_height Maximum height available (logical units)
             * @return The total size needed to accommodate all children
             *
             * @exception Any exception thrown by child->measure()
             * @exception std::bad_alloc If derived implementation allocates memory
             * @note Exception safety: Depends on derived class implementation
             * @note Base class provides no exception guarantees - derived classes should document their behavior
             */
            virtual logical_size measure_children(
                const ui_element<Backend>* parent,
                logical_unit available_width,
                logical_unit available_height) const = 0;

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
             * @param content_area The area available for children (excludes padding/margin) in logical units
             *
             * @exception Any exception thrown by child->arrange()
             * @exception std::bad_alloc If derived implementation allocates memory
             * @note Exception safety: Depends on derived class implementation
             * @note Base class provides no exception guarantees - derived classes should document their behavior
             */
            virtual void arrange_children(
                ui_element<Backend>* parent,
                const logical_rect& content_area) = 0;

            /**
             * @brief Called when a child is removed from the parent
             *
             * Allows layout strategies to clean up any internal state associated
             * with the removed child (e.g., cell mappings, anchor positions).
             * Default implementation does nothing.
             *
             * @param child The child that was removed
             * @note Must not throw - cleanup operations should be noexcept
             */
            virtual void on_child_removed([[maybe_unused]] ui_element<Backend>* child) noexcept {
                // Default: no cleanup needed
            }

            /**
             * @brief Called when all children are removed from the parent
             *
             * Allows layout strategies to clear all internal state.
             * Default implementation does nothing.
             *
             * @note Must not throw - cleanup operations should be noexcept
             */
            virtual void on_children_cleared() noexcept {
                // Default: no cleanup needed
            }

        protected:
            // Only derived classes can construct
            layout_strategy() = default;

            // Protected helpers for accessing ui_element internals
            /**
             * @brief Get const reference to parent's children
             *
             * Protected helper method that derived layout strategies can use to
             * access the parent's children. This method is accessible because
             * layout_strategy is a friend of ui_element.
             *
             * @param parent The parent element
             * @return Const reference to the children vector
             */
            [[nodiscard]] static const auto& get_children(const ui_element<Backend>* parent) noexcept {
                return parent->children();
            }

            /**
             * @brief Get mutable reference to parent's children for arrange phase
             *
             * @param parent The parent element
             * @return Reference to the children vector
             */
            [[nodiscard]] static auto& get_mutable_children(ui_element<Backend>* parent) noexcept {
                return parent->mutable_children();
            }

            /**
             * @brief Get parent's last measured size
             *
             * @param parent The parent element
             * @return The last measured size
             */
            [[nodiscard]] static const logical_size& get_last_measured_size(const ui_element<Backend>* parent) noexcept {
                return parent->last_measured_size();
            }

            /**
             * @brief Get child's horizontal alignment
             *
             * @param child The child element
             * @return Horizontal alignment setting
             */
            [[nodiscard]] static horizontal_alignment get_h_align(const ui_element<Backend>* child) noexcept {
                return child->h_align();
            }

            /**
             * @brief Get child's vertical alignment
             *
             * @param child The child element
             * @return Vertical alignment setting
             */
            [[nodiscard]] static vertical_alignment get_v_align(const ui_element<Backend>* child) noexcept {
                return child->v_align();
            }
    };
}
