/**
 * @file window_presets.hh
 * @brief Convenience factories for common message / confirmation dialog
 *        shapes, built on the `ui_host::present_modal` →
 *        `presented_window<B>` ownership model.
 *
 * @details
 * Three kinds of helpers:
 * - `show_message_box` — full button-combination control (OK,
 *   OK/Cancel, Yes/No, Yes/No/Cancel, Abort/Retry/Ignore) with a
 *   callback that receives the raw `dialog_result`.
 * - `show_confirm` / `show_warning` — shortcut wrappers around
 *   `yes_no` / `ok_cancel` that translate `dialog_result` into a
 *   boolean.
 * - `show_info` / `show_error` — fire-and-forget notice dialogs
 *   (OK button only, no callback).
 *
 * Each helper returns a `presented_window<B>` that the caller must
 * retain until the dialog closes. The returned presenter is the
 * only handle that keeps the dialog alive — dropping it closes the
 * dialog. Callers who want fire-and-forget semantics on a
 * simple-shell `app_window` should use the helpers in
 * `<onyxui/ui_app/dialogs.hh>` (`message_box`, `confirm`,
 * `input_dialog`, `error_box`) instead; those route through
 * `app_window::show_modal` and take care of ownership automatically.
 */

#pragma once

#include <onyxui/ui_host.hh>
#include <onyxui/widgets/window/dialog.hh>
#include <onyxui/widgets/window/presented_window.hh>

#include <functional>
#include <memory>
#include <string>
#include <utility>

namespace onyxui {

    /**
     * @enum message_box_buttons
     * @brief Standard button combinations for message boxes
     */
    enum class message_box_buttons : uint8_t {
        ok,                     ///< OK button only
        ok_cancel,              ///< OK and Cancel buttons
        yes_no,                 ///< Yes and No buttons
        yes_no_cancel,          ///< Yes, No, and Cancel buttons
        abort_retry_ignore      ///< Abort, Retry, and Ignore buttons
    };

    namespace detail {

        template<UIBackend Backend>
        inline std::unique_ptr<dialog<Backend>> build_message_dialog(
            std::string title,
            std::string message,
            message_box_buttons buttons) {
            auto dlg = std::make_unique<dialog<Backend>>(std::move(title));
            dlg->set_message(std::move(message));

            switch (buttons) {
                case message_box_buttons::ok:
                    dlg->add_ok_button();
                    break;
                case message_box_buttons::ok_cancel:
                    dlg->add_ok_cancel_buttons();
                    break;
                case message_box_buttons::yes_no:
                    dlg->add_yes_no_buttons();
                    break;
                case message_box_buttons::yes_no_cancel:
                    dlg->add_yes_no_cancel_buttons();
                    break;
                case message_box_buttons::abort_retry_ignore:
                    dlg->add_abort_retry_ignore_buttons();
                    break;
            }
            return dlg;
        }

    } // namespace detail

    /**
     * @brief Present a modal message box with a button combination and
     *        a typed result callback.
     *
     * @param host Host to present on; takes ownership of the dialog.
     * @param title Window title.
     * @param message Message text.
     * @param buttons Button combination to show.
     * @param on_result Invoked with the raw `dialog_result` when the
     *                  user dismisses the dialog. May be null.
     * @return RAII presenter — keep it alive until the dialog closes.
     *
     * @code
     * auto dlg = show_message_box(
     *     host,
     *     "Save Changes?",
     *     "Do you want to save?",
     *     message_box_buttons::yes_no_cancel,
     *     [](typename dialog<Backend>::dialog_result r) {
     *         if (r == dialog<Backend>::dialog_result::yes) save_file();
     *     });
     * // Keep `dlg` alive (e.g. stash on a parent) until the user
     * // dismisses the dialog.
     * @endcode
     */
    template<UIBackend Backend>
    [[nodiscard]] presented_window<Backend> show_message_box(
        ui_host<Backend>& host,
        std::string title,
        std::string message,
        message_box_buttons buttons,
        std::function<void(typename dialog<Backend>::dialog_result)> on_result = {}) {
        auto dlg = detail::build_message_dialog<Backend>(
            std::move(title), std::move(message), buttons);

        if (on_result) {
            dlg->result_ready.connect(std::move(on_result));
        }

        return host.present_modal(std::move(dlg));
    }

    /**
     * @brief Info message — OK button only. Fire-and-forget shape:
     *        callers typically discard the returned presenter into
     *        a container whose drop order they own (e.g. vector of
     *        live dialogs on their scene).
     */
    template<UIBackend Backend>
    [[nodiscard]] presented_window<Backend> show_info(
        ui_host<Backend>& host,
        std::string message,
        std::string title = "Information") {
        return show_message_box(host, std::move(title), std::move(message),
                                message_box_buttons::ok);
    }

    /**
     * @brief Yes/No confirmation — callback receives `true` for Yes,
     *        `false` for No.
     */
    template<UIBackend Backend>
    [[nodiscard]] presented_window<Backend> show_confirm(
        ui_host<Backend>& host,
        std::string title,
        std::string message,
        std::function<void(bool confirmed)> callback) {
        return show_message_box(
            host, std::move(title), std::move(message),
            message_box_buttons::yes_no,
            [cb = std::move(callback)](typename dialog<Backend>::dialog_result r) {
                if (cb) cb(r == dialog<Backend>::dialog_result::yes);
            });
    }

    /**
     * @brief OK/Cancel warning — callback receives `true` for OK,
     *        `false` for Cancel.
     */
    template<UIBackend Backend>
    [[nodiscard]] presented_window<Backend> show_warning(
        ui_host<Backend>& host,
        std::string title,
        std::string message,
        std::function<void(bool proceed)> callback) {
        return show_message_box(
            host, std::move(title), std::move(message),
            message_box_buttons::ok_cancel,
            [cb = std::move(callback)](typename dialog<Backend>::dialog_result r) {
                if (cb) cb(r == dialog<Backend>::dialog_result::ok);
            });
    }

    /**
     * @brief Error message — OK button only. Shares the `show_info`
     *        shape; the default title is "Error".
     */
    template<UIBackend Backend>
    [[nodiscard]] presented_window<Backend> show_error(
        ui_host<Backend>& host,
        std::string message,
        std::string title = "Error") {
        return show_message_box(host, std::move(title), std::move(message),
                                message_box_buttons::ok);
    }

} // namespace onyxui
