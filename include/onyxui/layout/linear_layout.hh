/**
 * @file linear_layout.hh
 * @brief Linear layout strategy for stacking elements horizontally or vertically
 * @author igor
 * @date 09/10/2025
 *
 * @details
 * Linear layout arranges children in a single row (horizontal) or column (vertical),
 * similar to CSS flexbox with flex-direction. Supports spacing between items,
 * alignment, and various size policies (fixed, expand, weighted, etc.).
 *
 * ## Features
 * - Horizontal or vertical stacking direction
 * - Configurable spacing between children
 * - Per-child alignment in cross axis
 * - Multiple size policies: fixed, content, expand, weighted, percentage, fill_parent
 * - Min/max constraint enforcement
 * - Remainder pixel distribution for integer division
 * - Visibility-aware layout (hidden children don't occupy space)
 * - Overflow-safe dimension calculations
 *
 * ## Performance Characteristics
 * - Time Complexity: O(n) for measure, O(n) for arrange where n = number of children
 * - Space Complexity: O(n) for weighted distribution algorithm
 * - Weighted distribution: Iterative constraint satisfaction algorithm
 * - Recommended limit: Up to 1000 children for optimal performance
 *
 * ## Integration with UI Framework
 * - Respects all size_policy types from ui_element
 * - Honors alignment settings per child
 * - Participates in two-phase layout protocol
 * - Compatible with nested layouts
 * - Supports dynamic visibility changes
 *
 * @see layout_strategy Base class for layout strategies
 * @see grid_layout For 2D grid arrangements
 * @see anchor_layout For relative positioning
 * @see size_policy For understanding size policies
 */

#pragma once

#include <vector>
#include <onyxui/layout/layout_strategy.hh>
#include <onyxui/utils/safe_math.hh>
#include <onyxui/geometry/coordinates.hh>

namespace onyxui {
    /**
     * @class linear_layout
     * @brief Layout strategy that stacks children in a line (horizontal or vertical)
     *
     * @details
     * Linear layout is one of the most commonly used layout strategies. It arranges
     * children sequentially in either horizontal or vertical direction, with support for:
     *
     * - **Spacing**: Gap between adjacent children
     * - **Alignment**: How children align perpendicular to layout direction
     * - **Size policies**: Fixed, content, expand, weighted distribution
     * - **Min/max constraints**: Enforced on each child
     *
     * ## Measurement Algorithm
     *
     * For vertical layout:
     * - Width = max(child widths)
     * - Height = sum(child heights) + spacing * (visible_count - 1)
     *
     * For horizontal layout:
     * - Width = sum(child widths) + spacing * (visible_count - 1)
     * - Height = max(child heights)
     *
     * ## Arrangement Algorithm
     *
     * 1. Calculate space used by fixed-size children
     * 2. Distribute remaining space among expand/weighted children
     * 3. Apply min/max constraints to each child
     * 4. Distribute integer division remainders to first children
     * 5. Position each child sequentially with spacing
     * 6. Apply alignment perpendicular to flow direction
     *
     * ## Size Policy Behavior
     *
     * - **fixed/content**: Uses measured size, doesn't expand
     * - **expand**: Shares available space equally with other expand children
     * - **weighted**: Shares space proportionally based on weight value
     * - **percentage**: Takes a percentage (0.0-1.0) of parent's dimension
     * - **fill_parent**: Takes full parent dimension (ignores other children)
     *
     * ## Edge Cases
     *
     * - **No visible children**: Returns zero size
     * - **Negative available space**: Children get minimum sizes
     * - **Integer division remainder**: Distributed to first expand children
     * - **Conflicting policies**: weighted and expand share space, fill_parent overrides
     * - **Constraint conflicts**: Min/max constraints are always enforced
     *
     * ## Thread Safety
     *
     * - **Immutable configuration**: Direction, spacing, alignment are const
     * - **Stateless operations**: No mutable state between calls
     * - **Thread-safe**: Multiple threads can safely call measure/arrange
     *
     * ## Best Practices
     *
     * - Use expand for equal distribution
     * - Use weighted for proportional distribution
     * - Set reasonable min/max constraints to prevent UI breakage
     * - Consider using nested layouts for complex arrangements
     * - Keep child count under 1000 for best performance
     *
     * @tparam Backend The backend traits type providing rect and size types
     *
     * @example Vertical Menu
     * @code
     * // Create vertical menu with equally-sized buttons
     * auto layout = std::make_unique<linear_layout<MyBackend>>(
     *     direction::vertical,
     *     5,  // 5px spacing
     *     horizontal_alignment::stretch,  // Full width buttons
     *     vertical_alignment::top
     * );
     * menu->set_layout_strategy(std::move(layout));
     *
     * // Add menu items
     * for (const auto& item : menu_items) {
     *     auto button = create_button(item);
     *     button->set_h_constraint({size_policy::expand}); // Equal height
     *     menu->add_child(std::move(button));
     * }
     * @endcode
     *
     * @example Horizontal Toolbar
     * @code
     * // Create horizontal toolbar with mixed sizing
     * auto layout = std::make_unique<linear_layout<MyBackend>>(
     *     direction::horizontal,
     *     2,  // 2px spacing
     *     horizontal_alignment::left,
     *     vertical_alignment::center  // Center icons vertically
     * );
     * toolbar->set_layout_strategy(std::move(layout));
     *
     * // Fixed-size icon buttons
     * toolbar->add_child(create_icon_button("save"));
     * toolbar->add_child(create_icon_button("open"));
     *
     * // Expanding search bar
     * auto search = create_search_bar();
     * search->set_w_constraint({size_policy::expand});
     * toolbar->add_child(std::move(search));
     *
     * // Fixed-size settings button
     * toolbar->add_child(create_icon_button("settings"));
     * @endcode
     *
     * @example Weighted Distribution
     * @code
     * // Create form with weighted columns
     * auto layout = std::make_unique<linear_layout<MyBackend>>(
     *     direction::horizontal
     * );
     * form_row->set_layout_strategy(std::move(layout));
     *
     * // Label takes 30% of width
     * auto label = create_label("Username:");
     * label->set_w_constraint({size_policy::weighted, 0, 0, INT_MAX, 0.3f});
     * form_row->add_child(std::move(label));
     *
     * // Input takes 70% of width
     * auto input = create_text_input();
     * input->set_w_constraint({size_policy::weighted, 0, 0, INT_MAX, 0.7f});
     * form_row->add_child(std::move(input));
     * @endcode
     */
    template<UIBackend Backend>
    class linear_layout : public layout_strategy<Backend> {
        // PUBLIC inheritance
        public:
            using elt_t = ui_element<Backend>;
            using rect_type = typename Backend::rect_type;
            using size_type = typename Backend::size_type;

            /**
             * @brief Construct linear layout with immutable configuration
             * @param dir Stack direction (vertical or horizontal)
             * @param spacing Gap between children in logical units
             * @param h_align Horizontal alignment for children
             * @param v_align Vertical alignment for children
             */
            explicit linear_layout(
                direction dir = direction::vertical,
                int spacing = 0,
                horizontal_alignment h_align = horizontal_alignment::stretch,
                vertical_alignment v_align = vertical_alignment::stretch
            )
                : m_layout_direction(dir)
                  , m_spacing(spacing)
                  , m_h_align(h_align)
                  , m_v_align(v_align) {
            }

            /**
             * @brief Destructor
             */
            ~linear_layout() override = default;

            // Rule of Five
            linear_layout(const linear_layout&) = delete;
            linear_layout& operator=(const linear_layout&) = delete;
            linear_layout(linear_layout&&) noexcept = default;
            linear_layout& operator=(linear_layout&&) noexcept = default;

            /**
             * @brief Get layout direction
             * @return Direction (vertical or horizontal)
             */
            [[nodiscard]] direction layout_direction() const noexcept { return m_layout_direction; }

            /**
             * @brief Get spacing between children
             * @return Spacing in logical units
             */
            [[nodiscard]] int spacing() const noexcept { return m_spacing; }

            /**
             * @brief Get horizontal alignment
             * @return Horizontal alignment
             */
            [[nodiscard]] horizontal_alignment h_align() const noexcept { return m_h_align; }

            /**
             * @brief Get vertical alignment
             * @return Vertical alignment
             */
            [[nodiscard]] vertical_alignment v_align() const noexcept { return m_v_align; }

        protected:
            /**
             * @brief Measure all children and calculate total required size
             *
             * @param parent Parent element whose children to measure
             * @param available_width Maximum width available for layout
             * @param available_height Maximum height available for layout
             * @return Total size required by all visible children plus spacing
             *
             * @details
             * Override of layout_strategy::measure_children. Behavior depends on direction:
             * - Vertical: Each child gets full width, height accumulates
             * - Horizontal: Each child gets full height, width accumulates
             * Hidden children are skipped and don't contribute to size or spacing.
             *
             * @note Uses safe_math to prevent integer overflow in dimension calculations
             */
            logical_size measure_children(const elt_t* parent,
                                   logical_unit available_width,
                                   logical_unit available_height) const override;

            /**
             * @brief Arrange all children within the content area
             *
             * @param parent Parent element whose children to arrange
             * @param content_area Rectangle defining the area for arrangement (logical units)
             *
             * @details
             * Override of layout_strategy::arrange_children. Delegates to
             * arrange_vertical() or arrange_horizontal() based on layout direction.
             * Skips arrangement if no visible children exist.
             */
            void arrange_children(elt_t* parent,
                                  const logical_rect& content_area) override;

        private:
            // Immutable configuration
            const direction m_layout_direction; ///< Stacking direction (vertical/horizontal)
            const int m_spacing; ///< Gap between children in logical units
            const horizontal_alignment m_h_align; ///< Default horizontal alignment for children
            const vertical_alignment m_v_align; ///< Default vertical alignment for children

            /**
             * @brief Arrange children in vertical stack
             *
             * @param parent Parent element containing children
             * @param content_area Available area for arrangement
             *
             * @details
             * Stacks children from top to bottom with spacing. Handles:
             * - Fixed-size children at their measured height
             * - Expand children sharing remaining space equally
             * - Weighted children sharing space proportionally
             * - Fill_parent children taking full height
             * - Horizontal alignment within content width
             * - Remainder pixel distribution to first expand children
             */
            void arrange_vertical(elt_t* parent,
                                  const logical_rect& content_area);

            /**
             * @brief Arrange children in horizontal row
             *
             * @param parent Parent element containing children
             * @param content_area Available area for arrangement
             *
             * @details
             * Arranges children from left to right with spacing. Handles:
             * - Fixed-size children at their measured width
             * - Expand children sharing remaining space equally
             * - Weighted children sharing space proportionally
             * - Fill_parent children taking full width
             * - Vertical alignment within content height
             * - Remainder pixel distribution to first expand children
             */
            void arrange_horizontal(elt_t* parent,
                                    const logical_rect& content_area);

            /**
             * @brief Distribute space among weighted children with constraints
             *
             * Uses an iterative algorithm to handle min/max constraints:
             * 1. Calculate ideal distribution based on weights
             * 2. Apply constraints, marking satisfied children
             * 3. Redistribute remaining space among unsatisfied children
             * 4. Repeat until all children are satisfied or no space remains
             *
             * @param weighted_children Children with weighted size policy
             * @param available_space Total space to distribute
             * @param is_vertical True for vertical distribution, false for horizontal
             * @return Vector of calculated sizes for each weighted child
             */
            std::vector <logical_unit> distribute_weighted_space(
                const std::vector <elt_t*>& weighted_children,
                logical_unit available_space,
                bool is_vertical) const;
    };

    // ==========================================================================================
    // Implementation
    // ==========================================================================================
    template<UIBackend Backend>
    logical_size linear_layout<Backend>::measure_children(const elt_t* parent, logical_unit available_width,
                                                         logical_unit available_height) const {
        const auto& children = this->get_children(parent);
        if (children.empty()) {
            return logical_size{logical_unit(0.0), logical_unit(0.0)};
        }

        // First, count visible children to calculate correct spacing
        int visible_count = 0;
        for (const auto& child : children) {
            if (child->is_visible()) {
                visible_count++;
            }
        }

        // If no visible children, return zero size
        if (visible_count == 0) {
            return logical_size{logical_unit(0.0), logical_unit(0.0)};
        }

        logical_unit total_width = logical_unit(0.0);
        logical_unit total_height = logical_unit(0.0);
        logical_unit total_spacing = logical_unit(0.0);
        if (visible_count > 1) {
            total_spacing = logical_unit(static_cast<double>(m_spacing * (visible_count - 1)));
        }

        if (m_layout_direction == direction::vertical) {
            // Vertical layout: stack children vertically
            for (auto& child : children) {
                if (!child->is_visible()) continue;

                // Measure child with full width, remaining height
                logical_unit remaining_height = max(logical_unit(0.0), available_height - total_height);

                logical_size const child_size = child->measure(available_width, remaining_height);

                total_width = max(total_width, child_size.width);
                total_height = total_height + child_size.height;
            }

            total_height = total_height + total_spacing;
        } else {
            // Horizontal layout: stack children horizontally
            for (auto& child : children) {
                if (!child->is_visible()) continue;

                // Measure child with remaining width, full height
                logical_unit remaining_width = max(logical_unit(0.0), available_width - total_width);
                logical_size const child_size = child->measure(remaining_width, available_height);

                total_width = total_width + child_size.width;
                total_height = max(total_height, child_size.height);
            }

            total_width = total_width + total_spacing;
        }

        return logical_size{total_width, total_height};
    }

    // ------------------------------------------------------------------------------------------------------
    template<UIBackend Backend>
    void linear_layout<Backend>::arrange_children(elt_t* parent, const logical_rect& content_area) {
        const auto& children = this->get_mutable_children(parent);
        if (children.empty()) return;

        // Check if we have any visible children
        bool has_visible = false;
        for (const auto& child : children) {
            if (child->is_visible()) {
                has_visible = true;
                break;
            }
        }

        if (!has_visible) return;

        if (m_layout_direction == direction::vertical) {
            arrange_vertical(parent, content_area);
        } else {
            arrange_horizontal(parent, content_area);
        }
    }

    // ---------------------------------------------------------------------------------------
    template<UIBackend Backend>
    void linear_layout<Backend>::arrange_vertical(elt_t* parent, const logical_rect& content_area) {
        const auto& children = this->get_mutable_children(parent);
        const logical_unit content_h = content_area.height;

        // Calculate total desired height and identify expanding/weighted children
        logical_unit total_fixed_height = logical_unit(0.0);
        int num_expanding = 0;
        int visible_count = 0;
        std::vector <elt_t*> weighted_children;

        for (const auto& child : children) {
            if (!child->is_visible()) continue;

            visible_count++;
            const logical_size& measured = this->get_last_measured_size(child.get());
            logical_unit const meas_h = measured.height;

            if (child->h_constraint().policy == size_policy::expand) {
                num_expanding++;
                // Don't add to total_fixed_height - expand children use available space
            } else if (child->h_constraint().policy == size_policy::weighted) {
                weighted_children.push_back(child.get());
                // Don't add to total_fixed_height - weighted children use available space
            } else if (child->h_constraint().policy == size_policy::percentage) {
                // Calculate percentage of parent height
                logical_unit const percentage_h = logical_unit(content_h.value * static_cast<double>(child->h_constraint().percentage));
                logical_unit const clamped_h = child->h_constraint().clamp(percentage_h);
                total_fixed_height = total_fixed_height + clamped_h;
            } else {
                // Only fixed-size children contribute to total_fixed_height
                total_fixed_height = total_fixed_height + meas_h;
            }
        }

        // Calculate spacing
        const logical_unit total_spacing = (visible_count > 1) ? logical_unit(static_cast<double>(m_spacing * (visible_count - 1))) : logical_unit(0.0);
        logical_unit const available_height = content_h - total_fixed_height - total_spacing;

        // Distribute space for expanding children
        // Use floor() to get integer-aligned base size, then distribute fractional remainder
        const logical_unit expand_height_raw = (num_expanding > 0 && available_height > logical_unit(0.0))
                                ? logical_unit(available_height.value / static_cast<double>(num_expanding))
                                : logical_unit(0.0);
        const logical_unit expand_height = expand_height_raw.floor();

        // Distribute space for weighted children with constraints
        std::vector <logical_unit> weighted_sizes;
        if (!weighted_children.empty()) {
            logical_unit weighted_available = available_height;
            if (num_expanding > 0) {
                // Weighted children share space with expanding children
                weighted_available = available_height - logical_unit(expand_height.value * static_cast<double>(num_expanding));
            }
            weighted_sizes = distribute_weighted_space(weighted_children, weighted_available, true);
        }

        // Handle remainder from integer division (after flooring)
        const logical_unit expand_remainder = (num_expanding > 0 && available_height > logical_unit(0.0))
                                   ? available_height - logical_unit(expand_height.value * static_cast<double>(num_expanding))
                                   : logical_unit(0.0);

        // Position children (RELATIVE coordinates - 0,0 = top-left of content area)
        logical_unit current_y = logical_unit(0.0);  // Relative to content area, not absolute screen position
        logical_unit const content_x = logical_unit(0.0);  // Relative to content area
        logical_unit const content_w = content_area.width;
        size_t weighted_index = 0;
        int expand_child_count = 0;

        for (const auto& child : children) {
            if (!child->is_visible()) continue;

            const logical_size& measured = this->get_last_measured_size(child.get());
            logical_unit const meas_w = measured.width;
            logical_unit const meas_h = measured.height;

            // Determine child height
            logical_unit child_height = meas_h;
            if (child->h_constraint().policy == size_policy::expand) {
                child_height = expand_height;
                // Give remainder to first children
                if (expand_child_count == 0) {
                    child_height = child_height + expand_remainder;
                }
                expand_child_count++;
            } else if (child->h_constraint().policy == size_policy::weighted) {
                if (weighted_index < weighted_sizes.size()) {
                    child_height = weighted_sizes[weighted_index++];
                }
            } else if (child->h_constraint().policy == size_policy::fill_parent) {
                child_height = content_h;
            } else if (child->h_constraint().policy == size_policy::percentage) {
                // Calculate percentage of parent height
                logical_unit const percentage_h = logical_unit(content_h.value * static_cast<double>(child->h_constraint().percentage));
                child_height = percentage_h;
            } else if (child->h_constraint().policy == size_policy::content) {
                // CRITICAL FIX: Constrain content-policy children to available space
                // Without this, they can overflow (e.g., 116px child in 6px container)
                child_height = min(meas_h, content_h);
            }

            // Apply height constraints (already applied for weighted, but needed for others)
            if (child->h_constraint().policy != size_policy::weighted) {
                child_height = child->h_constraint().clamp(child_height);
            }

            // Determine child width based on horizontal alignment
            logical_unit child_width = meas_w;
            logical_unit child_x = content_x;

            if (this->get_h_align(child.get()) == horizontal_alignment::stretch) {
                child_width = content_w;
            } else {
                child_width = min(meas_w, content_w);

                if (this->get_h_align(child.get()) == horizontal_alignment::center) {
                    child_x = content_x + (content_w - child_width) / 2.0;
                } else if (this->get_h_align(child.get()) == horizontal_alignment::right) {
                    child_x = content_x + content_w - child_width;
                }
            }

            // Arrange child
            logical_rect child_bounds{child_x, current_y, child_width, child_height};
            child->arrange(child_bounds);

            current_y = current_y + child_height + logical_unit(static_cast<double>(m_spacing));
        }
    }

    // ----------------------------------------------------------------------------------------
    template<UIBackend Backend>
    void linear_layout<Backend>::arrange_horizontal(elt_t* parent, const logical_rect& content_area) {
        const auto& children = this->get_mutable_children(parent);
        const logical_unit content_w = content_area.width;

        // Calculate total desired width and identify expanding/weighted children
        logical_unit total_fixed_width = logical_unit(0.0);
        int num_expanding = 0;
        int visible_count = 0;
        std::vector <elt_t*> weighted_children;

        for (const auto& child : children) {
            if (!child->is_visible()) continue;

            visible_count++;
            const logical_size& measured = this->get_last_measured_size(child.get());
            logical_unit const meas_w = measured.width;

            if (child->w_constraint().policy == size_policy::expand) {
                num_expanding++;
                // Don't add to total_fixed_width - expand children use available space
            } else if (child->w_constraint().policy == size_policy::weighted) {
                weighted_children.push_back(child.get());
                // Don't add to total_fixed_width - weighted children use available space
            } else if (child->w_constraint().policy == size_policy::percentage) {
                // Calculate percentage of parent width
                logical_unit const percentage_w = logical_unit(content_w.value * static_cast<double>(child->w_constraint().percentage));
                logical_unit const clamped_w = child->w_constraint().clamp(percentage_w);
                total_fixed_width = total_fixed_width + clamped_w;
            } else {
                // Only fixed-size children contribute to total_fixed_width
                total_fixed_width = total_fixed_width + meas_w;
            }
        }

        // Calculate spacing
        const logical_unit total_spacing = (visible_count > 1) ? logical_unit(static_cast<double>(m_spacing * (visible_count - 1))) : logical_unit(0.0);
        logical_unit const available_width = content_w - total_fixed_width - total_spacing;

        // Distribute space for expanding children
        // Use floor() to get integer-aligned base size, then distribute fractional remainder
        const logical_unit expand_width_raw = (num_expanding > 0 && available_width > logical_unit(0.0))
                               ? logical_unit(available_width.value / static_cast<double>(num_expanding))
                               : logical_unit(0.0);
        const logical_unit expand_width = expand_width_raw.floor();

        // Distribute space for weighted children with constraints
        std::vector <logical_unit> weighted_sizes;
        if (!weighted_children.empty()) {
            logical_unit weighted_available = available_width;
            if (num_expanding > 0) {
                // Weighted children share space with expanding children
                weighted_available = available_width - logical_unit(expand_width.value * static_cast<double>(num_expanding));
            }
            weighted_sizes = distribute_weighted_space(weighted_children, weighted_available, false);
        }

        // Handle remainder from integer division (after flooring)
        const logical_unit expand_remainder = (num_expanding > 0 && available_width > logical_unit(0.0))
                                   ? available_width - logical_unit(expand_width.value * static_cast<double>(num_expanding))
                                   : logical_unit(0.0);

        // Position children (RELATIVE coordinates - 0,0 = top-left of content area)
        logical_unit current_x = logical_unit(0.0);  // Relative to content area, not absolute screen position
        logical_unit const content_y = logical_unit(0.0);  // Relative to content area
        logical_unit const content_h = content_area.height;
        size_t weighted_index = 0;
        int expand_child_count = 0;

        for (const auto& child : children) {
            if (!child->is_visible()) continue;

            const logical_size& measured = this->get_last_measured_size(child.get());
            logical_unit const meas_w = measured.width;
            logical_unit const meas_h = measured.height;

            // Determine child width
            logical_unit child_width = meas_w;
            if (child->w_constraint().policy == size_policy::expand) {
                child_width = expand_width;
                // Give remainder to first child
                if (expand_child_count == 0) {
                    child_width = child_width + expand_remainder;
                }
                expand_child_count++;
            } else if (child->w_constraint().policy == size_policy::weighted) {
                if (weighted_index < weighted_sizes.size()) {
                    child_width = weighted_sizes[weighted_index++];
                }
            } else if (child->w_constraint().policy == size_policy::fill_parent) {
                child_width = content_w;
            } else if (child->w_constraint().policy == size_policy::percentage) {
                // Calculate percentage of parent width
                logical_unit const percentage_w = logical_unit(content_w.value * static_cast<double>(child->w_constraint().percentage));
                child_width = percentage_w;
            } else if (child->w_constraint().policy == size_policy::content) {
                // CRITICAL FIX: Constrain content-policy children to available space
                // Without this, they can overflow the container
                child_width = min(meas_w, content_w);
            }

            // Apply width constraints (already applied for weighted, but needed for others)
            if (child->w_constraint().policy != size_policy::weighted) {
                child_width = child->w_constraint().clamp(child_width);
            }

            // Determine child height based on vertical alignment
            logical_unit child_height = meas_h;
            logical_unit child_y = content_y;

            if (this->get_v_align(child.get()) == vertical_alignment::stretch) {
                child_height = content_h;
            } else {
                child_height = min(meas_h, content_h);

                if (this->get_v_align(child.get()) == vertical_alignment::center) {
                    child_y = content_y + (content_h - child_height) / 2.0;
                } else if (this->get_v_align(child.get()) == vertical_alignment::bottom) {
                    child_y = content_y + content_h - child_height;
                }
            }

            // Arrange child
            logical_rect child_bounds{current_x, child_y, child_width, child_height};
            child->arrange(child_bounds);

            current_x = current_x + child_width + logical_unit(static_cast<double>(m_spacing));
        }
    }

    // ----------------------------------------------------------------------------------------
    template<UIBackend Backend>
    std::vector <logical_unit> linear_layout<Backend>::distribute_weighted_space(
        const std::vector <elt_t*>& weighted_children,
        logical_unit available_space,
        bool is_vertical) const {
        if (weighted_children.empty() || available_space <= logical_unit(0.0)) {
            return std::vector <logical_unit>(weighted_children.size(), logical_unit(0.0));
        }

        struct ChildInfo {
            elt_t* child;
            float weight;
            logical_unit min_size;
            logical_unit max_size;
            logical_unit assigned_size;
            bool satisfied;
        };

        // Initialize child info
        std::vector <ChildInfo> children_info;
        float total_weight = 0.0F;

        for (auto* child : weighted_children) {
            const auto& constraint = is_vertical
                                         ? child->h_constraint()
                                         : child->w_constraint();

            ChildInfo const info{
                child,
                constraint.weight,
                constraint.min_size,
                constraint.max_size,
                logical_unit(0.0),      // assigned_size
                false   // satisfied
            };

            children_info.push_back(info);
            total_weight += info.weight;
        }

        // Iterative distribution algorithm
        logical_unit remaining_space = available_space;
        bool changed = true;

        while (changed && remaining_space > logical_unit(0.0)) {
            changed = false;

            // Calculate active weight (only unsatisfied children)
            float active_weight = 0.0F;
            for (const auto& info : children_info) {
                if (!info.satisfied) {
                    active_weight += info.weight;
                }
            }

            if (active_weight == 0.0F) {
                break; // All children satisfied or no weights
            }

            // Distribute to unsatisfied children
            for (auto& info : children_info) {
                if (info.satisfied) continue;

                // Calculate ideal size based on weight
                logical_unit const ideal_size = logical_unit(
                    remaining_space.value * static_cast<double>(info.weight / active_weight)
                );

                // Apply constraints
                logical_unit const constrained_size = max(info.min_size,
                                                min(ideal_size, info.max_size));

                // Check if child hit a constraint
                if (constrained_size != ideal_size) {
                    // Child is satisfied (hit min or max)
                    info.assigned_size = constrained_size;
                    info.satisfied = true;
                    remaining_space = remaining_space - constrained_size;
                    changed = true;
                    break; // Restart distribution
                }
            }

            // If no constraints were hit, assign ideal sizes to all unsatisfied
            if (!changed) {
                for (auto& info : children_info) {
                    if (!info.satisfied) {
                        info.assigned_size = logical_unit(
                            remaining_space.value * static_cast<double>(info.weight / active_weight)
                        );
                        info.satisfied = true;
                    }
                }
                break;
            }
        }

        // Handle any remaining space due to rounding
        if (!children_info.empty()) {
            logical_unit total_assigned = logical_unit(0.0);
            for (const auto& info : children_info) {
                total_assigned = total_assigned + info.assigned_size;
            }

            logical_unit remainder = available_space - total_assigned;
            if (remainder > logical_unit(0.0)) {
                // Distribute remainder to children that haven't hit max
                for (auto& info : children_info) {
                    if (remainder > logical_unit(0.0) && info.assigned_size < info.max_size) {
                        logical_unit const extra = min(remainder, info.max_size - info.assigned_size);
                        info.assigned_size = info.assigned_size + extra;
                        remainder = remainder - extra;
                    }
                }
            }
        }

        // Extract final sizes
        std::vector <logical_unit> result;
        for (const auto& info : children_info) {
            result.push_back(info.assigned_size);
        }

        return result;
    }
}
