/**
 * @file grid_layout.hh
 * @brief Grid-based layout strategy for tabular arrangements
 * @author igor
 * @date 09/10/2025
 *
 * Grid layout arranges children in a two-dimensional grid with rows and columns,
 * similar to CSS Grid or HTML tables. Supports cell spanning, auto-sizing,
 * fixed sizing, and spacing between cells.
 */

#pragma once

#include <vector>
#include <unordered_map>
#include <set>
#include <onyxui/layout_strategy.hh>

namespace onyxui {
    /**
     * @struct grid_cell_info
     * @brief Grid position and span information for a child element
     */
    struct grid_cell_info {
        int row = 0;          ///< Row index (0-based)
        int column = 0;       ///< Column index (0-based)
        int row_span = 1;     ///< Number of rows to span
        int column_span = 1;  ///< Number of columns to span
    };

    /**
     * @class grid_layout
     * @brief Layout strategy that arranges children in a 2D grid
     *
     * Grid layout provides a flexible table-like arrangement with support for:
     *
     * - **Fixed column count**: Specify number of columns (rows auto-calculated)
     * - **Cell spanning**: Elements can span multiple rows/columns
     * - **Auto-sizing**: Cell sizes determined by content
     * - **Fixed sizing**: Explicit row/column dimensions
     * - **Spacing**: Gaps between cells (horizontal and vertical)
     * - **Alignment**: Per-element alignment within cells
     *
     * ## Auto-Assignment
     *
     * Children without explicit cell assignments are placed left-to-right,
     * top-to-bottom automatically, wrapping at `num_columns`.
     *
     * ## Sizing Modes
     *
     * - **Auto-size** (default): Each column takes width of widest content,
     *   each row takes height of tallest content
     * - **Fixed-size**: Use predefined `column_widths` and `row_heights`
     *
     * @tparam TRect Rectangle type satisfying RectLike concept
     * @tparam TSize Size type satisfying SizeLike concept
     *
     * @example
     * @code
     * // Create 3-column grid
     * auto layout = std::make_unique<grid_layout<SDL_Rect, SDL_Size>>();
     * layout->num_columns = 3;
     * layout->column_spacing = 5;
     * layout->row_spacing = 5;
     * panel->set_layout_strategy(std::move(layout));
     *
     * // Add 9 children (auto-assigned to 3x3 grid)
     * for (int i = 0; i < 9; i++) {
     *     auto cell = std::make_unique<ui_element<SDL_Rect, SDL_Size>>(panel.get());
     *     panel->add_child(std::move(cell));
     * }
     *
     * // Or explicitly position with spanning
     * layout->set_cell(header.get(), 0, 0, 1, 3);  // Span 3 columns
     * @endcode
     */
    template<RectLike TRect, SizeLike TSize>
    class grid_layout : public layout_strategy <TRect, TSize> {  // PUBLIC inheritance
    public:
        using elt_t = ui_element <TRect, TSize>;

        /**
         * @brief Construct grid layout with immutable configuration
         * @param num_columns Number of columns in grid
         * @param num_rows Number of rows (-1 for auto-calculate)
         * @param column_spacing Horizontal gap between cells
         * @param row_spacing Vertical gap between cells
         * @param auto_size If true, size cells from content
         * @param column_widths Fixed column widths (empty for auto)
         * @param row_heights Fixed row heights (empty for auto)
         */
        explicit grid_layout(
            int num_columns = 1,
            int num_rows = -1,
            int column_spacing = 0,
            int row_spacing = 0,
            bool auto_size = true,
            std::vector<int> column_widths = {},
            std::vector<int> row_heights = {}
        )
            : m_num_columns(num_columns < 1 ? 1 : num_columns)
            , m_num_rows(num_rows)
            , m_column_spacing(column_spacing)
            , m_row_spacing(row_spacing)
            , m_auto_size_cells(auto_size)
            , m_column_widths(std::move(column_widths))
            , m_row_heights(std::move(row_heights))
        {}

        /**
         * @brief Explicitly assign a child to a grid cell
         * @param child Pointer to the child element
         * @param row Row index (0-based)
         * @param col Column index (0-based)
         * @param row_span Number of rows to span (default 1)
         * @param col_span Number of columns to span (default 1)
         */
        void set_cell(elt_t* child, int row, int col,
                      int row_span = 1, int col_span = 1) {
            if (!child || row < 0 || col < 0 || row_span < 1 || col_span < 1) {
                return;  // Validation
            }
            m_cell_mapping[child] = {row, col, row_span, col_span};
        }

        /**
         * @brief Get number of columns
         * @return Number of columns
         */
        int num_columns() const noexcept { return m_num_columns; }

        /**
         * @brief Get number of rows
         * @return Number of rows (-1 if auto-calculated)
         */
        int num_rows() const noexcept { return m_num_rows; }

    private:
        // Immutable configuration
        const int m_num_columns;
        const int m_num_rows;
        const int m_column_spacing;
        const int m_row_spacing;
        const bool m_auto_size_cells;

        // Mutable sizing state (calculated during measure)
        mutable std::vector<int> m_column_widths;
        mutable std::vector<int> m_row_heights;

        // Mutable cell mapping (positional data, not config)
        std::unordered_map<elt_t*, grid_cell_info> m_cell_mapping;

        // Private override methods
        TSize measure_children(elt_t* parent,
                               int available_width,
                               int available_height) override;

        void arrange_children(elt_t* parent,
                              const TRect& content_area) override;

        // Private helpers
        /**
         * @brief Auto-assign children to cells left-to-right, top-to-bottom
         * @param parent Parent element
         */
        void auto_assign_cells(elt_t* parent);

        /**
         * @brief Calculate number of rows needed
         * @param parent Parent element
         * @return Number of rows
         */
        int calculate_row_count(elt_t* parent);

        /**
         * @brief Measure grid with auto-sizing enabled
         * @param parent Parent element
         * @param available_width Available width
         * @param available_height Available height
         * @param actual_rows Number of rows
         */
        void measure_auto_sized_grid(elt_t* parent,
                                     int available_width,
                                     int available_height,
                                     int actual_rows);

        /**
         * @brief Use fixed/default grid sizes
         * @param actual_rows Number of rows
         */
        void use_fixed_grid_sizes(int actual_rows);

        /**
         * @brief Distribute spanning cell sizes across columns/rows
         * @param parent Parent element
         *
         * This implements a two-pass algorithm similar to HTML table layout:
         * 1. First pass: Size non-spanning cells
         * 2. Second pass: Distribute spanning cell requirements
         */
        void distribute_spanning_sizes(elt_t* parent);

        /**
         * @brief Calculate which cells are occupied (including by spanning cells)
         * @return Set of (row, col) pairs that are occupied
         */
        std::set<std::pair<int, int>> calculate_occupied_cells() const;

        /**
         * @brief Find the next free cell starting from given position
         * @param occupied Set of occupied cells
         * @param start_row Starting row for search
         * @param start_col Starting column for search
         * @return Pair of (row, col) for next free cell
         */
        std::pair<int, int> find_next_free_cell(
            const std::set<std::pair<int, int>>& occupied,
            int start_row, int start_col) const;
    };

    // ==================================================================================================
    // Implementation
    // ==================================================================================================

    // -------------------------------------------------------------------------------------------------------
    template<RectLike TRect, SizeLike TSize>
    TSize grid_layout <TRect, TSize>::measure_children(elt_t* parent, int available_width, int available_height) {
        if (parent->children().empty()) {
            TSize result = {};
            size_utils::set_size(result, 0, 0);
            return result;
        }

        // Auto-assign cells if not explicitly set
        auto_assign_cells(parent);

        // Calculate actual row count
        int actual_rows = calculate_row_count(parent);

        if (m_auto_size_cells) {
            // Measure all children to determine cell sizes
            measure_auto_sized_grid(parent, available_width, available_height,
                                    actual_rows);
        } else {
            // Use predefined sizes
            use_fixed_grid_sizes(actual_rows);
        }

        // Calculate total grid size
        int total_width = 0;
        for (int w : m_column_widths) total_width += w;
        total_width += m_column_spacing * (m_num_columns - 1);

        int total_height = 0;
        for (int h : m_row_heights) total_height += h;
        total_height += m_row_spacing * (actual_rows - 1);

        TSize result = {};
        size_utils::set_size(result, total_width, total_height);
        return result;
    }

    // -------------------------------------------------------------------------------------------------------
    template<RectLike TRect, SizeLike TSize>
    void grid_layout <TRect, TSize>::arrange_children(elt_t* parent, const TRect& content_area) {
        if (parent->children().empty()) return;

        int actual_rows = m_row_heights.size();

        // Calculate cell positions
        std::vector <int> column_positions(m_num_columns);
        std::vector <int> row_positions(actual_rows);

        int content_x = rect_utils::get_x(content_area);
        int content_y = rect_utils::get_y(content_area);

        column_positions[0] = content_x;
        for (int i = 1; i < m_num_columns; ++i) {
            column_positions[i] = column_positions[i - 1] +
                                  m_column_widths[i - 1] + m_column_spacing;
        }

        row_positions[0] = content_y;
        for (int i = 1; i < actual_rows; ++i) {
            row_positions[i] = row_positions[i - 1] +
                               m_row_heights[i - 1] + m_row_spacing;
        }

        // Arrange each child
        for (auto& child : parent->children()) {
            if (!child->is_visible()) continue;

            auto it = m_cell_mapping.find(child.get());
            if (it == m_cell_mapping.end()) continue;

            const grid_cell_info& cell = it->second;

            // Calculate cell bounds
            int cell_x = column_positions[cell.column];
            int cell_y = row_positions[cell.row];

            int cell_width = 0;
            for (int i = 0; i < cell.column_span; ++i) {
                if (cell.column + i < m_num_columns) {
                    cell_width += m_column_widths[cell.column + i];
                    if (i < cell.column_span - 1) {
                        cell_width += m_column_spacing;
                    }
                }
            }

            int cell_height = 0;
            for (int i = 0; i < cell.row_span; ++i) {
                if (cell.row + i < actual_rows) {
                    cell_height += m_row_heights[cell.row + i];
                    if (i < cell.row_span - 1) {
                        cell_height += m_row_spacing;
                    }
                }
            }

            // Apply alignment within cell
            TSize measured = child->last_measured_size();
            int meas_w = size_utils::get_width(measured);
            int meas_h = size_utils::get_height(measured);

            int child_width = meas_w;
            int child_height = meas_h;
            int child_x = cell_x;
            int child_y = cell_y;

            if (child->h_align() == horizontal_alignment::stretch) {
                child_width = cell_width;
            } else {
                child_width = std::min(meas_w, cell_width);
                if (child->h_align() == horizontal_alignment::center) {
                    child_x = cell_x + (cell_width - child_width) / 2;
                } else if (child->h_align() == horizontal_alignment::right) {
                    child_x = cell_x + cell_width - child_width;
                }
            }

            if (child->v_align() == vertical_alignment::stretch) {
                child_height = cell_height;
            } else {
                child_height = std::min(meas_h, cell_height);
                if (child->v_align() == vertical_alignment::center) {
                    child_y = cell_y + (cell_height - child_height) / 2;
                } else if (child->v_align() == vertical_alignment::bottom) {
                    child_y = cell_y + cell_height - child_height;
                }
            }

            TRect child_bounds;
            rect_utils::set_bounds(child_bounds, child_x, child_y,
                                   child_width, child_height);
            child->arrange(child_bounds);
        }
    }

    // -------------------------------------------------------------------------------------------------------
    template<RectLike TRect, SizeLike TSize>
    void grid_layout <TRect, TSize>::auto_assign_cells(elt_t* parent) {
        // Calculate which cells are already occupied by explicitly positioned cells
        auto occupied = calculate_occupied_cells();

        int current_row = 0;
        int current_col = 0;

        for (auto& child : parent->children()) {
            if (!child->is_visible()) continue;

            // Skip if already assigned
            if (m_cell_mapping.contains(child.get())) {
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
    template<RectLike TRect, SizeLike TSize>
    int grid_layout <TRect, TSize>::calculate_row_count(elt_t* parent) {
        if (m_num_rows > 0) return m_num_rows;

        int max_row = 0;
        for (auto& child : parent->children()) {
            if (!child->is_visible()) continue;

            auto it = m_cell_mapping.find(child.get());
            if (it != m_cell_mapping.end()) {
                int end_row = it->second.row + it->second.row_span;
                max_row = std::max(max_row, end_row);
            }
        }

        return std::max(1, max_row);
    }

    // -------------------------------------------------------------------------------------------------------
    template<RectLike TRect, SizeLike TSize>
    void grid_layout <TRect, TSize>::measure_auto_sized_grid(elt_t* parent, int available_width, int available_height,
                                                             int actual_rows) {
        m_column_widths.resize(m_num_columns, 0);
        m_row_heights.resize(actual_rows, 0);

        // First pass: Measure non-spanning cells
        for (auto& child : parent->children()) {
            if (!child->is_visible()) continue;

            auto it = m_cell_mapping.find(child.get());
            if (it == m_cell_mapping.end()) continue;

            const grid_cell_info& cell = it->second;

            // Only process single-span cells in first pass
            if (cell.column_span == 1 || cell.row_span == 1) {
                // Measure child with unconstrained size
                TSize measured = child->measure(available_width, available_height);
                int meas_w = size_utils::get_width(measured);
                int meas_h = size_utils::get_height(measured);

                // Update column sizes for single-column spans
                if (cell.column_span == 1) {
                    m_column_widths[cell.column] =
                        std::max(m_column_widths[cell.column], meas_w);
                }

                // Update row sizes for single-row spans
                if (cell.row_span == 1) {
                    m_row_heights[cell.row] =
                        std::max(m_row_heights[cell.row], meas_h);
                }
            }
        }

        // Second pass: Handle spanning cells
        distribute_spanning_sizes(parent);
    }

    // -------------------------------------------------------------------------------------------------------
    template<RectLike TRect, SizeLike TSize>
    void grid_layout <TRect, TSize>::use_fixed_grid_sizes(int actual_rows) {
        if (m_column_widths.empty()) {
            m_column_widths.resize(m_num_columns, 100); // Default width
        }

        if (m_row_heights.empty()) {
            m_row_heights.resize(actual_rows, 100); // Default height
        }
    }

    // -------------------------------------------------------------------------------------------------------
    template<RectLike TRect, SizeLike TSize>
    void grid_layout <TRect, TSize>::distribute_spanning_sizes(elt_t* parent) {
        // Process spanning cells (cells that span multiple columns or rows)
        // We need to ensure the spanned columns/rows together are at least as wide/tall
        // as the spanning cell requires

        // First handle column spanning
        for (auto& child : parent->children()) {
            if (!child->is_visible()) continue;

            auto it = m_cell_mapping.find(child.get());
            if (it == m_cell_mapping.end()) continue;

            const grid_cell_info& cell = it->second;

            // Handle cells that span multiple columns
            if (cell.column_span > 1) {
                // Get the child's measured size
                TSize measured = child->last_measured_size();
                int required_width = size_utils::get_width(measured);

                // Calculate current total width of spanned columns
                int current_total = 0;
                for (int i = 0; i < cell.column_span && (cell.column + i) < m_num_columns; ++i) {
                    current_total += m_column_widths[cell.column + i];
                    if (i > 0) {
                        current_total += m_column_spacing;  // Include spacing between columns
                    }
                }

                // If the spanning cell needs more space, distribute it evenly
                if (required_width > current_total) {
                    int extra_needed = required_width - current_total;
                    int columns_to_expand = std::min(cell.column_span,
                                                    m_num_columns - cell.column);

                    if (columns_to_expand > 0) {
                        int extra_per_column = extra_needed / columns_to_expand;
                        int remainder = extra_needed % columns_to_expand;

                        for (int i = 0; i < columns_to_expand; ++i) {
                            m_column_widths[cell.column + i] += extra_per_column;
                            // Distribute remainder to first columns
                            if (i < remainder) {
                                m_column_widths[cell.column + i] += 1;
                            }
                        }
                    }
                }
            }
        }

        // Then handle row spanning (same logic)
        for (auto& child : parent->children()) {
            if (!child->is_visible()) continue;

            auto it = m_cell_mapping.find(child.get());
            if (it == m_cell_mapping.end()) continue;

            const grid_cell_info& cell = it->second;

            // Handle cells that span multiple rows
            if (cell.row_span > 1) {
                // Get the child's measured size
                TSize measured = child->last_measured_size();
                int required_height = size_utils::get_height(measured);

                // Calculate current total height of spanned rows
                int current_total = 0;
                int actual_rows = m_row_heights.size();
                for (int i = 0; i < cell.row_span && (cell.row + i) < actual_rows; ++i) {
                    current_total += m_row_heights[cell.row + i];
                    if (i > 0) {
                        current_total += m_row_spacing;  // Include spacing between rows
                    }
                }

                // If the spanning cell needs more space, distribute it evenly
                if (required_height > current_total) {
                    int extra_needed = required_height - current_total;
                    int rows_to_expand = std::min(cell.row_span,
                                                  actual_rows - cell.row);

                    if (rows_to_expand > 0) {
                        int extra_per_row = extra_needed / rows_to_expand;
                        int remainder = extra_needed % rows_to_expand;

                        for (int i = 0; i < rows_to_expand; ++i) {
                            m_row_heights[cell.row + i] += extra_per_row;
                            // Distribute remainder to first rows
                            if (i < remainder) {
                                m_row_heights[cell.row + i] += 1;
                            }
                        }
                    }
                }
            }
        }
    }

    // -------------------------------------------------------------------------------------------------------
    template<RectLike TRect, SizeLike TSize>
    std::set<std::pair<int, int>> grid_layout <TRect, TSize>::calculate_occupied_cells() const {
        std::set<std::pair<int, int>> occupied;

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
    template<RectLike TRect, SizeLike TSize>
    std::pair<int, int> grid_layout <TRect, TSize>::find_next_free_cell(
        const std::set<std::pair<int, int>>& occupied,
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
