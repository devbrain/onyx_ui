/**
 * @file run.cc
 * @brief conio implementation of `onyxui::simple::run()` /
 *        `onyxui::simple::quit()` per
 *        docs/ONYXUI_SIMPLE_SHELL_DESIGN.md §6.2 and §7.
 *
 * ## v1 single-window restriction
 *
 * Same as the sdlpp backend: the registry can hold multiple
 * `app_window`s but only one is routed events in v1. Unlike SDL,
 * conio's termbox2 library owns a single terminal surface — there is
 * no OS-window-id multiplexing to add later; true multi-window
 * conio would require tiling panels inside the terminal.
 *
 * The loop polls termbox2 ONCE per frame via `conio_poll_event()`
 * (non-blocking) and forwards each event to the single registered
 * window. Quit is triggered by `simple::quit()` or by the backend's
 * should_quit() flag — termbox2 has no OS-level close event.
 */

// This TU plays the same role as a bundle header; signal that to the
// guardrail check in the simple/* headers.
#define ONYXUI_SIMPLE_BUNDLE_INCLUDED 1

// Promote aliases into onyxui::simple before app_window.hh is parsed.
#include <onyxui/backend/conio.hh>

namespace onyxui::simple {
    using ::onyxui::conio::backend;
    #define ONYXUI_TYPE(name) using ::onyxui::conio::name;
    #include <onyxui/detail/public_types.inc>
    #undef ONYXUI_TYPE
}

#include <onyxui/simple/run.hh>
#include <onyxui/simple/app_window.hh>
#include <onyxui/simple/detail/runtime.hh>

#include <onyxui/conio/conio_backend.hh>

#include <iostream>
#include <vector>

namespace onyxui::simple {

    namespace {

        // Collect the currently-registered windows into a local
        // vector. Gives run() a stable snapshot for one iteration.
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
               detail::registered_window_count() > 0 &&
               !::onyxui::conio::conio_backend::should_quit()) {
            auto windows = snapshot_registered();

            if (windows.size() > 1) {
                static bool warned_once = false;
                if (!warned_once) {
                    std::cerr
                        << "[onyxui::simple::run] warning: "
                        << windows.size()
                        << " app_windows are registered. v1 only "
                           "correctly routes events to the first-"
                           "registered window — conio's terminal "
                           "surface cannot host multiple interactive "
                           "windows natively.\n";
                    warned_once = true;
                }
            }

            // One termbox2 event poll per frame. Route to the first
            // registered window (the v1 single-window contract).
            if (!windows.empty()) {
                auto* primary = windows.front();
                tb_event ev;
                const int result = ::onyxui::conio::conio_poll_event(&ev);
                if (result == TB_OK) {
                    (void)primary->dispatch_native_event(&ev);
                } else if (result == TB_ERR_POLL) {
                    // Polling error — termbox2 is unhappy. Request
                    // quit rather than spin on the error forever.
                    detail::request_quit(1);
                    break;
                }
                // TB_ERR_NO_EVENT just means nothing to do this tick.
            }

            // Render + present every registered window. On conio,
            // the "present" step is `tb_present()` inside the
            // renderer, which flips the vram buffer to the terminal.
            for (auto* w : windows) {
                w->render_frame();
            }
            for (auto* w : windows) {
                w->present();
            }
        }

        return detail::exit_code();
    }

    void quit(int exit_code) noexcept {
        detail::request_quit(exit_code);
    }

} // namespace onyxui::simple
