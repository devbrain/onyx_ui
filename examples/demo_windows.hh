//
// Created by Claude Code on 2025-11-09.
//
// Window demonstration helpers for OnyxUI demo
// Creates and manages demo windows showcasing the window system
//

#pragma once
#include <onyxui/widgets/window/window.hh>
#include <onyxui/widgets/window/dialog.hh>
#include <onyxui/widgets/containers/vbox.hh>
#include <onyxui/widgets/label.hh>
#include <onyxui/widgets/button.hh>
#include <onyxui/widgets/text_view.hh>
#include <onyxui/services/ui_services.hh>
#include <vector>
#include <memory>
#include <algorithm>
#include <iostream>

namespace demo_windows {

    // Storage for active windows (keeps them alive)
    template<onyxui::UIBackend Backend>
    inline std::vector<std::shared_ptr<onyxui::window<Backend>>>& get_active_windows() {
        static std::vector<std::shared_ptr<onyxui::window<Backend>>> windows;
        return windows;
    }

    // Helper to register a window and auto-cleanup on close
    template<onyxui::UIBackend Backend>
    void register_window(std::shared_ptr<onyxui::window<Backend>> win) {
        auto& windows = get_active_windows<Backend>();
        windows.push_back(win);

        // Auto-cleanup when window closes
        win->closed.connect([win]() {
            auto& windows = get_active_windows<Backend>();
            auto it = std::find(windows.begin(), windows.end(), win);
            if (it != windows.end()) {
                windows.erase(it);
            }
        });
    }

    /**
     * @brief Create and show a basic demo window with title and content
     * @tparam Backend UI backend type
     * @param title Window title
     * @param content_text Content to display in the window
     * @param workspace Workspace element for maximize bounds (nullptr for none)
     * @return Shared pointer to created window
     */
    template <onyxui::UIBackend Backend>
    std::shared_ptr<onyxui::window<Backend>> show_basic_window_with_workspace(
        const std::string& title,
        const std::string& content_text,
        onyxui::ui_element<Backend>* workspace = nullptr
    ) {

        // Create window with title and basic flags
        typename onyxui::window<Backend>::window_flags flags;
        flags.has_close_button = true;
        flags.has_minimize_button = true;
        flags.has_maximize_button = true;
        flags.has_menu_button = true;
        flags.is_resizable = true;
        flags.is_movable = true;

        auto win = std::make_shared<onyxui::window<Backend>>(title, flags, nullptr);  // No parent - floating layer

        // Create content FIRST
        auto content = std::make_unique<onyxui::vbox<Backend>>(1);
        content->template emplace_child<onyxui::label>(content_text);
        win->set_content(std::move(content));

        // Set initial size and position AFTER content is added
        win->set_size(40, 15);
        win->set_position(5, 3);

        // Set workspace for maximize behavior
        if (workspace) {
            win->set_workspace(workspace);
        }

        // Register window to keep it alive
        register_window(win);

        // Show window (automatically registers with window_manager)
        win->show();

        return win;
    }

    /**
     * @brief Create and show a window with scrollable text content
     * @tparam Backend UI backend type
     * @param workspace Workspace element for maximize bounds (nullptr for none)
     */
    template <onyxui::UIBackend Backend>
    void show_scrollable_window(onyxui::ui_element<Backend>* workspace = nullptr) {

        typename onyxui::window<Backend>::window_flags flags;
        flags.has_close_button = true;
        flags.has_minimize_button = true;
        flags.has_maximize_button = true;
        flags.has_menu_button = true;
        flags.is_resizable = true;
        flags.is_movable = true;
        flags.is_scrollable = true;  // Enable scrollbars for overflowing content

        auto win = std::make_shared<onyxui::window<Backend>>("Scrollable Content", flags);

        // Create content with text view
        auto content = std::make_unique<onyxui::vbox<Backend>>(0);

        auto* text_view = content->template emplace_child<onyxui::text_view>();
        text_view->set_has_border(true);
        text_view->set_horizontal_align(onyxui::horizontal_alignment::stretch);
        text_view->set_vertical_align(onyxui::vertical_alignment::stretch);

        // Generate demo content
        std::string demo_text =
            "Scrollable Window Demo\n"
            "======================\n"
            "\n"
            "This window demonstrates:\n"
            "  * Window with scrollable content\n"
            "  * Text view inside window\n"
            "  * Automatic scrollbar management\n"
            "  * Window dragging via title bar\n"
            "  * Window resizing (if enabled)\n"
            "  * Close/minimize/maximize buttons\n"
            "\n"
            "Navigation:\n"
            "  - Arrow keys: Scroll line by line\n"
            "  - Page Up/Down: Scroll by page\n"
            "  - Home: Jump to top\n"
            "  - End: Jump to bottom\n"
            "\n"
            "Sample Log Entries:\n";

        for (int i = 1; i <= 30; i++) {
            demo_text += "[" + std::to_string(i) + "] Log entry at " +
                        std::to_string(1000 + (i * 50)) + " ms\n";
        }

        text_view->set_text(demo_text);

        win->set_content(std::move(content));

        // Set initial size and position AFTER content is added
        win->set_size(50, 20);
        win->set_position(10, 5);

        // Set workspace for maximize behavior
        if (workspace) {
            win->set_workspace(workspace);
        }

        // Register window to keep it alive
        register_window(win);

        // Show window
        win->show();
    }

    /**
     * @brief Create and show a modal dialog
     * @tparam Backend UI backend type
     * @param message Dialog message
     */
    template <onyxui::UIBackend Backend>
    void show_modal_dialog(const std::string& message) {

        auto dialog = std::make_shared<onyxui::dialog<Backend>>("Information");
        dialog->set_size(40, 10);
        dialog->set_position(15, 8);

        // Create content
        auto content = std::make_unique<onyxui::vbox<Backend>>(2);
        content->set_padding(onyxui::thickness::all(2));

        auto* msg_label = content->template emplace_child<onyxui::label>(message);
        msg_label->set_horizontal_align(onyxui::horizontal_alignment::center);

        // Add OK button
        auto* ok_btn = content->template emplace_child<onyxui::button>("OK");
        ok_btn->set_horizontal_align(onyxui::horizontal_alignment::center);
        ok_btn->clicked.connect([dialog]() {
            dialog->close();
        });

        dialog->set_content(std::move(content));

        // Register window to keep it alive (cast to base)
        register_window(std::static_pointer_cast<onyxui::window<Backend>>(dialog));

        // Show as modal (blocks other windows)
        dialog->show_modal();
    }

    /**
     * @brief Create and show a window with interactive controls
     * @tparam Backend UI backend type
     * @param workspace Workspace element for maximize bounds (nullptr for none)
     */
    template <onyxui::UIBackend Backend>
    void show_controls_window(onyxui::ui_element<Backend>* workspace = nullptr) {

        typename onyxui::window<Backend>::window_flags flags;
        flags.has_close_button = true;
        flags.has_minimize_button = true;
        flags.has_maximize_button = true;
        flags.has_menu_button = true;
        flags.is_resizable = false;
        flags.is_movable = true;
        flags.is_scrollable = true;  // Enable scrollbars for overflowing content

        auto win = std::make_shared<onyxui::window<Backend>>("Interactive Controls", flags);

        auto content = std::make_unique<onyxui::vbox<Backend>>(1);

        // Title
        auto* title = content->template emplace_child<onyxui::label>(
            "Window Control Demonstration"
        );
        title->set_horizontal_align(onyxui::horizontal_alignment::center);

        std::cout << "[DEBUG Interactive Controls] Created title label\n";

        // Spacer
        content->template emplace_child<onyxui::label>("");

        // Description
        content->template emplace_child<onyxui::label>(
            "This window demonstrates:"
        );
        content->template emplace_child<onyxui::label>(
            "  * Draggable title bar"
        );
        content->template emplace_child<onyxui::label>(
            "  * Minimize/Maximize/Close buttons"
        );
        content->template emplace_child<onyxui::label>(
            "  * System menu (click hamburger icon)"
        );
        content->template emplace_child<onyxui::label>(
            "  * Window focus management"
        );

        // Spacer
        content->template emplace_child<onyxui::label>("");

        // Add action buttons
        auto* spawn_btn = content->template emplace_child<onyxui::button>(
            "Spawn Another Window"
        );
        spawn_btn->set_horizontal_align(onyxui::horizontal_alignment::center);
        spawn_btn->clicked.connect([workspace]() {
            show_basic_window_with_workspace<Backend>(
                "Spawned Window",
                "This window was spawned from another window!",
                workspace  // Pass workspace through
            );
        });

        auto* dialog_btn = content->template emplace_child<onyxui::button>(
            "Show Modal Dialog"
        );
        dialog_btn->set_horizontal_align(onyxui::horizontal_alignment::center);
        dialog_btn->clicked.connect([]() {
            show_modal_dialog<Backend>(
                "This is a modal dialog!\nIt blocks interaction with other windows."
            );
        });

        std::cout << "[DEBUG Interactive Controls] About to set content\n";
        win->set_content(std::move(content));
        std::cout << "[DEBUG Interactive Controls] Content set\n";

        // Set initial size and position AFTER content is added
        std::cout << "[DEBUG Interactive Controls] About to set size 45x18\n";
        win->set_size(45, 18);
        std::cout << "[DEBUG Interactive Controls] Size set\n";

        win->set_position(8, 4);
        std::cout << "[DEBUG Interactive Controls] Position set to (8, 4)\n";

        // Set workspace for maximize behavior
        if (workspace) {
            win->set_workspace(workspace);
        }

        // Register window to keep it alive
        register_window(win);

        // Show window
        std::cout << "[DEBUG Interactive Controls] About to show window\n";
        win->show();
        std::cout << "[DEBUG Interactive Controls] Window shown\n";

        // Check final content bounds
        auto* final_content = win->get_content();
        if (final_content) {
            auto fc_bounds = final_content->bounds();
            std::cout << "[DEBUG Interactive Controls] Final content bounds: ("
                      << fc_bounds.x() << ", " << fc_bounds.y() << ", "
                      << fc_bounds.width() << ", " << fc_bounds.height() << ")\n";
        } else {
            std::cout << "[DEBUG Interactive Controls] Final content is NULL!\n";
        }
    }

} // namespace demo_windows
