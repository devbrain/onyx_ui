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
        std::cerr << "[DEBUG] register_window: Window added to storage, use_count=" << win.use_count()
                  << ", total windows=" << windows.size() << std::endl;

        // Auto-cleanup when window closes
        win->closed.connect([win]() {
            std::cerr << "[DEBUG] Window closed signal received" << std::endl;
            auto& windows = get_active_windows<Backend>();
            auto it = std::find(windows.begin(), windows.end(), win);
            if (it != windows.end()) {
                windows.erase(it);
                std::cerr << "[DEBUG] Window removed from storage, remaining windows=" << windows.size() << std::endl;
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
        std::cerr << "[DEBUG] show_basic_window() called with title: '" << title << "'" << std::endl;

        // Create window with title and basic flags
        typename onyxui::window<Backend>::window_flags flags;
        flags.has_close_button = true;
        flags.has_minimize_button = true;
        flags.has_maximize_button = true;
        flags.has_menu_button = true;
        flags.is_resizable = true;
        flags.is_movable = true;

        std::cerr << "[DEBUG] Creating window..." << std::endl;
        auto win = std::make_shared<onyxui::window<Backend>>(title, flags, nullptr);  // No parent - floating layer
        std::cerr << "[DEBUG] Window created, use_count=" << win.use_count() << std::endl;

        // Create content FIRST
        auto content = std::make_unique<onyxui::vbox<Backend>>(1);
        content->template emplace_child<onyxui::label>(content_text);
        win->set_content(std::move(content));
        std::cerr << "[DEBUG] Window content set" << std::endl;

        // Set initial size and position AFTER content is added
        win->set_size(40, 15);
        win->set_position(5, 3);
        std::cerr << "[DEBUG] Window size set to 40x15, position set to (5,3)" << std::endl;

        // Set workspace for maximize behavior
        if (workspace) {
            win->set_workspace(workspace);
            std::cerr << "[DEBUG] Workspace set for maximize behavior" << std::endl;
        }

        // Register window to keep it alive
        register_window(win);
        std::cerr << "[DEBUG] Window registered, total windows=" << get_active_windows<Backend>().size()
                  << ", use_count=" << win.use_count() << std::endl;

        // Show window (automatically registers with window_manager)
        std::cerr << "[DEBUG] Calling win->show()..." << std::endl;
        win->show();
        std::cerr << "[DEBUG] win->show() completed" << std::endl;

        return win;
    }

    /**
     * @brief Create and show a window with scrollable text content
     * @tparam Backend UI backend type
     * @param workspace Workspace element for maximize bounds (nullptr for none)
     */
    template <onyxui::UIBackend Backend>
    void show_scrollable_window(onyxui::ui_element<Backend>* workspace = nullptr) {
        std::cerr << "[DEBUG] show_scrollable_window() called" << std::endl;

        typename onyxui::window<Backend>::window_flags flags;
        flags.has_close_button = true;
        flags.has_minimize_button = true;
        flags.has_maximize_button = true;
        flags.is_resizable = true;
        flags.is_movable = true;

        std::cerr << "[DEBUG] Creating scrollable window..." << std::endl;
        auto win = std::make_shared<onyxui::window<Backend>>("Scrollable Content", flags);
        std::cerr << "[DEBUG] Scrollable window created, use_count=" << win.use_count() << std::endl;
        win->set_size(50, 20);
        win->set_position(10, 5);
        std::cerr << "[DEBUG] Scrollable window size set to 50x20, position set to (10,5)" << std::endl;

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
        std::cerr << "[DEBUG] Scrollable window text content set" << std::endl;

        win->set_content(std::move(content));
        std::cerr << "[DEBUG] Scrollable window content set" << std::endl;

        // Set workspace for maximize behavior
        if (workspace) {
            win->set_workspace(workspace);
            std::cerr << "[DEBUG] Workspace set for maximize behavior" << std::endl;
        }

        // Register window to keep it alive
        register_window(win);
        std::cerr << "[DEBUG] Scrollable window registered, total windows=" << get_active_windows<Backend>().size()
                  << ", use_count=" << win.use_count() << std::endl;

        // Show window
        std::cerr << "[DEBUG] Calling win->show() for scrollable window..." << std::endl;
        win->show();
        std::cerr << "[DEBUG] Scrollable window show() completed" << std::endl;
    }

    /**
     * @brief Create and show a modal dialog
     * @tparam Backend UI backend type
     * @param message Dialog message
     */
    template <onyxui::UIBackend Backend>
    void show_modal_dialog(const std::string& message) {
        std::cerr << "[DEBUG] show_modal_dialog() called with message: '" << message << "'" << std::endl;

        std::cerr << "[DEBUG] Creating modal dialog..." << std::endl;
        auto dialog = std::make_shared<onyxui::dialog<Backend>>("Information");
        std::cerr << "[DEBUG] Dialog created, use_count=" << dialog.use_count() << std::endl;
        dialog->set_size(40, 10);
        dialog->set_position(15, 8);
        std::cerr << "[DEBUG] Dialog size set to 40x10, position set to (15,8)" << std::endl;

        // Create content
        auto content = std::make_unique<onyxui::vbox<Backend>>(2);
        content->set_padding(onyxui::thickness::all(2));

        auto* msg_label = content->template emplace_child<onyxui::label>(message);
        msg_label->set_horizontal_align(onyxui::horizontal_alignment::center);

        // Add OK button
        auto* ok_btn = content->template emplace_child<onyxui::button>("OK");
        ok_btn->set_horizontal_align(onyxui::horizontal_alignment::center);
        ok_btn->clicked.connect([dialog]() {
            std::cerr << "[DEBUG] Dialog OK button clicked" << std::endl;
            dialog->close();
        });
        std::cerr << "[DEBUG] Dialog content created with message and OK button" << std::endl;

        dialog->set_content(std::move(content));
        std::cerr << "[DEBUG] Dialog content set" << std::endl;

        // Register window to keep it alive (cast to base)
        register_window(std::static_pointer_cast<onyxui::window<Backend>>(dialog));
        std::cerr << "[DEBUG] Dialog registered, total windows=" << get_active_windows<Backend>().size()
                  << ", use_count=" << dialog.use_count() << std::endl;

        // Show as modal (blocks other windows)
        std::cerr << "[DEBUG] Calling dialog->show_modal()..." << std::endl;
        dialog->show_modal();
        std::cerr << "[DEBUG] Dialog show_modal() completed" << std::endl;
    }

    /**
     * @brief Create and show a window with interactive controls
     * @tparam Backend UI backend type
     * @param workspace Workspace element for maximize bounds (nullptr for none)
     */
    template <onyxui::UIBackend Backend>
    void show_controls_window(onyxui::ui_element<Backend>* workspace = nullptr) {
        std::cerr << "[DEBUG] show_controls_window() called" << std::endl;

        typename onyxui::window<Backend>::window_flags flags;
        flags.has_close_button = true;
        flags.has_minimize_button = true;
        flags.has_maximize_button = true;
        flags.has_menu_button = true;
        flags.is_resizable = false;
        flags.is_movable = true;

        std::cerr << "[DEBUG] Creating interactive controls window..." << std::endl;
        auto win = std::make_shared<onyxui::window<Backend>>("Interactive Controls", flags);
        std::cerr << "[DEBUG] Controls window created, use_count=" << win.use_count() << std::endl;
        win->set_size(45, 18);
        win->set_position(8, 4);
        std::cerr << "[DEBUG] Controls window size set to 45x18, position set to (8,4)" << std::endl;

        auto content = std::make_unique<onyxui::vbox<Backend>>(1);

        // Title
        auto* title = content->template emplace_child<onyxui::label>(
            "Window Control Demonstration"
        );
        title->set_horizontal_align(onyxui::horizontal_alignment::center);

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
        std::cerr << "[DEBUG] Controls window content created with buttons" << std::endl;

        win->set_content(std::move(content));
        std::cerr << "[DEBUG] Controls window content set" << std::endl;

        // Set workspace for maximize behavior
        if (workspace) {
            win->set_workspace(workspace);
            std::cerr << "[DEBUG] Workspace set for maximize behavior" << std::endl;
        }

        // Register window to keep it alive
        register_window(win);
        std::cerr << "[DEBUG] Controls window registered, total windows=" << get_active_windows<Backend>().size()
                  << ", use_count=" << win.use_count() << std::endl;

        // Show window
        std::cerr << "[DEBUG] Calling win->show() for controls window..." << std::endl;
        win->show();
        std::cerr << "[DEBUG] Controls window show() completed" << std::endl;
    }

} // namespace demo_windows
