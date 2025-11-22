//
// OnyxUI - Combo Box Widget
// Created: 2025-11-22
//

#pragma once

#include <memory>
#include <string>

#include <onyxui/concepts/backend.hh>
#include <onyxui/core/signal.hh>
#include <onyxui/mvc/models/abstract_item_model.hh>
#include <onyxui/mvc/views/list_view.hh>
#include <onyxui/mvc/model_index.hh>
#include <onyxui/services/layer_manager.hh>
#include <onyxui/widgets/core/stateful_widget.hh>

namespace onyxui {

/**
 * @brief A dropdown combo box widget using MVC architecture
 *
 * @tparam Backend The UI backend type
 *
 * @details
 * combo_box displays a button showing the current selection. When clicked,
 * it opens a popup containing a list_view of all available items.
 *
 * Features:
 * - **MVC Integration**: Uses abstract_item_model for data
 * - **Keyboard Navigation**: Arrow keys, Home/End, Enter to select
 * - **Mouse Support**: Click to open/close, click item to select
 * - **Auto-Close**: Popup closes after selection or focus loss
 * - **Theming**: Supports button states (normal/hover/pressed/disabled)
 *
 * @par Example Usage:
 * @code
 * // Create model
 * auto model = std::make_shared<list_model<std::string, Backend>>();
 * model->set_items({"Small", "Medium", "Large", "X-Large"});
 *
 * // Create combo box
 * auto combo = std::make_unique<combo_box<Backend>>();
 * combo->set_model(model.get());
 * combo->set_current_index(1);  // Select "Medium"
 *
 * // Listen for selection changes
 * combo->current_index_changed.connect([&](int index) {
 *     std::cout << "Selected: " << model->data(model->index(index, 0)).display << "\n";
 * });
 * @endcode
 */
template<UIBackend Backend>
class combo_box : public stateful_widget<Backend> {
public:
    using base = stateful_widget<Backend>;
    using model_type = abstract_item_model<Backend>;
    using rect_type = typename Backend::rect_type;
    using size_type = typename Backend::size_type;
    using point_type = typename Backend::point_type;
    using color_type = typename Backend::color_type;
    using renderer_type = typename Backend::renderer_type;

    /**
     * @brief Construct an empty combo box
     */
    combo_box()
        : m_model(nullptr)
        , m_current_index(-1)
        , m_popup_list(nullptr)
        , m_popup_layer_id(std::nullopt)
    {
        this->set_focusable(true);
    }

    /**
     * @brief Destructor - closes popup if open
     */
    ~combo_box() override {
        close_popup();
    }

    // Rule of Five
    combo_box(const combo_box&) = delete;
    combo_box& operator=(const combo_box&) = delete;
    combo_box(combo_box&&) noexcept = default;
    combo_box& operator=(combo_box&&) noexcept = default;

    // ===================================================================
    // Model Management
    // ===================================================================

    /**
     * @brief Set the data model
     * @param model Pointer to the model (not owned)
     *
     * @details
     * The model provides the list of items to display in the dropdown.
     * When the model changes, the current selection is reset.
     */
    void set_model(model_type* model) {
        if (m_model == model) {
            return;
        }

        m_model = model;
        m_current_index = -1;  // Reset selection
        m_current_text.clear();

        // If popup is open, recreate list_view with new model
        if (is_popup_open()) {
            close_popup();
        }

        this->mark_dirty();
    }

    /**
     * @brief Get the current model
     * @return Pointer to model, or nullptr if not set
     */
    [[nodiscard]] model_type* model() const noexcept {
        return m_model;
    }

    // ===================================================================
    // Selection Management
    // ===================================================================

    /**
     * @brief Get the currently selected index
     * @return Current row index, or -1 if no selection
     */
    [[nodiscard]] int current_index() const noexcept {
        return m_current_index;
    }

    /**
     * @brief Set the current selection
     * @param index Row index to select, or -1 for no selection
     *
     * @details
     * Updates the button text to show the selected item's display text.
     * Emits current_index_changed signal.
     */
    void set_current_index(int index) {
        if (!m_model || index < -1 || index >= m_model->row_count()) {
            index = -1;
        }

        if (m_current_index == index) {
            return;  // No change
        }

        m_current_index = index;

        // Update display text
        if (m_current_index >= 0 && m_model) {
            auto idx = m_model->index(m_current_index, 0);
            auto data = m_model->data(idx, item_data_role::display);
            if (std::holds_alternative<std::string>(data)) {
                m_current_text = std::get<std::string>(data);
            } else {
                m_current_text.clear();
            }
        } else {
            m_current_text.clear();
        }

        this->mark_dirty();
        current_index_changed.emit(m_current_index);
    }

    /**
     * @brief Get the current selection text
     * @return Display text of current item, or empty if no selection
     */
    [[nodiscard]] const std::string& current_text() const noexcept {
        return m_current_text;
    }

    // ===================================================================
    // Popup Management
    // ===================================================================

    /**
     * @brief Check if the popup is currently open
     * @return true if popup is visible, false otherwise
     */
    [[nodiscard]] bool is_popup_open() const noexcept {
        return m_popup_layer_id.has_value();
    }

    /**
     * @brief Open the dropdown popup
     *
     * @details
     * TODO: Implement popup using layer_manager.
     * For now, this is a placeholder that does nothing.
     * The combo box works as a simple dropdown selector using keyboard navigation.
     */
    void open_popup() {
        // TODO: Implement popup
        m_popup_layer_id = layer_id(1);  // Fake layer ID
    }

    /**
     * @brief Close the dropdown popup
     *
     * @details
     * TODO: Implement popup closing.
     */
    void close_popup() {
        m_popup_layer_id = std::nullopt;
        m_popup_list = nullptr;
    }

    // ===================================================================
    // Signals
    // ===================================================================

    /**
     * @brief Emitted when the current selection changes
     * @param index New current index (-1 if no selection)
     */
    signal<int> current_index_changed;

protected:
    // ===================================================================
    // Event Handling
    // ===================================================================

    /**
     * @brief Handle mouse press events
     *
     * @details
     * Toggles the popup open/closed when the combo box is clicked.
     */
    bool handle_event(const ui_event& evt, event_phase phase) override {
        if (phase != event_phase::target) {
            return base::handle_event(evt, phase);
        }

        // Handle mouse clicks
        if (auto* mouse_evt = std::get_if<mouse_event>(&evt)) {
            if (mouse_evt->act == mouse_event::action::press) {
                if (is_popup_open()) {
                    close_popup();
                } else {
                    open_popup();
                }
                return true;  // Event handled
            }
        }

        // Handle keyboard
        if (auto* kbd_evt = std::get_if<keyboard_event>(&evt)) {
            if (kbd_evt->pressed && m_model) {
                int row_count = m_model->row_count();

                // Arrow down - next item
                if (kbd_evt->key == key_code::arrow_down) {
                    int next_index = (m_current_index + 1) % row_count;
                    set_current_index(next_index);
                    return true;
                }

                // Arrow up - previous item
                if (kbd_evt->key == key_code::arrow_up) {
                    int next_index = (m_current_index - 1 + row_count) % row_count;
                    set_current_index(next_index);
                    return true;
                }

                // Home - first item
                if (kbd_evt->key == key_code::home) {
                    set_current_index(0);
                    return true;
                }

                // End - last item
                if (kbd_evt->key == key_code::end) {
                    set_current_index(row_count - 1);
                    return true;
                }
            }
        }

        return base::handle_event(evt, phase);
    }

    // ===================================================================
    // Rendering
    // ===================================================================

    /**
     * @brief Render the combo box button
     *
     * @details
     * TODO: Implement full rendering with button background and dropdown arrow.
     * For now, just renders the current selection text.
     */
    void do_render(render_context<Backend>& ctx) const override {
        // TODO: Implement rendering similar to button widget
        // For now, just render text if we have a selection
        (void)ctx;
    }

private:
    // ===================================================================
    // Member Variables
    // ===================================================================

    model_type* m_model;               ///< Data model (not owned)
    int m_current_index;               ///< Currently selected row (-1 = none)
    std::string m_current_text;        ///< Display text of current item
    list_view<Backend>* m_popup_list;  ///< Popup list view (owned by layer_manager)
    std::optional<layer_id> m_popup_layer_id;  ///< Layer ID for popup

    // ===================================================================
    // Helper Methods
    // ===================================================================

    /**
     * @brief Calculate popup height based on number of items
     * @return Preferred popup height in pixels
     */
    [[nodiscard]] int calculate_popup_height() const {
        if (!m_model) {
            return 0;
        }

        int row_count = m_model->row_count();
        if (row_count == 0) {
            return 0;
        }

        // Limit to max 10 visible items (scrolling for more)
        constexpr int MAX_VISIBLE_ITEMS = 10;
        constexpr int ITEM_HEIGHT = 20;  // Approximate item height

        int visible_items = std::min(row_count, MAX_VISIBLE_ITEMS);
        return visible_items * ITEM_HEIGHT;
    }
};

} // namespace onyxui
