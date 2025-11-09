---
sidebar_position: 10
---

# Window

The `onyxui::window` is a draggable, resizable window widget with title bar, control buttons, and support for modal dialogs.

## Overview

A window is a top-level UI container that provides a complete windowing experience with title bar, minimize/maximize/close buttons, drag-to-move, edge resizing, and modal support. Windows automatically register with the `window_manager` service for lifecycle tracking.

## Key Features

- **Title Bar:** Customizable title with optional menu/minimize/maximize/close buttons
- **Draggable:** Click and drag title bar to move window
- **Resizable:** Drag from edges (N, S, E, W) or corners (NE, NW, SE, SW)
- **Window States:** Normal, minimized, maximized with state transitions
- **Modal Support:** Block background windows and manage focus automatically
- **System Menu:** Optional menu with Restore, Move, Size, Minimize, Maximize, Close
- **Scrollable Content:** Optional scroll_view wrapper for large content
- **Focus Management:** Visual indicators and automatic focus tracking
- **Themable:** Full theme integration with focused/unfocused states
- **Signals:** Comprehensive event notifications for lifecycle, state, position, size, and focus

## Usage

### Basic Window

```cpp
#include <onyxui/widgets/window/window.hh>
#include <onyxui/widgets/vbox.hh>
#include <onyxui/widgets/label.hh>

// Create window with default flags
auto win = std::make_unique<window<Backend>>("My Window");

// Add content
auto content = std::make_unique<vbox<Backend>>(5);
content->emplace_child<label>("Hello, Window!");
win->set_content(std::move(content));

// Position and show
win->set_position(100, 50);
win->set_size(600, 400);
win->show();
```

### Custom Configuration

```cpp
// Configure window features
window<Backend>::window_flags flags;
flags.has_title_bar = true;
flags.has_menu_button = true;        // System menu
flags.has_minimize_button = true;
flags.has_maximize_button = true;
flags.has_close_button = true;
flags.is_resizable = true;
flags.is_movable = true;
flags.is_scrollable = false;
flags.is_modal = false;

// Size constraints
flags.min_width = 200;
flags.min_height = 100;
flags.max_width = 1024;              // 0 = unlimited
flags.max_height = 768;
flags.resize_border_width = 4;

auto win = std::make_unique<window<Backend>>("Configured Window", flags);
```

### Modal Dialog

```cpp
// Configure as modal
window<Backend>::window_flags flags;
flags.is_modal = true;
flags.has_minimize_button = false;
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

## Window States

Windows support three states via the `window_state` enum:

```cpp
enum class window_state : uint8_t {
    normal,      // Standard window state
    minimized,   // Hidden from view
    maximized    // Fills parent container
};
```

### State Transitions

```cpp
// Minimize window
win->minimize();

// Maximize window
win->maximize();

// Restore to normal
win->restore();

// Query current state
auto state = win->get_state();
if (state == window<Backend>::window_state::minimized) {
    win->restore();
}
```

## Resize Handles

Windows can be resized from eight positions via the `resize_handle` enum:

```cpp
enum class resize_handle : uint8_t {
    none,        // Not on a resize handle
    north,       // Top edge
    south,       // Bottom edge
    east,        // Right edge
    west,        // Left edge
    north_east,  // Top-right corner
    north_west,  // Top-left corner
    south_east,  // Bottom-right corner
    south_west   // Bottom-left corner
};
```

## API Reference

### Template Parameters

- `Backend`: The backend traits class (e.g., `conio_backend`, `test_canvas_backend`)

### Constructor

```cpp
explicit window(
    std::string title = "",
    window_flags flags = {},
    ui_element<Backend>* parent = nullptr
);
```

**Parameters:**
- `title`: Window title text
- `flags`: Window configuration (buttons, resizing, modal, etc.)
- `parent`: Parent UI element (nullptr for top-level)

### Public Methods

#### State Management

- **`void minimize()`:** Minimize window (hides from view)
- **`void maximize()`:** Maximize window (fills parent container)
- **`void restore()`:** Restore window from minimized or maximized state
- **`void close()`:** Close window (emits closing/closed signals)
- **`window_state get_state() const noexcept`:** Get current window state

#### Position and Size

- **`void set_position(int x, int y)`:** Set window position
- **`void set_size(int width, int height)`:** Set window size
- **`rect_type get_normal_bounds() const noexcept`:** Get normal (non-maximized) bounds

#### Content Management

- **`void set_content(std::unique_ptr<ui_element<Backend>> content)`:** Set window content widget
- **`ui_element<Backend>* get_content() noexcept`:** Get content widget pointer

#### Title

- **`void set_title(const std::string& title)`:** Set window title
- **`const std::string& get_title() const noexcept`:** Get window title

#### Flags and State

- **`bool is_modal() const noexcept`:** Check if window is modal
- **`const window_flags& get_flags() const noexcept`:** Get window configuration flags

#### Focus Management

- **`void set_focus(bool has_focus)`:** Set window focus state
- **`bool has_focus() const noexcept`:** Get window focus state

#### Display

- **`void show()`:** Show window (adds to layer_manager)
- **`void show_modal()`:** Show window as modal (blocks other windows)
- **`void hide()`:** Hide window (removes from layer_manager)

#### Testing

- **`window_system_menu<Backend>* get_system_menu() noexcept`:** Get system menu (for testing)

### Window Flags

The `window_flags` struct configures window features:

```cpp
struct window_flags {
    // Title bar
    bool has_title_bar = true;          // Show title bar
    bool has_menu_button = false;       // System menu button
    bool has_minimize_button = true;    // Minimize button
    bool has_maximize_button = true;    // Maximize button
    bool has_close_button = true;       // Close button

    // Interaction
    bool is_resizable = true;           // Resize from edges/corners
    bool is_movable = true;             // Drag by title bar
    bool is_scrollable = false;         // Wrap content in scroll_view
    bool is_modal = false;              // Block other windows
    bool dim_background = false;        // Dim background when modal

    // Size constraints
    int min_width = 100;                // Minimum window width
    int min_height = 50;                // Minimum window height
    int max_width = 0;                  // Maximum width (0 = no limit)
    int max_height = 0;                 // Maximum height (0 = no limit)
    int resize_border_width = 4;        // Resize handle size
};
```

### Signals

Windows emit the following signals:

#### Lifecycle

- **`signal<> closing`:** Emitted before close (can be used to cancel in future)
- **`signal<> closed`:** Emitted after window closed

#### State Changes

- **`signal<> minimized_sig`:** Emitted after minimize
- **`signal<> maximized_sig`:** Emitted after maximize
- **`signal<> restored_sig`:** Emitted after restore

#### Position and Size

- **`signal<> moved`:** Emitted after position changes
- **`signal<> resized_sig`:** Emitted after size changes

#### Focus

- **`signal<> focus_gained`:** Emitted when window gains focus
- **`signal<> focus_lost`:** Emitted when window loses focus

### Signal Usage

```cpp
// Lifecycle
win->closing.connect([]() {
    std::cout << "Window is closing\n";
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
    std::cout << "Window restored\n";
});

// Position/size
win->moved.connect([]() {
    std::cout << "Window moved\n";
});

win->resized_sig.connect([]() {
    std::cout << "Window resized\n";
});

// Focus
win->focus_gained.connect([]() {
    std::cout << "Window gained focus\n";
});

win->focus_lost.connect([]() {
    std::cout << "Window lost focus\n";
});
```

## Keyboard Interaction

Windows support keyboard interaction via semantic actions:

### Hotkeys

| Action | Default Key (Windows) | Norton Commander |
|--------|----------------------|------------------|
| Close window | Alt+F4 | Alt+F3 |
| Window menu | Alt+Space | Alt+Space |
| Minimize | - | - |
| Maximize | - | - |

### Focus and Navigation

1. **Tab Navigation:** Windows are focusable and participate in tab order
2. **Click to Focus:** Clicking a window automatically requests focus
3. **Modal Auto-Focus:** Modal windows automatically request focus when shown
4. **Focus Restoration:** Closing modal restores focus to previous active window

## Theming

The window's appearance is controlled by the `window_style` in the theme:

### Theme Properties

```cpp
struct window_style {
    // Title bar (focused)
    state_style title_focused;

    // Title bar (unfocused)
    state_style title_unfocused;

    // Window border
    color_type border_color_focused;
    color_type border_color_unfocused;
    box_style border_focused;
    box_style border_unfocused;

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

Windows automatically change appearance based on focus state:

- **Focused Window:**
  - Uses `theme.window.title_focused` colors (e.g., blue)
  - Uses `theme.window.border_focused` style (e.g., double border)

- **Unfocused Window:**
  - Uses `theme.window.title_unfocused` colors (e.g., gray)
  - Uses `theme.window.border_unfocused` style (e.g., single border)

### Customizing Theme

```cpp
// Apply theme at root
root->apply_theme("Norton Blue", ctx.themes());

// All windows inherit theme automatically
auto win = std::make_unique<window<Backend>>("Themed Window");
```

## Integration with Services

### Window Manager

Windows automatically register with the `window_manager` service:

```cpp
// Registration happens in constructor
auto win = std::make_unique<window<Backend>>("Title");

// Access window manager
auto* mgr = ui_services<Backend>::windows();

// Window is registered
auto all_windows = mgr->get_all_windows();  // Contains win

// Unregistration happens in destructor
// No manual cleanup needed
```

### Layer Manager

Windows integrate with the `layer_manager` for rendering and Z-order:

```cpp
// Show as non-modal (layer_type::dialog)
win->show();

// Show as modal (layer_type::modal with event blocking)
win->show_modal();

// Hide removes from layer_manager
win->hide();
```

### Focus Manager

Windows request focus automatically:

```cpp
// Auto-focus on click (handled in event system)
// Auto-focus on show_modal()
// Focus restoration on modal close

// Manual focus control
auto* input = ui_services<Backend>::input();
input->set_focus(win);
```

## Advanced Usage

### Custom Window Subclass

```cpp
template<UIBackend Backend>
class custom_window : public window<Backend> {
public:
    explicit custom_window(std::string title)
        : window<Backend>(std::move(title))
    {
        // Custom initialization
    }

protected:
    // Override close behavior
    void on_close() override {
        // Custom cleanup before close
        window<Backend>::on_close();
    }

    // Override event handling
    bool handle_event(const ui_event& event, event_phase phase) override {
        // Custom event logic
        return window<Backend>::handle_event(event, phase);
    }
};
```

### Scrollable Window

```cpp
// Enable automatic scrolling
window_flags flags;
flags.is_scrollable = true;

auto win = std::make_unique<window<Backend>>("Scrollable", flags);

// Content automatically wrapped in scroll_view
// Scrollbars appear when content exceeds window size
```

### Modal Lifecycle

```cpp
// Modal windows manage focus automatically:

// 1. show_modal() saves previous active window
auto dialog = std::make_shared<window<Backend>>("Dialog", modal_flags);
dialog->show_modal();
// - Saves current active window
// - Sets dialog as active
// - Requests focus for dialog
// - Blocks events to background

// 2. close() restores previous focus
dialog->close();
// - Restores previous active window
// - Restores focus to previous window
// - Removes from layer_manager
// - Emits closed signal
```

## Related Components

- **window_title_bar** - Draggable title bar widget
- **window_content_area** - Content container (optionally scrollable)
- **window_system_menu** - System menu popup
- **window_list_dialog** - Turbo Vision-style window list
- **window_manager** - Service for tracking all windows

## See Also

- [Window System Guide](../../guides/window-system.md) - Complete guide with examples
- [Dialog Widget](./dialog.md) - Result-based dialogs with OK/Cancel patterns
- [Scroll View](./scroll-view.md) - Scrollable content container
- [Button](./button.md) - Buttons for dialog actions
