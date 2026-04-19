/**
 * @file dialogs.hh
 * @brief Fire-and-forget dialog helpers for the simple shell.
 *
 * Per docs/ONYXUI_SIMPLE_SHELL_DESIGN.md §6.3. Each helper:
 *
 *   1. Builds a dialog widget on the heap (title + message + buttons).
 *   2. Connects `window::closed` with the user's result callback.
 *   3. Hands ownership to `app_window::show_modal(...)` — the host
 *      stores the presenter internally and destroys the window when
 *      `closed` fires.
 *
 * onyx_ui is header-only. The dialog logic is genuinely backend-
 * agnostic — every type it touches (`window`, `vbox`, `label`,
 * `button`, …) is reached via the `onyxui::simple` aliases that the
 * bundle header populates BEFORE this header is parsed. The
 * implementations therefore live here as `inline` definitions.
 *
 * Not standalone. Like `app_window.hh`, this header uses unqualified
 * names that must be in scope via a bundle header. The
 * `ONYXUI_SIMPLE_BUNDLE_INCLUDED` guardrail below catches misuse.
 *
 * No auto-find-active-window in v1 — each helper takes the parent
 * `app_window&` explicitly.
 */

#pragma once

#ifndef ONYXUI_SIMPLE_BUNDLE_INCLUDED
#  error "<onyxui/simple/dialogs.hh> is not a standalone header. " \
         "Include a bundle header instead — typically " \
         "<onyxui/for/sdlpp.hh>. See " \
         "docs/ONYXUI_SIMPLE_SHELL_DESIGN.md §5.2."
#endif

#include <onyxui/simple/app_window.hh>

#include <onyxui/core/types.hh>

#include <functional>
#include <memory>
#include <string>
#include <utility>

namespace onyxui::simple {

    namespace detail {

        // Build a simple "title bar + message label" window. Caller
        // adds the button row via `add_button_row`.
        inline std::unique_ptr<window> build_message_window(
            const std::string& title,
            const std::string& message) {
            typename window::window_flags flags;
            flags.is_resizable = false;
            flags.is_movable = true;
            flags.has_close_button = false;
            flags.has_minimize_button = false;
            flags.has_maximize_button = false;

            auto w = std::make_unique<window>(title, flags);
            w->set_width_constraint(
                {::onyxui::size_policy::fixed, ::onyxui::logical_unit(360)});
            w->set_window_focus(true);

            auto content = std::make_unique<vbox>(::onyxui::spacing::medium);
            content->set_margin(::onyxui::logical_thickness(
                ::onyxui::logical_unit(16), ::onyxui::logical_unit(12),
                ::onyxui::logical_unit(16), ::onyxui::logical_unit(12)));
            content->template emplace_child<label>(message);

            w->set_content(std::move(content));
            return w;
        }

        // Add a right-aligned button row and return the hbox so
        // callers can put buttons in it.
        inline hbox* add_button_row(window* w) {
            auto* content = dynamic_cast<vbox*>(w->get_content());
            if (!content) return nullptr;
            content->template emplace_child<spacer>(0, 4);
            auto* row = content->template emplace_child<hbox>(
                ::onyxui::spacing::small);
            row->set_horizontal_align(::onyxui::horizontal_alignment::right);
            return row;
        }

    } // namespace detail

    // -----------------------------------------------------------------

    /// Modal OK dialog. Fire-and-forget — no callback. Use `confirm`
    /// or `input_dialog` when the caller needs the user's answer.
    inline void message_box(app_window& parent,
                            const std::string& title,
                            const std::string& message) {
        auto w = detail::build_message_window(title, message);
        auto* raw = w.get();

        if (auto* row = detail::add_button_row(raw)) {
            auto* ok = row->template emplace_child<button>("OK");
            ok->clicked.connect([raw]() { raw->close(); });
        }

        parent.show_modal(std::move(w));
    }

    /// Modal error dialog — shortcut for `message_box` with the
    /// title defaulting to "Error". Visual styling (red border,
    /// icon) is a future nicety; v1 delegates to `message_box`.
    inline void error_box(app_window& parent,
                          const std::string& message,
                          const std::string& title = "Error") {
        message_box(parent, title, message);
    }

    /// Modal Yes/No dialog. `on_result` fires with true for yes,
    /// false for no. Callback is invoked when the user dismisses —
    /// never if the parent app_window dies first (the dialog is
    /// destroyed with it and `closed` is not fired during teardown).
    inline void confirm(app_window& parent,
                        const std::string& title,
                        const std::string& message,
                        std::function<void(bool yes)> on_result) {
        auto w = detail::build_message_window(title, message);
        auto* raw = w.get();

        auto result = std::make_shared<bool>(false);

        if (auto* row = detail::add_button_row(raw)) {
            auto* yes_btn = row->template emplace_child<button>("Yes");
            yes_btn->clicked.connect([raw, result]() {
                *result = true;
                raw->close();
            });
            auto* no_btn = row->template emplace_child<button>("No");
            no_btn->clicked.connect([raw, result]() {
                *result = false;
                raw->close();
            });
        }

        raw->closed.connect(
            [result, cb = std::move(on_result)]() {
                if (cb) cb(*result);
            });

        parent.show_modal(std::move(w));
    }

    /// Modal single-line input dialog. `on_result` fires with
    /// (true, entered_text) on OK, (false, "") on Cancel.
    inline void input_dialog(
        app_window& parent,
        const std::string& title,
        const std::string& prompt,
        std::function<void(bool ok, std::string value)> on_result) {
        typename window::window_flags flags;
        flags.is_resizable = false;
        flags.is_movable = true;
        flags.has_close_button = false;
        flags.has_minimize_button = false;
        flags.has_maximize_button = false;

        auto w = std::make_unique<window>(title, flags);
        w->set_width_constraint(
            {::onyxui::size_policy::fixed, ::onyxui::logical_unit(400)});
        w->set_window_focus(true);

        auto content = std::make_unique<vbox>(::onyxui::spacing::small);
        content->set_margin(::onyxui::logical_thickness(
            ::onyxui::logical_unit(16), ::onyxui::logical_unit(12),
            ::onyxui::logical_unit(16), ::onyxui::logical_unit(12)));

        content->template emplace_child<label>(prompt);
        auto* input = content->template emplace_child<line_edit>();
        input->set_visible_chars(40);

        content->template emplace_child<spacer>(0, 4);
        auto* row = content->template emplace_child<hbox>(
            ::onyxui::spacing::small);
        row->set_horizontal_align(::onyxui::horizontal_alignment::right);

        auto* raw = w.get();
        auto result_ok = std::make_shared<bool>(false);
        auto result_value = std::make_shared<std::string>();

        auto* ok_btn = row->template emplace_child<button>("OK");
        ok_btn->clicked.connect([raw, input, result_ok, result_value]() {
            *result_ok = true;
            *result_value = input->text();
            raw->close();
        });
        auto* cancel_btn = row->template emplace_child<button>("Cancel");
        cancel_btn->clicked.connect([raw, result_ok, result_value]() {
            *result_ok = false;
            result_value->clear();
            raw->close();
        });

        w->set_content(std::move(content));

        raw->closed.connect(
            [result_ok, result_value, cb = std::move(on_result)]() {
                if (cb) cb(*result_ok, *result_value);
            });

        parent.show_modal(std::move(w));
    }

} // namespace onyxui::simple
