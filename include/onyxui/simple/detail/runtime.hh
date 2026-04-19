/**
 * @file detail/runtime.hh
 * @brief Internal registry that `app_window` uses to talk to the
 *        `run()` loop (WAR-54). Not part of the public API.
 *
 * Lives in a `detail::` namespace to discourage consumer use.
 */

#pragma once

#include <cstddef>
#include <functional>

namespace onyxui::simple {
    class app_window;
}

namespace onyxui::simple::detail {

    /// Add this app_window to the set of windows driven by run().
    /// No-op if already registered.
    void register_window(app_window* w);

    /// Remove from the set. No-op if not registered.
    void unregister_window(app_window* w);

    /// Number of windows currently registered.
    [[nodiscard]] std::size_t registered_window_count() noexcept;

    /// Iterate through registered windows. The callback is invoked
    /// with each live `app_window*`. Defined in runtime.cc.
    void for_each_registered_window(void (*fn)(app_window*, void*), void* userdata);

    /// Signal the run loop to exit at the next iteration.
    void request_quit(int exit_code) noexcept;

    /// Read the quit request flag (and the exit code if set).
    [[nodiscard]] bool quit_requested() noexcept;
    [[nodiscard]] int exit_code() noexcept;

    /// Reset quit state — for re-entering run() in tests.
    void reset_quit() noexcept;

    // ----------------------------------------------------------------
    // Live-dialog registry. Each fire-and-forget dialog helper
    // (message_box, confirm, input_dialog, error_box) owns a
    // presented_window — but the helper returns void so there's
    // nowhere for the caller to hold the presenter. The registry
    // holds it externally and is keyed by an opaque pointer (the
    // dialog window's address) so the dialog's close handler can
    // dismiss itself without fighting an ownership cycle.
    //
    // `disposer` is a type-erased drop: whatever the helper
    // captured (its unique_ptr<presented_window<backend>>) gets
    // released when the disposer runs. Keeping the API void*-keyed
    // lets this header stay backend-agnostic.
    // ----------------------------------------------------------------

    void register_live_dialog(void* key, std::function<void()> disposer);

    /// Dismiss the registered dialog matching @p key. Runs the
    /// stored disposer (drops the presenter → destroys the window)
    /// and removes the entry. No-op if no entry matches.
    void dismiss_live_dialog(void* key);

    /// Count of live dialogs registered. For tests / diagnostics.
    [[nodiscard]] std::size_t live_dialog_count() noexcept;

} // namespace onyxui::simple::detail
