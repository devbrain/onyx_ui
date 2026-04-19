/**
 * @file run.cc
 * @brief sdlpp implementation of `onyxui::simple::run()` /
 *        `onyxui::simple::quit()` per
 *        docs/ONYXUI_SIMPLE_SHELL_DESIGN.md §6.2 and §7.
 *
 * Single-threaded. Drives every registered `app_window` each frame:
 * pump events, render, present. Exits when the registry drains or
 * when `quit()` is called.
 */

// Promote aliases into onyxui::simple before app_window.hh is parsed.
#include <onyxui/backend/sdlpp.hh>

namespace onyxui::simple {
    using ::onyxui::sdlpp::backend;
    #define ONYXUI_TYPE(name) using ::onyxui::sdlpp::name;
    #include <onyxui/detail/public_types.inc>
    #undef ONYXUI_TYPE
}

#include <onyxui/simple/run.hh>
#include <onyxui/simple/app_window.hh>
#include <onyxui/simple/detail/runtime.hh>

namespace onyxui::simple {

    int run() {
        // Reset quit state so a prior run() that ended via quit()
        // doesn't short-circuit the next one (useful in tests and for
        // tools that re-enter the loop).
        detail::reset_quit();

        // v1 frame shape:
        //   - Each iteration, snapshot the registered windows.
        //   - For each window: drain its events (returns true on quit
        //     request) + render a frame.
        //   - Present all windows at end of frame.
        //   - Exit when registry is empty or quit was requested.
        //
        // Multi-window caveat (tracked alongside WAR-53): SDL's event
        // queue is global, and app_window::pump_events() drains it
        // entirely into that window's host. With two simple::app_windows
        // live at once, the first one polled will consume events
        // destined for the second. Single-window tools (the default
        // simple-shell case) are unaffected.

        while (!detail::quit_requested() &&
               detail::registered_window_count() > 0) {
            bool any_close_requested = false;

            detail::for_each_registered_window(
                [](app_window* w, void* userdata) {
                    if (w->pump_events()) {
                        *static_cast<bool*>(userdata) = true;
                    }
                    w->render_frame();
                },
                &any_close_requested);

            // Present after all windows have rendered — keeps frame
            // pacing consistent across a multi-window setup.
            detail::for_each_registered_window(
                [](app_window* w, void* /*userdata*/) {
                    w->present();
                },
                nullptr);

            if (any_close_requested) {
                // A window observed an SDL_QUIT. Treat as a request
                // to exit the loop after this iteration. Windows that
                // want to stay open can override — they won't
                // unregister themselves just because SDL_QUIT fired.
                detail::request_quit(0);
            }
        }

        return detail::exit_code();
    }

    void quit(int exit_code) noexcept {
        detail::request_quit(exit_code);
    }

} // namespace onyxui::simple
