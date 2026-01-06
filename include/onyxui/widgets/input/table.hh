//
// OnyxUI - Simple Table Widget
// Created: 2026-01-05
// Refactored: 2026-01-05 (inherits from table_view)
//

#pragma once

#include <memory>
#include <string>
#include <vector>

#include <onyxui/concepts/backend.hh>
#include <onyxui/core/signal.hh>
#include <onyxui/mvc/models/table_model.hh>
#include <onyxui/mvc/views/table_view.hh>
#include <onyxui/mvc/selection/item_selection_model.hh>

namespace onyxui {

/**
 * @brief A simple table widget with string-based API
 *
 * @tparam Backend The UI backend type
 *
 * @details
 * table is the easy-to-use table widget with a simple string-based API.
 * It inherits from table_view (privately) and owns a string_table_model,
 * exposing a simplified interface.
 *
 * For advanced use cases requiring custom models, delegates, or selection models,
 * use table_view directly instead.
 *
 * Features:
 * - **Simple API**: set_columns(), add_row(), set_cell(), cell()
 * - **Self-Contained**: Owns its data internally
 * - **No MVC Knowledge Required**: Just use strings
 * - **Row Selection**: Click to select rows
 * - **Headers**: Optional header row display
 * - **Grid Lines**: Optional horizontal/vertical grid lines
 * - **Scrolling**: Inherits table_view scrolling support
 *
 * @par Example Usage:
 * @code
 * // Create table with columns
 * auto tbl = std::make_unique<table<Backend>>();
 * tbl->set_columns({"Name", "Age", "City"});
 *
 * // Add rows
 * tbl->add_row({"Alice", "30", "New York"});
 * tbl->add_row({"Bob", "25", "Chicago"});
 * tbl->add_row({"Charlie", "35", "Boston"});
 *
 * // Enable headers
 * tbl->set_headers_visible(true);
 *
 * // Listen for selection changes
 * tbl->current_row_changed.connect([](int row) {
 *     std::cout << "Selected row: " << row << "\n";
 * });
 *
 * // Listen for row activation (double-click or Enter)
 * tbl->row_activated.connect([](int row) {
 *     std::cout << "Activated row: " << row << "\n";
 * });
 * @endcode
 *
 * @see table_view for advanced MVC usage
 * @see table_model for custom data types
 */
template<UIBackend Backend>
class table : private table_view<Backend> {
public:
    using base = table_view<Backend>;
    using model_type = string_table_model<Backend>;
    using selection_model_type = item_selection_model<Backend>;

    // Expose ui_element interface from base
    using base::measure;
    using base::arrange;
    using base::bounds;
    using base::render;
    using base::handle_event;
    using base::has_focus;
    using base::is_focusable;
    using base::set_focusable;
    using base::parent;

    // Expose table_view configuration methods
    using base::set_column_width;
    using base::set_headers_visible;
    using base::headers_visible;
    using base::set_horizontal_grid_visible;
    using base::set_vertical_grid_visible;

    // ===================================================================
    // Signals
    // ===================================================================

    /// @brief Emitted when current row changes
    signal<int> current_row_changed;

    /// @brief Emitted when a row is activated (double-click or Enter)
    signal<int> row_activated;

    /// @brief Emitted when a row is clicked
    signal<int> row_clicked;

    /// @brief Emitted when a row is double-clicked
    signal<int> row_double_clicked;

    /// @brief Emitted when selection changes (for multi-selection)
    signal<> selection_changed;

    // ===================================================================
    // Constructors
    // ===================================================================

    /**
     * @brief Construct an empty table
     */
    table()
        : m_model(std::make_shared<model_type>())
    {
        // Configure the view with our internal model
        base::set_model(m_model.get());

        // Forward signals from base view to our simple signals
        if (auto* sel_model = base::selection_model()) {
            m_current_changed_conn = scoped_connection(
                sel_model->current_changed,
                [this](const model_index& current, const model_index& /*previous*/) {
                    int row = current.is_valid() ? current.row : -1;
                    current_row_changed.emit(row);
                }
            );

            m_selection_changed_conn = scoped_connection(
                sel_model->selection_changed,
                [this](const std::vector<model_index>&, const std::vector<model_index>&) {
                    selection_changed.emit();
                }
            );
        }

        m_activated_conn = scoped_connection(
            base::activated,
            [this](const model_index& idx) {
                if (idx.is_valid()) {
                    row_activated.emit(idx.row);
                }
            }
        );

        m_clicked_conn = scoped_connection(
            base::clicked,
            [this](const model_index& idx) {
                if (idx.is_valid()) {
                    row_clicked.emit(idx.row);
                }
            }
        );

        m_double_clicked_conn = scoped_connection(
            base::double_clicked,
            [this](const model_index& idx) {
                if (idx.is_valid()) {
                    row_double_clicked.emit(idx.row);
                }
            }
        );
    }

    /**
     * @brief Construct with column headers
     * @param columns List of column header strings
     */
    explicit table(std::vector<std::string> columns)
        : table()
    {
        set_columns(std::move(columns));
    }

    // Non-copyable, movable
    table(const table&) = delete;
    table& operator=(const table&) = delete;
    table(table&&) noexcept = default;
    table& operator=(table&&) noexcept = default;

    // ===================================================================
    // Column Configuration
    // ===================================================================

    /**
     * @brief Set column headers
     * @param columns List of column header strings
     *
     * @details
     * This also sets the column count. Call before adding rows.
     * After calling set_columns(), column_count() returns the new count.
     */
    void set_columns(std::vector<std::string> columns) {
        m_model->set_headers(std::move(columns));
    }

    /**
     * @brief Get the number of columns
     * @return Column count
     */
    [[nodiscard]] int column_count() const noexcept {
        return m_model->column_count();
    }

    /**
     * @brief Get column header text
     * @param column Column index (0-based)
     * @return Header text, or empty string if invalid
     */
    [[nodiscard]] std::string column_header(int column) const {
        return m_model->header(column);
    }

    // ===================================================================
    // Row Management
    // ===================================================================

    /**
     * @brief Add a row to the end of the table
     * @param values Cell values for each column
     *
     * @details
     * The values vector should have the same size as column_count().
     * Missing values will be empty strings, extra values will be ignored.
     */
    void add_row(std::vector<std::string> values) {
        // Pad or truncate to match column count
        int cols = column_count();
        if (static_cast<int>(values.size()) < cols) {
            values.resize(static_cast<std::size_t>(cols));
        } else if (static_cast<int>(values.size()) > cols) {
            values.resize(static_cast<std::size_t>(cols));
        }
        m_model->append(std::move(values));
    }

    /**
     * @brief Insert a row at a specific position
     * @param row Row index (0-based)
     * @param values Cell values for each column
     */
    void insert_row(int row, std::vector<std::string> values) {
        int cols = column_count();
        if (static_cast<int>(values.size()) < cols) {
            values.resize(static_cast<std::size_t>(cols));
        } else if (static_cast<int>(values.size()) > cols) {
            values.resize(static_cast<std::size_t>(cols));
        }
        m_model->insert(row, std::move(values));
    }

    /**
     * @brief Remove a row
     * @param row Row index (0-based)
     */
    void remove_row(int row) {
        if (row < 0 || row >= row_count()) {
            return;
        }

        // Remember current selection for adjustment
        int current = current_row();

        // Remove from model
        m_model->remove(row);

        // Adjust selection
        if (current == row) {
            // Removed selected row - clear selection
            base::set_current_index(model_index{});
        } else if (current > row) {
            // Selection was after removed row - adjust
            base::set_current_index(m_model->index(current - 1, 0));
        }
    }

    /**
     * @brief Remove all rows
     */
    void clear() {
        base::set_current_index(model_index{});
        m_model->clear();
    }

    /**
     * @brief Replace all rows
     * @param rows New list of rows (each row is a vector of cell strings)
     */
    void set_rows(std::vector<std::vector<std::string>> rows) {
        m_model->set_rows(std::move(rows));
    }

    /**
     * @brief Get the number of rows
     * @return Row count
     */
    [[nodiscard]] int row_count() const noexcept {
        return m_model->row_count();
    }

    // ===================================================================
    // Cell Access
    // ===================================================================

    /**
     * @brief Get cell value
     * @param row Row index (0-based)
     * @param column Column index (0-based)
     * @return Cell text, or empty string if invalid
     */
    [[nodiscard]] std::string cell(int row, int column) const {
        if (row < 0 || row >= row_count() ||
            column < 0 || column >= column_count()) {
            return {};
        }
        auto data = m_model->data(m_model->index(row, column), item_data_role::display);
        if (std::holds_alternative<std::string>(data)) {
            return std::get<std::string>(data);
        }
        return {};
    }

    /**
     * @brief Set cell value
     * @param row Row index (0-based)
     * @param column Column index (0-based)
     * @param value New cell text
     *
     * @note Uses set_data() for efficient in-place update.
     */
    void set_cell(int row, int column, const std::string& value) {
        if (row < 0 || row >= row_count() ||
            column < 0 || column >= column_count()) {
            return;
        }
        // Use set_data() for efficient in-place update
        auto idx = m_model->index(row, column);
        m_model->set_data(idx, value, item_data_role::edit);
    }

    // ===================================================================
    // Selection Management
    // ===================================================================

    /**
     * @brief Get the currently selected row
     * @return Current row index, or -1 if no selection
     */
    [[nodiscard]] int current_row() const noexcept {
        auto idx = base::current_index();
        return idx.is_valid() ? idx.row : -1;
    }

    /**
     * @brief Set the current selection
     * @param row Row index to select, or -1 for no selection
     */
    void set_current_row(int row) {
        if (row < 0 || row >= row_count()) {
            // Clear selection
            base::set_current_index(model_index{});
            return;
        }

        auto idx = m_model->index(row, 0);
        base::set_current_index(idx);
    }

    /**
     * @brief Set both grid lines visibility
     * @param visible True to show grid lines
     */
    void set_grid_visible(bool visible) {
        set_horizontal_grid_visible(visible);
        set_vertical_grid_visible(visible);
    }

    // ===================================================================
    // Scrolling
    // ===================================================================

    /**
     * @brief Scroll to make a row visible
     * @param row Row index
     */
    void scroll_to_row(int row) {
        if (row < 0 || row >= row_count()) return;
        auto idx = m_model->index(row, 0);
        base::scroll_to(idx);
    }

    // ===================================================================
    // Access to Underlying View (advanced)
    // ===================================================================

    /**
     * @brief Get the underlying table_view
     * @return Pointer to table_view (this)
     *
     * @note For advanced configuration only.
     */
    [[nodiscard]] table_view<Backend>* view() noexcept { return this; }
    [[nodiscard]] const table_view<Backend>* view() const noexcept { return this; }

    /**
     * @brief Get the underlying model
     * @return Shared pointer to model
     *
     * @note For advanced configuration only.
     */
    // NOLINTNEXTLINE(google-hidding-function) - intentionally hides base::model() with different return type
    [[nodiscard]] std::shared_ptr<model_type> model() noexcept { return m_model; }
    // NOLINTNEXTLINE(google-hidding-function) - intentionally hides base::model() with different return type
    [[nodiscard]] std::shared_ptr<const model_type> model() const noexcept { return m_model; }

private:
    std::shared_ptr<model_type> m_model;

    // Signal connections (RAII cleanup)
    scoped_connection m_current_changed_conn;
    scoped_connection m_selection_changed_conn;
    scoped_connection m_activated_conn;
    scoped_connection m_clicked_conn;
    scoped_connection m_double_clicked_conn;
};

} // namespace onyxui
