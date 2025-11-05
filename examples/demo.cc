//
// Created by igor on 20/10/2025.
//

#include <onyxui/conio/conio_backend.hh>
#include <onyxui/ui_handle.hh>
#include "demo.hh"

int main([[maybe_unused]] int argc,[[maybe_unused]]  char* argv[]) {
     try {
        onyxui::scoped_ui_context<onyxui::conio::conio_backend> ui_ctx;
        auto widget = std::make_unique<main_widget<onyxui::conio::conio_backend>>();
        auto* widget_ptr = widget.get();
        onyxui::ui_handle<onyxui::conio::conio_backend> ui(std::move(widget));

        // Give widget access to renderer for screenshot functionality
        widget_ptr->set_renderer(&ui.renderer());

        while (!widget_ptr->should_quit()) {
            ui.display();
            ui.present();

            // Use non-blocking peek with 16ms timeout (~60fps refresh rate)
            // This allows periodic redraws for layer changes without explicit flag checking
            tb_event ev;
            int poll_result = tb_peek_event(&ev, 16);

            // If timeout or interrupt, continue to next frame (allows layer change redraws)
            if (poll_result != TB_OK) {
                continue;  // Timeout or error - redraw on next iteration
            }

            // Handle Ctrl+C specially (always quit)
            if (ev.type == TB_EVENT_KEY && ev.ch == 'c' && (ev.mod & TB_MOD_CTRL)) {
                widget_ptr->quit();
                continue;
            }

            // Debug keyboard events
            if (ev.type == TB_EVENT_KEY) {
                std::cerr << "[demo] Keyboard event: key=" << ev.key << " ch=" << (char)ev.ch << " mod=" << ev.mod << "\n";
            }
            if (ev.type == TB_EVENT_MOUSE) {
                std::cerr << "[demo] Mouse event: x=" << ev.x << " y=" << ev.y << " button=" << ev.key << "\n";
            }

            // Route all events through ui_handle's event system
            bool handled = ui.handle_event(ev);
            std::cerr << "[demo] Event handled: " << handled << "\n";

            // If ESC wasn't handled by UI (no menu open), quit
            if (ev.type == TB_EVENT_KEY && ev.key == TB_KEY_ESC && !handled) {
                widget_ptr->quit();
                continue;
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