/**
 * @file run.hh
 * @brief Blessed main loop for the simple shell.
 *
 * Per docs/ONYXUI_SIMPLE_SHELL_DESIGN.md §6.2. Pump events, render,
 * and flip for every registered `app_window` until the last one
 * closes or `quit()` is called.
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
 * bundle header (`<onyxui/for/sdlpp.hh>` or
 * `<onyxui/for/conio.hh>`). Including it alone is still allowed —
 * there are no aliased names in the signatures — but consumers who
 * construct an `app_window` need the bundle anyway.
 */

#pragma once

namespace onyxui::simple {

    /// Drive the main loop until the last registered `app_window`
    /// closes or `quit()` is called. Returns the exit code (0 on a
    /// normal close, whatever `quit(code)` was passed otherwise).
    ///
    /// If no windows are registered when `run()` is called, it
    /// returns 0 immediately without entering the loop — useful for
    /// tests and for programs that show all their windows behind a
    /// command-line guard.
    int run();

    /// Request the main loop to exit at the next iteration with the
    /// given exit code. Does NOT close any windows — leaves that to
    /// the consumer or to normal OS-close handling.
    void quit(int exit_code = 0) noexcept;

} // namespace onyxui::simple
