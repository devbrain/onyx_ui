//
// OnyxUI - Simple Combo Box Widget
// Created: 2025-11-22
// Refactored: 2026-04-21 (composition over private inheritance)
//

#pragma once

#include <memory>
#include <string>
#include <vector>

#include <onyxui/concepts/backend.hh>
#include <onyxui/core/signal.hh>
#include <onyxui/layout/linear_layout.hh>
#include <onyxui/mvc/models/list_model.hh>
#include <onyxui/mvc/views/combo_box_view.hh>
#include <onyxui/widgets/core/widget.hh>

namespace onyxui {

/**
 * @brief A simple dropdown combo box widget
 *
 * @tparam Backend The UI backend type
 *
 * @details
 * combo_box is the easy-to-use combo box widget with a simple string-based API.
 * It owns a combo_box_view child and a list_model, and exposes a simplified
 * interface over them.
 *
 * For advanced use cases requiring custom models, delegates, or selection
 * models, access the inner view via `view()`, or use combo_box_view directly.
 *
 * Features:
 * - **Simple API**: add_item(), remove_item(), clear(), set_items()
 * - **Self-Contained**: Owns its data and view internally
 * - **No MVC Knowledge Required**: Just use strings
 * - **Composition, not inheritance**: `emplace_child<combo_box>()` works like
 *   any other widget.
 *
 * @par Example Usage:
 * @code
 * auto* combo = parent->emplace_child<combo_box<Backend>>();
 * combo->set_items({"Apple", "Banana", "Cherry"});
 * combo->set_current_index(0);
 *
 * combo->current_index_changed.connect([](int index) {
 *     std::cout << "Selected index: " << index << "\n";
 * });
 * @endcode
 *
 * @see combo_box_view for advanced MVC usage
 */
template<UIBackend Backend>
class combo_box : public widget<Backend> {
public:
    using base = widget<Backend>;
    using model_type = list_model<std::string, Backend>;
    using view_type = combo_box_view<Backend>;

    /**
     * @brief Construct an empty combo box
     */
    combo_box()
        : m_model(std::make_shared<model_type>())
    {
        // Single-child layout so the view stretches to fill us.
        this->set_layout_strategy(
            std::make_unique<linear_layout<Backend>>(direction::vertical)
        );

        m_view = this->template emplace_child<view_type>();
        m_view->set_model(m_model.get());
        m_view->set_horizontal_align(horizontal_alignment::stretch);
        m_view->set_vertical_align(vertical_alignment::stretch);

        // The wrapper is transparent; focus lives on the view.
        this->set_focusable(false);

        m_current_changed_conn = scoped_connection(
            m_view->current_changed,
            [this](const model_index& idx) {
                int row = idx.is_valid() ? idx.row : -1;
                current_index_changed.emit(row);
                if (row >= 0) {
                    current_text_changed.emit(item_text(row));
                }
            }
        );

        m_activated_conn = scoped_connection(
            m_view->activated,
            [this](const model_index& idx) {
                if (idx.is_valid()) {
                    activated.emit(idx.row);
                }
            }
        );
    }

    /**
     * @brief Construct with initial items
     */
    explicit combo_box(std::vector<std::string> items)
        : combo_box()
    {
        set_items(std::move(items));
    }

    combo_box(const combo_box&) = delete;
    combo_box& operator=(const combo_box&) = delete;
    combo_box(combo_box&&) noexcept = default;
    combo_box& operator=(combo_box&&) noexcept = default;

    // ===================================================================
    // Simple Item Management API
    // ===================================================================

    void add_item(const std::string& text) {
        m_model->append(text);
    }

    void insert_item(int index, const std::string& text) {
        m_model->insert(index, text);
    }

    void remove_item(int index) {
        if (index < 0 || index >= count()) {
            return;
        }
        int const current = current_index();
        m_model->remove(index);
        if (current == index) {
            m_view->set_current_row(-1);
        } else if (current > index) {
            m_view->set_current_row(current - 1);
        }
    }

    void clear() {
        m_view->set_current_row(-1);
        m_model->clear();
    }

    void set_items(std::vector<std::string> items) {
        m_model->set_items(std::move(items));
    }

    [[nodiscard]] int count() const noexcept {
        return m_model->row_count();
    }

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

    [[nodiscard]] int current_index() const noexcept {
        return m_view->current_row();
    }

    void set_current_index(int index) {
        m_view->set_current_row(index);
    }

    [[nodiscard]] std::string current_text() const {
        return m_view->current_text();
    }

    bool set_current_text(const std::string& text) {
        int const index = find_text(text);
        if (index >= 0) {
            set_current_index(index);
            return true;
        }
        return false;
    }

    // ===================================================================
    // Popup Control
    // ===================================================================

    [[nodiscard]] bool is_popup_open() const noexcept {
        return m_view->is_popup_visible();
    }

    void open_popup() { m_view->show_popup(); }
    void close_popup() { m_view->hide_popup(); }

    // ===================================================================
    // Advanced Access (for power users)
    // ===================================================================

    /**
     * @brief Get the internal model (read-only)
     */
    [[nodiscard]] const model_type* model() const noexcept {
        return m_model.get();
    }

    /**
     * @brief Get the internal view for advanced MVC configuration.
     */
    [[nodiscard]] view_type* view() noexcept { return m_view; }
    [[nodiscard]] const view_type* view() const noexcept { return m_view; }

    // ===================================================================
    // Signals
    // ===================================================================

    signal<int> current_index_changed;
    signal<const std::string&> current_text_changed;
    signal<int> activated;

private:
    std::shared_ptr<model_type> m_model;
    view_type* m_view = nullptr;  ///< Non-owning; owned by children()

    scoped_connection m_current_changed_conn;
    scoped_connection m_activated_conn;
};

} // namespace onyxui
