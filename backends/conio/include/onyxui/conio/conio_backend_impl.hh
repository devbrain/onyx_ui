/**
 * @file conio_backend_impl.hh
 * @brief Template implementation for conio_backend::run_app
 *
 * This file is included at the end of conio_backend.hh
 */

#pragma once

#include <iostream>

namespace onyxui::conio {

template<template<typename> class Widget>
int conio_backend::run_app(std::function<void(Widget<conio_backend>&)> setup)
{
    try {
        // Create UI context with terminal metrics (1 logical unit = 1 char)
        scoped_ui_context<conio_backend> ui_ctx(make_terminal_metrics<conio_backend>());

        // Create widget
        auto widget = std::make_unique<Widget<conio_backend>>();
        auto* widget_ptr = widget.get();

        // Call setup callback if provided
        if (setup) {
            setup(*widget_ptr);
        }

        // Create VRAM and renderer (vram handles tb_init/tb_shutdown)
        auto vram_instance = std::make_shared<vram>();
        conio_renderer onyx_renderer(vram_instance);

        // Create UI handle
        ui_handle<conio_backend> ui(std::move(widget), std::move(onyx_renderer));

        // Initial display
        ui.display();

        // Main event loop
        // Check widget's should_quit if available, otherwise use backend's
        while (true) {
            // Check quit conditions
            if constexpr (requires { widget_ptr->should_quit(); }) {
                if (widget_ptr->should_quit()) break;
            }
            if (should_quit()) break;

            tb_event ev;
            const int result = conio_poll_event(&ev);

            if (result == TB_OK) {
                // Pass native event to ui_handle (it does conversion internally)
                ui.handle_event(ev);

                // Display after handling event
                ui.display();
            } else if (result == TB_ERR_POLL) {
                // Error polling - exit
                break;
            }
            // TB_ERR_NO_EVENT means no event available, continue
        }

        // Cleanup handled by vram destructor
        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}

} // namespace onyxui::conio
