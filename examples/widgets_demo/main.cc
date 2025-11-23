//
// Created by Claude Code on 2025-11-23.
//
// OnyxUI Widgets Demo - Entry Point
// Comprehensive demonstration of all OnyxUI framework features
//

#include <onyxui/conio/conio_backend.hh>
#include <onyxui/ui_handle.hh>
#include "widgets_demo.hh"

int main([[maybe_unused]] int argc, [[maybe_unused]] char* argv[]) {
    try {
        // Create UI context (registers themes, services)
        onyxui::scoped_ui_context<onyxui::conio::conio_backend> ui_ctx;

        // Create main widget
        auto widget = std::make_unique<widgets_demo<onyxui::conio::conio_backend>>();
        auto* widget_ptr = widget.get();

        // Create UI handle
        onyxui::ui_handle<onyxui::conio::conio_backend> ui(std::move(widget));

        // Give widget access to renderer for screenshot functionality
        widget_ptr->set_renderer(&ui.renderer());

        // Initial display
        ui.display();
        ui.present();

        // Main event loop
        while (!widget_ptr->should_quit()) {
            // Use blocking poll for events
            // This prevents continuous redraws when nothing is happening
            tb_event ev;
            int poll_result = tb_poll_event(&ev);

            // If error or interrupt, skip
            if (poll_result != TB_OK) {
                continue;  // Error - skip this iteration
            }

            // Always redraw after an event (something might have changed)
            bool needs_redraw = true;

            // Handle Ctrl+C specially (always quit)
            if (ev.type == TB_EVENT_KEY && ev.ch == 'c' && (ev.mod & TB_MOD_CTRL)) {
                widget_ptr->quit();
                continue;
            }

            // Route all events through ui_handle's event system
            bool handled = ui.handle_event(ev);

            // If ESC wasn't handled by UI (no menu open), quit
            if (ev.type == TB_EVENT_KEY && ev.key == TB_KEY_ESC && !handled) {
                widget_ptr->quit();
                continue;
            }

            // Only redraw if the event caused changes
            if (needs_redraw) {
                ui.display();
                ui.present();
            }
        }

        // Context automatically cleaned up on scope exit
        return 0;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;

        // Context automatically cleaned up on scope exit
        return 1;
    }
}
