/**
 * @file sdlpp_backend_impl.hh
 * @brief Template implementation for sdlpp_backend::run_app
 *
 * This file is included at the end of sdlpp_backend.hh
 */

#pragma once

#include <sdlpp/core/core.hh>
#include <sdlpp/video/window.hh>
#include <sdlpp/video/renderer.hh>
#include <sdlpp/events/events.hh>


namespace onyxui::sdlpp {

template<template<typename> class Widget>
int sdlpp_backend::run_app(
    const char* title,
    int width,
    int height,
    std::function<void(Widget<sdlpp_backend>&)> setup)
{
    try {
        // Initialize SDL
        ::sdlpp::init sdl(::sdlpp::init_flags::video);

        // Create window
        auto window_result = ::sdlpp::window::create(
            title, width, height,
            ::sdlpp::window_flags::resizable);

        if (!window_result) {
            std::cerr << "Failed to create window: " << window_result.error() << "\n";
            return 1;
        }

        auto& window = *window_result;

        // Create SDL renderer
        auto renderer_result = window.create_renderer();
        if (!renderer_result) {
            std::cerr << "Failed to create renderer: " << renderer_result.error() << "\n";
            return 1;
        }

        auto& sdl_renderer = *renderer_result;

        // Enable vsync to limit framerate and save CPU
        sdl_renderer.set_vsync(1);

        // Initialize backend with renderer
        if (!init(sdl_renderer)) {
            std::cerr << "Failed to initialize sdlpp_backend\n";
            return 1;
        }

        // Create UI context (registers themes, services)
        scoped_ui_context<sdlpp_backend> ui_ctx;

        // Create widget
        auto widget = std::make_unique<Widget<sdlpp_backend>>();
        auto* widget_ptr = widget.get();

        // Call setup callback if provided
        if (setup) {
            setup(*widget_ptr);
        }

        // Create OnyxUI renderer wrapping SDL renderer
        sdlpp_renderer onyx_renderer(sdl_renderer);

        // Create UI handle
        ui_handle<sdlpp_backend> ui(std::move(widget), std::move(onyx_renderer));

        // Clear quit flag
        clear_quit_flag();

        // Initial display
        ui.display();
        ui.present();

        // Main event loop
        // Check widget's should_quit if available, otherwise use backend's
        auto check_quit = [&]() {
            if constexpr (requires { widget_ptr->should_quit(); }) {
                if (widget_ptr->should_quit()) return true;
            }
            return should_quit();
        };

        while (!check_quit()) {
            // Process all pending events
            while (auto event = ::sdlpp::event_queue::poll()) {
                // Check for quit event
                if (event->type() == ::sdlpp::event_type::quit) {
                    goto exit_loop;
                }

                // Pass native event to ui_handle
                ui.handle_event(*event);
            }

            // Clear background
            sdl_renderer.set_draw_color(::sdlpp::color{192, 192, 192, 255});
            sdl_renderer.clear();

            // Display UI
            ui.display();

            // Present frame (with vsync this limits to ~60fps)
            sdl_renderer.present();
        }

    exit_loop:
        // Cleanup
        shutdown();

        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        shutdown();
        return 1;
    }
}

} // namespace onyxui::sdlpp
