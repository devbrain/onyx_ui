//
// Created by igor on 16/10/2025.
//

#pragma once

#include <onyxui/widgets/widget.hh>
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

            void simulate_click() {
                this->handle_click(0, 0);
            }
    };
}
