//
// OnyxUI MVC System - Model Index
// Created: 2025-11-22
//

#pragma once

#include <cstddef>
#include <functional>  // For std::hash

namespace onyxui {

/**
 * @brief Uniquely identifies an item in a model
 *
 * @details
 * A model_index represents a specific item within a model's data structure.
 * It contains:
 * - row/column coordinates (0-based)
 * - internal_id for model-specific identification (e.g., tree node pointer)
 * - model pointer for validity checks and data access
 *
 * Model indices are lightweight value types that can be copied freely.
 * They remain valid until the model's structure changes (row insertion/deletion).
 *
 * @par Example Usage:
 * @code
 * // Get index for row 3, column 0
 * model_index idx = model->index(3, 0);
 *
 * // Check validity
 * if (idx.is_valid()) {
 *     auto data = model->data(idx, item_data_role::display);
 * }
 * @endcode
 */
struct model_index {
    int row = -1;              ///< Row number (0-based), -1 = invalid
    int column = -1;           ///< Column number (0-based), -1 = invalid
    void* internal_id = nullptr; ///< Model-specific identifier (e.g., tree node)
    const void* model = nullptr; ///< Pointer to owning model (for validity checks)

    /**
     * @brief Check if this index is valid
     * @return true if the index points to a valid item, false otherwise
     *
     * @details
     * An index is valid if:
     * - row >= 0 AND
     * - column >= 0 AND
     * - model != nullptr
     *
     * Invalid indices (default constructed or invalidated) return false.
     */
    [[nodiscard]] constexpr bool is_valid() const noexcept {
        return row >= 0 && column >= 0 && model != nullptr;
    }

    /**
     * @brief Default construct an invalid index
     */
    constexpr model_index() noexcept = default;

    /**
     * @brief Construct a model index
     * @param r Row number
     * @param c Column number
     * @param id Model-specific internal identifier
     * @param m Pointer to owning model
     */
    constexpr model_index(int r, int c, void* id, const void* m) noexcept
        : row(r), column(c), internal_id(id), model(m) {}
};

/**
 * @brief Equality comparison for model indices
 * @param lhs Left-hand side index
 * @param rhs Right-hand side index
 * @return true if indices refer to the same item in the same model
 *
 * @details
 * Two indices are equal if ALL of the following match:
 * - Same row
 * - Same column
 * - Same model pointer
 * - Same internal_id
 */
inline constexpr bool operator==(const model_index& lhs, const model_index& rhs) noexcept {
    return lhs.row == rhs.row
        && lhs.column == rhs.column
        && lhs.model == rhs.model
        && lhs.internal_id == rhs.internal_id;
}

/**
 * @brief Inequality comparison for model indices
 */
inline constexpr bool operator!=(const model_index& lhs, const model_index& rhs) noexcept {
    return !(lhs == rhs);
}

} // namespace onyxui

// Hash function for model_index (for use in std::unordered_set, std::unordered_map)
namespace std {
    template<>
    struct hash<onyxui::model_index> {
        std::size_t operator()(const onyxui::model_index& idx) const noexcept {
            std::size_t h1 = std::hash<int>{}(idx.row);
            std::size_t h2 = std::hash<int>{}(idx.column);
            std::size_t h3 = std::hash<const void*>{}(idx.model);
            std::size_t h4 = std::hash<void*>{}(idx.internal_id);
            return h1 ^ (h2 << 1) ^ (h3 << 2) ^ (h4 << 3);
        }
    };
}
