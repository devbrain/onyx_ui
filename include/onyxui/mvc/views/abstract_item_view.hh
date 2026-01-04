//
// OnyxUI MVC System - Abstract Item View
// Created: 2025-11-22
//

#pragma once

#include <memory>
#include <chrono>
#include <cmath>

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
 * - Delegate: Shared via shared_ptr (views can share delegates)
 * - Selection Model: Shared via shared_ptr (views can share selection state)
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
        , m_selection_model(std::make_shared<selection_model_type>()) {

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

        // Reset selection anchor (old indices are invalid for new model)
        m_selection_anchor = {};

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
        this->mark_dirty();
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
     * @param selection Shared pointer to selection model
     *
     * @details
     * Selection models are shared via shared_ptr, allowing multiple views
     * to share the same selection state (synchronized selection).
     *
     * Pass nullptr to create a new default selection model.
     *
     * When selection model changes, view:
     * 1. Disconnects from old selection signals
     * 2. Sets the new selection model
     * 3. Synchronizes selection model with this view's model
     *    (calls set_model(), which clears any stale selections)
     * 4. Connects to new selection signals
     * 5. Repaints to show new selection state
     *
     * @note The selection model's model reference is always set to this
     * view's model. Cross-model selection (where selection model tracks
     * a different model than the view displays) is not supported.
     */
    void set_selection_model(std::shared_ptr<selection_model_type> selection) {
        if (m_selection_model == selection) {
            return;  // No change
        }

        // Disconnect old selection signals
        m_selection_changed_conn.disconnect();
        m_current_changed_conn.disconnect();

        // Set new selection model (or create default if nullptr)
        m_selection_model = selection ? std::move(selection)
                                      : std::make_shared<selection_model_type>();

        // Synchronize selection model with this view's model
        // This ensures the selection model tracks the same model we're displaying,
        // and clears any stale selections from a previous model
        m_selection_model->set_model(m_model);

        // Reset selection anchor (old anchor may reference different model/state)
        m_selection_anchor = {};

        // Connect new selection signals
        connect_selection_signals();

        this->mark_dirty();
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
                return handle_key_press(
                    static_cast<int>(kbd_evt->key),
                    static_cast<int>(kbd_evt->modifiers)
                );
            }
        }

        // Handle mouse events
        if (auto* mouse_evt = std::get_if<mouse_event>(&evt)) {
            if (mouse_evt->act == mouse_event::action::press &&
                mouse_evt->btn == mouse_event::button::left) {
                return handle_mouse_press(*mouse_evt);
            }
        }

        return base::handle_event(evt, phase);
    }

    /**
     * @brief Handle mouse press with double-click detection
     * @param evt Mouse event
     * @return true if handled
     */
    bool handle_mouse_press(const mouse_event& evt) {
        auto now = std::chrono::steady_clock::now();

        // Check for double-click: same position within time threshold
        bool is_double_click = false;
        if (m_last_click_time.time_since_epoch().count() > 0) {
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                now - m_last_click_time).count();

            // Double-click threshold: 500ms and within 4 logical units
            constexpr long DOUBLE_CLICK_MS = 500;
            constexpr double DOUBLE_CLICK_DISTANCE = 4.0;

            if (elapsed < DOUBLE_CLICK_MS) {
                double dx = std::abs(evt.x.value - m_last_click_x.value);
                double dy = std::abs(evt.y.value - m_last_click_y.value);
                if (dx <= DOUBLE_CLICK_DISTANCE && dy <= DOUBLE_CLICK_DISTANCE) {
                    is_double_click = true;
                }
            }
        }

        // Store click state for next comparison
        m_last_click_time = now;
        m_last_click_x = evt.x;
        m_last_click_y = evt.y;

        if (is_double_click) {
            // Reset tracking to avoid triple-click being treated as double
            m_last_click_time = std::chrono::steady_clock::time_point{};
            return handle_mouse_double_click(evt.x, evt.y);
        } else {
            return handle_mouse_click(evt.x, evt.y, evt.modifiers.ctrl, evt.modifiers.shift);
        }
    }

    /**
     * @brief Handle key press for navigation
     * @param key Key code
     * @param modifiers Key modifiers (bitmask with ctrl, shift, alt flags)
     * @return true if handled, false otherwise
     *
     * @details
     * Keyboard behavior varies by selection mode:
     *
     * - **no_selection**: Arrow keys move focus only
     * - **single_selection**: Arrow keys move and select
     * - **multi_selection**: Arrow keys move focus, Space toggles selection
     * - **extended_selection**:
     *   - Arrow: Move and select (clears others)
     *   - Shift+Arrow: Extend selection from anchor
     *   - Ctrl+Arrow: Move focus only
     *   - Space: Toggle current item
     * - **contiguous_selection**:
     *   - Arrow: Move and select (clears others)
     *   - Shift+Arrow: Extend selection from anchor
     *   - Space: No effect (only contiguous ranges allowed)
     */
    virtual bool handle_key_press(int key, int modifiers) {
        if (!m_model || !m_selection_model) {
            return false;
        }

        // Extract modifier flags
        bool const ctrl_held = (modifiers & static_cast<int>(key_modifier::ctrl)) != 0;
        bool const shift_held = (modifiers & static_cast<int>(key_modifier::shift)) != 0;

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
            // Toggle selection in multi/extended selection modes
            if (current.is_valid()) {
                auto mode = m_selection_model->get_selection_mode();
                if (mode == selection_mode::multi_selection ||
                    mode == selection_mode::extended_selection) {
                    m_selection_model->toggle(current);
                }
            }
            return true;
        } else {
            return false;  // Not handled
        }

        // Apply navigation with selection behavior based on mode
        if (next.is_valid()) {
            auto mode = m_selection_model->get_selection_mode();

            switch (mode) {
                case selection_mode::no_selection:
                    // Only move focus, no selection changes
                    m_selection_model->set_current_index(next);
                    break;

                case selection_mode::single_selection:
                    // Arrow keys always move and select
                    m_selection_model->set_current_index(next);
                    m_selection_model->select(next);
                    m_selection_anchor = next;
                    break;

                case selection_mode::multi_selection:
                    // Arrow keys only move focus (Space toggles selection)
                    m_selection_model->set_current_index(next);
                    break;

                case selection_mode::extended_selection:
                    // Standard desktop keyboard navigation:
                    // - Ctrl+Arrow: Move focus only
                    // - Shift+Arrow: Extend selection from anchor
                    // - Arrow: Move and select (clears others)
                    if (ctrl_held && !shift_held) {
                        // Ctrl+Arrow: Move focus only
                        m_selection_model->set_current_index(next);
                    } else if (shift_held) {
                        // Shift+Arrow: Extend selection from anchor
                        // Ctrl+Shift: Add range to existing selection
                        if (!m_selection_anchor.is_valid()) {
                            m_selection_anchor = current.is_valid() ? current : next;
                        }
                        select_range(m_selection_anchor, next, !ctrl_held);
                        m_selection_model->set_current_index(next);
                    } else {
                        // Normal arrow: Move and select (like single_selection)
                        m_selection_model->clear_selection();
                        m_selection_model->select(next);
                        m_selection_model->set_current_index(next);
                        m_selection_anchor = next;
                    }
                    break;

                case selection_mode::contiguous_selection:
                    // Similar to extended, but no Ctrl support
                    if (shift_held) {
                        // Shift+Arrow: Extend selection from anchor
                        if (!m_selection_anchor.is_valid()) {
                            m_selection_anchor = current.is_valid() ? current : next;
                        }
                        select_range(m_selection_anchor, next, true);  // Always clear for contiguous
                        m_selection_model->set_current_index(next);
                    } else {
                        // Normal arrow: Move and select
                        m_selection_model->clear_selection();
                        m_selection_model->select(next);
                        m_selection_model->set_current_index(next);
                        m_selection_anchor = next;
                    }
                    break;
            }

            // Scroll to ensure visible
            scroll_to(next);
            return true;
        }

        return false;
    }

    /**
     * @brief Handle mouse click with modifier support
     * @param x Mouse X (absolute logical coordinates)
     * @param y Mouse Y (absolute logical coordinates)
     * @param ctrl_held true if Ctrl key is held
     * @param shift_held true if Shift key is held
     * @return true if handled
     *
     * @details
     * Click behavior depends on modifiers and selection mode:
     * - **No modifiers**: Select only this item, set as current
     * - **Ctrl+Click** (multi-selection only): Toggle selection of this item
     * - **Shift+Click** (multi-selection only): Extend selection from anchor to this item
     *
     * @note Coordinates are absolute (screen-relative) and are converted to
     * view-relative before calling index_at().
     */
    virtual bool handle_mouse_click(logical_unit x, logical_unit y,
                                    bool ctrl_held = false, bool shift_held = false) {
        // Convert absolute coordinates to view-relative for index_at()
        auto abs_bounds = this->get_absolute_logical_bounds();
        logical_unit const rel_x = x - abs_bounds.x;
        logical_unit const rel_y = y - abs_bounds.y;

        model_index index = index_at(rel_x, rel_y);

        if (index.is_valid()) {
            clicked.emit(index);

            if (m_selection_model) {
                auto mode = m_selection_model->get_selection_mode();

                // Handle based on selection mode
                switch (mode) {
                    case selection_mode::no_selection:
                        // Only update current, no selection
                        m_selection_model->set_current_index(index);
                        break;

                    case selection_mode::single_selection:
                        // Always select only this item
                        m_selection_model->clear_selection();
                        m_selection_model->select(index);
                        m_selection_model->set_current_index(index);
                        m_selection_anchor = index;
                        break;

                    case selection_mode::multi_selection:
                        // Click toggles, modifiers ignored (each click toggles)
                        m_selection_model->toggle(index);
                        m_selection_model->set_current_index(index);
                        m_selection_anchor = index;
                        break;

                    case selection_mode::extended_selection:
                        // Standard desktop behavior with modifiers
                        if (ctrl_held && !shift_held) {
                            // Ctrl+Click: Toggle this item
                            m_selection_model->toggle(index);
                            m_selection_model->set_current_index(index);
                            m_selection_anchor = index;
                        } else if (shift_held && m_selection_anchor.is_valid()) {
                            // Shift+Click: Range selection
                            // Ctrl+Shift+Click: Add range to existing selection
                            select_range(m_selection_anchor, index, !ctrl_held);
                            m_selection_model->set_current_index(index);
                        } else {
                            // Normal click: Select only this item
                            m_selection_model->clear_selection();
                            m_selection_model->select(index);
                            m_selection_model->set_current_index(index);
                            m_selection_anchor = index;
                        }
                        break;

                    case selection_mode::contiguous_selection:
                        // Only contiguous ranges allowed
                        if (shift_held && m_selection_anchor.is_valid()) {
                            // Shift+Click: Range selection (always clears for contiguous)
                            select_range(m_selection_anchor, index, true);
                            m_selection_model->set_current_index(index);
                        } else {
                            // Normal click: Select only this item (sets new anchor)
                            m_selection_model->clear_selection();
                            m_selection_model->select(index);
                            m_selection_model->set_current_index(index);
                            m_selection_anchor = index;
                        }
                        break;
                }
            }

            return true;
        }

        return false;
    }

    /**
     * @brief Select range of items between two indices
     * @param from Start index (anchor)
     * @param to End index (target)
     * @param clear_first If true, clear existing selection before selecting range.
     *                    If false, add range to existing selection (for Ctrl+Shift).
     *
     * @details
     * Selects all items in the row range [from.row, to.row] (inclusive).
     * This is a simple row-based range selection suitable for list_view.
     * Subclasses can override for different selection patterns (e.g., table_view).
     */
    virtual void select_range(const model_index& from, const model_index& to, bool clear_first = true) {
        if (!m_model || !m_selection_model) {
            return;
        }

        // Optionally clear existing selection
        if (clear_first) {
            m_selection_model->clear_selection();
        }

        int start_row = std::min(from.row, to.row);
        int end_row = std::max(from.row, to.row);

        for (int row = start_row; row <= end_row; ++row) {
            if (row >= 0 && row < m_model->row_count()) {
                model_index idx = m_model->index(row, 0);
                m_selection_model->select(idx);
            }
        }
    }

    /**
     * @brief Handle mouse double-click
     * @param x Mouse X (absolute logical coordinates)
     * @param y Mouse Y (absolute logical coordinates)
     * @return true if handled
     *
     * @note Coordinates are absolute (screen-relative) and are converted to
     * view-relative before calling index_at().
     */
    virtual bool handle_mouse_double_click(logical_unit x, logical_unit y) {
        // Convert absolute coordinates to view-relative for index_at()
        auto abs_bounds = this->get_absolute_logical_bounds();
        logical_unit const rel_x = x - abs_bounds.x;
        logical_unit const rel_y = y - abs_bounds.y;

        model_index index = index_at(rel_x, rel_y);

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
    std::shared_ptr<selection_model_type> m_selection_model;

    // Double-click detection
    std::chrono::steady_clock::time_point m_last_click_time{};
    logical_unit m_last_click_x{0.0};
    logical_unit m_last_click_y{0.0};

    // Selection anchor for shift+click range selection
    model_index m_selection_anchor{};

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
