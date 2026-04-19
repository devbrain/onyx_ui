/**
 * @file app_window.cc
 * @brief conio-backed implementation of `onyxui::simple::app_window`
 *        per docs/ONYXUI_SIMPLE_SHELL_DESIGN.md §6.1.
 *
 * Implements the class methods over a termbox2 terminal + conio_renderer
 * + ui_host<conio_backend>. Differs from the sdlpp impl in three
 * notable ways:
 *
 *   1. No OS window — termbox2 owns the terminal surface. `show()` /
 *      `hide()` are effectively no-ops; termbox2 is live for the
 *      duration of the conio_renderer's lifetime.
 *   2. termbox2 init/shutdown is RAII-owned by the vram struct inside
 *      `conio_renderer`. No process-level ref-counting needed.
 *   3. The width/height constructor arguments are advisory — the
 *      terminal's actual dimensions come from tb_width()/tb_height().
 */

// This TU plays the same role as a bundle header — it promotes the
// aliases into onyxui::simple and then includes the simple/* header.
// Signal that to the guardrail check in the header.
#define ONYXUI_SIMPLE_BUNDLE_INCLUDED 1

// 1. Backend-fixed aliases into onyxui::conio::.
#include <onyxui/backend/conio.hh>

// 2. Promote the canonical type list into onyxui::simple before
//    app_window.hh is parsed — the header uses unqualified
//    `ui_host` / `ui_element` / `window` names and needs the aliases
//    in scope.
namespace onyxui::simple {
    using ::onyxui::conio::backend;
    #define ONYXUI_TYPE(name) using ::onyxui::conio::name;
    #include <onyxui/detail/public_types.inc>
    #undef ONYXUI_TYPE
} // namespace onyxui::simple

// 3. Now app_window.hh can be parsed with the aliases visible.
#include <onyxui/simple/app_window.hh>
#include <onyxui/simple/detail/runtime.hh>

#include <onyxui/conio/conio_backend.hh>
#include <onyxui/conio/conio_renderer.hh>

#include <algorithm>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace onyxui::simple {

    // -----------------------------------------------------------------
    // Pimpl
    // -----------------------------------------------------------------

    struct app_window::impl {
        std::string title;
        int initial_width;
        int initial_height;

        // `conio_renderer` owns the `vram` that calls tb_init() on
        // construction and tb_shutdown() on destruction. Wrapping in
        // std::optional lets us defer termbox initialisation — the
        // host tree is built (inside push_scope) before any terminal
        // output is drawn.
        std::optional<::onyxui::conio::conio_renderer> onyx_renderer;
        std::unique_ptr<ui_host> host;

        // Live modal presenters — same design as the sdlpp backend
        // (see backends/sdlpp/src/simple/app_window.cc for the full
        // rationale). Destruction order is enforced by declaring
        // `modals` AFTER `host` so presenters die before the
        // layer_manager they target. Entries are mark-don't-destroy
        // to decouple signal slot dispatch from window teardown.
        struct live_modal {
            window* key;
            presented_window presenter;
            bool pending_destroy = false;
        };
        std::vector<live_modal> modals;

        bool open = false;
        bool close_requested = false;

        impl(std::string t, int w, int h)
            : title(std::move(t)),
              initial_width(w),
              initial_height(h) {
            // Configure metrics. Terminal is 1 logical unit = 1
            // character cell; no DPI scaling.
            auto metrics =
                ::onyxui::make_terminal_metrics<backend>();
            host = std::make_unique<ui_host>(metrics);

            // Bring up termbox2 via the renderer. This must happen
            // AFTER the host is constructed; constructing the renderer
            // captures the current terminal dimensions, so the host's
            // mount() / render() calls see a live surface. Any output
            // written to stdout/stderr before this point is fine —
            // termbox2 clears the screen on init.
            onyx_renderer.emplace();
        }

        ~impl() {
            if (open) {
                detail::unregister_window(nullptr);
            }
            // Reverse-declaration-order destruction does the rest:
            //   modals → host → onyx_renderer (which tears down
            //   termbox via vram).
        }
    };

    // -----------------------------------------------------------------
    // Public API
    // -----------------------------------------------------------------

    app_window::app_window(std::string title, int width, int height)
        : pimpl_(std::make_unique<impl>(std::move(title), width, height)) {}

    app_window::~app_window() {
        // Modal dialogs live in pimpl_->modals; pimpl destruction
        // reverses declaration order and runs the presenter dtors
        // before the host they target. No explicit dismiss needed.
        if (pimpl_ && pimpl_->open) {
            detail::unregister_window(this);
            pimpl_->open = false;
        }
    }

    void app_window::set_content(std::unique_ptr<ui_element> root) {
        pimpl_->host->mount(std::move(root));
    }

    void app_window::show() {
        if (pimpl_->open) return;
        // termbox2 has no separate "show" — it's already displaying
        // into the terminal. Register with the run loop so run()
        // drives frames for us.
        detail::register_window(this);
        pimpl_->open = true;
    }

    void app_window::close() {
        if (!pimpl_->open) return;

        // Drop any live modal dialogs before we unregister. Clearing
        // the vector runs each presenter's dtor, which destroys its
        // window; the connected close-slots become no-ops once the
        // entry is gone (see the guarded-erase logic in show_modal).
        pimpl_->modals.clear();

        detail::unregister_window(this);
        pimpl_->open = false;
        // As with the sdlpp backend, leave the mounted root for
        // pimpl destruction to handle in declaration order so it
        // runs with the host still alive.
    }

    void app_window::set_title(const std::string& title) {
        // termbox2 has no concept of a window title — terminals
        // own their window chrome. Update the stored title so tests
        // and diagnostics can still read it back.
        pimpl_->title = title;
    }

    bool app_window::is_open() const noexcept {
        return pimpl_ && pimpl_->open;
    }

    ui_host& app_window::host() noexcept {
        return *pimpl_->host;
    }

    void app_window::take_screenshot(std::ostream& sink) const {
        if (pimpl_ && pimpl_->onyx_renderer) {
            pimpl_->onyx_renderer->take_screenshot(sink);
        }
    }

    void app_window::show_modal(std::unique_ptr<window> win) {
        if (!pimpl_->open || !win) return;

        drain_pending_modals();

        auto* raw = win.get();
        auto presenter = pimpl_->host->present_modal(std::move(win));
        pimpl_->modals.push_back({raw, std::move(presenter), false});

        // Mark-don't-destroy: the actual removal happens in
        // `drain_pending_modals()` on the next run-loop tick, so
        // caller-connected `closed` slots can't race with window
        // teardown regardless of signal dispatch order.
        raw->closed.connect([this, raw]() {
            auto& m = pimpl_->modals;
            auto it = std::find_if(m.begin(), m.end(),
                [raw](const impl::live_modal& lm) { return lm.key == raw; });
            if (it != m.end()) {
                it->pending_destroy = true;
            }
        });
    }

    std::size_t app_window::modal_count() const noexcept {
        return pimpl_ ? pimpl_->modals.size() : 0;
    }

    void app_window::drain_pending_modals() {
        if (!pimpl_) return;
        auto& m = pimpl_->modals;
        m.erase(
            std::remove_if(m.begin(), m.end(),
                [](const impl::live_modal& lm) {
                    return lm.pending_destroy;
                }),
            m.end());
    }

    // --- runtime helpers (called by run()) ---

    void app_window::render_frame() {
        if (!pimpl_->open || !pimpl_->onyx_renderer) {
            return;
        }

        drain_pending_modals();

        pimpl_->host->render(*pimpl_->onyx_renderer);
    }

    bool app_window::dispatch_native_event(const void* native_event) {
        if (!native_event || !pimpl_->open) return false;
        const auto& ev = *static_cast<const tb_event*>(native_event);

        // termbox2 has no "quit" event — the terminal never closes
        // from under us. Quit requests come from widget code calling
        // `simple::quit()` or the backend's should_quit() flag.
        if (pimpl_->host) {
            (void)pimpl_->host->handle_event(ev);
        }
        return false;
    }

    void app_window::present() {
        if (pimpl_->onyx_renderer && pimpl_->open) {
            pimpl_->onyx_renderer->present();
        }
    }

} // namespace onyxui::simple
