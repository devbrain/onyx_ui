//
// OnyxUI MVC System - Table View
// Created: 2026-01-05
//

#pragma once

#include <vector>
#include <cmath>
#include <algorithm>

#include <onyxui/concepts/backend.hh>
#include <onyxui/core/rendering/render_context.hh>
#include <onyxui/mvc/views/abstract_item_view.hh>
#include <onyxui/services/ui_services.hh>

namespace onyxui {

/**
 * @brief Selection behavior for table_view
 *
 * @details
 * Determines what gets selected when the user clicks:
 * - select_items: Individual cells (spreadsheet-like)
 * - select_rows: Entire rows (file manager, table views - default)
 * - select_columns: Entire columns (spreadsheet column operations)
 */
enum class selection_behavior : std::uint8_t {
    select_items,   ///< Individual cells
    select_rows,    ///< Entire rows (default)
    select_columns  ///< Entire columns
};

/**
 * @brief A table view for displaying 2D tabular data
 *
 * @tparam Backend The UI backend type
 *
 * @details
 * table_view displays data from a table_model in rows and columns.
 * It provides:
 *
 * - **Column Management**: Fixed, auto, or stretch column widths
 * - **Headers**: Optional column headers with click-to-sort
 * - **Grid Lines**: Optional horizontal/vertical grid lines
 * - **Selection**: Row, cell, or column selection modes
 * - **Virtual Scrolling**: Only visible cells are rendered
 * - **Keyboard Navigation**: Arrow keys (including left/right for cells)
 *
 * @par Example Usage:
 * @code
 * // Create model
 * auto model = std::make_shared<person_table_model<Backend>>();
 * model->set_rows(people);
 *
 * // Create view
 * auto view = std::make_unique<table_view<Backend>>();
 * view->set_model(model.get());
 *
 * // Configure columns
 * view->set_column_width(0, 100);  // Name: fixed 100
 * view->set_column_width(1, 50);   // Age: fixed 50
 * view->set_column_stretch(2, 1);  // Email: stretch to fill
 *
 * // Enable headers and sorting
 * view->set_headers_visible(true);
 * view->set_sorting_enabled(true);
 *
 * // Add to UI
 * container->add_child(std::move(view));
 * @endcode
 */
template<UIBackend Backend>
class table_view : public abstract_item_view<Backend> {
public:
    using base = abstract_item_view<Backend>;
    using rect_type = typename Backend::rect_type;
    using size_type = typename Backend::size_type;

    // ===================================================================
    // Constants
    // ===================================================================

    /// Border width in logical units
    static constexpr double BORDER_WIDTH = 1.0;

    /// Default header height in logical units
    static constexpr int DEFAULT_HEADER_HEIGHT = 24;

    /// Default minimum column width
    static constexpr int DEFAULT_MIN_COLUMN_WIDTH = 20;

    /// Default auto column width (used when measuring headers)
    static constexpr int DEFAULT_AUTO_COLUMN_WIDTH = 50;

    /// Padding inside header cells
    static constexpr int HEADER_PADDING = 4;

    /// Width reserved for sort indicator
    static constexpr int SORT_INDICATOR_WIDTH = 12;

    /**
     * @brief Column configuration
     */
    struct column_info {
        int width = 0;                              ///< Width in logical units (0 = auto)
        int min_width = DEFAULT_MIN_COLUMN_WIDTH;   ///< Minimum width
        int stretch_factor = 0;                     ///< Stretch factor (0 = don't stretch)

        int calculated_width = 0;  ///< Calculated width after layout
    };

    /**
     * @brief Construct empty table view
     */
    table_view() = default;

    // ===================================================================
    // Model/Delegate Overrides (reset row height)
    // ===================================================================

    /**
     * @brief Set the data model
     * @param model Pointer to model (view does not take ownership)
     */
    void set_model(typename base::model_type* model) {
        m_row_height = 0;  // Reset to recalculate from delegate
        base::set_model(model);
    }

    /**
     * @brief Set item delegate for rendering
     * @param delegate Shared pointer to delegate
     */
    void set_delegate(std::shared_ptr<typename base::delegate_type> delegate) {
        m_row_height = 0;  // Reset to recalculate from delegate
        base::set_delegate(std::move(delegate));
    }

    // ===================================================================
    // Column Configuration
    // ===================================================================

    /**
     * @brief Set column width (fixed width)
     * @param column Column index (0-based)
     * @param width Width in logical units (0 = auto)
     */
    void set_column_width(int column, int width) {
        ensure_column_exists(column);
        m_columns[static_cast<std::size_t>(column)].width = width;
        update_geometries();
        this->invalidate_measure();
    }

    /**
     * @brief Get column width
     * @param column Column index
     * @return Calculated column width (after layout)
     */
    [[nodiscard]] int column_width(int column) const {
        if (column < 0 || column >= static_cast<int>(m_columns.size())) {
            return 0;
        }
        return m_columns[static_cast<std::size_t>(column)].calculated_width;
    }

    /**
     * @brief Set minimum column width
     * @param column Column index
     * @param min_width Minimum width in logical units
     */
    void set_minimum_column_width(int column, int min_width) {
        ensure_column_exists(column);
        m_columns[static_cast<std::size_t>(column)].min_width = min_width;
        update_geometries();
        this->invalidate_measure();
    }

    /**
     * @brief Set column stretch factor
     * @param column Column index
     * @param stretch_factor Stretch factor (0 = don't stretch)
     */
    void set_column_stretch(int column, int stretch_factor) {
        ensure_column_exists(column);
        m_columns[static_cast<std::size_t>(column)].stretch_factor = stretch_factor;
        update_geometries();
        this->invalidate_measure();
    }

    /**
     * @brief Get total width of all columns
     * @return Sum of calculated column widths
     */
    [[nodiscard]] int total_column_width() const noexcept {
        int total = 0;
        for (const auto& col : m_columns) {
            total += col.calculated_width;
        }
        return total;
    }

    // ===================================================================
    // Headers
    // ===================================================================

    /**
     * @brief Show/hide column headers
     * @param visible true to show headers
     */
    void set_headers_visible(bool visible) {
        if (m_headers_visible == visible) return;
        m_headers_visible = visible;
        update_geometries();
        this->invalidate_measure();
    }

    /**
     * @brief Check if headers are visible
     */
    [[nodiscard]] bool headers_visible() const noexcept {
        return m_headers_visible;
    }

    /**
     * @brief Set header row height
     * @param height Height in logical units
     */
    void set_header_height(int height) {
        m_header_height = height;
        update_geometries();
        this->invalidate_measure();
    }

    /**
     * @brief Get header row height
     */
    [[nodiscard]] int header_height() const noexcept {
        return m_header_height;
    }

    // ===================================================================
    // Grid Lines
    // ===================================================================

    /**
     * @brief Show/hide horizontal grid lines between rows
     */
    void set_horizontal_grid_visible(bool visible) {
        m_horizontal_grid_visible = visible;
        this->mark_dirty();
    }

    [[nodiscard]] bool horizontal_grid_visible() const noexcept {
        return m_horizontal_grid_visible;
    }

    /**
     * @brief Show/hide vertical grid lines between columns
     */
    void set_vertical_grid_visible(bool visible) {
        m_vertical_grid_visible = visible;
        this->mark_dirty();
    }

    [[nodiscard]] bool vertical_grid_visible() const noexcept {
        return m_vertical_grid_visible;
    }

    // ===================================================================
    // Selection Behavior
    // ===================================================================

    /**
     * @brief Set selection behavior (row/cell/column)
     */
    void set_selection_behavior(selection_behavior behavior) {
        m_selection_behavior = behavior;
    }

    [[nodiscard]] selection_behavior get_selection_behavior() const noexcept {
        return m_selection_behavior;
    }

    // ===================================================================
    // Sorting
    // ===================================================================

    /**
     * @brief Enable/disable clickable headers for sorting
     */
    void set_sorting_enabled(bool enabled) {
        m_sorting_enabled = enabled;
    }

    [[nodiscard]] bool sorting_enabled() const noexcept {
        return m_sorting_enabled;
    }

    /**
     * @brief Get current sort column (-1 = none)
     */
    [[nodiscard]] int sort_column() const noexcept {
        return m_sort_column;
    }

    /**
     * @brief Get current sort order
     */
    [[nodiscard]] sort_order get_sort_order() const noexcept {
        return m_sort_order;
    }

    // ===================================================================
    // Scrolling
    // ===================================================================

    /**
     * @brief Get horizontal scroll offset
     */
    [[nodiscard]] double scroll_offset_x() const noexcept {
        return m_scroll_offset_x;
    }

    /**
     * @brief Get vertical scroll offset
     */
    [[nodiscard]] double scroll_offset_y() const noexcept {
        return m_scroll_offset_y;
    }

    /**
     * @brief Set scroll offset
     */
    void set_scroll_offset(double x, double y) {
        double const max_x = max_scroll_offset_x();
        double const max_y = max_scroll_offset_y();

        x = std::max(0.0, std::min(x, max_x));
        y = std::max(0.0, std::min(y, max_y));

        if (std::abs(x - m_scroll_offset_x) > 0.001 ||
            std::abs(y - m_scroll_offset_y) > 0.001) {
            m_scroll_offset_x = x;
            m_scroll_offset_y = y;
            this->mark_dirty();
            scroll_offset_changed.emit(m_scroll_offset_x, m_scroll_offset_y);
        }
    }

    /**
     * @brief Scroll to make a column visible
     */
    void scroll_to_column(int column) {
        if (column < 0 || column >= static_cast<int>(m_columns.size())) {
            return;
        }

        // Calculate column start position
        int col_x = 0;
        for (int c = 0; c < column; ++c) {
            col_x += m_columns[static_cast<std::size_t>(c)].calculated_width;
        }

        int const col_width = m_columns[static_cast<std::size_t>(column)].calculated_width;
        int const col_end = col_x + col_width;

        double const view_width = this->bounds().width.value - (BORDER_WIDTH * 2);

        if (static_cast<double>(col_x) < m_scroll_offset_x) {
            set_scroll_offset(static_cast<double>(col_x), m_scroll_offset_y);
        } else if (static_cast<double>(col_end) > m_scroll_offset_x + view_width) {
            set_scroll_offset(static_cast<double>(col_end) - view_width, m_scroll_offset_y);
        }
    }

    /**
     * @brief Maximum horizontal scroll offset
     */
    [[nodiscard]] double max_scroll_offset_x() const noexcept {
        double const view_width = this->bounds().width.value - (BORDER_WIDTH * 2);
        double const content_width = static_cast<double>(total_column_width());
        return std::max(0.0, content_width - view_width);
    }

    /**
     * @brief Maximum vertical scroll offset
     */
    [[nodiscard]] double max_scroll_offset_y() const noexcept {
        double const view_height = this->bounds().height.value - (BORDER_WIDTH * 2);

        // Account for header height
        double effective_height = view_height;
        if (m_headers_visible) {
            effective_height -= static_cast<double>(m_header_height);
        }

        double const content_height = static_cast<double>(m_content_height);
        return std::max(0.0, content_height - effective_height);
    }

    /**
     * @brief Signal emitted when scroll offset changes
     */
    signal<double, double> scroll_offset_changed;

    /**
     * @brief Signal emitted when header is clicked
     */
    signal<int> header_clicked;

    /**
     * @brief Signal emitted when column is resized
     */
    signal<int, int> column_resized;  // column, new_width

    // ===================================================================
    // Pure Virtual Methods from abstract_item_view
    // ===================================================================

    /**
     * @brief Get index at screen position
     */
    [[nodiscard]] model_index index_at(logical_unit x, logical_unit y) const override {
        if (!this->m_model || m_cell_rects.empty()) {
            return {};
        }

        // Convert widget-relative coordinates to content-relative
        double const px = x.value - BORDER_WIDTH + m_scroll_offset_x;
        double py = y.value - BORDER_WIDTH + m_scroll_offset_y;

        // Account for header
        if (m_headers_visible) {
            double const header_bottom = static_cast<double>(m_header_height);
            if (y.value - BORDER_WIDTH < header_bottom) {
                // Click is in header area - return invalid (header clicks handled separately)
                return {};
            }
            py -= header_bottom;  // Adjust for header
        }

        // Find row
        int row = -1;
        int row_count = this->m_model->row_count();
        for (int r = 0; r < row_count; ++r) {
            double const row_top = static_cast<double>(r * m_row_height);
            double const row_bottom = row_top + static_cast<double>(m_row_height);
            if (py >= row_top && py < row_bottom) {
                row = r;
                break;
            }
        }
        if (row < 0) return {};

        // Find column
        int col = -1;
        double col_x = 0.0;
        for (int c = 0; c < static_cast<int>(m_columns.size()); ++c) {
            double const col_width = static_cast<double>(m_columns[static_cast<std::size_t>(c)].calculated_width);
            if (px >= col_x && px < col_x + col_width) {
                col = c;
                break;
            }
            col_x += col_width;
        }
        if (col < 0) return {};

        return this->m_model->index(row, col);
    }

    /**
     * @brief Get visual rectangle for a cell
     */
    [[nodiscard]] rect_type visual_rect(const model_index& index) const override {
        if (!index.is_valid() ||
            index.row < 0 || index.row >= this->m_model->row_count() ||
            index.column < 0 || index.column >= static_cast<int>(m_columns.size())) {
            return rect_type{};
        }

        // Calculate cell position in content coordinates
        double col_x = 0.0;
        for (int c = 0; c < index.column; ++c) {
            col_x += static_cast<double>(m_columns[static_cast<std::size_t>(c)].calculated_width);
        }

        double const cell_y = static_cast<double>(index.row * m_row_height);
        double const cell_w = static_cast<double>(m_columns[static_cast<std::size_t>(index.column)].calculated_width);
        double const cell_h = static_cast<double>(m_row_height);

        // Adjust for scroll offset and header
        double const scrolled_x = col_x - m_scroll_offset_x;
        double scrolled_y = cell_y - m_scroll_offset_y;
        if (m_headers_visible) {
            scrolled_y += static_cast<double>(m_header_height);
        }

        // Convert to physical
        auto const* metrics = ui_services<Backend>::metrics();
        if (metrics) {
            return rect_type{
                metrics->snap_to_physical_x(logical_unit(scrolled_x), snap_mode::floor),
                metrics->snap_to_physical_y(logical_unit(scrolled_y), snap_mode::floor),
                metrics->snap_to_physical_x(logical_unit(cell_w), snap_mode::round),
                metrics->snap_to_physical_y(logical_unit(cell_h), snap_mode::round)
            };
        }

        return rect_type{
            static_cast<int>(scrolled_x),
            static_cast<int>(scrolled_y),
            static_cast<int>(cell_w),
            static_cast<int>(cell_h)
        };
    }

    /**
     * @brief Scroll to make cell visible
     */
    void scroll_to(const model_index& index) override {
        if (!index.is_valid()) return;

        // Ensure geometries are up to date before scrolling
        if (m_columns.empty() || index.column >= static_cast<int>(m_columns.size())) {
            // Force geometry update if columns not yet calculated
            update_geometries();
            // Still empty? Can't scroll to unknown column
            if (m_columns.empty() || index.column >= static_cast<int>(m_columns.size())) {
                return;
            }
        }

        double const view_height = this->bounds().height.value;
        double const view_width = this->bounds().width.value;

        if (view_height <= 0 || view_width <= 0) return;

        double const effective_height = view_height - (BORDER_WIDTH * 2) -
            (m_headers_visible ? static_cast<double>(m_header_height) : 0.0);
        double const effective_width = view_width - (BORDER_WIDTH * 2);

        // Vertical scrolling (only if row height is known)
        double new_scroll_y = m_scroll_offset_y;
        if (m_row_height > 0) {
            double const row_top = static_cast<double>(index.row * m_row_height);
            double const row_bottom = row_top + static_cast<double>(m_row_height);

            if (row_top < m_scroll_offset_y) {
                new_scroll_y = row_top;
            } else if (row_bottom > m_scroll_offset_y + effective_height) {
                new_scroll_y = row_bottom - effective_height;
            }
        }

        // Horizontal scrolling
        double col_x = 0.0;
        for (int c = 0; c < index.column; ++c) {
            col_x += static_cast<double>(m_columns[static_cast<std::size_t>(c)].calculated_width);
        }
        double const col_right = col_x + static_cast<double>(m_columns[static_cast<std::size_t>(index.column)].calculated_width);

        double new_scroll_x = m_scroll_offset_x;
        if (col_x < m_scroll_offset_x) {
            new_scroll_x = col_x;
        } else if (col_right > m_scroll_offset_x + effective_width) {
            new_scroll_x = col_right - effective_width;
        }

        set_scroll_offset(new_scroll_x, new_scroll_y);
    }

    /**
     * @brief Update internal geometries after model changes
     */
    void update_geometries() override {
        m_cell_rects.clear();

        if (!this->m_model || !this->m_delegate) {
            m_content_width = 0;
            m_content_height = 0;
            return;
        }

        int const row_count = this->m_model->row_count();
        int const col_count = this->m_model->column_count();

        if (row_count == 0 || col_count == 0) {
            m_content_width = 0;
            m_content_height = 0;
            return;
        }

        // Ensure column info exists
        ensure_columns_match_model();

        // Calculate column widths
        calculate_column_widths();

        // Calculate row height (uniform for all rows)
        auto const* metrics = ui_services<Backend>::metrics();
        if (m_row_height <= 0) {
            // Get row height from delegate
            auto idx = this->m_model->index(0, 0);
            auto size_hint = this->m_delegate->size_hint(idx);
            m_row_height = metrics
                ? static_cast<int>(metrics->physical_to_logical_y(size_hint.h).value)
                : size_hint.h;
            m_row_height = std::max(m_row_height, 1);
        }

        // Calculate cell rectangles
        m_cell_rects.resize(static_cast<std::size_t>(row_count));
        for (int row = 0; row < row_count; ++row) {
            auto& row_rects = m_cell_rects[static_cast<std::size_t>(row)];
            row_rects.resize(static_cast<std::size_t>(col_count));

            double col_x = 0.0;
            for (int col = 0; col < col_count; ++col) {
                double const col_w = static_cast<double>(m_columns[static_cast<std::size_t>(col)].calculated_width);
                row_rects[static_cast<std::size_t>(col)] = logical_rect{
                    logical_unit(col_x),
                    logical_unit(static_cast<double>(row * m_row_height)),
                    logical_unit(col_w),
                    logical_unit(static_cast<double>(m_row_height))
                };
                col_x += col_w;
            }
        }

        m_content_width = total_column_width();
        m_content_height = row_count * m_row_height;
    }

    // ===================================================================
    // Rendering
    // ===================================================================

    void do_render(render_context<Backend>& ctx) const override {
        auto const& pos = ctx.position();
        int const base_x = static_cast<int>(pos.x);
        int const base_y = static_cast<int>(pos.y);

        auto current_bounds = this->bounds();
        auto const [physical_width, physical_height] = ctx.get_final_dims(
            current_bounds.width.to_int(), current_bounds.height.to_int());

        auto* themes = ui_services<Backend>::themes();
        auto const* theme = themes ? themes->get_current_theme() : nullptr;
        auto const* metrics = ui_services<Backend>::metrics();

        int const physical_border_x = metrics
            ? metrics->snap_to_physical_x(logical_unit(BORDER_WIDTH), snap_mode::round)
            : 1;
        int const physical_border_y = metrics
            ? metrics->snap_to_physical_y(logical_unit(BORDER_WIDTH), snap_mode::round)
            : 1;

        // Draw background and border
        if (theme) {
            rect_type bg_rect{base_x, base_y, physical_width, physical_height};
            ctx.fill_rect(bg_rect, theme->list.item_background);
            ctx.draw_rect(bg_rect, theme->list.focus_box_style);
        }

        if (!this->m_model || !this->m_delegate || m_cell_rects.empty()) {
            return;
        }

        int const content_x = base_x + physical_border_x;
        int content_y = base_y + physical_border_y;
        int const content_width = physical_width - (physical_border_x * 2);
        int content_height = physical_height - (physical_border_y * 2);

        // Render headers
        if (m_headers_visible) {
            render_headers(ctx, content_x, content_y, content_width, theme, metrics);
            int const physical_header_height = metrics
                ? metrics->snap_to_physical_y(logical_unit(static_cast<double>(m_header_height)), snap_mode::round)
                : m_header_height;
            content_y += physical_header_height;
            content_height -= physical_header_height;
        }

        // Virtual scrolling: calculate visible region in logical units
        // content_height/width are in physical pixels, convert to logical
        double const visible_height = metrics
            ? metrics->physical_to_logical_y(content_height).value
            : static_cast<double>(content_height);
        double const visible_width = metrics
            ? metrics->physical_to_logical_x(content_width).value
            : static_cast<double>(content_width);

        // Find visible rows
        int const first_visible_row = static_cast<int>(std::floor(m_scroll_offset_y / static_cast<double>(m_row_height)));
        int const last_visible_row = static_cast<int>(std::ceil((m_scroll_offset_y + visible_height) / static_cast<double>(m_row_height)));

        int const row_count = this->m_model->row_count();
        int const col_count = this->m_model->column_count();

        // Render visible cells
        for (int row = first_visible_row; row <= last_visible_row && row < row_count; ++row) {
            if (row < 0) continue;

            double col_x = 0.0;
            for (int col = 0; col < col_count; ++col) {
                double const col_w = static_cast<double>(m_columns[static_cast<std::size_t>(col)].calculated_width);

                // Skip if not visible horizontally
                if (col_x + col_w <= m_scroll_offset_x || col_x >= m_scroll_offset_x + visible_width) {
                    col_x += col_w;
                    continue;
                }

                model_index idx = this->m_model->index(row, col);

                bool is_selected = is_cell_selected(idx);
                bool has_focus = this->m_selection_model && this->m_selection_model->current_index() == idx;

                // Calculate cell position
                double const cell_y = static_cast<double>(row * m_row_height) - m_scroll_offset_y;
                double const cell_x = col_x - m_scroll_offset_x;

                int const abs_x = content_x + (metrics
                    ? metrics->snap_to_physical_x(logical_unit(cell_x), snap_mode::floor)
                    : static_cast<int>(cell_x));
                int const abs_y = content_y + (metrics
                    ? metrics->snap_to_physical_y(logical_unit(cell_y), snap_mode::floor)
                    : static_cast<int>(cell_y));
                int const abs_w = metrics
                    ? metrics->snap_to_physical_x(logical_unit(col_w), snap_mode::round)
                    : static_cast<int>(col_w);
                int const abs_h = metrics
                    ? metrics->snap_to_physical_y(logical_unit(static_cast<double>(m_row_height)), snap_mode::round)
                    : m_row_height;

                rect_type cell_rect{abs_x, abs_y, abs_w, abs_h};
                this->m_delegate->paint(ctx, idx, cell_rect, is_selected, has_focus);

                col_x += col_w;
            }

            // Draw horizontal grid line
            if (m_horizontal_grid_visible && theme) {
                double const line_y = static_cast<double>((row + 1) * m_row_height) - m_scroll_offset_y;
                if (line_y > 0 && line_y < visible_height) {
                    int const abs_line_y = content_y + (metrics
                        ? metrics->snap_to_physical_y(logical_unit(line_y), snap_mode::floor)
                        : static_cast<int>(line_y));
                    rect_type line_rect{content_x, abs_line_y, content_width, 1};
                    ctx.fill_rect(line_rect, theme->table.grid_line_color);
                }
            }
        }

        // Draw vertical grid lines
        if (m_vertical_grid_visible && theme) {
            double col_x = 0.0;
            for (int col = 0; col < col_count; ++col) {
                col_x += static_cast<double>(m_columns[static_cast<std::size_t>(col)].calculated_width);
                double const line_x = col_x - m_scroll_offset_x;
                if (line_x > 0 && line_x < visible_width) {
                    int const abs_line_x = content_x + (metrics
                        ? metrics->snap_to_physical_x(logical_unit(line_x), snap_mode::floor)
                        : static_cast<int>(line_x));
                    rect_type line_rect{abs_line_x, content_y, 1, content_height};
                    ctx.fill_rect(line_rect, theme->table.grid_line_color);
                }
            }
        }
    }

protected:
    // ===================================================================
    // Layout
    // ===================================================================

    [[nodiscard]] logical_size get_content_size() const override {
        const_cast<table_view*>(this)->update_geometries();

        int total_height = m_content_height;
        if (m_headers_visible) {
            total_height += m_header_height;
        }

        return {
            logical_unit(static_cast<double>(m_content_width)),
            logical_unit(static_cast<double>(total_height))
        };
    }

    void do_arrange(const logical_rect& final_bounds) override {
        bool const bounds_changed = (final_bounds != m_last_arranged_bounds);
        m_last_arranged_bounds = final_bounds;

        base::do_arrange(final_bounds);
        update_geometries();

        if (bounds_changed && this->m_selection_model) {
            auto current = this->m_selection_model->current_index();
            if (current.is_valid()) {
                scroll_to(current);
            }
        }
    }

public:
    // ===================================================================
    // Event Handling (public - part of event dispatch interface)
    // ===================================================================

    bool handle_event(const ui_event& evt, event_phase phase) override {
        if (phase != event_phase::target) {
            return base::handle_event(evt, phase);
        }

        // Handle mouse events for header clicks
        if (auto* mouse_evt = std::get_if<mouse_event>(&evt)) {
            if (mouse_evt->act == mouse_event::action::press &&
                mouse_evt->btn == mouse_event::button::left) {

                // Check for header click
                if (m_headers_visible && m_sorting_enabled) {
                    auto abs_bounds = this->get_absolute_logical_bounds();
                    logical_unit const rel_y = mouse_evt->y - abs_bounds.y;

                    if (rel_y.value - BORDER_WIDTH < static_cast<double>(m_header_height)) {
                        // Click in header area
                        logical_unit const rel_x = mouse_evt->x - abs_bounds.x;
                        int const clicked_col = column_at_x(rel_x.value - BORDER_WIDTH + m_scroll_offset_x);
                        if (clicked_col >= 0) {
                            handle_header_click(clicked_col);
                            return true;
                        }
                    }
                }
            }
        }

        return base::handle_event(evt, phase);
    }

protected:
    /**
     * @brief Handle mouse click with selection behavior
     */
    bool handle_mouse_click(logical_unit x, logical_unit y,
                           bool ctrl_held, bool shift_held) override {
        auto abs_bounds = this->get_absolute_logical_bounds();
        logical_unit const rel_x = x - abs_bounds.x;
        logical_unit const rel_y = y - abs_bounds.y;

        model_index index = index_at(rel_x, rel_y);

        if (index.is_valid()) {
            this->clicked.emit(index);

            if (this->m_selection_model) {
                switch (m_selection_behavior) {
                    case selection_behavior::select_rows:
                        handle_row_selection(index.row, ctrl_held, shift_held);
                        break;
                    case selection_behavior::select_items:
                        handle_cell_selection(index, ctrl_held, shift_held);
                        break;
                    case selection_behavior::select_columns:
                        handle_column_selection(index.column, ctrl_held, shift_held);
                        break;
                }
                this->m_selection_model->set_current_index(index);
            }
            return true;
        }
        return false;
    }

    // ===================================================================
    // Keyboard Navigation
    // ===================================================================

    model_index move_cursor_down(const model_index& current) override {
        return base::move_cursor_down(current);
    }

    model_index move_cursor_up(const model_index& current) override {
        return base::move_cursor_up(current);
    }

    /**
     * @brief Handle key press for navigation
     */
    bool handle_key_press(int key, int modifiers) override {
        if (!this->m_model || !this->m_selection_model) {
            return false;
        }

        model_index current = this->m_selection_model->current_index();

        // Handle left/right for cell selection
        if (m_selection_behavior == selection_behavior::select_items) {
            if (key == static_cast<int>(key_code::arrow_left)) {
                auto next = move_cursor_left(current);
                if (next.is_valid()) {
                    this->m_selection_model->set_current_index(next);
                    this->m_selection_model->clear_selection();
                    this->m_selection_model->select(next);
                    scroll_to(next);
                    return true;
                }
            } else if (key == static_cast<int>(key_code::arrow_right)) {
                auto next = move_cursor_right(current);
                if (next.is_valid()) {
                    this->m_selection_model->set_current_index(next);
                    this->m_selection_model->clear_selection();
                    this->m_selection_model->select(next);
                    scroll_to(next);
                    return true;
                }
            }
        }

        // Tab navigation for cell mode
        if (m_selection_behavior == selection_behavior::select_items) {
            bool const shift_held = (modifiers & static_cast<int>(key_modifier::shift)) != 0;
            if (key == static_cast<int>(key_code::tab)) {
                auto next = shift_held ? move_cursor_left(current) : move_cursor_right(current);
                if (next.is_valid()) {
                    this->m_selection_model->set_current_index(next);
                    this->m_selection_model->clear_selection();
                    this->m_selection_model->select(next);
                    scroll_to(next);
                    return true;
                }
            }
        }

        return base::handle_key_press(key, modifiers);
    }

    /**
     * @brief Move cursor left one column
     */
    [[nodiscard]] model_index move_cursor_left(const model_index& current) const {
        if (!this->m_model) return {};

        if (!current.is_valid()) {
            int const col_count = this->m_model->column_count();
            return this->m_model->index(0, col_count - 1);
        }

        if (current.column > 0) {
            return this->m_model->index(current.row, current.column - 1);
        }

        // Wrap to previous row
        if (current.row > 0) {
            int const col_count = this->m_model->column_count();
            return this->m_model->index(current.row - 1, col_count - 1);
        }

        return current;
    }

    /**
     * @brief Move cursor right one column
     */
    [[nodiscard]] model_index move_cursor_right(const model_index& current) const {
        if (!this->m_model) return {};

        int const col_count = this->m_model->column_count();

        if (!current.is_valid()) {
            return this->m_model->index(0, 0);
        }

        if (current.column < col_count - 1) {
            return this->m_model->index(current.row, current.column + 1);
        }

        // Wrap to next row
        if (current.row < this->m_model->row_count() - 1) {
            return this->m_model->index(current.row + 1, 0);
        }

        return current;
    }

private:
    // ===================================================================
    // Helper Methods
    // ===================================================================

    void ensure_column_exists(int column) {
        while (static_cast<int>(m_columns.size()) <= column) {
            m_columns.push_back(column_info{});
        }
    }

    void ensure_columns_match_model() {
        if (!this->m_model) return;
        int const col_count = this->m_model->column_count();
        while (static_cast<int>(m_columns.size()) < col_count) {
            m_columns.push_back(column_info{});
        }
    }

    void calculate_column_widths() {
        if (!this->m_model || !this->m_delegate) return;

        int const col_count = this->m_model->column_count();
        double const view_width = this->bounds().width.value - (BORDER_WIDTH * 2);  // Account for border

        auto const* metrics = ui_services<Backend>::metrics();

        // First pass: calculate auto widths and fixed widths
        int total_fixed = 0;
        int total_stretch = 0;

        for (int col = 0; col < col_count && col < static_cast<int>(m_columns.size()); ++col) {
            auto& col_info = m_columns[static_cast<std::size_t>(col)];

            if (col_info.stretch_factor > 0) {
                total_stretch += col_info.stretch_factor;
                col_info.calculated_width = col_info.min_width;
                total_fixed += col_info.min_width;
            } else if (col_info.width > 0) {
                col_info.calculated_width = std::max(col_info.width, col_info.min_width);
                total_fixed += col_info.calculated_width;
            } else {
                // Auto width: measure content
                int max_width = col_info.min_width;

                // Use default auto width for header area
                max_width = std::max(max_width, DEFAULT_AUTO_COLUMN_WIDTH);

                // Sample rows for width
                int const sample_rows = std::min(this->m_model->row_count(), 20);
                for (int row = 0; row < sample_rows; ++row) {
                    auto idx = this->m_model->index(row, col);
                    auto size = this->m_delegate->size_hint(idx);
                    int const logical_w = metrics
                        ? static_cast<int>(metrics->physical_to_logical_x(size.w).value)
                        : size.w;
                    max_width = std::max(max_width, logical_w);
                }

                col_info.calculated_width = max_width;
                total_fixed += col_info.calculated_width;
            }
        }

        // Second pass: distribute remaining space to stretch columns
        if (total_stretch > 0) {
            double const remaining = view_width - static_cast<double>(total_fixed);
            if (remaining > 0) {
                for (int col = 0; col < col_count && col < static_cast<int>(m_columns.size()); ++col) {
                    auto& col_info = m_columns[static_cast<std::size_t>(col)];
                    if (col_info.stretch_factor > 0) {
                        double const share = remaining * static_cast<double>(col_info.stretch_factor) /
                            static_cast<double>(total_stretch);
                        col_info.calculated_width = std::max(col_info.min_width,
                            col_info.min_width + static_cast<int>(share));
                    }
                }
            }
        }
    }

    [[nodiscard]] int column_at_x(double x) const {
        double col_x = 0.0;
        for (int col = 0; col < static_cast<int>(m_columns.size()); ++col) {
            double const col_w = static_cast<double>(m_columns[static_cast<std::size_t>(col)].calculated_width);
            if (x >= col_x && x < col_x + col_w) {
                return col;
            }
            col_x += col_w;
        }
        return -1;
    }

    [[nodiscard]] bool is_cell_selected(const model_index& index) const {
        if (!this->m_selection_model || !this->m_model) return false;

        switch (m_selection_behavior) {
            case selection_behavior::select_rows:
                // Row selection stores selection at column 0, so check only that
                // This is O(1) instead of O(columns)
                {
                    auto row_idx = this->m_model->index(index.row, 0);
                    return this->m_selection_model->is_selected(row_idx);
                }

            case selection_behavior::select_columns:
                // Column selection stores selection at row 0, so check only that
                // This is O(1) instead of O(rows)
                {
                    auto col_idx = this->m_model->index(0, index.column);
                    return this->m_selection_model->is_selected(col_idx);
                }

            case selection_behavior::select_items:
            default:
                return this->m_selection_model->is_selected(index);
        }
    }

    void handle_row_selection(int row, bool ctrl_held, bool shift_held) {
        if (!this->m_model || !this->m_selection_model) return;

        auto mode = this->m_selection_model->get_selection_mode();
        auto idx = this->m_model->index(row, 0);

        // multi_selection: Click toggles, modifiers ignored
        if (mode == selection_mode::multi_selection) {
            this->m_selection_model->toggle(idx);
            this->m_selection_model->set_current_index(idx);
            this->m_selection_anchor = idx;
            return;
        }

        // For contiguous_selection, shift always clears and ignores Ctrl
        bool const is_contiguous = (mode == selection_mode::contiguous_selection);

        if (shift_held && this->m_selection_anchor.is_valid() &&
            mode != selection_mode::single_selection) {
            // Range selection: select all rows from anchor to current
            // Contiguous mode: always clear (Ctrl is ignored)
            // Extended mode: clear unless Ctrl is held
            if (is_contiguous || !ctrl_held) {
                this->m_selection_model->clear_selection();
            }
            int const anchor_row = this->m_selection_anchor.row;
            int const start_row = std::min(anchor_row, row);
            int const end_row = std::max(anchor_row, row);
            for (int r = start_row; r <= end_row; ++r) {
                auto row_idx = this->m_model->index(r, 0);
                this->m_selection_model->select(row_idx);
            }
            // Don't update anchor on shift-click
        } else if (ctrl_held && mode == selection_mode::extended_selection) {
            // Toggle selection (only in extended mode, not contiguous)
            if (this->m_selection_model->is_selected(idx)) {
                this->m_selection_model->deselect(idx);
            } else {
                this->m_selection_model->select(idx);
            }
            this->m_selection_anchor = idx;
        } else {
            // Simple selection (or single_selection/contiguous_selection mode without shift)
            this->m_selection_model->clear_selection();
            this->m_selection_model->select(idx);
            this->m_selection_anchor = idx;
        }
    }

    void handle_cell_selection(const model_index& index, bool ctrl_held, bool shift_held) {
        if (!this->m_selection_model) return;

        auto mode = this->m_selection_model->get_selection_mode();

        // multi_selection: Click toggles, modifiers ignored
        if (mode == selection_mode::multi_selection) {
            this->m_selection_model->toggle(index);
            this->m_selection_model->set_current_index(index);
            this->m_selection_anchor = index;
            return;
        }

        // For contiguous_selection, shift always clears and ignores Ctrl
        bool const is_contiguous = (mode == selection_mode::contiguous_selection);

        if (shift_held && this->m_selection_anchor.is_valid() &&
            mode != selection_mode::single_selection) {
            // Rectangle selection
            // Contiguous mode: always clear (Ctrl is ignored)
            // Extended mode: clear unless Ctrl is held
            if (is_contiguous || !ctrl_held) {
                this->m_selection_model->clear_selection();
            }
            select_rectangle(this->m_selection_anchor, index);
        } else if (ctrl_held && mode == selection_mode::extended_selection) {
            // Toggle selection (only in extended mode, not contiguous)
            this->m_selection_model->toggle(index);
            this->m_selection_anchor = index;
        } else {
            // Clear and select the new cell (single/contiguous mode without shift)
            this->m_selection_model->clear_selection();
            this->m_selection_model->select(index);
            this->m_selection_anchor = index;
        }
    }

    void handle_column_selection(int column, bool ctrl_held, bool shift_held) {
        if (!this->m_model || !this->m_selection_model) return;

        auto mode = this->m_selection_model->get_selection_mode();
        auto idx = this->m_model->index(0, column);

        // multi_selection: Click toggles, modifiers ignored
        if (mode == selection_mode::multi_selection) {
            this->m_selection_model->toggle(idx);
            this->m_selection_model->set_current_index(idx);
            this->m_selection_anchor = idx;
            return;
        }

        // For contiguous_selection, shift always clears and ignores Ctrl
        bool const is_contiguous = (mode == selection_mode::contiguous_selection);

        if (shift_held && this->m_selection_anchor.is_valid() &&
            mode != selection_mode::single_selection) {
            // Range selection: select all columns from anchor to current
            // In contiguous mode, always clear (Ctrl is ignored)
            if (is_contiguous || !ctrl_held) {
                this->m_selection_model->clear_selection();
            }
            int const anchor_col = this->m_selection_anchor.column;
            int const start_col = std::min(anchor_col, column);
            int const end_col = std::max(anchor_col, column);
            for (int c = start_col; c <= end_col; ++c) {
                auto col_idx = this->m_model->index(0, c);
                this->m_selection_model->select(col_idx);
            }
            // Don't update anchor on shift-click
        } else if (ctrl_held && mode == selection_mode::extended_selection) {
            // Toggle selection (only in extended mode, not contiguous)
            if (this->m_selection_model->is_selected(idx)) {
                this->m_selection_model->deselect(idx);
            } else {
                this->m_selection_model->select(idx);
            }
            this->m_selection_anchor = idx;
        } else {
            // Simple selection (or single_selection/contiguous mode without shift)
            this->m_selection_model->clear_selection();
            this->m_selection_model->select(idx);
            this->m_selection_anchor = idx;
        }
    }

    void select_rectangle(const model_index& from, const model_index& to) {
        if (!this->m_model || !this->m_selection_model) return;

        int const r1 = std::min(from.row, to.row);
        int const r2 = std::max(from.row, to.row);
        int const c1 = std::min(from.column, to.column);
        int const c2 = std::max(from.column, to.column);

        for (int row = r1; row <= r2; ++row) {
            for (int col = c1; col <= c2; ++col) {
                auto idx = this->m_model->index(row, col);
                this->m_selection_model->select(idx);
            }
        }
    }

    void handle_header_click(int column) {
        if (m_sort_column == column) {
            // Toggle sort order
            m_sort_order = (m_sort_order == sort_order::ascending)
                ? sort_order::descending
                : sort_order::ascending;
        } else {
            m_sort_column = column;
            m_sort_order = sort_order::ascending;
        }

        if (this->m_model) {
            this->m_model->sort(column, m_sort_order);
        }

        header_clicked.emit(column);
        this->mark_dirty();
    }

    void render_headers(render_context<Backend>& ctx, int content_x, int content_y,
                       int content_width, auto const* theme, auto const* metrics) const {
        if (!this->m_model || !theme) return;

        int const physical_header_height = metrics
            ? metrics->snap_to_physical_y(logical_unit(static_cast<double>(m_header_height)), snap_mode::round)
            : m_header_height;

        // Draw header background
        rect_type header_bg{content_x, content_y, content_width, physical_header_height};
        ctx.fill_rect(header_bg, theme->table.header_background);

        // Draw column headers
        double col_x = 0.0 - m_scroll_offset_x;
        int const col_count = this->m_model->column_count();

        for (int col = 0; col < col_count && col < static_cast<int>(m_columns.size()); ++col) {
            double const col_w = static_cast<double>(m_columns[static_cast<std::size_t>(col)].calculated_width);

            // Skip if not visible
            if (col_x + col_w <= 0 || col_x >= static_cast<double>(content_width)) {
                col_x += col_w;
                continue;
            }

            int const abs_x = content_x + (metrics
                ? metrics->snap_to_physical_x(logical_unit(col_x), snap_mode::floor)
                : static_cast<int>(col_x));
            int const abs_w = metrics
                ? metrics->snap_to_physical_x(logical_unit(col_w), snap_mode::round)
                : static_cast<int>(col_w);

            // Get header text from model
            std::string header_text = this->m_model->header(col);
            if (header_text.empty()) {
                // Use column index as fallback
                header_text = "Col " + std::to_string(col);
            }

            // Draw header cell
            rect_type cell_rect{abs_x, content_y, abs_w, physical_header_height};

            // Draw header text with padding
            ctx.draw_text(header_text, {abs_x + HEADER_PADDING, content_y + 2}, theme->table.header_font, theme->table.header_foreground);

            // Draw sort indicator
            if (m_sort_column == col) {
                std::string indicator = (m_sort_order == sort_order::ascending) ? " ^" : " v";
                int const text_x = abs_x + abs_w - SORT_INDICATOR_WIDTH;
                ctx.draw_text(indicator, {text_x, content_y + 2}, theme->table.header_font, theme->table.sort_indicator_color);
            }

            col_x += col_w;
        }

        // Draw header bottom border
        rect_type header_border{content_x, content_y + physical_header_height - 1, content_width, 1};
        ctx.fill_rect(header_border, theme->table.header_border);
    }

    // ===================================================================
    // Member Variables
    // ===================================================================

    std::vector<column_info> m_columns;
    std::vector<std::vector<logical_rect>> m_cell_rects;

    int m_content_width = 0;
    int m_content_height = 0;
    int m_row_height = 0;

    double m_scroll_offset_x = 0.0;
    double m_scroll_offset_y = 0.0;

    bool m_headers_visible = false;
    int m_header_height = DEFAULT_HEADER_HEIGHT;

    bool m_horizontal_grid_visible = false;
    bool m_vertical_grid_visible = false;

    selection_behavior m_selection_behavior = selection_behavior::select_rows;

    bool m_sorting_enabled = false;
    int m_sort_column = -1;
    sort_order m_sort_order = sort_order::ascending;

    logical_rect m_last_arranged_bounds{};
};

} // namespace onyxui
