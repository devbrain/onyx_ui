//
// OnyxUI MVC System - Item Selection Model
// Created: 2025-11-22
//

#pragma once

#include <unordered_set>
#include <vector>

#include <onyxui/core/signal.hh>
#include <onyxui/mvc/model_index.hh>

namespace onyxui {

// Forward declarations
template<UIBackend Backend>
class abstract_item_model;

/**
 * @brief Selection modes for item views
 *
 * @details
 * Defines how users can select items in a view.
 */
enum class selection_mode : std::uint8_t {
    /**
     * @brief No items can be selected
     *
     * @details
     * Useful for read-only views where selection is not needed.
     * Keyboard navigation still works (current item changes).
     */
    no_selection,

    /**
     * @brief Only one item can be selected at a time
     *
     * @details
     * Clicking an item deselects the previous selection.
     * This is the most common mode for simple lists.
     *
     * @par Behavior:
     * - Click: Select clicked item, deselect others
     * - Keyboard: Arrow keys select new item
     */
    single_selection,

    /**
     * @brief Multiple items can be selected independently
     *
     * @details
     * Users can select/deselect items independently.
     * Each click toggles the item's selection state.
     *
     * @par Behavior:
     * - Click: Toggle clicked item (others unchanged)
     * - Keyboard: Arrow keys move focus, Space toggles selection
     */
    multi_selection,

    /**
     * @brief Extended multi-selection with modifiers
     *
     * @details
     * Standard desktop multi-selection behavior:
     * - Click: Select single item (like single_selection)
     * - Ctrl+Click: Toggle item (add/remove from selection)
     * - Shift+Click: Select range from anchor to clicked
     *
     * @par Behavior:
     * - Click: Select clicked item, clear others
     * - Ctrl+Click: Toggle clicked item
     * - Shift+Click: Select range
     * - Keyboard: Arrow keys move selection (Shift extends, Ctrl moves focus only)
     *
     * This is the most powerful mode, used in file managers, etc.
     */
    extended_selection,

    /**
     * @brief Multi-selection restricted to contiguous ranges
     *
     * @details
     * Like extended_selection but only allows selecting contiguous ranges.
     * Useful when operations require consecutive items.
     *
     * @par Behavior:
     * - Click: Select clicked item, clear others
     * - Shift+Click: Select range (deselects non-contiguous items)
     * - Ctrl+Click: Not supported (acts like regular click)
     */
    contiguous_selection
};

/**
 * @brief Selection command flags
 *
 * @details
 * Commands can be combined to specify selection operations.
 * Used internally by item_selection_model.
 */
enum class selection_flag : std::uint8_t {
    no_update   = 0,       ///< Do nothing
    clear       = 1 << 0,  ///< Clear existing selection
    select      = 1 << 1,  ///< Select specified items
    deselect    = 1 << 2,  ///< Deselect specified items
    toggle      = 1 << 3,  ///< Toggle selection state
    current     = 1 << 4   ///< Update current item
};

/// Type alias for combined selection flags
using selection_flags = std::uint8_t;

/// Bitwise OR for selection flags
inline constexpr selection_flags operator|(selection_flag lhs, selection_flag rhs) noexcept {
    return static_cast<selection_flags>(static_cast<std::uint8_t>(lhs) | static_cast<std::uint8_t>(rhs));
}

/// Bitwise AND for selection flags
inline constexpr bool operator&(selection_flags lhs, selection_flag rhs) noexcept {
    return (lhs & static_cast<std::uint8_t>(rhs)) != 0;
}

/**
 * @brief Manages item selection for views
 *
 * @details
 * item_selection_model tracks:
 * - Which items are selected
 * - Which item is current (has keyboard focus)
 * - Selection mode (single, multi, extended, etc.)
 *
 * Views use this to implement selection behavior without duplicating logic.
 * Multiple views can share the same selection model for synchronized selection.
 *
 * @par Thread Safety:
 * NOT thread-safe. Access only from UI thread.
 *
 * @par Example Usage:
 * @code
 * // Create selection model
 * auto selection = std::make_unique<item_selection_model>();
 * selection->set_selection_mode(selection_mode::single_selection);
 *
 * // Connect to view
 * view->set_selection_model(selection.get());
 *
 * // Listen for changes
 * selection->selection_changed.connect([](auto& selected, auto& deselected) {
 *     std::cout << "Selection changed!\n";
 * });
 *
 * // Programmatically select items
 * selection->select(model->index(3, 0));
 * @endcode
 */
template<UIBackend Backend>
class item_selection_model {
public:
    /**
     * @brief Construct selection model with optional mode
     * @param mode Initial selection mode (default: single_selection)
     */
    explicit item_selection_model(selection_mode mode = selection_mode::single_selection)
        : m_selection_mode(mode) {}

    // ===================================================================
    // Model Management
    // ===================================================================

    /**
     * @brief Set the model being used for selection
     * @param model Pointer to the model
     *
     * @details
     * When the model changes, all selections are cleared.
     * The model pointer is not owned by the selection model.
     */
    void set_model(const abstract_item_model<Backend>* model) {
        if (m_model != model) {
            clear_selection();
            m_current_index = {};
            m_model = model;
        }
    }

    /**
     * @brief Get the current model
     * @return Pointer to model, or nullptr if not set
     */
    [[nodiscard]] const abstract_item_model<Backend>* model() const noexcept {
        return m_model;
    }

    // ===================================================================
    // Selection Mode
    // ===================================================================

    /**
     * @brief Get current selection mode
     * @return Current selection mode
     */
    [[nodiscard]] selection_mode get_selection_mode() const noexcept {
        return m_selection_mode;
    }

    /**
     * @brief Set selection mode
     * @param mode New selection mode
     *
     * @details
     * Changing the mode does NOT clear existing selection.
     * Call clear_selection() first if desired.
     */
    void set_selection_mode(selection_mode mode) {
        m_selection_mode = mode;
    }

    // ===================================================================
    // Current Item (Focus)
    // ===================================================================

    /**
     * @brief Get current (focused) item
     * @return Current index, or invalid if no current item
     *
     * @details
     * The current item is the one with keyboard focus.
     * It may or may not be selected.
     */
    [[nodiscard]] const model_index& current_index() const noexcept {
        return m_current_index;
    }

    /**
     * @brief Set current (focused) item
     * @param index New current index
     *
     * @details
     * Emits current_changed signal.
     * Does NOT change selection (use select() for that).
     */
    void set_current_index(const model_index& index) {
        if (m_current_index == index) {
            return;  // No change
        }

        model_index previous = m_current_index;
        m_current_index = index;

        current_changed.emit(m_current_index, previous);
    }

    // ===================================================================
    // Selection Queries
    // ===================================================================

    /**
     * @brief Check if an item is selected
     * @param index Item to check
     * @return true if selected, false otherwise
     */
    [[nodiscard]] bool is_selected(const model_index& index) const {
        return index.is_valid() && m_selected_indices.count(index) > 0;
    }

    /**
     * @brief Get all selected items
     * @return Vector of selected indices
     *
     * @details
     * Order is unspecified (set ordering).
     * For sorted order, caller must sort by row/column.
     */
    [[nodiscard]] std::vector<model_index> selected_indices() const {
        return std::vector<model_index>(m_selected_indices.begin(), m_selected_indices.end());
    }

    /**
     * @brief Check if selection is empty
     * @return true if no items selected, false otherwise
     */
    [[nodiscard]] bool has_selection() const noexcept {
        return !m_selected_indices.empty();
    }

    // ===================================================================
    // Selection Modification
    // ===================================================================

    /**
     * @brief Select an item
     * @param index Item to select
     * @param command Selection command flags
     *
     * @details
     * Behavior depends on selection mode:
     * - no_selection: No-op
     * - single_selection: Clears others, selects this one
     * - multi_selection: Adds to selection
     * - extended_selection: Respects command flags
     *
     * Emits selection_changed signal if selection changed.
     *
     * @par Command Flags:
     * - clear: Clear existing selection first
     * - select: Select the item
     * - deselect: Deselect the item
     * - toggle: Toggle selection state
     * - current: Also set as current item
     */
    void select(const model_index& index, selection_flags command = static_cast<selection_flags>(selection_flag::select)) {
        if (!index.is_valid()) {
            return;
        }

        if (m_selection_mode == selection_mode::no_selection) {
            return;  // No selection allowed
        }

        // Track changes
        std::vector<model_index> newly_selected;
        std::vector<model_index> newly_deselected;

        // Handle clear command
        if (command & selection_flag::clear) {
            if (!m_selected_indices.empty()) {
                newly_deselected = selected_indices();
                m_selected_indices.clear();
            }
        }

        // Single selection mode always clears others
        if (m_selection_mode == selection_mode::single_selection) {
            if (!m_selected_indices.empty() && m_selected_indices.count(index) == 0) {
                newly_deselected = selected_indices();
                m_selected_indices.clear();
            }
        }

        // Handle select/deselect/toggle
        if (command & selection_flag::toggle) {
            if (is_selected(index)) {
                m_selected_indices.erase(index);
                newly_deselected.push_back(index);
            } else {
                m_selected_indices.insert(index);
                newly_selected.push_back(index);
            }
        } else if (command & selection_flag::select) {
            if (m_selected_indices.insert(index).second) {
                newly_selected.push_back(index);
            }
        } else if (command & selection_flag::deselect) {
            if (m_selected_indices.erase(index) > 0) {
                newly_deselected.push_back(index);
            }
        }

        // Handle current command
        if (command & selection_flag::current) {
            set_current_index(index);
        }

        // Emit signal if selection changed
        if (!newly_selected.empty() || !newly_deselected.empty()) {
            selection_changed.emit(newly_selected, newly_deselected);
        }
    }

    /**
     * @brief Deselect an item
     * @param index Item to deselect
     */
    void deselect(const model_index& index) {
        select(index, static_cast<selection_flags>(selection_flag::deselect));
    }

    /**
     * @brief Toggle selection of an item
     * @param index Item to toggle
     */
    void toggle(const model_index& index) {
        select(index, static_cast<selection_flags>(selection_flag::toggle));
    }

    /**
     * @brief Clear all selections
     *
     * @details
     * Emits selection_changed signal if selection was not empty.
     * Does NOT change current item.
     */
    void clear_selection() {
        if (m_selected_indices.empty()) {
            return;  // Already empty
        }

        std::vector<model_index> deselected = selected_indices();
        m_selected_indices.clear();

        selection_changed.emit({}, deselected);
    }

    /**
     * @brief Select all items in a range
     * @param top_left Top-left index of range
     * @param bottom_right Bottom-right index of range
     *
     * @details
     * Selects all items from top_left to bottom_right (inclusive).
     * Useful for Shift+Click range selection.
     *
     * @par Behavior:
     * - In single_selection: Selects only bottom_right
     * - In multi_selection: Selects entire range
     * - In extended_selection: Selects range (respects mode)
     *
     * Emits selection_changed signal.
     */
    void select_range(const model_index& top_left, const model_index& bottom_right) {
        if (!top_left.is_valid() || !bottom_right.is_valid()) {
            return;
        }

        if (m_selection_mode == selection_mode::no_selection) {
            return;
        }

        if (m_selection_mode == selection_mode::single_selection) {
            // Single selection: only select the last item
            select(bottom_right);
            return;
        }

        // Multi-selection: select all items in range
        std::vector<model_index> newly_selected;

        int start_row = top_left.row < bottom_right.row ? top_left.row : bottom_right.row;
        int end_row = top_left.row > bottom_right.row ? top_left.row : bottom_right.row;

        for (int row = start_row; row <= end_row; ++row) {
            model_index idx{row, 0, nullptr, top_left.model};
            if (m_selected_indices.insert(idx).second) {
                newly_selected.push_back(idx);
            }
        }

        if (!newly_selected.empty()) {
            selection_changed.emit(newly_selected, {});
        }
    }

    // ===================================================================
    // Signals
    // ===================================================================

    /**
     * @brief Emitted when selection changes
     * @param selected Newly selected items
     * @param deselected Newly deselected items
     *
     * @details
     * Views listen to this to update their rendering.
     * Only items that changed state are included.
     */
    signal<const std::vector<model_index>&, const std::vector<model_index>&> selection_changed;

    /**
     * @brief Emitted when current item changes
     * @param current New current index
     * @param previous Previous current index
     *
     * @details
     * Views listen to this to update focus rectangle.
     */
    signal<const model_index&, const model_index&> current_changed;

private:
    const abstract_item_model<Backend>* m_model = nullptr;  // Model being selected from
    selection_mode m_selection_mode;
    model_index m_current_index;  // Current (focused) item
    std::unordered_set<model_index> m_selected_indices;  // Selected items
};

} // namespace onyxui
