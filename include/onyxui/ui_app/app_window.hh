/**
 * @file app_window.hh
 * @brief Top-level standalone-tool window for the simple shell.
 *
 * This header is NOT standalone. It declares `onyxui::ui_app::app_window`
 * referencing the unqualified names `ui_host`, `ui_element` and `window`
 * in the enclosing namespace. Those names are brought into
 * `onyxui::ui_app` by the bundle header (`<onyxui/for/sdlpp.hh>`)
 * BEFORE this header is included. Including this file directly without
 * a bundle header in scope is a compile error by construction — see
 * `docs/ONYXUI_SIMPLE_SHELL_DESIGN.md` §5.2.
 *
 * Implementation lives in a backend-specific translation unit (one per
 * backend — `backends/sdlpp/src/simple/app_window.cc` for sdlpp). The
 * class is not a template; exactly one backend's implementation is
 * linked per binary per the compile-time backend rule (§4).
 */

#pragma once

// Guardrail: the simple/* headers are NOT standalone. They use the
// unqualified names `ui_element`, `ui_host` and `window` in namespace
// `onyxui::ui_app`, which a bundle header (`<onyxui/for/sdlpp.hh>`)
// populates via using-declarations BEFORE this file is parsed.
// Including this header directly would produce a cryptic cascade of
// "unknown type" errors; this check turns that into a readable
// message.
#ifndef ONYXUI_UI_APP_BUNDLE_INCLUDED
#  error "<onyxui/ui_app/app_window.hh> is not a standalone header. " \
         "Include a bundle header instead — typically " \
         "<onyxui/for/sdlpp.hh>. See " \
         "docs/ONYXUI_SIMPLE_SHELL_DESIGN.md §5.2."
#endif

#include <cstddef>
#include <iosfwd>
#include <memory>
#include <string>

namespace onyxui::ui_app {

    // The three names this class's API mentions are brought in by the
    // bundle header via using-declarations before this file is parsed:
    //   using ::onyxui::sdlpp::ui_element;
    //   using ::onyxui::sdlpp::ui_host;
    //   using ::onyxui::sdlpp::window;
    // If they're not in scope, the compiler will reject the
    // declarations below — which is exactly what we want.

    /**
     * @brief Top-level OS window owning one mounted UI tree.
     *
     * Non-copyable, non-moveable. Internally owns an OS window, a
     * renderer bound to it, and a `ui_host`. Construction creates the
     * OS surface in a hidden state; call `show()` to make it visible
     * and register it with the simple-shell run loop.
     */
    class app_window {
    public:
        app_window(std::string title, int width, int height);
        ~app_window();

        app_window(const app_window&) = delete;
        app_window& operator=(const app_window&) = delete;
        app_window(app_window&&) = delete;
        app_window& operator=(app_window&&) = delete;

        /// Mount the root widget on the internal ui_host. Replaces any
        /// previously mounted root.
        void set_content(std::unique_ptr<ui_element> root);

        /// Make the OS window visible and register it with the run
        /// loop. Idempotent — calling on an already-shown window is a
        /// no-op.
        void show();

        /// Hide the OS window and unregister it from the run loop.
        /// After `close()`, `is_open()` returns false and the window
        /// is no longer driven by `onyxui::ui_app::run()`.
        ///
        /// The mounted root is NOT destroyed — a subsequent `show()`
        /// brings the same UI tree back. Consumers that want the old
        /// tree gone should either mount a fresh root via
        /// `set_content(…)` before calling `show()` again, or unmount
        /// via `host().unmount()`.
        ///
        /// Idempotent.
        void close();

        /// Update the OS window title.
        void set_title(const std::string& title);

        [[nodiscard]] bool is_open() const noexcept;

        /// Present a top-level `window` widget modally over this app
        /// window. Takes ownership; the presenter is stored
        /// internally and dropped automatically on the next run-loop
        /// tick after the window's `closed` signal fires (typically
        /// when the user clicks a dismiss button that calls
        /// `window::close()`).
        ///
        /// Consumers needing a result callback may connect to
        /// `win->closed` in any order relative to the internal
        /// cleanup slot — the window is not destroyed from inside
        /// `closed` emission. Removal is deferred to
        /// `drain_pending_modals()`, which runs between frames, so
        /// every slot in the signal sees the window alive regardless
        /// of dispatch order.
        ///
        /// No-op if this app_window is not `is_open()` — the dialog
        /// would otherwise attach to a host that's being torn down.
        ///
        /// `message_box`, `confirm`, `input_dialog` are thin wrappers
        /// around this method.
        void show_modal(std::unique_ptr<window> win);

        /// Number of modal windows currently presented via
        /// `show_modal`, including any that have fired `closed` but
        /// haven't been drained yet. For tests and diagnostics.
        [[nodiscard]] std::size_t modal_count() const noexcept;

        /// Access the underlying `ui_host`. Advanced hatch for
        /// non-modal overlays, service lookups, and anything outside
        /// the simple-shell happy path. `show_modal` covers the
        /// modal-dialog case — most simple-shell code never needs
        /// this method.
        [[nodiscard]] ui_host& host() noexcept;

        /// Dump a backend-specific textual representation of the
        /// current frame into @p sink. Forwards directly to the
        /// backend renderer's `take_screenshot` routine; this method
        /// does not normalise the output format.
        ///
        /// Current backends write debug/diagnostic text (the sdlpp
        /// renderer emits a short header with viewport dimensions;
        /// the conio renderer emits character-cell art of the
        /// terminal buffer). Neither produces an image binary — a
        /// real PPM/PNG path would require work on the renderer
        /// side, not here.
        ///
        /// No-op if the renderer is not yet initialised (e.g.
        /// called before `show()`).
        void take_screenshot(std::ostream& sink) const;

        // ----------------------------------------------------------
        // Simple-shell runtime internals. Public so the `run()` loop
        // can drive frames without friend-class boilerplate. Consumers
        // should not call these directly.
        // ----------------------------------------------------------

        /// Render one frame (called by `onyxui::ui_app::run()` each
        /// loop iteration). Drains any `show_modal` presenters whose
        /// window has already fired `closed` before drawing, so
        /// dismissals that landed during last frame's event pump
        /// take effect before this frame paints.
        void render_frame();

        /// Remove any modals that have fired `closed` and are
        /// awaiting cleanup. Exposed for tests and for unusual
        /// embed-the-run-loop callers that need to poke the deferred
        /// teardown at a specific point. `render_frame()` calls this
        /// on every tick; most consumers never need it directly.
        void drain_pending_modals();

        /// Flip the back buffer (called by `run()` after all windows
        /// have rendered).
        void present();

        /// Dispatch a pre-polled backend-native event to this window's
        /// host. The `native_event` pointer is cast to the backend's
        /// event type inside the implementation — keeping this
        /// declaration backend-agnostic. Returns true if the event
        /// signalled a close request (e.g. SDL_QUIT).
        ///
        /// The run loop (`onyxui::ui_app::run()`) polls the OS event
        /// queue ONCE per frame and calls this on the target window —
        /// app_window does not pump its own queue. This avoids the
        /// multi-window misrouting that per-window polling would cause.
        [[nodiscard]] bool dispatch_native_event(const void* native_event);

    private:
        struct impl;
        std::unique_ptr<impl> pimpl_;
    };

} // namespace onyxui::ui_app
