/**
 * @file tile_backend_impl.hh
 * @brief Template implementation for sdlpp_tile_backend::run_app
 *
 * This file is included at the end of tile_backend.hh
 *
 * @note This file is part of the SDL backend (requires SDL3/lib_sdlpp).
 */

#pragma once

#include <sdlpp/core/core.hh>
#include <sdlpp/video/window.hh>
#include <sdlpp/video/renderer.hh>
#include <sdlpp/events/events.hh>

#include <onyxui/services/ui_context.hh>
#include <onyxui/services/ui_services.hh>
#include <onyxui/core/backend_metrics.hh>
#include <onyxui/events/hit_test_path.hh>
#include <onyxui/events/event_router.hh>

#include <iostream>
#include <chrono>

namespace onyxui::tile {

template<template<typename> class Widget>
int sdlpp_tile_backend::run_app(
    const char* title,
    int width,
    int height,
    std::function<void(Widget<sdlpp_tile_backend>&)> setup)
{
    try {
        // Verify theme is set
        if (!has_theme()) {
            std::cerr << "Error: No tile theme set. Call tile::set_theme() before run_app()\n";
            return 1;
        }

        // Create UI context with 1:1 pixel metrics (tile backend uses direct pixels)
        // This MUST be created before any widgets since they use ui_services::metrics()
        backend_metrics<sdlpp_tile_backend> tile_metrics{
            .logical_to_physical_x = 1.0,  // 1 logical unit = 1 pixel
            .logical_to_physical_y = 1.0,  // 1 logical unit = 1 pixel
            .dpi_scale = 1.0               // No DPI scaling for tile-based rendering
        };
        scoped_ui_context<sdlpp_tile_backend> ui_ctx(tile_metrics);

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

        // Enable vsync
        sdl_renderer.set_vsync(1);

        // Create tile renderer
        tile_renderer onyx_renderer(sdl_renderer);

        // Set up atlas from theme
        tile_atlas* atlas = get_theme().atlas;
        if (!atlas) {
            std::cerr << "Error: tile_theme.atlas is null\n";
            return 1;
        }
        if (!atlas->texture) {
            std::cerr << "Error: tile_theme.atlas->texture is null\n";
            return 1;
        }

        // Set texture type explicitly for sdlpp backend (required for debug validation)
        atlas->tex_type = texture_type::sdlpp;

        onyx_renderer.set_atlas(atlas);
        onyx_renderer.set_texture(static_cast<::sdlpp::texture*>(atlas->texture));

        // Register global renderer for tile widgets
        set_renderer(&onyx_renderer);

        // Create widget
        auto widget = std::make_unique<Widget<sdlpp_tile_backend>>();
        auto* widget_ptr = widget.get();

        // Call setup callback if provided
        if (setup) {
            setup(*widget_ptr);
        }

        // Track time for animations
        auto last_time = std::chrono::steady_clock::now();

        // Get initial viewport size
        auto viewport = onyx_renderer.get_viewport();
        int viewport_w = viewport.w;
        int viewport_h = viewport.h;

        bool quit = false;

        // Main event loop
        while (!quit) {
            // Calculate delta time
            auto now = std::chrono::steady_clock::now();
            auto delta = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_time);
            last_time = now;
            [[maybe_unused]] unsigned int delta_ms = static_cast<unsigned int>(delta.count());

            // Process all pending events
            while (auto event = ::sdlpp::event_queue::poll()) {
                if (event->type() == ::sdlpp::event_type::quit) {
                    quit = true;
                    break;
                }

                // Handle window resize
                if (event->type() == ::sdlpp::event_type::window_resized) {
                    viewport = onyx_renderer.get_viewport();
                    viewport_w = viewport.w;
                    viewport_h = viewport.h;
                }

                // Convert SDL event to ui_event and route properly
                auto ui_evt = sdlpp_tile_backend::create_event(*event);
                if (ui_evt) {
                    // Proper three-phase event routing with hit testing
                    if (auto* mouse_evt = std::get_if<mouse_event>(&*ui_evt)) {
                        // Hit test to find target widget (using logical coordinates)
                        hit_test_path<sdlpp_tile_backend> hit_path;
                        auto* target = widget_ptr->hit_test_logical(
                            mouse_evt->x,
                            mouse_evt->y,
                            hit_path
                        );

                        if (target && !hit_path.empty()) {
                            // Three-phase event routing: capture -> target -> bubble
                            route_event(*ui_evt, hit_path);
                        }
                    } else if (std::get_if<keyboard_event>(&*ui_evt)) {
                        // Keyboard events go to focused widget, or root if none focused
                        auto* focused = ui_ctx.input().get_focused();
                        if (focused) {
                            focused->handle_event(*ui_evt, event_phase::target);
                        } else {
                            widget_ptr->handle_event(*ui_evt, event_phase::target);
                        }
                    } else if (std::get_if<resize_event>(&*ui_evt)) {
                        // Resize events go to root widget
                        widget_ptr->handle_event(*ui_evt, event_phase::target);
                    }
                }
            }

            if (quit) break;

            // Perform layout
            (void)widget_ptr->measure(
                logical_unit(static_cast<double>(viewport_w)),
                logical_unit(static_cast<double>(viewport_h))
            );
            widget_ptr->arrange(logical_rect{
                0.0_lu, 0.0_lu,
                logical_unit(static_cast<double>(viewport_w)),
                logical_unit(static_cast<double>(viewport_h))
            });

            // Clear background
            sdl_renderer.set_draw_color(::sdlpp::color{0, 0, 0, 255});
            sdl_renderer.clear();

            // Render the widget tree using proper render() method
            // Get current UI theme (auto-registered by scoped_ui_context)
            const auto* ui_theme = ui_ctx.themes().get_current_theme();
            widget_ptr->render(onyx_renderer, ui_theme, ui_ctx.metrics());

            // Present frame
            sdl_renderer.present();
        }

        // Unregister global renderer before it goes out of scope
        set_renderer(nullptr);

        return 0;
    }
    catch (const std::exception& e) {
        set_renderer(nullptr);  // Ensure cleanup even on exception
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}

template<typename Widget>
    requires (!requires { typename Widget::template rebind<sdlpp_tile_backend>; })
int sdlpp_tile_backend::run_app(
    const char* title,
    int width,
    int height,
    std::function<void(Widget&)> setup)
{
    try {
        // Verify theme is set
        if (!has_theme()) {
            std::cerr << "Error: No tile theme set. Call tile::set_theme() before run_app()\n";
            return 1;
        }

        // Create UI context with 1:1 pixel metrics (tile backend uses direct pixels)
        // This MUST be created before any widgets since they use ui_services::metrics()
        backend_metrics<sdlpp_tile_backend> tile_metrics{
            .logical_to_physical_x = 1.0,  // 1 logical unit = 1 pixel
            .logical_to_physical_y = 1.0,  // 1 logical unit = 1 pixel
            .dpi_scale = 1.0               // No DPI scaling for tile-based rendering
        };
        scoped_ui_context<sdlpp_tile_backend> ui_ctx(tile_metrics);

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

        // Enable vsync
        sdl_renderer.set_vsync(1);

        // Create tile renderer
        tile_renderer onyx_renderer(sdl_renderer);

        // Set up atlas from theme
        tile_atlas* atlas = get_theme().atlas;
        if (!atlas) {
            std::cerr << "Error: tile_theme.atlas is null\n";
            return 1;
        }
        if (!atlas->texture) {
            std::cerr << "Error: tile_theme.atlas->texture is null\n";
            return 1;
        }

        // Set texture type explicitly for sdlpp backend (required for debug validation)
        atlas->tex_type = texture_type::sdlpp;

        onyx_renderer.set_atlas(atlas);
        onyx_renderer.set_texture(static_cast<::sdlpp::texture*>(atlas->texture));

        // Register global renderer for tile widgets
        set_renderer(&onyx_renderer);

        // Create widget
        auto widget = std::make_unique<Widget>();
        auto* widget_ptr = widget.get();

        // Call setup callback if provided
        if (setup) {
            setup(*widget_ptr);
        }

        // Track time for animations
        auto last_time = std::chrono::steady_clock::now();

        // Get initial viewport size
        auto viewport = onyx_renderer.get_viewport();
        int viewport_w = viewport.w;
        int viewport_h = viewport.h;

        bool quit = false;

        // Main event loop
        while (!quit) {
            // Calculate delta time
            auto now = std::chrono::steady_clock::now();
            auto delta = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_time);
            last_time = now;
            [[maybe_unused]] unsigned int delta_ms = static_cast<unsigned int>(delta.count());

            // Process all pending events
            while (auto event = ::sdlpp::event_queue::poll()) {
                if (event->type() == ::sdlpp::event_type::quit) {
                    quit = true;
                    break;
                }

                // Handle window resize
                if (event->type() == ::sdlpp::event_type::window_resized) {
                    viewport = onyx_renderer.get_viewport();
                    viewport_w = viewport.w;
                    viewport_h = viewport.h;
                }

                // Convert SDL event to ui_event and route properly
                auto ui_evt = sdlpp_tile_backend::create_event(*event);
                if (ui_evt) {
                    // Proper three-phase event routing with hit testing
                    if (auto* mouse_evt = std::get_if<mouse_event>(&*ui_evt)) {
                        // Hit test to find target widget (using logical coordinates)
                        hit_test_path<sdlpp_tile_backend> hit_path;
                        auto* target = widget_ptr->hit_test_logical(
                            mouse_evt->x,
                            mouse_evt->y,
                            hit_path
                        );

                        if (target && !hit_path.empty()) {
                            // Three-phase event routing: capture -> target -> bubble
                            route_event(*ui_evt, hit_path);
                        }
                    } else if (std::get_if<keyboard_event>(&*ui_evt)) {
                        // Keyboard events go to focused widget, or root if none focused
                        auto* focused = ui_ctx.input().get_focused();
                        if (focused) {
                            focused->handle_event(*ui_evt, event_phase::target);
                        } else {
                            widget_ptr->handle_event(*ui_evt, event_phase::target);
                        }
                    } else if (std::get_if<resize_event>(&*ui_evt)) {
                        // Resize events go to root widget
                        widget_ptr->handle_event(*ui_evt, event_phase::target);
                    }
                }
            }

            if (quit) break;

            // Perform layout
            (void)widget_ptr->measure(
                logical_unit(static_cast<double>(viewport_w)),
                logical_unit(static_cast<double>(viewport_h))
            );
            widget_ptr->arrange(logical_rect{
                0.0_lu, 0.0_lu,
                logical_unit(static_cast<double>(viewport_w)),
                logical_unit(static_cast<double>(viewport_h))
            });

            // Clear background
            sdl_renderer.set_draw_color(::sdlpp::color{0, 0, 0, 255});
            sdl_renderer.clear();

            // Render the widget tree using proper render() method
            // Get current UI theme (auto-registered by scoped_ui_context)
            const auto* ui_theme = ui_ctx.themes().get_current_theme();
            widget_ptr->render(onyx_renderer, ui_theme, ui_ctx.metrics());

            // Present frame
            sdl_renderer.present();
        }

        // Unregister global renderer before it goes out of scope
        set_renderer(nullptr);

        return 0;
    }
    catch (const std::exception& e) {
        set_renderer(nullptr);  // Ensure cleanup even on exception
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}

} // namespace onyxui::tile
