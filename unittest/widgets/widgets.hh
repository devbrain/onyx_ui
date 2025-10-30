//
// Created by igor on 16/10/2025.
//

#pragma once

#include <../../include/onyxui/widgets/core/widget.hh>
#include <onyxui/widgets/button.hh>

namespace onyxui {
    // Test fixture to expose protected methods and simulate events
    template<UIBackend Backend>
    class test_widget : public widget <Backend> {
        public:
            using widget <Backend>::widget;
            using widget <Backend>::handle_mouse_enter;
            using widget <Backend>::handle_mouse_leave;
            using widget <Backend>::handle_click;
            using widget <Backend>::handle_focus_gained;
            using widget <Backend>::handle_focus_lost;

            // Simulate a complete click (press + release)
            void simulate_click() {
                this->handle_click(0, 0);
            }
    };

    template<UIBackend Backend>
    class test_button : public button <Backend> {
        public:
            using button <Backend>::button;
            using button <Backend>::handle_click;
            using button <Backend>::handle_mouse_down;
            using button <Backend>::handle_mouse_up;
            using button <Backend>::handle_mouse_leave;
            using button <Backend>::handle_mouse_enter;
            using button <Backend>::process_mouse_move;
            using button <Backend>::process_mouse_button;

            void simulate_click() {
                this->handle_click(0, 0);
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
