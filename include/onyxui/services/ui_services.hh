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
 * Uses a stack-based approach with thread-local storage:
 * 1. **Zero Boilerplate**: Just create scoped_ui_context
 * 2. **Multiple UIs**: Each context is independent
 * 3. **Nested Contexts**: Modal dialogs, popups, etc.
 * 4. **Thread Safe**: Each thread has its own stack
 * 5. **RAII Cleanup**: Automatic push/pop via scoped_ui_context
 *
 * ## Thread Safety
 *
 * Context stacks are thread_local, so each thread maintains its own stack.
 * This allows multi-threaded rendering with independent UI contexts per thread.
 *
 * ## Usage Example
 *
 * @code
 * // Single UI context (most common)
 * int main() {
 *     scoped_ui_context<Backend> ctx;
 *
 *     // Register themes
 *     ctx.themes().register_theme(my_theme);
 *
 *     // Create UI
 *     auto ui = ui_handle<Backend>(...);
 *     ui.display();
 *
 *     // Context auto-cleaned up
 * }
 *
 * // Multiple contexts
 * scoped_ui_context<Backend> hud_ctx;
 * auto hud = create_hud();
 *
 * {
 *     scoped_ui_context<Backend> menu_ctx;
 *     auto menu = create_menu();
 *     // Each UI uses its own context
 * }
 * @endcode
 */

#pragma once

#define ONYXUI_UI_SERVICES_HH_INCLUDED

#include <onyxui/concepts/backend.hh>
#include <onyxui/services/ui_context.hh>
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
     * ## Best Practice
     *
     * Use scoped_ui_context for RAII management:
     *
     * @code
     * {
     *     scoped_ui_context<Backend> ctx;
     *     // Use ui services...
     * }  // Context automatically popped
     * @endcode
     *
     * @example Multiple Independent UIs
     * @code
     * // Game with HUD + pause menu
     * scoped_ui_context<Backend> hud_ctx;
     * ui_handle<Backend> hud(...);
     *
     * // Later, show pause menu
     * {
     *     scoped_ui_context<Backend> menu_ctx;
     *     ui_handle<Backend> menu(...);
     *     // menu uses menu_ctx, hud uses hud_ctx
     * }
     * @endcode
     *
     * @example Nested Modal Dialog
     * @code
     * // Main UI active
     * scoped_ui_context<Backend> main_ctx;
     * ui_handle<Backend> main_ui(...);
     *
     * // Show modal dialog
     * {
     *     scoped_ui_context<Backend> dialog_ctx;
     *     ui_handle<Backend> dialog(...);
     *     // Dialog loop...
     * }
     * // Back to main UI
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
         * @note Use scoped_ui_context for automatic push/pop management
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
         * @note Use scoped_ui_context for automatic push/pop management
         */
        static void pop_context() noexcept {
            auto& stack = context_stack();
            if (!stack.empty()) {
                stack.pop_back();
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

    // ================================================================
    // scoped_ui_context Implementation
    // ================================================================

    template<UIBackend Backend>
    scoped_ui_context<Backend>::scoped_ui_context() {
        ui_services<Backend>::push_context(&m_context);
    }

    template<UIBackend Backend>
    scoped_ui_context<Backend>::~scoped_ui_context() {
        ui_services<Backend>::pop_context();
    }

} // namespace onyxui
