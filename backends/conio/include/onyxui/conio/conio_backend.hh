//
// Created by igor on 16/10/2025.
//

#pragma once

#include <onyxui/concepts/backend.hh>
#include <onyxui/conio/geometry.hh>
#include <onyxui/conio/colors.hh>
#include <onyxui/conio/conio_renderer.hh>
#include <onyxui/conio/conio_events.hh>
#include <onyxui/theme_registry.hh>
#include <onyxui/backends/conio/onyxui_conio_export.h>

namespace onyxui::conio {
    // ======================================================================
    // Backend Definition
    // ======================================================================

    /**
     * @struct conio_backend
     * @brief Console I/O backend for terminal-based UIs using termbox2
     *
     * This backend provides all required types for the UIBackend concept:
     * - Geometric types: rect, size, point
     * - Event types: termbox2 events
     * - Renderer: conio_renderer using vram
     *
     * @example Usage
     * @code
     * using element = ui_element<conio_backend>;
     * auto root = std::make_unique<element>();
     *
     * auto vram_instance = std::make_shared<vram>();
     * conio_renderer renderer(vram_instance);
     *
     * root->measure(80, 24);
     * root->arrange(conio_backend::rect{0, 0, 80, 24});
     * root->render(&renderer);
     * @endcode
     */
    struct conio_backend {
        // Required geometric types
        using rect_type = rect;
        using size_type = size;
        using point_type = point;

        // Required event types (from termbox2)
        using event_type = tb_event;
        using keyboard_event_type = tb_event;
        using mouse_button_event_type = tb_event;
        using mouse_motion_event_type = tb_event;
        using mouse_wheel_event_type = tb_event;
        using text_input_event_type = tb_event;
        using window_event_type = tb_event;

        // Required rendering types
        using color_type = color;
        using renderer_type = conio_renderer;

        // Backend identification
        static constexpr const char* name() { return "Conio"; }

        /**
         * @brief Register backend-provided themes
         * @param registry Theme registry to populate
         *
         * @details
         * Registers all built-in conio themes. The first theme registered
         * is considered the default theme.
         *
         * This method is called automatically by ui_context on first
         * context creation, so manual registration is not required.
         *
         * Registered themes (in order):
         * 1. "Norton Blue" (default) - Classic Norton Utilities
         * 2. "Borland Turbo" - Turbo Pascal/C++ IDE
         * 3. "Midnight Commander" - MC file manager
         * 4. "DOS Edit" - MS-DOS Edit
         */
        ONYXUI_CONIO_EXPORT static void register_themes(onyxui::theme_registry<conio_backend>& registry);
    };

    // ======================================================================
    // Exported Termbox2 Wrapper Functions
    // ======================================================================
    // These wrappers provide properly exported symbols for termbox2 functions
    // needed by applications using the conio backend as a shared library.

    /**
     * @brief Get terminal width (wrapper for tb_width)
     * @return Terminal width in characters
     */
    ONYXUI_CONIO_EXPORT int conio_get_width();

    /**
     * @brief Get terminal height (wrapper for tb_height)
     * @return Terminal height in characters
     */
    ONYXUI_CONIO_EXPORT int conio_get_height();

    /**
     * @brief Poll for terminal events (wrapper for tb_poll_event)
     * @param event Pointer to event structure to fill
     * @return Status code from tb_poll_event
     */
    ONYXUI_CONIO_EXPORT int conio_poll_event(tb_event* event);

} // namespace onyxui::conio


// Static assertion to verify backend compliance (after event_traits specialization)
static_assert(onyxui::UIBackend<onyxui::conio::conio_backend>, "conio_backend must satisfy UIBackend concept");
