/**
 * @file app_window.cc
 * @brief sdlpp-backed implementation of `onyxui::simple::app_window`
 *        per docs/ONYXUI_SIMPLE_SHELL_DESIGN.md §6.1.
 *
 * Establishes the backend-fixed aliases in `onyxui::simple` (so the
 * non-standalone app_window header can parse), then implements the
 * class methods over an SDL_Window + renderer + ui_host<sdlpp_backend>.
 */

// 1. Backend-fixed aliases into onyxui::sdlpp::, and the ui_host /
//    window / ui_element templates we need to alias.
#include <onyxui/backend/sdlpp.hh>

// 2. Promote the canonical type list into onyxui::simple before
//    app_window.hh is parsed — the header uses unqualified
//    `ui_host` / `ui_element` names and needs the aliases in scope.
namespace onyxui::simple {
    using ::onyxui::sdlpp::backend;
    #define ONYXUI_TYPE(name) using ::onyxui::sdlpp::name;
    #include <onyxui/detail/public_types.inc>
    #undef ONYXUI_TYPE
} // namespace onyxui::simple

// 3. Now app_window.hh can be parsed with the aliases visible.
#include <onyxui/simple/app_window.hh>
#include <onyxui/simple/detail/runtime.hh>

#include <onyxui/sdlpp/sdlpp_backend.hh>
#include <onyxui/sdlpp/sdlpp_renderer.hh>

#include <sdlpp/core/core.hh>
#include <sdlpp/events/events.hh>
#include <sdlpp/events/event_types.hh>
#include <sdlpp/video/window.hh>
#include <sdlpp/video/renderer.hh>
#include <sdlpp/video/color.hh>

#include <atomic>
#include <iostream>
#include <memory>
#include <optional>
#include <string>

namespace onyxui::simple {

    namespace {
        // SDL must be initialised exactly once for the process. Ref-
        // count so the first app_window lights it up and the last one
        // tears it down.
        struct sdl_lifecycle {
            ::sdlpp::init core{::sdlpp::init_flags::video};
        };

        std::atomic<int> s_sdl_refcount = 0;
        std::unique_ptr<sdl_lifecycle> s_sdl_lifecycle;

        void sdl_retain() {
            if (s_sdl_refcount.fetch_add(1) == 0) {
                s_sdl_lifecycle = std::make_unique<sdl_lifecycle>();
            }
        }

        void sdl_release() {
            if (s_sdl_refcount.fetch_sub(1) == 1) {
                s_sdl_lifecycle.reset();
            }
        }
    } // anonymous namespace

    // -----------------------------------------------------------------
    // Pimpl
    // -----------------------------------------------------------------

    struct app_window::impl {
        std::string title;
        int initial_width;
        int initial_height;

        std::optional<::sdlpp::window>   sdl_window;
        std::optional<::sdlpp::renderer> sdl_renderer;
        std::optional<::onyxui::sdlpp::sdlpp_renderer> onyx_renderer;
        std::unique_ptr<ui_host>         host;

        bool open = false;
        bool close_requested = false;

        impl(std::string t, int w, int h)
            : title(std::move(t)),
              initial_width(w),
              initial_height(h) {
            sdl_retain();

            auto win_result = ::sdlpp::window::create(
                title, initial_width, initial_height,
                ::sdlpp::window_flags::resizable |
                ::sdlpp::window_flags::hidden);
            if (!win_result) {
                std::cerr << "[onyxui::simple::app_window] failed to create "
                          << "window: " << win_result.error() << "\n";
                sdl_release();
                throw std::runtime_error("sdlpp::window::create failed");
            }
            sdl_window.emplace(std::move(*win_result));

            auto renderer_result = sdl_window->create_renderer();
            if (!renderer_result) {
                std::cerr << "[onyxui::simple::app_window] failed to create "
                          << "renderer: " << renderer_result.error() << "\n";
                sdl_window.reset();
                sdl_release();
                throw std::runtime_error("window::create_renderer failed");
            }
            sdl_renderer.emplace(std::move(*renderer_result));
            sdl_renderer->set_vsync(1);

            // Configure metrics. 1:1 logical-to-physical with the
            // window's display scale applied for HiDPI.
            backend_metrics<backend> metrics;
            metrics.logical_to_physical_x = 1.0;
            metrics.logical_to_physical_y = 1.0;
            metrics.dpi_scale = static_cast<double>(sdl_window->display_scale());

            ::onyxui::sdlpp::sdlpp_backend::init(*sdl_renderer);
            host = std::make_unique<ui_host>(metrics);
            onyx_renderer.emplace(*sdl_renderer);
        }

        ~impl() {
            if (open) {
                detail::unregister_window(nullptr /*TODO owning pointer*/);
            }
            host.reset();
            onyx_renderer.reset();
            ::onyxui::sdlpp::sdlpp_backend::shutdown();
            sdl_renderer.reset();
            sdl_window.reset();
            sdl_release();
        }
    };

    // -----------------------------------------------------------------
    // Public API
    // -----------------------------------------------------------------

    app_window::app_window(std::string title, int width, int height)
        : pimpl_(std::make_unique<impl>(std::move(title), width, height)) {}

    app_window::~app_window() {
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
        if (pimpl_->sdl_window) {
            pimpl_->sdl_window->show();
        }
        detail::register_window(this);
        pimpl_->open = true;
    }

    void app_window::close() {
        if (!pimpl_->open) return;
        if (pimpl_->sdl_window) {
            pimpl_->sdl_window->hide();
        }
        detail::unregister_window(this);
        pimpl_->open = false;
        // Dropping the mounted root now would trigger widget
        // destruction outside any host scope. Let the pimpl destructor
        // handle it in declaration order so destruction runs with the
        // host still alive.
    }

    void app_window::set_title(const std::string& title) {
        pimpl_->title = title;
        if (pimpl_->sdl_window) {
            pimpl_->sdl_window->set_title(title);
        }
    }

    bool app_window::is_open() const noexcept {
        return pimpl_ && pimpl_->open;
    }

    ui_host& app_window::host() noexcept {
        return *pimpl_->host;
    }

    // --- runtime helpers (called by WAR-54's run()) ---

    void app_window::render_frame() {
        if (!pimpl_->open || !pimpl_->onyx_renderer || !pimpl_->sdl_renderer) {
            return;
        }
        // Clear the SDL back buffer to a neutral background. Dialog
        // helpers and widget chrome paint on top.
        pimpl_->sdl_renderer->set_draw_color(::sdlpp::color{192, 192, 192, 255});
        pimpl_->sdl_renderer->clear();

        pimpl_->host->render(*pimpl_->onyx_renderer);
    }

    bool app_window::pump_events() {
        // v1 simplification: drain ALL events from SDL's global queue
        // and route everything to this window's host. This works for
        // single-window tools (the default simple-shell use case).
        // Multi-window apps need event_queue::poll in the run() loop
        // with explicit window-id routing — tracked for a future
        // revision.
        while (auto event = ::sdlpp::event_queue::poll()) {
            if (event->type() == ::sdlpp::event_type::quit) {
                pimpl_->close_requested = true;
                return true;
            }
            if (pimpl_->host) {
                (void)pimpl_->host->handle_event(*event);
            }
        }
        return pimpl_->close_requested;
    }

    void app_window::present() {
        if (pimpl_->sdl_renderer && pimpl_->open) {
            pimpl_->sdl_renderer->present();
        }
    }

} // namespace onyxui::simple
