//
// OnyxUI MVC System - Abstract Item Model
// Created: 2025-11-22
//

#pragma once

#include <any>
#include <string>
#include <variant>

#include <onyxui/concepts/backend.hh>
#include <onyxui/core/signal.hh>
#include <onyxui/mvc/model_index.hh>
#include <onyxui/mvc/item_data_role.hh>

namespace onyxui {

/**
 * @brief Item flags defining item behavior
 *
 * @details
 * Flags can be combined using bitwise OR:
 * @code
 * item_flags flags = item_flag::enabled | item_flag::selectable | item_flag::editable;
 * @endcode
 */
enum class item_flag : std::uint8_t {
    no_flags    = 0,      ///< No flags set (item is disabled, non-selectable)
    enabled     = 1 << 0, ///< Item is enabled and can receive user input
    selectable  = 1 << 1, ///< Item can be selected
    editable    = 1 << 2, ///< Item can be edited (future: in-place editing)
    drag_enabled = 1 << 3, ///< Item can be dragged (future: drag-drop)
    drop_enabled = 1 << 4, ///< Item can accept drops (future: drag-drop)
    user_checkable = 1 << 5 ///< Item has a checkbox (future: checkable items)
};

/// Type alias for combined item flags
using item_flags = std::uint8_t;

/// Bitwise OR for item flags
inline constexpr item_flags operator|(item_flag lhs, item_flag rhs) noexcept {
    return static_cast<item_flags>(static_cast<std::uint8_t>(lhs) | static_cast<std::uint8_t>(rhs));
}

/// Bitwise AND for item flags
inline constexpr item_flags operator&(item_flags lhs, item_flag rhs) noexcept {
    return static_cast<item_flags>(lhs & static_cast<std::uint8_t>(rhs));
}

/**
 * @brief Sort order for model sorting
 */
enum class sort_order : std::uint8_t {
    ascending,   ///< Sort in ascending order (A-Z, 0-9)
    descending   ///< Sort in descending order (Z-A, 9-0)
};

/**
 * @brief Abstract base class for all item models
 *
 * @tparam Backend The UI backend type
 *
 * @details
 * abstract_item_model defines the interface that all models must implement
 * to work with OnyxUI's model/view architecture. It provides:
 *
 * - **Data Access**: row_count(), column_count(), index(), parent(), data()
 * - **Data Modification**: set_data() (optional, default is read-only)
 * - **Structure Navigation**: index() and parent() for hierarchical models
 * - **Change Notification**: Signals to notify views when data changes
 *
 * @par Thread Safety:
 * Models are NOT thread-safe by design. All access must be from the UI thread.
 * Background data loading should use separate models, then swap on UI thread.
 *
 * @par Typical Subclasses:
 * - list_model<T>: Simple list of items
 * - table_model: 2D grid of data
 * - tree_model: Hierarchical data
 * - sort_filter_proxy_model: Wrapper for sorting/filtering
 *
 * @par Example Implementation:
 * See list_model.hh for a complete example.
 */
template<UIBackend Backend>
class abstract_item_model {
public:
    /**
     * @brief Variant type for model data
     *
     * @details
     * Models return data as a variant containing one of:
     * - std::string (for display text, tooltips)
     * - int, double, bool (for numeric/boolean data)
     * - Backend::color_type (for colors)
     * - std::any (for arbitrary custom types)
     */
    using variant_type = std::variant<
        std::monostate,  // Empty/invalid
        std::string,
        int,
        double,
        bool,
        typename Backend::color_type,
        std::any  // For custom types
    >;

    virtual ~abstract_item_model() = default;

    // ===================================================================
    // Data Access (Pure Virtual - Must Implement)
    // ===================================================================

    /**
     * @brief Get the number of rows under a parent
     * @param parent Parent index (invalid = root)
     * @return Number of rows
     *
     * @details
     * For flat lists: Returns total item count when parent is invalid.
     * For trees: Returns number of children when parent is valid.
     *
     * @par Example:
     * @code
     * int total_items = model->row_count();  // Root items
     * int num_children = model->row_count(parent_index);  // Children of parent
     * @endcode
     */
    [[nodiscard]] virtual int row_count(const model_index& parent = {}) const = 0;

    /**
     * @brief Get the number of columns under a parent
     * @param parent Parent index (invalid = root)
     * @return Number of columns
     *
     * @details
     * For lists: Always returns 1.
     * For tables: Returns number of columns.
     * For trees: Returns number of columns (usually 1).
     */
    [[nodiscard]] virtual int column_count(const model_index& parent = {}) const = 0;

    /**
     * @brief Create an index for a specific row/column
     * @param row Row number (0-based)
     * @param column Column number (0-based)
     * @param parent Parent index (invalid = root)
     * @return Model index, or invalid index if out of bounds
     *
     * @details
     * Creates a model_index that uniquely identifies an item.
     * Returns invalid index if row/column are out of range.
     *
     * @par Example:
     * @code
     * model_index idx = model->index(3, 0);  // Row 3, column 0
     * if (idx.is_valid()) {
     *     auto data = model->data(idx);
     * }
     * @endcode
     */
    [[nodiscard]] virtual model_index index(int row, int column, const model_index& parent = {}) const = 0;

    /**
     * @brief Get the parent of a child index
     * @param child Child index
     * @return Parent index, or invalid index if child is a root item
     *
     * @details
     * For lists/tables: Always returns invalid index (no hierarchy).
     * For trees: Returns the parent node's index.
     */
    [[nodiscard]] virtual model_index parent(const model_index& child) const = 0;

    /**
     * @brief Get data for an item and role
     * @param index Item index
     * @param role Data role (display, edit, tooltip, etc.)
     * @return Data as variant, or monostate if not available
     *
     * @details
     * Returns different aspects of the same item depending on role:
     * - display: User-visible text
     * - edit: Raw data for editing
     * - tooltip: Tooltip text
     * - background/foreground: Custom colors
     *
     * @par Example:
     * @code
     * auto data = model->data(index, item_data_role::display);
     * if (std::holds_alternative<std::string>(data)) {
     *     std::string text = std::get<std::string>(data);
     * }
     * @endcode
     */
    [[nodiscard]] virtual variant_type data(
        const model_index& index,
        item_data_role role = item_data_role::display
    ) const = 0;

    // ===================================================================
    // Data Modification (Optional - Override for Editable Models)
    // ===================================================================

    /**
     * @brief Set data for an item
     * @param index Item index
     * @param value New value
     * @param role Data role (usually edit)
     * @return true if data was set, false if read-only or invalid
     *
     * @details
     * Default implementation returns false (read-only model).
     * Editable models should override this and emit data_changed signal.
     *
     * @par Example:
     * @code
     * bool success = model->set_data(index, std::string("New Text"), item_data_role::edit);
     * // Model emits data_changed signal if successful
     * @endcode
     */
    virtual bool set_data(
        const model_index& index,
        const variant_type& value,
        item_data_role role = item_data_role::edit
    ) {
        (void)index;
        (void)value;
        (void)role;
        return false;  // Default: read-only
    }

    // ===================================================================
    // Item Properties
    // ===================================================================

    /**
     * @brief Get flags for an item
     * @param index Item index
     * @return Item flags (enabled, selectable, editable, etc.)
     *
     * @details
     * Default: enabled | selectable
     * Models can override to disable items, make them non-selectable, etc.
     *
     * @par Example:
     * @code
     * item_flags flags = model->flags(index);
     * bool is_editable = (flags & item_flag::editable) != 0;
     * @endcode
     */
    [[nodiscard]] virtual item_flags flags(const model_index& index) const {
        (void)index;
        return static_cast<item_flags>(item_flag::enabled) |
               static_cast<item_flags>(item_flag::selectable);
    }

    // ===================================================================
    // Sorting (Optional)
    // ===================================================================

    /**
     * @brief Sort model by column
     * @param column Column to sort by
     * @param order Sort order (ascending/descending)
     *
     * @details
     * Default: no-op (model doesn't support sorting).
     * Sortable models should override and emit layout_changed signal.
     *
     * @par Example:
     * @code
     * model->sort(0, sort_order::ascending);  // Sort by first column
     * // Model emits layout_changed signal
     * @endcode
     */
    virtual void sort(int column, sort_order order = sort_order::ascending) {
        (void)column;
        (void)order;
        // Default: no-op
    }

    // ===================================================================
    // Change Notification Signals
    // ===================================================================

    /**
     * @brief Emitted when item data changes
     * @param top_left Top-left index of changed region
     * @param bottom_right Bottom-right index of changed region
     *
     * @details
     * Views listen to this signal to repaint affected items.
     * For single item: top_left == bottom_right.
     */
    signal<model_index, model_index> data_changed;

    /**
     * @brief Emitted after rows are inserted
     * @param parent Parent index
     * @param first First inserted row
     * @param last Last inserted row
     *
     * @details
     * Views listen to this signal to update their layout.
     * Always preceded by rows_about_to_be_inserted.
     */
    signal<model_index, int, int> rows_inserted;

    /**
     * @brief Emitted after rows are removed
     * @param parent Parent index
     * @param first First removed row
     * @param last Last removed row
     *
     * @details
     * Views listen to this signal to update their layout.
     * Always preceded by rows_about_to_be_removed.
     */
    signal<model_index, int, int> rows_removed;

    /**
     * @brief Emitted before rows are inserted
     * @param parent Parent index
     * @param first First row to be inserted
     * @param last Last row to be inserted
     *
     * @details
     * Views listen to this signal to prepare for structural changes.
     * Always followed by rows_inserted (or end_insert_rows).
     */
    signal<model_index, int, int> rows_about_to_be_inserted;

    /**
     * @brief Emitted before rows are removed
     * @param parent Parent index
     * @param first First row to be removed
     * @param last Last row to be removed
     *
     * @details
     * Views listen to this signal to prepare for structural changes.
     * Always followed by rows_removed (or end_remove_rows).
     */
    signal<model_index, int, int> rows_about_to_be_removed;

    /**
     * @brief Emitted when model layout changes significantly
     *
     * @details
     * Used when:
     * - Model is sorted
     * - Large structural changes occur
     * - Simpler than tracking individual row changes
     *
     * Views respond by performing a full relayout.
     */
    signal<> layout_changed;

    /**
     * @brief Emitted before model layout changes
     *
     * @details
     * Allows views to save state (e.g., current selection) before
     * the layout changes. Always followed by layout_changed.
     */
    signal<> layout_about_to_be_changed;

protected:
    // ===================================================================
    // Helper Methods for Subclasses
    // ===================================================================

    /**
     * @brief Begin inserting rows
     * @param parent Parent index
     * @param first First row to insert
     * @param last Last row to insert
     *
     * @details
     * Call this BEFORE modifying the model's data structure.
     * Emits rows_about_to_be_inserted signal.
     * Must be paired with end_insert_rows().
     *
     * @par Example:
     * @code
     * begin_insert_rows({}, 5, 7);  // Inserting rows 5-7 at root
     * m_items.insert(...);          // Actually insert data
     * end_insert_rows();            // Notify views
     * @endcode
     */
    void begin_insert_rows(const model_index& parent, int first, int last) {
        rows_about_to_be_inserted.emit(parent, first, last);
    }

    /**
     * @brief End inserting rows
     *
     * @details
     * Call this AFTER modifying the model's data structure.
     * Must be paired with begin_insert_rows().
     */
    void end_insert_rows() {
        // Views handle rows_about_to_be_inserted to prepare for changes
        // No additional signal needed here
    }

    /**
     * @brief Begin removing rows
     * @param parent Parent index
     * @param first First row to remove
     * @param last Last row to remove
     *
     * @details
     * Call this BEFORE modifying the model's data structure.
     * Emits rows_about_to_be_removed signal.
     * Must be paired with end_remove_rows().
     *
     * @par Example:
     * @code
     * begin_remove_rows({}, 3, 5);  // Removing rows 3-5 at root
     * m_items.erase(...);           // Actually remove data
     * end_remove_rows();            // Notify views
     * @endcode
     */
    void begin_remove_rows(const model_index& parent, int first, int last) {
        rows_about_to_be_removed.emit(parent, first, last);
    }

    /**
     * @brief End removing rows
     *
     * @details
     * Call this AFTER modifying the model's data structure.
     * Must be paired with begin_remove_rows().
     */
    void end_remove_rows() {
        // Views handle rows_about_to_be_removed to prepare for changes
        // No additional signal needed here
    }

    /**
     * @brief Begin layout change
     *
     * @details
     * Call this before making large structural changes (e.g., sorting).
     * Emits layout_about_to_be_changed signal.
     * Must be paired with end_layout_change().
     */
    void begin_layout_change() {
        layout_about_to_be_changed.emit();
    }

    /**
     * @brief End layout change
     *
     * @details
     * Call this after making large structural changes.
     * Emits layout_changed signal.
     * Must be paired with begin_layout_change().
     */
    void end_layout_change() {
        layout_changed.emit();
    }
};

} // namespace onyxui
