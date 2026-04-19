/**
 * @file dialogs.hh
 * @brief Fire-and-forget dialog helpers for the simple shell.
 *
 * Per docs/ONYXUI_SIMPLE_SHELL_DESIGN.md §6.3. Each helper:
 *
 *   1. Builds a dialog widget on the heap (title + message + buttons).
 *   2. Presents it modal via `parent.host().present_modal(...)`.
 *   3. Registers the resulting presenter with the simple-shell
 *      runtime so it stays alive until the dialog closes.
 *   4. When the user clicks OK / Cancel / etc., the dialog's
 *      `result_ready` signal fires, the user's callback runs, and
 *      the runtime drops the presenter — destroying the dialog.
 *
 * Not standalone. Like `app_window.hh`, this header uses
 * unqualified names (`app_window`) that must be in scope via a
 * bundle header (`<onyxui/for/sdlpp.hh>` or similar).
 *
 * No auto-find-active-window in v1 — each helper takes the parent
 * `app_window&` explicitly.
 */

#pragma once

#include <functional>
#include <string>

namespace onyxui::simple {

    class app_window;  // forward-decl; definition in app_window.hh

    /// Modal OK dialog. Fire-and-forget — no callback. Use `confirm`
    /// or `input_dialog` when the caller needs the user's answer.
    void message_box(app_window& parent,
                     const std::string& title,
                     const std::string& message);

    /// Modal Yes/No dialog. `on_result` fires with true for yes,
    /// false for no or cancel. Never invoked if the window dies
    /// before the user dismisses.
    void confirm(app_window& parent,
                 const std::string& title,
                 const std::string& message,
                 std::function<void(bool yes)> on_result);

    /// Modal single-line input dialog. `on_result` fires with
    /// (true, entered_text) on OK, (false, "") on Cancel.
    void input_dialog(app_window& parent,
                      const std::string& title,
                      const std::string& prompt,
                      std::function<void(bool ok, std::string value)> on_result);

    /// Modal error dialog — shortcut for `message_box` with the
    /// title defaulting to "Error" and the same visual weight.
    void error_box(app_window& parent,
                   const std::string& message,
                   const std::string& title = "Error");

} // namespace onyxui::simple
