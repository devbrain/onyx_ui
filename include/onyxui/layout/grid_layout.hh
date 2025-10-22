/**
 * @file grid_layout.hh
 * @brief Grid-based layout strategy for tabular arrangements
 * @author igor
 * @date 09/10/2025
 *
 * @details
 * Grid layout arranges children in a two-dimensional grid with rows and columns,
 * similar to CSS Grid or HTML tables. Supports cell spanning, auto-sizing,
 * fixed sizing, and spacing between cells.
 *
 * ## Features
 * - Fixed or auto-calculated column/row counts
 * - Cell spanning across multiple rows/columns
 * - Auto-sizing based on content or fixed dimensions
 * - Configurable spacing between cells
 * - Per-element alignment within cells
 * - Automatic cell assignment with conflict resolution
 * - Overflow-safe arithmetic for large grids
 *
 * ## Performance Characteristics
 * - Time Complexity: O(n) for measure, O(n) for arrange where n = number of children
 * - Space Complexity: O(r*c + n) where r = rows, c = columns, n = children
 * - Recommended limits: Up to 100x100 grid for optimal performance
 * - Spanning distribution: Two-pass algorithm similar to HTML table layout
 *
 * ## Integration with UI Framework
 *
 * Grid layout integrates seamlessly with the Onyx UI framework:
 * - Respects size constraints (min/max/preferred) of child elements
 * - Honors alignment settings (horizontal and vertical) within cells
 * - Participates in two-phase layout (measure then arrange)
 * - Supports visibility toggling (hidden children don't occupy cells)
 * - Compatible with nested layouts (grids can contain other layouts)
 *
 * @see layout_strategy Base class for layout strategies
 * @see linear_layout For simpler one-dimensional layouts
 * @see anchor_layout For relative positioning layouts
 * @see ui_element Base class for UI elements
 */

#pragma once

#include <vector>
#include <unordered_map>
#include <set>
#include <onyxui/layout_strategy.hh>
#include <onyxui/utils/safe_math.hh>

namespace onyxui {
    /**
     * @struct grid_cell_info
     * @brief Grid position and span information for a child element
     *
     * @details
     * Stores the position and span information for a child element in the grid.
     * Row and column indices are 0-based. Span values indicate how many cells
     * the element occupies in each direction (minimum 1).
     *
     * @note Cell positions are validated during assignment to ensure they don't
     * exceed grid bounds when using fixed dimensions.
     */
    struct grid_cell_info {
        int row = 0; ///< Row index (0-based), top row is 0
        int column = 0; ///< Column index (0-based), leftmost column is 0
        int row_span = 1; ///< Number of rows to span (minimum 1, maximum 100)
        int column_span = 1; ///< Number of columns to span (minimum 1, maximum 100)
    };

    /**
     * @class grid_layout
     * @brief Layout strategy that arranges children in a 2D grid
     *
     * @details
     * Grid layout provides a flexible table-like arrangement with support for:
     *
     * - **Fixed column count**: Specify number of columns (rows auto-calculated)
     * - **Cell spanning**: Elements can span multiple rows/columns
     * - **Auto-sizing**: Cell sizes determined by content
     * - **Fixed sizing**: Explicit row/column dimensions
     * - **Spacing**: Gaps between cells (horizontal and vertical)
     * - **Alignment**: Per-element alignment within cells
     *
     * ## Auto-Assignment Algorithm
     *
     * Children without explicit cell assignments are placed left-to-right,
     * top-to-bottom automatically, wrapping at `num_columns`. The algorithm:
     * 1. Scans from current position (initially 0,0)
     * 2. Skips cells occupied by spanning elements
     * 3. Places child in first available cell
     * 4. Updates scan position for next child
     *
     * ## Sizing Modes
     *
     * - **Auto-size** (default): Each column takes width of widest content,
     *   each row takes height of tallest content. Spanning cells distribute
     *   their size requirements across spanned columns/rows.
     * - **Fixed-size**: Use predefined `fixed_column_widths` and `fixed_row_heights`
     *   arrays passed to constructor. If arrays are shorter than needed,
     *   remaining columns/rows default to 100 pixels.
     *
     * ## Edge Cases and Error Handling
     *
     * - **Conflicting assignments**: set_cell() validates and returns false on conflict
     * - **Out-of-bounds spanning**: Spans are clipped to grid boundaries
     * - **Zero/negative dimensions**: Constructor clamps minimum to 1 column
     * - **Excessive spanning**: Limited to 100 cells in each direction
     * - **Integer overflow**: Uses safe_math for dimension calculations
     * - **Empty grid**: Returns zero size when no children present
     *
     * ## Best Practices
     *
     * - Keep grids under 100x100 for optimal performance
     * - Use fixed sizing for predictable layouts
     * - Minimize spanning cells for simpler calculations
     * - Set explicit positions for important elements, let others auto-assign
     * - Use consistent spacing for visual harmony
     *
     * ## Thread Safety
     *
     * - **Immutable configuration**: All grid parameters are const after construction
     * - **Mutable state**: Column widths, row heights, and cell mappings are mutable
     *   for lazy calculation during const measure operations
     * - **Not thread-safe**: Do not access the same grid_layout instance from multiple
     *   threads concurrently. Layout operations (measure/arrange) and cell modifications
     *   (set_cell) must be performed on a single thread (typically the UI thread).
     * - **Parent hierarchy**: Follows same thread safety model as ui_element tree
     *
     * @thread_safety Not thread-safe. Single-threaded access only (typically UI thread).
     *                ONYXUI_THREAD_SAFE flag does not affect grid_layout thread safety.
     *
     * @tparam Backend The backend traits type providing rect and size types
     *
     * @example
     * @code
     * // Create a 3-column auto-sizing grid with spacing
     * auto grid = std::make_unique<grid_layout<MyBackend>>(
     *     3,    // 3 columns
     *     -1,   // Auto-calculate rows
     *     10,   // 10px horizontal spacing
     *     10,   // 10px vertical spacing
     *     true  // Auto-size cells
     * );
     * parent->set_layout_strategy(std::move(grid));
     *
     * // Children will be auto-assigned to cells
     * for (int i = 0; i < 9; i++) {
     *     parent->add_child(create_button(std::to_string(i)));
     * }
     * @endcode
     *
     * @example
     * @code
     * auto grid = std::make_unique<grid_layout<MyBackend>>(4); // 4 columns
     * parent->set_layout_strategy(grid);
     *
     * // Create header that spans all 4 columns
     * auto header = create_header();
     * parent->add_child(header);
     * grid->set_cell(header.get(), 0, 0, 1, 4);  // row 0, col 0, span 1x4
     *
     * // Create sidebar that spans 3 rows
     * auto sidebar = create_sidebar();
     * parent->add_child(sidebar);
     * grid->set_cell(sidebar.get(), 1, 0, 3, 1);  // row 1, col 0, span 3x1
     *
     * // Remaining children auto-assigned around occupied cells
     * @endcode
     *
     * @example
     * @code
     * // Create grid with fixed column widths
     * std::vector<int> col_widths = {100, 200, 100};  // 3 columns: narrow, wide, narrow
     * std::vector<int> row_heights = {50, 100, 100, 50};  // 4 rows of different heights
     *
     * auto grid = std::make_unique<grid_layout<MyBackend>>(
     *     3,           // 3 columns
     *     4,           // 4 rows
     *     5, 5,        // spacing
     *     false,       // Use fixed sizing
     *     col_widths,  // Fixed column widths
     *     row_heights  // Fixed row heights
     * );
     * @endcode
     */
    template<UIBackend Backend>
    class grid_layout : public layout_strategy<Backend> {
        // PUBLIC inheritance
        public:
            using elt_t = ui_element<Backend>;
            using rect_type = typename Backend::rect_type;
            using size_type = typename Backend::size_type;

            /**
             * @brief Construct grid layout with immutable configuration
             *
             * @param num_columns Number of columns in grid (minimum 1, clamped if less)
             * @param num_rows Number of rows (-1 for auto-calculate based on children)
             * @param column_spacing Horizontal gap between cells in pixels
             * @param row_spacing Vertical gap between cells in pixels
             * @param auto_size If true, size cells from content; if false, use fixed sizes
             * @param fixed_column_widths Array of fixed column widths in pixels (ignored if auto_size=true).
             *                            If shorter than num_columns, remaining columns default to 100px
             * @param fixed_row_heights Array of fixed row heights in pixels (ignored if auto_size=true).
             *                          If shorter than actual rows, remaining rows default to 100px
             *
             * @note All parameters are immutable after construction to ensure thread safety
             *       and predictable layout behavior.
             *
             * @warning If num_columns < 1, it will be clamped to 1. Negative spacing values
             *          are allowed but may produce overlapping cells.
             */
            explicit grid_layout(
                int num_columns = 1,
                int num_rows = -1,
                int column_spacing = 0,
                int row_spacing = 0,
                bool auto_size = true,
                std::vector <int> fixed_column_widths = {},
                std::vector <int> fixed_row_heights = {}
            );

            /**
             * @brief Destructor
             */
            ~grid_layout() override = default;

            // Rule of Five
            grid_layout(const grid_layout&) = delete;
            grid_layout& operator=(const grid_layout&) = delete;
            grid_layout(grid_layout&&) noexcept = default;
            grid_layout& operator=(grid_layout&&) noexcept = default;

            /**
             * @brief Explicitly assign a child to a grid cell
             *
             * Validates all parameters to ensure cell assignment is within bounds.
             * For fixed row count, validates that row + row_span doesn't exceed it.
             * Always validates that column + col_span doesn't exceed num_columns.
             *
             * @param child Pointer to the child element
             * @param row Row index (0-based)
             * @param col Column index (0-based)
             * @param row_span Number of rows to span (default 1)
             * @param col_span Number of columns to span (default 1)
             * @return true if cell was successfully assigned, false if validation failed
             */
            bool set_cell(elt_t* child, int row, int col,
                          int row_span = 1, int col_span = 1);

            /**
             * @brief Get number of columns
             * @return Number of columns
             */
            [[nodiscard]] int num_columns() const noexcept { return m_num_columns; }

            /**
             * @brief Get number of rows
             * @return Number of rows (-1 if auto-calculated)
             */
            [[nodiscard]] int num_rows() const noexcept { return m_num_rows; }

        protected:
            /**
             * @brief Measure all children and calculate total grid size
             *
             * @param parent Parent element whose children to measure
             * @param available_width Maximum width available for the grid
             * @param available_height Maximum height available for the grid
             * @return Total size required by the grid including spacing
             *
             * @details
             * Override of layout_strategy::measure_children. Performs:
             * 1. Auto-assigns unpositioned children to cells
             * 2. Calculates actual row count if auto-calculating
             * 3. Measures all children (auto-size mode) or applies fixed sizes
             * 4. Calculates total grid dimensions with spacing
             *
             * @note This is a const method but modifies mutable layout state
             */
            size_type measure_children(const elt_t* parent,
                                   int available_width,
                                   int available_height) const override;

            /**
             * @brief Arrange all children within the content area
             *
             * @param parent Parent element whose children to arrange
             * @param content_area Rectangle defining the area for child arrangement
             *
             * @details
             * Override of layout_strategy::arrange_children. Performs:
             * 1. Calculates cell positions from column/row sizes
             * 2. For each child, calculates its cell bounds (including spans)
             * 3. Applies alignment within cell
             * 4. Calls child->arrange() with final bounds
             */
            void arrange_children(elt_t* parent,
                                  const rect_type& content_area) override;

            /**
             * @brief Handle child removal notification
             *
             * @param child Child that was removed
             *
             * @details
             * Override of layout_strategy::on_child_removed.
             * Removes the child's cell assignment from mapping.
             */
            void on_child_removed(elt_t* child) noexcept override {
                m_cell_mapping.erase(child);
            }

            /**
             * @brief Handle all children cleared notification
             *
             * @details
             * Override of layout_strategy::on_children_cleared.
             * Clears all cell assignments.
             */
            void on_children_cleared() noexcept override {
                m_cell_mapping.clear();
            }

        private:
            // Immutable configuration
            const int m_num_columns; ///< Number of columns (minimum 1)
            const int m_num_rows; ///< Number of rows (-1 for auto)
            const int m_column_spacing; ///< Horizontal spacing between cells
            const int m_row_spacing; ///< Vertical spacing between cells
            const bool m_auto_size_cells; ///< True to size from content
            const std::vector <int> m_fixed_column_widths; ///< Fixed column widths (if not auto-sizing)
            const std::vector <int> m_fixed_row_heights; ///< Fixed row heights (if not auto-sizing)

            // Calculated sizing state (modified during measure)
            mutable std::vector <int> m_column_widths; ///< Calculated column widths for current layout
            mutable std::vector <int> m_row_heights; ///< Calculated row heights for current layout

            // Cell mapping (positional data, not config)
            mutable std::unordered_map <elt_t*, grid_cell_info> m_cell_mapping; ///< Child to cell position mapping

            // Private helpers
            /**
             * @brief Auto-assign children to cells left-to-right, top-to-bottom
             *
             * @param parent Parent element containing children to assign
             *
             * @details
             * Implements a scan-line algorithm that:
             * 1. Tracks occupied cells from explicitly positioned children
             * 2. For each unassigned child, finds next free cell
             * 3. Assigns child to that cell with 1x1 span
             * 4. Updates occupied cell tracker
             *
             * @note Called during measure phase, modifies mutable m_cell_mapping
             */
            void auto_assign_cells(const elt_t* parent) const;

            /**
             * @brief Calculate number of rows needed based on child positions
             *
             * @param parent Parent element
             * @return Number of rows required (minimum 1)
             *
             * @details
             * If m_num_rows is fixed (>0), returns it directly.
             * Otherwise calculates based on maximum row+span of all children.
             */
            int calculate_row_count(const elt_t* parent) const;

            /**
             * @brief Measure grid with auto-sizing enabled
             *
             * @param parent Parent element
             * @param available_width Maximum width available for layout
             * @param available_height Maximum height available for layout
             * @param actual_rows Number of rows in grid
             *
             * @details
             * Implements two-pass measurement:
             * 1. First pass: Measures non-spanning cells, sets column/row sizes
             * 2. Second pass: Handles spanning cells via distribute_spanning_sizes()
             *
             * @note Updates mutable m_column_widths and m_row_heights arrays
             */
            void measure_auto_sized_grid(const elt_t* parent,
                                         int available_width,
                                         int available_height,
                                         int actual_rows) const;

            /**
             * @brief Apply fixed or default grid sizes
             *
             * @param actual_rows Number of rows needed
             *
             * @details
             * Uses m_fixed_column_widths/m_fixed_row_heights if provided,
             * otherwise defaults to 100px per column/row.
             */
            void use_fixed_grid_sizes(int actual_rows) const;

            /**
             * @brief Distribute spanning cell sizes across columns/rows
             * @param parent Parent element
             *
             * This implements a two-pass algorithm similar to HTML table layout:
             * 1. First pass: Size non-spanning cells
             * 2. Second pass: Distribute spanning cell requirements
             */
            void distribute_spanning_sizes(const elt_t* parent) const;

            /**
             * @brief Calculate which cells are occupied (including by spanning cells)
             * @return Set of (row, col) pairs that are occupied
             */
            std::set <std::pair <int, int>> calculate_occupied_cells() const;

            /**
             * @brief Find the next free cell starting from given position
             * @param occupied Set of occupied cells
             * @param start_row Starting row for search
             * @param start_col Starting column for search
             * @return Pair of (row, col) for next free cell
             */
            std::pair <int, int> find_next_free_cell(
                const std::set <std::pair <int, int>>& occupied,
                int start_row, int start_col) const;
    };

    // ==================================================================================================
    // Implementation
    // ==================================================================================================

    template<UIBackend Backend>
    grid_layout<Backend>::grid_layout(int num_columns, int num_rows, int column_spacing, int row_spacing,
                                            bool auto_size, std::vector <int> fixed_column_widths,
                                            std::vector <int> fixed_row_heights)
        : m_num_columns(num_columns < 1 ? 1 : num_columns)
          , m_num_rows(num_rows)
          , m_column_spacing(column_spacing)
          , m_row_spacing(row_spacing)
          , m_auto_size_cells(auto_size)
          , m_fixed_column_widths(std::move(fixed_column_widths))
          , m_fixed_row_heights(std::move(fixed_row_heights)) {
    }

    // -------------------------------------------------------------------------------------------------------
    template<UIBackend Backend>
    bool grid_layout<Backend>::set_cell(elt_t* child, int row, int col, int row_span, int col_span) {
        // Null pointer check
        if (!child) {
            return false;
        }

        // Basic parameter validation
        if (row < 0 || col < 0 || row_span < 1 || col_span < 1) {
            return false;
        }

        // Column bounds check
        if (col >= m_num_columns) {
            return false; // Column index out of bounds
        }

        if (col + col_span > m_num_columns) {
            return false; // Column span exceeds grid width
        }

        // Row bounds check (only if fixed row count)
        if (m_num_rows > 0) {
            if (row >= m_num_rows) {
                return false; // Row index out of bounds
            }

            if (row + row_span > m_num_rows) {
                return false; // Row span exceeds grid height
            }
        }

        // Maximum span sanity check (prevent extreme values)
        constexpr int MAX_SPAN = 100; // Reasonable maximum
        if (row_span > MAX_SPAN || col_span > MAX_SPAN) {
            return false;
        }

        // All validation passed, assign the cell
        m_cell_mapping[child] = {row, col, row_span, col_span};
        return true;
    }

    // -------------------------------------------------------------------------------------------------------
    template<UIBackend Backend>
    typename Backend::size_type grid_layout<Backend>::measure_children(const elt_t* parent, int available_width,
                                                       int available_height) const {
        if (this->get_children(parent).empty()) {
            size_type result = {};
            size_utils::set_size(result, 0, 0);
            return result;
        }

        // Auto-assign cells if not explicitly set
        auto_assign_cells(parent);

        // Calculate actual row count
        const int actual_rows = calculate_row_count(parent);

        if (m_auto_size_cells) {
            // Measure all children to determine cell sizes
            measure_auto_sized_grid(parent, available_width, available_height,
                                    actual_rows);
        } else {
            // Use predefined sizes
            use_fixed_grid_sizes(actual_rows);
        }

        // Calculate total grid size with overflow protection
        int total_width = 0;
        for (int const w : m_column_widths) {
            safe_math::accumulate_safe(total_width, w);
        }
        int column_spacing_total = 0;
        if (m_num_columns > 1) {
            column_spacing_total = safe_math::multiply_clamped(m_column_spacing, m_num_columns - 1);
        }
        safe_math::accumulate_safe(total_width, column_spacing_total);

        int total_height = 0;
        for (int const h : m_row_heights) {
            safe_math::accumulate_safe(total_height, h);
        }
        int row_spacing_total = 0;
        if (actual_rows > 1) {
            row_spacing_total = safe_math::multiply_clamped(m_row_spacing, actual_rows - 1);
        }
        safe_math::accumulate_safe(total_height, row_spacing_total);

        size_type result = {};
        size_utils::set_size(result, total_width, total_height);
        return result;
    }

    // -------------------------------------------------------------------------------------------------------
    template<UIBackend Backend>
    void grid_layout<Backend>::arrange_children(elt_t* parent, const rect_type& content_area) {
        if (parent->children().empty()) return;

        const int actual_rows = static_cast<int>(m_row_heights.size());

        // Calculate cell positions
        std::vector <int> column_positions(static_cast<size_t>(m_num_columns));
        std::vector <int> row_positions(static_cast<size_t>(actual_rows));

        int const content_x = rect_utils::get_x(content_area);
        int const content_y = rect_utils::get_y(content_area);

        column_positions[0] = content_x;
        for (int i = 1; i < m_num_columns; ++i) {
            column_positions[static_cast<size_t>(i)] = column_positions[static_cast<size_t>(i - 1)] +
                                  m_column_widths[static_cast<size_t>(i - 1)] + m_column_spacing;
        }

        row_positions[0] = content_y;
        for (int i = 1; i < actual_rows; ++i) {
            row_positions[static_cast<size_t>(i)] = row_positions[static_cast<size_t>(i - 1)] +
                               m_row_heights[static_cast<size_t>(i - 1)] + m_row_spacing;
        }

        // Arrange each child
        for (auto& child : this->get_mutable_children(parent)) {
            if (!child->is_visible()) continue;

            auto it = m_cell_mapping.find(child.get());
            if (it == m_cell_mapping.end()) continue;

            const grid_cell_info& cell = it->second;

            // Calculate cell bounds
            int const cell_x = column_positions[static_cast<size_t>(cell.column)];
            int const cell_y = row_positions[static_cast<size_t>(cell.row)];

            int cell_width = 0;
            for (int i = 0; i < cell.column_span; ++i) {
                if (cell.column + i < m_num_columns) {
                    cell_width += m_column_widths[static_cast<size_t>(cell.column + i)];
                    if (i < cell.column_span - 1) {
                        cell_width += m_column_spacing;
                    }
                }
            }

            int cell_height = 0;
            for (int i = 0; i < cell.row_span; ++i) {
                if (cell.row + i < actual_rows) {
                    cell_height += m_row_heights[static_cast<size_t>(cell.row + i)];
                    if (i < cell.row_span - 1) {
                        cell_height += m_row_spacing;
                    }
                }
            }

            // Apply alignment within cell
            size_type const measured = this->get_last_measured_size(child.get());
            int const meas_w = size_utils::get_width(measured);
            int const meas_h = size_utils::get_height(measured);

            int child_width = meas_w;
            int child_height = meas_h;
            int child_x = cell_x;
            int child_y = cell_y;

            if (this->get_h_align(child.get()) == horizontal_alignment::stretch) {
                child_width = cell_width;
            } else {
                child_width = std::min(meas_w, cell_width);
                if (this->get_h_align(child.get()) == horizontal_alignment::center) {
                    child_x = cell_x + (cell_width - child_width) / 2;
                } else if (this->get_h_align(child.get()) == horizontal_alignment::right) {
                    child_x = cell_x + cell_width - child_width;
                }
            }

            if (this->get_v_align(child.get()) == vertical_alignment::stretch) {
                child_height = cell_height;
            } else {
                child_height = std::min(meas_h, cell_height);
                if (this->get_v_align(child.get()) == vertical_alignment::center) {
                    child_y = cell_y + (cell_height - child_height) / 2;
                } else if (this->get_v_align(child.get()) == vertical_alignment::bottom) {
                    child_y = cell_y + cell_height - child_height;
                }
            }

            rect_type child_bounds;
            rect_utils::set_bounds(child_bounds, child_x, child_y,
                                   child_width, child_height);
            child->arrange(child_bounds);
        }
    }

    // -------------------------------------------------------------------------------------------------------
    template<UIBackend Backend>
    void grid_layout<Backend>::auto_assign_cells(const elt_t* parent) const {
        // Calculate which cells are already occupied by explicitly positioned cells
        auto occupied = calculate_occupied_cells();

        int current_row = 0;
        int current_col = 0;

        for (const auto& child : this->get_children(parent)) {
            if (!child->is_visible()) continue;

            // Skip if already assigned
            if (m_cell_mapping.find(child.get()) != m_cell_mapping.end()) {
                continue;
            }

            // Find next free cell (accounting for occupied cells)
            auto [free_row, free_col] = find_next_free_cell(occupied, current_row, current_col);

            // Assign child to the free cell
            m_cell_mapping[child.get()] = {free_row, free_col, 1, 1};

            // Mark this cell as occupied
            occupied.insert({free_row, free_col});

            // Update current position for next search
            current_col = free_col + 1;
            current_row = free_row;

            if (current_col >= m_num_columns) {
                current_col = 0;
                current_row++;
            }
        }
    }

    // -------------------------------------------------------------------------------------------------------
    template<UIBackend Backend>
    int grid_layout<Backend>::calculate_row_count(const elt_t* parent) const {
        if (m_num_rows > 0) return m_num_rows;

        int max_row = 0;
        for (const auto& child : this->get_children(parent)) {
            if (!child->is_visible()) continue;

            auto it = m_cell_mapping.find(child.get());
            if (it != m_cell_mapping.end()) {
                int const end_row = it->second.row + it->second.row_span;
                max_row = std::max(max_row, end_row);
            }
        }

        return std::max(1, max_row);
    }

    // -------------------------------------------------------------------------------------------------------
    template<UIBackend Backend>
    void grid_layout<Backend>::measure_auto_sized_grid(const elt_t* parent, int available_width,
                                                             int available_height,
                                                             int actual_rows) const {
        m_column_widths.resize(static_cast<size_t>(m_num_columns), 0);
        m_row_heights.resize(static_cast<size_t>(actual_rows), 0);

        // First pass: Measure non-spanning cells
        for (const auto& child : this->get_children(parent)) {
            if (!child->is_visible()) continue;

            auto it = m_cell_mapping.find(child.get());
            if (it == m_cell_mapping.end()) continue;

            const grid_cell_info& cell = it->second;

            // Only process single-span cells in first pass
            if (cell.column_span == 1 || cell.row_span == 1) {
                // Measure child with unconstrained size
                size_type const measured = child->measure(available_width, available_height);
                int const meas_w = size_utils::get_width(measured);
                int const meas_h = size_utils::get_height(measured);

                // Update column sizes for single-column spans
                if (cell.column_span == 1) {
                    m_column_widths[static_cast<size_t>(cell.column)] =
                        std::max(m_column_widths[static_cast<size_t>(cell.column)], meas_w);
                }

                // Update row sizes for single-row spans
                if (cell.row_span == 1) {
                    m_row_heights[static_cast<size_t>(cell.row)] =
                        std::max(m_row_heights[static_cast<size_t>(cell.row)], meas_h);
                }
            }
        }

        // Second pass: Handle spanning cells
        distribute_spanning_sizes(parent);
    }

    // -------------------------------------------------------------------------------------------------------
    template<UIBackend Backend>
    void grid_layout<Backend>::use_fixed_grid_sizes(int actual_rows) const {
        // Use fixed widths if provided, otherwise default
        if (!m_fixed_column_widths.empty()) {
            m_column_widths = m_fixed_column_widths;
        } else {
            m_column_widths.resize(static_cast<size_t>(m_num_columns), 100); // Default width
        }

        // Use fixed heights if provided, otherwise default
        if (!m_fixed_row_heights.empty()) {
            m_row_heights = m_fixed_row_heights;
        } else {
            m_row_heights.resize(static_cast<size_t>(actual_rows), 100); // Default height
        }
    }

    // -------------------------------------------------------------------------------------------------------
    template<UIBackend Backend>
    void grid_layout<Backend>::distribute_spanning_sizes(const elt_t* parent) const {
        // Process spanning cells (cells that span multiple columns or rows)
        // We need to ensure the spanned columns/rows together are at least as wide/tall
        // as the spanning cell requires

        // First handle column spanning
        for (const auto& child : this->get_children(parent)) {
            if (!child->is_visible()) continue;

            auto it = m_cell_mapping.find(child.get());
            if (it == m_cell_mapping.end()) continue;

            const grid_cell_info& cell = it->second;

            // Handle cells that span multiple columns
            if (cell.column_span > 1) {
                // Get the child's measured size
                size_type const measured = this->get_last_measured_size(child.get());
                int const required_width = size_utils::get_width(measured);

                // Calculate current total width of spanned columns
                int current_total = 0;
                for (int i = 0; i < cell.column_span && (cell.column + i) < m_num_columns; ++i) {
                    current_total += m_column_widths[static_cast<size_t>(cell.column + i)];
                    if (i > 0) {
                        current_total += m_column_spacing; // Include spacing between columns
                    }
                }

                // If the spanning cell needs more space, distribute it evenly
                if (required_width > current_total) {
                    int const extra_needed = required_width - current_total;
                    int const columns_to_expand = std::min(cell.column_span,
                                                     m_num_columns - cell.column);

                    if (columns_to_expand > 0) {
                        int const extra_per_column = extra_needed / columns_to_expand;
                        int const remainder = extra_needed % columns_to_expand;

                        for (int i = 0; i < columns_to_expand; ++i) {
                            m_column_widths[static_cast<size_t>(cell.column + i)] += extra_per_column;
                            // Distribute remainder to first columns
                            if (i < remainder) {
                                m_column_widths[static_cast<size_t>(cell.column + i)] += 1;
                            }
                        }
                    }
                }
            }
        }

        // Then handle row spanning (same logic)
        for (const auto& child : this->get_children(parent)) {
            if (!child->is_visible()) continue;

            auto it = m_cell_mapping.find(child.get());
            if (it == m_cell_mapping.end()) continue;

            const grid_cell_info& cell = it->second;

            // Handle cells that span multiple rows
            if (cell.row_span > 1) {
                // Get the child's measured size
                size_type const measured = this->get_last_measured_size(child.get());
                int const required_height = size_utils::get_height(measured);

                // Calculate current total height of spanned rows
                int current_total = 0;
                int const actual_rows = static_cast<int>(m_row_heights.size());
                for (int i = 0; i < cell.row_span && (cell.row + i) < actual_rows; ++i) {
                    current_total += m_row_heights[static_cast<size_t>(cell.row + i)];
                    if (i > 0) {
                        current_total += m_row_spacing; // Include spacing between rows
                    }
                }

                // If the spanning cell needs more space, distribute it evenly
                if (required_height > current_total) {
                    int const extra_needed = required_height - current_total;
                    int const rows_to_expand = std::min(cell.row_span,
                                                  actual_rows - cell.row);

                    if (rows_to_expand > 0) {
                        int const extra_per_row = extra_needed / rows_to_expand;
                        int const remainder = extra_needed % rows_to_expand;

                        for (int i = 0; i < rows_to_expand; ++i) {
                            m_row_heights[static_cast<size_t>(cell.row + i)] += extra_per_row;
                            // Distribute remainder to first rows
                            if (i < remainder) {
                                m_row_heights[static_cast<size_t>(cell.row + i)] += 1;
                            }
                        }
                    }
                }
            }
        }
    }

    // -------------------------------------------------------------------------------------------------------
    template<UIBackend Backend>
    std::set <std::pair <int, int>> grid_layout<Backend>::calculate_occupied_cells() const {
        std::set <std::pair <int, int>> occupied;

        // Mark all cells that are occupied by existing cell mappings
        for (const auto& [child, info] : m_cell_mapping) {
            // For each mapped cell, mark all cells it spans as occupied
            for (int r = info.row; r < info.row + info.row_span; ++r) {
                for (int c = info.column; c < info.column + info.column_span; ++c) {
                    // Only mark cells that are within grid bounds
                    if (c < m_num_columns && (m_num_rows < 0 || r < m_num_rows)) {
                        occupied.insert({r, c});
                    }
                }
            }
        }

        return occupied;
    }

    // -------------------------------------------------------------------------------------------------------
    template<UIBackend Backend>
    std::pair <int, int> grid_layout<Backend>::find_next_free_cell(
        const std::set <std::pair <int, int>>& occupied,
        int start_row, int start_col) const {
        int row = start_row;
        int col = start_col;

        // Keep searching until we find a free cell
        while (occupied.count({row, col}) > 0) {
            col++;
            if (col >= m_num_columns) {
                col = 0;
                row++;
            }

            // Safety check: prevent infinite loop in degenerate cases
            // If we've gone past a reasonable limit, just return the position
            if (row > 1000) {
                break;
            }
        }

        return {row, col};
    }
}
