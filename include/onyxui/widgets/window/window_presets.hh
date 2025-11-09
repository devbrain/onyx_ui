/**
 * @file window_presets.hh
 * @brief Convenience functions and presets for common window/dialog patterns
 * @author Claude Code
 * @date 2025-11-08
 *
 * @details
 * Provides helper functions for common dialog patterns:
 * - show_message_box(): Full-featured message box with custom buttons
 * - show_info(): Simple info message (OK button only)
 * - show_confirm(): Yes/No confirmation dialog
 */

#pragma once

#include <onyxui/widgets/window/dialog.hh>
#include <string>
#include <functional>
#include <memory>

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

    /**
     * @brief Show a message box dialog (Pattern 3: Maximum Convenience)
     *
     * @tparam Backend UI backend type
     * @param title Dialog title
     * @param message Message text to display
     * @param buttons Button combination
     * @param callback Callback invoked with dialog result
     *
     * @details
     * Creates and shows a modal message box dialog with the specified button combination.
     * The dialog is owned by the caller who must keep it alive until closed.
     *
     * ## Usage
     * @code
     * show_message_box<Backend>(
     *     "Save Changes?",
     *     "Do you want to save your changes before closing?",
     *     message_box_buttons::yes_no_cancel,
     *     [](dialog<Backend>::dialog_result result) {
     *         switch (result) {
     *             case dialog<Backend>::dialog_result::yes:
     *                 save_file();
     *                 break;
     *             case dialog<Backend>::dialog_result::no:
     *                 discard_changes();
     *                 break;
     *             default:
     *                 break;
     *         }
     *     }
     * );
     * @endcode
     *
     * @note The returned shared_ptr must be kept alive until the dialog closes.
     *       The dialog will auto-destroy when the shared_ptr goes out of scope.
     */
    template<UIBackend Backend>
    std::shared_ptr<dialog<Backend>> show_message_box(
        std::string title,
        std::string message,
        message_box_buttons buttons,
        std::function<void(typename dialog<Backend>::dialog_result)> callback
    ) {
        // Create dialog (owned by returned shared_ptr)
        auto dlg = std::make_shared<dialog<Backend>>(std::move(title));
        dlg->set_message(std::move(message));

        // Add appropriate buttons
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

        // Show modal with callback
        dlg->show_modal(std::move(callback));

        // Return shared_ptr so caller can keep dialog alive
        return dlg;
    }

    /**
     * @brief Show info message (OK button only, no callback)
     *
     * @tparam Backend UI backend type
     * @param message Message text to display
     * @param title Optional dialog title (defaults to "Information")
     *
     * @details
     * Displays a simple informational message with OK button only.
     * No callback is needed since there's only one possible result.
     *
     * ## Usage
     * @code
     * show_info<Backend>("File saved successfully!");
     * show_info<Backend>("Operation completed.", "Success");
     * @endcode
     *
     * @note The returned shared_ptr must be kept alive until the dialog closes.
     */
    template<UIBackend Backend>
    std::shared_ptr<dialog<Backend>> show_info(
        std::string message,
        std::string title = "Information"
    ) {
        auto dlg = std::make_shared<dialog<Backend>>(std::move(title));
        dlg->set_message(std::move(message));
        dlg->add_ok_button();
        dlg->show_modal();

        return dlg;
    }

    /**
     * @brief Show confirmation dialog (yes/no)
     *
     * @tparam Backend UI backend type
     * @param title Dialog title
     * @param message Message text to display
     * @param callback Callback with simplified boolean result (true=yes, false=no)
     *
     * @details
     * Shows a Yes/No confirmation dialog with simplified boolean callback.
     * True means "Yes", false means "No".
     *
     * ## Usage
     * @code
     * show_confirm<Backend>(
     *     "Delete File?",
     *     "This action cannot be undone.",
     *     [](bool confirmed) {
     *         if (confirmed) {
     *             delete_file();
     *         }
     *     }
     * );
     * @endcode
     *
     * @note The returned shared_ptr must be kept alive until the dialog closes.
     */
    template<UIBackend Backend>
    std::shared_ptr<dialog<Backend>> show_confirm(
        std::string title,
        std::string message,
        std::function<void(bool confirmed)> callback
    ) {
        auto dlg = std::make_shared<dialog<Backend>>(std::move(title));
        dlg->set_message(std::move(message));
        dlg->add_yes_no_buttons();

        // Wrap callback to convert dialog_result to bool
        dlg->show_modal([callback = std::move(callback)](typename dialog<Backend>::dialog_result result) {
            callback(result == dialog<Backend>::dialog_result::yes);
        });

        return dlg;
    }

    /**
     * @brief Show warning message box (OK and Cancel buttons)
     *
     * @tparam Backend UI backend type
     * @param title Dialog title
     * @param message Warning message text
     * @param callback Callback with boolean result (true=OK, false=Cancel)
     *
     * @details
     * Shows a warning dialog with OK/Cancel buttons.
     * Useful for warning users before proceeding with an action.
     *
     * ## Usage
     * @code
     * show_warning<Backend>(
     *     "Overwrite File?",
     *     "The file already exists. Do you want to overwrite it?",
     *     [](bool proceed) {
     *         if (proceed) {
     *             overwrite_file();
     *         }
     *     }
     * );
     * @endcode
     *
     * @note The returned shared_ptr must be kept alive until the dialog closes.
     */
    template<UIBackend Backend>
    std::shared_ptr<dialog<Backend>> show_warning(
        std::string title,
        std::string message,
        std::function<void(bool proceed)> callback
    ) {
        auto dlg = std::make_shared<dialog<Backend>>(std::move(title));
        dlg->set_message(std::move(message));
        dlg->add_ok_cancel_buttons();

        // Wrap callback to convert dialog_result to bool
        dlg->show_modal([callback = std::move(callback)](typename dialog<Backend>::dialog_result result) {
            callback(result == dialog<Backend>::dialog_result::ok);
        });

        return dlg;
    }

    /**
     * @brief Show error message (OK button only)
     *
     * @tparam Backend UI backend type
     * @param message Error message text
     * @param title Optional dialog title (defaults to "Error")
     *
     * @details
     * Displays an error message with OK button only.
     * Similar to show_info() but with "Error" as default title.
     *
     * ## Usage
     * @code
     * show_error<Backend>("Failed to open file: permission denied");
     * show_error<Backend>("Network timeout occurred", "Connection Error");
     * @endcode
     *
     * @note The returned shared_ptr must be kept alive until the dialog closes.
     */
    template<UIBackend Backend>
    std::shared_ptr<dialog<Backend>> show_error(
        std::string message,
        std::string title = "Error"
    ) {
        auto dlg = std::make_shared<dialog<Backend>>(std::move(title));
        dlg->set_message(std::move(message));
        dlg->add_ok_button();
        dlg->show_modal();

        return dlg;
    }

} // namespace onyxui
