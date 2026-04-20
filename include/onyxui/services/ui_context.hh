/**
 * @file ui_context.hh
 * @brief UI context container with push/pop stack pattern for service management
 * @author Assistant
 * @date 2025-10-20
 *
 * @details
 * This file provides the core context management infrastructure for the onyx_ui framework.
 * It implements a push/pop stack pattern (similar to OpenGL's matrix stack or Dear ImGui's
 * ID stack) for managing UI services with automatic lifetime management via RAII.
 *
 * ## What is a UI Context?
 *
 * A **ui_context** provides access to all framework-level services with a hybrid
 * ownership model designed for both global configuration and per-UI isolation:
 *
 * ### Shared Services (Singleton Pattern):
 * - **theme_registry**: Application-wide theme storage (Dark mode, Light mode, etc.)
 * - **hotkey_manager**: Global keyboard shortcuts (Ctrl+Q quit, F1 help, etc.)
 * - Shared across ALL contexts - register once, available everywhere
 * - Persist for entire application lifetime
 *
 * ### Per-Context Services (Isolated):
 * - **layer_manager**: Overlay layers (popups, menus, tooltips, dialogs)
 * - **focus_manager**: Keyboard focus and Tab navigation
 * - Fresh instance per context - independent UI states
 * - Cleaned up when context is destroyed
 *
 * This hybrid approach allows multiple independent UIs (each with their own layers
 * and focus) while sharing common configuration (themes and hotkeys).
 *
 * ## Design Rationale: Why Push/Pop Stack?
 *
 * ### Problems with Previous Approaches:
 *
 * **1. Singleton Pattern:**
 * ```cpp
 * // ❌ Cannot support multiple independent UIs (game HUD + pause menu)
 * // ❌ Cannot support nested contexts (modal dialogs)
 * // ❌ Difficult to test in parallel
 * ui_services::get_instance().layers()->show_popup(...);
 * ```
 *
 * **2. Manual Registration:**
 * ```cpp
 * // ❌ Lots of boilerplate (5+ lines per test/app)
 * // ❌ Easy to forget cleanup (memory leaks, crashes)
 * // ❌ Error-prone (forget to register → nullptr crashes)
 * hierarchical_focus_manager<Backend, ui_element<Backend>> focus_mgr;
 * ui_services<Backend>::set_focus_manager(&focus_mgr);
 * // ... use services ...
 * ui_services<Backend>::clear();  // Easy to forget!
 * ```
 *
 * ### Benefits of Push/Pop Stack Pattern:
 *
 * ✅ **Zero Boilerplate** - One line: `ui_host<Backend> ctx;`
 * ✅ **Automatic Cleanup** - RAII guarantees cleanup (even with exceptions)
 * ✅ **Multiple Independent UIs** - Game HUD + pause menu + debug overlay
 * ✅ **Nested Contexts** - Modal dialogs over main UI with proper stacking
 * ✅ **Thread Safe** - Each thread has its own context stack (thread_local)
 * ✅ **Cannot Forget** - Compiler enforces proper usage via RAII
 * ✅ **Testable** - Each test gets isolated context automatically
 *
 * ## Hybrid Service Model: Shared vs Per-Context
 *
 * The framework uses a **hybrid ownership model** to balance global configuration
 * with per-UI isolation:
 *
 * ### Why Share Themes and Hotkeys?
 *
 * **Theme Registry (Shared):**
 * - Themes are application-wide visual styles (Dark, Light, High Contrast, etc.)
 * - Should be registered once at startup, available everywhere
 * - Users expect theme changes to persist across dialogs/screens
 * - Example: User selects "Dark mode" - all UIs should respect this choice
 *
 * **Hotkey Manager (Shared):**
 * - Hotkeys are global keyboard shortcuts (Ctrl+Q quit, F1 help, etc.)
 * - Should work consistently across all UI screens
 * - Users expect Ctrl+Q to quit regardless of which dialog is open
 * - Example: Alt+F4 should close application from any UI state
 *
 * ### Why Isolate Layers and Focus?
 *
 * **Layer Manager (Per-Context):**
 * - Each UI needs its own popup/menu stack
 * - Game HUD shouldn't see pause menu's popups (and vice versa)
 * - When context is destroyed, its popups should close
 * - Example: Modal dialog has its own menus that close when dialog closes
 *
 * **Focus Manager (Per-Context):**
 * - Each UI needs its own focus state
 * - Tab navigation should be scoped to current UI
 * - Different UIs can have different widgets focused simultaneously
 * - Example: HUD has one widget focused, pause menu has different widget focused
 *
 * ### Visual Example:
 *
 * ```
 * Application Lifetime
 * ├── Shared: Theme Registry [Dark, Light, High Contrast]  ← Persists
 * ├── Shared: Hotkey Manager [Ctrl+Q quit, F1 help]       ← Persists
 * │
 * ├── Context 1 (Main Menu)
 * │   ├── Layer Manager: [Background, Logo]               ← Isolated
 * │   └── Focus Manager: [Start Button focused]           ← Isolated
 * │
 * ├── Context 2 (Game HUD)
 * │   ├── Layer Manager: [Health bar, Score]              ← Isolated
 * │   └── Focus Manager: [No focus]                       ← Isolated
 * │
 * └── Context 3 (Pause Menu)
 *     ├── Layer Manager: [Menu popup, Options dialog]     ← Isolated
 *     └── Focus Manager: [Resume button focused]          ← Isolated
 * ```
 *
 * ## Core Concepts
 *
 * ### Stack Management
 *
 * Contexts are managed via a thread-local stack in ui_services.
 * `ui_host<B>` owns a context for its lifetime but is **dormant
 * between calls** — it does NOT push itself on the ambient stack
 * from its constructor. Instead, the per-call entry points push
 * the host's context for the duration of the call:
 *
 * - `ui_host::render(...)` pushes on entry, pops on return
 * - `ui_host::handle_event(...)` pushes on entry, pops on return
 * - `ui_host::present(...)` / `present_modal(...)` — same pattern
 *
 * For code that needs ambient services **outside** those calls
 * (widget tree construction in a factory, test fixtures, one-shot
 * utilities), `ui_host::push_scope()` returns a `scope_token` RAII
 * object that keeps the host's context on the stack for its own
 * lifetime. See `ui_host.hh` for both helpers.
 *
 * `ui_services<Backend>::current()` returns the top of the stack
 * (nullptr if none is active). Stack depth reflects nesting —
 * depth 0 = no ambient context, depth 1 = one ui_host's per-call
 * guard or one `scope_token` in scope, depth >1 = nested (e.g. a
 * render call from inside another host's callback).
 *
 * ### Service Access
 *
 * Services are accessed through `ui_services` static methods:
 * ```cpp
 * if (auto* layers = ui_services<Backend>::layers()) {
 *     layers->show_popup(...);  // Uses service from current context
 * }
 * ```
 *
 * All accessors return `nullptr` if no context is active (safe).
 *
 * ## Usage Examples
 *
 * ### Example 1: Basic Application with Shared Services
 * ```cpp
 * int main() {
 *     // Create context - services available automatically
 *     ui_host<conio_backend> ctx;
 *
 *     // Register themes (SHARED - persist for entire application)
 *     ctx.themes().register_theme(dark_theme);
 *     ctx.themes().register_theme(light_theme);
 *
 *     // Register global hotkeys (SHARED - work from any UI state)
 *     auto quit_action = std::make_shared<action<conio_backend>>();
 *     quit_action->set_shortcut('q', key_modifier::ctrl);
 *     quit_action->triggered.connect([]() { exit(0); });
 *     ctx.hotkeys().register_action(quit_action);
 *
 *     // Create UI - automatically uses current context
 *     auto root_widget = create_main_window();
 *     ui_host<conio_backend> ui(std::move(root_widget), renderer);
 *
 *     // Main loop
 *     while (!should_quit) {
 *         ui.display(bounds);
 *         ui.handle_event(get_event());
 *         ui.present();
 *     }
 *
 *     // Context cleaned up (layers/focus destroyed, themes/hotkeys persist)
 *     return 0;
 * }
 * ```
 *
 * ### Example 2: Theme and Hotkey Persistence Across Contexts
 * ```cpp
 * // Demonstrate that themes and hotkeys persist across contexts
 * void demonstrate_persistence() {
 *     // First context - register themes and hotkeys
 *     {
 *         ui_host<Backend> ctx1;
 *
 *         // Register themes (SHARED - persist beyond this scope)
 *         ctx1.themes().register_theme(dark_theme);
 *         std::cout << "ctx1: Registered Dark theme" << std::endl;
 *
 *         // Register hotkey (SHARED - persist beyond this scope)
 *         auto quit = std::make_shared<action<Backend>>();
 *         quit->set_shortcut('q', key_modifier::ctrl);
 *         ctx1.hotkeys().register_action(quit);
 *         std::cout << "ctx1: Registered Ctrl+Q hotkey" << std::endl;
 *
 *         // ctx1's layers and focus will be destroyed when scope ends
 *     }  // ← ctx1 destroyed, but themes/hotkeys PERSIST!
 *
 *     // Second context - themes and hotkeys still available!
 *     {
 *         ui_host<Backend> ctx2;
 *
 *         // ✅ Theme registered in ctx1 is still available in ctx2
 *         auto* theme = ctx2.themes().get_theme("Dark");
 *         if (theme) {
 *             std::cout << "ctx2: Dark theme found! (persisted from ctx1)" << std::endl;
 *         }
 *
 *         // ✅ Hotkey registered in ctx1 is still active in ctx2
 *         // Ctrl+Q will work even though ctx1 is gone!
 *         std::cout << "ctx2: Ctrl+Q hotkey active! (persisted from ctx1)" << std::endl;
 *
 *         // But ctx2 has fresh layers and focus (isolated from ctx1)
 *         ctx2.layers().show_popup(menu);  // ctx2's own popup stack
 *         ctx2.focus().set_focus(button);  // ctx2's own focus state
 *     }
 * }
 * ```
 *
 * ### Example 3: Game Engine with Multiple Independent UIs
 * ```cpp
 * class Game {
 *     ui_host<Backend> m_hud_context;    // HUD's layers/focus
 *     ui_host<Backend> m_hud;
 *
 *     ui_host<Backend> m_menu_context;   // Menu's layers/focus
 *     ui_host<Backend> m_menu;
 *
 *     Game() {
 *         // Register themes once (SHARED across HUD and menu)
 *         m_hud_context.themes().register_theme(game_theme);
 *
 *         // Register global hotkeys once (SHARED across HUD and menu)
 *         auto pause_action = std::make_shared<action<Backend>>();
 *         pause_action->set_shortcut(TB_KEY_ESC);
 *         pause_action->triggered.connect([this]() { toggle_pause(); });
 *         m_hud_context.hotkeys().register_action(pause_action);
 *     }
 *
 *     void render() {
 *         render_world();
 *
 *         // Render HUD (uses m_hud_context for layers/focus)
 *         // But shares themes/hotkeys with menu!
 *         m_hud.display();
 *         m_hud.present();
 *
 *         // Render pause menu if open (uses m_menu_context for layers/focus)
 *         // But shares themes/hotkeys with HUD!
 *         if (paused) {
 *             m_menu.display();  // Separate popups from HUD
 *             m_menu.present();  // Separate focus from HUD
 *         }
 *     }
 * };
 * ```
 *
 * ### Example 4: Nested Modal Dialog
 * ```cpp
 * // Main UI context
 * ui_host<Backend> main_ctx;
 * ui_host<Backend> main_ui(...);
 *
 * // Main event loop
 * while (!quit) {
 *     main_ui.display();
 *
 *     if (show_dialog) {
 *         // Push nested context for dialog
 *         ui_host<Backend> dialog_ctx;
 *         ui_host<Backend> dialog_ui(...);
 *
 *         // Dialog event loop (blocks main loop)
 *         while (!dialog_closed) {
 *             dialog_ui.display();
 *             dialog_ui.handle_event(get_event());
 *             dialog_ui.present();
 *         }
 *
 *         // dialog_ctx automatically popped here
 *     }
 *
 *     main_ui.present();
 * }
 * // main_ctx automatically popped here
 * ```
 *
 * ### Example 5: Multi-threaded Rendering
 * ```cpp
 * void render_thread_1() {
 *     ui_host<Backend> ctx_1;  // Thread 1's context
 *     ui_host<Backend> ui_1(...);
 *
 *     while (running) {
 *         ui_1.display();
 *         ui_1.present();
 *     }
 * }
 *
 * void render_thread_2() {
 *     ui_host<Backend> ctx_2;  // Thread 2's context
 *     ui_host<Backend> ui_2(...);
 *
 *     while (running) {
 *         ui_2.display();
 *         ui_2.present();
 *     }
 * }
 *
 * // Each thread maintains independent context stack (thread_local)
 * std::thread t1(render_thread_1);
 * std::thread t2(render_thread_2);
 * ```
 *
 * ### Example 6: Unit Testing
 * ```cpp
 * TEST_CASE("Menu navigation") {
 *     // Setup - one line!
 *     ui_host<Backend> ctx;
 *
 *     // Test code - services available automatically
 *     auto menu = create_menu();
 *     menu->open();
 *
 *     CHECK(ctx.layers().layer_count() == 1);
 *
 *     // Cleanup automatic - even if CHECK fails!
 * }
 * ```
 *
 * ### Example 7: Exception Safety
 * ```cpp
 * void risky_operation() {
 *     ui_host<Backend> ctx;
 *     ui_host<Backend> ui(...);
 *
 *     // Even if this throws...
 *     ui.display();
 *     might_throw();
 *     ui.present();
 *
 *     // ...context is still cleaned up properly (RAII)
 * }
 * ```
 *
 * ## Thread Safety
 *
 * The context stack is **thread_local**, meaning:
 * - Each thread maintains its own independent stack
 * - No synchronization required between threads
 * - Safe for multi-threaded rendering with separate contexts
 *
 * **Thread-Safe Operations:**
 * - Creating contexts in different threads ✅
 * - Accessing services within same thread ✅
 * - Nested contexts within same thread ✅
 *
 * **NOT Thread-Safe:**
 * - Sharing contexts between threads ❌ (use separate contexts per thread)
 * - Accessing services from different thread ❌ (each thread needs own context)
 *
 * ## Best Practices
 *
 * ### ✅ DO:
 * - Use `ui_host` for automatic lifetime management
 * - Create one context per UI instance
 * - Check for nullptr when accessing services: `if (auto* svc = ui_services<>::layers())`
 * - Use nested contexts for modal dialogs and popups
 * - Create separate contexts for independent UIs (HUD, menu, debug overlay)
 *
 * ### ❌ DON'T:
 * - Create ui_context directly (use ui_host instead)
 * - Manually call push_context/pop_context (let RAII handle it)
 * - Share contexts between threads (each thread needs its own)
 * - Access services without nullptr check (no context = nullptr)
 * - Forget to create context before creating widgets (services won't be available)
 *
 * ## Common Pitfalls and Solutions
 *
 * ### Pitfall 1: Accessing Services Without Context
 * ```cpp
 * // ❌ WRONG: No context created
 * auto* layers = ui_services<Backend>::layers();  // Returns nullptr!
 * layers->show_popup(...);  // CRASH!
 *
 * // ✅ CORRECT: Create context first
 * ui_host<Backend> ctx;
 * auto* layers = ui_services<Backend>::layers();
 * if (layers) {  // Always check!
 *     layers->show_popup(...);
 * }
 * ```
 *
 * ### Pitfall 2: Context Lifetime Too Short
 * ```cpp
 * // ❌ WRONG: Context destroyed too early
 * ui_host<Backend> create_ui() {
 *     ui_host<Backend> ctx;
 *     return ui_host<Backend>(...);  // Context destroyed when function returns!
 * }
 *
 * // ✅ CORRECT: Keep context alive as long as UI exists
 * struct App {
 *     ui_host<Backend> ctx;  // Member variable
 *     ui_host<Backend> ui;
 * };
 * ```
 *
 * ### Pitfall 3: Wrong Context Order
 * ```cpp
 * // ❌ WRONG: ui_host created before context
 * ui_host<Backend> ui(...);
 * ui_host<Backend> ctx;  // Too late!
 *
 * // ✅ CORRECT: Context before UI
 * ui_host<Backend> ctx;
 * ui_host<Backend> ui(...);  // Services available
 * ```
 *
 * ## Performance Considerations
 *
 * - Context creation: O(1) - just constructs three service objects
 * - Context push/pop: O(1) - vector push_back/pop_back
 * - Service access: O(1) - returns reference from top of stack
 * - Memory overhead: ~3 pointers per context on stack
 * - Thread-local storage: Zero contention (no locks required)
 *
 * Creating contexts is lightweight - don't hesitate to use nested contexts for clarity.
 *
 * ## Debugging Tips
 *
 * ### Check Context Stack Depth
 * ```cpp
 * // How many contexts are active?
 * size_t depth = ui_services<Backend>::depth();
 * // 0 = no context, 1 = single UI, >1 = nested
 * ```
 *
 * ### Verify Context is Active
 * ```cpp
 * if (!ui_services<Backend>::has_context()) {
 *     std::cerr << "ERROR: No UI context active!" << std::endl;
 * }
 * ```
 *
 * ### Get Current Context
 * ```cpp
 * auto* ctx = ui_services<Backend>::current();
 * if (ctx) {
 *     std::cerr << "Layers: " << ctx->layers().layer_count() << std::endl;
 * }
 * ```
 *
 * ## Migration from Old Pattern
 *
 * ### Before (Manual Registration):
 * ```cpp
 * // Old pattern - lots of boilerplate
 * hierarchical_focus_manager<Backend, ui_element<Backend>> focus_mgr;
 * layer_manager<Backend> layer_mgr;
 * theme_registry<Backend> themes;
 *
 * ui_services<Backend>::set_focus_manager(&focus_mgr);
 * ui_services<Backend>::set_layer_manager(&layer_mgr);
 * ui_services<Backend>::set_theme_registry(&themes);
 *
 * // Use services...
 *
 * ui_services<Backend>::clear();  // Easy to forget!
 * ```
 *
 * ### After (Push/Pop Pattern):
 * ```cpp
 * // New pattern - one line!
 * ui_host<Backend> ctx;
 *
 * // Use services (automatically available)...
 *
 * // Cleanup automatic (RAII)
 * ```
 *
 * ### Migration Checklist:
 * 1. Replace manual service creation with `ui_host<Backend> ctx;`
 * 2. Remove all `ui_services::set_*()` calls
 * 3. Remove all `ui_services::clear()` calls
 * 4. Change `&focus_mgr` to `ctx.focus()` for direct access
 * 5. Change `layer_mgr.foo()` to `ctx.layers().foo()`
 * 6. Change `themes.bar()` to `ctx.themes().bar()`
 *
 * ## Integration with Existing Code
 *
 * The context system integrates seamlessly with existing widgets and services:
 * - **Widgets**: Automatically use services from current context via `ui_services<Backend>::*`
 * - **menu_bar**: Uses layer_manager from current context for popup menus
 * - **menu**: Uses focus_manager from current context for keyboard navigation
 * - **ui_host**: Renders overlay layers from current context automatically
 * - **themeable**: Applies themes from theme_registry in current context
 *
 * No changes required to widget code - services are accessed via ui_services static methods.
 *
 * ## See Also
 * - ui_services.hh - Service locator with push/pop stack implementation
 * - ui_host.hh - Main UI handle that uses services from current context
 * - layer_manager.hh - Overlay layer management service
 * - focus_manager.hh - Focus navigation service
 * - theme_registry.hh - Theme storage and lookup service
 */

#pragma once

#include <onyxui/concepts/backend.hh>
#include <onyxui/core/backend_metrics.hh>
#include <onyxui/services/layer_manager.hh>
#include <onyxui/services/input_manager.hh>
#include <onyxui/theming/theme_registry.hh>
#include <onyxui/hotkeys/hotkey_manager.hh>
#include <onyxui/hotkeys/hotkey_scheme_registry.hh>
#include <onyxui/hotkeys/builtin_hotkey_schemes.hh>
#include <onyxui/services/background_renderer.hh>
#include <onyxui/services/window_manager.hh>

namespace onyxui {

    // Forward declarations
    template<UIBackend Backend> class ui_element;
    template<UIBackend Backend> class ui_services;

    /**
     * @class ui_context
     * @brief Hybrid service provider with shared global config and per-context UI state
     *
     * @tparam Backend The UI backend type satisfying UIBackend concept
     *
     * @details
     * **ui_context** provides access to all framework-level services using a hybrid
     * ownership model that balances global configuration with per-UI isolation.
     *
     * ### Per-Context Services (Owned by each context):
     * - **layer_manager<Backend>**: Manages overlay layers (menus, dialogs, tooltips)
     * - **hierarchical_focus_manager<Backend, ui_element<Backend>>**: Manages keyboard focus
     * - Each context gets fresh instances - isolated UI state
     * - Destroyed when context is destroyed
     *
     * ### Shared Services (Global singleton):
     * - **theme_registry<Backend>**: Application-wide theme storage
     * - **hotkey_manager<Backend>**: Global keyboard shortcuts
     * - Shared across ALL contexts via static shared_ptr
     * - Persist for entire application lifetime
     * - Lazily initialized on first context creation
     *
     * ### Design Pattern:
     * This class implements a **hybrid pattern** combining:
     * - **Aggregate Root** for per-context services (owns layer_manager, focus_manager)
     * - **Singleton** for shared services (static theme_registry, hotkey_manager)
     *
     * ### Lifetime and Ownership:
     * - Per-context services: Owned by each ui_context instance
     * - Shared services: Owned by static shared_ptr (singleton)
     * - Contexts are non-copyable (unique ownership of per-context services)
     * - Contexts are movable (for storage in containers)
     * - Per-context services destroyed with context
     * - Shared services persist until program exit
     *
     * ### Stack Management:
     * Contexts are pushed onto a thread-local stack managed by ui_services.
     * Use ui_host for automatic RAII-based push/pop.
     *
     * ### Thread Safety:
     * - Each thread has its own context stack (thread_local)
     * - Contexts are NOT thread-safe themselves (single-threaded access)
     * - Create separate contexts per thread for multi-threaded UIs
     *
     * ### Usage Pattern:
     * **Don't create ui_context directly** - use ui_host instead:
     * ```cpp
     * // ✅ Recommended
     * ui_host<Backend> ctx;
     * ctx.layers().show_popup(...);
     *
     * // ❌ Not recommended (manual stack management)
     * ui_context<Backend> ctx;
     * ui_services<Backend>::push_context(&ctx);
     * // ... use services ...
     * ui_services<Backend>::pop_context();
     * ```
     *
     * ### Access Patterns:
     * 1. **Direct access** (via ui_host):
     *    ```cpp
     *    ui_host<Backend> ctx;
     *    ctx.layers().show_popup(...);
     *    ```
     *
     * 2. **Service locator** (from widgets/internal code):
     *    ```cpp
     *    if (auto* layers = ui_services<Backend>::layers()) {
     *        layers->show_popup(...);
     *    }
     *    ```
     *
     * ### Member Access:
     * - `layers()` - Get reference to layer_manager
     * - `focus()` - Get reference to focus_manager
     * - `themes()` - Get reference to theme_registry
     *
     * All accessors return references (never nullptr) since services are
     * always constructed. The nullptr checks are only needed when accessing
     * through ui_services (which returns nullptr if no context active).
     *
     * @see ui_host for RAII wrapper
     * @see ui_services for service locator access
     */
    template<UIBackend Backend>
    class ui_context {
    public:
        using layer_manager_type = layer_manager<Backend>;
        using input_manager_type = hierarchical_input_manager<Backend, ui_element<Backend>>;
        using theme_registry_type = theme_registry<Backend>;
        using theme_type = ui_theme<Backend>;
        using hotkey_manager_type = hotkey_manager<Backend>;
        using hotkey_scheme_registry_type = hotkey_scheme_registry;
        using background_renderer_type = background_renderer<Backend>;
        using window_manager_type = window_manager<Backend>;
        using metrics_type = backend_metrics<Backend>;

        /**
         * @brief Construct a new UI context with backend metrics
         *
         * @param metrics Backend metrics for logical-to-physical coordinate conversion
         *
         * @details
         * Per-context services (layer_manager, focus_manager) are created fresh.
         * Shared services (theme_registry, hotkey_manager) are initialized once
         * and shared across all contexts.
         *
         * **Backend metrics are REQUIRED:**
         * Use make_terminal_metrics<Backend>() for terminal/conio backends.
         * Use make_gui_metrics<Backend>() for SDL/pixel-based backends.
         *
         * **Automatic theme/background synchronization:**
         * Connects background_renderer to theme_changed signal, ensuring background
         * color automatically updates when themes are switched.
         */
        explicit ui_context(metrics_type metrics)
            : m_metrics(metrics)
            , m_theme_connection(get_theme_signal(), [this](const theme_type* theme) {
                m_background_renderer.on_theme_changed(theme);
            })
        {
            ensure_shared_services_initialized();

            // Wire up input_manager to layer_manager for capture handling
            m_layer_manager.set_input_manager(&m_input_manager);

            // NOTE: activate_focused is NOT registered as a global handler
            // It's dispatched to the focused widget via handle_semantic_action()
            // This allows widgets (buttons, etc.) to handle activation properly
        }

        /**
         * @brief Destructor
         */
        ~ui_context() = default;

        // Non-copyable (owns resources)
        ui_context(const ui_context&) = delete;
        ui_context& operator=(const ui_context&) = delete;

        // Movable
        ui_context(ui_context&&) noexcept = default;
        ui_context& operator=(ui_context&&) noexcept = default;

        /**
         * @brief Get the layer manager
         * @return Reference to layer manager
         */
        [[nodiscard]] layer_manager_type& layers() noexcept {
            return m_layer_manager;
        }

        /**
         * @brief Get the layer manager (const)
         * @return Const reference to layer manager
         */
        [[nodiscard]] const layer_manager_type& layers() const noexcept {
            return m_layer_manager;
        }

        /**
         * @brief Get the input manager (focus, mouse capture, hover)
         * @return Reference to input manager
         */
        [[nodiscard]] input_manager_type& input() noexcept {
            return m_input_manager;
        }

        /**
         * @brief Get the input manager (const)
         * @return Const reference to input manager
         */
        [[nodiscard]] const input_manager_type& input() const noexcept {
            return m_input_manager;
        }

        /**
         * @brief Get the theme registry (shared across all contexts)
         * @return Reference to shared theme registry
         *
         * @details
         * The theme registry is shared across all UI contexts.
         * Themes registered in one context are available in all contexts.
         */
        [[nodiscard]] theme_registry_type& themes() noexcept {
            ensure_shared_services_initialized();
            return *s_shared_themes;
        }

        /**
         * @brief Get the theme registry (const, shared across all contexts)
         * @return Const reference to shared theme registry
         */
        [[nodiscard]] const theme_registry_type& themes() const noexcept {
            ensure_shared_services_initialized();
            return *s_shared_themes;
        }

        /**
         * @brief Get the hotkey manager (shared across all contexts)
         * @return Reference to shared hotkey manager
         *
         * @details
         * The hotkey manager is shared across all UI contexts.
         * Hotkeys registered in one context are active in all contexts.
         */
        [[nodiscard]] hotkey_manager_type& hotkeys() noexcept {
            ensure_shared_services_initialized();
            return *s_shared_hotkeys;
        }

        /**
         * @brief Get the hotkey manager (const, shared across all contexts)
         * @return Const reference to shared hotkey manager
         */
        [[nodiscard]] const hotkey_manager_type& hotkeys() const noexcept {
            ensure_shared_services_initialized();
            return *s_shared_hotkeys;
        }

        /**
         * @brief Get the hotkey scheme registry (shared across all contexts)
         * @return Reference to shared hotkey scheme registry
         *
         * @details
         * The hotkey scheme registry is shared across all UI contexts.
         * Schemes registered in one context are available in all contexts.
         * Built-in schemes (Windows, Norton Commander) are auto-registered
         * on first context creation.
         */
        [[nodiscard]] hotkey_scheme_registry_type& hotkey_schemes() noexcept {
            ensure_shared_services_initialized();
            return *s_shared_hotkey_schemes;
        }

        /**
         * @brief Get the hotkey scheme registry (const, shared across all contexts)
         * @return Const reference to shared hotkey scheme registry
         */
        [[nodiscard]] const hotkey_scheme_registry_type& hotkey_schemes() const noexcept {
            ensure_shared_services_initialized();
            return *s_shared_hotkey_schemes;
        }

        /**
         * @brief Get the background renderer
         * @return Reference to background renderer
         */
        [[nodiscard]] background_renderer_type& background() noexcept {
            return m_background_renderer;
        }

        /**
         * @brief Get the background renderer (const)
         * @return Const reference to background renderer
         */
        [[nodiscard]] const background_renderer_type& background() const noexcept {
            return m_background_renderer;
        }

        /**
         * @brief Get the window manager
         * @return Reference to window manager
         */
        [[nodiscard]] window_manager_type& windows() noexcept {
            return m_window_manager;
        }

        /**
         * @brief Get the window manager (const)
         * @return Const reference to window manager
         */
        [[nodiscard]] const window_manager_type& windows() const noexcept {
            return m_window_manager;
        }

        /**
         * @brief Get the backend metrics
         * @return Const reference to backend metrics
         *
         * @details
         * Backend metrics provide conversion between logical and physical coordinates.
         * Use snap_rect(), snap_size() etc. to convert logical bounds to physical
         * coordinates for rendering.
         */
        [[nodiscard]] const metrics_type& metrics() const noexcept {
            return m_metrics;
        }

    private:
        // Backend metrics (required - determines coordinate scaling)
        metrics_type m_metrics;
        // Per-context services (isolated per UI instance)
        layer_manager_type m_layer_manager;
        input_manager_type m_input_manager;
        background_renderer_type m_background_renderer;
        window_manager_type m_window_manager;
        scoped_connection m_theme_connection;  ///< Auto-disconnects on destruction

        // Shared services (singleton pattern - shared across all contexts)
        static inline std::shared_ptr<theme_registry_type> s_shared_themes;
        static inline std::shared_ptr<hotkey_manager_type> s_shared_hotkeys;
        static inline std::shared_ptr<hotkey_scheme_registry_type> s_shared_hotkey_schemes;

        /**
         * @brief Get theme_changed signal reference (for scoped_connection initialization)
         * @return Reference to the theme_changed signal
         */
        static auto& get_theme_signal() {
            ensure_shared_services_initialized();
            return s_shared_themes->theme_changed;
        }

        /**
         * @brief Initialize shared services on first access
         *
         * @details
         * Shared services are initialized lazily on first context creation.
         * Once created, they persist across all contexts until program exit.
         *
         * On first initialization, this also registers backend-provided themes
         * by calling Backend::register_themes(). The first theme registered is
         * considered the default theme.
         */
        static void ensure_shared_services_initialized() {
            if (!s_shared_themes) {
                s_shared_themes = std::make_shared<theme_registry_type>();

                // Auto-register backend-provided themes (first is default)
                // Only call if Backend provides register_themes() method
                if constexpr (requires { Backend::register_themes(*s_shared_themes); }) {
                    Backend::register_themes(*s_shared_themes);
                }
            }
            if (!s_shared_hotkey_schemes) {
                s_shared_hotkey_schemes = std::make_shared<hotkey_scheme_registry_type>();

                // Auto-register built-in hotkey schemes (first is default)
                s_shared_hotkey_schemes->register_scheme(builtin_hotkey_schemes::windows());
                s_shared_hotkey_schemes->register_scheme(builtin_hotkey_schemes::norton_commander());
            }
            if (!s_shared_hotkeys) {
                // Pass scheme registry to hotkey_manager for semantic action support
                s_shared_hotkeys = std::make_shared<hotkey_manager_type>(s_shared_hotkey_schemes.get());
            }
        }

        friend class ui_services<Backend>;
    };

} // namespace onyxui
