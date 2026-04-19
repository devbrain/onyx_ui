/**
 * @file detail/runtime.hh
 * @brief Internal bookkeeping for the simple-shell `run()` loop
 *        and the live-dialog registry.
 *
 * onyx_ui is a header-only library. The state backing the run loop
 * (registered windows, quit flag) and the live-dialog registry
 * therefore lives here as `inline thread_local` variables and the
 * helpers that touch them are `inline` functions. Every TU that
 * includes this header compiles its own copy; the `inline`
 * specifier deduplicates the definitions at link time.
 *
 * Not part of the public API — consumers interact with the simple
 * shell through `<onyxui/simple/app_window.hh>`,
 * `<onyxui/simple/run.hh>`, and `<onyxui/simple/dialogs.hh>`.
 */

#pragma once

#include <algorithm>
#include <cstddef>
#include <functional>
#include <utility>
#include <vector>

namespace onyxui::simple {
    class app_window;
}

namespace onyxui::simple::detail {

    // --------------------------------------------------------------
    // Thread-local state. `inline thread_local` is valid from C++17
    // onwards and gives us a header-only singleton per thread.
    // --------------------------------------------------------------

    inline thread_local std::vector<app_window*> g_windows;
    inline thread_local bool g_quit_requested = false;
    inline thread_local int  g_exit_code = 0;

    struct live_dialog_entry {
        app_window* owner;
        void* key;
        std::function<void()> disposer;
    };
    inline thread_local std::vector<live_dialog_entry> g_live_dialogs;

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

    // --------------------------------------------------------------
    // Live-dialog registry. See docs/ONYXUI_SIMPLE_SHELL_DESIGN.md
    // §6.3 — each fire-and-forget dialog helper owns a presented
    // window. The helper stores a type-erased disposer here, keyed
    // by the dialog's raw pointer and tagged with the owning
    // app_window so the owner's close() / destructor can drain any
    // still-live dialogs before the host goes away (otherwise the
    // registry would leak them).
    // --------------------------------------------------------------

    inline void register_live_dialog(app_window* owner, void* key,
                                     std::function<void()> disposer) {
        if (!key) return;
        g_live_dialogs.push_back({owner, key, std::move(disposer)});
    }

    /// Dismiss the registered dialog matching @p key. Runs the
    /// stored disposer (drops the presenter → destroys the window)
    /// and removes the entry. No-op if no entry matches.
    inline void dismiss_live_dialog(void* key) {
        if (!key) return;
        auto it = std::find_if(g_live_dialogs.begin(),
                               g_live_dialogs.end(),
                               [key](const live_dialog_entry& d) {
                                   return d.key == key;
                               });
        if (it == g_live_dialogs.end()) return;

        // Move the disposer out, remove the entry, THEN invoke.
        // Running the disposer destroys the dialog widget — which
        // destroys its signals — whose emit() may be what's calling
        // us. The post-WAR-48 signal self-destruction protection
        // keeps that safe, but we also need the registry to be in a
        // consistent state first so recursive dismissal calls from
        // the disposer become no-ops.
        auto disposer = std::move(it->disposer);
        g_live_dialogs.erase(it);
        if (disposer) disposer();
    }

    /// Dismiss every registered dialog whose owner matches.
    /// Called from `app_window::close()` and `app_window::~app_window()`
    /// so dialogs can't outlive their host window.
    inline void dismiss_dialogs_for(app_window* owner) {
        if (!owner) return;

        std::vector<std::function<void()>> pending;
        auto it = g_live_dialogs.begin();
        while (it != g_live_dialogs.end()) {
            if (it->owner == owner) {
                pending.push_back(std::move(it->disposer));
                it = g_live_dialogs.erase(it);
            } else {
                ++it;
            }
        }
        for (auto& d : pending) {
            if (d) d();
        }
    }

    /// Count of live dialogs currently registered on this thread.
    /// For tests / diagnostics.
    [[nodiscard]] inline std::size_t live_dialog_count() noexcept {
        return g_live_dialogs.size();
    }

} // namespace onyxui::simple::detail
