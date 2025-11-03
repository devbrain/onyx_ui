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
 * ## Recursion Safety
 *
 * Several methods use recursion to traverse the UI tree. All recursive operations
 * are bounded by the UI tree depth, which is typically shallow in practice:
 *
 * - **Typical depth**: 5-15 levels (most applications)
 * - **Complex applications**: 20-30 levels
 * - **Stress-tested**: 100 levels (verified in tests)
 * - **Theoretical maximum**: Limited by stack size (~1000-10000 levels depending on platform)
 *
 * **Recursive Functions:**
 * 1. `invalidate_measure()` - Tail recursion walking UP the parent chain
 * 2. `invalidate_arrange()` - Recursion propagating DOWN to children
 * 3. `hit_test()` - Depth-first search DOWN the tree
 *
 * All recursive functions have early termination conditions and are safe for
 * normal UI hierarchies. Stack overflow is not a practical concern for typical use.
 *
 * **Performance Characteristics:**
 * - Recursion depth = O(tree height), typically < 30
 * - Stack frame size is small (~100-200 bytes per level)
 * - Total stack usage typically < 10KB for deep hierarchies
 *
 * @note If building pathological hierarchies (>100 levels), consider flattening
 *       the structure or using a different layout approach.
 *
 * @tparam Backend The backend traits type (e.g., sdl_backend, glfw_backend)
 */

#pragma once

#include <vector>
#include <algorithm>
#include <limits>
#include <memory>
#include <utility>  // for std::exchange

#include <onyxui/concepts/backend.hh>
#include <onyxui/core/event_target.hh>
#include <onyxui/layout/layout_strategy.hh>
#include <onyxui/theming/themeable.hh>
#include <onyxui/utils/safe_math.hh>
#include <onyxui/core/rendering/render_context.hh>
#include <onyxui/core/rendering/draw_context.hh>
#include <onyxui/core/rendering/render_info.hh>

namespace onyxui {
    /**
     * @struct thickness
     * @brief Represents spacing on all four sides (margin or padding)
     */
    struct thickness {
        int left; ///< Left spacing in pixels
        int top; ///< Top spacing in pixels
        int right; ///< Right spacing in pixels
        int bottom; ///< Bottom spacing in pixels

        /**
         * @brief Calculate total horizontal spacing (left + right)
         */
        [[nodiscard]] int horizontal() const noexcept { return left + right; }

        /**
         * @brief Calculate total vertical spacing (top + bottom)
         */
        [[nodiscard]] int vertical() const noexcept { return top + bottom; }

        /**
         * @brief Create uniform thickness on all sides
         * @param value The spacing value for all sides
         * @return thickness with equal spacing on all sides
         *
         * @example
         * @code
         * element->set_padding(thickness::all(10));  // 10px on all sides
         * @endcode
         */
        [[nodiscard]] static constexpr thickness all(int value) noexcept {
            return {value, value, value, value};
        }

        /**
         * @brief Create symmetric thickness (horizontal and vertical)
         * @param horizontal The spacing for left and right sides
         * @param vertical The spacing for top and bottom sides
         * @return thickness with symmetric spacing
         *
         * @example
         * @code
         * // 20px left/right, 10px top/bottom
         * element->set_margin(thickness::symmetric(20, 10));
         * @endcode
         */
        [[nodiscard]] static constexpr thickness symmetric(int horizontal, int vertical) noexcept {
            return {horizontal, vertical, horizontal, vertical};
        }

        /**
         * @brief Create thickness with no spacing
         * @return thickness with all sides set to zero
         *
         * @example
         * @code
         * element->set_padding(thickness::none());  // No padding
         * @endcode
         */
        [[nodiscard]] static constexpr thickness none() noexcept {
            return {0, 0, 0, 0};
        }

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
    class ui_element : public event_target <Backend>, public themeable <Backend> {
        static_assert(UIBackend <Backend>,
                      "Template parameter must satisfy UIBackend concept. "
                      "Backend must provide rect_type, size_type, point_type, and event types.");

        public:
            // Backend type (for template metaprogramming)
            using backend_type = Backend;

            // Type aliases from backend
            using rect_type = Backend::rect_type;
            using size_type = Backend::size_type;
            using point_type = Backend::point_type;
            using event_type = Backend::event_type;
            using theme_type = themeable <Backend>::theme_type;
            using renderer_type = Backend::renderer_type;
            // Convenience aliases
            using ui_element_ptr = std::unique_ptr <ui_element>;
            using layout_strategy_ptr = std::unique_ptr <layout_strategy <Backend>>;

            // Grant access to layout strategies
            friend class layout_strategy <Backend>;

            /**
             * @brief Construct a UI element
             * @param parent Pointer to the parent element (or nullptr for root)
             */
            explicit ui_element(ui_element* parent);

            /**
             * @brief Virtual destructor for proper cleanup
             *
             * @details
             * Nulls out all children's parent pointers before destruction as a
             * defensive measure. This helps catch bugs where code might hold raw
             * pointers to children and access them after parent destruction.
             *
             * @note Exception safety: No-throw guarantee (noexcept)
             */
            ~ui_element() noexcept override {
                // Defensive: null out children's parent pointers before destruction
                // This catches bugs where someone holds a child pointer after parent dies
                for (auto& child : m_children) {
                    if (child) {
                        child->m_parent = nullptr;
                    }
                }
            }

            // Rule of Five
            /**
             * @brief Move constructor with strong exception safety
             *
             * @note Exception Safety: Uses member initializer list for all moves,
             *       ensuring strong exception guarantee (all-or-nothing semantics).
             *       If any move throws, the object remains in its original state.
             *
             * @note noexcept specification is conditional on base classes to propagate
             *       their exception guarantees. All member types (unique_ptr, vector, PODs)
             *       are nothrow movable.
             *
             * @note Updates children's parent pointers to maintain tree consistency
             */
            ui_element(ui_element&& other) noexcept(
                std::is_nothrow_move_constructible_v<event_target<Backend>> &&
                std::is_nothrow_move_constructible_v<themeable<Backend>>);

            /**
             * @brief Move assignment operator with strong exception safety
             *
             * @note Exception Safety: All member moves are noexcept. If base class
             *       move assignment throws, the object state may be partially moved.
             *       Conditional noexcept propagates base class guarantees.
             *
             * @note Updates children's parent pointers to maintain tree consistency
             */
            ui_element& operator=(ui_element&& other) noexcept(
                std::is_nothrow_move_assignable_v<event_target<Backend>> &&
                std::is_nothrow_move_assignable_v<themeable<Backend>>);

            ui_element(const ui_element&) = delete;
            ui_element& operator=(const ui_element&) = delete;

            /**
             * @brief Add a child element (takes ownership)
             * @param child The child element to add (must be non-null)
             *
             * @exception std::bad_alloc If vector reallocation fails
             * @note Exception safety: Strong guarantee - if exception thrown, no state changes occur
             * @note If child is null, this is a no-op (no exception thrown)
             */
            void add_child(ui_element_ptr child);

            /**
             * @brief Emplace a child element with automatic backend deduction
             * @tparam WidgetTemplate Widget template class (e.g., onyxui::label, onyxui::button)
             * @tparam Args Constructor argument types
             * @param args Arguments to forward to the widget constructor
             * @return Pointer to the newly created child
             *
             * @exception std::bad_alloc If widget construction or add_child fails
             * @note Exception safety: Strong guarantee - if exception thrown, no state changes occur
             *
             * @details
             * Creates a widget in-place, automatically using the parent's Backend type.
             * This eliminates the need to specify the backend explicitly.
             *
             * @example
             * @code
             * auto root = std::make_unique<panel<conio_backend>>();
             * root->emplace_child<onyxui::label>("Hello");  // Backend deduced!
             * auto* btn = root->emplace_child<onyxui::button>("Click");
             * @endcode
             */
            template<template<UIBackend> class WidgetTemplate, typename... Args>
            auto* emplace_child(Args&&... args) {
                using WidgetType = WidgetTemplate<Backend>;
                auto child = std::make_unique<WidgetType>(std::forward<Args>(args)...);
                auto* ptr = child.get();
                add_child(std::move(child));
                return ptr;
            }

            /**
             * @brief Remove a child element (returns ownership)
             * @param child Pointer to the child to remove
             * @return The removed child, or nullptr if not found
             *
             * @exception Any exception thrown by layout_strategy::on_child_removed()
             * @note Exception safety: Basic guarantee - child removed from vector even if cleanup throws
             * @note Child's parent pointer is reset to nullptr before returning
             */
            [[nodiscard]] ui_element_ptr remove_child(ui_element* child);

            /**
             * @brief Remove all child elements
             *
             * @note Exception safety: No-throw guarantee (noexcept)
             * @note All children's parent pointers are reset to nullptr
             * @note Layout strategy's on_children_cleared() is called before clearing
             */
            void clear_children() noexcept;

            /**
             * @brief Invalidate the measure cache
             *
             * @note Exception safety: No-throw guarantee (noexcept)
             * @note Recursively propagates up the parent chain
             * @note Only modifies state flags, no allocations
             */
            void invalidate_measure() noexcept;

            /**
             * @brief Invalidate the arrange cache
             *
             * @note Exception safety: No-throw guarantee (noexcept)
             * @note Recursively propagates down to all children
             * @note Only modifies state flags, no allocations
             */
            void invalidate_arrange() noexcept;

            /**
             * @brief Measure phase: calculate desired size
             * @param available_width Maximum width available (pixels)
             * @param available_height Maximum height available (pixels)
             * @return The measured size for this element
             *
             * @exception Any exception thrown by do_measure() override
             * @exception Any exception thrown by layout_strategy::measure_children()
             * @note Exception safety: Basic guarantee - cache may be partially updated if exception thrown
             * @note Results are cached - repeated calls with same parameters return cached value
             */
            [[nodiscard]] size_type measure(int available_width, int available_height);

            /**
             * @brief Measure element with no constraints (natural size)
             * @return The natural/preferred size of the element
             *
             * @details
             * Convenience method that measures the element with unconstrained space.
             * This returns the element's preferred size when it has unlimited room.
             * Useful for determining the minimum size needed to display content
             * without clipping or wrapping.
             *
             * @note Equivalent to measure(std::numeric_limits<int>::max(), std::numeric_limits<int>::max())
             */
            [[nodiscard]] size_type measure_unconstrained() {
                return measure(std::numeric_limits<int>::max(),
                              std::numeric_limits<int>::max());
            }

            /**
             * @brief Arrange phase: assign final bounds
             * @param final_bounds The allocated rectangle for this element
             *
             * @exception Any exception thrown by do_arrange() override
             * @exception Any exception thrown by layout_strategy::arrange_children()
             * @note Exception safety: Basic guarantee - bounds updated before arrange, children may be partially arranged
             * @note Skipped if bounds unchanged and layout state is valid
             */
            void arrange(const rect_type& final_bounds);

            /**
             * @brief Sort children by their z_index values
             *
             * @exception Any exception thrown by std::stable_sort (typically std::bad_alloc)
             * @note Exception safety: Basic guarantee - children may be partially sorted if exception thrown
             * @note Uses stable sort to maintain relative order of elements with equal z_index
             */
            void sort_children_by_z_index();

            /**
             * @brief Update child z-ordering
             *
             * @exception Any exception thrown by std::stable_sort (typically std::bad_alloc)
             * @note Exception safety: Basic guarantee - calls sort_children_by_z_index() then invalidate_arrange()
             * @note Convenience method that combines sorting and layout invalidation
             */
            void update_child_order();

            /**
             * @brief Perform hit testing to find element at coordinates
             * @param x X coordinate in pixels
             * @param y Y coordinate in pixels
             * @return Pointer to the deepest visible element at (x,y), or nullptr if none found
             *
             * @note Exception safety: No-throw guarantee (no allocations, simple traversal)
             * @note Searches in reverse z-order (highest z-index checked first)
             * @note Only visible elements are considered for hit testing
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
                    // Mark dirty before changing visibility
                    // This ensures hidden elements get cleared
                    if (m_visible) {
                        mark_dirty();
                    }

                    m_visible = visible;
                    invalidate_arrange();

                    // Mark dirty after becoming visible
                    // This ensures newly visible elements get drawn
                    if (m_visible) {
                        mark_dirty();
                    }
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

            /**
       * @brief Render this element and its children with proper clipping
       * @param renderer The backend renderer instance
       * @param theme Global theme pointer (REQUIRED - must not be nullptr)
       * @throws std::runtime_error if theme is nullptr
       */
            void render(renderer_type& renderer, const theme_type* theme) {
                if (!theme) {
                    throw std::runtime_error("render() called with null theme - theme is required for rendering");
                }
                // Create default parent style from theme
                resolved_style<Backend> default_style = resolved_style<Backend>::from_theme(*theme);
                render(renderer, {}, theme, default_style);  // Empty vector means "render everything"
            }

            /**
       * @brief Render this element and its children with dirty rectangle optimization
       * @param renderer The backend renderer instance
       * @param dirty_regions Regions that need redrawing (empty = render everything)
       * @param theme Global theme pointer (may be nullptr - fetched once by ui_handle and passed down tree)
       * @param parent_style Parent's resolved style (accumulated top-down, no recursion!)
       *
       * @details
       * Uses dirty rectangle optimization to skip rendering elements that don't
       * intersect with any dirty region. This significantly improves performance
       * when only small portions of the UI change.
       *
       * The theme is fetched once at the top level (ui_handle::display()) and
       * passed down through the widget tree to avoid repeated lookups.
       *
       * Parent style is passed down during top-down traversal, eliminating the need
       * for recursive parent lookups during style resolution. This is O(1) per widget
       * instead of O(depth), following the same pattern used by browser rendering engines.
       */
            void render(renderer_type& renderer, const std::vector<rect_type>& dirty_regions,
                       const theme_type* theme, const resolved_style<Backend>& parent_style,
                       point_type const& offset = point_type{0, 0}) {
                if (!m_visible) {
                    return;
                }

                // Resolve style by merging parent_style with my overrides (no recursion!)
                auto style = this->resolve_style(theme, parent_style);

                // Bounds are now RELATIVE to parent's content area
                // Add accumulated offset to get absolute screen position
                point_type absolute_pos{
                    rect_utils::get_x(m_bounds) + point_utils::get_x(offset),
                    rect_utils::get_y(m_bounds) + point_utils::get_y(offset)
                };

                size_type size;
                size_utils::set_size(size,
                    rect_utils::get_width(m_bounds),
                    rect_utils::get_height(m_bounds));

                // Create draw context with resolved style, ABSOLUTE position, size, dirty regions, and theme
                // The absolute_pos accounts for all parent content area offsets
                draw_context<Backend> ctx(renderer, style, absolute_pos, size, dirty_regions, theme);

                // Skip rendering if this element doesn't intersect with any dirty region
                // TODO: Need to check absolute bounds, not relative
                rect_type absolute_bounds;
                rect_utils::make_absolute_bounds(absolute_bounds, absolute_pos, m_bounds);

                if (!ctx.should_render(absolute_bounds)) {
                    // Early return - don't render this element or its children
                    return;
                }

                // Render this element (style already resolved in ctx)
                do_render(ctx);

                // Set clip rect for children (content area only)
                rect_type const content_area = get_content_area();

                // Calculate absolute clip rect
                // content_area position is now relative to widget's bounds origin, so add absolute_pos
                rect_type absolute_clip;
                rect_utils::set_bounds(absolute_clip,
                    point_utils::get_x(absolute_pos) + rect_utils::get_x(content_area),
                    point_utils::get_y(absolute_pos) + rect_utils::get_y(content_area),
                    rect_utils::get_width(content_area),
                    rect_utils::get_height(content_area));

                // Push clip rect before rendering children
                renderer.push_clip(absolute_clip);

                // Render children in z-order (they're already clipped)
                // Children are arranged at RELATIVE coordinates (0,0 = top-left of content area)
                // Accumulate offset for children: widget's absolute position + content area offset
                point_type child_offset{
                    point_utils::get_x(absolute_pos) + rect_utils::get_x(content_area),
                    point_utils::get_y(absolute_pos) + rect_utils::get_y(content_area)
                };

                for (const auto& child : m_children) {
                    child->render(renderer, dirty_regions, theme, style, child_offset);
                }

                // Restore previous clip rect
                renderer.pop_clip();
            }

            // -----------------------------------------------------------------------
            // Public Accessors
            // -----------------------------------------------------------------------

            [[nodiscard]] const rect_type& bounds() const noexcept { return m_bounds; }
            [[nodiscard]] bool is_visible() const noexcept { return m_visible; }
            [[nodiscard]] const size_constraint& width_constraint() const noexcept { return m_width_constraint; }
            [[nodiscard]] const size_constraint& height_constraint() const noexcept { return m_height_constraint; }
            [[nodiscard]] horizontal_alignment horizontal_align() const noexcept { return m_h_align; }
            [[nodiscard]] vertical_alignment vertical_align() const noexcept { return m_v_align; }
            [[nodiscard]] const thickness& margin() const noexcept { return m_margin; }
            [[nodiscard]] const thickness& padding() const noexcept { return m_padding; }
            [[nodiscard]] int z_order() const noexcept { return m_z_index; }

            // For layout strategies
            [[nodiscard]] const std::vector <ui_element_ptr>& children() const noexcept { return m_children; }
            [[nodiscard]] std::vector <ui_element_ptr>& mutable_children() noexcept { return m_children; }
            [[nodiscard]] const size_type& last_measured_size() const noexcept { return m_last_measured_size; }
            [[nodiscard]] horizontal_alignment h_align() const noexcept { return m_h_align; }
            [[nodiscard]] vertical_alignment v_align() const noexcept { return m_v_align; }
            [[nodiscard]] const size_constraint& w_constraint() const noexcept { return m_width_constraint; }
            [[nodiscard]] const size_constraint& h_constraint() const noexcept { return m_height_constraint; }
            [[nodiscard]] ui_element* parent() const noexcept { return m_parent; }
            [[nodiscard]] bool has_layout_strategy() const noexcept { return m_layout_strategy != nullptr; }

            /**
             * @brief Mark this element's bounds as dirty (needs redraw)
             *
             * @details
             * Marks the current bounds as needing to be cleared/redrawn.
             * This propagates up to the root element which collects dirty regions.
             */
            void mark_dirty();

            /**
             * @brief Mark a specific region as dirty (internal use)
             * @param region The region that needs redrawing
             */
            void mark_dirty_region(const rect_type& region);

            /**
             * @brief Get and clear accumulated dirty regions (root only)
             * @return Vector of dirty regions that need clearing
             *
             * @details
             * Only valid for root elements. Returns all accumulated dirty
             * regions and clears the internal list.
             */
            [[nodiscard]] std::vector<rect_type> get_and_clear_dirty_regions();

        protected:
            // -----------------------------------------------------------------------
            // Protected Virtual Methods (from event_target)
            // -----------------------------------------------------------------------

            /**
             * @brief Check if a point is inside this element
             * Implementation for event_target's pure virtual method
             *
             * NOTE: This is only correct for root elements at (0,0).
             * For nested elements, use the full widget hierarchy hit_test().
             */
            [[nodiscard]] bool is_inside(int x, int y) const override {
                // For root elements, relative = absolute
                // For nested elements, this is incorrect but required by event_target interface
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
            [[nodiscard]] virtual size_type get_content_size() const {
                size_type s = {};
                size_utils::set_size(s, 0, 0);
                return s;
            }

            /**
             * @brief Get parent for CSS-style theme inheritance
             *
             * @details Enables inherited properties (background_color, font, etc.)
             *          to walk up the parent chain.
             */
            [[nodiscard]] const themeable<Backend>* get_themeable_parent() const override {
                return m_parent;
            }

            /**
             * @brief Render this element using render context (visitor pattern)
             * @param ctx Render context (draw_context for rendering, measure_context for sizing)
             *
             * @details
             * Override this method to implement custom rendering. The context
             * handles both rendering (draw_context) and measurement (measure_context),
             * eliminating duplication between rendering and sizing logic.
             *
             * **Important**: This method is const to allow measurement from const widgets.
             */
            virtual void do_render([[maybe_unused]] render_context<Backend>& ctx) const {
                // Base: nothing to render
            }

            /**
             * @brief Calculate the content area (bounds minus margin and padding)
             * @note noexcept - uses only safe arithmetic operations
             * @note virtual - subclasses can override to account for borders
             */
            virtual rect_type get_content_area() const noexcept {
                // RELATIVE COORDINATES: Position is relative to widget's own bounds origin (0, 0)
                // NOT relative to parent - that's handled during rendering via offset accumulation
                const int x = safe_math::add_clamped(m_margin.left, m_padding.left);
                const int y = safe_math::add_clamped(m_margin.top, m_padding.top);

                // Calculate dimensions with safe subtraction
                const int total_h_spacing = safe_math::add_clamped(m_margin.horizontal(), m_padding.horizontal());
                const int total_v_spacing = safe_math::add_clamped(m_margin.vertical(), m_padding.vertical());

                int w = safe_math::subtract_clamped(rect_utils::get_width(m_bounds), total_h_spacing);
                int h = safe_math::subtract_clamped(rect_utils::get_height(m_bounds), total_v_spacing);

                // Clamp to non-negative
                w = std::max(0, w);
                h = std::max(0, h);

                rect_type content_area;
                rect_utils::set_bounds(content_area, x, y, w, h);
                return content_area;
            }

        private:

            // Parent element (non-owning)
            ui_element* m_parent = nullptr;

            // Children (owned)
            std::vector <ui_element_ptr> m_children;

            // Layout strategy (owned)
            layout_strategy_ptr m_layout_strategy;

            // Layout state
            enum class layout_state : std::uint8_t {
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

            // Dirty region tracking (only used by root element)
            std::vector<rect_type> m_dirty_regions;
    };

    // ===============================================================================
    // Implementation
    // ===============================================================================

    template<UIBackend Backend>
    ui_element <Backend>::ui_element(ui_element* parent)
        : event_target <Backend>(), m_parent(parent) {
    }

    template<UIBackend Backend>
    ui_element<Backend>::ui_element(ui_element&& other) noexcept(
        std::is_nothrow_move_constructible_v<event_target<Backend>> &&
        std::is_nothrow_move_constructible_v<themeable<Backend>>)
        // Initialize base classes first (required order), but only move the base class parts
        : event_target<Backend>(std::move(static_cast<event_target<Backend>&>(other)))
        , themeable<Backend>(std::move(static_cast<themeable<Backend>&>(other)))
        // Now move member variables (after bases, as compiler requires)
        // All moves in initializer list ensure strong exception safety (all-or-nothing)
        , m_parent(std::exchange(other.m_parent, nullptr))
        , m_children(std::move(other.m_children))
        , m_layout_strategy(std::move(other.m_layout_strategy))
        , measure_state(other.measure_state)
        , arrange_state(other.arrange_state)
        , m_last_measured_size(std::move(other.m_last_measured_size))
        , m_last_available_width(other.m_last_available_width)
        , m_last_available_height(other.m_last_available_height)
        , m_visible(other.m_visible)
        , m_bounds(std::move(other.m_bounds))
        , m_z_index(other.m_z_index)
        , m_width_constraint(other.m_width_constraint)
        , m_height_constraint(other.m_height_constraint)
        , m_h_align(other.m_h_align)
        , m_v_align(other.m_v_align)
        , m_margin(other.m_margin)
        , m_padding(other.m_padding)
    {
        // CRITICAL: Fix children's parent pointers
        // This is the only operation in the body - if it throws, object is already constructed
        // so this doesn't affect exception safety of the constructor itself
        for (auto& child : m_children) {
            child->m_parent = this;
        }
    }

    template<UIBackend Backend>
    ui_element<Backend>& ui_element<Backend>::operator=(ui_element&& other) noexcept(
        std::is_nothrow_move_assignable_v<event_target<Backend>> &&
        std::is_nothrow_move_assignable_v<themeable<Backend>>) {
        if (this != &other) {
            // Move base classes (only move base parts, not entire derived object)
            event_target<Backend>::operator=(std::move(static_cast<event_target<Backend>&>(other)));
            themeable<Backend>::operator=(std::move(static_cast<themeable<Backend>&>(other)));

            // Move members - all member moves are noexcept (unique_ptr, vector, PODs)
            m_parent = std::exchange(other.m_parent, nullptr);
            m_children = std::move(other.m_children);
            m_layout_strategy = std::move(other.m_layout_strategy);
            measure_state = other.measure_state;
            arrange_state = other.arrange_state;
            m_last_measured_size = std::move(other.m_last_measured_size);
            m_last_available_width = other.m_last_available_width;
            m_last_available_height = other.m_last_available_height;
            m_visible = other.m_visible;
            m_bounds = std::move(other.m_bounds);
            m_z_index = other.m_z_index;
            m_width_constraint = other.m_width_constraint;
            m_height_constraint = other.m_height_constraint;
            m_h_align = other.m_h_align;
            m_v_align = other.m_v_align;
            m_margin = other.m_margin;
            m_padding = other.m_padding;

            // CRITICAL: Fix children's parent pointers
            for (auto& child : m_children) {
                child->m_parent = this;
            }
        }
        return *this;
    }

    // -------------------------------------------------------------------------------
    // Tree Management
    // -------------------------------------------------------------------------------

    template<UIBackend Backend>
    void ui_element <Backend>::add_child(ui_element_ptr child) {
        if (child) {
            // Push to vector first - if this throws, no state has been modified
            m_children.push_back(std::move(child));
            // Now safe to modify the child's parent pointer
            m_children.back()->m_parent = this;
            invalidate_measure();
        }
    }

    template<UIBackend Backend>
    void ui_element <Backend>::clear_children() noexcept {
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
    typename ui_element <Backend>::ui_element_ptr
    ui_element <Backend>::remove_child(ui_element* child) {
        auto it = std::find_if(m_children.begin(), m_children.end(),
                               [child](const ui_element_ptr& ptr) {
                                   return ptr.get() == child;
                               });

        if (it != m_children.end()) {
            // Exception safety: Remove child from vector FIRST, then call cleanup.
            // This ensures the child is removed even if layout_strategy->on_child_removed() throws.
            // Basic guarantee: child removed from vector even if cleanup throws.
            ui_element_ptr removed = std::move(*it);
            m_children.erase(it);
            removed->m_parent = nullptr;

            // Cleanup can throw, but child is already removed at this point
            if (m_layout_strategy) {
                m_layout_strategy->on_child_removed(removed.get());
            }

            invalidate_measure();
            return removed;
        }
        return nullptr;
    }

    // -------------------------------------------------------------------------------
    // Invalidation
    // -------------------------------------------------------------------------------

    // NOLINTNEXTLINE(misc-no-recursion)
    // Note: Tail recursion walking up parent chain, bounded by UI tree height
    template<UIBackend Backend>
    void ui_element <Backend>::invalidate_measure() noexcept {
        if (measure_state != layout_state::valid) {
            return;
        }

        measure_state = layout_state::dirty;
        arrange_state = layout_state::dirty;

        if (m_parent) {
            m_parent->invalidate_measure();
        }
    }

    // NOLINTNEXTLINE(misc-no-recursion)
    // Note: Recursive tree traversal propagating down to children, bounded by UI tree height
    template<UIBackend Backend>
    void ui_element <Backend>::invalidate_arrange() noexcept {
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
    typename ui_element <Backend>::size_type
    ui_element <Backend>::measure(int available_width, int available_height) {
        // Check cache
        if (measure_state == layout_state::valid &&
            m_last_available_width == available_width &&
            m_last_available_height == available_height) {
            return m_last_measured_size;
        }

        // Account for margin using safe subtraction
        int content_width = safe_math::subtract_clamped(available_width, m_margin.horizontal());
        int content_height = safe_math::subtract_clamped(available_height, m_margin.vertical());
        content_width = std::max(0, content_width);
        content_height = std::max(0, content_height);

        // Measure content
        size_type measured = do_measure(content_width, content_height);

        // Add margin back using safe addition
        int meas_w = safe_math::add_clamped(size_utils::get_width(measured), m_margin.horizontal());
        int meas_h = safe_math::add_clamped(size_utils::get_height(measured), m_margin.vertical());

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
    void ui_element <Backend>::arrange(const rect_type& final_bounds) {
        // Check if bounds changed
        const bool bounds_changed = !rect_utils::equal(m_bounds, final_bounds);

        // Mark old bounds as dirty if they changed
        if (bounds_changed && m_visible) {
            mark_dirty();
        }

        m_bounds = final_bounds;

        if (!bounds_changed &&
            arrange_state == layout_state::valid &&
            measure_state == layout_state::valid) {
            return;
        }

        // Calculate content area using safe arithmetic
        // RELATIVE COORDINATES: content_area position is relative to widget's own bounds origin
        rect_type content_area;

        // Position is relative to bounds origin (0, 0), not absolute
        const int x = safe_math::add_clamped(m_margin.left, m_padding.left);
        const int y = safe_math::add_clamped(m_margin.top, m_padding.top);

        // Calculate dimensions with safe subtraction
        const int total_h_spacing = safe_math::add_clamped(m_margin.horizontal(), m_padding.horizontal());
        const int total_v_spacing = safe_math::add_clamped(m_margin.vertical(), m_padding.vertical());

        int w = safe_math::subtract_clamped(rect_utils::get_width(final_bounds), total_h_spacing);
        int h = safe_math::subtract_clamped(rect_utils::get_height(final_bounds), total_v_spacing);

        // Clamp to non-negative
        w = std::max(0, w);
        h = std::max(0, h);

        rect_utils::set_bounds(content_area, x, y, w, h);

        do_arrange(content_area);
        arrange_state = layout_state::valid;
    }

    // -------------------------------------------------------------------------------
    // Z-Order
    // -------------------------------------------------------------------------------

    template<UIBackend Backend>
    void ui_element <Backend>::sort_children_by_z_index() {
        std::stable_sort(m_children.begin(), m_children.end(),
                         [](const ui_element_ptr& a, const ui_element_ptr& b) {
                             return a->m_z_index < b->m_z_index;
                         });
    }

    template<UIBackend Backend>
    void ui_element <Backend>::update_child_order() {
        sort_children_by_z_index();
        invalidate_arrange();
    }

    // -------------------------------------------------------------------------------
    // Hit Testing
    // -------------------------------------------------------------------------------

    // NOLINTNEXTLINE(misc-no-recursion)
    // Note: Recursive tree traversal for hit testing, bounded by UI tree height
    template<UIBackend Backend>
    ui_element <Backend>* ui_element <Backend>::hit_test(int x, int y) {
        if (!m_visible) {
            return nullptr;
        }

        // RELATIVE COORDINATES: Calculate absolute bounds for hit testing
        // For root elements, m_bounds is at (0,0) so relative = absolute
        // For nested elements, we need to account for parent positions
        // This implementation assumes root element is at screen (0,0)

        // Calculate absolute bounds (assuming parent offset has been applied)
        rect_type absolute_bounds = m_bounds;  // For root, this is correct

        if (!rect_utils::contains(absolute_bounds, x, y)) {
            return nullptr;
        }

        // For children, convert absolute point to relative coordinates
        // Children are positioned relative to our content area
        rect_type content_area = get_content_area();
        int const child_offset_x = rect_utils::get_x(content_area);
        int const child_offset_y = rect_utils::get_y(content_area);

        // Convert absolute coordinates to relative for children
        int const child_x = x - child_offset_x;
        int const child_y = y - child_offset_y;

        // Check children in reverse order (highest z-index first)
        for (auto it = m_children.rbegin(); it != m_children.rend(); ++it) {
            if (ui_element* hit = (*it)->hit_test(child_x, child_y)) {
                return hit;
            }
        }

        return this;
    }

    // -------------------------------------------------------------------------------
    // Virtual Methods
    // -------------------------------------------------------------------------------

    template<UIBackend Backend>
    typename ui_element <Backend>::size_type
    ui_element <Backend>::do_measure(int available_width, int available_height) {
        if (m_layout_strategy) {
            // Subtract padding from available space before passing to layout strategy
            // (margin is already subtracted by measure())
            const int content_width = safe_math::subtract_clamped(available_width, m_padding.horizontal());
            const int content_height = safe_math::subtract_clamped(available_height, m_padding.vertical());

            size_type const content_size = m_layout_strategy->measure_children(
                static_cast <const ui_element*>(this),
                std::max(0, content_width),
                std::max(0, content_height));

            // Add padding back to the measured size
            int const w = safe_math::add_clamped(size_utils::get_width(content_size), m_padding.horizontal());
            int const h = safe_math::add_clamped(size_utils::get_height(content_size), m_padding.vertical());

            size_type result{};
            size_utils::set_size(result, w, h);
            return result;
        }
        return get_content_size();
    }

    template<UIBackend Backend>
    void ui_element <Backend>::do_arrange(const rect_type& final_bounds) {
        (void)final_bounds;  // Mark as used (bounds already set in arrange())
        if (m_layout_strategy) {
            // Use content area instead of full bounds to account for borders/padding
            m_layout_strategy->arrange_children(this, get_content_area());
        }
    }

    // -------------------------------------------------------------------------------
    // Dirty Region Tracking
    // -------------------------------------------------------------------------------

    template<UIBackend Backend>
    void ui_element<Backend>::mark_dirty() {
        // Convert relative bounds to absolute before marking dirty
        // Walk up parent chain to accumulate absolute position
        int abs_x = rect_utils::get_x(m_bounds);
        int abs_y = rect_utils::get_y(m_bounds);

        ui_element* current_parent = m_parent;
        while (current_parent) {
            // Add parent's content area offset
            rect_type parent_content = current_parent->get_content_area();
            abs_x += rect_utils::get_x(parent_content);
            abs_y += rect_utils::get_y(parent_content);
            current_parent = current_parent->m_parent;
        }

        // Create absolute bounds rectangle
        rect_type absolute_bounds;
        rect_utils::make_absolute_bounds(absolute_bounds, abs_x, abs_y, m_bounds);

        mark_dirty_region(absolute_bounds);
    }

    template<UIBackend Backend>
    void ui_element<Backend>::mark_dirty_region(const rect_type& region) {
        // Region is expected to be in ABSOLUTE coordinates
        // Propagate up to root
        if (m_parent) {
            m_parent->mark_dirty_region(region);
        } else {
            // We're the root - store the absolute dirty region
            m_dirty_regions.push_back(region);
        }
    }

    template<UIBackend Backend>
    std::vector<typename ui_element<Backend>::rect_type>
    ui_element<Backend>::get_and_clear_dirty_regions() {
        auto regions = std::move(m_dirty_regions);
        m_dirty_regions.clear();
        return regions;
    }
}
