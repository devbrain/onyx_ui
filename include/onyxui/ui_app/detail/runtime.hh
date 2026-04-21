/**
 * @file detail/runtime.hh
 * @brief Internal bookkeeping for the simple-shell `run()` loop.
 *
 * onyx_ui is a header-only library. The state backing the run loop
 * (registered windows, quit flag) lives here as `inline thread_local`
 * variables and the helpers that touch them are `inline` functions.
 * Every TU that includes this header compiles its own copy; the
 * `inline` specifier deduplicates the definitions at link time.
 *
 * Not part of the public API — consumers interact with the simple
 * shell through `<onyxui/ui_app/app_window.hh>`,
 * `<onyxui/ui_app/run.hh>`, and `<onyxui/ui_app/dialogs.hh>`.
 *
 * Modal-dialog ownership is NOT tracked here. Each `app_window` owns
 * its own modal presenters via `app_window::show_modal(...)` — this
 * keeps the lifetime tied to the host that rendered the dialog and
 * avoids the DSO-split problem a thread-local registry would have in
 * shared-library builds.
 */

#pragma once

#include <algorithm>
#include <cstddef>
#include <vector>

namespace onyxui::ui_app {
    class app_window;
}

namespace onyxui::ui_app::detail {

    // --------------------------------------------------------------
    // Thread-local state. `inline thread_local` is valid from C++17
    // onwards and gives us a header-only singleton per thread.
    //
    // All state here is run-loop-scoped (windows participating in
    // `run()`, quit flag). Per-window state — notably the set of
    // live modal dialogs — lives on `app_window` itself so its
    // lifetime follows the host.
    // --------------------------------------------------------------

    inline thread_local std::vector<app_window*> g_windows;
    inline thread_local bool g_quit_requested = false;
    inline thread_local int  g_exit_code = 0;

    // --------------------------------------------------------------
    // Window registry
    // --------------------------------------------------------------

    inline void register_window(app_window* w) {
        if (!w) return;
        if (std::find(g_windows.begin(), g_windows.end(), w) != g_windows.end()) {
            return;
        }
        g_windows.push_back(w);
    }

    inline void unregister_window(app_window* w) {
        if (!w) return;
        g_windows.erase(
            std::remove(g_windows.begin(), g_windows.end(), w),
            g_windows.end());
    }

    [[nodiscard]] inline std::size_t registered_window_count() noexcept {
        return g_windows.size();
    }

    inline void for_each_registered_window(
        void (*fn)(app_window*, void*), void* userdata) {
        if (!fn) return;
        // Snapshot so callbacks can safely unregister mid-iteration.
        const std::vector<app_window*> snapshot(
            g_windows.begin(), g_windows.end());
        for (auto* w : snapshot) {
            fn(w, userdata);
        }
    }

    // --------------------------------------------------------------
    // Quit flag
    // --------------------------------------------------------------

    inline void request_quit(int code) noexcept {
        g_quit_requested = true;
        g_exit_code = code;
    }

    [[nodiscard]] inline bool quit_requested() noexcept {
        return g_quit_requested;
    }

    [[nodiscard]] inline int exit_code() noexcept {
        return g_exit_code;
    }

    inline void reset_quit() noexcept {
        g_quit_requested = false;
        g_exit_code = 0;
    }

} // namespace onyxui::ui_app::detail
