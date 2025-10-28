//
// SDL2 Backend Demo Application
//

#include <onyxui/sdl2/sdl2_backend.hh>
#include <onyxui/ui_context.hh>
#include <onyxui/ui_services.hh>
#include <onyxui/widgets/button.hh>
#include <onyxui/widgets/label.hh>
#include <onyxui/widgets/vbox.hh>
#include <onyxui/widgets/panel.hh>

#include <iostream>
#include <memory>

using namespace onyxui;
using namespace onyxui::sdl2;

int main() {
    // Initialize SDL2
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL initialization failed: " << SDL_GetError() << "\n";
        return 1;
    }

    // Initialize SDL_ttf
    if (TTF_Init() < 0) {
        std::cerr << "SDL_ttf initialization failed: " << TTF_GetError() << "\n";
        SDL_Quit();
        return 1;
    }

    // Create window
    SDL_Window* window = SDL_CreateWindow(
        "OnyxUI SDL2 Demo - Windows 3.11 Theme",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        800,
        600,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
    );

    if (!window) {
        std::cerr << "Window creation failed: " << SDL_GetError() << "\n";
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    try {
        // Create UI context and renderer
        scoped_ui_context<sdl2_backend> ui_ctx;
        sdl2_renderer renderer(window, 800, 600);

        // Create UI hierarchy
        auto root = create_vbox<sdl2_backend>();
        root->set_spacing(10);
        root->set_margin({10, 10, 10, 10});

        // Title label
        auto title = create_label<sdl2_backend>("Windows 3.11 Theme Demo");
        root->add_child(std::move(title));

        // Create panel with buttons
        auto panel = create_panel<sdl2_backend>();
        auto buttons_box = create_vbox<sdl2_backend>();
        buttons_box->set_spacing(5);

        // Add some buttons
        auto btn1 = create_button<sdl2_backend>("Button 1");
        auto btn2 = create_button<sdl2_backend>("Button 2");
        auto btn3 = create_button<sdl2_backend>("Disabled Button");
        btn3->set_enabled(false);

        buttons_box->add_child(std::move(btn1));
        buttons_box->add_child(std::move(btn2));
        buttons_box->add_child(std::move(btn3));

        panel->add_child(std::move(buttons_box));
        root->add_child(std::move(panel));

        // Info label
        auto info = create_label<sdl2_backend>("Classic Windows 3.11 Look and Feel");
        root->add_child(std::move(info));

        // Layout
        root->measure(800, 600);
        root->arrange(sdl2_backend::rect{0, 0, 800, 600});

        // Main loop
        bool running = true;
        SDL_Event event;

        while (running) {
            while (SDL_PollEvent(&event)) {
                if (event.type == SDL_QUIT) {
                    running = false;
                } else if (event.type == SDL_WINDOWEVENT &&
                           event.window.event == SDL_WINDOWEVENT_RESIZED) {
                    int new_w = event.window.data1;
                    int new_h = event.window.data2;
                    renderer.on_resize(new_w, new_h);
                    root->measure(new_w, new_h);
                    root->arrange(sdl2_backend::rect{0, 0, new_w, new_h});
                }

                // TODO: Dispatch events to UI tree
                // root->handle_event(event);
            }

            // Render
            renderer.set_background({192, 192, 192});  // Windows gray
            renderer.clear_region(renderer.get_viewport());

            // Get current theme (required for rendering)
            auto* themes = ui_services<sdl2_backend>::themes();
            if (!themes) {
                throw std::runtime_error("Theme service not initialized!");
            }
            auto* theme_ptr = themes->get_current_theme();
            if (!theme_ptr) {
                throw std::runtime_error("No current theme set!");
            }
            const auto& theme = *theme_ptr;

            root->render(&renderer, theme);
            renderer.present();

            SDL_Delay(16);  // ~60 FPS
        }

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();

    return 0;
}
