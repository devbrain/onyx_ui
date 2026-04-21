/**
 * @file run.cc
 * @brief sdlpp implementation of `onyxui::ui_app::run()` /
 *        `onyxui::ui_app::quit()` per
 *        docs/ONYXUI_SIMPLE_SHELL_DESIGN.md §6.2 and §7.
 *
 * ## v1 single-window restriction
 *
 * The registry in runtime.cc can hold multiple `app_window`s and
 * this loop iterates all of them, but **only one registered window
 * at a time is supported in v1**. SDL's event queue is process-
 * global; multi-window routing requires matching each event to the
 * `app_window` with the correct OS window_id, which v1 does not do.
 *
 * The run loop polls the OS queue ONCE per frame (no longer per-
 * window — the previous per-window poll was the bug the reviewer
 * flagged) and forwards each event to the single registered
 * window. If a second window ever shows up, this function logs and
 * routes everything to the first-registered window, matching the
 * single-window contract.
 */

// This TU plays the same role as a bundle header; signal that to the
// guardrail check in the simple/* headers.
#define ONYXUI_UI_APP_BUNDLE_INCLUDED 1

// Promote aliases into onyxui::ui_app before app_window.hh is parsed.
#include <onyxui/backend/sdlpp.hh>

namespace onyxui::ui_app {
    using ::onyxui::ui::backend;
    #define ONYXUI_TYPE(name) using ::onyxui::ui::name;
    #include <onyxui/detail/public_types.inc>
    #undef ONYXUI_TYPE
}

#include <onyxui/ui_app/run.hh>
#include <onyxui/ui_app/app_window.hh>
#include <onyxui/ui_app/detail/runtime.hh>

#include <sdlpp/events/events.hh>
#include <sdlpp/events/event_types.hh>

#include <iostream>
#include <vector>

namespace onyxui::ui_app {

    namespace {

        // Collect the currently-registered windows into a local
        // vector. Gives run() a stable snapshot for one iteration
        // without the per-callback indirection of
        // detail::for_each_registered_window.
        std::vector<app_window*> snapshot_registered() {
            std::vector<app_window*> out;
            detail::for_each_registered_window(
                [](app_window* w, void* userdata) {
                    static_cast<std::vector<app_window*>*>(userdata)
                        ->push_back(w);
                },
                &out);
            return out;
        }

    } // anonymous namespace

    int run() {
        detail::reset_quit();

        while (!detail::quit_requested() &&
               detail::registered_window_count() > 0) {
            auto windows = snapshot_registered();

            if (windows.size() > 1) {
                static bool warned_once = false;
                if (!warned_once) {
                    std::cerr
                        << "[onyxui::ui_app::run] warning: "
                        << windows.size()
                        << " app_windows are registered. v1 only "
                           "correctly routes events to the first-"
                           "registered window — multi-window support "
                           "is not implemented yet.\n";
                    warned_once = true;
                }
            }

            // One global event pump per frame. Route to the first
            // registered window (the v1 single-window contract).
            bool close_requested = false;
            if (!windows.empty()) {
                auto* primary = windows.front();
                while (auto ev = ::sdlpp::event_queue::poll()) {
                    if (ev->type() == ::sdlpp::event_type::quit) {
                        close_requested = true;
                        break;
                    }
                    (void)primary->dispatch_native_event(&*ev);
                }
            }

            // Render + present every registered window. Even windows
            // that aren't receiving events still redraw themselves
            // on any invalidation their widgets produce.
            for (auto* w : windows) {
                w->render_frame();
            }
            for (auto* w : windows) {
                w->present();
            }

            if (close_requested) {
                detail::request_quit(0);
            }
        }

        return detail::exit_code();
    }

    void quit(int exit_code) noexcept {
        detail::request_quit(exit_code);
    }

} // namespace onyxui::ui_app
