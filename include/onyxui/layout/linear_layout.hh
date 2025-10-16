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
 * - Multiple size policies: fixed, content, expand, weighted, fill_parent
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
#include <onyxui/layout_strategy.hh>
#include <onyxui/utils/safe_math.hh>

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
             * @param spacing Gap between children in pixels
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
             * @return Spacing in pixels
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
            size_type measure_children(const elt_t* parent,
                                   int available_width,
                                   int available_height) const override;

            /**
             * @brief Arrange all children within the content area
             *
             * @param parent Parent element whose children to arrange
             * @param content_area Rectangle defining the area for arrangement
             *
             * @details
             * Override of layout_strategy::arrange_children. Delegates to
             * arrange_vertical() or arrange_horizontal() based on layout direction.
             * Skips arrangement if no visible children exist.
             */
            void arrange_children(elt_t* parent,
                                  const rect_type& content_area) override;

        private:
            // Immutable configuration
            const direction m_layout_direction; ///< Stacking direction (vertical/horizontal)
            const int m_spacing; ///< Gap between children in pixels
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
                                  const rect_type& content_area);

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
                                    const rect_type& content_area);

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
            std::vector <int> distribute_weighted_space(
                const std::vector <elt_t*>& weighted_children,
                int available_space,
                bool is_vertical) const;
    };

    // ==========================================================================================
    // Implementation
    // ==========================================================================================
    template<UIBackend Backend>
    typename Backend::size_type linear_layout<Backend>::measure_children(const elt_t* parent, int available_width,
                                                         int available_height) const {
        const auto& children = this->get_children(parent);
        if (children.empty()) {
            size_type result = {};
            size_utils::set_size(result, 0, 0);
            return result;
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
            size_type result = {};
            size_utils::set_size(result, 0, 0);
            return result;
        }

        int total_width = 0;
        int total_height = 0;
        int total_spacing = 0;
        if (visible_count > 1) {
            total_spacing = safe_math::multiply_clamped(m_spacing, visible_count - 1);
        }

        if (m_layout_direction == direction::vertical) {
            // Vertical layout: stack children vertically
            for (auto& child : children) {
                if (!child->is_visible()) continue;

                // Measure child with full width, remaining height
                int remaining_height = std::max(0, available_height - total_height);
                size_type child_size = child->measure(available_width, remaining_height);

                int child_w = size_utils::get_width(child_size);
                int child_h = size_utils::get_height(child_size);

                total_width = std::max(total_width, child_w);
                safe_math::accumulate_safe(total_height, child_h);
            }

            safe_math::accumulate_safe(total_height, total_spacing);
        } else {
            // Horizontal layout: stack children horizontally
            for (auto& child : children) {
                if (!child->is_visible()) continue;

                // Measure child with remaining width, full height
                int remaining_width = std::max(0, available_width - total_width);
                size_type child_size = child->measure(remaining_width, available_height);

                int child_w = size_utils::get_width(child_size);
                int child_h = size_utils::get_height(child_size);

                safe_math::accumulate_safe(total_width, child_w);
                total_height = std::max(total_height, child_h);
            }

            safe_math::accumulate_safe(total_width, total_spacing);
        }

        size_type result = {};
        size_utils::set_size(result, total_width, total_height);
        return result;
    }

    // ------------------------------------------------------------------------------------------------------
    template<UIBackend Backend>
    void linear_layout<Backend>::arrange_children(elt_t* parent, const rect_type& content_area) {
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
    void linear_layout<Backend>::arrange_vertical(elt_t* parent, const rect_type& content_area) {
        const auto& children = this->get_mutable_children(parent);
        const int content_h = rect_utils::get_height(content_area);

        // Calculate total desired height and identify expanding/weighted children
        int total_fixed_height = 0;
        int num_expanding = 0;
        int visible_count = 0;
        std::vector <elt_t*> weighted_children;

        for (const auto& child : children) {
            if (!child->is_visible()) continue;

            visible_count++;
            const size_type& measured = this->get_last_measured_size(child.get());
            int meas_h = size_utils::get_height(measured);

            if (child->h_constraint().policy == size_policy::expand) {
                num_expanding++;
                // Don't add to total_fixed_height - expand children use available space
            } else if (child->h_constraint().policy == size_policy::weighted) {
                weighted_children.push_back(child.get());
                // Don't add to total_fixed_height - weighted children use available space
            } else {
                // Only fixed-size children contribute to total_fixed_height
                total_fixed_height += meas_h;
            }
        }

        // Calculate spacing
        const int total_spacing = (visible_count > 1) ? m_spacing * (visible_count - 1) : 0;
        int available_height = content_h - total_fixed_height - total_spacing;

        // Distribute space for expanding children
        const int expand_height = (num_expanding > 0 && available_height > 0)
                                ? available_height / num_expanding
                                : 0;

        // Distribute space for weighted children with constraints
        std::vector <int> weighted_sizes;
        if (!weighted_children.empty()) {
            int weighted_available = available_height;
            if (num_expanding > 0) {
                // Weighted children share space with expanding children
                weighted_available = available_height - (expand_height * num_expanding);
            }
            weighted_sizes = distribute_weighted_space(weighted_children, weighted_available, true);
        }

        // Handle remainder from integer division
        const int expand_remainder = (num_expanding > 0 && available_height > 0)
                                   ? available_height - (expand_height * num_expanding)
                                   : 0;

        // Position children
        int current_y = rect_utils::get_y(content_area);
        int content_x = rect_utils::get_x(content_area);
        int content_w = rect_utils::get_width(content_area);
        size_t weighted_index = 0;
        int expand_child_count = 0;

        for (const auto& child : children) {
            if (!child->is_visible()) continue;

            const size_type& measured = this->get_last_measured_size(child.get());
            int meas_w = size_utils::get_width(measured);
            int meas_h = size_utils::get_height(measured);

            // Determine child height
            int child_height = meas_h;
            if (child->h_constraint().policy == size_policy::expand) {
                child_height = expand_height;
                // Give remainder pixels to first children
                if (expand_child_count < expand_remainder) {
                    child_height++;
                }
                expand_child_count++;
            } else if (child->h_constraint().policy == size_policy::weighted) {
                if (weighted_index < weighted_sizes.size()) {
                    child_height = weighted_sizes[weighted_index++];
                }
            } else if (child->h_constraint().policy == size_policy::fill_parent) {
                child_height = content_h;
            }

            // Apply height constraints (already applied for weighted, but needed for others)
            if (child->h_constraint().policy != size_policy::weighted) {
                child_height = child->h_constraint().clamp(child_height);
            }

            // Determine child width based on horizontal alignment
            int child_width = meas_w;
            int child_x = content_x;

            if (this->get_h_align(child.get()) == horizontal_alignment::stretch) {
                child_width = content_w;
            } else {
                child_width = std::min(meas_w, content_w);

                if (this->get_h_align(child.get()) == horizontal_alignment::center) {
                    child_x = content_x + (content_w - child_width) / 2;
                } else if (this->get_h_align(child.get()) == horizontal_alignment::right) {
                    child_x = content_x + content_w - child_width;
                }
            }

            // Arrange child
            rect_type child_bounds;
            rect_utils::set_bounds(child_bounds, child_x, current_y,
                                   child_width, child_height);
            child->arrange(child_bounds);

            current_y += child_height + m_spacing;
        }
    }

    // ----------------------------------------------------------------------------------------
    template<UIBackend Backend>
    void linear_layout<Backend>::arrange_horizontal(elt_t* parent, const rect_type& content_area) {
        const auto& children = this->get_mutable_children(parent);
        const int content_w = rect_utils::get_width(content_area);

        // Calculate total desired width and identify expanding/weighted children
        int total_fixed_width = 0;
        int num_expanding = 0;
        int visible_count = 0;
        std::vector <elt_t*> weighted_children;

        for (const auto& child : children) {
            if (!child->is_visible()) continue;

            visible_count++;
            const size_type& measured = this->get_last_measured_size(child.get());
            int meas_w = size_utils::get_width(measured);

            if (child->w_constraint().policy == size_policy::expand) {
                num_expanding++;
                // Don't add to total_fixed_width - expand children use available space
            } else if (child->w_constraint().policy == size_policy::weighted) {
                weighted_children.push_back(child.get());
                // Don't add to total_fixed_width - weighted children use available space
            } else {
                // Only fixed-size children contribute to total_fixed_width
                total_fixed_width += meas_w;
            }
        }

        // Calculate spacing
        const int total_spacing = (visible_count > 1) ? m_spacing * (visible_count - 1) : 0;
        int available_width = content_w - total_fixed_width - total_spacing;

        // Distribute space for expanding children
        const int expand_width = (num_expanding > 0 && available_width > 0)
                               ? available_width / num_expanding
                               : 0;

        // Distribute space for weighted children with constraints
        std::vector <int> weighted_sizes;
        if (!weighted_children.empty()) {
            int weighted_available = available_width;
            if (num_expanding > 0) {
                // Weighted children share space with expanding children
                weighted_available = available_width - (expand_width * num_expanding);
            }
            weighted_sizes = distribute_weighted_space(weighted_children, weighted_available, false);
        }

        // Handle remainder from integer division
        const int expand_remainder = (num_expanding > 0 && available_width > 0)
                                   ? available_width - (expand_width * num_expanding)
                                   : 0;

        // Position children
        int current_x = rect_utils::get_x(content_area);
        int content_y = rect_utils::get_y(content_area);
        int content_h = rect_utils::get_height(content_area);
        size_t weighted_index = 0;
        int expand_child_count = 0;

        for (const auto& child : children) {
            if (!child->is_visible()) continue;

            const size_type& measured = this->get_last_measured_size(child.get());
            int meas_w = size_utils::get_width(measured);
            int meas_h = size_utils::get_height(measured);

            // Determine child width
            int child_width = meas_w;
            if (child->w_constraint().policy == size_policy::expand) {
                child_width = expand_width;
                // Give remainder pixels to first children
                if (expand_child_count < expand_remainder) {
                    child_width++;
                }
                expand_child_count++;
            } else if (child->w_constraint().policy == size_policy::weighted) {
                if (weighted_index < weighted_sizes.size()) {
                    child_width = weighted_sizes[weighted_index++];
                }
            } else if (child->w_constraint().policy == size_policy::fill_parent) {
                child_width = content_w;
            }

            // Apply width constraints (already applied for weighted, but needed for others)
            if (child->w_constraint().policy != size_policy::weighted) {
                child_width = child->w_constraint().clamp(child_width);
            }

            // Determine child height based on vertical alignment
            int child_height = meas_h;
            int child_y = content_y;

            if (this->get_v_align(child.get()) == vertical_alignment::stretch) {
                child_height = content_h;
            } else {
                child_height = std::min(meas_h, content_h);

                if (this->get_v_align(child.get()) == vertical_alignment::center) {
                    child_y = content_y + (content_h - child_height) / 2;
                } else if (this->get_v_align(child.get()) == vertical_alignment::bottom) {
                    child_y = content_y + content_h - child_height;
                }
            }

            // Arrange child
            rect_type child_bounds;
            rect_utils::set_bounds(child_bounds, current_x, child_y,
                                   child_width, child_height);
            child->arrange(child_bounds);

            current_x += child_width + m_spacing;
        }
    }

    // ----------------------------------------------------------------------------------------
    template<UIBackend Backend>
    std::vector <int> linear_layout<Backend>::distribute_weighted_space(
        const std::vector <elt_t*>& weighted_children,
        int available_space,
        bool is_vertical) const {
        if (weighted_children.empty() || available_space <= 0) {
            return std::vector <int>(weighted_children.size(), 0);
        }

        struct ChildInfo {
            elt_t* child;
            float weight;
            int min_size;
            int max_size;
            int assigned_size;
            bool satisfied;
        };

        // Initialize child info
        std::vector <ChildInfo> children_info;
        float total_weight = 0.0f;

        for (auto* child : weighted_children) {
            const auto& constraint = is_vertical
                                         ? child->h_constraint()
                                         : child->w_constraint();

            ChildInfo info;
            info.child = child;
            info.weight = constraint.weight;
            info.min_size = constraint.min_size;
            info.max_size = constraint.max_size;
            info.assigned_size = 0;
            info.satisfied = false;

            children_info.push_back(info);
            total_weight += info.weight;
        }

        // Iterative distribution algorithm
        int remaining_space = available_space;
        bool changed = true;

        while (changed && remaining_space > 0) {
            changed = false;

            // Calculate active weight (only unsatisfied children)
            float active_weight = 0.0f;
            for (const auto& info : children_info) {
                if (!info.satisfied) {
                    active_weight += info.weight;
                }
            }

            if (active_weight == 0.0f) {
                break; // All children satisfied or no weights
            }

            // Distribute to unsatisfied children
            for (auto& info : children_info) {
                if (info.satisfied) continue;

                // Calculate ideal size based on weight
                int ideal_size = static_cast <int>(
                    static_cast<float>(remaining_space) * (info.weight / active_weight)
                );

                // Apply constraints
                int constrained_size = std::max(info.min_size,
                                                std::min(ideal_size, info.max_size));

                // Check if child hit a constraint
                if (constrained_size != ideal_size) {
                    // Child is satisfied (hit min or max)
                    info.assigned_size = constrained_size;
                    info.satisfied = true;
                    remaining_space -= constrained_size;
                    changed = true;
                    break; // Restart distribution
                }
            }

            // If no constraints were hit, assign ideal sizes to all unsatisfied
            if (!changed) {
                for (auto& info : children_info) {
                    if (!info.satisfied) {
                        info.assigned_size = static_cast <int>(
                            static_cast<float>(remaining_space) * (info.weight / active_weight)
                        );
                        info.satisfied = true;
                    }
                }
                break;
            }
        }

        // Handle any remaining space due to rounding
        if (!children_info.empty()) {
            int total_assigned = 0;
            for (const auto& info : children_info) {
                total_assigned += info.assigned_size;
            }

            int remainder = available_space - total_assigned;
            if (remainder > 0) {
                // Distribute remainder to children that haven't hit max
                for (auto& info : children_info) {
                    if (remainder > 0 && info.assigned_size < info.max_size) {
                        int extra = std::min(remainder, info.max_size - info.assigned_size);
                        info.assigned_size += extra;
                        remainder -= extra;
                    }
                }
            }
        }

        // Extract final sizes
        std::vector <int> result;
        for (const auto& info : children_info) {
            result.push_back(info.assigned_size);
        }

        return result;
    }
}
