/**
 * @file conio_backend.hh
 * @brief Console I/O backend implementation using termbox2
 * @author igor
 * @date 16/10/2025
 *
 * @details
 * Provides a text-based UI backend for terminal applications.
 * Uses termbox2 library for cross-platform console rendering.
 *
 * This backend supports two usage modes:
 *
 * ## Standalone App Mode
 * Use run_app() for a complete application with event loop:
 * @code
 * int main() {
 *     return conio_backend::run_app<MyWidget>();
 * }
 * @endcode
 *
 * ## Custom Integration Mode
 * Use init/shutdown/process_event for custom event loops:
 * @code
 * conio_backend::init();
 * while (running) {
 *     tb_event ev;
 *     if (conio_poll_event(&ev) == TB_OK) {
 *         if (auto ui_ev = conio_backend::process_event(ev)) {
 *             ui.handle_event(*ui_ev);
 *         }
 *     }
 *     ui.display();
 * }
 * conio_backend::shutdown();
 * @endcode
 */

#pragma once

#include <onyxui/concepts/backend.hh>
#include <onyxui/conio/geometry.hh>
#include <onyxui/conio/colors.hh>
#include <onyxui/conio/conio_renderer.hh>
#include <onyxui/conio/conio_events.hh>
#include <onyxui/events/ui_event.hh>
#include <onyxui/theming/theme_registry.hh>
#include <onyxui/services/ui_context.hh>
#include <onyxui/ui_handle.hh>
#include <onyxui/backends/conio/onyxui_conio_export.h>
#include <optional>
#include <cctype>
#include <functional>
#include <memory>

namespace onyxui::conio {
    // ======================================================================
    // Backend Definition
    // ======================================================================

    /**
     * @struct conio_backend
     * @brief Console I/O backend for terminal-based UIs using termbox2
     *
     * This backend provides all required types for the UIBackend concept:
     * - Geometric types: rect, size, point
     * - Event types: termbox2 events
     * - Renderer: conio_renderer using vram
     *
     * @example Usage
     * @code
     * using element = ui_element<conio_backend>;
     * auto root = std::make_unique<element>();
     *
     * // Renderer owns vram internally (which handles termbox2 init/shutdown)
     * conio_renderer renderer;
     *
     * root->measure(80, 24);
     * root->arrange(conio_backend::rect{0, 0, 80, 24});
     * root->render(&renderer);
     * @endcode
     */
    struct conio_backend {
        // Required geometric types
        using rect_type = rect;
        using size_type = size;
        using point_type = point;

        // Required event types (from termbox2)
        using event_type = tb_event;
        using keyboard_event_type = tb_event;
        using mouse_button_event_type = tb_event;
        using mouse_motion_event_type = tb_event;
        using mouse_wheel_event_type = tb_event;
        using text_input_event_type = tb_event;
        using window_event_type = tb_event;

        // Required rendering types
        using color_type = color;
        using renderer_type = conio_renderer;

        // Backend identification
        static constexpr const char* name() { return "Conio"; }

        // Terminal backends don't have continuous mouse tracking
        // (no mouse move events, only click events)
        static constexpr bool has_mouse_tracking = false;

        /**
         * @brief Convert termbox2 event to framework-level ui_event
         * @param native Termbox2 event from tb_poll_event()
         * @return Framework event, or nullopt if unknown event type
         *
         * @details
         * Performs full normalization of platform-specific quirks:
         * - **Enter/Tab**: Removes spurious Ctrl modifier (terminal encoding)
         * - **Uppercase letters**: Converts to lowercase with shift=true
         * - **Mouse buttons**: Maps termbox2 button codes to framework enums
         * - **Resize**: Queries current terminal dimensions
         *
         * @example Keyboard Event Conversion
         * @code
         * tb_event native;
         * tb_poll_event(&native);
         *
         * if (auto evt = conio_backend::create_event(native)) {
         *     if (auto* kbd = std::get_if<keyboard_event>(&evt.value())) {
         *         // Handle keyboard event
         *     }
         * }
         * @endcode
         */
        [[nodiscard]] static std::optional<onyxui::ui_event> create_event(const tb_event& native) noexcept {
            using namespace onyxui;

            // ===== Keyboard Events =====
            if (native.type == TB_EVENT_KEY) {
                keyboard_event kbd{};

                // Map termbox keys directly to key_code
                if (native.key >= TB_KEY_F12 && native.key <= TB_KEY_F1) {
                    // Function keys: F1-F12
                    const int f_num = (TB_KEY_F1 - native.key) + 1;
                    kbd.key = function_key_from_number(f_num);
                } else if (native.key == TB_KEY_ARROW_UP) {
                    kbd.key = key_code::arrow_up;
                } else if (native.key == TB_KEY_ARROW_DOWN) {
                    kbd.key = key_code::arrow_down;
                } else if (native.key == TB_KEY_ARROW_LEFT) {
                    kbd.key = key_code::arrow_left;
                } else if (native.key == TB_KEY_ARROW_RIGHT) {
                    kbd.key = key_code::arrow_right;
                } else if (native.key == TB_KEY_HOME) {
                    kbd.key = key_code::home;
                } else if (native.key == TB_KEY_END) {
                    kbd.key = key_code::end;
                } else if (native.key == TB_KEY_PGUP) {
                    kbd.key = key_code::page_up;
                } else if (native.key == TB_KEY_PGDN) {
                    kbd.key = key_code::page_down;
                } else if (native.key == TB_KEY_INSERT) {
                    kbd.key = key_code::insert;
                } else if (native.key == TB_KEY_DELETE) {
                    kbd.key = key_code::delete_key;
                } else if (native.key == TB_KEY_BACKSPACE || native.key == TB_KEY_BACKSPACE2) {
                    kbd.key = key_code::backspace;
                } else if (native.key == TB_KEY_ENTER) {
                    kbd.key = key_code::enter;
                } else if (native.key == TB_KEY_TAB) {
                    kbd.key = key_code::tab;
                } else if (native.key == TB_KEY_ESC) {
                    kbd.key = key_code::escape;
                } else if (native.key == TB_KEY_SPACE || native.ch == ' ') {
                    kbd.key = key_code::space;
                } else if (native.ch != 0) {
                    // Character input - normalize uppercase to lowercase
                    if (native.ch >= 'A' && native.ch <= 'Z') {
                        kbd.key = static_cast<key_code>(native.ch - 'A' + 'a');
                    } else if (native.ch >= 'a' && native.ch <= 'z') {
                        kbd.key = static_cast<key_code>(native.ch);
                    } else if (native.ch >= 32 && native.ch <= 126) {
                        // ASCII printable characters
                        kbd.key = static_cast<key_code>(native.ch);
                    } else {
                        // Non-ASCII character (Unicode, extended ASCII, etc.)
                        //
                        // LIMITATION: The current key_code enum only supports ASCII range (0-127).
                        // Unicode characters (accented letters, CJK, emoji, etc.) cannot be
                        // represented as key_code values.
                        //
                        // TODO: For proper Unicode input support, add a text_input_event type
                        // to ui_event that carries UTF-8 text, separate from keyboard_event
                        // which is meant for key identification (hotkeys, navigation).
                        //
                        // For now, non-ASCII characters are dropped.
                        return std::nullopt;
                    }
                } else {
                    // Unknown key
                    return std::nullopt;
                }

                // Build modifiers (terminal reality: Enter=Ctrl+M, Tab=Ctrl+I, Backspace=Ctrl+H)
                //
                // Keys that have inherent Ctrl encoding (ASCII control characters):
                // - Tab = Ctrl+I (0x09) - suppress spurious Ctrl, but allow Shift for Shift+Tab
                // - Enter = Ctrl+M (0x0D) - suppress spurious Ctrl
                // - Backspace = Ctrl+H (0x08) - suppress spurious Ctrl
                //
                // Note: Ctrl+Tab may not be reliably detectable in many terminals since
                // Tab is already a Ctrl sequence. Consider using Alt+Tab or other combos
                // for tab_widget navigation if Ctrl+Tab doesn't work in your terminal.
                const bool has_inherent_ctrl = (native.key == TB_KEY_ENTER ||
                                                native.key == TB_KEY_TAB ||
                                                native.key == TB_KEY_BACKSPACE ||
                                                native.key == TB_KEY_BACKSPACE2);

                key_modifier mods = key_modifier::none;

                // Suppress Ctrl only for keys that have inherent Ctrl encoding
                if (!has_inherent_ctrl && (native.mod & TB_MOD_CTRL) != 0) {
                    mods |= key_modifier::ctrl;
                }

                // Alt is always passed through (no spurious encoding issues)
                if ((native.mod & TB_MOD_ALT) != 0) {
                    mods |= key_modifier::alt;
                }

                // Handle Shift:
                // - For uppercase letters: shift=true (already normalized to lowercase above)
                // - For other keys: pass through Shift modifier (including Shift+Tab!)
                if (native.ch >= 'A' && native.ch <= 'Z') {
                    mods |= key_modifier::shift;
                } else if ((native.mod & TB_MOD_SHIFT) != 0) {
                    mods |= key_modifier::shift;
                }

                kbd.modifiers = mods;
                kbd.pressed = true;  // termbox2 doesn't distinguish press/release

                return ui_event{kbd};
            }

            // ===== Mouse Events =====
            if (native.type == TB_EVENT_MOUSE) {
                mouse_event mouse{};
                // Terminal coordinates are cell-based; use cell centers to avoid half-pixel drift
                // (centering fixes hit-testing when layouts land on .5 logical positions)
                mouse.x = logical_unit(static_cast<double>(native.x) + 0.5);
                mouse.y = logical_unit(static_cast<double>(native.y) + 0.5);

                // Convert button/action
                if (native.key == TB_KEY_MOUSE_LEFT) {
                    mouse.btn = mouse_event::button::left;
                    mouse.act = mouse_event::action::press;
                } else if (native.key == TB_KEY_MOUSE_RIGHT) {
                    mouse.btn = mouse_event::button::right;
                    mouse.act = mouse_event::action::press;
                } else if (native.key == TB_KEY_MOUSE_MIDDLE) {
                    mouse.btn = mouse_event::button::middle;
                    mouse.act = mouse_event::action::press;
                } else if (native.key == TB_KEY_MOUSE_RELEASE) {
                    mouse.btn = mouse_event::button::none;  // termbox2 doesn't track which button
                    mouse.act = mouse_event::action::release;
                } else if (native.key == TB_KEY_MOUSE_WHEEL_UP) {
                    mouse.btn = mouse_event::button::none;
                    mouse.act = mouse_event::action::wheel_up;
                } else if (native.key == TB_KEY_MOUSE_WHEEL_DOWN) {
                    mouse.btn = mouse_event::button::none;
                    mouse.act = mouse_event::action::wheel_down;
                } else {
                    // Unknown mouse event
                    return std::nullopt;
                }

                // Copy modifiers
                mouse.modifiers.ctrl = (native.mod & TB_MOD_CTRL) != 0;
                mouse.modifiers.alt = (native.mod & TB_MOD_ALT) != 0;
                mouse.modifiers.shift = (native.mod & TB_MOD_SHIFT) != 0;

                return ui_event{mouse};
            }

            // ===== Resize Events =====
            if (native.type == TB_EVENT_RESIZE) {
                resize_event resize{};
                resize.width = conio_get_width();
                resize.height = conio_get_height();
                return ui_event{resize};
            }

            // Unknown event type
            return std::nullopt;
        }

        /**
         * @brief Register backend-provided themes
         * @param registry Theme registry to populate
         *
         * @details
         * Registers all built-in conio themes. The first theme registered
         * is considered the default theme.
         *
         * This method is called automatically by ui_context on first
         * context creation, so manual registration is not required.
         *
         * Registered themes (in order):
         * 1. "Norton Blue" (default) - Classic Norton Utilities
         * 2. "Borland Turbo" - Turbo Pascal/C++ IDE
         * 3. "Midnight Commander" - MC file manager
         * 4. "DOS Edit" - MS-DOS Edit
         */
        ONYXUI_CONIO_EXPORT static void register_themes(onyxui::theme_registry<conio_backend>& registry);

        // ========================================================================
        // Platform Integration (Custom Integration Mode)
        // ========================================================================

        /**
         * @brief Check if termbox2 is initialized (by vram)
         * @return true if termbox2 is ready for use, false otherwise
         *
         * @details
         * Termbox2 lifecycle is owned by vram via RAII. Creating a conio_renderer
         * automatically initializes termbox2 (via its internal vram). This method
         * simply checks whether termbox2 is currently initialized.
         *
         * In custom integration mode, create conio_renderer first, then call this
         * to verify initialization succeeded.
         */
        ONYXUI_CONIO_EXPORT static bool init();

        /**
         * @brief Clear backend state tracking
         *
         * @details
         * Termbox2 shutdown is handled by vram destructor (RAII). This method
         * only clears the backend's internal state tracking. Call this after
         * the conio_renderer (and its vram) have been destroyed.
         */
        ONYXUI_CONIO_EXPORT static void shutdown();

        /**
         * @brief Process termbox event and convert to ui_event
         * @param event Native termbox event
         * @return ui_event if event is relevant to UI, nullopt otherwise
         *
         * Use this in custom integration to convert termbox events.
         * This is essentially a wrapper around create_event().
         */
        [[nodiscard]] static std::optional<ui_event> process_event(const tb_event& event) {
            return create_event(event);
        }

        /**
         * @brief Check if quit was requested
         * @return true if application should exit
         */
        [[nodiscard]] ONYXUI_CONIO_EXPORT static bool should_quit() noexcept;

        /**
         * @brief Clear the quit flag
         */
        ONYXUI_CONIO_EXPORT static void clear_quit_flag() noexcept;

        /**
         * @brief Request application quit
         */
        ONYXUI_CONIO_EXPORT static void request_quit() noexcept;

        // ========================================================================
        // Standalone Application Mode
        // ========================================================================

        /**
         * @brief Run a complete standalone terminal application
         * @tparam Widget Widget class template (must accept Backend parameter)
         * @param setup Optional callback to configure widget after creation
         * @return Exit code (0 for success)
         *
         * Initializes termbox2, runs event loop, handles cleanup.
         * Terminal dimensions are used automatically.
         *
         * @code
         * template<typename Backend>
         * class MyApp : public main_window<Backend> {
         *     // ... widget implementation
         * };
         *
         * int main() {
         *     return conio_backend::run_app<MyApp>();
         * }
         * @endcode
         */
        template<template<typename> class Widget>
        static int run_app(std::function<void(Widget<conio_backend>&)> setup = nullptr);
    };

    // ======================================================================
    // Exported Termbox2 Wrapper Functions
    // ======================================================================
    // These wrappers provide properly exported symbols for termbox2 functions
    // needed by applications using the conio backend as a shared library.

    /**
     * @brief Get terminal width (wrapper for tb_width)
     * @return Terminal width in characters
     */
    ONYXUI_CONIO_EXPORT int conio_get_width();

    /**
     * @brief Get terminal height (wrapper for tb_height)
     * @return Terminal height in characters
     */
    ONYXUI_CONIO_EXPORT int conio_get_height();

    /**
     * @brief Poll for terminal events (wrapper for tb_poll_event)
     * @param event Pointer to event structure to fill
     * @return Status code from tb_poll_event
     */
    ONYXUI_CONIO_EXPORT int conio_poll_event(tb_event* event);

} // namespace onyxui::conio


// Static assertion to verify backend compliance (after event_traits specialization)
static_assert(onyxui::UIBackend<onyxui::conio::conio_backend>, "conio_backend must satisfy UIBackend concept");

// Include template implementation
#include <onyxui/conio/conio_backend_impl.hh>
