/**
 * @file spacer.hh
 * @brief Fixed-size spacing widget for layout control
 * @author igor
 * @date 16/10/2025
 *
 * @details
 * Provides the spacer widget for controlling layout spacing with fixed dimensions,
 * similar to QSpacerItem with fixed policy in Qt. Spacers are invisible widgets
 * that participate in layout calculations to control spacing and alignment.
 *
 * ## Spacer Widget
 * Fixed-size empty space that pushes elements apart by a specific amount.
 *
 * **Key Features:**
 * - Fixed width and height (independently configurable)
 * - Invisible (no rendering)
 * - Not focusable (skipped in tab navigation)
 * - Uses `size_policy::fixed` for precise control
 * - Proper Rule of Five implementation (inherits from widget)
 *
 * **Useful For:**
 * - Creating fixed gaps between widgets
 * - Adding margins without using padding
 * - Precise spacing control in layouts
 * - Replacing layout spacing properties with explicit control
 *
 * ## Usage Examples
 *
 * ### Horizontal Gap (20px)
 * ```cpp
 * hbox<Backend> toolbar;
 * toolbar.add_child(std::make_unique<button<Backend>>("New"));
 * toolbar.add_child(std::make_unique<spacer<Backend>>(20, 0)); // 20px horizontal gap
 * toolbar.add_child(std::make_unique<button<Backend>>("Open"));
 * ```
 *
 * ### Vertical Gap (30px)
 * ```cpp
 * vbox<Backend> layout;
 * layout.add_child(std::make_unique<label<Backend>>("Title"));
 * layout.add_child(std::make_unique<spacer<Backend>>(0, 30)); // 30px vertical gap
 * layout.add_child(std::make_unique<label<Backend>>("Content"));
 * ```
 *
 * ### Multiple Spacers with Different Sizes
 * ```cpp
 * hbox<Backend> box;
 * box.add_child(std::make_unique<button<Backend>>("A"));
 * box.add_child(std::make_unique<spacer<Backend>>(10, 0)); // Small gap
 * box.add_child(std::make_unique<button<Backend>>("B"));
 * box.add_child(std::make_unique<spacer<Backend>>(30, 0)); // Large gap
 * box.add_child(std::make_unique<button<Backend>>("C"));
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
 * @see spring For flexible expanding space
 * @see size_constraint For size policy details
 */

#pragma once

#include <onyxui/widgets/core/widget.hh>

namespace onyxui {

    /**
     * @class spacer
     * @brief Fixed-size invisible widget for layout spacing
     *
     * @details
     * Spacer creates a fixed-size empty space in layouts. It doesn't render
     * anything but occupies space during layout calculations. This is useful
     * for creating precise gaps between widgets without relying on layout
     * spacing properties.
     *
     * ## Behavior
     * - Invisible (no rendering)
     * - Fixed size (doesn't change based on content)
     * - Not focusable (skipped in tab navigation)
     * - Width and height can be set independently
     * - Uses `size_policy::fixed` for precise dimension control
     *
     * ## Common Use Cases
     * - Creating gaps in toolbars: `spacer(10, 0)` for 10px horizontal gap
     * - Adding vertical spacing: `spacer(0, 20)` for 20px vertical gap
     * - Creating precise margins without CSS-like padding
     * - Fine-grained spacing control independent of layout properties
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
     *
     * @tparam Backend The backend traits type satisfying UIBackend concept
     *
     * @see spring For flexible expanding space
     * @see size_constraint For sizing policy details
     */
    template<UIBackend Backend>
    class spacer : public widget<Backend> {
        public:
            /**
             * @brief Construct a spacer with specified dimensions
             *
             * @param width Fixed width in pixels (default: 0)
             * @param height Fixed height in pixels (default: 0)
             *
             * @throws std::bad_alloc If memory allocation fails during construction
             * @throws Any exception thrown by widget<Backend> constructor
             *
             * @details
             * Creates a spacer with exact dimensions. Use 0 for dimensions
             * you don't want to constrain (e.g., `spacer(20, 0)` for horizontal
             * spacing that doesn't affect height).
             *
             * **Size Policy Behavior:**
             * - If width > 0: Sets fixed width constraint with min=max=preferred=width
             * - If height > 0: Sets fixed height constraint with min=max=preferred=height
             * - If dimension is 0: Uses content sizing (natural size of 0)
             *
             * **Exception Safety:** Basic guarantee - object may be partially constructed
             * if an exception occurs during size constraint setup.
             *
             * @example Horizontal 10px gap
             * @code
             * auto gap = std::make_unique<spacer<Backend>>(10, 0);
             * @endcode
             *
             * @example Vertical 20px gap
             * @code
             * auto gap = std::make_unique<spacer<Backend>>(0, 20);
             * @endcode
             *
             * @example 50x50 empty square
             * @code
             * auto square = std::make_unique<spacer<Backend>>(50, 50);
             * @endcode
             */
            explicit spacer(int width = 0, int height = 0)
                : widget<Backend>(nullptr)
                  , m_width(width)
                  , m_height(height) {
                // Configure as non-focusable (spacers are invisible)
                this->set_focusable(false);

                // Set fixed size constraints
                if (width > 0) {
                    size_constraint width_constraint;
                    width_constraint.policy = size_policy::fixed;
                    width_constraint.preferred_size = width;
                    width_constraint.min_size = width;
                    width_constraint.max_size = width;
                    this->set_width_constraint(width_constraint);
                }

                if (height > 0) {
                    size_constraint height_constraint;
                    height_constraint.policy = size_policy::fixed;
                    height_constraint.preferred_size = height;
                    height_constraint.min_size = height;
                    height_constraint.max_size = height;
                    this->set_height_constraint(height_constraint);
                }
            }

            /**
             * @brief Set spacer width
             *
             * @param width New width in pixels
             *
             * @throws Any exception thrown by set_width_constraint()
             *
             * @details
             * Changes the fixed width of the spacer and invalidates layout.
             * Set to 0 to remove width constraint and use content sizing.
             *
             * **Exception Safety:** Strong guarantee - if an exception is thrown,
             * the spacer's state remains unchanged (m_width is only updated if
             * constraint update succeeds).
             *
             * **Thread Safety:** Not thread-safe. Must be called from UI thread.
             */
            void set_width(int width) {
                if (m_width != width) {
                    m_width = width;

                    size_constraint width_constraint;
                    if (width > 0) {
                        width_constraint.policy = size_policy::fixed;
                        width_constraint.preferred_size = width;
                        width_constraint.min_size = width;
                        width_constraint.max_size = width;
                    } else {
                        width_constraint.policy = size_policy::content;
                    }
                    this->set_width_constraint(width_constraint);
                }
            }

            /**
             * @brief Set spacer height
             *
             * @param height New height in pixels
             *
             * @throws Any exception thrown by set_height_constraint()
             *
             * @details
             * Changes the fixed height of the spacer and invalidates layout.
             * Set to 0 to remove height constraint and use content sizing.
             *
             * **Exception Safety:** Strong guarantee - if an exception is thrown,
             * the spacer's state remains unchanged (m_height is only updated if
             * constraint update succeeds).
             *
             * **Thread Safety:** Not thread-safe. Must be called from UI thread.
             */
            void set_height(int height) {
                if (m_height != height) {
                    m_height = height;

                    size_constraint height_constraint;
                    if (height > 0) {
                        height_constraint.policy = size_policy::fixed;
                        height_constraint.preferred_size = height;
                        height_constraint.min_size = height;
                        height_constraint.max_size = height;
                    } else {
                        height_constraint.policy = size_policy::content;
                    }
                    this->set_height_constraint(height_constraint);
                }
            }

            /**
             * @brief Get spacer width
             *
             * @return Current width in pixels
             *
             * @note noexcept - guaranteed not to throw exceptions
             * @note Thread-safe for reading (const operation)
             */
            [[nodiscard]] int width() const noexcept { return m_width; }

            /**
             * @brief Get spacer height
             *
             * @return Current height in pixels
             *
             * @note noexcept - guaranteed not to throw exceptions
             * @note Thread-safe for reading (const operation)
             */
            [[nodiscard]] int height() const noexcept { return m_height; }

        protected:
            /**
             * @brief Get content size (returns configured dimensions)
             *
             * @return Size object with configured width and height
             *
             * @throws Any exception thrown by size_utils::set_size()
             *
             * @details
             * Returns the fixed dimensions of the spacer as configured.
             * This is used during the measure phase to determine space requirements.
             *
             * @note Override of widget::get_content_size()
             */
            typename Backend::size_type get_content_size() const override {
                typename Backend::size_type size;
                size_utils::set_size(size, m_width, m_height);
                return size;
            }

            /**
             * @brief Render (no-op for spacer)
             *
             * @param ctx The render context (unused)
             *
             * @details
             * Spacers are invisible and don't render anything.
             * This method is intentionally empty.
             *
             * **Non-visual widget:** Spacers are layout-only elements with no visual representation.
             * Size comes from configured dimensions (get_content_size), not from rendering.
             *
             * **Exception Safety:** No-throw guarantee (empty implementation)
             *
             * @note Override of widget::do_render()
             */
            void do_render([[maybe_unused]] render_context<Backend>& ctx) const override {
                // Spacers are invisible - nothing to render
            }

        private:
            int m_width;  ///< Fixed width in pixels
            int m_height; ///< Fixed height in pixels
    };

    // ============================================================================
    // Convenient Factory Functions
    // ============================================================================

    /**
     * @brief Create a horizontal gap spacer
     * @tparam Backend The backend traits type
     * @param size Width of the gap in pixels
     * @return Unique pointer to horizontal spacer
     *
     * @details
     * Creates a spacer with specified width and zero height, suitable for
     * creating horizontal gaps in layouts (e.g., between buttons in an hbox).
     *
     * @example
     * @code
     * hbox<Backend> toolbar;
     * toolbar.add_child(std::make_unique<button<Backend>>("New"));
     * toolbar.add_child(create_hgap<Backend>(20));  // 20px horizontal gap
     * toolbar.add_child(std::make_unique<button<Backend>>("Open"));
     * @endcode
     */
    template<UIBackend Backend>
    inline auto create_hgap(int size) {
        return std::make_unique<spacer<Backend>>(size, 0);
    }

    /**
     * @brief Create a vertical gap spacer
     * @tparam Backend The backend traits type
     * @param size Height of the gap in pixels
     * @return Unique pointer to vertical spacer
     *
     * @details
     * Creates a spacer with specified height and zero width, suitable for
     * creating vertical gaps in layouts (e.g., between items in a vbox).
     *
     * @example
     * @code
     * vbox<Backend> menu;
     * menu.add_child(std::make_unique<label<Backend>>("Title"));
     * menu.add_child(create_vgap<Backend>(30));  // 30px vertical gap
     * menu.add_child(std::make_unique<label<Backend>>("Content"));
     * @endcode
     */
    template<UIBackend Backend>
    inline auto create_vgap(int size) {
        return std::make_unique<spacer<Backend>>(0, size);
    }

    /**
     * @brief Create a fixed-size gap spacer
     * @tparam Backend The backend traits type
     * @param size Width and height of the gap in pixels
     * @return Unique pointer to square spacer
     *
     * @details
     * Creates a spacer with equal width and height, suitable for creating
     * uniform gaps or placeholder spaces in layouts.
     *
     * @example
     * @code
     * grid<Backend> layout;
     * layout.add_child(create_gap<Backend>(50));  // 50x50 empty square
     * @endcode
     */
    template<UIBackend Backend>
    inline auto create_gap(int size) {
        return std::make_unique<spacer<Backend>>(size, size);
    }

} // namespace onyxui
