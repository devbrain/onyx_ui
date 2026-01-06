//
// OnyxUI MVC System - Table Model
// Created: 2026-01-05
//

#pragma once

#include <algorithm>
#include <string>
#include <vector>

#include <onyxui/mvc/models/abstract_item_model.hh>

namespace onyxui {

/**
 * @brief A generic table model for 2D tabular data
 *
 * @tparam Row The row type (user-defined struct or tuple)
 * @tparam Backend The UI backend type
 *
 * @details
 * table_model provides a type-safe 2D data container that integrates with the
 * MVC system. Unlike list_model which stores a single-column list, table_model
 * stores rows of data with multiple columns.
 *
 * To use this class, you must subclass it and override the `column_data()`
 * method to extract column values from your row type.
 *
 * @par Example Usage:
 * @code
 * // Define a row type
 * struct Person {
 *     std::string name;
 *     int age;
 *     std::string email;
 * };
 *
 * // Create a model subclass
 * template<UIBackend Backend>
 * class person_table_model : public table_model<Person, Backend> {
 * public:
 *     person_table_model() {
 *         this->set_headers({"Name", "Age", "Email"});
 *     }
 *
 * protected:
 *     variant_type column_data(const Person& p, int col, item_data_role role) const override {
 *         if (role != item_data_role::display) return std::monostate{};
 *         switch (col) {
 *             case 0: return p.name;
 *             case 1: return std::to_string(p.age);
 *             case 2: return p.email;
 *             default: return std::monostate{};
 *         }
 *     }
 * };
 *
 * // Use the model
 * auto model = std::make_shared<person_table_model<Backend>>();
 * model->set_rows({
 *     {"Alice", 30, "alice@example.com"},
 *     {"Bob", 25, "bob@example.com"}
 * });
 * @endcode
 *
 * @par Thread Safety:
 * NOT thread-safe. Access only from UI thread.
 */
template<typename Row, UIBackend Backend>
class table_model : public abstract_item_model<Backend> {
public:
    using value_type = Row;
    using container_type = std::vector<Row>;
    using base = abstract_item_model<Backend>;
    using variant_type = typename base::variant_type;

    // ===================================================================
    // Constructors
    // ===================================================================

    /**
     * @brief Construct an empty table model
     */
    table_model() = default;

    /**
     * @brief Construct with initial rows
     * @param rows Initial list of rows
     */
    explicit table_model(container_type rows)
        : m_rows(std::move(rows)) {}

    // ===================================================================
    // Column Configuration
    // ===================================================================

    /**
     * @brief Set column headers
     * @param headers Vector of header strings
     *
     * @details
     * This also sets the column count. Call this before populating data.
     * After setting headers, column_count() will return the number of headers.
     *
     * @par Example:
     * @code
     * model->set_headers({"Name", "Age", "Email", "Status"});
     * @endcode
     */
    void set_headers(std::vector<std::string> headers) {
        m_headers = std::move(headers);
        this->layout_changed.emit();
    }

    /**
     * @brief Get header text for a column
     * @param column Column index (0-based)
     * @return Header text, or empty string if column is out of range
     */
    [[nodiscard]] std::string header(int column) const override {
        if (column < 0 || column >= static_cast<int>(m_headers.size())) {
            return "";
        }
        return m_headers[static_cast<std::size_t>(column)];
    }

    /**
     * @brief Get all headers
     * @return Const reference to header vector
     */
    [[nodiscard]] const std::vector<std::string>& headers() const noexcept {
        return m_headers;
    }

    // ===================================================================
    // Data Access (Override from abstract_item_model)
    // ===================================================================

    /**
     * @brief Get the number of rows
     * @param parent Parent index (ignored for tables - flat structure)
     * @return Number of rows
     */
    [[nodiscard]] int row_count(const model_index& parent = {}) const override {
        if (parent.is_valid()) {
            return 0;  // Tables are flat (no hierarchy)
        }
        return static_cast<int>(m_rows.size());
    }

    /**
     * @brief Get the number of columns
     * @param parent Parent index (ignored for tables - flat structure)
     * @return Number of columns (based on header count)
     */
    [[nodiscard]] int column_count(const model_index& parent = {}) const override {
        (void)parent;
        return static_cast<int>(m_headers.size());
    }

    /**
     * @brief Create an index for a specific row/column
     * @param row Row number (0-based)
     * @param column Column number (0-based)
     * @param parent Parent index (ignored for tables)
     * @return Valid model_index, or invalid if out of bounds
     */
    [[nodiscard]] model_index index(int row, int column, const model_index& parent = {}) const override {
        // Tables don't support hierarchy
        if (parent.is_valid()) {
            return {};
        }

        // Validate row/column bounds
        if (row < 0 || row >= static_cast<int>(m_rows.size()) ||
            column < 0 || column >= static_cast<int>(m_headers.size())) {
            return {};
        }

        // Create valid index
        return {row, column, nullptr, this};
    }

    /**
     * @brief Get the parent of a child index
     * @param child Child index
     * @return Always returns invalid index (tables are flat)
     */
    [[nodiscard]] model_index parent(const model_index& child) const override {
        (void)child;
        return {};  // Tables have no parent (flat structure)
    }

    /**
     * @brief Get data for a cell
     * @param index Cell index (row, column)
     * @param role Data role (display, edit, tooltip, etc.)
     * @return Data as variant, or monostate if not available
     *
     * @details
     * Delegates to the pure virtual column_data() method, which subclasses
     * must implement to extract column values from their row type.
     */
    [[nodiscard]] variant_type data(
        const model_index& index,
        item_data_role role = item_data_role::display
    ) const override {
        // Validate index
        if (!index.is_valid() ||
            index.row < 0 || index.row >= static_cast<int>(m_rows.size()) ||
            index.column < 0 || index.column >= static_cast<int>(m_headers.size())) {
            return std::monostate{};
        }

        const Row& row = m_rows[static_cast<std::size_t>(index.row)];
        return column_data(row, index.column, role);
    }

    /**
     * @brief Set data for a cell
     * @param index Cell index (row, column)
     * @param value New value
     * @param role Data role (usually edit)
     * @return true if data was set, false if read-only or invalid
     *
     * @details
     * Delegates to the virtual set_column_data() method, which subclasses
     * can implement to modify their row data.
     * Default implementation returns false (read-only model).
     */
    bool set_data(
        const model_index& index,
        const variant_type& value,
        item_data_role role = item_data_role::edit
    ) override {
        // Validate index
        if (!index.is_valid() ||
            index.row < 0 || index.row >= static_cast<int>(m_rows.size()) ||
            index.column < 0 || index.column >= static_cast<int>(m_headers.size())) {
            return false;
        }

        Row& row = m_rows[static_cast<std::size_t>(index.row)];
        bool success = set_column_data(row, index.column, value, role);
        if (success) {
            this->data_changed.emit(index, index);
        }
        return success;
    }

    // ===================================================================
    // Row Manipulation
    // ===================================================================

    /**
     * @brief Replace all rows in the model
     * @param rows New list of rows
     *
     * @details
     * Efficiently replaces all rows by:
     * 1. Removing old rows (emits rows_removed)
     * 2. Adding new rows (emits rows_inserted)
     */
    void set_rows(container_type rows) {
        // Remove old rows
        if (!m_rows.empty()) {
            int const last_row = static_cast<int>(m_rows.size()) - 1;
            this->begin_remove_rows({}, 0, last_row);
            m_rows.clear();
            this->end_remove_rows();
            this->rows_removed.emit({}, 0, last_row);
        }

        // Add new rows
        if (!rows.empty()) {
            int const last_row = static_cast<int>(rows.size()) - 1;
            this->begin_insert_rows({}, 0, last_row);
            m_rows = std::move(rows);
            this->end_insert_rows();
            this->rows_inserted.emit({}, 0, last_row);
        }
    }

    /**
     * @brief Append a row to the end
     * @param row Row to append
     */
    void append(const Row& row) {
        int row_idx = static_cast<int>(m_rows.size());
        this->begin_insert_rows({}, row_idx, row_idx);
        m_rows.push_back(row);
        this->end_insert_rows();
        this->rows_inserted.emit({}, row_idx, row_idx);
    }

    /**
     * @brief Append a row to the end (move version)
     * @param row Row to append (moved)
     */
    void append(Row&& row) {
        int row_idx = static_cast<int>(m_rows.size());
        this->begin_insert_rows({}, row_idx, row_idx);
        m_rows.push_back(std::move(row));
        this->end_insert_rows();
        this->rows_inserted.emit({}, row_idx, row_idx);
    }

    /**
     * @brief Insert a row at a specific position
     * @param row_idx Row index (0-based)
     * @param row Row to insert
     */
    void insert(int row_idx, const Row& row) {
        if (row_idx < 0 || row_idx > static_cast<int>(m_rows.size())) {
            return;  // Out of range
        }

        this->begin_insert_rows({}, row_idx, row_idx);
        m_rows.insert(m_rows.begin() + row_idx, row);
        this->end_insert_rows();
        this->rows_inserted.emit({}, row_idx, row_idx);
    }

    /**
     * @brief Remove a row at a specific position
     * @param row_idx Row index (0-based)
     */
    void remove(int row_idx) {
        if (row_idx < 0 || row_idx >= static_cast<int>(m_rows.size())) {
            return;  // Out of range
        }

        this->begin_remove_rows({}, row_idx, row_idx);
        m_rows.erase(m_rows.begin() + row_idx);
        this->end_remove_rows();
        this->rows_removed.emit({}, row_idx, row_idx);
    }

    /**
     * @brief Clear all rows from the model
     */
    void clear() {
        if (m_rows.empty()) {
            return;  // Already empty
        }

        int const last_row = static_cast<int>(m_rows.size()) - 1;
        this->begin_remove_rows({}, 0, last_row);
        m_rows.clear();
        this->end_remove_rows();
        this->rows_removed.emit({}, 0, last_row);
    }

    // ===================================================================
    // Direct Access (for advanced use cases)
    // ===================================================================

    /**
     * @brief Get read-only access to underlying rows
     * @return Const reference to std::vector<Row>
     */
    [[nodiscard]] const container_type& rows() const noexcept {
        return m_rows;
    }

    /**
     * @brief Get row at specific index
     * @param row_idx Row number (0-based)
     * @return Const reference to row
     * @throws std::out_of_range if row is invalid
     */
    [[nodiscard]] const Row& at(int row_idx) const {
        return m_rows.at(static_cast<std::size_t>(row_idx));
    }

    // ===================================================================
    // Sorting (Optional Override)
    // ===================================================================

    /**
     * @brief Sort model by column
     * @param column Column to sort by
     * @param order Sort order (ascending/descending)
     *
     * @details
     * Default implementation is a no-op.
     * Subclasses should override if sorting is needed.
     *
     * A typical implementation would:
     * 1. Call begin_layout_change()
     * 2. std::sort rows using column comparison
     * 3. Call end_layout_change()
     *
     * @par Example Override:
     * @code
     * void sort(int column, sort_order order) override {
     *     this->begin_layout_change();
     *     std::sort(m_rows.begin(), m_rows.end(),
     *         [column, order](const Row& a, const Row& b) {
     *             // Custom comparison based on column
     *             return (order == sort_order::ascending)
     *                 ? compare(a, b, column)
     *                 : compare(b, a, column);
     *         });
     *     this->end_layout_change();
     * }
     * @endcode
     */
    void sort(int column, sort_order order = sort_order::ascending) override {
        (void)column;
        (void)order;
        // Default: no-op
        // Subclasses can override to implement sorting
    }

protected:
    // ===================================================================
    // Pure Virtual Method (Subclasses Must Implement)
    // ===================================================================

    /**
     * @brief Extract column value from a row
     * @param row The row data
     * @param column Column index (0-based)
     * @param role Data role (display, edit, background, etc.)
     * @return Data as variant for the cell
     *
     * @details
     * Subclasses MUST override this to extract column values from their
     * specific row type. This is the key customization point.
     *
     * @par Example:
     * @code
     * variant_type column_data(const Person& p, int col, item_data_role role) const override {
     *     if (role != item_data_role::display) return std::monostate{};
     *     switch (col) {
     *         case 0: return p.name;
     *         case 1: return std::to_string(p.age);
     *         case 2: return p.email;
     *         default: return std::monostate{};
     *     }
     * }
     * @endcode
     */
    [[nodiscard]] virtual variant_type column_data(
        const Row& row,
        int column,
        item_data_role role
    ) const = 0;

    /**
     * @brief Set column value in a row
     * @param row The row data to modify
     * @param column Column index (0-based)
     * @param value New value
     * @param role Data role (usually edit)
     * @return true if data was set, false if read-only or unsupported
     *
     * @details
     * Default implementation returns false (read-only model).
     * Subclasses can override to support editing.
     *
     * @par Example:
     * @code
     * bool set_column_data(Person& p, int col, const variant_type& value, item_data_role role) override {
     *     if (role != item_data_role::edit) return false;
     *     if (!std::holds_alternative<std::string>(value)) return false;
     *     const auto& str = std::get<std::string>(value);
     *     switch (col) {
     *         case 0: p.name = str; return true;
     *         case 1: p.age = std::stoi(str); return true;
     *         case 2: p.email = str; return true;
     *         default: return false;
     *     }
     * }
     * @endcode
     */
    virtual bool set_column_data(
        Row& row,
        int column,
        const variant_type& value,
        item_data_role role
    ) {
        (void)row;
        (void)column;
        (void)value;
        (void)role;
        return false;  // Default: read-only
    }

private:
    container_type m_rows;                  ///< Row data
    std::vector<std::string> m_headers;     ///< Column headers
};

// ===================================================================
// Convenience: String-Based Table Model
// ===================================================================

/**
 * @brief A simple table model using vector<string> for each row
 *
 * @tparam Backend The UI backend type
 *
 * @details
 * string_table_model is a concrete table_model that uses
 * std::vector<std::string> as the row type. Each row is a vector
 * of strings, one per column.
 *
 * This is useful for simple string-based tables where you don't
 * need a custom row type.
 *
 * @par Example:
 * @code
 * auto model = std::make_shared<string_table_model<Backend>>();
 * model->set_headers({"Name", "Age", "City"});
 * model->append({"Alice", "30", "New York"});
 * model->append({"Bob", "25", "Chicago"});
 * @endcode
 */
template<UIBackend Backend>
class string_table_model : public table_model<std::vector<std::string>, Backend> {
public:
    using base = table_model<std::vector<std::string>, Backend>;
    using variant_type = typename base::variant_type;

protected:
    /**
     * @brief Extract string from the row's column
     */
    [[nodiscard]] variant_type column_data(
        const std::vector<std::string>& row,
        int column,
        item_data_role role
    ) const override {
        if (role == item_data_role::display || role == item_data_role::edit) {
            if (column >= 0 && column < static_cast<int>(row.size())) {
                return row[static_cast<std::size_t>(column)];
            }
        }
        return std::monostate{};
    }

    /**
     * @brief Set string in the row's column
     */
    bool set_column_data(
        std::vector<std::string>& row,
        int column,
        const variant_type& value,
        item_data_role role
    ) override {
        if (role != item_data_role::edit && role != item_data_role::display) {
            return false;
        }
        if (column < 0 || column >= static_cast<int>(row.size())) {
            return false;
        }
        if (!std::holds_alternative<std::string>(value)) {
            return false;
        }
        row[static_cast<std::size_t>(column)] = std::get<std::string>(value);
        return true;
    }
};

} // namespace onyxui
