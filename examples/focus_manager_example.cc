/**
 * @file focus_manager_example.cc
 * @brief Example demonstrating the new focus_manager with event traits
 *
 * This example shows how to use the rewritten focus_manager that's compatible
 * with the event_target and event traits architecture.
 */

#include <iostream>
#include <vector>
#include <string>

// Define SDL backend for this example
#define ONYXUI_SDL_BACKEND
#include <onyxui/focus_manager.hh>
#include <onyxui/event_target.hh>

// Simple button class that can be focused
template<typename EventType>
class button : public onyxui::event_target<EventType> {
public:
    button(const std::string& label) : m_label(label) {
        // Make buttons focusable by default
        this->set_focusable(true);
    }

    // Override to provide hit testing
    bool is_inside(int x, int y) const override {
        // Simple rectangular bounds check
        return x >= m_x && x < m_x + m_width &&
               y >= m_y && y < m_y + m_height;
    }

    // Override focus handlers to show visual feedback
    bool handle_focus_gained() override {
        std::cout << "Button '" << m_label << "' gained focus\n";
        return onyxui::event_target<EventType>::handle_focus_gained();
    }

    bool handle_focus_lost() override {
        std::cout << "Button '" << m_label << "' lost focus\n";
        return onyxui::event_target<EventType>::handle_focus_lost();
    }

    bool handle_click(int x, int y) override {
        std::cout << "Button '" << m_label << "' clicked at (" << x << ", " << y << ")\n";
        return true;
    }

    bool handle_key_down(int key, bool shift, bool ctrl, bool alt) override {
        std::cout << "Button '" << m_label << "' received key: " << key
                  << " (shift:" << shift << ", ctrl:" << ctrl << ", alt:" << alt << ")\n";
        return true;
    }

    // Set position and size
    void set_bounds(int x, int y, int w, int h) {
        m_x = x;
        m_y = y;
        m_width = w;
        m_height = h;
    }

    const std::string& label() const { return m_label; }

private:
    std::string m_label;
    int m_x = 0, m_y = 0;
    int m_width = 100, m_height = 30;
};

// Container that holds multiple buttons
template<typename EventType>
class button_panel : public onyxui::event_target<EventType> {
public:
    // Add a button to the panel
    void add_button(button<EventType>* btn) {
        m_buttons.push_back(btn);
    }

    // Get all buttons as event targets
    std::vector<onyxui::event_target<EventType>*> get_targets() {
        std::vector<onyxui::event_target<EventType>*> targets;
        for (auto* btn : m_buttons) {
            targets.push_back(btn);
        }
        return targets;
    }

    bool is_inside(int x, int y) const override {
        // Panel covers entire area
        return true;
    }

private:
    std::vector<button<EventType>*> m_buttons;
};

#ifdef ONYXUI_SDL_BACKEND
void demonstrate_focus_manager() {
    std::cout << "=== Focus Manager Example with Event Traits ===\n\n";

    // Create focus manager for SDL events
    onyxui::focus_manager<SDL_Event> focus_mgr;

    // Create some buttons
    button<SDL_Event> btn_ok("OK");
    button<SDL_Event> btn_cancel("Cancel");
    button<SDL_Event> btn_help("Help");

    // Set positions
    btn_ok.set_bounds(10, 10, 100, 30);
    btn_cancel.set_bounds(120, 10, 100, 30);
    btn_help.set_bounds(230, 10, 100, 30);

    // Set tab indices
    btn_ok.set_tab_index(0);
    btn_cancel.set_tab_index(1);
    btn_help.set_tab_index(2);

    // Create panel and add buttons
    button_panel<SDL_Event> panel;
    panel.add_button(&btn_ok);
    panel.add_button(&btn_cancel);
    panel.add_button(&btn_help);

    // Get all targets for focus management
    auto targets = panel.get_targets();

    std::cout << "Created 3 buttons: OK, Cancel, Help\n";
    std::cout << "Tab indices: OK(0), Cancel(1), Help(2)\n\n";

    // Set initial focus
    std::cout << "Setting initial focus to OK button:\n";
    focus_mgr.set_focus(&btn_ok);
    std::cout << "\n";

    // Navigate forward
    std::cout << "Moving focus forward (Tab):\n";
    focus_mgr.focus_next(targets);
    std::cout << "\n";

    std::cout << "Moving focus forward again:\n";
    focus_mgr.focus_next(targets);
    std::cout << "\n";

    std::cout << "Moving focus forward (should wrap to first):\n";
    focus_mgr.focus_next(targets);
    std::cout << "\n";

    // Navigate backward
    std::cout << "Moving focus backward (Shift+Tab):\n";
    focus_mgr.focus_previous(targets);
    std::cout << "\n";

    // Simulate keyboard event handling
    std::cout << "Simulating keyboard events:\n";

    // Create a fake keyboard event
    SDL_KeyboardEvent key_event;
    key_event.type = SDL_KEYDOWN;
    key_event.keysym.sym = SDLK_SPACE;
    key_event.keysym.mod = KMOD_NONE;
    key_event.repeat = 0;
    key_event.timestamp = 0;

    std::cout << "Sending SPACE key to focused element:\n";
    if (focus_mgr.forward_to_focused(key_event)) {
        std::cout << "Event was handled by focused element\n";
    }
    std::cout << "\n";

    // Test Tab navigation with actual event
    key_event.keysym.sym = SDLK_TAB;
    std::cout << "Processing Tab key event:\n";
    if (focus_mgr.handle_tab_navigation(key_event, targets)) {
        std::cout << "Tab navigation handled - focus moved\n";
    }
    std::cout << "\n";

    // Test disabling an element
    std::cout << "Disabling Cancel button and trying to focus it:\n";
    btn_cancel.set_enabled(false);
    if (!focus_mgr.set_focus(&btn_cancel)) {
        std::cout << "Could not focus disabled button (correct behavior)\n";
    }
    std::cout << "\n";

    // Clear focus
    std::cout << "Clearing focus:\n";
    focus_mgr.clear_focus();
    if (!focus_mgr.get_focused()) {
        std::cout << "No element has focus\n";
    }
    std::cout << "\n";

    std::cout << "=== Example Complete ===\n";
}
#endif

int main() {
    #ifdef ONYXUI_SDL_BACKEND
    demonstrate_focus_manager();
    #else
    std::cout << "This example requires SDL backend to be defined.\n";
    std::cout << "Compile with -DONYXUI_SDL_BACKEND\n";
    #endif
    return 0;
}