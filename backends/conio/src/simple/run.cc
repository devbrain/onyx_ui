/**
 * @file run.cc
 * @brief conio implementation of `onyxui::simple::run()` /
 *        `onyxui::simple::quit()` per
 *        docs/ONYXUI_SIMPLE_SHELL_DESIGN.md §6.2 and §7.
 *
 * ## v1 single-window restriction
 *
 * Conio is **strictly single-window**. Termbox2 owns a single
 * process-global terminal surface; there is no OS-window-id
 * multiplexing to add later. `app_window::show()` already refuses
 * the second concurrent registration (see
 * `backends/conio/src/simple/app_window.cc`), but just in case a
 * caller uses a non-blessed path to wire a second window into the
 * registry, this loop routes **both** input and rendering to the
 * first-registered window only. The old behaviour of rendering every
 * registered window onto the same terminal produced "see B, control
 * A" — one window rendered last was visually on top while the
 * first window kept receiving input. Matching the routing on output
 * avoids that.
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
                        << " app_windows are registered. Conio is "
                           "single-window — only the first-registered "
                           "window will be driven (input AND output). "
                           "The others are frozen.\n";
                    warned_once = true;
                }
            }

            // One termbox2 event poll per frame. Route everything —
            // input and rendering — to the first registered window.
            // Secondary windows stay frozen to avoid the "see B,
            // control A" glitch from rendering all windows onto the
            // shared terminal.
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

                primary->render_frame();
                primary->present();
            }
        }

        return detail::exit_code();
    }

    void quit(int exit_code) noexcept {
        detail::request_quit(exit_code);
    }

} // namespace onyxui::simple
