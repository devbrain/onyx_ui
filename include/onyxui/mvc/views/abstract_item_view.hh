//
// OnyxUI MVC System - Abstract Item View
// Created: 2025-11-22
//

#pragma once

#include <memory>

#include <onyxui/concepts/backend.hh>
#include <onyxui/core/signal.hh>
#include <onyxui/events/ui_event.hh>
#include <onyxui/mvc/delegates/abstract_item_delegate.hh>
#include <onyxui/mvc/delegates/default_item_delegate.hh>
#include <onyxui/mvc/model_index.hh>
#include <onyxui/mvc/models/abstract_item_model.hh>
#include <onyxui/mvc/selection/item_selection_model.hh>
#include <onyxui/widgets/core/widget.hh>

namespace onyxui {

/**
 * @brief Abstract base class for all item views
 *
 * @tparam Backend The UI backend type
 *
 * @details
 * abstract_item_view provides the common foundation for all MVC views:
 * - list_view (vertical list)
 * - table_view (rows and columns)
 * - tree_view (hierarchical data)
 *
 * It handles:
 * - **Model Integration**: Connects to models, listens for changes
 * - **Delegate Integration**: Delegates item rendering
 * - **Selection Management**: Tracks selected/current items
 * - **Keyboard Navigation**: Arrow keys, Page Up/Down, Home/End
 * - **Mouse Interaction**: Click to select, double-click to activate
 * - **Change Notifications**: Emits signals when user interacts
 *
 * @par Ownership:
 * - Model: NOT owned (views can share models)
 * - Delegate: Owned via shared_ptr (views can share delegates)
 * - Selection Model: Owned or shared (configurable)
 *
 * @par Thread Safety:
 * NOT thread-safe. Access only from UI thread.
 *
 * @par Subclassing:
 * Subclasses must implement:
 * - index_at(point): Hit testing for mouse clicks
 * - visual_rect(index): Get rectangle for an item
 * - update_geometries(): Recalculate item positions after model changes
 * - do_render(): Actual rendering (inherited from widget)
 *
 * @par Example Subclass:
 * See list_view.hh for a complete implementation.
 */
template<UIBackend Backend>
class abstract_item_view : public widget<Backend> {
public:
    using base = widget<Backend>;
    using model_type = abstract_item_model<Backend>;
    using delegate_type = abstract_item_delegate<Backend>;
    using selection_model_type = item_selection_model<Backend>;

    /**
     * @brief Construct view with no model
     */
    abstract_item_view()
        : m_model(nullptr)
        , m_delegate(std::make_shared<default_item_delegate<Backend>>())
        , m_selection_model(std::make_unique<selection_model_type>())
        , m_owns_selection_model(true) {

        // Connect selection signals
        connect_selection_signals();
    }

    ~abstract_item_view() override {
        // Disconnect model signals
        disconnect_model_signals();
    }

    // ===================================================================
    // Model
    // ===================================================================

    /**
     * @brief Set the model for this view
     * @param model Pointer to model (NOT owned, can be nullptr)
     *
     * @details
     * Views do not own models - multiple views can share the same model.
     * When model changes, view automatically:
     * 1. Disconnects from old model signals
     * 2. Clears selection
     * 3. Connects to new model signals
     * 4. Calls update_geometries() to recalculate layout
     * 5. Invalidates layout for repaint
     */
    void set_model(model_type* model) {
        if (m_model == model) {
            return;  // No change
        }

        // Disconnect old model
        disconnect_model_signals();

        m_model = model;

        // Update selection model (clears selection since old indices are invalid)
        if (m_selection_model) {
            m_selection_model->set_model(m_model);
        }

        // Connect new model
        if (m_model) {
            connect_model_signals();
        }

        // Update layout
        update_geometries();
        this->invalidate_measure();
    }

    /**
     * @brief Get the current model
     * @return Pointer to model, or nullptr if no model set
     */
    [[nodiscard]] model_type* model() const noexcept {
        return m_model;
    }

    // ===================================================================
    // Delegate
    // ===================================================================

    /**
     * @brief Set item delegate for rendering
     * @param delegate Shared pointer to delegate (must not be null)
     *
     * @details
     * Delegates are shared (not owned exclusively).
     * Multiple views can share the same delegate.
     * When delegate changes, view repaints.
     */
    void set_delegate(std::shared_ptr<delegate_type> delegate) {
        if (!delegate) {
            return;  // Reject null delegate
        }

        m_delegate = std::move(delegate);
        this->invalidate_paint();
    }

    /**
     * @brief Get current delegate
     * @return Shared pointer to delegate (never null)
     */
    [[nodiscard]] std::shared_ptr<delegate_type> delegate() const noexcept {
        return m_delegate;
    }

    // ===================================================================
    // Selection Model
    // ===================================================================

    /**
     * @brief Set selection model
     * @param selection Pointer to selection model (can be nullptr)
     * @param take_ownership If true, view owns the model (default: false)
     *
     * @details
     * Selection models can be owned or shared:
     * - Owned: View deletes model on destruction
     * - Shared: Multiple views share same selection (synchronized selection)
     *
     * When selection model changes, view:
     * 1. Disconnects from old selection signals
     * 2. Connects to new selection signals
     * 3. Repaints to show new selection state
     */
    void set_selection_model(selection_model_type* selection, bool take_ownership = false) {
        if (m_selection_model.get() == selection) {
            return;  // No change
        }

        // Disconnect old selection signals
        m_selection_changed_conn.disconnect();
        m_current_changed_conn.disconnect();

        // Handle ownership
        if (m_owns_selection_model && take_ownership) {
            m_selection_model.reset(selection);
        } else {
            m_selection_model.release();
            m_selection_model.reset(selection);
        }
        m_owns_selection_model = take_ownership;

        // Connect new selection signals
        if (m_selection_model) {
            connect_selection_signals();
        }

        this->invalidate_paint();
    }

    /**
     * @brief Get selection model
     * @return Pointer to selection model (can be nullptr)
     */
    [[nodiscard]] selection_model_type* selection_model() const noexcept {
        return m_selection_model.get();
    }

    // ===================================================================
    // Selection Convenience Methods
    // ===================================================================

    /**
     * @brief Get current (focused) index
     * @return Current index, or invalid if no current item
     */
    [[nodiscard]] model_index current_index() const {
        return m_selection_model ? m_selection_model->current_index() : model_index{};
    }

    /**
     * @brief Set current (focused) index
     * @param index New current index
     */
    void set_current_index(const model_index& index) {
        if (m_selection_model) {
            m_selection_model->set_current_index(index);
        }
    }

    /**
     * @brief Get all selected indices
     * @return Vector of selected indices
     */
    [[nodiscard]] std::vector<model_index> selected_indices() const {
        return m_selection_model ? m_selection_model->selected_indices() : std::vector<model_index>{};
    }

    // ===================================================================
    // Pure Virtual Methods (Subclasses Must Implement)
    // ===================================================================

    /**
     * @brief Get index at screen position
     * @param x X coordinate (view-relative)
     * @param y Y coordinate (view-relative)
     * @return Index at position, or invalid if no item
     *
     * @details
     * Used for hit testing during mouse clicks.
     * Subclasses implement based on their layout strategy.
     */
    [[nodiscard]] virtual model_index index_at(logical_unit x, logical_unit y) const = 0;

    /**
     * @brief Get visual rectangle for an item
     * @param index Item index
     * @return Rectangle in view coordinates, or empty if not visible
     *
     * @details
     * Returns the screen rectangle occupied by the item.
     * Used for scrolling to items, drawing selection, etc.
     */
    [[nodiscard]] virtual typename Backend::rect_type visual_rect(const model_index& index) const = 0;

    /**
     * @brief Scroll view to make item visible
     * @param index Item to scroll to
     *
     * @details
     * Ensures the item is visible by scrolling if necessary.
     * No-op if item is already visible or index is invalid.
     */
    virtual void scroll_to(const model_index& index) = 0;

    /**
     * @brief Update internal geometries after model changes
     *
     * @details
     * Called when model structure changes (rows inserted/removed/reset).
     * Subclasses recalculate item positions, scrollbar ranges, etc.
     *
     * This is separate from measure/arrange to avoid recalculating
     * on every layout pass (only when model changes).
     */
    virtual void update_geometries() = 0;

    // ===================================================================
    // Signals
    // ===================================================================

    /**
     * @brief Emitted when user activates an item
     * @param index Activated item
     *
     * @details
     * Activation typically occurs on:
     * - Double-click
     * - Enter key press
     */
    signal<const model_index&> activated;

    /**
     * @brief Emitted when user clicks an item
     * @param index Clicked item
     *
     * @details
     * Single-click (even if it doesn't change selection).
     */
    signal<const model_index&> clicked;

    /**
     * @brief Emitted when user double-clicks an item
     * @param index Double-clicked item
     *
     * @details
     * Always followed by activated signal.
     */
    signal<const model_index&> double_clicked;

protected:
    // ===================================================================
    // Event Handling
    // ===================================================================

    /**
     * @brief Handle keyboard events
     *
     * @details
     * Default implementation handles:
     * - Arrow keys: Navigate between items
     * - Page Up/Down: Navigate by page
     * - Home/End: Jump to first/last item
     * - Enter: Activate current item
     * - Space: Toggle selection (multi-selection mode)
     *
     * Subclasses can override to customize navigation.
     */
    bool handle_event(const ui_event& evt, event_phase phase) override {
        if (phase != event_phase::target) {
            return base::handle_event(evt, phase);
        }

        // Handle keyboard events
        if (auto* kbd_evt = std::get_if<keyboard_event>(&evt)) {
            if (kbd_evt->pressed) {
                return handle_key_press(static_cast<int>(kbd_evt->key), 0);
            }
        }

        // Handle mouse events
        if (auto* mouse_evt = std::get_if<mouse_event>(&evt)) {
            if (mouse_evt->act == mouse_event::action::press) {
                return handle_mouse_click(mouse_evt->x, mouse_evt->y);
            }
            // TODO: Add double-click handling when event system supports it
        }

        return base::handle_event(evt, phase);
    }

    /**
     * @brief Handle key press for navigation
     * @param key Key code
     * @param modifiers Key modifiers
     * @return true if handled, false otherwise
     */
    virtual bool handle_key_press(int key, int modifiers) {
        if (!m_model || !m_selection_model) {
            return false;
        }

        model_index current = m_selection_model->current_index();
        model_index next;

        // Arrow navigation
        if (key == static_cast<int>(key_code::arrow_down)) {
            next = move_cursor_down(current);
        } else if (key == static_cast<int>(key_code::arrow_up)) {
            next = move_cursor_up(current);
        } else if (key == static_cast<int>(key_code::page_down)) {
            next = move_cursor_page_down(current);
        } else if (key == static_cast<int>(key_code::page_up)) {
            next = move_cursor_page_up(current);
        } else if (key == static_cast<int>(key_code::home)) {
            next = move_cursor_home();
        } else if (key == static_cast<int>(key_code::end)) {
            next = move_cursor_end();
        } else if (key == static_cast<int>(key_code::enter)) {
            // Activate current item
            if (current.is_valid()) {
                activated.emit(current);
            }
            return true;
        } else if (key == static_cast<int>(key_code::space)) {
            // Toggle selection in multi-selection mode
            if (current.is_valid() && m_selection_model->get_selection_mode() == selection_mode::multi_selection) {
                m_selection_model->toggle(current);
            }
            return true;
        } else {
            return false;  // Not handled
        }

        // Apply navigation
        if (next.is_valid()) {
            m_selection_model->set_current_index(next);

            // In single-selection mode, current item is automatically selected
            if (m_selection_model->get_selection_mode() == selection_mode::single_selection) {
                m_selection_model->select(next);
            }

            // Scroll to ensure visible
            scroll_to(next);
            return true;
        }

        return false;
    }

    /**
     * @brief Handle mouse click
     * @param x Mouse X (view-relative, logical coordinates)
     * @param y Mouse Y (view-relative, logical coordinates)
     * @return true if handled
     */
    virtual bool handle_mouse_click(logical_unit x, logical_unit y) {
        model_index index = index_at(x, y);

        if (index.is_valid()) {
            clicked.emit(index);

            if (m_selection_model) {
                // TODO: Handle Ctrl/Shift modifiers for extended selection
                m_selection_model->set_current_index(index);
                m_selection_model->select(index);
            }

            return true;
        }

        return false;
    }

    /**
     * @brief Handle mouse double-click
     * @param x Mouse X (view-relative, logical coordinates)
     * @param y Mouse Y (view-relative, logical coordinates)
     * @return true if handled
     */
    virtual bool handle_mouse_double_click(logical_unit x, logical_unit y) {
        model_index index = index_at(x, y);

        if (index.is_valid()) {
            double_clicked.emit(index);
            activated.emit(index);
            return true;
        }

        return false;
    }

    // ===================================================================
    // Cursor Movement (Subclasses Can Override)
    // ===================================================================

    /**
     * @brief Move cursor down one item
     * @param current Current index
     * @return Next index, or invalid if at bottom
     */
    virtual model_index move_cursor_down(const model_index& current) {
        if (!m_model) return {};

        if (!current.is_valid()) {
            // No current - start at first item
            return m_model->index(0, 0);
        }

        // Move to next row
        int next_row = current.row + 1;
        if (next_row < m_model->row_count()) {
            return m_model->index(next_row, current.column);
        }

        return {};  // Already at bottom
    }

    /**
     * @brief Move cursor up one item
     */
    virtual model_index move_cursor_up(const model_index& current) {
        if (!m_model) return {};

        if (!current.is_valid()) {
            // No current - start at last item
            int last_row = m_model->row_count() - 1;
            if (last_row >= 0) {
                return m_model->index(last_row, 0);
            }
            return {};
        }

        // Move to previous row
        int prev_row = current.row - 1;
        if (prev_row >= 0) {
            return m_model->index(prev_row, current.column);
        }

        return {};  // Already at top
    }

    /**
     * @brief Move cursor down by one page
     */
    virtual model_index move_cursor_page_down(const model_index& current) {
        // Default: move down 10 items (subclasses override based on visible rows)
        if (!m_model) return {};

        int target_row = current.is_valid() ? current.row + 10 : 10;
        int max_row = m_model->row_count() - 1;
        if (target_row > max_row) {
            target_row = max_row;
        }

        if (target_row >= 0) {
            return m_model->index(target_row, current.is_valid() ? current.column : 0);
        }

        return {};
    }

    /**
     * @brief Move cursor up by one page
     */
    virtual model_index move_cursor_page_up(const model_index& current) {
        if (!m_model) return {};

        int target_row = current.is_valid() ? current.row - 10 : 0;
        if (target_row < 0) {
            target_row = 0;
        }

        return m_model->index(target_row, current.is_valid() ? current.column : 0);
    }

    /**
     * @brief Move cursor to first item
     */
    virtual model_index move_cursor_home() {
        if (!m_model || m_model->row_count() == 0) {
            return {};
        }
        return m_model->index(0, 0);
    }

    /**
     * @brief Move cursor to last item
     */
    virtual model_index move_cursor_end() {
        if (!m_model) return {};

        int last_row = m_model->row_count() - 1;
        if (last_row >= 0) {
            return m_model->index(last_row, 0);
        }

        return {};
    }

    // ===================================================================
    // Model Signal Handlers
    // ===================================================================

    /**
     * @brief Handle model data changes
     */
    virtual void on_data_changed(const model_index& top_left, const model_index& bottom_right) {
        (void)top_left;
        (void)bottom_right;

        // Repaint affected region
        // For simplicity, repaint entire view (subclasses can optimize)
        this->mark_dirty();
    }

    /**
     * @brief Handle rows inserted
     */
    virtual void on_rows_inserted(const model_index& parent, int first, int last) {
        (void)parent;
        (void)first;
        (void)last;

        update_geometries();
        this->invalidate_measure();
    }

    /**
     * @brief Handle rows removed
     */
    virtual void on_rows_removed(const model_index& parent, int first, int last) {
        (void)parent;
        (void)first;
        (void)last;

        update_geometries();
        this->invalidate_measure();
    }

    /**
     * @brief Handle layout changed
     */
    virtual void on_layout_changed() {
        update_geometries();
        this->invalidate_measure();
    }

    // ===================================================================
    // Selection Signal Handlers
    // ===================================================================

    /**
     * @brief Handle selection changes
     */
    virtual void on_selection_changed(const std::vector<model_index>& selected, const std::vector<model_index>& deselected) {
        (void)selected;
        (void)deselected;

        // Repaint to show new selection state
        this->mark_dirty();
    }

    /**
     * @brief Handle current item changes
     */
    virtual void on_current_changed(const model_index& current, const model_index& previous) {
        (void)current;
        (void)previous;

        // Repaint to show new focus rectangle
        this->mark_dirty();
    }

    // ===================================================================
    // Member Variables
    // ===================================================================

    model_type* m_model;
    std::shared_ptr<delegate_type> m_delegate;
    std::unique_ptr<selection_model_type> m_selection_model;
    bool m_owns_selection_model;

    // Signal connections
    scoped_connection m_data_changed_conn;
    scoped_connection m_rows_inserted_conn;
    scoped_connection m_rows_removed_conn;
    scoped_connection m_layout_changed_conn;
    scoped_connection m_selection_changed_conn;
    scoped_connection m_current_changed_conn;

private:
    /**
     * @brief Connect to model signals
     */
    void connect_model_signals() {
        if (!m_model) return;

        m_data_changed_conn = scoped_connection(
            m_model->data_changed,
            [this](const model_index& tl, const model_index& br) {
                on_data_changed(tl, br);
            }
        );

        m_rows_inserted_conn = scoped_connection(
            m_model->rows_inserted,
            [this](const model_index& parent, int first, int last) {
                on_rows_inserted(parent, first, last);
            }
        );

        m_rows_removed_conn = scoped_connection(
            m_model->rows_removed,
            [this](const model_index& parent, int first, int last) {
                on_rows_removed(parent, first, last);
            }
        );

        m_layout_changed_conn = scoped_connection(
            m_model->layout_changed,
            [this]() {
                on_layout_changed();
            }
        );
    }

    /**
     * @brief Disconnect from model signals
     */
    void disconnect_model_signals() {
        m_data_changed_conn.disconnect();
        m_rows_inserted_conn.disconnect();
        m_rows_removed_conn.disconnect();
        m_layout_changed_conn.disconnect();
    }

    /**
     * @brief Connect to selection model signals
     */
    void connect_selection_signals() {
        if (!m_selection_model) return;

        m_selection_changed_conn = scoped_connection(
            m_selection_model->selection_changed,
            [this](const std::vector<model_index>& sel, const std::vector<model_index>& desel) {
                on_selection_changed(sel, desel);
            }
        );

        m_current_changed_conn = scoped_connection(
            m_selection_model->current_changed,
            [this](const model_index& cur, const model_index& prev) {
                on_current_changed(cur, prev);
            }
        );
    }
};

} // namespace onyxui
