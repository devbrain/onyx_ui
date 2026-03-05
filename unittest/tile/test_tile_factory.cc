/**
 * @file test_tile_factory.cc
 * @brief Unit tests for tile factory functions
 */

#include <doctest/doctest.h>
#include "../utils/test_helpers.hh"
#include <onyxui/tile/tile_factory.hh>
#include <onyxui/tile/tile_core.hh>
#include <stdexcept>

using namespace onyxui;
using namespace onyxui::tile;
using namespace onyxui::testing;

// ============================================================================
// Stub implementations (same as test_tile_widgets.cc)
// ============================================================================

namespace {
    const tile_theme* g_factory_test_theme = nullptr;
    tile_renderer* g_factory_test_renderer = nullptr;
}

namespace onyxui::tile {
    // Use existing stubs from test_tile_widgets - just need theme/renderer access
}

using Backend = test_canvas_backend;

// ============================================================================
// Basic Widget Factory Tests
// ============================================================================

TEST_SUITE("tile_factory_basic_widgets") {
    TEST_CASE("make_label creates label") {
        auto label = make_label<Backend>("Test Label");

        CHECK(label != nullptr);
        CHECK(label->text() == "Test Label");
        CHECK_FALSE(label->is_centered());
    }

    TEST_CASE("make_label_centered creates centered label") {
        auto label = make_label_centered<Backend>("Centered");

        CHECK(label != nullptr);
        CHECK(label->text() == "Centered");
        CHECK(label->is_centered());
    }

    TEST_CASE("make_button creates button") {
        auto button = make_button<Backend>("Click Me");

        CHECK(button != nullptr);
        CHECK(button->text() == "Click Me");
    }

    TEST_CASE("make_button with handler") {
        bool clicked = false;
        auto button = make_button<Backend>("Test", [&clicked]() {
            clicked = true;
        });

        button->clicked.emit();
        CHECK(clicked);
    }

    TEST_CASE("make_checkbox creates checkbox") {
        auto checkbox = make_checkbox<Backend>("Enable", true);

        CHECK(checkbox != nullptr);
        CHECK(checkbox->text() == "Enable");
        CHECK(checkbox->is_checked());
    }

    TEST_CASE("make_checkbox with handler") {
        bool last_value = true;
        auto checkbox = make_checkbox<Backend>("Test", false, [&last_value](bool val) {
            last_value = val;
        });

        checkbox->set_checked(true);
        CHECK(last_value == true);
    }

    TEST_CASE("make_tri_state_checkbox") {
        auto checkbox = make_tri_state_checkbox<Backend>(
            "Mixed", checkbox_state::indeterminate);

        CHECK(checkbox != nullptr);
        CHECK(checkbox->is_tri_state_enabled());
        CHECK(checkbox->get_state() == checkbox_state::indeterminate);
    }

    TEST_CASE("make_progress_bar creates progress bar") {
        auto progress = make_progress_bar<Backend>(50, 0, 100);

        CHECK(progress != nullptr);
        CHECK(progress->value() == 50);
        CHECK(progress->minimum() == 0);
        CHECK(progress->maximum() == 100);
    }

    TEST_CASE("make_indeterminate_progress") {
        auto progress = make_indeterminate_progress<Backend>();

        CHECK(progress != nullptr);
        CHECK(progress->is_indeterminate());
    }

    TEST_CASE("make_slider creates slider") {
        auto slider = make_slider<Backend>(25, 0, 50);

        CHECK(slider != nullptr);
        CHECK(slider->value() == 25);
        CHECK(slider->minimum() == 0);
        CHECK(slider->maximum() == 50);
    }

    TEST_CASE("make_slider with handler") {
        int last_value = -1;
        auto slider = make_slider<Backend>(0, 0, 100, [&last_value](int val) {
            last_value = val;
        });

        slider->set_value(42);
        CHECK(last_value == 42);
    }

    TEST_CASE("make_text_input creates input") {
        auto input = make_text_input<Backend>("Initial", "Placeholder");

        CHECK(input != nullptr);
        CHECK(input->text() == "Initial");
        CHECK(input->placeholder() == "Placeholder");
    }

    TEST_CASE("make_password_input creates password field") {
        auto input = make_password_input<Backend>("Enter password");

        CHECK(input != nullptr);
        CHECK(input->is_password_mode());
        CHECK(input->placeholder() == "Enter password");
    }

    TEST_CASE("make_combo_box with initializer list") {
        auto combo = make_combo_box<Backend>({"A", "B", "C"}, 1);

        CHECK(combo != nullptr);
        CHECK(combo->count() == 3);
        CHECK(combo->current_index() == 1);
        CHECK(combo->current_text() == "B");
    }

    TEST_CASE("make_combo_box with vector") {
        std::vector<std::string> items = {"X", "Y", "Z"};
        auto combo = make_combo_box<Backend>(items, 2);

        CHECK(combo != nullptr);
        CHECK(combo->count() == 3);
        CHECK(combo->current_index() == 2);
        CHECK(combo->current_text() == "Z");
    }

    TEST_CASE("make_combo_box with handler") {
        int last_index = -1;
        auto combo = make_combo_box<Backend>({"One", "Two"}, 0,
            [&last_index](int idx) {
                last_index = idx;
            });

        combo->set_current_index(1);
        CHECK(last_index == 1);
    }
}

// ============================================================================
// Scrollbar Factory Tests
// ============================================================================

TEST_SUITE("tile_factory_scrollbars") {
    TEST_CASE("make_v_scrollbar creates vertical scrollbar") {
        auto scrollbar = make_v_scrollbar<Backend>(500, 100);

        CHECK(scrollbar != nullptr);
        CHECK(scrollbar->get_orientation() == orientation::vertical);
        CHECK(scrollbar->content_size() == 500);
        CHECK(scrollbar->viewport_size() == 100);
    }

    TEST_CASE("make_h_scrollbar creates horizontal scrollbar") {
        auto scrollbar = make_h_scrollbar<Backend>(300, 50);

        CHECK(scrollbar != nullptr);
        CHECK(scrollbar->get_orientation() == orientation::horizontal);
        CHECK(scrollbar->content_size() == 300);
        CHECK(scrollbar->viewport_size() == 50);
    }
}

// ============================================================================
// Radio Group Factory Tests
// ============================================================================

TEST_SUITE("tile_factory_radio_group") {
    TEST_CASE("make_radio_group creates group with options") {
        auto group = make_radio_group<Backend>({"Small", "Medium", "Large"}, 1);

        CHECK(group != nullptr);
        CHECK(group->count() == 3);
        CHECK(group->get_selected_index() == 1);
    }

    TEST_CASE("make_radio_group with handler") {
        int last_index = -1;
        auto group = make_radio_group<Backend>({"A", "B"}, -1,
            [&last_index](int idx) {
                last_index = idx;
            });

        group->set_selected_index(0);
        CHECK(last_index == 0);
    }
}

// ============================================================================
// Composite Widget Factory Tests
// ============================================================================

TEST_SUITE("tile_factory_composites") {
    TEST_CASE("make_labeled_slider creates hbox with label and slider") {
        auto row = make_labeled_slider<Backend>("Volume:", 50, 0, 100);

        CHECK(row != nullptr);
        // hbox should have children
    }

    TEST_CASE("make_labeled_input creates hbox with label and input") {
        auto row = make_labeled_input<Backend>("Name:", "John", "Enter name");

        CHECK(row != nullptr);
    }

    TEST_CASE("make_labeled_combo creates hbox with label and combo") {
        auto row = make_labeled_combo<Backend>("Size:", {"S", "M", "L"}, 1);

        CHECK(row != nullptr);
    }

    TEST_CASE("make_button_row creates multiple buttons") {
        bool btn1_clicked = false;
        bool btn2_clicked = false;

        auto row = make_button_row<Backend>({
            {"Button 1", [&btn1_clicked]() { btn1_clicked = true; }},
            {"Button 2", [&btn2_clicked]() { btn2_clicked = true; }}
        });

        CHECK(row != nullptr);
    }

    TEST_CASE("make_ok_cancel_buttons creates button pair") {
        bool ok_clicked = false;
        bool cancel_clicked = false;

        auto buttons = make_ok_cancel_buttons<Backend>(
            [&ok_clicked]() { ok_clicked = true; },
            [&cancel_clicked]() { cancel_clicked = true; }
        );

        CHECK(buttons != nullptr);
    }

    TEST_CASE("make_yes_no_buttons creates button pair") {
        auto buttons = make_yes_no_buttons<Backend>(
            []() {},
            []() {}
        );

        CHECK(buttons != nullptr);
    }

    TEST_CASE("make_yes_no_cancel_buttons creates button triple") {
        auto buttons = make_yes_no_cancel_buttons<Backend>(
            []() {},
            []() {},
            []() {}
        );

        CHECK(buttons != nullptr);
    }
}

// ============================================================================
// Panel Factory Tests
// ============================================================================

TEST_SUITE("tile_factory_panels") {
    TEST_CASE("make_titled_panel creates panel with title") {
        auto panel = make_titled_panel<Backend>("Settings");

        CHECK(panel != nullptr);
    }

    TEST_CASE("make_form creates form layout") {
        auto form = make_form<Backend>({
            {"Name:", "Enter name"},
            {"Email:", "Enter email"},
            {"Phone:", "Enter phone"}
        });

        CHECK(form != nullptr);
    }
}

// ============================================================================
// Dialog Factory Tests
// ============================================================================

TEST_SUITE("tile_factory_dialogs") {
    TEST_CASE("make_message_dialog creates dialog") {
        bool ok_clicked = false;
        auto dialog = make_message_dialog<Backend>(
            "Info",
            "Operation complete!",
            [&ok_clicked]() { ok_clicked = true; }
        );

        CHECK(dialog.panel != nullptr);
        CHECK(dialog.title_label != nullptr);
        CHECK(dialog.message_label != nullptr);
        CHECK(dialog.title_label->text() == "Info");
        CHECK(dialog.message_label->text() == "Operation complete!");
    }

    TEST_CASE("make_confirm_dialog creates dialog") {
        bool yes_clicked = false;
        bool no_clicked = false;

        auto dialog = make_confirm_dialog<Backend>(
            "Confirm",
            "Are you sure?",
            [&yes_clicked]() { yes_clicked = true; },
            [&no_clicked]() { no_clicked = true; }
        );

        CHECK(dialog.panel != nullptr);
        CHECK(dialog.title_label != nullptr);
        CHECK(dialog.message_label != nullptr);
    }

    TEST_CASE("make_input_dialog creates dialog with input") {
        std::string result;
        auto dialog = make_input_dialog<Backend>(
            "Input",
            "Enter value:",
            "default",
            [&result](const std::string& val) { result = val; }
        );

        CHECK(dialog.panel != nullptr);
        CHECK(dialog.title_label != nullptr);
        CHECK(dialog.prompt_label != nullptr);
        CHECK(dialog.input != nullptr);
        CHECK(dialog.input->text() == "default");
    }

    TEST_CASE("make_progress_dialog creates dialog") {
        auto dialog = make_progress_dialog<Backend>(
            "Loading",
            "Please wait...",
            false
        );

        CHECK(dialog.panel != nullptr);
        CHECK(dialog.title_label != nullptr);
        CHECK(dialog.status_label != nullptr);
        CHECK(dialog.progress_bar != nullptr);
        CHECK_FALSE(dialog.progress_bar->is_indeterminate());
    }

    TEST_CASE("make_progress_dialog indeterminate") {
        auto dialog = make_progress_dialog<Backend>(
            "Processing",
            "Working...",
            true
        );

        CHECK(dialog.progress_bar != nullptr);
        CHECK(dialog.progress_bar->is_indeterminate());
    }
}

// ============================================================================
// Menu/Toolbar Factory Tests
// ============================================================================

TEST_SUITE("tile_factory_menus") {
    TEST_CASE("make_menu creates vertical menu") {
        bool item1_clicked = false;
        bool item2_clicked = false;

        auto menu = make_menu<Backend>({
            {"New", [&item1_clicked]() { item1_clicked = true; }},
            {"Open", [&item2_clicked]() { item2_clicked = true; }},
            {"Save", nullptr, false}  // Disabled
        });

        CHECK(menu != nullptr);
    }

    TEST_CASE("make_toolbar creates horizontal toolbar") {
        auto toolbar = make_toolbar<Backend>({
            {"Cut", []() {}},
            {"Copy", []() {}},
            {"Paste", []() {}}
        });

        CHECK(toolbar != nullptr);
    }
}

// ============================================================================
// Settings Panel Factory Tests
// ============================================================================

TEST_SUITE("tile_factory_settings") {
    TEST_CASE("make_settings_panel with checkbox") {
        bool sound_enabled = false;

        auto panel = make_settings_panel<Backend>({
            {
                .label = "Enable Sound",
                .control_type = setting_def::type::checkbox,
                .checkbox_value = true,
                .on_checkbox_change = [&sound_enabled](bool val) {
                    sound_enabled = val;
                }
            }
        });

        CHECK(panel != nullptr);
    }

    TEST_CASE("make_settings_panel with slider") {
        int volume = 0;

        auto panel = make_settings_panel<Backend>({
            {
                .label = "Volume",
                .control_type = setting_def::type::slider,
                .slider_value = 75,
                .slider_min = 0,
                .slider_max = 100,
                .on_slider_change = [&volume](int val) {
                    volume = val;
                }
            }
        });

        CHECK(panel != nullptr);
    }

    TEST_CASE("make_settings_panel with combo") {
        int quality = 0;

        auto panel = make_settings_panel<Backend>({
            {
                .label = "Quality",
                .control_type = setting_def::type::combo,
                .combo_items = {"Low", "Medium", "High"},
                .combo_selected = 1,
                .on_combo_change = [&quality](int val) {
                    quality = val;
                }
            }
        });

        CHECK(panel != nullptr);
    }

    TEST_CASE("make_settings_panel mixed controls") {
        auto panel = make_settings_panel<Backend>({
            {
                .label = "Fullscreen",
                .control_type = setting_def::type::checkbox,
                .checkbox_value = false
            },
            {
                .label = "Brightness",
                .control_type = setting_def::type::slider,
                .slider_value = 50,
                .slider_min = 0,
                .slider_max = 100
            },
            {
                .label = "Resolution",
                .control_type = setting_def::type::combo,
                .combo_items = {"800x600", "1024x768", "1920x1080"},
                .combo_selected = 2
            }
        });

        CHECK(panel != nullptr);
    }
}
