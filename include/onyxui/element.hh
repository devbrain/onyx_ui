/**
 * @file element.hh
 * @brief Core UI element class with two-pass layout algorithm
 * @author igor
 * @date 08/10/2025
 *
 * @details
 * This file contains the main ui_element class, which represents a node in the
 * UI tree. Elements form a hierarchy with unique_ptr ownership, and use a
 * two-pass layout algorithm (measure then arrange) for efficient positioning.
 *
 * ## Architecture Overview
 *
 * The ui_element class is the foundation of the layout system. Every visual
 * component inherits from ui_element, forming a tree structure where:
 * - Parents own children via std::unique_ptr (automatic memory management)
 * - Layout strategies control how children are positioned
 * - Two-phase algorithm ensures efficient layout calculation
 * - Smart caching minimizes redundant computations
 *
 * ## Core Features
 *
 * - **Hierarchical Structure**: Tree-based parent-child relationships
 * - **Two-Phase Layout**: Measure (bottom-up) then Arrange (top-down)
 * - **Smart Invalidation**: Minimal recalculation through targeted cache invalidation
 * - **Size Constraints**: Flexible sizing with min/max bounds and policies
 * - **Alignment Control**: Precise positioning within allocated space
 * - **Z-Order Management**: Layering support for overlapping elements
 * - **Hit Testing**: Efficient coordinate-based element lookup
 * - **Margin and Padding**: Box model with internal and external spacing
 * - **Event Handling**: Integrated event support through event_target base
 *
 * ## Backend Template Design
 *
 * The element uses a single Backend template parameter that provides all
 * platform-specific types through a traits structure. This simplifies the API
 * and ensures type consistency.
 *
 * @tparam Backend The backend traits type (e.g., sdl_backend, glfw_backend)
 */

#pragma once

#include <vector>
#include <algorithm>
#include <memory>

#include <onyxui/backend.hh>
#include <onyxui/concepts.hh>
#include <onyxui/event_target.hh>
#include <onyxui/layout_strategy.hh>

namespace onyxui {
    /**
     * @struct thickness
     * @brief Represents spacing on all four sides (margin or padding)
     */
    struct thickness {
        int left;   ///< Left spacing in pixels
        int top;    ///< Top spacing in pixels
        int right;  ///< Right spacing in pixels
        int bottom; ///< Bottom spacing in pixels

        /**
         * @brief Calculate total horizontal spacing (left + right)
         */
        [[nodiscard]] int horizontal() const noexcept { return left + right; }

        /**
         * @brief Calculate total vertical spacing (top + bottom)
         */
        [[nodiscard]] int vertical() const noexcept { return top + bottom; }

        bool operator==(const thickness& other) const noexcept {
            return left == other.left && top == other.top &&
                   right == other.right && bottom == other.bottom;
        }

        bool operator!=(const thickness& other) const noexcept {
            return !(*this == other);
        }
    };

    /**
     * @class ui_element
     * @brief Base class for all UI elements in the layout tree
     *
     * @details
     * ui_element represents a node in the UI hierarchy with integrated event handling.
     * It inherits from event_target to provide event processing capabilities alongside
     * layout management.
     *
     * ## Two-Pass Layout Algorithm
     *
     * ### Phase 1: Measure (Bottom-Up)
     * - Starts with leaf elements and propagates upward
     * - Each element calculates its desired size
     * - Results are cached until invalidation
     *
     * ### Phase 2: Arrange (Top-Down)
     * - Starts with root element and propagates downward
     * - Each element receives final bounds from parent
     * - Element arranges children within content area
     *
     * @tparam Backend The backend traits type satisfying UIBackend concept
     *
     * @example Creating a Layout
     * @code
     * using my_ui = ui_element<sdl_backend>;
     *
     * auto root = std::make_unique<my_ui>(nullptr);
     * root->set_layout_strategy(std::make_unique<linear_layout<sdl_backend>>());
     * root->set_padding({10, 10, 10, 10});
     *
     * auto button = std::make_unique<my_ui>(nullptr);
     * button->set_focusable(true);
     * root->add_child(std::move(button));
     * @endcode
     */
    template<UIBackend Backend>
    class ui_element : public event_target<Backend> {
        static_assert(UIBackend<Backend>,
                      "Template parameter must satisfy UIBackend concept. "
                      "Backend must provide rect_type, size_type, point_type, and event types.");
    public:
        // Type aliases from backend
        using rect_type = typename Backend::rect_type;
        using size_type = typename Backend::size_type;
        using point_type = typename Backend::point_type;
        using event_type = typename Backend::event_type;

        // Convenience aliases
        using ui_element_ptr = std::unique_ptr<ui_element>;
        using layout_strategy_ptr = std::unique_ptr<layout_strategy<Backend>>;

        // Grant access to layout strategies
        friend class layout_strategy<Backend>;

    public:
        /**
         * @brief Construct a UI element
         * @param parent Pointer to the parent element (or nullptr for root)
         */
        explicit ui_element(ui_element* parent);

        /**
         * @brief Virtual destructor for proper cleanup
         */
        virtual ~ui_element() noexcept = default;

        // Rule of Five
        ui_element(ui_element&& other) noexcept = default;
        ui_element& operator=(ui_element&& other) noexcept = default;
        ui_element(const ui_element&) = delete;
        ui_element& operator=(const ui_element&) = delete;

        /**
         * @brief Add a child element (takes ownership)
         */
        void add_child(ui_element_ptr child);

        /**
         * @brief Remove a child element (returns ownership)
         */
        [[nodiscard]] ui_element_ptr remove_child(ui_element* child);

        /**
         * @brief Remove all child elements
         */
        void clear_children() noexcept;

        /**
         * @brief Invalidate the measure cache
         */
        void invalidate_measure();

        /**
         * @brief Invalidate the arrange cache
         */
        void invalidate_arrange();

        /**
         * @brief Measure phase: calculate desired size
         */
        [[nodiscard]] size_type measure(int available_width, int available_height);

        /**
         * @brief Arrange phase: assign final bounds
         */
        void arrange(const rect_type& final_bounds);

        /**
         * @brief Sort children by their z_index values
         */
        void sort_children_by_z_index();

        /**
         * @brief Update child z-ordering
         */
        void update_child_order();

        /**
         * @brief Perform hit testing to find element at coordinates
         */
        [[nodiscard]] ui_element* hit_test(int x, int y);

        // -----------------------------------------------------------------------
        // Public Setters
        // -----------------------------------------------------------------------

        /**
         * @brief Set visibility state
         */
        void set_visible(bool visible) {
            if (m_visible != visible) {
                m_visible = visible;
                invalidate_arrange();
            }
        }

        /**
         * @brief Set layout strategy
         */
        void set_layout_strategy(layout_strategy_ptr strategy) {
            m_layout_strategy = std::move(strategy);
            invalidate_measure();
        }

        /**
         * @brief Set width constraint
         */
        void set_width_constraint(const size_constraint& constraint) {
            if (m_width_constraint != constraint) {
                m_width_constraint = constraint;
                invalidate_measure();
            }
        }

        /**
         * @brief Set height constraint
         */
        void set_height_constraint(const size_constraint& constraint) {
            if (m_height_constraint != constraint) {
                m_height_constraint = constraint;
                invalidate_measure();
            }
        }

        /**
         * @brief Set horizontal alignment
         */
        void set_horizontal_align(horizontal_alignment align) {
            if (m_h_align != align) {
                m_h_align = align;
                invalidate_arrange();
            }
        }

        /**
         * @brief Set vertical alignment
         */
        void set_vertical_align(vertical_alignment align) {
            if (m_v_align != align) {
                m_v_align = align;
                invalidate_arrange();
            }
        }

        /**
         * @brief Set margin
         */
        void set_margin(const thickness& margin) {
            if (m_margin != margin) {
                m_margin = margin;
                invalidate_measure();
            }
        }

        /**
         * @brief Set padding
         */
        void set_padding(const thickness& padding) {
            if (m_padding != padding) {
                m_padding = padding;
                invalidate_measure();
            }
        }

        /**
         * @brief Set z-order
         */
        void set_z_order(int z_index) {
            if (m_z_index != z_index) {
                m_z_index = z_index;
                if (m_parent) {
                    m_parent->invalidate_arrange();
                }
            }
        }

        // -----------------------------------------------------------------------
        // Public Accessors
        // -----------------------------------------------------------------------

        const rect_type& bounds() const noexcept { return m_bounds; }
        [[nodiscard]] bool is_visible() const noexcept { return m_visible; }
        [[nodiscard]] const size_constraint& width_constraint() const noexcept { return m_width_constraint; }
        [[nodiscard]] const size_constraint& height_constraint() const noexcept { return m_height_constraint; }
        [[nodiscard]] horizontal_alignment horizontal_align() const noexcept { return m_h_align; }
        [[nodiscard]] vertical_alignment vertical_align() const noexcept { return m_v_align; }
        [[nodiscard]] const thickness& margin() const noexcept { return m_margin; }
        [[nodiscard]] const thickness& padding() const noexcept { return m_padding; }
        [[nodiscard]] int z_order() const noexcept { return m_z_index; }

        // For layout strategies
        const std::vector<ui_element_ptr>& children() const noexcept { return m_children; }
        std::vector<ui_element_ptr>& mutable_children() noexcept { return m_children; }
        const size_type& last_measured_size() const noexcept { return m_last_measured_size; }
        [[nodiscard]] horizontal_alignment h_align() const noexcept { return m_h_align; }
        [[nodiscard]] vertical_alignment v_align() const noexcept { return m_v_align; }
        [[nodiscard]] const size_constraint& w_constraint() const noexcept { return m_width_constraint; }
        [[nodiscard]] const size_constraint& h_constraint() const noexcept { return m_height_constraint; }

    protected:
        // -----------------------------------------------------------------------
        // Protected Virtual Methods (from event_target)
        // -----------------------------------------------------------------------

        /**
         * @brief Check if a point is inside this element
         * Implementation for event_target's pure virtual method
         */
        [[nodiscard]] bool is_inside(int x, int y) const override {
            return rect_utils::contains(m_bounds, x, y);
        }

        // -----------------------------------------------------------------------
        // Protected Virtual Methods for Customization
        // -----------------------------------------------------------------------

        /**
         * @brief Override to provide custom measurement logic
         */
        virtual size_type do_measure(int available_width, int available_height);

        /**
         * @brief Override to provide custom arrangement logic
         */
        virtual void do_arrange(const rect_type& final_bounds);

        /**
         * @brief Get the intrinsic content size
         */
        virtual size_type get_content_size() const {
            size_type s = {};
            size_utils::set_size(s, 0, 0);
            return s;
        }

    private:
        // Parent element (non-owning)
        ui_element* m_parent = nullptr;

        // Children (owned)
        std::vector<ui_element_ptr> m_children;

        // Layout strategy (owned)
        layout_strategy_ptr m_layout_strategy;

        // Layout state
        enum class layout_state {
            valid,
            dirty
        };

        layout_state measure_state = layout_state::dirty;
        layout_state arrange_state = layout_state::dirty;

        // Cache
        size_type m_last_measured_size = {};
        int m_last_available_width = -1;
        int m_last_available_height = -1;

        // Properties
        bool m_visible = true;
        rect_type m_bounds = {};
        int m_z_index = 0;
        size_constraint m_width_constraint;
        size_constraint m_height_constraint;
        horizontal_alignment m_h_align = horizontal_alignment::stretch;
        vertical_alignment m_v_align = vertical_alignment::stretch;
        thickness m_margin = {0, 0, 0, 0};
        thickness m_padding = {0, 0, 0, 0};
    };

    // ===============================================================================
    // Implementation
    // ===============================================================================

    template<UIBackend Backend>
    ui_element<Backend>::ui_element(ui_element* parent)
        : event_target<Backend>(), m_parent(parent) {
    }

    // -------------------------------------------------------------------------------
    // Tree Management
    // -------------------------------------------------------------------------------

    template<UIBackend Backend>
    void ui_element<Backend>::add_child(ui_element_ptr child) {
        if (child) {
            child->m_parent = this;
            m_children.push_back(std::move(child));
            invalidate_measure();
        }
    }

    template<UIBackend Backend>
    void ui_element<Backend>::clear_children() noexcept {
        if (m_layout_strategy) {
            m_layout_strategy->on_children_cleared();
        }

        for (auto& child : m_children) {
            child->m_parent = nullptr;
        }

        m_children.clear();
        invalidate_measure();
    }

    template<UIBackend Backend>
    typename ui_element<Backend>::ui_element_ptr
    ui_element<Backend>::remove_child(ui_element* child) {
        auto it = std::find_if(m_children.begin(), m_children.end(),
                               [child](const ui_element_ptr& ptr) {
                                   return ptr.get() == child;
                               });

        if (it != m_children.end()) {
            if (m_layout_strategy) {
                m_layout_strategy->on_child_removed(child);
            }

            ui_element_ptr removed = std::move(*it);
            m_children.erase(it);
            removed->m_parent = nullptr;
            invalidate_measure();
            return removed;
        }
        return nullptr;
    }

    // -------------------------------------------------------------------------------
    // Invalidation
    // -------------------------------------------------------------------------------

    template<UIBackend Backend>
    void ui_element<Backend>::invalidate_measure() {
        if (measure_state != layout_state::valid) {
            return;
        }

        measure_state = layout_state::dirty;
        arrange_state = layout_state::dirty;

        if (m_parent) {
            m_parent->invalidate_measure();
        }
    }

    template<UIBackend Backend>
    void ui_element<Backend>::invalidate_arrange() {
        if (arrange_state != layout_state::valid) {
            return;
        }

        arrange_state = layout_state::dirty;

        for (auto& child : m_children) {
            child->invalidate_arrange();
        }
    }

    // -------------------------------------------------------------------------------
    // Two-Pass Layout
    // -------------------------------------------------------------------------------

    template<UIBackend Backend>
    typename ui_element<Backend>::size_type
    ui_element<Backend>::measure(int available_width, int available_height) {
        // Check cache
        if (measure_state == layout_state::valid &&
            m_last_available_width == available_width &&
            m_last_available_height == available_height) {
            return m_last_measured_size;
        }

        // Account for margin
        int content_width = std::max(0, available_width - m_margin.horizontal());
        int content_height = std::max(0, available_height - m_margin.vertical());

        // Measure content
        size_type measured = do_measure(content_width, content_height);

        // Add margin back
        int meas_w = size_utils::get_width(measured) + m_margin.horizontal();
        int meas_h = size_utils::get_height(measured) + m_margin.vertical();

        // Apply constraints
        meas_w = m_width_constraint.clamp(meas_w);
        meas_h = m_height_constraint.clamp(meas_h);

        size_utils::set_size(measured, meas_w, meas_h);

        // Cache results
        m_last_measured_size = measured;
        m_last_available_width = available_width;
        m_last_available_height = available_height;

        measure_state = layout_state::valid;
        return m_last_measured_size;
    }

    template<UIBackend Backend>
    void ui_element<Backend>::arrange(const rect_type& final_bounds) {
        // Check if bounds changed
        bool bounds_changed = (rect_utils::get_x(m_bounds) != rect_utils::get_x(final_bounds) ||
                               rect_utils::get_y(m_bounds) != rect_utils::get_y(final_bounds) ||
                               rect_utils::get_width(m_bounds) != rect_utils::get_width(final_bounds) ||
                               rect_utils::get_height(m_bounds) != rect_utils::get_height(final_bounds));

        m_bounds = final_bounds;

        if (!bounds_changed &&
            arrange_state == layout_state::valid &&
            measure_state == layout_state::valid) {
            return;
        }

        // Calculate content area
        rect_type content_area;
        int x = rect_utils::get_x(final_bounds) + m_margin.left + m_padding.left;
        int y = rect_utils::get_y(final_bounds) + m_margin.top + m_padding.top;
        int w = std::max(0, rect_utils::get_width(final_bounds) - m_margin.horizontal() - m_padding.horizontal());
        int h = std::max(0, rect_utils::get_height(final_bounds) - m_margin.vertical() - m_padding.vertical());
        rect_utils::set_bounds(content_area, x, y, w, h);

        do_arrange(content_area);
        arrange_state = layout_state::valid;
    }

    // -------------------------------------------------------------------------------
    // Z-Order
    // -------------------------------------------------------------------------------

    template<UIBackend Backend>
    void ui_element<Backend>::sort_children_by_z_index() {
        std::stable_sort(m_children.begin(), m_children.end(),
                         [](const ui_element_ptr& a, const ui_element_ptr& b) {
                             return a->m_z_index < b->m_z_index;
                         });
    }

    template<UIBackend Backend>
    void ui_element<Backend>::update_child_order() {
        sort_children_by_z_index();
        invalidate_arrange();
    }

    // -------------------------------------------------------------------------------
    // Hit Testing
    // -------------------------------------------------------------------------------

    template<UIBackend Backend>
    ui_element<Backend>* ui_element<Backend>::hit_test(int x, int y) {
        if (!m_visible || !rect_utils::contains(m_bounds, x, y)) {
            return nullptr;
        }

        // Check children in reverse order (highest z-index first)
        for (auto it = m_children.rbegin(); it != m_children.rend(); ++it) {
            if (ui_element* hit = (*it)->hit_test(x, y)) {
                return hit;
            }
        }

        return this;
    }

    // -------------------------------------------------------------------------------
    // Virtual Methods
    // -------------------------------------------------------------------------------

    template<UIBackend Backend>
    typename ui_element<Backend>::size_type
    ui_element<Backend>::do_measure(int available_width, int available_height) {
        if (m_layout_strategy) {
            return m_layout_strategy->measure_children(
                static_cast<const ui_element*>(this), available_width, available_height);
        }
        return get_content_size();
    }

    template<UIBackend Backend>
    void ui_element<Backend>::do_arrange(const rect_type& final_bounds) {
        if (m_layout_strategy) {
            m_layout_strategy->arrange_children(this, final_bounds);
        }
    }
}