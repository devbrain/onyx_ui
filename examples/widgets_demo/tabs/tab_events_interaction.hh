//
// Created by Claude Code on 2025-11-23.
//
// Tab 3: Events & Interaction - Event system, focus management, and hotkeys
// Demonstrates event routing, focus control, and keyboard interaction
//

#pragma once
#include "../backend_config.hh"  // Pulls the simple-shell bundle + all widget headers.

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
 * @return Panel containing the complete tab content
 */
inline std::unique_ptr<ui::panel> create_tab_events_interaction() {
    auto tab = std::make_unique<ui::panel>();
    tab->set_padding(onyxui::logical_thickness(onyxui::logical_unit(1.0)));

    // Make entire tab scrollable
    auto* scroll = tab->emplace_child<ui::scroll_view>();

    // Main content container (vertical layout)
    auto* content = scroll->content_emplace_child<ui::vbox>(onyxui::spacing::tiny);  // 1px spacing

    // Title
    auto* title = content->emplace_child<ui::label>("Events & Interaction");
    title->set_horizontal_align(onyxui::horizontal_alignment::center);

    content->emplace_child<ui::label>("");  // Spacer

    // ========== ROW 1: Event System | Focus Management ==========
    auto* row1 = content->emplace_child<ui::hbox>(onyxui::spacing::medium);

    // ========== EVENT SYSTEM SECTION ==========
    auto* event_section = row1->emplace_child<ui::group_box>();
    event_section->set_title("Event System");
    event_section->set_vbox_layout(onyxui::spacing::tiny);

    // Three-Phase Routing
    event_section->emplace_child<ui::label>("Three-Phase Event Routing:");
    event_section->emplace_child<ui::label>("  1. CAPTURE Phase: Root to target (top-down)");
    event_section->emplace_child<ui::label>("     - Parent intercepts before children");
    event_section->emplace_child<ui::label>("     - Used for focus management, text_view click handling");
    event_section->emplace_child<ui::label>("");  // Spacer
    event_section->emplace_child<ui::label>("  2. TARGET Phase: At the target element");
    event_section->emplace_child<ui::label>("     - Normal event handling");
    event_section->emplace_child<ui::label>("     - Button clicks, checkbox toggles");
    event_section->emplace_child<ui::label>("");  // Spacer
    event_section->emplace_child<ui::label>("  3. BUBBLE Phase: Target to root (bottom-up)");
    event_section->emplace_child<ui::label>("     - Unused events propagate to parents");
    event_section->emplace_child<ui::label>("     - Container can handle child events");

    // Mouse Events
    event_section->emplace_child<ui::label>("");  // Spacer
    event_section->emplace_child<ui::label>("Mouse Events:");
    event_section->emplace_child<ui::label>("  - Click: Single click (button pressed & released)");
    event_section->emplace_child<ui::label>("  - Double-click: Two clicks in quick succession");
    event_section->emplace_child<ui::label>("  - Mouse move: Cursor movement (hover detection)");
    event_section->emplace_child<ui::label>("  - Mouse wheel: Scrolling (vertical/horizontal)");

    // Keyboard Events
    event_section->emplace_child<ui::label>("");  // Spacer
    event_section->emplace_child<ui::label>("Keyboard Events:");
    event_section->emplace_child<ui::label>("  - Key press: Character keys (a-z, 0-9, etc.)");
    event_section->emplace_child<ui::label>("  - Special keys: Tab, Enter, Escape, arrows");
    event_section->emplace_child<ui::label>("  - Modifiers: Ctrl, Alt, Shift combinations");
    event_section->emplace_child<ui::label>("  - Mnemonics: Alt+Letter for menu navigation");

    // Event Demonstration
    event_section->emplace_child<ui::label>("");  // Spacer
    event_section->emplace_child<ui::label>("Interactive Demo:");

    auto* event_demo_row = event_section->emplace_child<ui::hbox>(onyxui::spacing::small);
    auto* event_btn = event_demo_row->emplace_child<ui::button>("Click Me!");
    auto* event_label = event_demo_row->emplace_child<ui::label>("Not clicked yet");

    int click_count = 0;
    event_btn->clicked.connect([event_label, click_count]() mutable {
        click_count++;
        event_label->set_text("Clicked " + std::to_string(click_count) + " times");
    });

    // ========== FOCUS MANAGEMENT SECTION ==========
    auto* focus_section = row1->emplace_child<ui::group_box>();
    focus_section->set_title("Focus Management");
    focus_section->set_vbox_layout(onyxui::spacing::tiny);

    focus_section->emplace_child<ui::label>("Focus System:");
    focus_section->emplace_child<ui::label>("  - Tab order: Sequential navigation through focusable widgets");
    focus_section->emplace_child<ui::label>("  - Tab: Move focus forward");
    focus_section->emplace_child<ui::label>("  - Shift+Tab: Move focus backward");
    focus_section->emplace_child<ui::label>("  - Mouse click: Focus widget directly");
    focus_section->emplace_child<ui::label>("  - Programmatic: set_focus() on any widget");

    // Focus Indicators
    focus_section->emplace_child<ui::label>("");  // Spacer
    focus_section->emplace_child<ui::label>("Focus Indicators:");
    focus_section->emplace_child<ui::label>("  - Visual highlight (theme-dependent)");
    focus_section->emplace_child<ui::label>("  - Cursor position (text inputs)");
    focus_section->emplace_child<ui::label>("  - Focus rectangle or color change");

    // Focus Demo
    focus_section->emplace_child<ui::label>("");  // Spacer
    focus_section->emplace_child<ui::label>("Focus Navigation Demo (press Tab to cycle):");

    auto* focus_demo = focus_section->emplace_child<ui::vbox>(onyxui::spacing::tiny);
    auto* focus_edit1 = focus_demo->emplace_child<ui::line_edit>();
    focus_edit1->set_text("First input (Tab to next)");

    auto* focus_btn1 = focus_demo->emplace_child<ui::button>("Button 1");
    auto* focus_btn2 = focus_demo->emplace_child<ui::button>("Button 2");

    auto* focus_edit2 = focus_demo->emplace_child<ui::line_edit>();
    focus_edit2->set_text("Second input (Shift+Tab to previous)");

    auto* focus_checkbox = focus_demo->emplace_child<ui::checkbox>("Checkbox (also focusable)");

    // Focus Control Buttons
    focus_section->emplace_child<ui::label>("");  // Spacer
    focus_section->emplace_child<ui::label>("Programmatic Focus Control:");

    auto* focus_controls = focus_section->emplace_child<ui::hbox>(onyxui::spacing::small);
    auto* focus_first_btn = focus_controls->emplace_child<ui::button>("Focus First Input");
    focus_first_btn->clicked.connect([focus_edit1]() {
        auto* input_mgr = ui::ui_services::input();
        if (input_mgr) {
            input_mgr->set_focus(focus_edit1);
        }
    });

    auto* focus_button_btn = focus_controls->emplace_child<ui::button>("Focus Button 1");
    focus_button_btn->clicked.connect([focus_btn1]() {
        auto* input_mgr = ui::ui_services::input();
        if (input_mgr) {
            input_mgr->set_focus(focus_btn1);
        }
    });

    // ========== ROW 2: Hotkeys | Signal/Slot ==========
    auto* row2 = content->emplace_child<ui::hbox>(onyxui::spacing::medium);

    // ========== HOTKEYS SECTION ==========
    auto* hotkeys_section = row2->emplace_child<ui::group_box>();
    hotkeys_section->set_title("Hotkeys & Shortcuts");
    hotkeys_section->set_vbox_layout(onyxui::spacing::tiny);

    hotkeys_section->emplace_child<ui::label>("Hotkey System:");
    hotkeys_section->emplace_child<ui::label>("  - Global shortcuts: Work anywhere in the application");
    hotkeys_section->emplace_child<ui::label>("  - Actions: Encapsulate commands with shortcuts");
    hotkeys_section->emplace_child<ui::label>("  - Schemes: Windows vs Norton Commander key bindings");
    hotkeys_section->emplace_child<ui::label>("  - Semantic actions: High-level commands (activate_menu_bar)");

    // Registered Hotkeys
    hotkeys_section->emplace_child<ui::label>("");  // Spacer
    hotkeys_section->emplace_child<ui::label>("Application Hotkeys:");
    hotkeys_section->emplace_child<ui::label>("  F9 / Ctrl+S  - Take Screenshot");
    hotkeys_section->emplace_child<ui::label>("  Ctrl+M       - Open MVC Demo Window");
    hotkeys_section->emplace_child<ui::label>("  Ctrl+T       - Open Theme Editor Window");
    hotkeys_section->emplace_child<ui::label>("  F12          - Open Debug Tools Window");
    hotkeys_section->emplace_child<ui::label>("  Alt+F4       - Exit Application");
    hotkeys_section->emplace_child<ui::label>("  Ctrl+Tab     - Next Tab");
    hotkeys_section->emplace_child<ui::label>("  Ctrl+Shift+Tab - Previous Tab");

    // Mnemonic Navigation
    hotkeys_section->emplace_child<ui::label>("");  // Spacer
    hotkeys_section->emplace_child<ui::label>("Mnemonic Navigation:");
    hotkeys_section->emplace_child<ui::label>("  - Alt+Letter: Activate menu or button");
    hotkeys_section->emplace_child<ui::label>("  - Underlined letter indicates mnemonic");
    hotkeys_section->emplace_child<ui::label>("  - Example: Alt+F opens File menu");
    hotkeys_section->emplace_child<ui::label>("  - Example: Alt+W opens Windows menu");

    // Mnemonic Demo
    hotkeys_section->emplace_child<ui::label>("");  // Spacer
    hotkeys_section->emplace_child<ui::label>("Mnemonic Demo:");

    auto* mnemonic_row = hotkeys_section->emplace_child<ui::hbox>(onyxui::spacing::small);

    auto* mnemonic_btn1 = mnemonic_row->emplace_child<ui::button>("");
    mnemonic_btn1->set_mnemonic_text("&Save (Alt+S)");
    auto* mnemonic_label1 = mnemonic_row->emplace_child<ui::label>("Not pressed");
    mnemonic_btn1->clicked.connect([mnemonic_label1]() {
        mnemonic_label1->set_text("Save pressed!");
    });

    auto* mnemonic_btn2 = mnemonic_row->emplace_child<ui::button>("");
    mnemonic_btn2->set_mnemonic_text("&Cancel (Alt+C)");
    auto* mnemonic_label2 = mnemonic_row->emplace_child<ui::label>("Not pressed");
    mnemonic_btn2->clicked.connect([mnemonic_label2]() {
        mnemonic_label2->set_text("Cancel pressed!");
    });

    // ========== SIGNAL/SLOT PATTERN SECTION ==========
    auto* signal_section = row2->emplace_child<ui::group_box>();
    signal_section->set_title("Signal/Slot Pattern");
    signal_section->set_vbox_layout(onyxui::spacing::tiny);

    signal_section->emplace_child<ui::label>("Decoupled Communication:");
    signal_section->emplace_child<ui::label>("  - Signals: Event notifications (clicked, value_changed)");
    signal_section->emplace_child<ui::label>("  - Slots: Handler functions connected to signals");
    signal_section->emplace_child<ui::label>("  - Multiple connections: One signal, many handlers");
    signal_section->emplace_child<ui::label>("  - Scoped connections: RAII-based automatic cleanup");

    signal_section->emplace_child<ui::label>("");  // Spacer
    signal_section->emplace_child<ui::label>("Common Signals:");
    signal_section->emplace_child<ui::label>("  - button::clicked() - Button was clicked");
    signal_section->emplace_child<ui::label>("  - checkbox::toggled(bool) - Checkbox state changed");
    signal_section->emplace_child<ui::label>("  - slider::value_changed(int) - Slider value changed");
    signal_section->emplace_child<ui::label>("  - line_edit::text_changed(string) - Text modified");
    signal_section->emplace_child<ui::label>("  - window::closed() - Window was closed");
    signal_section->emplace_child<ui::label>("  - tab_widget::current_changed(int) - Tab switched");

    // Tips
    content->emplace_child<ui::label>("");  // Spacer
    auto* tips_section = content->emplace_child<ui::group_box>();
    tips_section->set_title("Tips & Best Practices");
    tips_section->set_vbox_layout(onyxui::spacing::tiny);

    tips_section->emplace_child<ui::label>("Event Handling:");
    tips_section->emplace_child<ui::label>("  - Return true from handle_event() to stop propagation");
    tips_section->emplace_child<ui::label>("  - Use CAPTURE phase for pre-processing (e.g., focus)");
    tips_section->emplace_child<ui::label>("  - Use TARGET phase for normal handling");
    tips_section->emplace_child<ui::label>("  - Use BUBBLE phase for container-level handling");

    tips_section->emplace_child<ui::label>("");  // Spacer
    tips_section->emplace_child<ui::label>("Focus Management:");
    tips_section->emplace_child<ui::label>("  - set_focusable(true) to allow focus");
    tips_section->emplace_child<ui::label>("  - set_focus() to programmatically focus a widget");
    tips_section->emplace_child<ui::label>("  - Tab order follows widget tree order by default");

    tips_section->emplace_child<ui::label>("");  // Spacer
    tips_section->emplace_child<ui::label>("Hotkeys:");
    tips_section->emplace_child<ui::label>("  - Keep actions alive with shared_ptr in member variable");
    tips_section->emplace_child<ui::label>("  - Use semantic actions for scheme-independent behavior");
    tips_section->emplace_child<ui::label>("  - Document all application hotkeys for discoverability");

    // Suppress unused variable warnings
    (void)focus_btn2;
    (void)focus_checkbox;

    return tab;
}

} // namespace widgets_demo_tabs
