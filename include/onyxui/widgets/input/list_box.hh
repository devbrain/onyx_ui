//
// OnyxUI - Simple List Box Widget
// Created: 2025-01-01
// Refactored: 2026-01-05 (inherits from list_view)
//

#pragma once

#include <memory>
#include <string>
#include <vector>

#include <onyxui/concepts/backend.hh>
#include <onyxui/core/signal.hh>
#include <onyxui/mvc/models/list_model.hh>
#include <onyxui/mvc/views/list_view.hh>
#include <onyxui/mvc/selection/item_selection_model.hh>

namespace onyxui {

/**
 * @brief A simple list box widget
 *
 * @tparam Backend The UI backend type
 *
 * @details
 * list_box is the easy-to-use list widget with a simple string-based API.
 * It inherits from list_view (privately) and owns a list_model,
 * exposing a simplified interface.
 *
 * For advanced use cases requiring custom models, delegates, or selection models,
 * use list_view directly instead.
 *
 * Features:
 * - **Simple API**: add_item(), remove_item(), clear(), set_items()
 * - **Self-Contained**: Owns its data internally
 * - **No MVC Knowledge Required**: Just use strings
 * - **Selection Modes**: Single or multi-selection support
 * - **Scrolling**: Inherits list_view scrolling support
 *
 * @par Example Usage:
 * @code
 * // Create list box with simple API
 * auto list = std::make_unique<list_box<Backend>>();
 * list->add_item("Apple");
 * list->add_item("Banana");
 * list->add_item("Cherry");
 * list->set_current_index(1);  // Select "Banana"
 *
 * // Or set all items at once
 * list->set_items({"Red", "Green", "Blue"});
 *
 * // Listen for selection changes
 * list->current_index_changed.connect([](int index) {
 *     std::cout << "Selected index: " << index << "\n";
 * });
 *
 * // Listen for item activation (double-click or Enter)
 * list->activated.connect([](int index) {
 *     std::cout << "Activated: " << index << "\n";
 * });
 * @endcode
 *
 * @see list_view for advanced MVC usage
 */
template<UIBackend Backend>
class list_box : private list_view<Backend> {
public:
    using base = list_view<Backend>;
    using model_type = list_model<std::string, Backend>;
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

    // Expose list_view scrolling methods
    using base::scroll_offset;
    using base::set_scroll_offset;

    /**
     * @brief Construct an empty list box
     */
    list_box()
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
                    current_index_changed.emit(row);
                    if (row >= 0) {
                        current_text_changed.emit(item_text(row));
                    }
                }
            );

            m_selection_changed_conn = scoped_connection(
                sel_model->selection_changed,
                [this](const std::vector<model_index>&, const std::vector<model_index>&) {
                    selection_changed.emit();
                }
            );

            m_selection_model = sel_model;
        }

        m_activated_conn = scoped_connection(
            base::activated,
            [this](const model_index& idx) {
                if (idx.is_valid()) {
                    activated.emit(idx.row);
                }
            }
        );

        m_clicked_conn = scoped_connection(
            base::clicked,
            [this](const model_index& idx) {
                if (idx.is_valid()) {
                    clicked.emit(idx.row);
                }
            }
        );

        m_double_clicked_conn = scoped_connection(
            base::double_clicked,
            [this](const model_index& idx) {
                if (idx.is_valid()) {
                    double_clicked.emit(idx.row);
                }
            }
        );
    }

    /**
     * @brief Construct with initial items
     * @param items Initial list of items
     */
    explicit list_box(std::vector<std::string> items)
        : list_box()
    {
        set_items(std::move(items));
    }

    // Non-copyable, movable
    list_box(const list_box&) = delete;
    list_box& operator=(const list_box&) = delete;
    list_box(list_box&&) noexcept = default;
    list_box& operator=(list_box&&) noexcept = default;

    // ===================================================================
    // Simple Item Management API
    // ===================================================================

    /**
     * @brief Add an item to the end of the list
     * @param text Display text for the item
     */
    void add_item(const std::string& text) {
        m_model->append(text);
    }

    /**
     * @brief Insert an item at a specific position
     * @param index Position to insert at (0-based)
     * @param text Display text for the item
     */
    void insert_item(int index, const std::string& text) {
        m_model->insert(index, text);
    }

    /**
     * @brief Remove an item at a specific position
     * @param index Position to remove (0-based)
     */
    void remove_item(int index) {
        if (index < 0 || index >= count()) {
            return;
        }

        // Remember current selection for adjustment
        int current = current_index();

        // Remove from model first
        m_model->remove(index);

        // Adjust selection based on what was removed
        if (current == index) {
            // Removed the selected item - clear selection
            base::set_current_index(model_index{});
        } else if (current > index) {
            // Selection was after removed item - adjust index
            base::set_current_index(m_model->index(current - 1, 0));
        }
    }

    /**
     * @brief Remove all items
     */
    void clear() {
        // Clear selection first
        base::set_current_index(model_index{});
        m_model->clear();
    }

    /**
     * @brief Replace all items with a new list
     * @param items New list of items
     */
    void set_items(std::vector<std::string> items) {
        m_model->set_items(std::move(items));
    }

    /**
     * @brief Get the number of items
     * @return Item count
     */
    [[nodiscard]] int count() const noexcept {
        return m_model->row_count();
    }

    /**
     * @brief Get item text at a specific index
     * @param index Item index (0-based)
     * @return Display text, or empty string if invalid index
     */
    [[nodiscard]] std::string item_text(int index) const {
        if (index < 0 || index >= count()) {
            return {};
        }
        auto data = m_model->data(m_model->index(index, 0), item_data_role::display);
        if (std::holds_alternative<std::string>(data)) {
            return std::get<std::string>(data);
        }
        return {};
    }

    /**
     * @brief Set item text at a specific index
     * @param index Item index (0-based)
     * @param text New display text
     */
    void set_item_text(int index, const std::string& text) {
        if (index < 0 || index >= count()) {
            return;
        }
        m_model->set_data(
            m_model->index(index, 0),
            std::any(text),
            item_data_role::edit
        );
    }

    /**
     * @brief Find an item by text
     * @param text Text to search for
     * @return Index of first matching item, or -1 if not found
     */
    [[nodiscard]] int find_text(const std::string& text) const {
        for (int i = 0; i < count(); ++i) {
            if (item_text(i) == text) {
                return i;
            }
        }
        return -1;
    }

    // ===================================================================
    // Selection Management
    // ===================================================================

    /**
     * @brief Get the currently selected index
     * @return Current row index, or -1 if no selection
     */
    [[nodiscard]] int current_index() const noexcept {
        auto idx = base::current_index();
        return idx.is_valid() ? idx.row : -1;
    }

    /**
     * @brief Set the current selection
     * @param index Row index to select, or -1 for no selection
     *
     * @details
     * In single-selection mode, this also selects the item.
     * In multi-selection mode, use select() to add to selection.
     */
    void set_current_index(int index) {
        if (index < 0 || index >= count()) {
            // Clear current and selection
            if (m_selection_model) {
                m_selection_model->set_current_index(model_index{});
                m_selection_model->clear_selection();
            }
        } else {
            auto idx = m_model->index(index, 0);
            // Set current and select in single-selection mode
            if (m_selection_model) {
                m_selection_model->set_current_index(idx);
                // In single selection, also select the current item
                if (m_selection_model->get_selection_mode() == selection_mode::single_selection) {
                    m_selection_model->select(idx, selection_flag::clear | selection_flag::select);
                }
            }
        }
    }

    /**
     * @brief Get the current selection text
     * @return Display text of current item, or empty if no selection
     */
    [[nodiscard]] std::string current_text() const {
        int idx = current_index();
        return idx >= 0 ? item_text(idx) : std::string{};
    }

    /**
     * @brief Set selection by text
     * @param text Text to select
     * @return true if item was found and selected, false otherwise
     */
    bool set_current_text(const std::string& text) {
        int index = find_text(text);
        if (index >= 0) {
            set_current_index(index);
            return true;
        }
        return false;
    }

    /**
     * @brief Check if an item is selected
     * @param index Item index
     * @return true if item is selected
     */
    [[nodiscard]] bool is_selected(int index) const {
        if (!m_selection_model || index < 0 || index >= count()) {
            return false;
        }
        return m_selection_model->is_selected(m_model->index(index, 0));
    }

    /**
     * @brief Get all selected indices
     * @return Vector of selected row indices
     */
    [[nodiscard]] std::vector<int> selected_indices() const {
        std::vector<int> result;
        if (!m_selection_model) return result;

        for (const auto& idx : m_selection_model->selected_indices()) {
            if (idx.is_valid()) {
                result.push_back(idx.row);
            }
        }
        return result;
    }

    /**
     * @brief Select an item (add to selection in multi-select mode)
     * @param index Item index
     */
    void select(int index) {
        if (!m_selection_model || index < 0 || index >= count()) {
            return;
        }
        m_selection_model->select(m_model->index(index, 0));
    }

    /**
     * @brief Deselect an item
     * @param index Item index
     */
    void deselect(int index) {
        if (!m_selection_model || index < 0 || index >= count()) {
            return;
        }
        m_selection_model->deselect(m_model->index(index, 0));
    }

    /**
     * @brief Clear all selections
     */
    void clear_selection() {
        if (m_selection_model) {
            m_selection_model->clear_selection();
        }
    }

    /**
     * @brief Select all items
     */
    void select_all() {
        if (!m_selection_model) return;

        for (int i = 0; i < count(); ++i) {
            m_selection_model->select(m_model->index(i, 0));
        }
    }

    // ===================================================================
    // Selection Mode
    // ===================================================================

    /**
     * @brief Set selection mode
     * @param mode Selection mode (single or multi)
     */
    void set_selection_mode(selection_mode mode) {
        if (m_selection_model) {
            m_selection_model->set_selection_mode(mode);
        }
    }

    /**
     * @brief Get current selection mode
     */
    [[nodiscard]] selection_mode get_selection_mode() const {
        return m_selection_model ? m_selection_model->get_selection_mode()
                                  : selection_mode::single_selection;
    }

    // ===================================================================
    // Scrolling
    // ===================================================================

    /**
     * @brief Scroll to make an item visible
     * @param index Item index to scroll to
     */
    void scroll_to(int index) {
        if (index < 0 || index >= count()) {
            return;
        }
        base::scroll_to(m_model->index(index, 0));
    }

    // ===================================================================
    // Advanced Access (for power users)
    // ===================================================================

    /**
     * @brief Get the internal model (read-only)
     * @return Pointer to internal list_model
     */
    [[nodiscard]] const model_type* model() const noexcept {
        return m_model.get();
    }

    /**
     * @brief Get the internal view
     * @return Pointer to internal list_view (this)
     */
    [[nodiscard]] list_view<Backend>* view() noexcept {
        return this;
    }

    [[nodiscard]] const list_view<Backend>* view() const noexcept {
        return this;
    }

    // ===================================================================
    // Signals
    // ===================================================================

    /**
     * @brief Emitted when the current (focused) item changes
     * @param index New current index (-1 if no current)
     */
    signal<int> current_index_changed;

    /**
     * @brief Emitted when the current text changes
     * @param text New current text
     */
    signal<const std::string&> current_text_changed;

    /**
     * @brief Emitted when selection changes (any item selected/deselected)
     */
    signal<> selection_changed;

    /**
     * @brief Emitted when an item is activated (double-click or Enter)
     * @param index Activated item index
     */
    signal<int> activated;

    /**
     * @brief Emitted when an item is clicked
     * @param index Clicked item index
     */
    signal<int> clicked;

    /**
     * @brief Emitted when an item is double-clicked
     * @param index Double-clicked item index
     */
    signal<int> double_clicked;

private:
    std::shared_ptr<model_type> m_model;        ///< Internal string list model (owned)
    selection_model_type* m_selection_model = nullptr;  ///< Selection model from view

    // Signal connections
    scoped_connection m_current_changed_conn;
    scoped_connection m_selection_changed_conn;
    scoped_connection m_activated_conn;
    scoped_connection m_clicked_conn;
    scoped_connection m_double_clicked_conn;
};

} // namespace onyxui
