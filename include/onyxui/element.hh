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
 *
 * ## Memory Management
 *
 * - **Ownership Model**: Parents own children via unique_ptr
 * - **No Cycles**: Raw pointers for parent references prevent cycles
 * - **RAII**: Automatic cleanup through destructors
 * - **Move Semantics**: Efficient transfer of ownership
 * - **No Copying**: Copy operations deleted to maintain unique ownership
 *
 * ## Performance Characteristics
 *
 * - **Time Complexity**:
 *   - Measure/Arrange: O(n) where n = total elements in subtree
 *   - Hit Test: O(depth * avg_children) worst case
 *   - Add/Remove Child: O(1) amortized
 * - **Space Complexity**: O(1) per element (excluding children)
 * - **Cache Efficiency**: Results cached until invalidation
 * - **Lazy Evaluation**: Only recalculate when necessary
 *
 * ## Thread Safety
 *
 * ui_element is NOT thread-safe. All operations should occur on the UI thread.
 * For multi-threaded scenarios, use external synchronization or message passing.
 *
 * @see layout_strategy For positioning algorithms
 * @see size_constraint For sizing configuration
 * @see thickness For margin and padding
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
     * @details
     * Used for both margin (external spacing) and padding (internal spacing).
     * All values are in pixels. Follows the CSS box model conventions where:
     * - **Margin**: External spacing that pushes the element away from neighbors
     * - **Padding**: Internal spacing between the element's edge and its content
     *
     * ## Usage Patterns
     *
     * ### Uniform Spacing
     * ```cpp
     * thickness uniform = {10, 10, 10, 10};  // Same on all sides
     * ```
     *
     * ### Asymmetric Spacing
     * ```cpp
     * thickness card_padding = {20, 15, 20, 15};  // More horizontal than vertical
     * thickness indent = {40, 0, 0, 0};           // Left indent only
     * ```
     *
     * ### Common UI Patterns
     * ```cpp
     * // Button padding
     * button->set_padding({12, 8, 12, 8});
     *
     * // Card margin
     * card->set_margin({0, 0, 0, 16});  // Bottom margin only
     *
     * // Dialog spacing
     * dialog->set_padding({24, 24, 24, 24});  // Generous uniform padding
     * ```
     *
     * ## Impact on Layout
     *
     * - **Margin**: Affects element's measured size and position
     * - **Padding**: Reduces available space for children but not measured size
     * - **Total Space**: element_bounds = content + padding + margin
     *
     * @note Negative values are technically allowed but may cause rendering issues.
     *       Most layout strategies assume non-negative spacing.
     *
     * @see ui_element::set_margin() To apply as margin
     * @see ui_element::set_padding() To apply as padding
     */
    struct thickness {
        int left; ///< Left spacing in pixels
        int top; ///< Top spacing in pixels
        int right; ///< Right spacing in pixels
        int bottom; ///< Bottom spacing in pixels

        /**
         * @brief Calculate total horizontal spacing (left + right)
         *
         * @return Sum of left and right spacing
         *
         * @details
         * Useful for calculating total width impact or available content width.
         * For example, content_width = total_width - padding.horizontal().
         */
        [[nodiscard]] int horizontal() const noexcept { return left + right; }

        /**
         * @brief Calculate total vertical spacing (top + bottom)
         *
         * @return Sum of top and bottom spacing
         *
         * @details
         * Useful for calculating total height impact or available content height.
         * For example, content_height = total_height - padding.vertical().
         */
        [[nodiscard]] int vertical() const noexcept { return top + bottom; }

        /**
         * @brief Compare two thickness values for equality
         *
         * @param other The other thickness to compare against
         * @return true if all four sides have equal values
         *
         * @details
         * Performs exact integer comparison. Used for change detection
         * to avoid unnecessary layout invalidation.
         */
        bool operator==(const thickness& other) const noexcept {
            return left == other.left && top == other.top &&
                   right == other.right && bottom == other.bottom;
        }

        /**
         * @brief Compare two thickness values for inequality
         *
         * @param other The other thickness to compare against
         * @return true if any side has different values
         */
        bool operator!=(const thickness& other) const noexcept {
            return !(*this == other);
        }
    };

    /**
     * @class ui_element
     * @brief Base class for all UI elements in the layout tree
     *
     * @details
     * ui_element represents a node in the UI hierarchy. Elements form a tree structure
     * where parents own their children via std::unique_ptr. The layout system uses a
     * two-pass algorithm for optimal performance and clean separation of concerns.
     *
     * ## Two-Pass Layout Algorithm
     *
     * ### Phase 1: Measure (Bottom-Up)
     * - Starts with leaf elements and propagates upward
     * - Each element calculates its desired size based on:
     *   - Content size (text, images, etc.)
     *   - Children's measured sizes (if container)
     *   - Size constraints (min/max bounds)
     *   - Available space from parent
     * - Results are cached until invalidation
     *
     * ### Phase 2: Arrange (Top-Down)
     * - Starts with root element and propagates downward
     * - Each element receives final bounds from parent
     * - Element arranges children within content area
     * - Applies alignment and positioning rules
     *
     * ## Invalidation Strategy
     *
     * Smart invalidation minimizes recalculation:
     * - **Measure Invalidation**: Propagates upward (parents may resize)
     * - **Arrange Invalidation**: Propagates downward (children reposition)
     * - **Selective Updates**: Only affected branches recalculate
     * - **Cache Validation**: Results reused when inputs unchanged
     *
     * ## Memory Model
     *
     * ```
     * Root (unique_ptr)
     *  ├── Child1 (unique_ptr) ← parent (raw ptr to Root)
     *  │   └── Grandchild (unique_ptr) ← parent (raw ptr to Child1)
     *  └── Child2 (unique_ptr) ← parent (raw ptr to Root)
     * ```
     *
     * - **Ownership**: Parents own children via unique_ptr
     * - **References**: Children reference parents via raw pointers
     * - **No Cycles**: Ownership flows down, references flow up
     * - **RAII**: Automatic cleanup when parent destroyed
     *
     * ## Common Patterns
     *
     * ### Creating a Layout
     * ```cpp
     * auto root = std::make_unique<ui_element<Rect, Size>>(nullptr);
     * root->set_layout_strategy(std::make_unique<linear_layout<Rect, Size>>());
     * root->set_padding({10, 10, 10, 10});
     *
     * for (int i = 0; i < 3; ++i) {
     *     auto child = std::make_unique<ui_element<Rect, Size>>(nullptr);
     *     child->set_height_constraint({size_policy::fixed, 50, 50, 50});
     *     root->add_child(std::move(child));
     * }
     * ```
     *
     * ### Custom Element
     * ```cpp
     * class Button : public ui_element<Rect, Size> {
     * protected:
     *     TSize get_content_size() const override {
     *         // Return text dimensions
     *         return measure_text(m_text);
     *     }
     *
     *     void do_arrange(const TRect& bounds) override {
     *         // Position text/icon
     *         arrange_text(bounds);
     *     }
     * };
     * ```
     *
     * ### Dynamic Updates
     * ```cpp
     * // Change property and invalidate
     * element->set_width_constraint({size_policy::expand});
     *
     * // Manual invalidation for custom properties
     * void set_text(const std::string& text) {
     *     m_text = text;
     *     invalidate_measure();  // Text size changed
     * }
     * ```
     *
     * ## Performance Guidelines
     *
     * - **Batch Updates**: Modify multiple properties before layout
     * - **Avoid During Layout**: Don't modify tree during measure/arrange
     * - **Cache Content Size**: Override get_content_size() efficiently
     * - **Minimize Invalidation**: Only invalidate when truly needed
     * - **Use Constraints**: Prefer constraints over manual sizing
     *
     * ## Thread Safety
     *
     * ui_element is NOT thread-safe. All operations must occur on the same thread,
     * typically the UI thread. For multi-threaded scenarios:
     * - Use message passing to UI thread
     * - Apply external synchronization
     * - Consider immutable data patterns
     *
     * @tparam TRect Rectangle type satisfying RectLike concept
     * @tparam TSize Size type satisfying SizeLike concept
     *
     * @example Complete Window Layout
     * @code
     * // Create main window layout
     * auto window = std::make_unique<ui_element<SDL_Rect, SDL_Size>>(nullptr);
     * window->set_layout_strategy(std::make_unique<anchor_layout<SDL_Rect, SDL_Size>>());
     *
     * // Title bar at top
     * auto title_bar = create_title_bar();
     * window->add_child(std::move(title_bar));
     *
     * // Content area in center
     * auto content = std::make_unique<ui_element<SDL_Rect, SDL_Size>>(nullptr);
     * content->set_margin({0, 40, 0, 40});  // Leave space for title and status
     * window->add_child(std::move(content));
     *
     * // Status bar at bottom
     * auto status = create_status_bar();
     * window->add_child(std::move(status));
     *
     * // Perform layout
     * window->measure(1920, 1080);
     * window->arrange({0, 0, 1920, 1080});
     * @endcode
     *
     * @see layout_strategy For positioning algorithms
     * @see size_constraint For sizing configuration
     * @see thickness For margin and padding
     */
    template<RectLike TRect, SizeLike TSize>
    class ui_element {
        public:
            using ui_element_ptr = std::unique_ptr <ui_element>; ///< Smart pointer for owned children
            using layout_strategy_ptr = std::unique_ptr <layout_strategy <TRect, TSize>>; ///< Smart pointer for layout

            // Grant access to layout strategies for protected members
            friend class layout_strategy <TRect, TSize>;

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

            // Rule of Five: explicitly handle copy and move semantics
            /**
             * @brief Move constructor
             * @param other Element to move from
             */
            ui_element(ui_element&& other) noexcept = default;

            /**
             * @brief Move assignment operator
             * @param other Element to move from
             * @return Reference to this
             */
            ui_element& operator=(ui_element&& other) noexcept = default;

            /**
             * @brief Deleted copy constructor (elements use unique_ptr ownership)
             */
            ui_element(const ui_element&) = delete;

            /**
             * @brief Deleted copy assignment (elements use unique_ptr ownership)
             */
            ui_element& operator=(const ui_element&) = delete;

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
             * @brief Remove all child elements
             *
             * Clears the parent pointers of all children. Notifies the layout
             * strategy to clear any internal mappings. Invalidates the measure
             * cache of this element and all ancestors.
             */
            void clear_children();

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
            // Public Property Setters (with invalidation)
            // -----------------------------------------------------------------------

            /**
             * @brief Set width constraint
             * @param constraint New width constraint
             */
            void set_width_constraint(const size_constraint& constraint) {
                if (m_width_constraint != constraint) {
                    m_width_constraint = constraint;
                    invalidate_measure();
                }
            }

            /**
             * @brief Set height constraint
             * @param constraint New height constraint
             */
            void set_height_constraint(const size_constraint& constraint) {
                if (m_height_constraint != constraint) {
                    m_height_constraint = constraint;
                    invalidate_measure();
                }
            }

            /**
             * @brief Set horizontal alignment
             * @param align New horizontal alignment
             */
            void set_horizontal_align(horizontal_alignment align) {
                if (m_h_align != align) {
                    m_h_align = align;
                    invalidate_arrange();
                }
            }

            /**
             * @brief Set vertical alignment
             * @param align New vertical alignment
             */
            void set_vertical_align(vertical_alignment align) {
                if (m_v_align != align) {
                    m_v_align = align;
                    invalidate_arrange();
                }
            }

            /**
             * @brief Set margin
             * @param margin New margin values
             */
            void set_margin(const thickness& margin) {
                if (m_margin != margin) {
                    m_margin = margin;
                    invalidate_measure();
                }
            }

            /**
             * @brief Set padding
             * @param padding New padding values
             */
            void set_padding(const thickness& padding) {
                if (m_padding != padding) {
                    m_padding = padding;
                    invalidate_measure();
                }
            }

            /**
             * @brief Set z-order
             * @param z_index New z-order value
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
            const thickness& margin() const noexcept { return m_margin; }

            /**
             * @brief Get padding (const)
             * @return Const reference to padding
             */
            const thickness& padding() const noexcept { return m_padding; }

            /**
             * @brief Get z-index (const)
             * @return Z-index value
             */
            int z_order() const noexcept { return m_z_index; }

            // -----------------------------------------------------------------------
            // Public Interface for Layout Strategies (accessed via friend)
            // These are conceptually protected but made public due to template
            // instantiation issues with friend declarations across compilation units
            // -----------------------------------------------------------------------

            /**
             * @brief Get children container (for layout strategies)
             * @return Const reference to children vector
             */
            const std::vector <ui_element_ptr>& children() const noexcept { return m_children; }

            /**
             * @brief Get mutable children container (for arrange phase)
             * @return Reference to children vector
             */
            std::vector <ui_element_ptr>& mutable_children() noexcept { return m_children; }

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
             * @brief Tracks layout cache validity
             */
            enum class layout_state {
                valid, ///< Layout is up to date, cache is valid
                dirty ///< This element needs layout recalculation
            };

            /// Current measure cache state
            layout_state measure_state = layout_state::dirty;

            /// Current arrange cache state
            layout_state arrange_state = layout_state::dirty;

            /// Cached measure results (for performance)
            TSize m_last_measured_size = {};
            int m_last_available_width = -1;
            int m_last_available_height = -1;

            /// Visibility flag (hidden elements skip layout and rendering)
            bool m_visible = true;

            /// Enabled flag (for interaction, doesn't affect layout)
            bool m_enabled = true;

            /// Final positioned bounds (set during arrange phase)
            TRect m_bounds = {};

            /// Z-order index (lower values behind, higher values in front)
            int m_z_index = 0;

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
        // Strong exception guarantee: if push_back throws, nothing is modified
        m_children.push_back(std::move(child));
        // These operations are noexcept, safe to do after push_back succeeds
        m_children.back()->m_parent = this;
        invalidate_measure();
    }

    // ------------------------------------------------------------------------------
    template<RectLike TRect, SizeLike TSize>
    void ui_element <TRect, TSize>::clear_children() {
        // Notify layout strategy about clearing all children
        if (m_layout_strategy) {
            m_layout_strategy->on_children_cleared();
        }

        // Clear parent pointers
        for (auto& child : m_children) {
            child->m_parent = nullptr;
        }

        m_children.clear();
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
            // Notify layout strategy about child removal
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
    // Invalidation (Cache Management)
    // -------------------------------------------------------------------------------

    template<RectLike TRect, SizeLike TSize>
    void ui_element <TRect, TSize>::invalidate_measure() {
        // If already dirty, nothing to do (prevents infinite recursion)
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
        // If already dirty, nothing to do (prevents infinite recursion)
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
        // Check if bounds actually changed
        bool bounds_changed = (rect_utils::get_x(m_bounds) != rect_utils::get_x(final_bounds) ||
                               rect_utils::get_y(m_bounds) != rect_utils::get_y(final_bounds) ||
                               rect_utils::get_width(m_bounds) != rect_utils::get_width(final_bounds) ||
                               rect_utils::get_height(m_bounds) != rect_utils::get_height(final_bounds));

        m_bounds = final_bounds;

        // Skip re-arrangement only if states are valid AND bounds haven't changed
        if (!bounds_changed &&
            arrange_state == layout_state::valid &&
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
    void ui_element <TRect, TSize>::sort_children_by_z_index() {
        std::stable_sort(m_children.begin(), m_children.end(),
                         [](const ui_element_ptr& a, const ui_element_ptr& b) {
                             return a->m_z_index < b->m_z_index;
                         });
    }

    // -----------------------------------------------------------------------------------------------------
    template<RectLike TRect, SizeLike TSize>
    void ui_element <TRect, TSize>::update_child_order() {
        sort_children_by_z_index();
        invalidate_arrange();
    }

    // -------------------------------------------------------------------------------
    // Hit Testing
    // -------------------------------------------------------------------------------

    template<RectLike TRect, SizeLike TSize>
    ui_element <TRect, TSize>* ui_element <TRect, TSize>::hit_test(int x, int y) {
        // Not visible or not within bounds
        if (!m_visible || !rect_utils::contains(m_bounds, x, y)) {
            return nullptr;
        }

        // Check children in reverse order (highest z-index first)
        for (auto it = m_children.rbegin(); it != m_children.rend(); ++it) {
            if (ui_element* hit = (*it)->hit_test(x, y)) {
                return hit;
            }
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
            return m_layout_strategy->measure_children(
                static_cast <const ui_element*>(this), available_width, available_height);
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
