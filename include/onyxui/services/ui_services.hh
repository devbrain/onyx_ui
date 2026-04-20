/**
 * @file ui_services.hh
 * @brief Global service registry with push/pop context stack pattern
 * @author Assistant
 * @date 2025-10-20
 *
 * @details
 * Provides centralized access to UI services using a push/pop context stack.
 * Contexts can be nested, allowing multiple independent UI instances.
 *
 * ## Design Rationale
 *
 * Stack-based, thread-local:
 *
 * 1. **Per-call scoping**: `ui_host<B>` owns its services but is
 *    dormant between calls. `render()` / `handle_event()` /
 *    `present()` push the context for their duration via an
 *    internal RAII guard, then pop on return. The host does NOT
 *    auto-push from its constructor.
 * 2. **Opt-in lifetime scoping**: code that needs ambient services
 *    outside the per-call entry points (widget factories, test
 *    fixtures) can use `ui_host::push_scope()` — it returns a
 *    `scope_token` that keeps the context on the stack for its
 *    own lifetime.
 * 3. **Multiple UIs**: Each `ui_host<B>` owns an independent
 *    context; stack depth >1 represents legitimate nesting
 *    (e.g. an embedded host whose `render()` is called from inside
 *    another host's event handler).
 * 4. **Thread Safe**: Each thread has its own context stack.
 *
 * ## Thread Safety
 *
 * Context stacks are thread_local, so each thread maintains its own stack.
 * This allows multi-threaded rendering with independent UI contexts per thread.
 *
 * ## Usage
 *
 * Consumers of the simple shell (`<onyxui/for/sdlpp.hh>`) never
 * touch `ui_services` directly — `app_window` drives the scope
 * push/pop for them. Engine embedders holding a `ui_host<B>`
 * query `ui_services<B>::themes()` etc. from inside handler
 * callbacks (where a scope is already active).
 *
 * For arbitrary out-of-call access (test setup, factories):
 *
 * @code
 * ui_host<Backend> host(metrics);
 * auto guard = host.push_scope();       // ambient active from here
 * ui_services<Backend>::themes()->...; // lookups now resolve
 * // guard dies at end of scope — ambient pops
 * @endcode
 */

#pragma once

#define ONYXUI_UI_SERVICES_HH_INCLUDED

#include <onyxui/concepts/backend.hh>
#include <onyxui/services/ui_context.hh>
#include <algorithm>
#include <vector>
#include <cassert>

namespace onyxui {

    // Forward declarations
    template<UIBackend Backend> class layer_manager;
    template<UIBackend Backend> class ui_element;
    template<UIBackend Backend, typename ElementType> class hierarchical_input_manager;
    template<UIBackend Backend> class theme_registry;
    template<UIBackend Backend> class background_renderer;
    template<UIBackend Backend> class window_manager;
    template<UIBackend Backend> struct backend_metrics;
    class hotkey_scheme_registry;

    /**
     * @class ui_services
     * @brief Global service registry with push/pop context stack
     *
     * @tparam Backend The UI backend type
     *
     * @details
     * Manages a thread-local stack of ui_context instances. The top of the stack
     * is the "current" context, which provides access to services.
     *
     * ## Stack Operations
     *
     * - **push_context(ctx)**: Push a context onto the stack (makes it current)
     * - **pop_context()**: Pop the current context (restores previous)
     * - **current()**: Get the current context (top of stack)
     *
     * ## Service Access
     *
     * - **layers()**: Get layer manager from current context
     * - **input()**: Get input manager from current context
     * - **themes()**: Get theme registry from current context
     *
     * All accessors return nullptr if no context is active (safe).
     *
     * ## When are the accessors non-null?
     *
     * - **Inside a `ui_host` per-call entry point**
     *   (`render()` / `handle_event()` / `present()`): yes.
     *   The host's RAII scope guard has pushed its context.
     * - **Inside code guarded by `ui_host::push_scope()`**:
     *   yes. The returned `scope_token` keeps the context on
     *   the stack for its own lifetime.
     * - **Everywhere else** (from a freshly-constructed `ui_host`'s
     *   perspective, or from unrelated code on the same thread):
     *   the accessors return nullptr. `ui_host<B>` does NOT
     *   auto-push from its constructor.
     *
     * @example Typical engine embedder flow
     * @code
     * onyxui::ui_host<Backend> host(metrics);
     * host.mount(build_ui());
     *
     * while (running) {
     *     // Per-call scope guard pushes/pops internally:
     *     host.handle_event(evt);   // ui_services::... works during this call
     *     host.render(renderer);    // and during this one
     * }                             // between calls, accessors return nullptr
     * @endcode
     *
     * @example Ambient-outside-calls (tests / factories)
     * @code
     * onyxui::ui_host<Backend> host(metrics);
     * auto guard = host.push_scope();   // RAII token
     * // ui_services<Backend>::themes() etc. work here.
     * auto widget = std::make_unique<my_root>(...);
     * // guard destructs → pop.
     * @endcode
     */
    template<UIBackend Backend>
    class ui_services {
    private:
        // Thread-local context stack - use getter function to avoid template conflicts
        static std::vector<ui_context<Backend>*>& context_stack() {
            static thread_local std::vector<ui_context<Backend>*> stack;
            return stack;
        }

    public:
        // ================================================================
        // Stack Management
        // ================================================================

        /**
         * @brief Push a context onto the stack
         * @param ctx Pointer to context (non-owning)
         *
         * @details
         * Makes the given context the "current" context for this thread.
         * The context must remain alive until pop_context() is called.
         *
         * @note Use ui_host for automatic push/pop management
         */
        static void push_context(ui_context<Backend>* ctx) noexcept {
            assert(ctx != nullptr && "Cannot push null context");
            context_stack().push_back(ctx);
        }

        /**
         * @brief Pop the current context from the stack
         *
         * @details
         * Removes the top context, restoring the previous context (if any).
         * Does nothing if the stack is empty.
         *
         * @note Use ui_host::push_scope() for automatic push/pop management
         */
        static void pop_context() noexcept {
            auto& stack = context_stack();
            if (!stack.empty()) {
                stack.pop_back();
            }
        }

        /**
         * @brief Pop @p expected from the context stack.
         *
         * Fast path: if @p expected is on top, pop it — same as the
         * zero-arg overload. Slow path: if @p expected is deeper in
         * the stack (scope tokens destroyed out of LIFO order),
         * erase it in place so the stack stays consistent with the
         * set of live tokens. No-op if @p expected is not on the
         * stack.
         *
         * This is the pop `ui_host::scope_token` uses, so that
         * out-of-order destruction can't silently pop the wrong
         * host's context.
         */
        static void pop_context(ui_context<Backend>* expected) noexcept {
            if (!expected) return;
            auto& stack = context_stack();
            if (stack.empty()) return;
            if (stack.back() == expected) {
                stack.pop_back();
                return;
            }
            // Out-of-order destruction: find and erase in place.
            auto it = std::find(stack.begin(), stack.end(), expected);
            if (it != stack.end()) {
                stack.erase(it);
            }
        }

        /**
         * @brief Get the current context
         * @return Pointer to current context, or nullptr if stack is empty
         *
         * @details
         * Returns the top of the context stack (most recently pushed context).
         * Returns nullptr if no context has been pushed.
         */
        [[nodiscard]] static ui_context<Backend>* current() noexcept {
            auto& stack = context_stack();
            return stack.empty() ? nullptr : stack.back();
        }

        // ================================================================
        // Service Access (Convenience Methods)
        // ================================================================

        /**
         * @brief Get the layer manager from current context
         * @return Pointer to layer manager, or nullptr if no context
         *
         * @details
         * Returns nullptr if:
         * - No context has been pushed
         * - Context stack is empty
         *
         * Always check for nullptr before using:
         * @code
         * if (auto* layers = ui_services<Backend>::layers()) {
         *     layers->show_popup(...);
         * }
         * @endcode
         */
        [[nodiscard]] static layer_manager<Backend>* layers() noexcept {
            auto* ctx = current();
            return ctx ? &ctx->layers() : nullptr;
        }

        /**
         * @brief Get the input manager from current context
         * @return Pointer to input manager, or nullptr if no context
         *
         * @details
         * Returns nullptr if:
         * - No context has been pushed
         * - Context stack is empty
         *
         * Always check for nullptr before using:
         * @code
         * if (auto* input = ui_services<Backend>::input()) {
         *     input->set_focus(widget);
         *     input->set_capture(widget);
         * }
         * @endcode
         */
        [[nodiscard]] static hierarchical_input_manager<Backend, ui_element<Backend>>* input() noexcept {
            auto* ctx = current();
            return ctx ? &ctx->input() : nullptr;
        }

        /**
         * @brief Get the theme registry from current context
         * @return Pointer to theme registry, or nullptr if no context
         *
         * @details
         * Returns nullptr if:
         * - No context has been pushed
         * - Context stack is empty
         *
         * Always check for nullptr before using:
         * @code
         * if (auto* themes = ui_services<Backend>::themes()) {
         *     if (auto theme = themes->get_theme("Dark")) {
         *         widget->apply_theme(*theme);
         *     }
         * }
         * @endcode
         */
        [[nodiscard]] static theme_registry<Backend>* themes() noexcept {
            auto* ctx = current();
            return ctx ? &ctx->themes() : nullptr;
        }

        /**
         * @brief Get the hotkey manager from current context
         * @return Pointer to hotkey manager, or nullptr if no context
         *
         * @details
         * Returns nullptr if:
         * - No context has been pushed
         * - Context stack is empty
         *
         * Always check for nullptr before using:
         * @code
         * if (auto* hotkeys = ui_services<Backend>::hotkeys()) {
         *     hotkeys->register_action(quit_action);
         * }
         * @endcode
         */
        [[nodiscard]] static hotkey_manager<Backend>* hotkeys() noexcept {
            auto* ctx = current();
            return ctx ? &ctx->hotkeys() : nullptr;
        }

        /**
         * @brief Get the hotkey scheme registry from current context
         * @return Pointer to hotkey scheme registry, or nullptr if no context
         *
         * @details
         * Returns nullptr if:
         * - No context has been pushed
         * - Context stack is empty
         *
         * Always check for nullptr before using:
         * @code
         * if (auto* schemes = ui_services<Backend>::hotkey_schemes()) {
         *     schemes->set_current_scheme("Norton Commander");
         * }
         * @endcode
         */
        [[nodiscard]] static hotkey_scheme_registry* hotkey_schemes() noexcept {
            auto* ctx = current();
            return ctx ? &ctx->hotkey_schemes() : nullptr;
        }

        /**
         * @brief Get the background renderer from current context
         * @return Pointer to background renderer, or nullptr if no context
         *
         * @details
         * Returns nullptr if:
         * - No context has been pushed
         * - Context stack is empty
         *
         * Always check for nullptr before using:
         * @code
         * if (auto* bg = ui_services<Backend>::background()) {
         *     bg->set_mode(background_mode::solid);
         *     bg->set_color({0, 0, 170});
         * }
         * @endcode
         */
        [[nodiscard]] static background_renderer<Backend>* background() noexcept {
            auto* ctx = current();
            return ctx ? &ctx->background() : nullptr;
        }

        /**
         * @brief Get the window manager from current context
         * @return Pointer to window manager, or nullptr if no context
         *
         * @details
         * Returns nullptr if:
         * - No context has been pushed
         * - Context stack is empty
         *
         * Always check for nullptr before using:
         * @code
         * if (auto* mgr = ui_services<Backend>::window_manager()) {
         *     mgr->show_window_list();
         *     auto minimized = mgr->get_minimized_windows();
         * }
         * @endcode
         */
        [[nodiscard]] static window_manager<Backend>* windows() noexcept {
            auto* ctx = current();
            return ctx ? &ctx->windows() : nullptr;
        }

        /**
         * @brief Get the backend metrics from current context
         * @return Pointer to backend metrics, or nullptr if no context
         *
         * @details
         * Backend metrics provide conversion between logical and physical coordinates.
         *
         * Returns nullptr if:
         * - No context has been pushed
         * - Context stack is empty
         *
         * Always check for nullptr before using:
         * @code
         * if (auto* m = ui_services<Backend>::metrics()) {
         *     auto physical_rect = m->snap_rect(logical_bounds);
         *     auto logical_size = m->physical_to_logical_size(physical_size);
         * }
         * @endcode
         */
        [[nodiscard]] static const backend_metrics<Backend>* metrics() noexcept {
            auto* ctx = current();
            return ctx ? &ctx->metrics() : nullptr;
        }

        // ================================================================
        // Introspection / Debugging
        // ================================================================

        /**
         * @brief Get the current context stack depth
         * @return Number of contexts on the stack
         *
         * @details
         * Useful for debugging nested contexts:
         * - depth() == 0: No context active
         * - depth() == 1: Single context
         * - depth() > 1: Nested contexts
         */
        [[nodiscard]] static size_t depth() noexcept {
            return context_stack().size();
        }

        /**
         * @brief Check if any context is active
         * @return true if at least one context is on the stack
         */
        [[nodiscard]] static bool has_context() noexcept {
            return !context_stack().empty();
        }
    };

} // namespace onyxui
