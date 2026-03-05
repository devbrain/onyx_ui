/**
 * @file tile_factory.hh
 * @brief Factory functions for creating tile-based UI widgets
 *
 * Provides convenient factory functions and builders for creating
 * pre-configured tile widgets and common UI patterns.
 */

#pragma once

#include <memory>
#include <string>
#include <vector>
#include <functional>
#include <initializer_list>

#include <onyxui/tile/widgets/tile_widgets.hh>
#include <onyxui/tile/tile_animation.hh>
#include <onyxui/widgets/containers/vbox.hh>
#include <onyxui/widgets/containers/hbox.hh>

namespace onyxui::tile {

// ============================================================================
// Widget Factory Functions
// ============================================================================

/**
 * @brief Create a text label
 */
template<UIBackend Backend>
[[nodiscard]] std::unique_ptr<tile_label<Backend>> make_label(
    const std::string& text) {
    return std::make_unique<tile_label<Backend>>(text);
}

/**
 * @brief Create a centered text label
 */
template<UIBackend Backend>
[[nodiscard]] std::unique_ptr<tile_label<Backend>> make_label_centered(
    const std::string& text) {
    auto label = std::make_unique<tile_label<Backend>>(text);
    label->set_centered(true);
    return label;
}

/**
 * @brief Create a button with click handler
 */
template<UIBackend Backend>
[[nodiscard]] std::unique_ptr<tile_button<Backend>> make_button(
    const std::string& text,
    std::function<void()> on_click = nullptr) {
    auto button = std::make_unique<tile_button<Backend>>(text);
    if (on_click) {
        button->clicked.connect(std::move(on_click));
    }
    return button;
}

/**
 * @brief Create a checkbox with change handler
 */
template<UIBackend Backend>
[[nodiscard]] std::unique_ptr<tile_checkbox<Backend>> make_checkbox(
    const std::string& text,
    bool checked = false,
    std::function<void(bool)> on_change = nullptr) {
    auto checkbox = std::make_unique<tile_checkbox<Backend>>(text);
    checkbox->set_checked(checked);
    if (on_change) {
        checkbox->toggled.connect(std::move(on_change));
    }
    return checkbox;
}

/**
 * @brief Create a tri-state checkbox
 */
template<UIBackend Backend>
[[nodiscard]] std::unique_ptr<tile_checkbox<Backend>> make_tri_state_checkbox(
    const std::string& text,
    checkbox_state initial_state = checkbox_state::unchecked,
    std::function<void(checkbox_state)> on_change = nullptr) {
    auto checkbox = std::make_unique<tile_checkbox<Backend>>(text);
    checkbox->set_tri_state_enabled(true);
    checkbox->set_state(initial_state);
    if (on_change) {
        checkbox->state_changed.connect(std::move(on_change));
    }
    return checkbox;
}

/**
 * @brief Create a progress bar
 */
template<UIBackend Backend>
[[nodiscard]] std::unique_ptr<tile_progress_bar<Backend>> make_progress_bar(
    int value = 0,
    int min = 0,
    int max = 100) {
    auto progress = std::make_unique<tile_progress_bar<Backend>>();
    progress->set_range(min, max);
    progress->set_value(value);
    return progress;
}

/**
 * @brief Create an indeterminate progress bar
 */
template<UIBackend Backend>
[[nodiscard]] std::unique_ptr<tile_progress_bar<Backend>> make_indeterminate_progress() {
    auto progress = std::make_unique<tile_progress_bar<Backend>>();
    progress->set_indeterminate(true);
    return progress;
}

/**
 * @brief Create a slider with change handler
 */
template<UIBackend Backend>
[[nodiscard]] std::unique_ptr<tile_slider<Backend>> make_slider(
    int value = 0,
    int min = 0,
    int max = 100,
    std::function<void(int)> on_change = nullptr) {
    auto slider = std::make_unique<tile_slider<Backend>>();
    slider->set_range(min, max);
    slider->set_value(value);
    if (on_change) {
        slider->value_changed.connect(std::move(on_change));
    }
    return slider;
}

/**
 * @brief Create a text input field
 */
template<UIBackend Backend>
[[nodiscard]] std::unique_ptr<tile_text_input<Backend>> make_text_input(
    const std::string& initial_text = "",
    const std::string& placeholder = "",
    std::function<void(const std::string&)> on_change = nullptr) {
    auto input = std::make_unique<tile_text_input<Backend>>(initial_text);
    if (!placeholder.empty()) {
        input->set_placeholder(placeholder);
    }
    if (on_change) {
        input->text_changed.connect(std::move(on_change));
    }
    return input;
}

/**
 * @brief Create a password input field
 */
template<UIBackend Backend>
[[nodiscard]] std::unique_ptr<tile_text_input<Backend>> make_password_input(
    const std::string& placeholder = "Password") {
    auto input = std::make_unique<tile_text_input<Backend>>();
    input->set_password_mode(true);
    input->set_placeholder(placeholder);
    return input;
}

/**
 * @brief Create a combo box with items
 */
template<UIBackend Backend>
[[nodiscard]] std::unique_ptr<tile_combo_box<Backend>> make_combo_box(
    std::initializer_list<std::string> items,
    int selected_index = 0,
    std::function<void(int)> on_change = nullptr) {
    auto combo = std::make_unique<tile_combo_box<Backend>>();
    for (const auto& item : items) {
        combo->add_item(item);
    }
    if (selected_index >= 0 && selected_index < static_cast<int>(items.size())) {
        combo->set_current_index(selected_index);
    }
    if (on_change) {
        combo->current_index_changed.connect(std::move(on_change));
    }
    return combo;
}

/**
 * @brief Create a combo box from a vector of strings
 */
template<UIBackend Backend>
[[nodiscard]] std::unique_ptr<tile_combo_box<Backend>> make_combo_box(
    const std::vector<std::string>& items,
    int selected_index = 0,
    std::function<void(int)> on_change = nullptr) {
    auto combo = std::make_unique<tile_combo_box<Backend>>();
    for (const auto& item : items) {
        combo->add_item(item);
    }
    if (selected_index >= 0 && selected_index < static_cast<int>(items.size())) {
        combo->set_current_index(selected_index);
    }
    if (on_change) {
        combo->current_index_changed.connect(std::move(on_change));
    }
    return combo;
}

/**
 * @brief Create a vertical scrollbar
 */
template<UIBackend Backend>
[[nodiscard]] std::unique_ptr<tile_scrollbar<Backend>> make_v_scrollbar(
    int content_size = 100,
    int viewport_size = 100,
    std::function<void(int)> on_scroll = nullptr) {
    auto scrollbar = std::make_unique<tile_scrollbar<Backend>>(orientation::vertical);
    scrollbar->set_content_size(content_size);
    scrollbar->set_viewport_size(viewport_size);
    if (on_scroll) {
        scrollbar->scroll_requested.connect(std::move(on_scroll));
    }
    return scrollbar;
}

/**
 * @brief Create a horizontal scrollbar
 */
template<UIBackend Backend>
[[nodiscard]] std::unique_ptr<tile_scrollbar<Backend>> make_h_scrollbar(
    int content_size = 100,
    int viewport_size = 100,
    std::function<void(int)> on_scroll = nullptr) {
    auto scrollbar = std::make_unique<tile_scrollbar<Backend>>(orientation::horizontal);
    scrollbar->set_content_size(content_size);
    scrollbar->set_viewport_size(viewport_size);
    if (on_scroll) {
        scrollbar->scroll_requested.connect(std::move(on_scroll));
    }
    return scrollbar;
}

// ============================================================================
// Radio Button Group Factory
// ============================================================================

/**
 * @brief Create a radio button group with options
 */
template<UIBackend Backend>
[[nodiscard]] std::unique_ptr<tile_radio_group<Backend>> make_radio_group(
    std::initializer_list<std::string> options,
    int selected_index = -1,
    std::function<void(int)> on_change = nullptr) {
    auto group = std::make_unique<tile_radio_group<Backend>>();
    for (const auto& option : options) {
        group->add_option(option);
    }
    if (selected_index >= 0) {
        group->set_selected_index(selected_index);
    }
    if (on_change) {
        group->selection_changed.connect(std::move(on_change));
    }
    return group;
}

// ============================================================================
// Composite Widget Factories
// ============================================================================

/**
 * @brief Create a labeled slider (label + slider in hbox)
 */
template<UIBackend Backend>
[[nodiscard]] std::unique_ptr<hbox<Backend>> make_labeled_slider(
    const std::string& label_text,
    int value = 0,
    int min = 0,
    int max = 100,
    std::function<void(int)> on_change = nullptr) {
    auto container = std::make_unique<hbox<Backend>>(spacing::small);

    container->add_child(make_label<Backend>(label_text));

    auto slider = make_slider<Backend>(value, min, max, std::move(on_change));
    container->add_child(std::move(slider));

    return container;
}

/**
 * @brief Create a labeled text input (label + input in hbox)
 */
template<UIBackend Backend>
[[nodiscard]] std::unique_ptr<hbox<Backend>> make_labeled_input(
    const std::string& label_text,
    const std::string& initial_text = "",
    const std::string& placeholder = "",
    std::function<void(const std::string&)> on_change = nullptr) {
    auto container = std::make_unique<hbox<Backend>>(spacing::small);

    container->add_child(make_label<Backend>(label_text));

    auto input = make_text_input<Backend>(initial_text, placeholder, std::move(on_change));
    container->add_child(std::move(input));

    return container;
}

/**
 * @brief Create a labeled combo box (label + combo in hbox)
 */
template<UIBackend Backend>
[[nodiscard]] std::unique_ptr<hbox<Backend>> make_labeled_combo(
    const std::string& label_text,
    std::initializer_list<std::string> items,
    int selected_index = 0,
    std::function<void(int)> on_change = nullptr) {
    auto container = std::make_unique<hbox<Backend>>(spacing::small);

    container->add_child(make_label<Backend>(label_text));

    auto combo = make_combo_box<Backend>(items, selected_index, std::move(on_change));
    container->add_child(std::move(combo));

    return container;
}

/**
 * @brief Create a button row (multiple buttons in hbox)
 */
template<UIBackend Backend>
[[nodiscard]] std::unique_ptr<hbox<Backend>> make_button_row(
    std::initializer_list<std::pair<std::string, std::function<void()>>> buttons,
    spacing button_spacing = spacing::medium) {
    auto container = std::make_unique<hbox<Backend>>(button_spacing);

    for (const auto& [text, handler] : buttons) {
        container->add_child(make_button<Backend>(text, handler));
    }

    return container;
}

/**
 * @brief Create OK/Cancel button pair
 */
template<UIBackend Backend>
[[nodiscard]] std::unique_ptr<hbox<Backend>> make_ok_cancel_buttons(
    std::function<void()> on_ok,
    std::function<void()> on_cancel,
    const std::string& ok_text = "OK",
    const std::string& cancel_text = "Cancel") {
    auto container = std::make_unique<hbox<Backend>>(spacing::medium);

    container->add_child(make_button<Backend>(ok_text, std::move(on_ok)));
    container->add_child(make_button<Backend>(cancel_text, std::move(on_cancel)));

    return container;
}

/**
 * @brief Create Yes/No button pair
 */
template<UIBackend Backend>
[[nodiscard]] std::unique_ptr<hbox<Backend>> make_yes_no_buttons(
    std::function<void()> on_yes,
    std::function<void()> on_no) {
    return make_ok_cancel_buttons<Backend>(
        std::move(on_yes), std::move(on_no), "Yes", "No");
}

/**
 * @brief Create Yes/No/Cancel button triple
 */
template<UIBackend Backend>
[[nodiscard]] std::unique_ptr<hbox<Backend>> make_yes_no_cancel_buttons(
    std::function<void()> on_yes,
    std::function<void()> on_no,
    std::function<void()> on_cancel) {
    auto container = std::make_unique<hbox<Backend>>(spacing::medium);

    container->add_child(make_button<Backend>("Yes", std::move(on_yes)));
    container->add_child(make_button<Backend>("No", std::move(on_no)));
    container->add_child(make_button<Backend>("Cancel", std::move(on_cancel)));

    return container;
}

// ============================================================================
// Panel Factories
// ============================================================================

/**
 * @brief Create a panel with a title label
 */
template<UIBackend Backend>
[[nodiscard]] std::unique_ptr<tile_panel<Backend>> make_titled_panel(
    const std::string& title,
    int panel_spacing = 4) {
    auto panel = std::make_unique<tile_panel<Backend>>(nullptr, panel_spacing);

    auto title_label = make_label<Backend>(title);
    panel->add_child(std::move(title_label));

    return panel;
}

/**
 * @brief Create a form layout (labels and inputs in a vbox)
 */
template<UIBackend Backend>
[[nodiscard]] std::unique_ptr<vbox<Backend>> make_form(
    std::initializer_list<std::pair<std::string, std::string>> fields,
    spacing form_spacing = spacing::small) {
    auto form = std::make_unique<vbox<Backend>>(form_spacing);

    for (const auto& [label, placeholder] : fields) {
        form->add_child(make_labeled_input<Backend>(label, "", placeholder));
    }

    return form;
}

// ============================================================================
// Dialog Factories
// ============================================================================

/**
 * @struct dialog_result
 * @brief Container for dialog widgets to allow access after creation
 */
template<UIBackend Backend>
struct dialog_result {
    std::unique_ptr<tile_panel<Backend>> panel;
    tile_label<Backend>* title_label = nullptr;
    tile_label<Backend>* message_label = nullptr;
};

/**
 * @brief Create a simple message dialog
 */
template<UIBackend Backend>
[[nodiscard]] dialog_result<Backend> make_message_dialog(
    const std::string& title,
    const std::string& message,
    std::function<void()> on_ok = nullptr) {
    dialog_result<Backend> result;

    result.panel = std::make_unique<tile_panel<Backend>>(nullptr, 8);

    // Title
    auto title_label = make_label<Backend>(title);
    result.title_label = title_label.get();
    result.panel->add_child(std::move(title_label));

    // Message
    auto message_label = make_label<Backend>(message);
    result.message_label = message_label.get();
    result.panel->add_child(std::move(message_label));

    // OK button
    result.panel->add_child(make_button<Backend>("OK", std::move(on_ok)));

    return result;
}

/**
 * @brief Create a confirmation dialog
 */
template<UIBackend Backend>
[[nodiscard]] dialog_result<Backend> make_confirm_dialog(
    const std::string& title,
    const std::string& message,
    std::function<void()> on_yes,
    std::function<void()> on_no) {
    dialog_result<Backend> result;

    result.panel = std::make_unique<tile_panel<Backend>>(nullptr, 8);

    // Title
    auto title_label = make_label<Backend>(title);
    result.title_label = title_label.get();
    result.panel->add_child(std::move(title_label));

    // Message
    auto message_label = make_label<Backend>(message);
    result.message_label = message_label.get();
    result.panel->add_child(std::move(message_label));

    // Buttons
    result.panel->add_child(make_yes_no_buttons<Backend>(
        std::move(on_yes), std::move(on_no)));

    return result;
}

/**
 * @struct input_dialog_result
 * @brief Container for input dialog widgets
 */
template<UIBackend Backend>
struct input_dialog_result {
    std::unique_ptr<tile_panel<Backend>> panel;
    tile_label<Backend>* title_label = nullptr;
    tile_label<Backend>* prompt_label = nullptr;
    tile_text_input<Backend>* input = nullptr;
};

/**
 * @brief Create a text input dialog
 */
template<UIBackend Backend>
[[nodiscard]] input_dialog_result<Backend> make_input_dialog(
    const std::string& title,
    const std::string& prompt,
    const std::string& default_value = "",
    std::function<void(const std::string&)> on_ok = nullptr,
    std::function<void()> on_cancel = nullptr) {
    input_dialog_result<Backend> result;

    result.panel = std::make_unique<tile_panel<Backend>>(nullptr, 8);

    // Title
    auto title_label = make_label<Backend>(title);
    result.title_label = title_label.get();
    result.panel->add_child(std::move(title_label));

    // Prompt
    auto prompt_label = make_label<Backend>(prompt);
    result.prompt_label = prompt_label.get();
    result.panel->add_child(std::move(prompt_label));

    // Input field
    auto input = make_text_input<Backend>(default_value);
    result.input = input.get();
    result.panel->add_child(std::move(input));

    // Buttons - capture input pointer for OK handler
    auto* input_ptr = result.input;
    auto buttons = std::make_unique<hbox<Backend>>(spacing::medium);
    buttons->add_child(make_button<Backend>("OK", [input_ptr, handler = std::move(on_ok)]() {
        if (handler && input_ptr) {
            handler(input_ptr->text());
        }
    }));
    buttons->add_child(make_button<Backend>("Cancel", std::move(on_cancel)));
    result.panel->add_child(std::move(buttons));

    return result;
}

// ============================================================================
// Progress Dialog Factory
// ============================================================================

/**
 * @struct progress_dialog_result
 * @brief Container for progress dialog widgets
 */
template<UIBackend Backend>
struct progress_dialog_result {
    std::unique_ptr<tile_panel<Backend>> panel;
    tile_label<Backend>* title_label = nullptr;
    tile_label<Backend>* status_label = nullptr;
    tile_progress_bar<Backend>* progress_bar = nullptr;
};

/**
 * @brief Create a progress dialog
 */
template<UIBackend Backend>
[[nodiscard]] progress_dialog_result<Backend> make_progress_dialog(
    const std::string& title,
    const std::string& status = "Please wait...",
    bool indeterminate = false) {
    progress_dialog_result<Backend> result;

    result.panel = std::make_unique<tile_panel<Backend>>(nullptr, 8);

    // Title
    auto title_label = make_label<Backend>(title);
    result.title_label = title_label.get();
    result.panel->add_child(std::move(title_label));

    // Status
    auto status_label = make_label<Backend>(status);
    result.status_label = status_label.get();
    result.panel->add_child(std::move(status_label));

    // Progress bar
    std::unique_ptr<tile_progress_bar<Backend>> progress;
    if (indeterminate) {
        progress = make_indeterminate_progress<Backend>();
    } else {
        progress = make_progress_bar<Backend>(0, 0, 100);
    }
    result.progress_bar = progress.get();
    result.panel->add_child(std::move(progress));

    return result;
}

// ============================================================================
// Menu/Toolbar Helpers
// ============================================================================

/**
 * @struct menu_item_def
 * @brief Definition for a menu item
 */
struct menu_item_def {
    std::string text;
    std::function<void()> action;
    bool enabled = true;
};

/**
 * @brief Create a vertical menu from item definitions
 */
template<UIBackend Backend>
[[nodiscard]] std::unique_ptr<vbox<Backend>> make_menu(
    std::initializer_list<menu_item_def> items,
    spacing menu_spacing = spacing::tiny) {
    auto menu = std::make_unique<vbox<Backend>>(menu_spacing);

    for (const auto& item : items) {
        auto button = make_button<Backend>(item.text, item.action);
        if (!item.enabled) {
            button->set_enabled(false);
        }
        menu->add_child(std::move(button));
    }

    return menu;
}

/**
 * @brief Create a horizontal toolbar from item definitions
 */
template<UIBackend Backend>
[[nodiscard]] std::unique_ptr<hbox<Backend>> make_toolbar(
    std::initializer_list<menu_item_def> items,
    spacing toolbar_spacing = spacing::small) {
    auto toolbar = std::make_unique<hbox<Backend>>(toolbar_spacing);

    for (const auto& item : items) {
        auto button = make_button<Backend>(item.text, item.action);
        if (!item.enabled) {
            button->set_enabled(false);
        }
        toolbar->add_child(std::move(button));
    }

    return toolbar;
}

// ============================================================================
// Settings Panel Factory
// ============================================================================

/**
 * @struct setting_def
 * @brief Definition for a setting control
 */
struct setting_def {
    enum class type { checkbox, slider, combo };

    std::string label;
    type control_type;

    // For checkbox
    bool checkbox_value = false;
    std::function<void(bool)> on_checkbox_change;

    // For slider
    int slider_value = 0;
    int slider_min = 0;
    int slider_max = 100;
    std::function<void(int)> on_slider_change;

    // For combo
    std::vector<std::string> combo_items;
    int combo_selected = 0;
    std::function<void(int)> on_combo_change;
};

/**
 * @brief Create a settings panel from definitions
 */
template<UIBackend Backend>
[[nodiscard]] std::unique_ptr<vbox<Backend>> make_settings_panel(
    std::initializer_list<setting_def> settings,
    spacing panel_spacing = spacing::medium) {
    auto panel = std::make_unique<vbox<Backend>>(panel_spacing);

    for (const auto& setting : settings) {
        switch (setting.control_type) {
            case setting_def::type::checkbox: {
                auto checkbox = make_checkbox<Backend>(
                    setting.label,
                    setting.checkbox_value,
                    setting.on_checkbox_change);
                panel->add_child(std::move(checkbox));
                break;
            }

            case setting_def::type::slider: {
                auto row = make_labeled_slider<Backend>(
                    setting.label,
                    setting.slider_value,
                    setting.slider_min,
                    setting.slider_max,
                    setting.on_slider_change);
                panel->add_child(std::move(row));
                break;
            }

            case setting_def::type::combo: {
                auto row = std::make_unique<hbox<Backend>>(spacing::small);
                row->add_child(make_label<Backend>(setting.label));

                auto combo = make_combo_box<Backend>(
                    setting.combo_items,
                    setting.combo_selected,
                    setting.on_combo_change);
                row->add_child(std::move(combo));

                panel->add_child(std::move(row));
                break;
            }
        }
    }

    return panel;
}

} // namespace onyxui::tile
