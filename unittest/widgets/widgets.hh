//
// Created by igor on 16/10/2025.
//

#pragma once

#include <../../include/onyxui/widgets/core/widget.hh>
#include <onyxui/widgets/button.hh>
#include <onyxui/events/ui_event.hh>

namespace onyxui {
    // Test fixture to expose protected methods and simulate events
    template<UIBackend Backend>
    class test_widget : public widget <Backend> {
        public:
            using widget <Backend>::widget;
            // New API - expose handle_mouse/keyboard/resize for testing
            using widget <Backend>::handle_mouse;
            using widget <Backend>::handle_keyboard;
            using widget <Backend>::handle_resize;

            // Public wrapper to expose protected set_focus for testing
            void test_set_focus(bool focused) {
                this->set_focus(focused);
            }

            // Simulate a complete click (press + release)
            void simulate_click() {
                // Ensure widget has valid bounds for hit testing
                auto current_bounds = this->bounds();
                if (current_bounds.width.to_int() == 0 || current_bounds.height.to_int() == 0) {
                    this->arrange(logical_rect{0_lu, 0_lu, 100_lu, 50_lu});
                }

                mouse_event press{.x = 0, .y = 0, .btn = mouse_event::button::left, .act = mouse_event::action::press, .modifiers = {}};
                this->handle_event(ui_event{press}, event_phase::target);

                mouse_event release{.x = 0, .y = 0, .btn = mouse_event::button::left, .act = mouse_event::action::release, .modifiers = {}};
                this->handle_event(ui_event{release}, event_phase::target);
            }
    };

    template<UIBackend Backend>
    class test_button : public button <Backend> {
        public:
            using button <Backend>::button;
            // New API - expose handle_mouse for testing
            using button <Backend>::handle_mouse;

            void simulate_click() {
                // Ensure widget has valid bounds for hit testing
                auto current_bounds = this->bounds();
                if (current_bounds.width.to_int() == 0 || current_bounds.height.to_int() == 0) {
                    this->arrange(logical_rect{0_lu, 0_lu, 100_lu, 50_lu});
                }

                mouse_event press{.x = 0, .y = 0, .btn = mouse_event::button::left, .act = mouse_event::action::press, .modifiers = {}};
                this->handle_event(ui_event{press}, event_phase::target);

                mouse_event release{.x = 0, .y = 0, .btn = mouse_event::button::left, .act = mouse_event::action::release, .modifiers = {}};
                this->handle_event(ui_event{release}, event_phase::target);
            }

            // Expose state query for testing (visual interaction_state)
            [[nodiscard]] bool is_visually_pressed() const {
                return this->get_interaction_state() == button<Backend>::interaction_state::pressed;
            }

            [[nodiscard]] bool is_visually_hovered() const {
                return this->get_interaction_state() == button<Backend>::interaction_state::hover;
            }

            [[nodiscard]] bool is_visually_normal() const {
                return this->get_interaction_state() == button<Backend>::interaction_state::normal;
            }

        protected:
            using button<Backend>::get_interaction_state;
    };
}
