//
// Created by Claude Code on 2025-11-23.
//
// MVC Demo Window - Model-View-Controller pattern demonstration
// Demonstrates list_view with model binding, dynamic updates, and selection
//

#pragma once
#include <onyxui/widgets/window/window.hh>
#include <onyxui/widgets/containers/vbox.hh>
#include <onyxui/widgets/containers/hbox.hh>
#include <onyxui/widgets/containers/group_box.hh>
#include <onyxui/widgets/containers/scroll_view.hh>
#include <onyxui/widgets/label.hh>
#include <onyxui/widgets/button.hh>
#include <onyxui/mvc/views/list_view.hh>
#include <onyxui/mvc/models/list_model.hh>
#include <memory>
#include <string>

namespace widgets_demo_windows {

/**
 * @brief Create and show MVC demo window
 * @tparam Backend UI backend type
 *
 * @details
 * Creates a window demonstrating the Model-View-Controller pattern with:
 * - list_model as the data source
 * - list_view as the view
 * - Buttons for dynamic model updates
 * - Selection display
 * - Signal/slot communication
 */
template<onyxui::UIBackend Backend>
std::shared_ptr<onyxui::window<Backend>> create_mvc_demo_window() {
    // Create window with flags
    typename onyxui::window<Backend>::window_flags flags;
    flags.has_close_button = true;
    flags.has_minimize_button = true;
    flags.has_maximize_button = true;
    flags.has_menu_button = true;
    flags.is_resizable = false;
    flags.is_movable = true;
    flags.is_scrollable = true;

    auto win = std::make_shared<onyxui::window<Backend>>("MVC Demo", flags);

    // Create content container
    auto content = std::make_unique<onyxui::vbox<Backend>>(1);

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
        "Honeydew"
    });

    // Title section
    auto* title_section = content->template emplace_child<onyxui::group_box>();
    title_section->set_title("MVC System Demo");
    auto* title_vbox = title_section->template emplace_child<onyxui::vbox>(1);

    title_vbox->template emplace_child<onyxui::label>("Model-View-Controller Pattern");
    title_vbox->template emplace_child<onyxui::label>("  - list_model: Data storage");
    title_vbox->template emplace_child<onyxui::label>("  - list_view: Visual representation");
    title_vbox->template emplace_child<onyxui::label>("  - Signals: Change notifications");

    // List View section
    auto* list_section = content->template emplace_child<onyxui::group_box>();
    list_section->set_title("Dynamic List View");
    auto* list_vbox = list_section->template emplace_child<onyxui::vbox>(1);

    auto* list_view = list_vbox->template emplace_child<onyxui::list_view>();
    list_view->set_model(model.get());

    // Controls section
    auto* controls_section = content->template emplace_child<onyxui::group_box>();
    controls_section->set_title("Model Controls");
    auto* controls_hbox = controls_section->template emplace_child<onyxui::hbox>(2);

    // Item counter for unique names (captured in lambdas)
    auto item_counter = std::make_shared<int>(1);

    auto* add_btn = controls_hbox->template emplace_child<onyxui::button>("Add Item");
    add_btn->clicked.connect([model, item_counter]() {
        model->append("New Item " + std::to_string((*item_counter)++));
    });

    auto* remove_btn = controls_hbox->template emplace_child<onyxui::button>("Remove Selected");
    remove_btn->clicked.connect([model, list_view]() {
        auto index = list_view->current_index();
        if (index.is_valid()) {
            model->remove(index.row);
        }
    });

    auto* clear_btn = controls_hbox->template emplace_child<onyxui::button>("Clear All");
    clear_btn->clicked.connect([model]() {
        model->clear();
    });

    // Selection display section
    auto* selection_section = content->template emplace_child<onyxui::group_box>();
    selection_section->set_title("Selection Info");
    auto* selection_vbox = selection_section->template emplace_child<onyxui::vbox>(1);

    auto* selection_label = selection_vbox->template emplace_child<onyxui::label>("Selection: (none)");

    // Connect clicked signal to update selection display
    list_view->clicked.connect([model, selection_label](const onyxui::model_index& index) {
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
    auto* stats_section = content->template emplace_child<onyxui::group_box>();
    stats_section->set_title("Statistics");
    auto* stats_vbox = stats_section->template emplace_child<onyxui::vbox>(1);

    auto* stats_label = stats_vbox->template emplace_child<onyxui::label>(
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
    auto* tips_section = content->template emplace_child<onyxui::group_box>();
    tips_section->set_title("Tips");
    auto* tips_vbox = tips_section->template emplace_child<onyxui::vbox>(1);

    tips_vbox->template emplace_child<onyxui::label>("  - Arrow keys: Navigate list");
    tips_vbox->template emplace_child<onyxui::label>("  - Enter: Activate item");
    tips_vbox->template emplace_child<onyxui::label>("  - Add/Remove: Modify model dynamically");
    tips_vbox->template emplace_child<onyxui::label>("  - Selection updates automatically!");

    // Set content on window
    win->set_content(std::move(content));

    // Set window size and position
    win->set_size(60, 25);
    win->set_position(10, 3);

    // Store model in window's user data to keep it alive
    // (This is a workaround - ideally we'd have a member variable)
    // For now, we rely on the shared_ptr captures in lambdas

    return win;
}

} // namespace widgets_demo_windows
