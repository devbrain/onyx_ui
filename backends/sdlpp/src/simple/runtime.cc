/**
 * @file runtime.cc
 * @brief Backend-agnostic bookkeeping for the simple-shell `run()`
 *        loop: the window registry and the quit flag.
 *
 * Physically lives under backends/sdlpp/src/simple/ only because the
 * simple shell is compiled into the sdlpp backend library in v1.
 * WAR-59 will either duplicate this file under backends/conio/ or
 * factor it into a shared static library.
 */

#include <onyxui/simple/detail/runtime.hh>

#include <algorithm>
#include <cstddef>
#include <vector>

namespace onyxui::simple::detail {

    namespace {
        // All state is thread-local: the run loop, like the ambient
        // ui_services stack it sits on top of, is a per-thread
        // concept.
        thread_local std::vector<app_window*> s_windows;
        thread_local bool s_quit_requested = false;
        thread_local int s_exit_code = 0;
    }

    void register_window(app_window* w) {
        if (!w) return;
        if (std::find(s_windows.begin(), s_windows.end(), w) != s_windows.end()) {
            return;
        }
        s_windows.push_back(w);
    }

    void unregister_window(app_window* w) {
        if (!w) return;
        s_windows.erase(
            std::remove(s_windows.begin(), s_windows.end(), w),
            s_windows.end());
    }

    std::size_t registered_window_count() noexcept {
        return s_windows.size();
    }

    void for_each_registered_window(void (*fn)(app_window*, void*), void* userdata) {
        if (!fn) return;
        // Snapshot the list so callbacks can safely unregister.
        const std::vector<app_window*> snapshot(s_windows.begin(), s_windows.end());
        for (auto* w : snapshot) {
            fn(w, userdata);
        }
    }

    void request_quit(int code) noexcept {
        s_quit_requested = true;
        s_exit_code = code;
    }

    bool quit_requested() noexcept {
        return s_quit_requested;
    }

    int exit_code() noexcept {
        return s_exit_code;
    }

    void reset_quit() noexcept {
        s_quit_requested = false;
        s_exit_code = 0;
    }

} // namespace onyxui::simple::detail
