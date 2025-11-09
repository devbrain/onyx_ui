/**
 * @file conio_backend.hh
 * @brief Console I/O backend implementation using termbox2
 * @author igor
 * @date 16/10/2025
 *
 * @details
 * Provides a text-based UI backend for terminal applications.
 * Uses termbox2 library for cross-platform console rendering.
 */

#pragma once

#include <onyxui/concepts/backend.hh>
#include <onyxui/conio/geometry.hh>
#include <onyxui/conio/colors.hh>
#include <onyxui/conio/conio_renderer.hh>
#include <onyxui/conio/conio_events.hh>
#include <onyxui/events/ui_event.hh>
#include <onyxui/theming/theme_registry.hh>
#include <onyxui/backends/conio/onyxui_conio_export.h>
#include <optional>
#include <cctype>

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
     * auto vram_instance = std::make_shared<vram>();
     * conio_renderer renderer(vram_instance);
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
                        // Unknown character
                        return std::nullopt;
                    }
                } else {
                    // Unknown key
                    return std::nullopt;
                }

                // Build modifiers (terminal reality: Enter=Ctrl+M, Tab=Ctrl+I)
                const bool is_enter_or_tab = (native.key == TB_KEY_ENTER || native.key == TB_KEY_TAB);

                key_modifier mods = key_modifier::none;
                if (!is_enter_or_tab && (native.mod & TB_MOD_CTRL) != 0) mods |= key_modifier::ctrl;
                if ((native.mod & TB_MOD_ALT) != 0) mods |= key_modifier::alt;

                // For uppercase letters, shift=true (already normalized to lowercase)
                if (native.ch >= 'A' && native.ch <= 'Z') {
                    mods |= key_modifier::shift;
                } else if (!is_enter_or_tab && (native.mod & TB_MOD_SHIFT) != 0) {
                    mods |= key_modifier::shift;
                }

                kbd.modifiers = mods;
                kbd.pressed = true;  // termbox2 doesn't distinguish press/release

                return ui_event{kbd};
            }

            // ===== Mouse Events =====
            if (native.type == TB_EVENT_MOUSE) {
                mouse_event mouse{};
                mouse.x = native.x;
                mouse.y = native.y;

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
