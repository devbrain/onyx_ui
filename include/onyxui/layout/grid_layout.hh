//
// Created by igor on 09/10/2025.
//

#pragma once

#include <vector>
#include <unordered_map>
#include <onyxui/layout_strategy.hh>

namespace onyxui {
    struct grid_cell_info {
        int row = 0;
        int column = 0;
        int row_span = 1;
        int column_span = 1;
    };

    template<RectLike TRect, SizeLike TSize>
    class grid_layout : layout_strategy <TRect, TSize> {
        using elt_t = ui_element <TRect, TSize>;

        int num_columns = 1;
        int num_rows = -1; // Auto-calculate if -1

        std::vector <int> column_widths; // Empty = equal distribution
        std::vector <int> row_heights; // Empty = equal distribution

        int column_spacing = 0;
        int row_spacing = 0;

        bool auto_size_cells = true;

        // Child -> grid cell mapping
        std::unordered_map <elt_t*, grid_cell_info> cell_mapping;

        void set_cell(elt_t* child, int row, int col,
                      int row_span = 1, int col_span = 1);

        TSize measure_children(elt_t* parent,
                               int available_width,
                               int available_height) override;

        void arrange_children(elt_t* parent,
                              const TRect& content_area) override;

        void auto_assign_cells(elt_t* parent);

        int calculate_row_count(elt_t* parent);

        void measure_auto_sized_grid(elt_t* parent,
                                     int available_width,
                                     int available_height,
                                     int actual_rows);

        void use_fixed_grid_sizes(int actual_rows);
    };

    // ==================================================================================================
    // Implementation
    // ==================================================================================================
    template<RectLike TRect, SizeLike TSize>
    void grid_layout <TRect, TSize>::set_cell(elt_t* child, int row, int col, int row_span, int col_span) {
        cell_mapping[child] = {row, col, row_span, col_span};
    }

    // -------------------------------------------------------------------------------------------------------
    template<RectLike TRect, SizeLike TSize>
    TSize grid_layout <TRect, TSize>::measure_children(elt_t* parent, int available_width, int available_height) {
        if (parent->children.empty()) {
            TSize result = {};
            size_utils::set_size(result, 0, 0);
            return result;
        }

        // Auto-assign cells if not explicitly set
        auto_assign_cells(parent);

        // Calculate actual row count
        int actual_rows = calculate_row_count(parent);

        if (auto_size_cells) {
            // Measure all children to determine cell sizes
            measure_auto_sized_grid(parent, available_width, available_height,
                                    actual_rows);
        } else {
            // Use predefined sizes
            use_fixed_grid_sizes(actual_rows);
        }

        // Calculate total grid size
        int total_width = 0;
        for (int w : column_widths) total_width += w;
        total_width += column_spacing * (num_columns - 1);

        int total_height = 0;
        for (int h : row_heights) total_height += h;
        total_height += row_spacing * (actual_rows - 1);

        TSize result = {};
        size_utils::set_size(result, total_width, total_height);
        return result;
    }

    // -------------------------------------------------------------------------------------------------------
    template<RectLike TRect, SizeLike TSize>
    void grid_layout <TRect, TSize>::arrange_children(elt_t* parent, const TRect& content_area) {
        if (parent->children.empty()) return;

        int actual_rows = row_heights.size();

        // Calculate cell positions
        std::vector <int> column_positions(num_columns);
        std::vector <int> row_positions(actual_rows);

        int content_x = rect_utils::get_x(content_area);
        int content_y = rect_utils::get_y(content_area);

        column_positions[0] = content_x;
        for (int i = 1; i < num_columns; ++i) {
            column_positions[i] = column_positions[i - 1] +
                                  column_widths[i - 1] + column_spacing;
        }

        row_positions[0] = content_y;
        for (int i = 1; i < actual_rows; ++i) {
            row_positions[i] = row_positions[i - 1] +
                               row_heights[i - 1] + row_spacing;
        }

        // Arrange each child
        for (auto& child : parent->children) {
            if (!child->visible) continue;

            auto it = cell_mapping.find(child.get());
            if (it == cell_mapping.end()) continue;

            const grid_cell_info& cell = it->second;

            // Calculate cell bounds
            int cell_x = column_positions[cell.column];
            int cell_y = row_positions[cell.row];

            int cell_width = 0;
            for (int i = 0; i < cell.column_span; ++i) {
                if (cell.column + i < num_columns) {
                    cell_width += column_widths[cell.column + i];
                    if (i < cell.column_span - 1) {
                        cell_width += column_spacing;
                    }
                }
            }

            int cell_height = 0;
            for (int i = 0; i < cell.row_span; ++i) {
                if (cell.row + i < actual_rows) {
                    cell_height += row_heights[cell.row + i];
                    if (i < cell.row_span - 1) {
                        cell_height += row_spacing;
                    }
                }
            }

            // Apply alignment within cell
            TSize measured = child->last_measured_size;
            int meas_w = size_utils::get_width(measured);
            int meas_h = size_utils::get_height(measured);

            int child_width = meas_w;
            int child_height = meas_h;
            int child_x = cell_x;
            int child_y = cell_y;

            if (child->h_align == horizontal_alignment::stretch) {
                child_width = cell_width;
            } else {
                child_width = std::min(meas_w, cell_width);
                if (child->h_align == horizontal_alignment::center) {
                    child_x = cell_x + (cell_width - child_width) / 2;
                } else if (child->h_align == horizontal_alignment::right) {
                    child_x = cell_x + cell_width - child_width;
                }
            }

            if (child->v_align == vertical_alignment::stretch) {
                child_height = cell_height;
            } else {
                child_height = std::min(meas_h, cell_height);
                if (child->v_align == vertical_alignment::center) {
                    child_y = cell_y + (cell_height - child_height) / 2;
                } else if (child->v_align == vertical_alignment::bottom) {
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
        int current_row = 0;
        int current_col = 0;

        for (auto& child : parent->children) {
            if (!child->visible) continue;

            // Skip if already assigned
            if (cell_mapping.find(child.get()) != cell_mapping.end()) {
                continue;
            }

            // Auto-assign to next available cell
            cell_mapping[child.get()] = {current_row, current_col, 1, 1};

            current_col++;
            if (current_col >= num_columns) {
                current_col = 0;
                current_row++;
            }
        }
    }

    // -------------------------------------------------------------------------------------------------------
    template<RectLike TRect, SizeLike TSize>
    int grid_layout <TRect, TSize>::calculate_row_count(elt_t* parent) {
        if (num_rows > 0) return num_rows;

        int max_row = 0;
        for (auto& child : parent->children) {
            if (!child->visible) continue;

            auto it = cell_mapping.find(child.get());
            if (it != cell_mapping.end()) {
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
        column_widths.resize(num_columns, 0);
        row_heights.resize(actual_rows, 0);

        // Measure all children
        for (auto& child : parent->children) {
            if (!child->visible) continue;

            auto it = cell_mapping.find(child.get());
            if (it == cell_mapping.end()) continue;

            const grid_cell_info& cell = it->second;

            // Measure child with unconstrained size
            TSize measured = child->measure(available_width, available_height);
            int meas_w = size_utils::get_width(measured);
            int meas_h = size_utils::get_height(measured);

            // Update column/row sizes (simple distribution for spans)
            if (cell.column_span == 1) {
                column_widths[cell.column] =
                    std::max(column_widths[cell.column], meas_w);
            }

            if (cell.row_span == 1) {
                row_heights[cell.row] =
                    std::max(row_heights[cell.row], meas_h);
            }
        }
    }

    // -------------------------------------------------------------------------------------------------------
    template<RectLike TRect, SizeLike TSize>
    void grid_layout <TRect, TSize>::use_fixed_grid_sizes(int actual_rows) {
        if (column_widths.empty()) {
            column_widths.resize(num_columns, 100); // Default width
        }

        if (row_heights.empty()) {
            row_heights.resize(actual_rows, 100); // Default height
        }
    }
}
