/**
 * @file ui_host.hh
 * @brief Consolidated embedding-side host type for OnyxUI.
 *
 * See `docs/ONYXUI_UI_HOST_DESIGN.md` for the full design rationale.
 *
 * `ui_host<Backend>` is the consolidated embedding type. It
 * replaces the legacy two-step assembly (a context-scope RAII
 * wrapper plus a separate render/event handle) that consumers
 * previously had to wire up by hand.
 * It owns:
 *
 *   - the scoped service stack (themes, focus, hotkeys, input, layers, metrics)
 *   - the mounted root widget tree
 *   - overlay presentation state (via layer_manager)
 *
 * It does NOT own:
 *
 *   - the renderer (passed per `render()` call — the consumer owns it)
 *   - the event loop
 *   - any error queue (see `docs/ONYXUI_UI_HOST_DESIGN.md` §5 — error
 *     surfacing stays an app-level concern)
 *
 * ## Typical use
 *
 * @code
 * ui_host<my_backend> host(my_metrics);
 * host.mount(build_ui());
 *
 * // In the consumer's render loop:
 * host.render(renderer, logical_rect{0_lu, 0_lu, width, height});
 * while (auto ev = poll_event()) host.handle_event(*ev);
 *
 * // Overlay presentation — no explicit layer_manager argument.
 * auto dialog = host.present_modal(std::make_unique<window<my_backend>>("Error"));
 * @endcode
 */

#pragma once

#include <memory>
#include <optional>
#include <onyxui/concepts/backend.hh>
#include <onyxui/concepts/rect_like.hh>
#include <onyxui/core/backend_metrics.hh>
#include <onyxui/core/element.hh>
#include <onyxui/core/geometry.hh>
#include <onyxui/core/physical_conversions.hh>
#include <onyxui/events/event_router.hh>
#include <onyxui/events/hit_test_path.hh>
#include <onyxui/hotkeys/hotkey_action.hh>
#include <onyxui/services/ui_context.hh>
#include <onyxui/services/ui_services.hh>
#include <onyxui/widgets/window/presented_window.hh>
#include <onyxui/widgets/window/window.hh>

namespace onyxui {

    /**
     * @class ui_host
     * @brief One type to mount and run a UI inside a custom host.
     *
     * @tparam Backend The UI backend type satisfying UIBackend.
     *
     * Non-copyable, non-moveable. Services hold back-pointers; moving
     * would invalidate them.
     */
    template<UIBackend Backend>
    class ui_host {
    public:
        using widget_type    = ui_element<Backend>;
        using window_type    = window<Backend>;
        using renderer_type  = typename Backend::renderer_type;
        using event_type     = typename Backend::event_type;
        using rect_type      = typename Backend::rect_type;

        // --------------------------------------------------------------
        // Construction
        // --------------------------------------------------------------

        explicit ui_host(backend_metrics<Backend> metrics = backend_metrics<Backend>{})
            : m_ctx(std::move(metrics)) {
            // The ui_context services live here for the host's lifetime
            // but are NOT pushed onto the ambient scope stack. Push/pop
            // happens per-call inside render() / handle_event() /
            // present() / present_modal() via the RAII `scope` guard
            // below — between calls the host is dormant, matching the
            // contract in docs/ONYXUI_UI_HOST_DESIGN.md §7.
        }

        ~ui_host() {
            // WAR-64: fire on_detached on any still-mounted tree
            // before `m_root`'s unique_ptr tears it down, so
            // widgets that unregister hotkeys or drop cached theme
            // pointers in `on_detached` don't leak that state.
            // Push our ambient scope so those hook bodies see live
            // `ui_services<B>::...`.
            if (m_root) {
                scope guard(m_ctx);
                m_root->detach_subtree(*this);
            }
        }

        ui_host(const ui_host&) = delete;
        ui_host& operator=(const ui_host&) = delete;
        ui_host(ui_host&&) = delete;
        ui_host& operator=(ui_host&&) = delete;

        // --------------------------------------------------------------
        // Root widget tree
        // --------------------------------------------------------------

        void mount(std::unique_ptr<widget_type> root) {
            // Detach any prior root first under our pushed scope so
            // its `on_detached` hooks see live ambient services.
            if (m_root) {
                scope guard(m_ctx);
                m_root->detach_subtree(*this);
            }

            m_root = std::move(root);

            // Same for the new subtree: push scope before we let
            // the walk fire on_attached hooks.
            if (m_root) {
                scope guard(m_ctx);
                m_root->attach_subtree(*this);
            }
        }

        [[nodiscard]] widget_type* root() noexcept {
            return m_root.get();
        }

        [[nodiscard]] const widget_type* root() const noexcept {
            return m_root.get();
        }

        /// Unmount and return the root. Live overlays are UNAFFECTED
        /// (see `docs/ONYXUI_UI_HOST_DESIGN.md` §7.1) — they remain
        /// registered with the layer manager and the consumer's
        /// presented_window handles stay valid. Re-mount replaces the
        /// root without disturbing overlays.
        [[nodiscard]] std::unique_ptr<widget_type> unmount() {
            if (m_root) {
                scope guard(m_ctx);
                m_root->detach_subtree(*this);
            }
            return std::move(m_root);
        }

        // --------------------------------------------------------------
        // Per-frame entry points
        // --------------------------------------------------------------

        /// Render the mounted tree (if any) and any overlay layers into
        /// @p renderer. @p viewport is the logical bounds the UI should
        /// fill — the origin is honored, so sub-viewport hosts lay out
        /// and paint at the supplied (x, y) rather than (0, 0). If no
        /// root is mounted, overlays still render over an empty
        /// background.
        void render(renderer_type& renderer, logical_rect viewport) {
            scope guard(m_ctx);
            const auto& metrics = m_ctx.metrics();
            const physical_rect physical_viewport = metrics.snap_rect(viewport);
            const rect_type bounds =
                to_backend_rect<Backend>(physical_viewport);
            dispatch_render(renderer, bounds, viewport);
        }

        /// Convenience overload: query @p renderer for its physical
        /// viewport (including its physical origin, if non-zero) and
        /// render into that. Matches the shape of the legacy
        /// `ui_host::render()` call.
        void render(renderer_type& renderer) {
            const rect_type bounds = renderer.get_viewport();
            const auto& metrics = m_ctx.metrics();
            const logical_unit logical_x =
                metrics.physical_to_logical_x(
                    physical_x(rect_utils::get_x(bounds)));
            const logical_unit logical_y =
                metrics.physical_to_logical_y(
                    physical_y(rect_utils::get_y(bounds)));
            const logical_size logical_viewport_size =
                metrics.physical_to_logical_size(
                    typename Backend::size_type{
                        rect_utils::get_width(bounds),
                        rect_utils::get_height(bounds)
                    });
            render(renderer,
                   logical_rect{logical_x, logical_y,
                                logical_viewport_size.width,
                                logical_viewport_size.height});
        }

        /// Dispatch a backend-native event through the UI. Returns true
        /// if the event was consumed.
        [[nodiscard]] bool handle_event(const event_type& native_event) {
            scope guard(m_ctx);
            return dispatch_event(native_event);
        }

        // --------------------------------------------------------------
        // Overlay presentation
        // --------------------------------------------------------------

        /// Present a non-modal overlay. The returned presenter owns the
        /// window; dropping it dismisses the overlay.
        [[nodiscard]] presented_window<Backend> present(
            std::unique_ptr<window_type> win) {
            // The `window<Backend>::show()` path itself doesn't hit
            // ambient services, but downstream widget construction and
            // theme resolution inside the overlay's content may — so
            // we push the scope for the duration of presentation.
            scope guard(m_ctx);
            return presented_window<Backend>(
                m_ctx.layers(), std::move(win),
                presentation_kind::modeless);
        }

        /// Present a modal overlay. The returned presenter owns the
        /// window; dropping it dismisses the modal.
        [[nodiscard]] presented_window<Backend> present_modal(
            std::unique_ptr<window_type> win,
            dialog_position pos = dialog_position::center) {
            // `window::show_modal` consults ambient window_manager and
            // focus_manager to swap active-window and focus state, so
            // the scope must be pushed here.
            scope guard(m_ctx);
            return presented_window<Backend>(
                m_ctx.layers(), std::move(win),
                presentation_kind::modal, pos);
        }

        // --------------------------------------------------------------
        // Escape hatches for power users
        // --------------------------------------------------------------

        [[nodiscard]] auto& layers()         noexcept { return m_ctx.layers(); }
        [[nodiscard]] auto& themes()         noexcept { return m_ctx.themes(); }
        [[nodiscard]] auto& hotkeys()        noexcept { return m_ctx.hotkeys(); }
        [[nodiscard]] auto& hotkey_schemes() noexcept { return m_ctx.hotkey_schemes(); }
        [[nodiscard]] auto& input()          noexcept { return m_ctx.input(); }
        [[nodiscard]] auto& windows()        noexcept { return m_ctx.windows(); }
        [[nodiscard]] const auto& metrics() const noexcept { return m_ctx.metrics(); }

        // `with_scope(F&&)` was retired in WAR-64. It existed to
        // paper over constructor-time ambient lookups in widgets
        // (menu_bar's hotkey registration, text_view's theme
        // padding). Now those lookups happen in `on_attached`,
        // which `mount()` / `add_child` already run inside a
        // pushed scope — no wrapping call-site helper is needed.
        //
        // If you still need ambient services during arbitrary code
        // between render/event ticks (tests, one-shot utilities),
        // use `push_scope()` below and hold the returned RAII
        // token for the duration of the call block.

        /// RAII token that keeps this host's context pushed as the
        /// ambient one for its lifetime. Returned from
        /// `push_scope()`.
        ///
        /// This replaces the pre-`ui_host` `ui_host<B>`
        /// pattern: tests and fixtures that used to rely on the
        /// context being ambient for their whole lifetime can now
        /// hold a `scope_token` instead of the context itself, while
        /// the owning `ui_host` retains per-call scope management
        /// for production use.
        class [[nodiscard]] scope_token {
        public:
            /// Construct from the host that owns the context. This
            /// overload is the one struct/class members should use,
            /// because it can be spelled without `auto` and does not
            /// leak the internal `ui_context<B>` type:
            /// @code
            /// struct fixture {
            ///     ui_host<B> host;
            ///     typename ui_host<B>::scope_token scope{host};
            /// };
            /// @endcode
            ///
            /// Remembers a pointer to the host's context so that
            /// destruction pops *this* context rather than whatever
            /// happens to be on top. Tokens destroyed out of LIFO
            /// order therefore never silently pop the wrong host's
            /// scope — see `ui_services::pop_context(ctx*)`.
            explicit scope_token(ui_host& host) noexcept
                : m_ctx(&host.m_ctx) {
                ui_services<Backend>::push_context(m_ctx);
            }
            ~scope_token() noexcept {
                ui_services<Backend>::pop_context(m_ctx);
            }
            scope_token(const scope_token&) = delete;
            scope_token& operator=(const scope_token&) = delete;
            scope_token(scope_token&&) = delete;
            scope_token& operator=(scope_token&&) = delete;
        private:
            ui_context<Backend>* m_ctx;
        };

        /// Push this host's context onto the ambient services stack
        /// and keep it there until the returned `scope_token` goes
        /// out of scope. Most consumers want the per-call scoping
        /// that `render()` / `handle_event()` already apply — reach
        /// for `push_scope()` only when you need ambient services
        /// during arbitrary code between those calls (tests,
        /// fixture setup, one-shot utilities).
        [[nodiscard]] scope_token push_scope() noexcept {
            return scope_token{*this};
        }

    private:
        // --------------------------------------------------------------
        // Scope guard: pushes the host's context onto the ui_services
        // stack for the duration of a call, pops on exit. This is how
        // ui_host honors "dormant between calls" — two hosts on one
        // thread don't fight for the ambient slot; only the one
        // currently inside render() / handle_event() / present() is
        // the ambient owner.
        // --------------------------------------------------------------

        class scope {
        public:
            explicit scope(ui_context<Backend>& ctx) noexcept {
                ui_services<Backend>::push_context(&ctx);
            }
            ~scope() noexcept {
                ui_services<Backend>::pop_context();
            }
            scope(const scope&) = delete;
            scope& operator=(const scope&) = delete;
            scope(scope&&) = delete;
            scope& operator=(scope&&) = delete;
        };

        // --------------------------------------------------------------
        // Render pipeline
        //
        // Duplicates ui_host::render logic minus the owned
        // renderer. Now that ui_host is retired (WAR-58) the shared
        // pipeline moves into a common helper. The caller is expected
        // to have already pushed a `scope` guard.
        // --------------------------------------------------------------

        void dispatch_render(renderer_type& renderer,
                             const rect_type& bounds,
                             logical_rect logical_viewport) {
            // Gather dirty regions (from root + layer removals) before
            // painting.
            std::vector<rect_type> dirty_regions;
            if (m_root) {
                dirty_regions = m_root->get_and_clear_dirty_regions();
            }
            auto& layers_mgr = m_ctx.layers();
            const auto& removed = layers_mgr.get_removed_layer_dirty_regions();
            for (const auto& logical_region : removed) {
                dirty_regions.push_back(
                    to_backend_rect<Backend>(
                        m_ctx.metrics().snap_rect(logical_region)));
            }
            layers_mgr.clear_removed_layer_dirty_regions();

            // Paint background first.
            if (auto* bg = ui_services<Backend>::background()) {
                bg->render(renderer, bounds, dirty_regions);
            }

            // Measure/arrange the root tree in logical coordinates.
            // Honor the viewport's ORIGIN, not just its size — a
            // sub-viewport host positions its root at (viewport.x,
            // viewport.y) so absolute child coordinates are correct.
            auto* theme_ptr = themes_current_or_null();
            if (m_root) {
                [[maybe_unused]] auto measured =
                    m_root->measure(logical_viewport.width,
                                    logical_viewport.height);
                m_root->arrange(logical_viewport);

                if (theme_ptr) {
                    m_root->render(renderer, theme_ptr, m_ctx.metrics());
                }
            }

            // Overlays render last, on top of the root.
            if (theme_ptr) {
                layers_mgr.render_all_layers(renderer, bounds,
                                             theme_ptr, m_ctx.metrics());
            }
        }

        [[nodiscard]] auto* themes_current_or_null() noexcept {
            return m_ctx.themes().get_current_theme();
        }

        // --------------------------------------------------------------
        // Event pipeline
        //
        // Duplicates ui_host::handle_event. Same retirement note as
        // dispatch_render above.
        // --------------------------------------------------------------

        [[nodiscard]] bool dispatch_event(const event_type& native_event) {
            auto ui_evt_opt = Backend::create_event(native_event);
            if (!ui_evt_opt) return false;
            const ui_event& ui_evt = *ui_evt_opt;

            auto* layers_ptr  = ui_services<Backend>::layers();
            auto* input_ptr   = ui_services<Backend>::input();
            auto* hotkeys_ptr = ui_services<Backend>::hotkeys();

            // 1. Overlay layers get first dibs.
            if (layers_ptr && layers_ptr->route_event(ui_evt)) {
                return true;
            }

            // 2. Resize — re-layout the root.
            if (auto* resize_evt = std::get_if<resize_event>(&ui_evt)) {
                if (m_root) {
                    const auto& metrics = m_ctx.metrics();
                    const logical_unit logical_w =
                        metrics.physical_to_logical_x(physical_x(resize_evt->width));
                    const logical_unit logical_h =
                        metrics.physical_to_logical_y(physical_y(resize_evt->height));
                    [[maybe_unused]] auto measured =
                        m_root->measure(logical_w, logical_h);
                    m_root->arrange(logical_rect{
                        0.0_lu, 0.0_lu, logical_w, logical_h});
                }
                return true;
            }

            // 3. Keyboard.
            if (auto* kbd_evt = std::get_if<keyboard_event>(&ui_evt)) {
                if (!m_root) return false;

                if (input_ptr &&
                    input_ptr->handle_tab_navigation_in_tree(*kbd_evt, m_root.get())) {
                    return true;
                }

                widget_type* focused = nullptr;
                if (input_ptr) {
                    focused = static_cast<widget_type*>(input_ptr->get_focused());
                }

                if (hotkeys_ptr && hotkeys_ptr->handle_ui_event(ui_evt, focused)) {
                    return true;
                }

                if (focused && focused->accepts_keys_as_click() && hotkeys_ptr) {
                    if (hotkeys_ptr->matches_action(*kbd_evt,
                                                    hotkey_action::activate_focused)) {
                        return true;
                    }
                }

                if (focused && hotkeys_ptr) {
                    constexpr hotkey_action scroll_actions[] = {
                        hotkey_action::scroll_up,
                        hotkey_action::scroll_down,
                        hotkey_action::scroll_page_up,
                        hotkey_action::scroll_page_down,
                        hotkey_action::scroll_home,
                        hotkey_action::scroll_end,
                    };
                    for (auto action : scroll_actions) {
                        if (hotkeys_ptr->matches_action(*kbd_evt, action)) {
                            return focused->handle_semantic_action(action);
                        }
                    }
                }

                if (focused) {
                    return focused->handle_event(ui_evt, event_phase::target);
                }
                return false;
            }

            // 4. Text input.
            if (std::get_if<text_input_event>(&ui_evt)) {
                widget_type* focused = nullptr;
                if (input_ptr) {
                    focused = static_cast<widget_type*>(input_ptr->get_focused());
                }
                if (focused) {
                    return focused->handle_event(ui_evt, event_phase::target);
                }
                return false;
            }

            // 5. Mouse.
            if (auto* mouse_evt = std::get_if<mouse_event>(&ui_evt)) {
                if (!input_ptr || !m_root) return false;

                const bool is_wheel =
                    (mouse_evt->act == mouse_event::action::wheel_up ||
                     mouse_evt->act == mouse_event::action::wheel_down);
                if (is_wheel && layers_ptr) {
                    layers_ptr->clear_layers(layer_type::popup);
                }

                auto* captured = static_cast<widget_type*>(input_ptr->get_captured());
                widget_type* target = nullptr;
                hit_test_path<Backend> hit_path;
                if (captured) {
                    target = captured;
                } else {
                    target = m_root->hit_test_logical(
                        mouse_evt->x, mouse_evt->y, hit_path);
                }

                if (!captured) {
                    input_ptr->set_hover(target);
                }

                const bool is_button_event =
                    (mouse_evt->act == mouse_event::action::press ||
                     mouse_evt->act == mouse_event::action::release);
                const bool is_press =
                    (mouse_evt->act == mouse_event::action::press);

                if (is_button_event) {
                    if (is_press) {
                        input_ptr->handle_capture_transfer_on_press(
                            target,
                            [&](auto* old_capture) {
                                mouse_event release{
                                    .x = mouse_evt->x,
                                    .y = mouse_evt->y,
                                    .btn = mouse_event::button::left,
                                    .act = mouse_event::action::release,
                                    .modifiers = {}
                                };
                                static_cast<widget_type*>(old_capture)->handle_event(
                                    ui_event{release}, event_phase::target);
                            });
                        input_ptr->set_capture(target);
                    } else {
                        input_ptr->release_capture();
                        input_ptr->set_hover(target);
                    }
                }

                if (target) {
                    if (captured) {
                        return target->handle_event(ui_evt, event_phase::target);
                    }
                    return route_event(ui_evt, hit_path);
                }
            }

            return false;
        }

        // --------------------------------------------------------------
        // Members. m_ctx is declared first so it is destroyed LAST —
        // after the root tree and any overlay wrappers hosted in it.
        //
        // Note: it's a plain `ui_context`, not a `ui_host`.
        // The scope-stack push/pop lives in the `scope` RAII guard
        // created per-call inside render() / handle_event() / present()
        // / present_modal(), so the host is dormant between calls.
        // --------------------------------------------------------------

        ui_context<Backend> m_ctx;
        std::unique_ptr<widget_type> m_root;
    };

} // namespace onyxui
