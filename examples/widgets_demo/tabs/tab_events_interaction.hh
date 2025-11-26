//
// Created by Claude Code on 2025-11-23.
//
// Tab 3: Events & Interaction - Event system, focus management, and hotkeys
// Demonstrates event routing, focus control, and keyboard interaction
//

#pragma once
#include <onyxui/widgets/containers/panel.hh>
#include <onyxui/widgets/containers/vbox.hh>
#include <onyxui/widgets/containers/hbox.hh>
#include <onyxui/widgets/containers/group_box.hh>
#include <onyxui/widgets/containers/scroll_view.hh>
#include <onyxui/widgets/button.hh>
#include <onyxui/widgets/label.hh>
#include <onyxui/widgets/input/line_edit.hh>
#include <onyxui/widgets/input/checkbox.hh>
#include <onyxui/services/ui_services.hh>
#include <memory>
#include <string>

namespace widgets_demo_tabs {

/**
 * @brief Create Tab 3: Events & Interaction
 *
 * @details
 * Demonstrates:
 * - Event system (three-phase routing, mouse/keyboard events)
 * - Focus management (tab order, focus control, focus chain)
 * - Hotkeys (global shortcuts, mnemonics, schemes)
 *
 * @tparam Backend UI backend type
 * @return Panel containing the complete tab content
 */
template<onyxui::UIBackend Backend>
std::unique_ptr<onyxui::panel<Backend>> create_tab_events_interaction() {
    auto tab = std::make_unique<onyxui::panel<Backend>>();

    // Make entire tab scrollable
    auto* scroll = tab->template emplace_child<onyxui::scroll_view>();

    // Main content container (vertical layout)
    auto* content = scroll->template emplace_child<onyxui::vbox>(1);  // 1px spacing

    // Title
    auto* title = content->template emplace_child<onyxui::label>("Events & Interaction");
    title->set_horizontal_align(onyxui::horizontal_alignment::center);

    content->template emplace_child<onyxui::label>("");  // Spacer

    // ========== ROW 1: Event System | Focus Management ==========
    auto* row1 = content->template emplace_child<onyxui::hbox>(2);

    // ========== EVENT SYSTEM SECTION ==========
    auto* event_section = row1->template emplace_child<onyxui::group_box>();
    event_section->set_title("Event System");
    event_section->set_vbox_layout(1);

    // Three-Phase Routing
    event_section->template emplace_child<onyxui::label>("Three-Phase Event Routing:");
    event_section->template emplace_child<onyxui::label>("  1. CAPTURE Phase: Root to target (top-down)");
    event_section->template emplace_child<onyxui::label>("     - Parent intercepts before children");
    event_section->template emplace_child<onyxui::label>("     - Used for focus management, text_view click handling");
    event_section->template emplace_child<onyxui::label>("");  // Spacer
    event_section->template emplace_child<onyxui::label>("  2. TARGET Phase: At the target element");
    event_section->template emplace_child<onyxui::label>("     - Normal event handling");
    event_section->template emplace_child<onyxui::label>("     - Button clicks, checkbox toggles");
    event_section->template emplace_child<onyxui::label>("");  // Spacer
    event_section->template emplace_child<onyxui::label>("  3. BUBBLE Phase: Target to root (bottom-up)");
    event_section->template emplace_child<onyxui::label>("     - Unused events propagate to parents");
    event_section->template emplace_child<onyxui::label>("     - Container can handle child events");

    // Mouse Events
    event_section->template emplace_child<onyxui::label>("");  // Spacer
    event_section->template emplace_child<onyxui::label>("Mouse Events:");
    event_section->template emplace_child<onyxui::label>("  - Click: Single click (button pressed & released)");
    event_section->template emplace_child<onyxui::label>("  - Double-click: Two clicks in quick succession");
    event_section->template emplace_child<onyxui::label>("  - Mouse move: Cursor movement (hover detection)");
    event_section->template emplace_child<onyxui::label>("  - Mouse wheel: Scrolling (vertical/horizontal)");

    // Keyboard Events
    event_section->template emplace_child<onyxui::label>("");  // Spacer
    event_section->template emplace_child<onyxui::label>("Keyboard Events:");
    event_section->template emplace_child<onyxui::label>("  - Key press: Character keys (a-z, 0-9, etc.)");
    event_section->template emplace_child<onyxui::label>("  - Special keys: Tab, Enter, Escape, arrows");
    event_section->template emplace_child<onyxui::label>("  - Modifiers: Ctrl, Alt, Shift combinations");
    event_section->template emplace_child<onyxui::label>("  - Mnemonics: Alt+Letter for menu navigation");

    // Event Demonstration
    event_section->template emplace_child<onyxui::label>("");  // Spacer
    event_section->template emplace_child<onyxui::label>("Interactive Demo:");

    auto* event_demo_row = event_section->template emplace_child<onyxui::hbox>(2);
    auto* event_btn = event_demo_row->template emplace_child<onyxui::button>("Click Me!");
    auto* event_label = event_demo_row->template emplace_child<onyxui::label>("Not clicked yet");

    int click_count = 0;
    event_btn->clicked.connect([event_label, click_count]() mutable {
        click_count++;
        event_label->set_text("Clicked " + std::to_string(click_count) + " times");
    });

    // ========== FOCUS MANAGEMENT SECTION ==========
    auto* focus_section = row1->template emplace_child<onyxui::group_box>();
    focus_section->set_title("Focus Management");
    focus_section->set_vbox_layout(1);

    focus_section->template emplace_child<onyxui::label>("Focus System:");
    focus_section->template emplace_child<onyxui::label>("  - Tab order: Sequential navigation through focusable widgets");
    focus_section->template emplace_child<onyxui::label>("  - Tab: Move focus forward");
    focus_section->template emplace_child<onyxui::label>("  - Shift+Tab: Move focus backward");
    focus_section->template emplace_child<onyxui::label>("  - Mouse click: Focus widget directly");
    focus_section->template emplace_child<onyxui::label>("  - Programmatic: set_focus() on any widget");

    // Focus Indicators
    focus_section->template emplace_child<onyxui::label>("");  // Spacer
    focus_section->template emplace_child<onyxui::label>("Focus Indicators:");
    focus_section->template emplace_child<onyxui::label>("  - Visual highlight (theme-dependent)");
    focus_section->template emplace_child<onyxui::label>("  - Cursor position (text inputs)");
    focus_section->template emplace_child<onyxui::label>("  - Focus rectangle or color change");

    // Focus Demo
    focus_section->template emplace_child<onyxui::label>("");  // Spacer
    focus_section->template emplace_child<onyxui::label>("Focus Navigation Demo (press Tab to cycle):");

    auto* focus_demo = focus_section->template emplace_child<onyxui::vbox>(1);
    auto* focus_edit1 = focus_demo->template emplace_child<onyxui::line_edit>();
    focus_edit1->set_text("First input (Tab to next)");

    auto* focus_btn1 = focus_demo->template emplace_child<onyxui::button>("Button 1");
    auto* focus_btn2 = focus_demo->template emplace_child<onyxui::button>("Button 2");

    auto* focus_edit2 = focus_demo->template emplace_child<onyxui::line_edit>();
    focus_edit2->set_text("Second input (Shift+Tab to previous)");

    auto* focus_checkbox = focus_demo->template emplace_child<onyxui::checkbox>("Checkbox (also focusable)");

    // Focus Control Buttons
    focus_section->template emplace_child<onyxui::label>("");  // Spacer
    focus_section->template emplace_child<onyxui::label>("Programmatic Focus Control:");

    auto* focus_controls = focus_section->template emplace_child<onyxui::hbox>(2);
    auto* focus_first_btn = focus_controls->template emplace_child<onyxui::button>("Focus First Input");
    focus_first_btn->clicked.connect([focus_edit1]() {
        auto* input_mgr = onyxui::ui_services<Backend>::input();
        if (input_mgr) {
            input_mgr->set_focus(focus_edit1);
        }
    });

    auto* focus_button_btn = focus_controls->template emplace_child<onyxui::button>("Focus Button 1");
    focus_button_btn->clicked.connect([focus_btn1]() {
        auto* input_mgr = onyxui::ui_services<Backend>::input();
        if (input_mgr) {
            input_mgr->set_focus(focus_btn1);
        }
    });

    // ========== ROW 2: Hotkeys | Signal/Slot ==========
    auto* row2 = content->template emplace_child<onyxui::hbox>(2);

    // ========== HOTKEYS SECTION ==========
    auto* hotkeys_section = row2->template emplace_child<onyxui::group_box>();
    hotkeys_section->set_title("Hotkeys & Shortcuts");
    hotkeys_section->set_vbox_layout(1);

    hotkeys_section->template emplace_child<onyxui::label>("Hotkey System:");
    hotkeys_section->template emplace_child<onyxui::label>("  - Global shortcuts: Work anywhere in the application");
    hotkeys_section->template emplace_child<onyxui::label>("  - Actions: Encapsulate commands with shortcuts");
    hotkeys_section->template emplace_child<onyxui::label>("  - Schemes: Windows vs Norton Commander key bindings");
    hotkeys_section->template emplace_child<onyxui::label>("  - Semantic actions: High-level commands (activate_menu_bar)");

    // Registered Hotkeys
    hotkeys_section->template emplace_child<onyxui::label>("");  // Spacer
    hotkeys_section->template emplace_child<onyxui::label>("Application Hotkeys:");
    hotkeys_section->template emplace_child<onyxui::label>("  F9 / Ctrl+S  - Take Screenshot");
    hotkeys_section->template emplace_child<onyxui::label>("  Ctrl+M       - Open MVC Demo Window");
    hotkeys_section->template emplace_child<onyxui::label>("  Ctrl+T       - Open Theme Editor Window");
    hotkeys_section->template emplace_child<onyxui::label>("  F12          - Open Debug Tools Window");
    hotkeys_section->template emplace_child<onyxui::label>("  Alt+F4       - Exit Application");
    hotkeys_section->template emplace_child<onyxui::label>("  Ctrl+Tab     - Next Tab");
    hotkeys_section->template emplace_child<onyxui::label>("  Ctrl+Shift+Tab - Previous Tab");

    // Mnemonic Navigation
    hotkeys_section->template emplace_child<onyxui::label>("");  // Spacer
    hotkeys_section->template emplace_child<onyxui::label>("Mnemonic Navigation:");
    hotkeys_section->template emplace_child<onyxui::label>("  - Alt+Letter: Activate menu or button");
    hotkeys_section->template emplace_child<onyxui::label>("  - Underlined letter indicates mnemonic");
    hotkeys_section->template emplace_child<onyxui::label>("  - Example: Alt+F opens File menu");
    hotkeys_section->template emplace_child<onyxui::label>("  - Example: Alt+W opens Windows menu");

    // Mnemonic Demo
    hotkeys_section->template emplace_child<onyxui::label>("");  // Spacer
    hotkeys_section->template emplace_child<onyxui::label>("Mnemonic Demo:");

    auto* mnemonic_row = hotkeys_section->template emplace_child<onyxui::hbox>(2);

    auto* mnemonic_btn1 = mnemonic_row->template emplace_child<onyxui::button>("");
    mnemonic_btn1->set_mnemonic_text("&Save (Alt+S)");
    auto* mnemonic_label1 = mnemonic_row->template emplace_child<onyxui::label>("Not pressed");
    mnemonic_btn1->clicked.connect([mnemonic_label1]() {
        mnemonic_label1->set_text("Save pressed!");
    });

    auto* mnemonic_btn2 = mnemonic_row->template emplace_child<onyxui::button>("");
    mnemonic_btn2->set_mnemonic_text("&Cancel (Alt+C)");
    auto* mnemonic_label2 = mnemonic_row->template emplace_child<onyxui::label>("Not pressed");
    mnemonic_btn2->clicked.connect([mnemonic_label2]() {
        mnemonic_label2->set_text("Cancel pressed!");
    });

    // ========== SIGNAL/SLOT PATTERN SECTION ==========
    auto* signal_section = row2->template emplace_child<onyxui::group_box>();
    signal_section->set_title("Signal/Slot Pattern");
    signal_section->set_vbox_layout(1);

    signal_section->template emplace_child<onyxui::label>("Decoupled Communication:");
    signal_section->template emplace_child<onyxui::label>("  - Signals: Event notifications (clicked, value_changed)");
    signal_section->template emplace_child<onyxui::label>("  - Slots: Handler functions connected to signals");
    signal_section->template emplace_child<onyxui::label>("  - Multiple connections: One signal, many handlers");
    signal_section->template emplace_child<onyxui::label>("  - Scoped connections: RAII-based automatic cleanup");

    signal_section->template emplace_child<onyxui::label>("");  // Spacer
    signal_section->template emplace_child<onyxui::label>("Common Signals:");
    signal_section->template emplace_child<onyxui::label>("  - button::clicked() - Button was clicked");
    signal_section->template emplace_child<onyxui::label>("  - checkbox::toggled(bool) - Checkbox state changed");
    signal_section->template emplace_child<onyxui::label>("  - slider::value_changed(int) - Slider value changed");
    signal_section->template emplace_child<onyxui::label>("  - line_edit::text_changed(string) - Text modified");
    signal_section->template emplace_child<onyxui::label>("  - window::closed() - Window was closed");
    signal_section->template emplace_child<onyxui::label>("  - tab_widget::current_changed(int) - Tab switched");

    // Tips
    content->template emplace_child<onyxui::label>("");  // Spacer
    auto* tips_section = content->template emplace_child<onyxui::group_box>();
    tips_section->set_title("Tips & Best Practices");
    tips_section->set_vbox_layout(1);

    tips_section->template emplace_child<onyxui::label>("Event Handling:");
    tips_section->template emplace_child<onyxui::label>("  - Return true from handle_event() to stop propagation");
    tips_section->template emplace_child<onyxui::label>("  - Use CAPTURE phase for pre-processing (e.g., focus)");
    tips_section->template emplace_child<onyxui::label>("  - Use TARGET phase for normal handling");
    tips_section->template emplace_child<onyxui::label>("  - Use BUBBLE phase for container-level handling");

    tips_section->template emplace_child<onyxui::label>("");  // Spacer
    tips_section->template emplace_child<onyxui::label>("Focus Management:");
    tips_section->template emplace_child<onyxui::label>("  - set_focusable(true) to allow focus");
    tips_section->template emplace_child<onyxui::label>("  - set_focus() to programmatically focus a widget");
    tips_section->template emplace_child<onyxui::label>("  - Tab order follows widget tree order by default");

    tips_section->template emplace_child<onyxui::label>("");  // Spacer
    tips_section->template emplace_child<onyxui::label>("Hotkeys:");
    tips_section->template emplace_child<onyxui::label>("  - Keep actions alive with shared_ptr in member variable");
    tips_section->template emplace_child<onyxui::label>("  - Use semantic actions for scheme-independent behavior");
    tips_section->template emplace_child<onyxui::label>("  - Document all application hotkeys for discoverability");

    return tab;
}

} // namespace widgets_demo_tabs
