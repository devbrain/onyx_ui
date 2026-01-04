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
#include <onyxui/core/signal.hh>
#include <onyxui/core/types.hh>
#include <onyxui/core/geometry.hh>
#include <onyxui/events/hit_test_path.hh>
#include <onyxui/geometry/coordinates.hh>
#include <onyxui/layout/layout_strategy.hh>
#include <onyxui/theming/themeable.hh>
#include <onyxui/utils/safe_math.hh>
#include <onyxui/core/rendering/render_context.hh>
#include <onyxui/core/rendering/draw_context.hh>
#include <onyxui/core/rendering/render_info.hh>
#include <onyxui/core/backend_metrics.hh>

namespace onyxui {
    /**
     * @brief Type alias for thickness (uses logical units)
     * @details This is an alias for logical_thickness from geometry.hh
     */
    using thickness = logical_thickness;

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

            // ================================================================
            // Signals
            // ================================================================

            /**
             * @brief Emitted when this element is about to be destroyed
             *
             * @details
             * This signal is emitted at the very start of the destructor, BEFORE
             * any child teardown or cleanup occurs. Services holding raw pointers
             * to elements (e.g., input_manager, layer_manager) should connect to
             * this signal and clear their pointers when notified.
             *
             * **Handler requirements:**
             * - Must NOT call back into the element being destroyed
             * - Must NOT assume children are still intact
             * - Should only clear/null out pointers, not perform complex operations
             *
             * **Move semantics:**
             * Moving a ui_element invalidates all existing connections to this
             * signal (they will safely no-op on disconnect). The moved-to element
             * gets a fresh signal with no connections - slots are NOT transferred.
             * This ensures handlers with captured pointers to the old element
             * cannot be invoked after the move. In practice, elements are
             * heap-allocated via unique_ptr and never moved directly.
             *
             * @param ui_element* Pointer to the element being destroyed
             */
            signal<ui_element*> destroying;

            // ================================================================
            // Construction / Destruction
            // ================================================================

            /**
             * @brief Construct a UI element
             * @param parent Pointer to the parent element (or nullptr for root)
             */
            explicit ui_element(ui_element* parent);

            /**
             * @brief Virtual destructor for proper cleanup
             *
             * @details
             * Emits the `destroying` signal FIRST, before any teardown, to notify
             * services holding raw pointers to this element (e.g., input_manager,
             * layer_manager). Then nulls out children's parent pointers as a
             * defensive measure.
             *
             * @note Exception safety: No-throw guarantee (noexcept)
             * @note Signal handlers must NOT call back into this element or assume
             *       children are still intact - this is a finalization notification.
             */
            ~ui_element() noexcept override {
                // Notify observers FIRST - before any teardown
                // Services holding raw pointers must clear them here
                destroying.emit(this);

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
             * @brief Check if element needs measure pass
             * @return true if measure cache is invalid and needs recalculation
             *
             * @details
             * Returns true when:
             * - Element was just created (never measured)
             * - invalidate_measure() was called
             * - Properties affecting size changed (width, height, margin, etc.)
             *
             * Useful for detecting if layout was invalidated during an arrange pass
             * (e.g., when scrollbar visibility changes), allowing conditional re-layout.
             */
            [[nodiscard]] bool needs_measure() const noexcept {
                return measure_state == layout_state::dirty;
            }

            /**
             * @brief Check if element needs arrange pass
             * @return true if arrange cache is invalid and needs recalculation
             *
             * @details
             * Returns true when:
             * - Element was just created (never arranged)
             * - invalidate_arrange() was called
             * - measure() was called (measure invalidates arrange)
             *
             * Useful for detecting if layout was invalidated during an arrange pass.
             */
            [[nodiscard]] bool needs_arrange() const noexcept {
                return arrange_state == layout_state::dirty;
            }

            /**
             * @brief Measure phase: calculate desired size
             * @param available_width Maximum width available (logical units)
             * @param available_height Maximum height available (logical units)
             * @return The measured size for this element (in logical units)
             *
             * @exception Any exception thrown by do_measure() override
             * @exception Any exception thrown by layout_strategy::measure_children()
             * @note Exception safety: Basic guarantee - cache may be partially updated if exception thrown
             * @note Results are cached - repeated calls with same parameters return cached value
             */
            [[nodiscard]] logical_size measure(logical_unit available_width, logical_unit available_height);

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
             * @note Equivalent to measure(logical_unit(max), logical_unit(max))
             */
            [[nodiscard]] logical_size measure_unconstrained() {
                constexpr double max_lu = 1e9;  // Large value for unconstrained
                return measure(logical_unit(max_lu), logical_unit(max_lu));
            }

            /**
             * @brief Arrange phase: assign final bounds
             * @param final_bounds The allocated rectangle for this element (in logical units)
             *
             * @exception Any exception thrown by do_arrange() override
             * @exception Any exception thrown by layout_strategy::arrange_children()
             * @note Exception safety: Basic guarantee - bounds updated before arrange, children may be partially arranged
             * @note Skipped if bounds unchanged and layout state is valid
             */
            void arrange(logical_rect final_bounds);

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

            // -----------------------------------------------------------------------
            // Hit Testing
            // -----------------------------------------------------------------------
            //
            // Hit testing finds the deepest visible element at a given coordinate.
            // All methods search in reverse z-order (highest z-index first).
            //
            // **Preferred API** (logical coordinates):
            //   hit_test_logical(logical_unit x, logical_unit y)
            //   hit_test_logical(logical_unit x, logical_unit y, hit_test_path&)
            //
            // **Legacy API** (physical coordinates, for backward compatibility):
            //   hit_test(int x, int y)
            //   hit_test(int x, int y, hit_test_path&)
            //   hit_test(absolute_point)
            //   hit_test(absolute_point, hit_test_path&)
            //
            // For scaled backends (SDL), mouse coordinates should be converted to
            // logical units once at the entry point (ui_handle.hh) using:
            //   metrics.physical_to_logical_x/y()
            //
            // @see hit_test_path For path structure and three-phase event routing
            // -----------------------------------------------------------------------

            /**
             * @brief Hit testing with logical coordinates (preferred)
             * @param x Logical X coordinate (absolute)
             * @param y Logical Y coordinate (absolute)
             * @return Pointer to the deepest visible element at (x, y), or nullptr
             *
             * @details
             * This is the preferred hit testing method for scaled backends (SDL).
             * Mouse coordinates should be converted to logical units once at the
             * entry point (ui_handle.hh), then passed here for consistent precision.
             */
            [[nodiscard]] ui_element* hit_test_logical(logical_unit x, logical_unit y);

            /**
             * @brief Hit testing with logical coordinates and path recording (preferred)
             * @param x Logical X coordinate (absolute)
             * @param y Logical Y coordinate (absolute)
             * @param path Path to record elements from root to target
             * @return Pointer to the deepest visible element at (x, y), or nullptr
             */
            [[nodiscard]] ui_element* hit_test_logical(logical_unit x, logical_unit y, hit_test_path<Backend>& path);

            /**
             * @brief Hit testing with physical coordinates (legacy)
             * @param x Physical X coordinate in screen pixels
             * @param y Physical Y coordinate in screen pixels
             * @return Pointer to the deepest visible element at (x, y), or nullptr
             *
             * @note For scaled backends, prefer hit_test_logical() with proper coordinate conversion.
             */
            [[nodiscard]] ui_element* hit_test(int x, int y) {
                // Treat physical int as logical for backward compatibility (works for 1:1 backends)
                return hit_test_logical(logical_unit(static_cast<double>(x)),
                                       logical_unit(static_cast<double>(y)));
            }

            /**
             * @brief Hit testing with physical coordinates and path recording (legacy)
             * @param x Physical X coordinate in screen pixels
             * @param y Physical Y coordinate in screen pixels
             * @param path Path to record elements from root to target
             * @return Pointer to the deepest visible element at (x, y), or nullptr
             *
             * @note For scaled backends, prefer hit_test_logical() with proper coordinate conversion.
             */
            [[nodiscard]] ui_element* hit_test(int x, int y, hit_test_path<Backend>& path) {
                return hit_test_logical(logical_unit(static_cast<double>(x)),
                                       logical_unit(static_cast<double>(y)), path);
            }

            // Keep absolute_point versions for backward compatibility
            [[nodiscard]] ui_element* hit_test(geometry::absolute_point<Backend> point) {
                int x = point.x();
                int y = point.y();
                return hit_test(x, y);
            }

            [[nodiscard]] ui_element* hit_test(geometry::absolute_point<Backend> point, hit_test_path<Backend>& path) {
                int x = point.x();
                int y = point.y();
                return hit_test(x, y, path);
            }

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

                    // CRITICAL FIX: Invalidate both measure AND arrange
                    // Visibility change affects measured size (hidden widgets should measure to 0)
                    invalidate_measure();  // Triggers full layout recalculation
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
                        // Re-sort children to reflect new z-order
                        m_parent->update_child_order();
                    }
                }
            }

            /**
       * @brief Render this element and its children with proper clipping
       * @param renderer The backend renderer instance
       * @param theme Global theme pointer (REQUIRED - must not be nullptr)
       * @param metrics Backend metrics for coordinate conversion (REQUIRED)
       * @throws std::runtime_error if theme is nullptr
       */
            void render(renderer_type& renderer, const theme_type* theme,
                       const backend_metrics<Backend>& metrics) {
                if (!theme) {
                    throw std::runtime_error("render() called with null theme - theme is required for rendering");
                }
                // Create default parent style from theme
                resolved_style<Backend> default_style = resolved_style<Backend>::from_theme(*theme);
                render(renderer, {}, theme, default_style, metrics, point_type{0, 0});  // Empty vector means "render everything"
            }

            /**
       * @brief Render this element and its children with dirty rectangle optimization
       * @param renderer The backend renderer instance
       * @param dirty_regions Regions that need redrawing (empty = render everything)
       * @param theme Global theme pointer (may be nullptr - fetched once by ui_handle and passed down tree)
       * @param parent_style Parent's resolved style (accumulated top-down, no recursion!)
       * @param metrics Backend metrics for logical-to-physical coordinate conversion
       * @param offset Accumulated physical offset from parent content areas
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
       *
       * Coordinate conversion:
       * - m_bounds is in logical units (set by arrange())
       * - metrics converts logical → physical for actual rendering
       * - offset is accumulated physical position from parent content areas
       */
            void render(renderer_type& renderer, const std::vector<rect_type>& dirty_regions,
                       const theme_type* theme, const resolved_style<Backend>& parent_style,
                       const backend_metrics<Backend>& metrics,
                       point_type const& offset = point_type{0, 0}) {
                if (!m_visible) {
                    return;
                }

                // Resolve style by merging parent_style with my overrides (no recursion!)
                auto style = this->resolve_style(theme, parent_style);

                // Convert logical bounds to physical using metrics
                // m_bounds is in logical units, snap_rect converts to physical
                rect_type physical_bounds = metrics.snap_rect(m_bounds);

                // Bounds are RELATIVE to parent's content area
                // Add accumulated physical offset to get absolute screen position
                point_type absolute_pos{
                    rect_utils::get_x(physical_bounds) + point_utils::get_x(offset),
                    rect_utils::get_y(physical_bounds) + point_utils::get_y(offset)
                };

                size_type size{
                    rect_utils::get_width(physical_bounds),
                    rect_utils::get_height(physical_bounds)
                };

                // Create draw context with resolved style, ABSOLUTE position, size, dirty regions, and theme
                // The absolute_pos accounts for all parent content area offsets
                draw_context<Backend> ctx(renderer, style, absolute_pos, size, dirty_regions, theme);

                // Skip rendering if this element doesn't intersect with any dirty region
                rect_type absolute_bounds;
                rect_utils::set_bounds(absolute_bounds,
                                     point_utils::get_x(absolute_pos),
                                     point_utils::get_y(absolute_pos),
                                     size_utils::get_width(size),
                                     size_utils::get_height(size));

                if (!ctx.should_render(absolute_bounds)) {
                    // Early return - don't render this element or its children
                    return;
                }

                // Render this element (style already resolved in ctx)
                do_render(ctx);

                // Set clip rect for children (content area only)
                logical_rect const content_area_logical = get_content_area();

                // Convert logical content area to physical
                rect_type content_area_physical = metrics.snap_rect(content_area_logical);

                // Calculate absolute clip rect
                // content_area position is relative to widget's bounds origin, so add absolute_pos
                rect_type absolute_clip;
                rect_utils::set_bounds(absolute_clip,
                    point_utils::get_x(absolute_pos) + rect_utils::get_x(content_area_physical),
                    point_utils::get_y(absolute_pos) + rect_utils::get_y(content_area_physical),
                    rect_utils::get_width(content_area_physical),
                    rect_utils::get_height(content_area_physical));

                // Push clip rect before rendering children
                renderer.push_clip(absolute_clip);

                // Render children in z-order (they're already clipped)
                // Children are arranged at RELATIVE coordinates (0,0 = top-left of content area)
                // Accumulate physical offset for children: widget's absolute position + content area physical offset
                point_type child_offset{
                    point_utils::get_x(absolute_pos) + rect_utils::get_x(content_area_physical),
                    point_utils::get_y(absolute_pos) + rect_utils::get_y(content_area_physical)
                };

                for (const auto& child : m_children) {
                    child->render(renderer, dirty_regions, theme, style, metrics, child_offset);
                }

                // Restore previous clip rect
                renderer.pop_clip();
            }

            // -----------------------------------------------------------------------
            // Public Accessors
            // -----------------------------------------------------------------------

            [[nodiscard]] const logical_rect& bounds() const noexcept {
                return m_bounds;
            }

            /**
             * @brief Get absolute bounds as snapped logical integers
             * @return Rectangle with absolute position and size as integers
             *
             * @details
             * Returns the widget's position relative to the UI root, with logical units
             * snapped to integers using floor/ceil for consistent boundaries.
             *
             * **Coordinate semantics:**
             * - "Absolute" means relative to UI root (not parent-relative)
             * - Values are logical units snapped to int (NOT DPI-scaled physical pixels)
             * - For terminal backends: 1 logical unit = 1 character cell
             * - For GUI backends: logical units snapped to grid, NOT physical pixels
             *
             * **Prefer more specific methods:**
             * - get_absolute_logical_bounds() - Full double precision logical coordinates
             * - get_screen_bounds(metrics) - True DPI-aware physical screen pixels
             *
             * **Use cases for this method:**
             * - Legacy code expecting int-based bounds
             * - Compatibility with older hit-testing code
             *
             * @example
             * @code
             * auto abs_bounds = widget->get_absolute_bounds();
             * int widget_abs_x = abs_bounds.x();  // Snapped logical coordinate
             * int widget_abs_y = abs_bounds.y();
             *
             * // Convert absolute click to widget-relative (loses fractional precision)
             * int rel_x = click_x - widget_abs_x;
             * int rel_y = click_y - widget_abs_y;
             * @endcode
             *
             * @note O(depth) - walks parent chain. Cache if calling repeatedly.
             * @see get_absolute_logical_bounds() for full-precision logical coordinates
             * @see get_screen_bounds() for DPI-aware physical pixel coordinates
             */
            [[nodiscard]] geometry::absolute_rect<Backend> get_absolute_bounds() const noexcept {
                // Walk parent chain to accumulate absolute position in logical units
                logical_unit abs_x = m_bounds.x;
                logical_unit abs_y = m_bounds.y;

                const ui_element* current_parent = m_parent;
                while (current_parent) {
                    abs_x = abs_x + current_parent->m_bounds.x;
                    abs_y = abs_y + current_parent->m_bounds.y;
                    logical_rect parent_content = current_parent->get_content_area();
                    abs_x = abs_x + parent_content.x;
                    abs_y = abs_y + parent_content.y;
                    current_parent = current_parent->m_parent;
                }

                // Snap logical coordinates to integer grid (for legacy int-based hit testing)
                // NOTE: This does NOT apply DPI scaling - use get_screen_bounds() for physical pixels
                // Position: floor (consistent with rendering), far edges: ceil for full coverage
                int const left   = static_cast<int>(std::floor(abs_x.value));
                int const top    = static_cast<int>(std::floor(abs_y.value));
                int const right  = static_cast<int>(std::ceil((abs_x + m_bounds.width).value));
                int const bottom = static_cast<int>(std::ceil((abs_y + m_bounds.height).value));

                rect_type bounds_snapped;
                rect_utils::set_bounds(bounds_snapped,
                                      left, top,
                                      right - left, bottom - top);
                return geometry::absolute_rect<Backend>{bounds_snapped};
            }

            /**
             * @brief Get absolute bounds in logical units
             * @return Absolute position and size in logical coordinate space
             *
             * @details
             * Returns the widget's absolute position in **logical units** (backend-agnostic).
             * This is the primary method for internal coordinate calculations like:
             * - Hit testing (comparing mouse position to widget bounds)
             * - Popup/menu positioning
             * - Widget-to-widget coordinate transforms
             *
             * For rendering or dirty region tracking, use get_screen_bounds() which
             * converts to physical pixels via backend_metrics.
             *
             * @note O(depth) - walks parent chain. Cache if calling repeatedly.
             * @see get_screen_bounds() for physical pixel coordinates
             */
            [[nodiscard]] logical_rect get_absolute_logical_bounds() const noexcept {
                logical_unit abs_x = m_bounds.x;
                logical_unit abs_y = m_bounds.y;

                const ui_element* current_parent = m_parent;
                while (current_parent) {
                    abs_x = abs_x + current_parent->m_bounds.x;
                    abs_y = abs_y + current_parent->m_bounds.y;
                    logical_rect parent_content = current_parent->get_content_area();
                    abs_x = abs_x + parent_content.x;
                    abs_y = abs_y + parent_content.y;
                    current_parent = current_parent->m_parent;
                }

                return logical_rect{abs_x, abs_y, m_bounds.width, m_bounds.height};
            }

            /**
             * @brief Get absolute bounds in physical screen pixels
             * @param metrics Backend metrics for logical→physical conversion
             * @return Absolute position and size in physical pixel coordinates
             *
             * @details
             * Returns the widget's screen position in **physical pixels**.
             * Uses metrics.snap_rect() for consistent floor/ceil snapping.
             * Use this for:
             * - Dirty region tracking
             * - External system integration (accessibility, screen capture)
             *
             * For internal coordinate calculations (hit testing, positioning),
             * prefer get_absolute_logical_bounds() which preserves precision.
             *
             * @note O(depth) - walks parent chain. Cache if calling repeatedly.
             * @see get_absolute_logical_bounds() for logical coordinates
             */
            [[nodiscard]] rect_type get_screen_bounds(const backend_metrics<Backend>& metrics) const noexcept {
                return metrics.snap_rect(get_absolute_logical_bounds());
            }

            /**
             * @brief Calculate the content area (bounds minus margin and padding)
             * @return Rectangle representing the content area (relative coordinates)
             * @note noexcept - uses only safe arithmetic operations
             * @note virtual - subclasses can override to account for borders, title bars, etc.
             *
             * @details
             * The content area is where child elements are positioned. It's calculated by
             * subtracting margin and padding from the element's bounds.
             *
             * Position is relative to this element's bounds origin (0,0). For absolute
             * positioning, use get_absolute_bounds() to convert to screen coordinates.
             */
            [[nodiscard]] virtual logical_rect get_content_area() const noexcept {
                // Content area position is relative to this widget's bounds origin (0,0).
                // Absolute positioning is computed during rendering by accumulating parent offsets.
                logical_unit x = m_margin.left + m_padding.left;
                logical_unit y = m_margin.top + m_padding.top;

                // Calculate dimensions
                logical_unit total_h_spacing = m_margin.horizontal() + m_padding.horizontal();
                logical_unit total_v_spacing = m_margin.vertical() + m_padding.vertical();

                logical_unit w = m_bounds.width - total_h_spacing;
                logical_unit h = m_bounds.height - total_v_spacing;

                // Clamp to non-negative
                w = max(w, logical_unit(0.0));
                h = max(h, logical_unit(0.0));

                return logical_rect{x, y, w, h};
            }

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
            [[nodiscard]] const logical_size& last_measured_size() const noexcept { return m_last_measured_size; }
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
             * @brief Check if a point is inside this element (absolute screen coordinates)
             * @param x Absolute screen X coordinate
             * @param y Absolute screen Y coordinate
             * @return true if point is within this element's absolute bounds
             *
             * @details
             * **CRITICAL**: After the relative coordinate refactoring (2025-11), child elements
             * store bounds relative to their parent's content area. This method converts relative
             * bounds to absolute screen coordinates by recursively walking up the parent chain.
             *
             * **Algorithm:**
             * 1. **Root element**: Bounds are already absolute → direct contains() check
             * 2. **Child element**:
             *    - Recursively compute parent's absolute position (walk to root)
             *    - Add parent's content area offset (accounts for margin/padding/border)
             *    - Add this element's relative position
             *    - Result: absolute screen coordinates for hit testing
             *
             * **Performance**: O(tree depth) per call - typically 5-15 levels, max ~30 in complex UIs.
             * The recursive lambda using std::function is necessary to handle relative→absolute
             * conversion at each level without polluting the public API with helper methods.
             *
             * **Thread Safety**: Not thread-safe (accesses parent chain without synchronization).
             *
             * **Why This Matters**: Mouse events arrive in absolute screen coordinates, but widget
             * bounds are stored relative to parent content. This conversion is essential for
             * correct event routing in deeply nested UIs (e.g., modal dialogs with buttons).
             *
             * @note Requires #include <functional> for std::function
             * @see geometry::to_absolute() for the public coordinate conversion API
             * @see hit_test() for the tree traversal algorithm that uses is_inside()
             */
            /**
             * @brief Check if a logical point is inside this element
             * @param x Logical X coordinate (absolute)
             * @param y Logical Y coordinate (absolute)
             * @return true if point is inside bounds
             *
             * @details
             * This is the preferred method for hit testing as it works entirely
             * in logical space, preserving fractional precision. Mouse coordinates
             * should be converted to logical once at the entry point (ui_handle.hh).
             */
            [[nodiscard]] bool is_inside_logical(logical_unit x, logical_unit y) const noexcept {
                logical_rect abs_bounds = get_absolute_logical_bounds();
                return x >= abs_bounds.x &&
                       x < (abs_bounds.x + abs_bounds.width) &&
                       y >= abs_bounds.y &&
                       y < (abs_bounds.y + abs_bounds.height);
            }

            /**
             * @brief Check if a point is inside this element (event_target interface)
             * @param x X coordinate in logical units
             * @param y Y coordinate in logical units
             * @return true if point is inside bounds
             */
            [[nodiscard]] bool is_inside(logical_unit x, logical_unit y) const override {
                return is_inside_logical(x, y);
            }

            /**
             * @brief RTTI-free downcast - returns this since ui_element IS a ui_element
             */
            [[nodiscard]] ui_element<Backend>* as_ui_element() noexcept override {
                return this;
            }

            /**
             * @brief RTTI-free const downcast
             */
            [[nodiscard]] const ui_element<Backend>* as_ui_element() const noexcept override {
                return this;
            }

            // -----------------------------------------------------------------------
            // Protected Virtual Methods for Customization
            // -----------------------------------------------------------------------

            /**
             * @brief Override to provide custom measurement logic
             */
            virtual logical_size do_measure(logical_unit available_width, logical_unit available_height);

            /**
             * @brief Override to provide custom arrangement logic
             */
            virtual void do_arrange(const logical_rect& final_bounds);

            /**
             * @brief Get the intrinsic content size
             */
            [[nodiscard]] virtual logical_size get_content_size() const {
                return logical_size{logical_unit(0.0), logical_unit(0.0)};
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
            logical_size m_last_measured_size = {};
            logical_unit m_last_available_width = logical_unit(-1.0);
            logical_unit m_last_available_height = logical_unit(-1.0);

            // Properties
            bool m_visible = true;
            logical_rect m_bounds = {};
            int m_z_index = 0;
            size_constraint m_width_constraint;
            size_constraint m_height_constraint;
            horizontal_alignment m_h_align = horizontal_alignment::stretch;
            vertical_alignment m_v_align = vertical_alignment::stretch;
            thickness m_margin = logical_thickness{};
            thickness m_padding = logical_thickness{};

            // Dirty region tracking (only used by root element)
            std::vector<rect_type> m_dirty_regions;
    };

    // ===============================================================================
    // Implementation
    // ===============================================================================

    template<UIBackend Backend>
    ui_element <Backend>::ui_element(ui_element* parent)
        : event_target <Backend>()
        , m_parent(parent)
    {
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

        // Propagate up to parent so re-arrangement starts from root
        if (m_parent) {
            m_parent->invalidate_arrange();
        }

        // Propagate down to children
        for (auto& child : m_children) {
            child->invalidate_arrange();
        }
    }

    // -------------------------------------------------------------------------------
    // Two-Pass Layout
    // -------------------------------------------------------------------------------

    template<UIBackend Backend>
    logical_size ui_element <Backend>::measure(logical_unit available_width, logical_unit available_height) {
        // Check cache
        if (measure_state == layout_state::valid &&
            m_last_available_width == available_width &&
            m_last_available_height == available_height) {
            return m_last_measured_size;
        }

        // Account for margin - subtract from available space
        logical_unit content_width = available_width - m_margin.horizontal();
        logical_unit content_height = available_height - m_margin.vertical();

        // Clamp to zero (can't have negative dimensions)
        content_width = max(content_width, logical_unit(0.0));
        content_height = max(content_height, logical_unit(0.0));

        // Measure content
        logical_size measured = do_measure(content_width, content_height);

        // Add margin back
        logical_unit meas_w = measured.width + m_margin.horizontal();
        logical_unit meas_h = measured.height + m_margin.vertical();

        // Apply constraints
        logical_unit clamped_w = m_width_constraint.clamp(meas_w);
        logical_unit clamped_h = m_height_constraint.clamp(meas_h);

        // Store clamped sizes
        measured.width = clamped_w;
        measured.height = clamped_h;

        // Cache results
        m_last_measured_size = measured;
        m_last_available_width = available_width;
        m_last_available_height = available_height;

        measure_state = layout_state::valid;
        return m_last_measured_size;
    }

    template<UIBackend Backend>
    void ui_element <Backend>::arrange(logical_rect final_bounds) {
        // Check if bounds changed
        const bool bounds_changed = (m_bounds != final_bounds);

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

        // Calculate content area
        // RELATIVE COORDINATES: content_area position is relative to widget's own bounds origin

        // Position is relative to bounds origin (0, 0), not absolute
        logical_unit x = m_margin.left + m_padding.left;
        logical_unit y = m_margin.top + m_padding.top;

        // Calculate dimensions
        logical_unit total_h_spacing = m_margin.horizontal() + m_padding.horizontal();
        logical_unit total_v_spacing = m_margin.vertical() + m_padding.vertical();

        logical_unit w = final_bounds.width - total_h_spacing;
        logical_unit h = final_bounds.height - total_v_spacing;

        // Clamp to non-negative
        w = max(w, logical_unit(0.0));
        h = max(h, logical_unit(0.0));

        logical_rect content_area{x, y, w, h};

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
    // Hit Testing (Logical Coordinates)
    // -------------------------------------------------------------------------------

    // NOLINTNEXTLINE(misc-no-recursion)
    // Note: Recursive tree traversal for hit testing, bounded by UI tree height
    template<UIBackend Backend>
    ui_element <Backend>* ui_element <Backend>::hit_test_logical(logical_unit x, logical_unit y) {
        if (!m_visible) {
            return nullptr;
        }

        // Check if point is inside this element using logical coordinates
        if (!is_inside_logical(x, y)) {
            return nullptr;
        }

        // Check children in reverse order (highest z-index first)
        for (auto it = m_children.rbegin(); it != m_children.rend(); ++it) {
            if (ui_element* hit = (*it)->hit_test_logical(x, y)) {
                return hit;
            }
        }

        return this;
    }

    // NOLINTNEXTLINE(misc-no-recursion)
    // Note: Recursive tree traversal for hit testing with path recording
    template<UIBackend Backend>
    ui_element <Backend>* ui_element <Backend>::hit_test_logical(logical_unit x, logical_unit y, hit_test_path<Backend>& path) {
        if (!m_visible) {
            return nullptr;
        }

        // Check if point is inside this element using logical coordinates
        if (!is_inside_logical(x, y)) {
            return nullptr;
        }

        // Record this element in the path
        path.push(this);

        // Check children in reverse order (highest z-index first)
        for (auto it = m_children.rbegin(); it != m_children.rend(); ++it) {
            if (ui_element* hit = (*it)->hit_test_logical(x, y, path)) {
                return hit;
            }
        }

        // This element is the target
        return this;
    }

    // -------------------------------------------------------------------------------
    // Virtual Methods
    // -------------------------------------------------------------------------------

    template<UIBackend Backend>
    logical_size ui_element <Backend>::do_measure(logical_unit available_width, logical_unit available_height) {
        if (m_layout_strategy) {
            // Subtract padding from available space before passing to layout strategy
            // (margin is already subtracted by measure())
            logical_unit content_width = available_width - m_padding.horizontal();
            logical_unit content_height = available_height - m_padding.vertical();

            // Clamp to zero
            content_width = max(content_width, logical_unit(0.0));
            content_height = max(content_height, logical_unit(0.0));

            logical_size const content_size = m_layout_strategy->measure_children(
                static_cast <const ui_element*>(this),
                content_width,
                content_height);

            // Add padding back to the measured size
            logical_unit w = content_size.width + m_padding.horizontal();
            logical_unit h = content_size.height + m_padding.vertical();

            return logical_size{w, h};
        }
        return get_content_size();
    }

    template<UIBackend Backend>
    void ui_element <Backend>::do_arrange(const logical_rect& final_bounds) {
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
        // Convert to physical screen coordinates using metrics
        // Uses floor for position, ceil for far edges to ensure full coverage
        const auto* metrics = ui_services<Backend>::metrics();
        rect_type absolute_bounds;
        if (metrics) {
            absolute_bounds = get_screen_bounds(*metrics);
        } else {
            // Fallback for no metrics (terminal backend with 1:1)
            logical_rect abs_logical = get_absolute_logical_bounds();
            rect_utils::set_bounds(absolute_bounds,
                                  abs_logical.x.to_int(), abs_logical.y.to_int(),
                                  abs_logical.width.to_int(), abs_logical.height.to_int());
        }

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
