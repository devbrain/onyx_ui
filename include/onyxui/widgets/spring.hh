/**
 * @file spring.hh
 * @brief Flexible expanding spacing widget for layout control
 * @author igor
 * @date 16/10/2025
 *
 * @details
 * Provides the spring widget for controlling layout spacing with flexible expansion,
 * similar to QSpacerItem with expanding policy in Qt or CSS flexbox's flex-grow.
 * Springs are invisible widgets that expand to fill available space and support
 * weighted proportional distribution.
 *
 * ## Spring Widget
 * Flexible space that expands to fill available space in layouts.
 *
 * **Key Features:**
 * - Flexible expansion (horizontal or vertical)
 * - Weighted distribution support (like CSS flex-grow)
 * - Min/max size constraints
 * - Invisible (no rendering)
 * - Not focusable (skipped in tab navigation)
 * - Always uses `size_policy::weighted` for proportional distribution
 * - Proper Rule of Five implementation (inherits from widget)
 *
 * **Useful For:**
 * - Pushing buttons to edges of toolbars
 * - Centering content by placing springs on both sides
 * - Distributing space proportionally with weights
 * - Flexible toolbars that adapt to available space
 * - Creating responsive layouts
 *
 * ## Usage Examples
 *
 * ### Push to Edges
 * ```cpp
 * hbox<Backend> toolbar;
 * toolbar.add_child(std::make_unique<button<Backend>>("File"));
 * toolbar.add_child(std::make_unique<spring<Backend>>()); // Expands to fill
 * toolbar.add_child(std::make_unique<button<Backend>>("Help"));
 * // Result: "File" on left, "Help" on right
 * ```
 *
 * ### Center Content
 * ```cpp
 * hbox<Backend> centered;
 * centered.add_child(std::make_unique<spring<Backend>>());
 * centered.add_child(std::make_unique<label<Backend>>("Centered"));
 * centered.add_child(std::make_unique<spring<Backend>>());
 * ```
 *
 * ### Weighted Distribution (3:1 ratio)
 * ```cpp
 * vbox<Backend> layout;
 * layout.add_child(std::make_unique<panel<Backend>>());         // Header
 * layout.add_child(std::make_unique<spring<Backend>>(3.0f, false)); // 3x vertical
 * layout.add_child(std::make_unique<panel<Backend>>());         // Content
 * layout.add_child(std::make_unique<spring<Backend>>(1.0f, false)); // 1x vertical
 * layout.add_child(std::make_unique<panel<Backend>>());         // Footer
 * ```
 *
 * ## Exception Safety
 * - Constructor: Basic guarantee (may throw from widget constructor)
 * - Setters: Strong guarantee (no changes if constraint update fails)
 * - Getters: No-throw guarantee (all marked noexcept)
 * - Rendering: No-throw guarantee (empty implementation)
 *
 * ## Thread Safety
 * Not thread-safe. All operations must be performed on the UI thread.
 *
 * @see spacer For fixed-size spacing
 * @see size_constraint For size policy details
 */

#pragma once

#include <onyxui/widgets/widget.hh>
#include <limits>

namespace onyxui {
    /**
     * @class spring
     * @brief Flexible expanding widget for layout spacing
     *
     * @details
     * Spring creates flexible space that expands to fill available space in
     * layouts. Unlike spacer (fixed size), spring grows and shrinks based on
     * available space. This is similar to Qt's QSpacerItem with expanding policy
     * or CSS flexbox's flex-grow.
     *
     * ## Behavior
     * - Invisible (no rendering)
     * - Flexible size (expands to fill available space)
     * - Not focusable (skipped in tab navigation)
     * - Can have min/max constraints
     * - Supports weighted distribution
     * - Orientation-aware (horizontal or vertical expansion)
     *
     * ## Sizing Policies
     * - **Always weighted**: Spring always uses weighted size policy
     * - **Weight parameter**: Determines proportional share (like CSS flex-grow)
     * - **Default weight (1.0)**: Equal distribution with other weight-1.0 springs
     *
     * ## Common Use Cases
     * - Pushing buttons to edges: Add spring between left and right groups
     * - Centering content: Springs on both sides with equal weight
     * - Proportional spacing: Different weights for different gaps
     * - Flexible toolbars: Springs absorb extra space
     * - Responsive layouts: Adapt to varying container sizes
     *
     * ## Rule of Five
     * This class follows the Rule of Five through inheritance from widget<Backend>:
     * - **Destructor**: Virtual destructor (inherited)
     * - **Copy constructor**: Deleted (inherited - not copyable)
     * - **Copy assignment**: Deleted (inherited - not copyable)
     * - **Move constructor**: Defaulted (inherited - movable)
     * - **Move assignment**: Defaulted (inherited - movable)
     *
     * ## Exception Safety
     * - Constructor provides basic exception guarantee
     * - Setters provide strong exception guarantee (atomic state change)
     * - Getters are noexcept (no-throw guarantee)
     * - Move operations are noexcept (inherited from widget)
     *
     * ## Const Correctness
     * - All getters are const-qualified and noexcept
     * - Setters modify internal state and are non-const
     * - No mutable data members
     * - Helper method update_constraint() is private and non-const
     *
     * @tparam Backend The backend traits type satisfying UIBackend concept
     *
     * @see spacer For fixed-size spacing
     * @see size_constraint For sizing policy details
     */
    template<UIBackend Backend>
    class spring : public widget<Backend> {
        public:
            /**
             * @brief Construct a spring with optional weight
             *
             * @param weight Weight for proportional distribution (default: 1.0)
             * @param horizontal True for horizontal expansion, false for vertical (default: true)
             *
             * @throws std::bad_alloc If memory allocation fails during construction
             * @throws Any exception thrown by widget<Backend> constructor
             *
             * @details
             * Creates a spring that expands to fill space. The weight determines
             * how much space it takes relative to other weighted widgets:
             * - weight = 1.0: Equal share (uses expand policy)
             * - weight ≠ 1.0: Proportional share (uses weighted policy)
             * - weight = 2.0: Takes 2x space vs weight=1.0 springs
             * - weight = 0.5: Takes half the space of weight=1.0 springs
             *
             * The horizontal parameter determines which dimension expands:
             * - true: Expands horizontally (for hbox layouts)
             * - false: Expands vertically (for vbox layouts)
             *
             * **Size Policy Behavior:**
             * - Always uses size_policy::weighted with the specified weight
             * - This ensures proper proportional distribution with other springs
             * - Min/max initialized to 0 and INT_MAX
             *
             * **Exception Safety:** Basic guarantee - object may be partially
             * constructed if exception occurs during constraint setup.
             *
             * @example Equal expansion (default)
             * @code
             * auto spring1 = std::make_unique<spring<Backend>>();
             * auto spring2 = std::make_unique<spring<Backend>>();
             * // Both get equal space
             * @endcode
             *
             * @example Weighted expansion (2:1 ratio)
             * @code
             * auto spring1 = std::make_unique<spring<Backend>>(2.0f);
             * auto spring2 = std::make_unique<spring<Backend>>(1.0f);
             * // spring1 gets 2x the space of spring2
             * @endcode
             *
             * @example Vertical expansion
             * @code
             * auto vspring = std::make_unique<spring<Backend>>(1.0f, false);
             * // Expands vertically in vbox layouts
             * @endcode
             */
            explicit spring(float weight = 1.0F, bool horizontal = true)
                : widget<Backend>(nullptr)
                  , m_weight(weight)
                  , m_horizontal(horizontal)
                  , m_min_size(0)
                  , m_max_size(std::numeric_limits<int>::max()) {
                // Configure as non-focusable (springs are invisible)
                this->set_focusable(false);

                // Set weighted constraint for the primary axis
                // Always use weighted policy (even for weight 1.0) to ensure
                // proper proportional distribution with other springs
                size_constraint constraint;
                constraint.policy = size_policy::weighted;
                constraint.weight = weight;
                constraint.min_size = m_min_size;
                constraint.max_size = m_max_size;

                if (horizontal) {
                    this->set_width_constraint(constraint);
                } else {
                    this->set_height_constraint(constraint);
                }
            }

            /**
             * @brief Set spring weight
             *
             * @param weight New weight value (must be > 0.0)
             *
             * @throws Any exception thrown by set_width_constraint() or set_height_constraint()
             *
             * @details
             * Changes the weight for proportional space distribution.
             * Higher weight means more space allocated to this spring.
             *
             * **Behavior:**
             * - If weight <= 0.0: No change (validation check)
             * - If weight == current: No change (early exit)
             * - Always uses weighted policy after calling this method
             *
             * **Exception Safety:** Strong guarantee - state unchanged if exception thrown.
             *
             * **Thread Safety:** Not thread-safe. Must be called from UI thread.
             */
            void set_weight(float weight) {
                if (m_weight != weight && weight > 0.0F) {
                    m_weight = weight;

                    // Always use weighted policy for proper proportional distribution
                    size_constraint constraint;
                    constraint.policy = size_policy::weighted;
                    constraint.weight = weight;
                    constraint.min_size = m_min_size;
                    constraint.max_size = m_max_size;

                    if (m_horizontal) {
                        this->set_width_constraint(constraint);
                    } else {
                        this->set_height_constraint(constraint);
                    }
                }
            }

            /**
             * @brief Set minimum size constraint
             *
             * @param min_size Minimum size in pixels
             *
             * @throws Any exception thrown by update_constraint()
             *
             * @details
             * The spring will never be smaller than this size, even if
             * there's limited space available.
             *
             * **Exception Safety:** Strong guarantee - state unchanged if exception thrown.
             *
             * **Thread Safety:** Not thread-safe. Must be called from UI thread.
             */
            void set_min_size(int min_size) {
                if (m_min_size != min_size) {
                    m_min_size = min_size;
                    update_constraint();
                }
            }

            /**
             * @brief Set maximum size constraint
             *
             * @param max_size Maximum size in pixels
             *
             * @throws Any exception thrown by update_constraint()
             *
             * @details
             * The spring will never be larger than this size, even if
             * there's abundant space available.
             *
             * **Exception Safety:** Strong guarantee - state unchanged if exception thrown.
             *
             * **Thread Safety:** Not thread-safe. Must be called from UI thread.
             */
            void set_max_size(int max_size) {
                if (m_max_size != max_size) {
                    m_max_size = max_size;
                    update_constraint();
                }
            }

            /**
             * @brief Get current weight
             *
             * @return Weight value for proportional distribution
             *
             * @note noexcept - guaranteed not to throw exceptions
             * @note Thread-safe for reading (const operation)
             */
            [[nodiscard]] float weight() const noexcept { return m_weight; }

            /**
             * @brief Get minimum size
             *
             * @return Minimum size constraint in pixels
             *
             * @note noexcept - guaranteed not to throw exceptions
             * @note Thread-safe for reading (const operation)
             */
            [[nodiscard]] int min_size() const noexcept { return m_min_size; }

            /**
             * @brief Get maximum size
             *
             * @return Maximum size constraint in pixels
             *
             * @note noexcept - guaranteed not to throw exceptions
             * @note Thread-safe for reading (const operation)
             */
            [[nodiscard]] int max_size() const noexcept { return m_max_size; }

            /**
             * @brief Check if spring expands horizontally
             *
             * @return true if horizontal expansion, false if vertical
             *
             * @note noexcept - guaranteed not to throw exceptions
             * @note Thread-safe for reading (const operation)
             */
            [[nodiscard]] bool is_horizontal() const noexcept { return m_horizontal; }

        protected:
            /**
             * @brief Get content size (returns minimal size)
             *
             * @return Size object with min_size for both dimensions
             *
             * @throws Any exception thrown by size_utils::set_size()
             *
             * @details
             * Returns the minimum size of the spring. The actual size will be
             * determined by the layout based on available space and the spring's
             * expansion policy (expand or weighted).
             *
             * @note Override of widget::get_content_size()
             */
            typename Backend::size_type get_content_size() const override {
                typename Backend::size_type size;
                size_utils::set_size(size, m_min_size, m_min_size);
                return size;
            }

            /**
             * @brief Render (no-op for spring)
             *
             * @param ctx The render context (unused)
             *
             * @details
             * Springs are invisible and don't render anything.
             * This method is intentionally empty.
             *
             * **Non-visual widget:** Springs are layout-only elements with no visual representation.
             * Size comes from configured minimum size (get_content_size), actual size determined by layout.
             *
             * **Exception Safety:** No-throw guarantee (empty implementation)
             *
             * @note Override of widget::do_render()
             */
            void do_render([[maybe_unused]] render_context<Backend>& ctx) const override {
                // Springs are invisible - nothing to render
            }

        private:
            /**
             * @brief Update size constraint after parameter change
             *
             * @throws Any exception thrown by set_width_constraint() or set_height_constraint()
             *
             * @details
             * Internal helper that applies current weight, min_size, and max_size
             * to the appropriate dimension constraint.
             *
             * **Policy Selection:**
             * - Always uses size_policy::weighted with the current weight value
             * - This ensures proper proportional distribution with other springs
             *
             * **Exception Safety:** Basic guarantee - constraint may be partially updated.
             */
            void update_constraint() {
                // Always use weighted policy for proper proportional distribution
                size_constraint constraint;
                constraint.policy = size_policy::weighted;
                constraint.weight = m_weight;
                constraint.min_size = m_min_size;
                constraint.max_size = m_max_size;

                if (m_horizontal) {
                    this->set_width_constraint(constraint);
                } else {
                    this->set_height_constraint(constraint);
                }
            }

            float m_weight;      ///< Weight for proportional distribution
            bool m_horizontal;   ///< True for horizontal, false for vertical
            int m_min_size;      ///< Minimum size in pixels
            int m_max_size;      ///< Maximum size in pixels
    };

    // ============================================================================
    // Convenient Factory Functions
    // ============================================================================

    /**
     * @brief Create a horizontal spring
     * @tparam Backend The backend traits type
     * @param weight Weight for proportional distribution (default: 1.0)
     * @return Unique pointer to horizontal spring
     *
     * @details
     * Creates a spring that expands horizontally. Use in hbox layouts to
     * push elements apart or center content. The weight determines how much
     * space this spring takes relative to other springs.
     *
     * @example Push buttons to edges
     * @code
     * hbox<Backend> toolbar;
     * toolbar.add_child(std::make_unique<button<Backend>>("File"));
     * toolbar.add_child(create_hspring<Backend>());  // Expands to fill space
     * toolbar.add_child(std::make_unique<button<Backend>>("Help"));
     * // Result: "File" on left, "Help" on right
     * @endcode
     *
     * @example Center content
     * @code
     * hbox<Backend> container;
     * container.add_child(create_hspring<Backend>());  // Left spring
     * container.add_child(std::make_unique<label<Backend>>("Centered"));
     * container.add_child(create_hspring<Backend>());  // Right spring
     * @endcode
     *
     * @example Weighted distribution (2:1 ratio)
     * @code
     * hbox<Backend> layout;
     * layout.add_child(create_hspring<Backend>(2.0f));  // Takes 2/3 of space
     * layout.add_child(std::make_unique<panel<Backend>>());
     * layout.add_child(create_hspring<Backend>(1.0f));  // Takes 1/3 of space
     * @endcode
     */
    template<UIBackend Backend>
    inline auto create_hspring(float weight = 1.0F) {
        return std::make_unique<spring<Backend>>(weight, true);
    }

    /**
     * @brief Create a vertical spring
     * @tparam Backend The backend traits type
     * @param weight Weight for proportional distribution (default: 1.0)
     * @return Unique pointer to vertical spring
     *
     * @details
     * Creates a spring that expands vertically. Use in vbox layouts to
     * push elements apart or center content. The weight determines how much
     * space this spring takes relative to other springs.
     *
     * @example Push content to top and bottom
     * @code
     * vbox<Backend> layout;
     * layout.add_child(std::make_unique<label<Backend>>("Header"));
     * layout.add_child(create_vspring<Backend>());  // Expands vertically
     * layout.add_child(std::make_unique<label<Backend>>("Footer"));
     * @endcode
     *
     * @example Weighted vertical distribution (3:1 ratio)
     * @code
     * vbox<Backend> layout;
     * layout.add_child(std::make_unique<panel<Backend>>());  // Header
     * layout.add_child(create_vspring<Backend>(3.0f));       // 3x space
     * layout.add_child(std::make_unique<panel<Backend>>());  // Content
     * layout.add_child(create_vspring<Backend>(1.0f));       // 1x space
     * layout.add_child(std::make_unique<panel<Backend>>());  // Footer
     * @endcode
     */
    template<UIBackend Backend>
    inline auto create_vspring(float weight = 1.0F) {
        return std::make_unique<spring<Backend>>(weight, false);
    }

} // namespace onyxui
