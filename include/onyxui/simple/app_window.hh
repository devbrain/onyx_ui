/**
 * @file app_window.hh
 * @brief Top-level standalone-tool window for the simple shell.
 *
 * This header is NOT standalone. It declares `onyxui::simple::app_window`
 * referencing the unqualified names `ui_host` and `ui_element` in the
 * enclosing namespace. Those names are brought into `onyxui::simple` by
 * the bundle header (`<onyxui/for/sdlpp.hh>` or `<onyxui/for/conio.hh>`)
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

#include <memory>
#include <string>

namespace onyxui::simple {

    // The two names this class's API mentions are brought in by the
    // bundle header via using-declarations before this file is parsed:
    //   using ::onyxui::sdlpp::ui_element;
    //   using ::onyxui::sdlpp::ui_host;
    // (or the conio equivalents). If neither is in scope, the
    // compiler will reject the declarations below — which is exactly
    // what we want.

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

        /// Hide the OS window, unregister from the run loop, and
        /// destroy the mounted root. After `close()`, `is_open()`
        /// returns false and the window is no longer driven by
        /// `onyxui::simple::run()`. Idempotent.
        void close();

        /// Update the OS window title.
        void set_title(const std::string& title);

        [[nodiscard]] bool is_open() const noexcept;

        /// Access the underlying `ui_host` — used by dialog helpers
        /// (`message_box`, `confirm`, …) and by advanced consumers
        /// that need overlay presentation or escape-hatch service
        /// lookups. Most simple-shell code never calls this.
        [[nodiscard]] ui_host& host() noexcept;

        // ----------------------------------------------------------
        // Simple-shell runtime internals. These are not intended for
        // consumer use; they're public so the runtime (`run()` loop)
        // can drive frames without friend-class boilerplate. A
        // consumer calling them directly will get the same behavior
        // the loop gets, which is fine but redundant.
        // ----------------------------------------------------------

        /// Render one frame (called by `onyxui::simple::run()` each
        /// loop iteration).
        void render_frame();

        /// Pump any pending OS events targeting this window and
        /// forward them to the ui_host. Returns true if a close
        /// request was observed on this window.
        [[nodiscard]] bool pump_events();

        /// Flip the back buffer (called by `run()` after all windows
        /// have rendered).
        void present();

    private:
        struct impl;
        std::unique_ptr<impl> pimpl_;
    };

} // namespace onyxui::simple
