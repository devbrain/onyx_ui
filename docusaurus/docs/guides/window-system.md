---
sidebar_position: 5
---

# Window System

OnyxUI provides a complete Turbo Vision-style windowing system with draggable, resizable windows, modal dialogs, focus management, and keyboard navigation.

## Overview

The window system includes:

```
┌─────────────────────────────────────────┐
│  ┌─ Window Title ─────────── [_][□][X] │  ← Title bar with controls
│  ├─────────────────────────────────────┤
│  │                                      │
│  │         Content Area                 │  ← Scrollable content
│  │                                      │
│  │                                      │
│  └─────────────────────────────────────┘
└─────────────────────────────────────────┘
   ↑                                     ↑
   └─ Resize from edges/corners ────────┘
```

### Components

1. **window** - Main windowing widget with title bar and content area
2. **window_title_bar** - Draggable title bar with control buttons
3. **window_content_area** - Content container (optionally scrollable)
4. **window_system_menu** - System menu (Restore, Move, Size, etc.)
5. **window_manager** - Service for tracking and managing all windows
6. **window_list_dialog** - Turbo Vision-style window list (Ctrl+W)

## Quick Start

The easiest way to create a window:

```cpp
#include <onyxui/widgets/window/window.hh>
#include <onyxui/widgets/vbox.hh>
#include <onyxui/widgets/label.hh>

// Create window with default settings
auto win = std::make_unique<window<Backend>>("My Application");

// Add content
auto content = std::make_unique<vbox<Backend>>(5);
content->emplace_child<label>("Hello, Window System!");
win->set_content(std::move(content));

// Show window
win->set_position(50, 10);
win->set_size(600, 400);
win->show();
```

### Window Flags

Customize window features via `window_flags`:

```cpp
window<Backend>::window_flags flags;

// Title bar and buttons
flags.has_title_bar = true;          // Show title bar
flags.has_menu_button = true;        // System menu button
flags.has_minimize_button = true;    // Minimize button
flags.has_maximize_button = true;    // Maximize/restore button
flags.has_close_button = true;       // Close button

// Interaction
flags.is_resizable = true;           // Resize from edges/corners
flags.is_movable = true;             // Drag by title bar
flags.is_scrollable = false;         // Wrap content in scroll_view
flags.is_modal = false;              // Block background windows

// Size constraints
flags.min_width = 200;
flags.min_height = 100;
flags.max_width = 0;                 // 0 = unlimited
flags.max_height = 0;
flags.resize_border_width = 4;       // Hit area size

auto win = std::make_unique<window<Backend>>("Title", flags);
```

## Core Features

### Window States

Windows support three states:

```cpp
// Minimize - hide from view
win->minimize();

// Maximize - fill parent container
win->maximize();

// Restore - return to normal size
win->restore();

// Query state
auto state = win->get_state();
if (state == window<Backend>::window_state::minimized) {
    // Window is minimized
}
```

### Modal Windows

Modal windows block interaction with background windows:

```cpp
// Configure as modal
window<Backend>::window_flags flags;
flags.is_modal = true;
flags.has_minimize_button = false;  // Usually don't minimize dialogs
flags.has_maximize_button = false;
flags.is_resizable = false;

auto dialog = std::make_shared<window<Backend>>("Confirm", flags);

// Add content
auto content = std::make_unique<vbox<Backend>>(5);
content->emplace_child<label>("Are you sure?");

auto* ok_btn = content->template emplace_child<button>("OK");
ok_btn->clicked.connect([dialog]() {
    dialog->close();
});

dialog->set_content(std::move(content));
dialog->show_modal();  // Auto-focuses and blocks background
```

**Modal Behavior:**
- Automatically saves previous active window
- Requests focus on show
- Blocks events to underlying windows
- Restores focus to previous window on close

### Dragging and Resizing

Windows can be moved and resized interactively:

```cpp
// Dragging
// - Click and drag title bar to move window
// - Only works if flags.is_movable = true

// Resizing
// - Drag from edges (N, S, E, W) or corners (NE, NW, SE, SW)
// - Only works if flags.is_resizable = true
// - Size constraints (min/max) are enforced automatically

// Programmatic control
win->set_position(100, 50);
win->set_size(800, 600);

// Listen for changes
win->moved.connect([]() {
    std::cout << "Window moved\n";
});

win->resized_sig.connect([]() {
    std::cout << "Window resized\n";
});
```

### System Menu

The system menu provides window operations:

```cpp
// Enable system menu
flags.has_menu_button = true;

auto win = std::make_unique<window<Backend>>("Title", flags);

// System menu appears when clicking menu button (≡)
// Contains:
// - Restore (if maximized/minimized)
// - Move (if movable)
// - Size (if resizable)
// - Minimize (if has minimize button)
// - Maximize (if has maximize button)
// - Close (if has close button)

// Menu items auto-disable based on current state
```

## Window Manager Service

The `window_manager` service tracks all windows:

```cpp
#include <onyxui/services/ui_services.hh>

// Get window manager instance
auto* mgr = ui_services<Backend>::windows();

// Enumerate windows
auto all = mgr->get_all_windows();
auto visible = mgr->get_visible_windows();
auto minimized = mgr->get_minimized_windows();
auto modals = mgr->get_modal_windows();

// Track active window
auto* active = mgr->get_active_window();
mgr->set_active_window(some_window);

// Window count
size_t count = mgr->get_window_count();
```

### Window List Dialog

Show Turbo Vision-style window list:

```cpp
// Show window list (usually triggered by Ctrl+W)
mgr->show_window_list();
```

Dialog displays:
```
┌─ Windows ─────────────────────────┐
│ [1] Editor - document.txt         │
│ [2] Calculator                    │
│ [3] Settings (minimized)          │
│ [4] Help (modal)                  │
│ [5] File Manager (maximized)      │
└───────────────────────────────────┘
```

**Navigation:**
- Arrow keys to select
- Enter to activate/restore window
- Escape to close dialog

### Window Cycling

Cycle through visible windows:

```cpp
// Cycle to next window (Ctrl+F6 in Norton Commander)
mgr->cycle_next_window();

// Cycle to previous window (Ctrl+Shift+F6)
mgr->cycle_previous_window();
```

### Custom Minimize Handler

Override minimize behavior for taskbar integration:

```cpp
// Set custom handler
mgr->set_minimize_handler([](window<Backend>* win) {
    my_taskbar->add_minimized_window(win);
});

// Clear handler (restore default)
mgr->clear_minimize_handler();

// Check if handler is set
if (mgr->has_custom_minimize_handler()) {
    // Custom handler active
}
```

## Keyboard Navigation

The window system integrates with semantic actions:

### Hotkey Bindings

| Action | Windows Scheme | Norton Commander |
|--------|---------------|------------------|
| Close window | Alt+F4 | Alt+F3 |
| Window menu | Alt+Space | Alt+Space |
| Show window list | Ctrl+W | Ctrl+W |
| Next window | Ctrl+Tab | Ctrl+F6 |
| Previous window | Ctrl+Shift+Tab | Ctrl+Shift+F6 |
| Move window | Ctrl+F7 | Ctrl+F7 |
| Resize window | Ctrl+F8 | Ctrl+F8 |

### Registering Hotkeys

```cpp
// Window manager registers hotkeys automatically
auto* mgr = ui_services<Backend>::windows();
mgr->register_hotkeys();

// Hotkeys respect current scheme
auto* scheme_reg = ui_services<Backend>::hotkey_schemes();
scheme_reg->set_active_scheme("Norton Commander");
```

## Window Signals

Windows emit signals for lifecycle events:

```cpp
// Lifecycle
win->closing.connect([]() {
    std::cout << "Window closing (can cancel in future)\n";
});

win->closed.connect([]() {
    std::cout << "Window closed\n";
});

// State changes
win->minimized_sig.connect([]() {
    std::cout << "Window minimized\n";
});

win->maximized_sig.connect([]() {
    std::cout << "Window maximized\n";
});

win->restored_sig.connect([]() {
    std::cout << "Window restored to normal\n";
});

// Position/size
win->moved.connect([]() {
    // Window position changed
});

win->resized_sig.connect([]() {
    // Window size changed
});

// Focus
win->focus_gained.connect([]() {
    std::cout << "Window gained focus\n";
});

win->focus_lost.connect([]() {
    std::cout << "Window lost focus\n";
});
```

## Theming

Windows integrate with the theme system:

### Theme Properties

```cpp
// Window theme structure
struct window_style {
    // Title bar (focused)
    state_style title_focused;

    // Title bar (unfocused)
    state_style title_unfocused;

    // Window border
    color_type border_color_focused;
    color_type border_color_unfocused;
    box_style border_focused;        // e.g., double_border
    box_style border_unfocused;      // e.g., single_border

    // Content area
    color_type content_background;

    // Dimensions
    int title_bar_height = 1;
    int border_width = 1;

    // Shadow (for menus)
    shadow_style shadow;
};
```

### Focus Visual Indicators

Windows automatically change appearance based on focus:

```cpp
// Focused window uses:
// - theme.window.title_focused (blue title bar)
// - theme.window.border_focused (double border)

// Unfocused window uses:
// - theme.window.title_unfocused (gray title bar)
// - theme.window.border_unfocused (single border)
```

## Advanced Features

### Scrollable Content

Enable automatic scrolling:

```cpp
window_flags flags;
flags.is_scrollable = true;

auto win = std::make_unique<window<Backend>>("Scrollable", flags);

// Content automatically wrapped in scroll_view
// Scrollbars appear when content exceeds window size
```

### Z-Order Management

Windows stack in layer_manager:

```cpp
// Show as non-modal (layer_type::dialog)
win->show();

// Show as modal (layer_type::modal)
win->show_modal();

// Z-order is automatic based on layer type
// Modal windows always appear above non-modal
```

### Window Manager Signals

Listen to window lifecycle events:

```cpp
auto* mgr = ui_services<Backend>::windows();

mgr->window_registered.connect([](window<Backend>* win) {
    std::cout << "Window registered: " << win->get_title() << "\n";
});

mgr->window_unregistered.connect([](window<Backend>* win) {
    std::cout << "Window unregistered\n";
});

mgr->window_minimized.connect([](window<Backend>* win) {
    std::cout << "Window minimized: " << win->get_title() << "\n";
});

mgr->window_restored.connect([](window<Backend>* win) {
    std::cout << "Window restored: " << win->get_title() << "\n";
});
```

## Complete Example

```cpp
#include <onyxui/ui_context.hh>
#include <onyxui/widgets/window/window.hh>
#include <onyxui/widgets/vbox.hh>
#include <onyxui/widgets/button.hh>
#include <onyxui/widgets/label.hh>

int main() {
    // Create UI context
    scoped_ui_context<Backend> ctx;

    // Apply theme
    auto root = std::make_unique<panel<Backend>>();
    root->apply_theme("Norton Blue", ctx.themes());

    // Configure main window
    window<Backend>::window_flags main_flags;
    main_flags.has_menu_button = true;
    main_flags.is_resizable = true;
    main_flags.min_width = 400;
    main_flags.min_height = 300;

    auto main_win = std::make_unique<window<Backend>>("My Application", main_flags);

    // Add content
    auto content = std::make_unique<vbox<Backend>>(5);
    content->emplace_child<label>("Welcome to the window system!");

    auto* dialog_btn = content->template emplace_child<button>("Show Dialog");
    dialog_btn->clicked.connect([&]() {
        // Create modal dialog
        window<Backend>::window_flags dialog_flags;
        dialog_flags.is_modal = true;
        dialog_flags.has_minimize_button = false;
        dialog_flags.has_maximize_button = false;
        dialog_flags.is_resizable = false;

        auto dialog = std::make_shared<window<Backend>>("Confirmation", dialog_flags);

        auto dialog_content = std::make_unique<vbox<Backend>>(5);
        dialog_content->emplace_child<label>("Are you sure?");

        auto* yes_btn = dialog_content->template emplace_child<button>("Yes");
        yes_btn->clicked.connect([dialog]() {
            std::cout << "User clicked Yes\n";
            dialog->close();
        });

        auto* no_btn = dialog_content->template emplace_child<button>("No");
        no_btn->clicked.connect([dialog]() {
            std::cout << "User clicked No\n";
            dialog->close();
        });

        dialog->set_content(std::move(dialog_content));
        dialog->set_size(400, 200);
        dialog->show_modal();
    });

    main_win->set_content(std::move(content));
    main_win->set_position(50, 10);
    main_win->set_size(600, 400);
    main_win->show();

    // Register window manager hotkeys
    auto* mgr = ui_services<Backend>::windows();
    if (mgr) {
        mgr->register_hotkeys();  // Enables Ctrl+W, Ctrl+F6, etc.
    }

    // Main loop...
    return 0;
}
```

## Best Practices

### Resource Management

```cpp
// Use unique_ptr for owned windows
auto win = std::make_unique<window<Backend>>("Title");

// Use shared_ptr for modal dialogs
auto dialog = std::make_shared<window<Backend>>("Dialog", modal_flags);

// Windows automatically register/unregister with window_manager
// No manual cleanup needed
```

### Focus Management

```cpp
// Focus is automatic - just show windows
win1->show();  // Gets focus
win2->show();  // Steals focus from win1

// Explicitly set active window if needed
auto* mgr = ui_services<Backend>::windows();
mgr->set_active_window(win1);
```

### Modal Dialogs

```cpp
// Always connect close handler before showing
dialog->closed.connect([dialog]() {
    // Handle dialog result
});

dialog->show_modal();  // Blocks until closed
```

## Troubleshooting

### Window Not Visible

```cpp
// Check size
win->set_size(600, 400);

// Check visibility
win->show();  // Not show_modal() for non-modal

// Check state
if (win->get_state() == window<Backend>::window_state::minimized) {
    win->restore();
}
```

### Modal Not Blocking

```cpp
// Ensure modal flag is set
flags.is_modal = true;

// Use show_modal(), not show()
dialog->show_modal();
```

### No Focus

```cpp
// Ensure window is focusable
win->set_focusable(true);

// Request focus explicitly
auto* input = ui_services<Backend>::input();
input->set_focus(win);
```

## See Also

- [Dialog Widget](../api-reference/widgets/dialog.md) - Result-based dialogs with OK/Cancel patterns
- [Scrolling System](./scrolling-system.md) - Scrollable content in windows
- [Theme Development](./theme-development.md) - Customizing window appearance
- [Widget Development](./widget-development.md) - Creating custom window types
