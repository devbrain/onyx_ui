/**
 * @file dialogs.cc
 * @brief sdlpp-backed implementation of the simple-shell dialog
 *        helpers per docs/ONYXUI_SIMPLE_SHELL_DESIGN.md §6.3.
 */

// 1. Backend-fixed aliases under onyxui::sdlpp::.
#include <onyxui/backend/sdlpp.hh>

// 2. Promote them into onyxui::simple:: before any simple/* header
//    is parsed.
namespace onyxui::simple {
    using ::onyxui::sdlpp::backend;
    #define ONYXUI_TYPE(name) using ::onyxui::sdlpp::name;
    #include <onyxui/detail/public_types.inc>
    #undef ONYXUI_TYPE
}

#include <onyxui/simple/app_window.hh>
#include <onyxui/simple/dialogs.hh>
#include <onyxui/simple/detail/runtime.hh>

#include <memory>
#include <string>
#include <utility>

namespace onyxui::simple {

    // Small shorthands for the onyxui:: primitives we reach for.
    // Qualified explicitly so the file's intent is unambiguous.
    using ::onyxui::horizontal_alignment;
    using ::onyxui::logical_thickness;
    using ::onyxui::logical_unit;
    using ::onyxui::size_policy;
    using ::onyxui::spacing;

    namespace {

        // Present a modal `window`, wire its `closed` signal to call
        // the user hook and dismiss via the live-dialog registry.
        // Returns the raw window pointer so callers can finish
        // building the content before handing off.
        //
        // The registry holds the presenter (owned by the disposer
        // lambda). When the dialog's close path fires `closed`, the
        // connected slot runs the user hook and then calls
        // dismiss_live_dialog — which runs the disposer, dropping
        // the presenter and destroying the window. Safe against
        // self-destruction during signal emit thanks to the
        // post-WAR-48 signal fix.
        // The disposer passed to register_live_dialog() must be
        // copy-constructible (std::function requirement). We hold
        // the presenter via std::shared_ptr so the capturing lambda
        // stays copyable — there's only ever one ref (the registry),
        // so it behaves like a unique_ptr with a copyable wrapper.
        window* present_and_register(
            app_window& parent,
            std::unique_ptr<window> win,
            std::function<void()> on_close_hook) {
            auto* raw = win.get();
            auto presenter = std::make_shared<presented_window>(
                parent.host().present_modal(std::move(win)));

            raw->closed.connect([raw, hook = std::move(on_close_hook)]() {
                if (hook) hook();
                detail::dismiss_live_dialog(raw);
            });

            detail::register_live_dialog(
                raw,
                [presenter]() { /* refcount drops when disposer dies */ });

            return raw;
        }

        // Build a simple "title bar + message label" window. Caller
        // adds the button row via `add_button_row`.
        std::unique_ptr<window> build_message_window(
            const std::string& title,
            const std::string& message) {
            typename window::window_flags flags;
            flags.is_resizable = false;
            flags.is_movable = true;
            flags.has_close_button = false;
            flags.has_minimize_button = false;
            flags.has_maximize_button = false;

            auto w = std::make_unique<window>(title, flags);
            w->set_width_constraint({size_policy::fixed, logical_unit(360)});
            w->set_window_focus(true);

            auto content = std::make_unique<::onyxui::vbox<backend>>(spacing::medium);
            content->set_margin(logical_thickness(
                logical_unit(16), logical_unit(12),
                logical_unit(16), logical_unit(12)));
            content->template emplace_child<::onyxui::label>(message);

            w->set_content(std::move(content));
            return w;
        }

        // Add a right-aligned button row and return the hbox so
        // callers can put buttons in it.
        hbox* add_button_row(window* w) {
            auto* content = dynamic_cast<::onyxui::vbox<backend>*>(w->get_content());
            if (!content) return nullptr;
            content->template emplace_child<::onyxui::spacer>(0, 4);
            auto* row = content->template emplace_child<::onyxui::hbox>(spacing::small);
            row->set_horizontal_align(horizontal_alignment::right);
            return row;
        }

    } // anonymous namespace

    // --------------------------------------------------------------

    void message_box(app_window& parent,
                     const std::string& title,
                     const std::string& message) {
        auto w = build_message_window(title, message);
        auto* raw = w.get();

        auto* row = add_button_row(raw);
        if (row) {
            auto* ok = row->template emplace_child<::onyxui::button>("OK");
            ok->clicked.connect([raw]() { raw->close(); });
        }

        (void)present_and_register(
            parent, std::move(w),
            []() { /* no user callback for message_box */ });
    }

    void error_box(app_window& parent,
                   const std::string& message,
                   const std::string& title) {
        // Visual styling (red border, icon, etc.) is a future nicety.
        // v1 delegates to message_box with an error title.
        message_box(parent, title, message);
    }

    void confirm(app_window& parent,
                 const std::string& title,
                 const std::string& message,
                 std::function<void(bool yes)> on_result) {
        auto w = build_message_window(title, message);
        auto* raw = w.get();

        auto result = std::make_shared<bool>(false);

        auto* row = add_button_row(raw);
        if (row) {
            auto* yes_btn = row->template emplace_child<::onyxui::button>("Yes");
            yes_btn->clicked.connect([raw, result]() {
                *result = true;
                raw->close();
            });
            auto* no_btn = row->template emplace_child<::onyxui::button>("No");
            no_btn->clicked.connect([raw, result]() {
                *result = false;
                raw->close();
            });
        }

        (void)present_and_register(
            parent, std::move(w),
            [result, cb = std::move(on_result)]() {
                if (cb) cb(*result);
            });
    }

    void input_dialog(app_window& parent,
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
        w->set_width_constraint({size_policy::fixed, logical_unit(400)});
        w->set_window_focus(true);

        auto content = std::make_unique<::onyxui::vbox<backend>>(spacing::small);
        content->set_margin(logical_thickness(
            logical_unit(16), logical_unit(12),
            logical_unit(16), logical_unit(12)));

        content->template emplace_child<::onyxui::label>(prompt);
        auto* input = content->template emplace_child<::onyxui::line_edit>();
        input->set_visible_chars(40);

        content->template emplace_child<::onyxui::spacer>(0, 4);
        auto* row = content->template emplace_child<::onyxui::hbox>(spacing::small);
        row->set_horizontal_align(horizontal_alignment::right);

        auto* raw = w.get();
        auto result_ok = std::make_shared<bool>(false);
        auto result_value = std::make_shared<std::string>();

        auto* ok_btn = row->template emplace_child<::onyxui::button>("OK");
        ok_btn->clicked.connect([raw, input, result_ok, result_value]() {
            *result_ok = true;
            *result_value = input->text();
            raw->close();
        });
        auto* cancel_btn = row->template emplace_child<::onyxui::button>("Cancel");
        cancel_btn->clicked.connect([raw, result_ok, result_value]() {
            *result_ok = false;
            result_value->clear();
            raw->close();
        });

        w->set_content(std::move(content));

        (void)present_and_register(
            parent, std::move(w),
            [result_ok, result_value, cb = std::move(on_result)]() {
                if (cb) cb(*result_ok, *result_value);
            });
    }

} // namespace onyxui::simple
