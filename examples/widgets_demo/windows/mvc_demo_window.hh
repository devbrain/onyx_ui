//
// Created by Claude Code on 2025-11-23.
//
// MVC Demo Window - Model-View-Controller pattern demonstration
// Demonstrates list_view with model binding, dynamic updates, and selection
//

#pragma once
#include "../backend_config.hh"  // Defines concrete Backend type
#include <onyxui/mvc/models/list_model.hh>
#include <memory>
#include <string>

namespace widgets_demo_windows {

using ui::button;
using ui::group_box;
using ui::hbox;
using ui::label;
using ui::list_view;
using ui::vbox;
using ui::window;

/**
 * @brief Create and show MVC demo window
 *
 * @details
 * Creates a window demonstrating the Model-View-Controller pattern with:
 * - list_model as the data source
 * - list_view as the view
 * - Buttons for dynamic model updates
 * - Selection display
 * - Signal/slot communication
 */
inline std::unique_ptr<window> create_mvc_demo_window() {
    // Create window with flags
    window::window_flags flags;
    flags.has_close_button = true;
    flags.has_minimize_button = true;
    flags.has_maximize_button = true;
    flags.has_menu_button = true;
    flags.is_resizable = false;
    flags.is_movable = true;
    flags.is_scrollable = true;

    auto win = std::make_unique<window>("MVC Demo", flags);

    auto* content = win->template emplace_content<vbox>(onyxui::spacing::tiny);

    // Create model with initial data (keep alive with shared_ptr)
    auto model = std::make_shared<onyxui::list_model<std::string, Backend>>();
    model->set_items({
        "Apple",
        "Banana",
        "Cherry",
        "Date",
        "Elderberry",
        "Fig",
        "Grape",
        "Honeydew",
        "Jackfruit",
        "Kiwi",
        "Lemon",
        "Mango"
    });

    // Title section
    auto* title_section = content->emplace_child<group_box>();
    title_section->set_title("MVC System Demo");
    auto* title_vbox = title_section->emplace_child<vbox>(onyxui::spacing::tiny);

    title_vbox->emplace_child<label>("Model-View-Controller Pattern");
    title_vbox->emplace_child<label>("  - list_model: Data storage");
    title_vbox->emplace_child<label>("  - list_view: Visual representation");
    title_vbox->emplace_child<label>("  - Signals: Change notifications");

    // List View section
    auto* list_section = content->emplace_child<group_box>();
    list_section->set_title("Dynamic List View");
    auto* list_vbox = list_section->emplace_child<vbox>(onyxui::spacing::tiny);

    auto* view = list_vbox->emplace_child<list_view>();
    view->set_model(model.get());

    // Controls section
    auto* controls_section = content->emplace_child<group_box>();
    controls_section->set_title("Model Controls");
    auto* controls_hbox = controls_section->emplace_child<hbox>(onyxui::spacing::medium);

    // Item counter for unique names (captured in lambdas)
    auto item_counter = std::make_shared<int>(1);

    auto* add_btn = controls_hbox->emplace_child<button>("Add Item");
    add_btn->clicked.connect([model, item_counter]() {
        model->append("New Item " + std::to_string((*item_counter)++));
    });

    auto* remove_btn = controls_hbox->emplace_child<button>("Remove Selected");
    remove_btn->clicked.connect([model, view]() {
        auto index = view->current_index();
        if (index.is_valid()) {
            model->remove(index.row);
        }
    });

    auto* clear_btn = controls_hbox->emplace_child<button>("Clear All");
    clear_btn->clicked.connect([model]() {
        model->clear();
    });

    // Selection display section
    auto* selection_section = content->emplace_child<group_box>();
    selection_section->set_title("Selection Info");
    auto* selection_vbox = selection_section->emplace_child<vbox>(onyxui::spacing::tiny);

    auto* selection_label = selection_vbox->emplace_child<label>("Selection: (none)");

    // Connect clicked signal to update selection display
    view->clicked.connect([model, selection_label](const onyxui::model_index& index) {
        if (index.is_valid()) {
            auto data = model->data(index, onyxui::item_data_role::display);
            if (std::holds_alternative<std::string>(data)) {
                std::string text = std::get<std::string>(data);
                selection_label->set_text("Selection: \"" + text + "\" (row " + std::to_string(index.row) + ")");
            }
        } else {
            selection_label->set_text("Selection: (none)");
        }
    });

    // Statistics section
    auto* stats_section = content->emplace_child<group_box>();
    stats_section->set_title("Statistics");
    auto* stats_vbox = stats_section->emplace_child<vbox>(onyxui::spacing::tiny);

    auto* stats_label = stats_vbox->emplace_child<label>(
        "Items: " + std::to_string(model->row_count())
    );

    // Update stats when model changes (check signal signatures)
    model->data_changed.connect([model, stats_label](const onyxui::model_index&, const onyxui::model_index&) {
        stats_label->set_text("Items: " + std::to_string(model->row_count()));
    });
    model->rows_inserted.connect([model, stats_label](const onyxui::model_index&, int, int) {
        stats_label->set_text("Items: " + std::to_string(model->row_count()));
    });
    model->rows_removed.connect([model, stats_label](const onyxui::model_index&, int, int) {
        stats_label->set_text("Items: " + std::to_string(model->row_count()));
    });

    // Tips section
    auto* tips_section = content->emplace_child<group_box>();
    tips_section->set_title("Tips");
    auto* tips_vbox = tips_section->emplace_child<vbox>(onyxui::spacing::tiny);

    tips_vbox->emplace_child<label>("  - Arrow keys: Navigate list");
    tips_vbox->emplace_child<label>("  - Enter: Activate item");
    tips_vbox->emplace_child<label>("  - Add/Remove: Modify model dynamically");
    tips_vbox->emplace_child<label>("  - Selection updates automatically!");

    // Set window size and position
    win->set_size(60, 25);
    win->set_position(10, 3);

    // Store model in window's user data to keep it alive
    // (This is a workaround - ideally we'd have a member variable)
    // For now, we rely on the shared_ptr captures in lambdas

    return win;
}

} // namespace widgets_demo_windows
