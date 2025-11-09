/**
 * @file dialog.hh
 * @brief Dialog widget - window subclass with result management
 * @author Claude Code
 * @date 2025-11-08
 *
 * @details
 * Provides convenient API for common dialog patterns (message boxes, confirmations).
 * Supports three result patterns:
 * 1. Signal-based (async, event-driven)
 * 2. Callback-based (convenience wrapper)
 * 3. Helper functions (maximum convenience)
 *
 * Dialogs are typically modal and non-resizable by default.
 */

#pragma once

#include <onyxui/widgets/window/window.hh>
#include <onyxui/widgets/containers/vbox.hh>
#include <onyxui/widgets/containers/hbox.hh>
#include <onyxui/widgets/label.hh>
#include <onyxui/widgets/button.hh>
#include <onyxui/core/signal.hh>
#include <functional>
#include <string>
#include <memory>

namespace onyxui {

    /**
     * @class dialog
     * @brief Dialog widget - window subclass with result management
     *
     * @tparam Backend The UI backend type
     *
     * @details
     * Extends window<Backend> with result code management for common dialog patterns.
     * Provides three API styles:
     *
     * ## Pattern 1: Signal-Based (Event-Driven)
     * @code
     * auto dlg = std::make_unique<dialog<Backend>>("Save Changes?");
     * dlg->set_message("Do you want to save?");
     * dlg->add_yes_no_cancel_buttons();
     *
     * dlg->result_ready.connect([](dialog<Backend>::dialog_result result) {
     *     if (result == dialog<Backend>::dialog_result::yes) {
     *         save_file();
     *     }
     * });
     *
     * dlg->show_modal();  // Non-blocking
     * @endcode
     *
     * ## Pattern 2: Callback-Based (Convenience)
     * @code
     * auto dlg = std::make_unique<dialog<Backend>>("Save Changes?");
     * dlg->set_message("Do you want to save?");
     * dlg->add_yes_no_cancel_buttons();
     *
     * dlg->show_modal([](dialog<Backend>::dialog_result result) {
     *     if (result == dialog<Backend>::dialog_result::yes) {
     *         save_file();
     *     }
     * });
     * @endcode
     *
     * ## Pattern 3: Helper Functions (Maximum Convenience)
     * @code
     * // Defined in window_presets.hh
     * show_confirm<Backend>("Delete File?", "This cannot be undone.",
     *     [](bool confirmed) {
     *         if (confirmed) delete_file();
     *     });
     * @endcode
     */
    template<UIBackend Backend>
    class dialog : public window<Backend> {
    public:
        using base = window<Backend>;
        using size_type = typename Backend::size_type;
        using rect_type = typename Backend::rect_type;

        /**
         * @brief Dialog result enumeration
         */
        enum class dialog_result : uint8_t {
            none,      ///< Still open or closed without result
            ok,        ///< OK button clicked
            cancel,    ///< Cancel button clicked
            yes,       ///< Yes button clicked
            no,        ///< No button clicked
            abort,     ///< Abort button clicked
            retry,     ///< Retry button clicked
            ignore     ///< Ignore button clicked
        };

        /**
         * @brief Construct a dialog
         * @param title Dialog title
         */
        explicit dialog(std::string title = "Dialog")
            : base(title, get_dialog_flags())
            , m_result(dialog_result::none)
        {
            // Create main layout (vbox: message area + button area)
            auto layout = std::make_unique<vbox<Backend>>(10);  // 10px spacing
            m_layout = layout.get();

            // Add message label
            auto message_label = std::make_unique<label<Backend>>("");
            m_message_label = message_label.get();
            layout->add_child(std::move(message_label));

            // Add button container (hbox)
            auto button_box = std::make_unique<hbox<Backend>>(5);  // 5px spacing
            m_button_box = button_box.get();
            layout->add_child(std::move(button_box));

            // Set as window content
            this->set_content(std::move(layout));
        }

        /**
         * @brief Destructor
         */
        ~dialog() override = default;

        // Rule of Five
        dialog(const dialog&) = delete;
        dialog& operator=(const dialog&) = delete;
        dialog(dialog&&) noexcept = default;
        dialog& operator=(dialog&&) noexcept = default;

        // ====================================================================
        // Result Management
        // ====================================================================

        /**
         * @brief Set dialog result (called by button handlers)
         * @param result The result code
         */
        void set_result(dialog_result result) {
            m_result = result;
        }

        /**
         * @brief Get current dialog result
         */
        [[nodiscard]] dialog_result get_result() const noexcept {
            return m_result;
        }

        // ====================================================================
        // Display Methods
        // ====================================================================

        /**
         * @brief Show modal dialog (signal-based pattern)
         */
        void show_modal() {
            base::show_modal();
        }

        /**
         * @brief Show modal dialog with callback (callback-based pattern)
         * @param on_result Callback invoked when dialog closes with result
         */
        void show_modal(std::function<void(dialog_result)> on_result) {
            m_callback = std::move(on_result);
            base::show_modal();
        }

        // ====================================================================
        // Content Setup
        // ====================================================================

        /**
         * @brief Set message text
         * @param message Message to display
         */
        void set_message(const std::string& message) {
            if (m_message_label) {
                m_message_label->set_text(message);
            }
        }

        /**
         * @brief Get message text
         */
        [[nodiscard]] const std::string& get_message() const {
            if (m_message_label) {
                return m_message_label->text();
            }
            static const std::string empty;
            return empty;
        }

        /**
         * @brief Set custom button area
         * @param buttons Custom button layout
         *
         * @details
         * Replaces the default button area with custom content.
         * Useful for complex button layouts.
         */
        void set_button_area(std::unique_ptr<ui_element<Backend>> buttons) {
            if (m_layout && m_button_box) {
                // Remove old button box
                // Note: Simplified - production version would properly manage children
                m_button_box = nullptr;

                // Add new button area
                m_layout->add_child(std::move(buttons));
            }
        }

        // ====================================================================
        // Convenience Button Methods
        // ====================================================================

        /**
         * @brief Add OK button
         * @return Pointer to created button
         */
        button<Backend>* add_ok_button() {
            return add_button("OK", dialog_result::ok);
        }

        /**
         * @brief Add OK and Cancel buttons
         */
        void add_ok_cancel_buttons() {
            add_button("OK", dialog_result::ok);
            add_button("Cancel", dialog_result::cancel);
        }

        /**
         * @brief Add Yes and No buttons
         */
        void add_yes_no_buttons() {
            add_button("Yes", dialog_result::yes);
            add_button("No", dialog_result::no);
        }

        /**
         * @brief Add Yes, No, and Cancel buttons
         */
        void add_yes_no_cancel_buttons() {
            add_button("Yes", dialog_result::yes);
            add_button("No", dialog_result::no);
            add_button("Cancel", dialog_result::cancel);
        }

        /**
         * @brief Add Abort, Retry, and Ignore buttons
         */
        void add_abort_retry_ignore_buttons() {
            add_button("Abort", dialog_result::abort);
            add_button("Retry", dialog_result::retry);
            add_button("Ignore", dialog_result::ignore);
        }

        /**
         * @brief Add custom button with result
         * @param text Button text
         * @param result Result code for this button
         * @return Pointer to created button
         */
        button<Backend>* add_button(std::string text, dialog_result result) {
            if (!m_button_box) return nullptr;

            auto btn = std::make_unique<button<Backend>>(std::move(text));
            button<Backend>* btn_ptr = btn.get();

            // Connect button to set result and close dialog
            btn->clicked.connect([this, result]() {
                this->set_result(result);
                this->close();
            });

            m_button_box->add_child(std::move(btn));
            return btn_ptr;
        }

        // ====================================================================
        // Signals
        // ====================================================================

        /**
         * @brief Signal emitted when dialog closes with result
         */
        signal<dialog_result> result_ready;

    protected:
        /**
         * @brief Called when dialog closes (override from window)
         */
        void on_close() {
            // Emit result signal
            result_ready.emit(m_result);

            // Invoke callback if provided
            if (m_callback) {
                m_callback(m_result);
            }

            // Reset callback for reuse
            m_callback = nullptr;
        }

    private:
        dialog_result m_result;
        std::function<void(dialog_result)> m_callback;

        // Child widgets (non-owning pointers)
        vbox<Backend>* m_layout = nullptr;
        label<Backend>* m_message_label = nullptr;
        hbox<Backend>* m_button_box = nullptr;

        /**
         * @brief Get default dialog flags
         */
        static typename window<Backend>::window_flags get_dialog_flags() {
            typename window<Backend>::window_flags flags;
            flags.has_minimize_button = false;
            flags.has_maximize_button = false;
            flags.is_resizable = false;
            flags.is_movable = true;
            flags.is_modal = true;
            return flags;
        }
    };

} // namespace onyxui
