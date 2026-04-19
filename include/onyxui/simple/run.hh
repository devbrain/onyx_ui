/**
 * @file run.hh
 * @brief Blessed main loop for the simple shell.
 *
 * Per docs/ONYXUI_SIMPLE_SHELL_DESIGN.md §6.2.
 *
 * ## v1 scope: single-window
 *
 * The simple shell is **single-window in v1**. Exactly one
 * `app_window` may be registered at a time. `run()` pumps OS events
 * once per frame and routes them to that window, then renders and
 * flips it. Calling `show()` on a second `app_window` while another
 * is registered logs a warning and routes events to the first — any
 * additional windows repaint but are effectively non-interactive.
 *
 * Multi-window support (per-window event routing, OS window_id →
 * `app_window` matching, menu/focus coordination across windows) is
 * tracked under future work — the public API intentionally accepts
 * only one live window so callers don't grow expectations the runtime
 * can't yet deliver.
 *
 * Typical usage (inside main()):
 *
 * @code
 * onyxui::simple::app_window win("Hello", 640, 480);
 * win.set_content(build_ui());
 * win.show();
 * return onyxui::simple::run();
 * @endcode
 *
 * Like `app_window.hh`, this header is included transitively by a
 * bundle header (`<onyxui/for/sdlpp.hh>`). Including it alone is
 * still allowed — there are no aliased names in the signatures — but
 * consumers who construct an `app_window` need the bundle anyway.
 */

#pragma once

namespace onyxui::simple {

    /// Drive the main loop until the registered `app_window` closes
    /// or `quit()` is called. Returns the exit code (0 on a normal
    /// close, whatever `quit(code)` was passed otherwise).
    ///
    /// If no window is registered when `run()` is called, it returns
    /// 0 immediately without entering the loop — useful for tests
    /// and for programs that show the window behind a command-line
    /// guard.
    int run();

    /// Request the main loop to exit at the next iteration with the
    /// given exit code. Does NOT close the window — leaves that to
    /// the consumer or to normal OS-close handling.
    void quit(int exit_code = 0) noexcept;

} // namespace onyxui::simple
