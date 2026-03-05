//
// Created by Claude Code on 2025-11-23.
//
// OnyxUI Widgets Demo - Entry Point
// Comprehensive demonstration of all OnyxUI framework features
//
// Backend selection via CMake:
//   -DONYXUI_USE_CONIO_BACKEND  for conio (terminal)
//   -DONYXUI_USE_SDLPP_BACKEND  for SDL++ (graphical)
//

#include <onyxui/ui_handle.hh>
#include "widgets_demo.hh"  // Includes backend_config.hh which defines Backend

// ============================================================================
// Application Entry Point
// ============================================================================

#if defined(ONYXUI_USE_SDLPP_BACKEND)

int main([[maybe_unused]] int argc, [[maybe_unused]] char* argv[]) {
    // Use run_app for simple standalone application
    return Backend::run_app<widgets_demo>(
        "OnyxUI Widgets Demo",
        1024,
        768,
        [](widgets_demo& widget) {
            // Setup callback - widget is ready but renderer not yet available
            // Note: screenshot functionality requires renderer access
        }
    );
}

#else // ONYXUI_USE_CONIO_BACKEND

int main([[maybe_unused]] int argc, [[maybe_unused]] char* argv[]) {
    try {
        // Create UI context (registers themes, services)
        onyxui::scoped_ui_context<Backend> ui_ctx{onyxui::make_terminal_metrics<Backend>()};

        // Create main widget
        auto widget = std::make_unique<widgets_demo>();
        auto* widget_ptr = widget.get();

        // Create UI handle (this initializes termbox via vram)
        onyxui::ui_handle<Backend> ui(std::move(widget));

        // Give widget access to renderer for screenshot functionality
        widget_ptr->set_renderer(&ui.renderer());

        // Initial display
        ui.display();
        ui.present();

        // Main event loop
        while (!widget_ptr->should_quit()) {
            // Use blocking poll for events via backend wrapper
            tb_event ev;
            int poll_result = onyxui::conio::conio_poll_event(&ev);

            // If error or interrupt, skip
            if (poll_result != TB_OK) {
                continue;
            }

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

            // Redraw after event
            ui.display();
            ui.present();
        }

        // Cleanup handled by ui_handle/vram destructor
        return 0;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}

#endif
