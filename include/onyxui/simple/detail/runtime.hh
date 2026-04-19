/**
 * @file detail/runtime.hh
 * @brief Internal registry that `app_window` uses to talk to the
 *        `run()` loop (WAR-54). Not part of the public API.
 *
 * Lives in a `detail::` namespace to discourage consumer use.
 */

#pragma once

#include <cstddef>

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

} // namespace onyxui::simple::detail
