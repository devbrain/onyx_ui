//
// OnyxUI MVC System - Combo Box View
// Created: 2025-01-01 (Phase 5 of MVC Refactoring)
//

#pragma once

#include <cmath>
#include <memory>
#include <string>
#include <variant>

#include <onyxui/concepts/backend.hh>
#include <onyxui/core/signal.hh>
#include <onyxui/core/raii/scoped_layer.hh>
#include <onyxui/events/ui_event.hh>
#include <onyxui/mvc/delegates/abstract_item_delegate.hh>
#include <onyxui/mvc/delegates/default_item_delegate.hh>
#include <onyxui/mvc/model_index.hh>
#include <onyxui/mvc/models/abstract_item_model.hh>
#include <onyxui/mvc/selection/item_selection_model.hh>
#include <onyxui/mvc/views/list_view.hh>
#include <onyxui/services/layer_manager.hh>
#include <onyxui/services/ui_services.hh>
#include <onyxui/widgets/containers/hbox.hh>
#include <onyxui/widgets/containers/scroll/scrollbar.hh>
#include <onyxui/widgets/core/stateful_widget.hh>
#include <onyxui/ui_constants.hh>

namespace onyxui {

/**
 * @brief Advanced combo box with full MVC support
 *
 * @tparam Backend The UI backend type
 *
 * @details
 * combo_box_view is the advanced combo box widget following the MVC pattern.
 * It supports custom models, delegates, and selection models.
 *
 * Features:
 * - **Full MVC**: Custom model, delegate, and selection model
 * - **Popup List View**: Proper list_view with scrolling and virtual rendering
 * - **Keyboard Navigation**: Full keyboard support including type-ahead
 * - **Mouse Support**: Click to open, click item to select
 * - **Auto-Close**: Closes on selection, outside click, or Escape
 * - **Theming**: Button-style appearance with state support
 *
 * @par Example Usage:
 * @code
 * // Create model with custom data
 * auto model = std::make_shared<list_model<MyItem, Backend>>();
 * model->set_items(my_items);
 *
 * // Create custom delegate
 * auto delegate = std::make_shared<MyItemDelegate<Backend>>();
 *
 * // Create combo box view
 * auto combo = std::make_unique<combo_box_view<Backend>>();
 * combo->set_model(model.get());
 * combo->set_delegate(delegate);
 *
 * // Listen for activation
 * combo->activated.connect([](const model_index& idx) {
 *     std::cout << "Activated row: " << idx.row << "\n";
 * });
 * @endcode
 *
 * @see combo_box for simple string-based usage
 * @see list_view for the popup list implementation
 */
template<UIBackend Backend>
class combo_box_view : public stateful_widget<Backend> {
public:
    using base = stateful_widget<Backend>;
    using model_type = abstract_item_model<Backend>;
    using delegate_type = abstract_item_delegate<Backend>;
    using selection_model_type = item_selection_model<Backend>;
    using rect_type = typename Backend::rect_type;
    using size_type = typename Backend::size_type;
    using color_type = typename Backend::color_type;
    using renderer_type = typename Backend::renderer_type;

    /**
     * @brief Construct an empty combo box view
     */
    combo_box_view()
        : m_model(nullptr)
        , m_delegate(std::make_shared<default_item_delegate<Backend>>())
        , m_selection_model(std::make_shared<selection_model_type>())
    {
        this->set_focusable(true);
        connect_selection_signals();
    }

    /**
     * @brief Destructor - closes popup if open
     */
    ~combo_box_view() override {
        hide_popup();
    }

    // Non-copyable, movable
    combo_box_view(const combo_box_view&) = delete;
    combo_box_view& operator=(const combo_box_view&) = delete;
    combo_box_view(combo_box_view&&) noexcept = default;
    combo_box_view& operator=(combo_box_view&&) noexcept = default;

    // ===================================================================
    // MVC API
    // ===================================================================

    /**
     * @brief Set the data model
     * @param model Pointer to model (not owned)
     */
    void set_model(model_type* model) {
        if (m_model == model) {
            return;
        }

        disconnect_model_signals();
        m_model = model;

        if (m_selection_model) {
            m_selection_model->set_model(m_model);
        }

        if (m_model) {
            connect_model_signals();
        }

        // Close popup if open since model changed
        if (is_popup_visible()) {
            hide_popup();
        }

        update_display_text();
        this->mark_dirty();
    }

    /**
     * @brief Get the current model
     */
    [[nodiscard]] model_type* model() const noexcept {
        return m_model;
    }

    /**
     * @brief Set the item delegate
     * @param delegate Shared pointer to delegate
     */
    void set_delegate(std::shared_ptr<delegate_type> delegate) {
        if (!delegate) {
            return;
        }
        m_delegate = std::move(delegate);
        this->mark_dirty();
    }

    /**
     * @brief Get the current delegate
     */
    [[nodiscard]] std::shared_ptr<delegate_type> delegate() const noexcept {
        return m_delegate;
    }

    /**
     * @brief Set the selection model
     * @param selection Shared pointer to selection model
     *
     * @details
     * Selection models are shared via shared_ptr, allowing multiple views
     * to share the same selection state. Pass nullptr to create a new
     * default selection model.
     */
    void set_selection_model(std::shared_ptr<selection_model_type> selection) {
        if (m_selection_model == selection) {
            return;
        }

        disconnect_selection_signals();

        // Set new selection model (or create default if nullptr)
        m_selection_model = selection ? std::move(selection)
                                      : std::make_shared<selection_model_type>();

        // Synchronize selection model with this view's model
        m_selection_model->set_model(m_model);

        connect_selection_signals();

        // Refresh display text from new selection's current index
        update_display_text();

        this->mark_dirty();
    }

    /**
     * @brief Get the selection model
     */
    [[nodiscard]] selection_model_type* selection_model() const noexcept {
        return m_selection_model.get();
    }

    // ===================================================================
    // Current Selection
    // ===================================================================

    /**
     * @brief Get the current index
     * @return Current model_index, or invalid if no selection
     */
    [[nodiscard]] model_index current_index() const {
        return m_selection_model ? m_selection_model->current_index() : model_index{};
    }

    /**
     * @brief Set the current index
     * @param index Index to select
     */
    void set_current_index(const model_index& index) {
        if (m_selection_model) {
            m_selection_model->set_current_index(index);
            m_selection_model->select(index);
        }
    }

    /**
     * @brief Set current selection by row number
     * @param row Row index (0-based), or -1 for no selection
     */
    void set_current_row(int row) {
        if (!m_model || row < 0 || row >= m_model->row_count()) {
            if (m_selection_model) {
                m_selection_model->clear_selection();
                m_selection_model->set_current_index(model_index{});  // Clear current
            }
            update_display_text();
            this->mark_dirty();
            return;
        }
        set_current_index(m_model->index(row, 0));
    }

    /**
     * @brief Get current row number
     * @return Row index, or -1 if no selection
     */
    [[nodiscard]] int current_row() const {
        auto idx = current_index();
        return idx.is_valid() ? idx.row : -1;
    }

    /**
     * @brief Get display text of current selection
     */
    [[nodiscard]] const std::string& current_text() const noexcept {
        return m_current_text;
    }

    // ===================================================================
    // Popup Control
    // ===================================================================

    /**
     * @brief Show the dropdown popup
     */
    void show_popup() {
        if (is_popup_visible() || !m_model || m_model->row_count() == 0) {
            return;
        }

        auto* layers = ui_services<Backend>::layers();
        if (!layers) {
            return;
        }

        int const row_count = m_model->row_count();
        bool const needs_scrollbar = row_count > MAX_VISIBLE_ITEMS;

        // Create list_view for the popup
        m_popup_list = std::make_unique<list_view<Backend>>();
        m_popup_list->set_model(m_model);
        m_popup_list->set_delegate(m_delegate);
        m_popup_list_ptr = m_popup_list.get();  // Store raw pointer for signals

        // Set current selection in popup
        auto cur = current_index();
        if (cur.is_valid()) {
            m_popup_list->set_current_index(cur);
            // Scroll to show current item
            m_popup_list->scroll_to(cur);
        }

        // Connect popup signals
        m_popup_activated_conn = scoped_connection(
            m_popup_list->activated,
            [this](const model_index& idx) {
                on_popup_item_activated(idx);
            }
        );

        m_popup_clicked_conn = scoped_connection(
            m_popup_list->clicked,
            [this](const model_index& idx) {
                on_popup_item_clicked(idx);
            }
        );

        // Calculate popup bounds
        auto combo_bounds = this->get_absolute_logical_bounds();
        int const popup_height = calculate_popup_height();

        // Determine popup widget (container with scrollbar, or just list_view)
        ui_element<Backend>* popup_widget = nullptr;

        if (needs_scrollbar) {
            // Create scrollbar
            m_popup_scrollbar = std::make_unique<scrollbar<Backend>>(
                orientation::vertical);
            m_popup_scrollbar_ptr = m_popup_scrollbar.get();

            // Set list_view to expand to fill remaining width (after scrollbar)
            size_constraint expand_width;
            expand_width.policy = size_policy::expand;
            m_popup_list->set_width_constraint(expand_width);

            // Create hbox container for list + scrollbar
            m_popup_container = std::make_unique<hbox<Backend>>(spacing::none);
            m_popup_container->set_visible(true);

            // Connect scroll signals BEFORE moving unique_ptrs
            connect_scroll_signals();

            // Add children to container
            m_popup_container->add_child(std::move(m_popup_list));
            m_popup_container->add_child(std::move(m_popup_scrollbar));

            // Initialize scrollbar with estimated values (will be updated on scroll)
            initialize_scrollbar();

            popup_widget = m_popup_container.get();
        } else {
            popup_widget = m_popup_list.get();
        }

        // Measure popup (let layer_manager handle arrangement)
        logical_unit const popup_width = combo_bounds.width;
        logical_unit const popup_height_lu{static_cast<double>(popup_height)};
        auto measured_size = popup_widget->measure(popup_width, popup_height_lu);

        // Use explicit popup dimensions (not measured width which may be small
        // if list_view has expand policy - expand only works during arrange)
        logical_size const preferred_size{popup_width, popup_height_lu};

        // Show popup via layer manager with preferred size
        auto layer_id = layers->show_popup(
            popup_widget,
            combo_bounds,
            popup_placement::below,
            [this]() { hide_popup(); },  // Close on outside click
            preferred_size  // Use explicit size, not measured
        );

        m_popup_layer = scoped_layer<Backend>(layers, layer_id);

        // Focus the popup list
        if (auto* input = ui_services<Backend>::input()) {
            input->set_focus(m_popup_list_ptr);
        }

        popup_shown.emit();
    }

    /**
     * @brief Hide the dropdown popup
     */
    void hide_popup() {
        if (!is_popup_visible()) {
            return;
        }

        // Disconnect popup signals
        m_popup_activated_conn.disconnect();
        m_popup_clicked_conn.disconnect();
        m_scroll_offset_conn.disconnect();
        m_scrollbar_scroll_conn.disconnect();

        // Reset layer (auto-removes from layer manager)
        m_popup_layer.reset();

        // Clear raw pointers
        m_popup_list_ptr = nullptr;
        m_popup_scrollbar_ptr = nullptr;

        // Destroy popup widgets (container owns children if scrollbar was used)
        m_popup_container.reset();
        m_popup_list.reset();  // May already be null if moved to container
        m_popup_scrollbar.reset();  // May already be null if moved to container

        // Return focus to combo box
        if (auto* input = ui_services<Backend>::input()) {
            input->set_focus(this);
        }

        popup_hidden.emit();
    }

    /**
     * @brief Check if popup is visible
     */
    [[nodiscard]] bool is_popup_visible() const noexcept {
        return m_popup_layer.is_valid();
    }

    /**
     * @brief Toggle popup visibility
     */
    void toggle_popup() {
        if (is_popup_visible()) {
            hide_popup();
        } else {
            show_popup();
        }
    }

    // ===================================================================
    // Signals
    // ===================================================================

    /**
     * @brief Emitted when an item is activated (double-click or Enter)
     */
    signal<const model_index&> activated;

    /**
     * @brief Emitted when current selection changes
     */
    signal<const model_index&> current_changed;

    /**
     * @brief Emitted when popup is shown
     */
    signal<> popup_shown;

    /**
     * @brief Emitted when popup is hidden
     */
    signal<> popup_hidden;

protected:
    // ===================================================================
    // Event Handling
    // ===================================================================

    bool handle_event(const ui_event& evt, event_phase phase) override {
        if (phase != event_phase::target) {
            return base::handle_event(evt, phase);
        }

        // Handle mouse clicks - toggle popup
        if (auto* mouse_evt = std::get_if<mouse_event>(&evt)) {
            if (mouse_evt->act == mouse_event::action::press &&
                mouse_evt->btn == mouse_event::button::left) {
                toggle_popup();
                return true;
            }
        }

        // Handle keyboard
        if (auto* kbd_evt = std::get_if<keyboard_event>(&evt)) {
            if (kbd_evt->pressed) {
                return handle_key_press(kbd_evt->key);
            }
        }

        return base::handle_event(evt, phase);
    }

    /**
     * @brief Handle key press
     */
    bool handle_key_press(key_code key) {
        // Space/Enter/Alt+Down opens popup
        if ((key == key_code::space || key == key_code::enter ||
             key == key_code::arrow_down) && !is_popup_visible()) {
            show_popup();
            return true;
        }

        // Escape closes popup
        if (key == key_code::escape && is_popup_visible()) {
            hide_popup();
            return true;
        }

        // Arrow keys navigate when closed
        if (!is_popup_visible() && m_model && m_model->row_count() > 0) {
            int current = current_row();
            int row_count = m_model->row_count();

            if (key == key_code::arrow_down) {
                set_current_row((current + 1) % row_count);
                return true;
            }
            if (key == key_code::arrow_up) {
                set_current_row((current - 1 + row_count) % row_count);
                return true;
            }
            if (key == key_code::home) {
                set_current_row(0);
                return true;
            }
            if (key == key_code::end) {
                set_current_row(row_count - 1);
                return true;
            }
        }

        return false;
    }

    // ===================================================================
    // Theme Styling
    // ===================================================================

    [[nodiscard]] bool should_inherit_colors() const override {
        return false;  // Stateful widget - use theme colors
    }

    [[nodiscard]] resolved_style<Backend> get_theme_style(const ui_theme<Backend>& theme) const override {
        return resolved_style<Backend>{
            .background_color = this->get_state_background(theme.button),
            .foreground_color = this->get_state_foreground(theme.button),
            .mnemonic_foreground = this->get_state_mnemonic_foreground(theme.button),
            .border_color = theme.border_color,
            .box_style = theme.button.box_style,
            .font = theme.button.normal.font,
            .opacity = 1.0f,
            .icon_style = std::optional<typename renderer_type::icon_style>{},
            .padding_horizontal = std::make_optional(theme.button.padding_horizontal),
            .padding_vertical = std::make_optional(theme.button.padding_vertical),
            .mnemonic_font = std::optional<typename renderer_type::font>{},
            .submenu_icon = std::optional<typename renderer_type::icon_style>{}
        };
    }

    // ===================================================================
    // Rendering
    // ===================================================================

    void do_render(render_context<Backend>& ctx) const override {
        // Get padding from resolved style
        int const padding_h = ctx.style().padding_horizontal.value
            .value_or(ui_constants::DEFAULT_BUTTON_PADDING_HORIZONTAL);
        int const padding_v = ctx.style().padding_vertical.value
            .value_or(ui_constants::DEFAULT_BUTTON_PADDING_VERTICAL);
        int const border = renderer_type::get_border_thickness(ctx.style().box_style);

        auto const& font = ctx.style().font;
        std::string display_text = m_current_text.empty() ? "(select)" : m_current_text;

        // Measure text and arrow icon
        auto text_size = renderer_type::measure_text(display_text, font);
        int const text_width = size_utils::get_width(text_size);
        int const text_height = size_utils::get_height(text_size);

        using icon_style = typename renderer_type::icon_style;
        auto arrow_size = renderer_type::get_icon_size(icon_style::arrow_down);
        int const arrow_width = size_utils::get_width(arrow_size);
        int const arrow_height = size_utils::get_height(arrow_size);

        int const content_height = std::max(text_height, arrow_height);
        int const natural_width = text_width + arrow_width + padding_h + (padding_h * 2) + (border * 2);
        int const natural_height = content_height + (padding_v * 2) + (border * 2);

        auto const [final_width, final_height] = ctx.get_final_dims(natural_width, natural_height);

        auto const& pos = ctx.position();
        int const x = static_cast<int>(pos.x);
        int const y = static_cast<int>(pos.y);

        // Draw box
        rect_type box_rect;
        rect_utils::set_bounds(box_rect, x, y, final_width, final_height);
        ctx.draw_rect(box_rect, ctx.style().box_style);

        auto fg = ctx.style().foreground_color;

        // Draw text
        int const text_x = x + border + padding_h;
        int const text_y = y + border + padding_v;
        typename Backend::point_type const text_pos{text_x, text_y};
        ctx.draw_text(display_text, text_pos, font, fg);

        // Draw arrow icon
        int const arrow_x = x + final_width - border - padding_h - arrow_width;
        int const arrow_y = y + border + padding_v + (content_height - arrow_height) / 2;
        typename Backend::point_type const arrow_pos{arrow_x, arrow_y};
        ctx.draw_icon(icon_style::arrow_down, arrow_pos);
    }

private:
    // ===================================================================
    // Popup Helpers
    // ===================================================================

    void on_popup_item_activated(const model_index& idx) {
        set_current_index(idx);
        hide_popup();
        activated.emit(idx);
    }

    void on_popup_item_clicked(const model_index& idx) {
        set_current_index(idx);
        hide_popup();
    }

    /**
     * @brief Calculate popup height in logical units
     * @return Logical height for popup (including border)
     *
     * @details
     * Delegate size_hint() returns physical units. We convert to logical
     * for layout calculations since measure/arrange use logical coordinates.
     * Height is limited to MAX_VISIBLE_ITEMS; scrollbar is shown for more items.
     */
    [[nodiscard]] int calculate_popup_height() const {
        if (!m_model || !m_delegate) {
            return 0;
        }

        int row_count = m_model->row_count();
        if (row_count == 0) {
            return 0;
        }

        // Get metrics for physical→logical conversion
        auto const* metrics = ui_services<Backend>::metrics();

        // Sum item heights (delegate returns physical, convert to logical)
        // Limit to MAX_VISIBLE_ITEMS - scrollbar shown for more
        double total_logical_height = 0.0;
        int visible_count = std::min(row_count, MAX_VISIBLE_ITEMS);

        for (int row = 0; row < visible_count; ++row) {
            auto idx = m_model->index(row, 0);
            auto size = m_delegate->size_hint(idx);  // Physical units

            // Convert physical height to logical
            double item_logical_height = metrics
                ? metrics->physical_to_logical_y(physical_y(size.h)).value
                : static_cast<double>(size.h);  // Fallback 1:1

            total_logical_height += item_logical_height;
        }

        // Add border height (2 logical units: top + bottom)
        constexpr double logical_border_height = 2.0;
        // Round up to avoid clipping last row at fractional scales
        return static_cast<int>(std::ceil(total_logical_height + logical_border_height));
    }

    void update_display_text() {
        m_current_text.clear();

        auto idx = current_index();
        if (!idx.is_valid() || !m_model) {
            return;
        }

        auto data = m_model->data(idx, item_data_role::display);
        if (std::holds_alternative<std::string>(data)) {
            m_current_text = std::get<std::string>(data);
        }
    }

    /**
     * @brief Connect bidirectional scroll signals between list and scrollbar
     *
     * @details
     * Uses raw pointers (m_popup_list_ptr, m_popup_scrollbar_ptr) that are
     * valid for the lifetime of the popup. Must be called BEFORE moving
     * unique_ptrs to the container.
     */
    void connect_scroll_signals() {
        if (!m_popup_list_ptr || !m_popup_scrollbar_ptr) {
            return;
        }

        // List → Scrollbar: update scrollbar position when list scrolls
        m_scroll_offset_conn = scoped_connection(
            m_popup_list_ptr->scroll_offset_changed,
            [this](double offset) {
                if (m_popup_scrollbar_ptr) {
                    // Get current scroll info from scrollbar and update position
                    auto info = m_popup_scrollbar_ptr->get_scroll_info();
                    info.scroll_y = offset;
                    m_popup_scrollbar_ptr->set_scroll_info(info);
                }
            }
        );

        // Scrollbar → List: scroll list when scrollbar is dragged/arrows clicked
        m_scrollbar_scroll_conn = scoped_connection(
            m_popup_scrollbar_ptr->scroll_requested,
            [this](int offset) {  // Note: signal is int, not double
                if (m_popup_list_ptr) {
                    m_popup_list_ptr->set_scroll_offset(static_cast<double>(offset));
                }
            }
        );
    }

    /**
     * @brief Initialize scrollbar with content/viewport dimensions
     *
     * @details
     * Estimates dimensions before layout. The scrollbar will be updated
     * dynamically as the list scrolls via connect_scroll_signals().
     */
    void initialize_scrollbar() {
        if (!m_popup_scrollbar_ptr || !m_model || !m_delegate) {
            return;
        }

        // Calculate total content height (all items)
        auto const* metrics = ui_services<Backend>::metrics();
        double total_content_height = 0.0;

        for (int row = 0; row < m_model->row_count(); ++row) {
            auto idx = m_model->index(row, 0);
            auto size = m_delegate->size_hint(idx);  // Physical units

            // Convert physical height to logical
            double item_logical_height = metrics
                ? metrics->physical_to_logical_y(physical_y(size.h)).value
                : static_cast<double>(size.h);

            total_content_height += item_logical_height;
        }

        // Calculate viewport height (MAX_VISIBLE_ITEMS only, no border)
        // list_view now correctly uses content area height for max_scroll
        double viewport_height = 0.0;
        int visible_count = std::min(m_model->row_count(), MAX_VISIBLE_ITEMS);

        for (int row = 0; row < visible_count; ++row) {
            auto idx = m_model->index(row, 0);
            auto size = m_delegate->size_hint(idx);

            double item_logical_height = metrics
                ? metrics->physical_to_logical_y(physical_y(size.h)).value
                : static_cast<double>(size.h);

            viewport_height += item_logical_height;
        }

        // Set scroll info - use list's current scroll position, not 0
        // The list may have already scrolled to show the selected item
        double current_scroll = m_popup_list_ptr ? m_popup_list_ptr->scroll_offset() : 0.0;

        scroll_info<Backend> info{};
        info.content_height = total_content_height;
        info.viewport_height = viewport_height;
        info.scroll_y = current_scroll;  // Sync with list's current position
        m_popup_scrollbar_ptr->set_scroll_info(info);
    }

    // ===================================================================
    // Signal Connections
    // ===================================================================

    void connect_model_signals() {
        if (!m_model) return;

        m_model_layout_conn = scoped_connection(
            m_model->layout_changed,
            [this]() {
                update_display_text();
                this->mark_dirty();
            }
        );
    }

    void disconnect_model_signals() {
        m_model_layout_conn.disconnect();
    }

    void connect_selection_signals() {
        if (!m_selection_model) return;

        m_selection_current_conn = scoped_connection(
            m_selection_model->current_changed,
            [this](const model_index& cur, const model_index& /*prev*/) {
                update_display_text();
                this->mark_dirty();
                current_changed.emit(cur);
            }
        );
    }

    void disconnect_selection_signals() {
        m_selection_current_conn.disconnect();
    }

    // ===================================================================
    // Constants
    // ===================================================================

    /// Maximum visible items in popup (keyboard scrolling for more)
    static constexpr int MAX_VISIBLE_ITEMS = 8;

    // ===================================================================
    // Member Variables
    // ===================================================================

    model_type* m_model;
    std::shared_ptr<delegate_type> m_delegate;
    std::shared_ptr<selection_model_type> m_selection_model;
    std::string m_current_text;

    // Popup
    std::unique_ptr<hbox<Backend>> m_popup_container;
    std::unique_ptr<list_view<Backend>> m_popup_list;
    std::unique_ptr<scrollbar<Backend>> m_popup_scrollbar;
    scoped_layer<Backend> m_popup_layer;
    scoped_connection m_popup_activated_conn;
    scoped_connection m_popup_clicked_conn;
    scoped_connection m_scroll_offset_conn;
    scoped_connection m_scrollbar_scroll_conn;

    // Raw pointers for signal callbacks (valid while popup is open)
    list_view<Backend>* m_popup_list_ptr = nullptr;
    scrollbar<Backend>* m_popup_scrollbar_ptr = nullptr;

    // Model/selection connections
    scoped_connection m_model_layout_conn;
    scoped_connection m_selection_current_conn;
};

} // namespace onyxui
