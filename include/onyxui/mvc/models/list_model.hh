//
// OnyxUI MVC System - List Model
// Created: 2025-11-22
//

#pragma once

#include <sstream>
#include <string>
#include <vector>

#include <onyxui/mvc/models/abstract_item_model.hh>

namespace onyxui {

/**
 * @brief A simple list model for linear data
 *
 * @tparam T The item type (std::string, int, custom struct, etc.)
 * @tparam Backend The UI backend type
 *
 * @details
 * list_model provides a simple, type-safe wrapper around std::vector<T>
 * that integrates with the MVC system. It automatically:
 * - Converts T to string for display
 * - Emits change signals when data is modified
 * - Provides type-safe access to underlying items
 *
 * @par Supported Item Types:
 * - std::string: Used as-is
 * - Arithmetic types (int, double, etc.): Converted via std::to_string
 * - Custom types: Require operator<< or specialization of to_string_impl
 *
 * @par Example Usage:
 * @code
 * // String list
 * auto model = std::make_shared<list_model<std::string, Backend>>();
 * model->set_items({"Apple", "Banana", "Cherry"});
 *
 * // Integer list
 * auto numbers = std::make_shared<list_model<int, Backend>>();
 * numbers->set_items({1, 2, 3, 4, 5});
 *
 * // Custom struct
 * struct Person { std::string name; int age; };
 * auto people = std::make_shared<list_model<Person, Backend>>();
 * people->append({"Alice", 30});
 * people->append({"Bob", 25});
 * @endcode
 *
 * @par Thread Safety:
 * NOT thread-safe. Access only from UI thread.
 */
template<typename T, UIBackend Backend>
class list_model : public abstract_item_model<Backend> {
public:
    using value_type = T;
    using container_type = std::vector<T>;
    using base = abstract_item_model<Backend>;

    // ===================================================================
    // Constructors
    // ===================================================================

    /**
     * @brief Construct an empty list model
     */
    list_model() = default;

    /**
     * @brief Construct with initial items
     * @param items Initial list of items
     *
     * @par Example:
     * @code
     * list_model<std::string, Backend> model({"A", "B", "C"});
     * @endcode
     */
    explicit list_model(container_type items)
        : m_items(std::move(items)) {}

    // ===================================================================
    // Data Access (Override from abstract_item_model)
    // ===================================================================

    [[nodiscard]] int row_count(const model_index& parent = {}) const override {
        if (parent.is_valid()) {
            return 0;  // Lists have no children
        }
        return static_cast<int>(m_items.size());
    }

    [[nodiscard]] int column_count(const model_index& parent = {}) const override {
        (void)parent;
        return 1;  // Lists have single column
    }

    [[nodiscard]] model_index index(int row, int column, const model_index& parent = {}) const override {
        // Lists don't support hierarchy
        if (parent.is_valid()) {
            return {};  // Invalid
        }

        // Validate row/column
        if (row < 0 || row >= static_cast<int>(m_items.size()) || column != 0) {
            return {};  // Invalid
        }

        // Create valid index
        return {row, column, nullptr, this};
    }

    [[nodiscard]] model_index parent(const model_index& child) const override {
        (void)child;
        return {};  // Lists have no parent (flat structure)
    }

    [[nodiscard]] typename base::variant_type data(
        const model_index& index,
        item_data_role role = item_data_role::display
    ) const override {
        // Validate index
        if (!index.is_valid() || index.row < 0 || index.row >= static_cast<int>(m_items.size())) {
            return std::monostate{};
        }

        const T& item = m_items[static_cast<std::size_t>(index.row)];

        // Handle display and edit roles
        if (role == item_data_role::display) {
            return to_string_impl(item);
        }

        if (role == item_data_role::edit) {
            // For edit role, return the raw item wrapped in std::any
            return std::any(item);
        }

        // Other roles not supported by default
        return std::monostate{};
    }

    bool set_data(
        const model_index& index,
        const typename base::variant_type& value,
        item_data_role role = item_data_role::edit
    ) override {
        // Validate index
        if (!index.is_valid() || index.row < 0 || index.row >= static_cast<int>(m_items.size())) {
            return false;
        }

        if (role != item_data_role::edit) {
            return false;  // Only support editing the actual data
        }

        // Try to extract T from variant
        if (std::holds_alternative<std::any>(value)) {
            const std::any& any_value = std::get<std::any>(value);
            try {
                m_items[static_cast<std::size_t>(index.row)] = std::any_cast<T>(any_value);
                this->data_changed.emit(index, index);
                return true;
            } catch (const std::bad_any_cast&) {
                return false;  // Wrong type
            }
        }

        return false;
    }

    // ===================================================================
    // List-Specific Operations
    // ===================================================================

    /**
     * @brief Replace all items in the model
     * @param items New list of items
     *
     * @details
     * Efficiently replaces all items by:
     * 1. Removing old items (emits rows_removed)
     * 2. Adding new items (emits rows_inserted)
     *
     * Views automatically update when signals are emitted.
     *
     * @par Example:
     * @code
     * model->set_items({"New", "List", "Of", "Items"});
     * @endcode
     */
    void set_items(container_type items) {
        // Remove old items
        if (!m_items.empty()) {
            int const last_row = static_cast<int>(m_items.size()) - 1;  // Store before clear
            this->begin_remove_rows({}, 0, last_row);
            m_items.clear();
            this->end_remove_rows();
            this->rows_removed.emit({}, 0, last_row);
        }

        // Add new items
        if (!items.empty()) {
            int const last_row = static_cast<int>(items.size()) - 1;
            this->begin_insert_rows({}, 0, last_row);
            m_items = std::move(items);
            this->end_insert_rows();
            this->rows_inserted.emit({}, 0, last_row);
        }
    }

    /**
     * @brief Append an item to the end of the list
     * @param item Item to append
     *
     * @par Example:
     * @code
     * model->append("New Item");
     * @endcode
     */
    void append(const T& item) {
        int row = static_cast<int>(m_items.size());
        this->begin_insert_rows({}, row, row);
        m_items.push_back(item);
        this->end_insert_rows();
        this->rows_inserted.emit({}, row, row);
    }

    /**
     * @brief Append an item to the end of the list (move version)
     * @param item Item to append (moved)
     */
    void append(T&& item) {
        int row = static_cast<int>(m_items.size());
        this->begin_insert_rows({}, row, row);
        m_items.push_back(std::move(item));
        this->end_insert_rows();
        this->rows_inserted.emit({}, row, row);
    }

    /**
     * @brief Insert an item at a specific position
     * @param row Row number (0-based)
     * @param item Item to insert
     *
     * @par Example:
     * @code
     * model->insert(0, "First Item");  // Insert at beginning
     * model->insert(2, "Third Item");  // Insert at position 2
     * @endcode
     */
    void insert(int row, const T& item) {
        if (row < 0 || row > static_cast<int>(m_items.size())) {
            return;  // Out of range
        }

        this->begin_insert_rows({}, row, row);
        m_items.insert(m_items.begin() + row, item);
        this->end_insert_rows();
        this->rows_inserted.emit({}, row, row);
    }

    /**
     * @brief Remove an item at a specific position
     * @param row Row number (0-based)
     *
     * @par Example:
     * @code
     * model->remove(0);  // Remove first item
     * model->remove(model->row_count() - 1);  // Remove last item
     * @endcode
     */
    void remove(int row) {
        if (row < 0 || row >= static_cast<int>(m_items.size())) {
            return;  // Out of range
        }

        this->begin_remove_rows({}, row, row);
        m_items.erase(m_items.begin() + row);
        this->end_remove_rows();
        this->rows_removed.emit({}, row, row);
    }

    /**
     * @brief Clear all items from the model
     *
     * @par Example:
     * @code
     * model->clear();
     * @endcode
     */
    void clear() {
        if (m_items.empty()) {
            return;  // Already empty
        }

        // Store count BEFORE clearing for correct signal emission
        int const last_row = static_cast<int>(m_items.size()) - 1;

        this->begin_remove_rows({}, 0, last_row);
        m_items.clear();
        this->end_remove_rows();
        this->rows_removed.emit({}, 0, last_row);
    }

    // ===================================================================
    // Direct Access (for advanced use cases)
    // ===================================================================

    /**
     * @brief Get read-only access to underlying container
     * @return Const reference to std::vector<T>
     *
     * @details
     * Use this for direct iteration or algorithms on the data.
     * DO NOT modify the returned container directly - use set_items(), append(), etc.
     *
     * @par Example:
     * @code
     * for (const auto& item : model->items()) {
     *     std::cout << item << "\n";
     * }
     * @endcode
     */
    [[nodiscard]] const container_type& items() const noexcept {
        return m_items;
    }

    /**
     * @brief Get item at specific row
     * @param row Row number (0-based)
     * @return Const reference to item
     * @throws std::out_of_range if row is invalid
     *
     * @par Example:
     * @code
     * const std::string& item = model->at(2);
     * @endcode
     */
    [[nodiscard]] const T& at(int row) const {
        return m_items.at(static_cast<std::size_t>(row));
    }

private:
    container_type m_items;

    // ===================================================================
    // Type Conversion for Display
    // ===================================================================

    /**
     * @brief Convert T to string for display role
     * @param value Value to convert
     * @return String representation
     *
     * @details
     * Conversion rules:
     * - std::string: Return as-is
     * - Arithmetic types: Use std::to_string
     * - Custom types: Use operator<< (must be defined)
     *
     * @par Specialization:
     * For custom types without operator<<, specialize this function:
     * @code
     * template<>
     * std::string list_model<Person, Backend>::to_string_impl(const Person& p) {
     *     return p.name + " (" + std::to_string(p.age) + ")";
     * }
     * @endcode
     */
    static std::string to_string_impl(const T& value) {
        if constexpr (std::is_same_v<T, std::string>) {
            return value;
        } else if constexpr (std::is_arithmetic_v<T>) {
            return std::to_string(value);
        } else {
            // Custom types must have operator<<
            std::ostringstream oss;
            oss << value;
            return oss.str();
        }
    }
};

} // namespace onyxui
