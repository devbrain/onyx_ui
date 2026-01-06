//
// OnyxUI - Simple Combo Box Widget
// Created: 2025-11-22
// Refactored: 2026-01-05 (inherits from combo_box_view)
//

#pragma once

#include <memory>
#include <string>
#include <vector>

#include <onyxui/concepts/backend.hh>
#include <onyxui/core/signal.hh>
#include <onyxui/mvc/models/list_model.hh>
#include <onyxui/mvc/views/combo_box_view.hh>

namespace onyxui {

/**
 * @brief A simple dropdown combo box widget
 *
 * @tparam Backend The UI backend type
 *
 * @details
 * combo_box is the easy-to-use combo box widget with a simple string-based API.
 * It inherits from combo_box_view (privately) and owns a list_model,
 * exposing a simplified interface.
 *
 * For advanced use cases requiring custom models, delegates, or selection models,
 * use combo_box_view directly instead.
 *
 * Features:
 * - **Simple API**: add_item(), remove_item(), clear(), set_items()
 * - **Self-Contained**: Owns its data internally
 * - **No MVC Knowledge Required**: Just use strings
 * - **Full Functionality**: Inherits all combo_box_view features
 *
 * @par Example Usage:
 * @code
 * // Create combo box with simple API
 * auto combo = std::make_unique<combo_box<Backend>>();
 * combo->add_item("Small");
 * combo->add_item("Medium");
 * combo->add_item("Large");
 * combo->set_current_index(1);  // Select "Medium"
 *
 * // Or set all items at once
 * combo->set_items({"Apple", "Banana", "Cherry"});
 *
 * // Listen for selection changes
 * combo->current_index_changed.connect([](int index) {
 *     std::cout << "Selected index: " << index << "\n";
 * });
 * @endcode
 *
 * @see combo_box_view for advanced MVC usage
 */
template<UIBackend Backend>
class combo_box : public combo_box_view<Backend> {
public:
    using base = combo_box_view<Backend>;
    using model_type = list_model<std::string, Backend>;

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

    // Expose combo_box_view popup methods
    using base::is_popup_visible;
    using base::show_popup;
    using base::hide_popup;

    // Expose combo_box_view selection methods
    using base::current_row;
    using base::set_current_row;
    using base::current_text;

    /**
     * @brief Construct an empty combo box
     */
    combo_box()
        : m_model(std::make_shared<model_type>())
    {
        // Configure the view with our internal model
        base::set_model(m_model.get());

        // Forward signals from base view to our simple signals
        m_current_changed_conn = scoped_connection(
            base::current_changed,
            [this](const model_index& idx) {
                int row = idx.is_valid() ? idx.row : -1;
                current_index_changed.emit(row);
                if (row >= 0) {
                    current_text_changed.emit(item_text(row));
                }
            }
        );

        m_activated_conn = scoped_connection(
            base::activated,
            [this](const model_index& idx) {
                if (idx.is_valid()) {
                    activated.emit(idx.row);
                }
            }
        );
    }

    /**
     * @brief Construct with initial items
     * @param items Initial list of items
     */
    explicit combo_box(std::vector<std::string> items)
        : combo_box()
    {
        set_items(std::move(items));
    }

    // Non-copyable, movable
    combo_box(const combo_box&) = delete;
    combo_box& operator=(const combo_box&) = delete;
    combo_box(combo_box&&) noexcept = default;
    combo_box& operator=(combo_box&&) noexcept = default;

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
            base::set_current_row(-1);
        } else if (current > index) {
            // Selection was after removed item - adjust index
            base::set_current_row(current - 1);
        }
        // If current < index, selection doesn't need adjustment
    }

    /**
     * @brief Remove all items
     */
    void clear() {
        // Clear selection first
        base::set_current_row(-1);
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
        return base::current_row();
    }

    /**
     * @brief Set the current selection
     * @param index Row index to select, or -1 for no selection
     */
    void set_current_index(int index) {
        base::set_current_row(index);
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

    // ===================================================================
    // Popup Control
    // ===================================================================

    /**
     * @brief Check if the popup is currently open
     * @return true if popup is visible
     */
    [[nodiscard]] bool is_popup_open() const noexcept {
        return base::is_popup_visible();
    }

    /**
     * @brief Open the dropdown popup
     */
    void open_popup() {
        base::show_popup();
    }

    /**
     * @brief Close the dropdown popup
     */
    void close_popup() {
        base::hide_popup();
    }

    // ===================================================================
    // Advanced Access (for power users)
    // ===================================================================

    /**
     * @brief Get the internal model (read-only)
     * @return Pointer to internal list_model
     *
     * @details
     * Use this for advanced operations like connecting to model signals.
     * Do NOT modify the model directly - use the simple API methods instead.
     */
    [[nodiscard]] const model_type* model() const noexcept {
        return m_model.get();
    }

    /**
     * @brief Get the internal view
     * @return Pointer to internal combo_box_view (this)
     *
     * @details
     * Use this for advanced operations like setting a custom delegate.
     */
    [[nodiscard]] combo_box_view<Backend>* view() noexcept {
        return this;
    }

    [[nodiscard]] const combo_box_view<Backend>* view() const noexcept {
        return this;
    }

    // ===================================================================
    // Signals
    // ===================================================================

    /**
     * @brief Emitted when the current selection changes
     * @param index New current index (-1 if no selection)
     */
    signal<int> current_index_changed;

    /**
     * @brief Emitted when the current text changes
     * @param text New current text
     */
    signal<const std::string&> current_text_changed;

    /**
     * @brief Emitted when an item is activated (double-click or Enter)
     * @param index Activated item index
     */
    signal<int> activated;

private:
    std::shared_ptr<model_type> m_model;  ///< Internal string list model (owned)

    // Signal connections
    scoped_connection m_current_changed_conn;
    scoped_connection m_activated_conn;
};

} // namespace onyxui
