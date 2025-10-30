//
// Created by igor on 20/10/2025.
//

#include <onyxui/conio/conio_backend.hh>
#include <onyxui/ui_handle.hh>
#include <../include/onyxui/services/background_renderer.hh>
#include "demo.hh"

int main([[maybe_unused]] int argc,[[maybe_unused]]  char* argv[]) {
     try {
        // 1. Create UI context (provides layer manager, focus manager, theme registry, background)
        //    Themes are automatically registered by conio_backend::register_themes()
        onyxui::scoped_ui_context<onyxui::conio::conio_backend> ui_ctx;

        // 2. Display available themes (auto-registered by backend)
        std::cerr << "Available themes:" << std::endl;
        for (const auto& name : ui_ctx.themes().list_theme_names()) {
            if (const auto* theme = ui_ctx.themes().get_theme(name)) {
                std::cerr << "  - " << theme->name;
                if (!theme->description.empty()) {
                    std::cerr << " (" << theme->description << ")";
                }
                std::cerr << std::endl;
            }
        }
        std::cerr << std::endl;

        // 3. Create main widget (discovers themes from ui_services)
        //    main_widget constructor calls themes->set_current_theme("Norton Blue")
        //    which automatically synchronizes the background via signal/slot
        //    No manual background setup needed!
        //
        //    **Automatic Theme/Background Synchronization:**
        //    - Initial theme: Norton Blue (color{0, 0, 170} - dark blue)
        //    - Press 1: Norton Blue → background updates to dark blue (0, 0, 170)
        //    - Press 2: Borland Turbo → background updates to cyan (0, 170, 170)
        //    - Press 3: Midnight Commander → background updates to black (0, 0, 0)
        //    - Press 4: DOS Edit → background updates to white (255, 255, 255)
        //
        //    The background_renderer automatically receives theme_changed signals
        //    via scoped_connection in ui_context constructor. Zero manual sync!
        auto widget = std::make_unique<main_widget<onyxui::conio::conio_backend>>();

        // Keep reference to widget for event handling before moving it
        auto* widget_ptr = widget.get();

        // 5. Create UI handle with widget as root (no desktop widget needed!)
        onyxui::ui_handle<onyxui::conio::conio_backend> ui(std::move(widget));

       // wait_for_debug();

        // 8. Main loop - event-driven design with periodic refresh
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

            // Route all events through ui_handle's event system
            bool handled = ui.handle_event(ev);

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