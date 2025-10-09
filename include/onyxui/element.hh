/**
 * @file element.hh
 * @brief Core UI element class with two-pass layout algorithm
 * @author igor
 * @date 08/10/2025
 *
 * This file contains the main ui_element class, which represents a node in the
 * UI tree. Elements form a hierarchy with unique_ptr ownership, and use a
 * two-pass layout algorithm (measure then arrange) for efficient positioning.
 */

#pragma once

#include <vector>
#include <algorithm>
#include <memory>

#include <onyxui/concepts.hh>
#include <onyxui/layout_strategy.hh>

namespace onyxui {
    /**
     * @struct thickness
     * @brief Represents spacing on all four sides (margin or padding)
     *
     * Used for both margin (external spacing) and padding (internal spacing).
     * All values are in pixels.
     *
     * @example
     * @code
     * thickness padding = {10, 5, 10, 5};  // left, top, right, bottom
     * int total_horizontal = padding.horizontal();  // 20
     * @endcode
     */
    struct thickness {
        int left;    ///< Left spacing in pixels
        int top;     ///< Top spacing in pixels
        int right;   ///< Right spacing in pixels
        int bottom;  ///< Bottom spacing in pixels

        /**
         * @brief Calculate total horizontal spacing (left + right)
         * @return Sum of left and right spacing
         */
        [[nodiscard]] int horizontal() const { return left + right; }

        /**
         * @brief Calculate total vertical spacing (top + bottom)
         * @return Sum of top and bottom spacing
         */
        [[nodiscard]] int vertical() const { return top + bottom; }
    };

    /**
     * @class ui_element
     * @brief Base class for all UI elements in the layout tree
     *
     * ui_element represents a node in the UI hierarchy. Elements form a tree structure
     * where parents own their children via std::unique_ptr. The layout system uses a
     * two-pass algorithm:
     *
     * 1. **Measure Pass** (bottom-up): Each element calculates its desired size
     *    based on available space and children's requirements
     * 2. **Arrange Pass** (top-down): Each element receives its final bounds and
     *    positions its children
     *
     * Layout is optimized through smart invalidation:
     * - `invalidate_measure()` propagates upward (parents need remeasurement)
     * - `invalidate_arrange()` propagates downward (children need repositioning)
     * - Results are cached to avoid redundant calculations
     *
     * Memory management:
     * - Children are owned by unique_ptr (parent owns children)
     * - Parent pointers are raw non-owning pointers
     * - Layout strategy is owned by unique_ptr
     *
     * @tparam TRect Rectangle type satisfying RectLike concept
     * @tparam TSize Size type satisfying SizeLike concept
     *
     * @example
     * @code
     * // Create a vertical panel with buttons
     * auto panel = std::make_unique<ui_element<SDL_Rect, SDL_Size>>(nullptr);
     * panel->set_layout_strategy(std::make_unique<linear_layout<SDL_Rect, SDL_Size>>());
     * panel->padding = {10, 10, 10, 10};
     *
     * auto button = std::make_unique<ui_element<SDL_Rect, SDL_Size>>(nullptr);
     * button->height_constraint.policy = size_policy::fixed;
     * button->height_constraint.preferred_size = 30;
     * panel->add_child(std::move(button));
     *
     * panel->measure(800, 600);
     * panel->arrange({0, 0, 800, 600});
     * @endcode
     */
    template<RectLike TRect, SizeLike TSize>
    class ui_element {
        public:
            using ui_element_ptr = std::unique_ptr <ui_element>;  ///< Smart pointer for owned children
            using layout_strategy_ptr = std::unique_ptr <layout_strategy <TRect, TSize>>;  ///< Smart pointer for layout

            // Grant access to layout strategies for protected members
            friend class layout_strategy<TRect, TSize>;

        public:
            /**
             * @brief Construct a UI element
             * @param parent Pointer to the parent element (or nullptr for root)
             */
            explicit ui_element(ui_element* parent);

            /**
             * @brief Virtual destructor for proper cleanup
             */
            virtual ~ui_element() = default;

            /**
             * @brief Add a child element (takes ownership)
             *
             * The child's parent pointer is set automatically. Invalidates
             * the measure cache of this element and all ancestors.
             *
             * @param child Unique pointer to the child element
             */
            void add_child(ui_element_ptr child);

            /**
             * @brief Remove a child element (returns ownership)
             *
             * The child's parent pointer is set to nullptr. Invalidates
             * the measure cache of this element and all ancestors.
             *
             * @param child Raw pointer to the child to remove
             * @return Unique pointer to the removed child, or nullptr if not found
             */
            ui_element_ptr remove_child(ui_element* child);

            /**
             * @brief Invalidate the measure cache
             *
             * Marks this element and all ancestors as needing remeasurement.
             * Call this when any property affecting size changes (e.g., content,
             * constraints, padding). Automatically invalidates arrange as well.
             *
             * Propagates upward through the tree.
             */
            void invalidate_measure();

            /**
             * @brief Invalidate the arrange cache
             *
             * Marks this element and all descendants as needing repositioning.
             * Call this when properties affecting position change (not size).
             *
             * Propagates downward through the tree.
             */
            void invalidate_arrange();

            /**
             * @brief Measure phase: calculate desired size
             *
             * Calculates the desired size of this element given the available space.
             * Results are cached - if called again with the same available space
             * and nothing has been invalidated, returns the cached result immediately.
             *
             * The algorithm:
             * 1. Check cache (return if valid)
             * 2. Account for margin
             * 3. Call do_measure() (which typically delegates to layout strategy)
             * 4. Add margin back and apply size constraints
             * 5. Cache and return result
             *
             * @param available_width Maximum width available (pixels)
             * @param available_height Maximum height available (pixels)
             * @return The desired size to accommodate content
             */
            TSize measure(int available_width, int available_height);

            /**
             * @brief Arrange phase: assign final bounds
             *
             * Sets the final position and size of this element, then arranges
             * all children within the content area (excluding margin and padding).
             *
             * The algorithm:
             * 1. Set bounds
             * 2. Check cache (return if valid)
             * 3. Calculate content area (subtract margin and padding)
             * 4. Call do_arrange() (which typically delegates to layout strategy)
             *
             * @param final_bounds The final rectangle assigned to this element
             */
            void arrange(const TRect& final_bounds);

            /**
             * @brief Sort children by their z_index values
             *
             * Children with lower z_index are drawn first (appear behind).
             * Uses stable_sort to preserve relative order of equal z_index values.
             */
            void sort_children_by_z_index();

            /**
             * @brief Update child z-ordering
             *
             * Convenience method that sorts children by z_index and invalidates
             * arrange. Call this after modifying z_index values or when z-order
             * matters (e.g., before rendering or hit testing).
             */
            void update_child_order();

            /**
             * @brief Perform hit testing to find element at coordinates
             *
             * Recursively searches the tree to find the topmost (highest z-index)
             * visible element that contains the given point. Children are tested
             * in reverse order (highest z-index first).
             *
             * @param x X coordinate in parent space
             * @param y Y coordinate in parent space
             * @return Pointer to the hit element, or nullptr if no hit
             */
            ui_element* hit_test(int x, int y);

            // -----------------------------------------------------------------------
            // Public Setters (with side effects)
            // -----------------------------------------------------------------------

            /**
             * @brief Set visibility state
             * @param visible True to show element, false to hide
             */
            void set_visible(bool visible) {
                if (m_visible != visible) {
                    m_visible = visible;
                    invalidate_arrange();
                }
            }

            /**
             * @brief Set enabled state
             * @param enabled True to enable element, false to disable
             */
            void set_enabled(bool enabled) noexcept {
                m_enabled = enabled;
            }

            /**
             * @brief Set layout strategy
             * @param strategy Unique pointer to layout strategy
             */
            void set_layout_strategy(layout_strategy_ptr strategy) {
                m_layout_strategy = std::move(strategy);
                invalidate_measure();
            }

            // -----------------------------------------------------------------------
            // Public Property Accessors (return references for modification)
            // -----------------------------------------------------------------------

            /**
             * @brief Get width constraint for modification
             * @return Reference to width constraint
             * @note Modifying invalidates measure cache
             */
            size_constraint& width_constraint() {
                invalidate_measure();
                return m_width_constraint;
            }

            /**
             * @brief Get height constraint for modification
             * @return Reference to height constraint
             * @note Modifying invalidates measure cache
             */
            size_constraint& height_constraint() {
                invalidate_measure();
                return m_height_constraint;
            }

            /**
             * @brief Get horizontal alignment for modification
             * @return Reference to horizontal alignment
             * @note Modifying invalidates arrange cache
             */
            horizontal_alignment& horizontal_align() {
                invalidate_arrange();
                return m_h_align;
            }

            /**
             * @brief Get vertical alignment for modification
             * @return Reference to vertical alignment
             * @note Modifying invalidates arrange cache
             */
            vertical_alignment& vertical_align() {
                invalidate_arrange();
                return m_v_align;
            }

            /**
             * @brief Get margin for modification
             * @return Reference to margin
             * @note Modifying invalidates measure cache
             */
            thickness& get_margin() {
                invalidate_measure();
                return m_margin;
            }

            /**
             * @brief Get padding for modification
             * @return Reference to padding
             * @note Modifying invalidates measure cache
             */
            thickness& get_padding() {
                invalidate_measure();
                return m_padding;
            }

            /**
             * @brief Get z-index for modification
             * @return Reference to z-index
             * @note Modifying invalidates arrange cache
             */
            int& z_order() {
                invalidate_arrange();
                return m_z_index;
            }

            // -----------------------------------------------------------------------
            // Public Const Accessors
            // -----------------------------------------------------------------------

            /**
             * @brief Get element bounds
             * @return Const reference to bounds
             */
            const TRect& bounds() const noexcept { return m_bounds; }

            /**
             * @brief Check if element is visible
             * @return True if visible, false otherwise
             */
            bool is_visible() const noexcept { return m_visible; }

            /**
             * @brief Check if element is enabled
             * @return True if enabled, false otherwise
             */
            bool is_enabled() const noexcept { return m_enabled; }

            /**
             * @brief Get width constraint (const)
             * @return Const reference to width constraint
             */
            const size_constraint& width_constraint() const noexcept { return m_width_constraint; }

            /**
             * @brief Get height constraint (const)
             * @return Const reference to height constraint
             */
            const size_constraint& height_constraint() const noexcept { return m_height_constraint; }

            /**
             * @brief Get horizontal alignment (const)
             * @return Horizontal alignment value
             */
            horizontal_alignment horizontal_align() const noexcept { return m_h_align; }

            /**
             * @brief Get vertical alignment (const)
             * @return Vertical alignment value
             */
            vertical_alignment vertical_align() const noexcept { return m_v_align; }

            /**
             * @brief Get margin (const)
             * @return Const reference to margin
             */
            const thickness& get_margin() const noexcept { return m_margin; }

            /**
             * @brief Get padding (const)
             * @return Const reference to padding
             */
            const thickness& get_padding() const noexcept { return m_padding; }

            /**
             * @brief Get z-index (const)
             * @return Z-index value
             */
            int z_order() const noexcept { return m_z_index; }

        protected:
            // -----------------------------------------------------------------------
            // Protected Interface for Layout Strategies
            // -----------------------------------------------------------------------

            /**
             * @brief Get children container (for layout strategies)
             * @return Const reference to children vector
             */
            const std::vector<ui_element_ptr>& children() const noexcept { return m_children; }

            /**
             * @brief Get last measured size (for layout strategies)
             * @return Const reference to last measured size
             */
            const TSize& last_measured_size() const noexcept { return m_last_measured_size; }

            /**
             * @brief Get horizontal alignment (for layout strategies)
             * @return Horizontal alignment value
             */
            horizontal_alignment h_align() const noexcept { return m_h_align; }

            /**
             * @brief Get vertical alignment (for layout strategies)
             * @return Vertical alignment value
             */
            vertical_alignment v_align() const noexcept { return m_v_align; }

            /**
             * @brief Get width constraint (for layout strategies)
             * @return Const reference to width constraint
             */
            const size_constraint& w_constraint() const noexcept { return m_width_constraint; }

            /**
             * @brief Get height constraint (for layout strategies)
             * @return Const reference to height constraint
             */
            const size_constraint& h_constraint() const noexcept { return m_height_constraint; }

            // -----------------------------------------------------------------------
            // Protected Virtual Methods for Customization
            // -----------------------------------------------------------------------
            /**
             * @brief Override to provide custom measurement logic
             *
             * Default implementation delegates to the layout strategy if present,
             * otherwise returns get_content_size(). Override in derived classes
             * to implement custom measurement (e.g., text measurement, image sizing).
             *
             * @param available_width Maximum width available (pixels)
             * @param available_height Maximum height available (pixels)
             * @return The desired size for this element's content
             */
            virtual TSize do_measure(int available_width, int available_height);

            /**
             * @brief Override to provide custom arrangement logic
             *
             * Default implementation delegates to the layout strategy if present.
             * Override in derived classes for custom child positioning logic.
             *
             * @param final_bounds The content area bounds (after subtracting margin/padding)
             */
            virtual void do_arrange(const TRect& final_bounds);

            /**
             * @brief Get the intrinsic content size
             *
             * Override this in derived classes to provide content size for
             * content-based sizing (e.g., text dimensions, image size).
             * Default returns (0, 0).
             *
             * @return The natural size of the content
             */
            virtual TSize get_content_size() const {
                TSize s = {};
                size_utils::set_size(s, 0, 0);
                return s;
            }

        private:
            /// Non-owning pointer to parent element (nullptr for root)
            ui_element* m_parent = nullptr;

            /// Owned children (owned via unique_ptr)
            std::vector <ui_element_ptr> m_children;

            /// Owned layout strategy (determines how children are arranged)
            layout_strategy_ptr m_layout_strategy;

            /**
             * @enum layout_state
             * @brief Tracks layout cache validity to prevent redundant invalidation
             */
            enum class layout_state {
                valid,      ///< Layout is up to date, cache is valid
                dirty,      ///< This element needs layout recalculation
                propagated  ///< Invalidation already propagated (prevents cycles)
            };

            /// Current measure cache state
            layout_state measure_state = layout_state::dirty;

            /// Current arrange cache state
            layout_state arrange_state = layout_state::dirty;

            /// Cached measure results (for performance)
            mutable TSize m_last_measured_size = {};
            mutable int m_last_available_width = -1;
            mutable int m_last_available_height = -1;

            /// Visibility flag (hidden elements skip layout and rendering)
            bool m_visible = true;

            /// Enabled flag (for interaction, doesn't affect layout)
            bool m_enabled = true;

            /// Final positioned bounds (set during arrange phase)
            TRect m_bounds = {};

            /// Z-order index (lower values behind, higher values in front)
            int z_index = 0;

            /// Width sizing constraints and policy
            size_constraint m_width_constraint;

            /// Height sizing constraints and policy
            size_constraint m_height_constraint;

            /// Horizontal alignment within allocated space
            horizontal_alignment m_h_align = horizontal_alignment::stretch;

            /// Vertical alignment within allocated space
            vertical_alignment m_v_align = vertical_alignment::stretch;

            /// External spacing (pushes away from neighbors)
            thickness m_margin = {0, 0, 0, 0};

            /// Internal spacing (pushes children inward)
            thickness m_padding = {0, 0, 0, 0};
    };

    // ===============================================================================
    // Implementation
    // ===============================================================================

    template<RectLike TRect, SizeLike TSize>
    ui_element <TRect, TSize>::ui_element(ui_element* parent)
        : m_parent(parent) {
    }

    // -------------------------------------------------------------------------------
    // Tree Management
    // -------------------------------------------------------------------------------

    template<RectLike TRect, SizeLike TSize>
    void ui_element <TRect, TSize>::add_child(ui_element_ptr child) {
        child->m_parent = this;
        m_children.push_back(std::move(child));
        invalidate_measure();
    }
    // ------------------------------------------------------------------------------
    template<RectLike TRect, SizeLike TSize>
    ui_element <TRect, TSize>::ui_element_ptr ui_element <TRect, TSize>::remove_child(ui_element* child) {
        auto it = std::find_if(m_children.begin(), m_children.end(),
                               [child](const ui_element_ptr& ptr) {
                                   return ptr.get() == child;
                               });

        if (it != m_children.end()) {
            ui_element_ptr removed = std::move(*it);
            m_children.erase(it);
            removed->m_parent = nullptr;
            invalidate_measure();
            return removed;
        }
        return nullptr;
    }

    // -------------------------------------------------------------------------------
    // Invalidation (Cache Management)
    // -------------------------------------------------------------------------------

    template<RectLike TRect, SizeLike TSize>
    void ui_element <TRect, TSize>::invalidate_measure() {
        // If already dirty or propagated, nothing to do
        if (measure_state != layout_state::valid) {
            return;
        }

        measure_state = layout_state::dirty;
        arrange_state = layout_state::dirty;

        // Propagate up
        if (m_parent) {
            m_parent->invalidate_measure();
        }
    }
    // -----------------------------------------------------------------------------------------------------
    template<RectLike TRect, SizeLike TSize>
    void ui_element <TRect, TSize>::invalidate_arrange() {
        if (arrange_state != layout_state::valid) {
            return;
        }

        arrange_state = layout_state::dirty;

        // Propagate down
        for (auto& child : m_children) {
            child->invalidate_arrange();
        }
    }

    // -------------------------------------------------------------------------------
    // Two-Pass Layout Algorithm
    // -------------------------------------------------------------------------------

    template<RectLike TRect, SizeLike TSize>
    TSize ui_element <TRect, TSize>::measure(int available_width, int available_height) {
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
        TSize measured = do_measure(content_width, content_height);

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
    // -----------------------------------------------------------------------------------------------------
    template<RectLike TRect, SizeLike TSize>
    void ui_element <TRect, TSize>::arrange(const TRect& final_bounds) {
        m_bounds = final_bounds;

        if (arrange_state == layout_state::valid &&
            measure_state == layout_state::valid) {
            return;
        }

        // Calculate content area (exclude margin and padding)
        TRect content_area;
        int x = rect_utils::get_x(final_bounds) + m_margin.left + m_padding.left;
        int y = rect_utils::get_y(final_bounds) + m_margin.top + m_padding.top;
        int w = std::max(0, rect_utils::get_width(final_bounds) - m_margin.horizontal() - m_padding.horizontal());
        int h = std::max(0, rect_utils::get_height(final_bounds) - m_margin.vertical() - m_padding.vertical());
        rect_utils::set_bounds(content_area, x, y, w, h);

        do_arrange(content_area);
        arrange_state = layout_state::valid;
    }

    // -------------------------------------------------------------------------------
    // Z-Order Management
    // -------------------------------------------------------------------------------

    template<RectLike TRect, SizeLike TSize>
    void ui_element<TRect, TSize>::sort_children_by_z_index() {
        std::stable_sort(m_children.begin(), m_children.end(),
                         [](const ui_element_ptr& a, const ui_element_ptr& b) {
                             return a->m_z_index < b->m_z_index;
                         });
    }

    // -----------------------------------------------------------------------------------------------------
    template<RectLike TRect, SizeLike TSize>
    void ui_element<TRect, TSize>::update_child_order() {
        sort_children_by_z_index();
        invalidate_arrange();
    }

    // -------------------------------------------------------------------------------
    // Hit Testing
    // -------------------------------------------------------------------------------

    template<RectLike TRect, SizeLike TSize>
    ui_element<TRect, TSize>* ui_element<TRect, TSize>::hit_test(int x, int y) {
        // Not visible or not within bounds
        if (!m_visible || !rect_utils::contains(m_bounds, x, y)) {
            return nullptr;
        }

        // Check children in reverse order (highest z-index first)
        for (auto it = m_children.rbegin(); it != m_children.rend(); ++it) {
            ui_element* hit = (*it)->hit_test(x, y);
            if (hit) return hit;
        }

        // No child hit, return this element
        return this;
    }

    // -------------------------------------------------------------------------------
    // Virtual Overrides (Customization Points)
    // -------------------------------------------------------------------------------

    template<RectLike TRect, SizeLike TSize>
    TSize ui_element <TRect, TSize>::do_measure(int available_width, int available_height) {
        // Default implementation: measure using layout strategy
        if (m_layout_strategy) {
            return m_layout_strategy->measure_children(this, available_width, available_height);
        }

        // Fallback: use content size
        return get_content_size();
    }
    // -----------------------------------------------------------------------------------------------------
    template<RectLike TRect, SizeLike TSize>
    void ui_element <TRect, TSize>::do_arrange(const TRect& final_bounds) {
        // Default implementation: arrange using layout strategy
        if (m_layout_strategy) {
            m_layout_strategy->arrange_children(this, final_bounds);
        }
    }
}
