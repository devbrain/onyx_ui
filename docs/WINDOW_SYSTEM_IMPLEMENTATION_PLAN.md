# Window System Implementation Plan

**Date:** 2025-11-08
**Status:** DESIGN PHASE
**Author:** Claude Code + User

---

## Executive Summary

Implement a comprehensive windowing system for OnyxUI that provides:
- Draggable, resizable windows with optional title bars
- Modal and non-modal window support
- Minimize/maximize/restore functionality
- Optional scrollable content
- Full theme integration
- Clean integration with existing layer_manager, focus_manager, and event systems

---

## Requirements Analysis

### User Requirements

1. ✅ **Optional and themeable title bar** with minimize/maximize icons
2. ✅ **Button for popup menu** (optional and themeable)
3. ✅ **Resizable and movable** windows
4. ✅ **Optionally scrollable** content
5. ✅ **Modal and non-modal** window support

### Derived Requirements

6. **Window state management** (normal, minimized, maximized, restored)
7. **Z-order management** (focus, bring-to-front)
8. **Close button** (standard window behavior)
9. **Theme integration** for all window components
10. **Event handling** for drag, resize, button clicks
11. **Keyboard support** (Alt+F4 close, Alt+Space menu, etc.)
12. **Content area** for arbitrary child widgets

---

## Architecture Design

### Component Hierarchy

```
window<Backend>
├── window_title_bar<Backend> (optional, composite widget)
│   ├── label (title text)
│   ├── button (menu - optional)
│   ├── button (minimize)
│   ├── button (maximize/restore)
│   └── button (close)
├── window_content_area<Backend>
│   └── scroll_view<Backend> (optional wrapper)
│       └── user content widget
└── window_resize_handle<Backend> (8 invisible hit areas)
    ├── top, bottom, left, right (edge resize)
    └── top-left, top-right, bottom-left, bottom-right (corner resize)

window_manager (service in ui_services)
├── Tracks all windows (minimized, normal, maximized)
├── Provides window enumeration
├── Handles Ctrl+W hotkey
└── Shows window_list_dialog

window_list_dialog<Backend> (Turbo Vision style)
├── Lists all windows with state indicators
├── Shows: "Window Title (minimized/maximized/modal)"
├── Keyboard navigation (arrows, Enter)
├── Filters (show all / only visible / only minimized)
└── Restore/activate on selection
```

### Class Structure

#### 1. **`window<Backend>`** - Main Window Widget
```cpp
template<UIBackend Backend>
class window : public widget_container<Backend> {
public:
    // Window states
    enum class window_state {
        normal,
        minimized,
        maximized
    };

    // Window flags
    struct window_flags {
        bool has_title_bar = true;
        bool has_menu_button = false;
        bool has_minimize_button = true;
        bool has_maximize_button = true;
        bool has_close_button = true;
        bool is_resizable = true;
        bool is_movable = true;
        bool is_scrollable = false;
        bool is_modal = false;
    };

    // Constructor
    explicit window(std::string title = "", window_flags flags = {});

    // Destructor (unregisters from window_manager)
    ~window();

    // State management
    void minimize();  // Hides window, registers as minimized in window_manager
    void maximize();  // Fills parent container, updates state
    void restore();   // Restores from minimized or maximized
    void close();     // Emits closing/closed signals, unregisters from window_manager
    window_state get_state() const;

    // Position and size
    void set_position(int x, int y);
    void set_size(int width, int height);
    rect_type get_normal_bounds() const;  // For restore

    // Content
    void set_content(std::unique_ptr<ui_element<Backend>> content);
    ui_element<Backend>* get_content();

    // Title
    void set_title(const std::string& title);
    const std::string& get_title() const;

    // Modal behavior
    void show_modal();  // Uses layer_manager modal layer
    void show();        // Uses layer_manager dialog layer
    void hide();        // Removes from layer_manager

    // Signals
    signal<> closing;        // Emitted before close (can cancel)
    signal<> closed;         // Emitted after close
    signal<> minimized;
    signal<> maximized;
    signal<> restored;
    signal<> moved;          // Emitted after position changes
    signal<> resized;        // Emitted after size changes
    signal<> focus_gained;
    signal<> focus_lost;

private:
    std::string m_title;
    window_flags m_flags;
    window_state m_state;
    rect_type m_normal_bounds;  // For restore from maximized
    rect_type m_before_minimize; // For restore from minimized

    // Child widgets
    std::unique_ptr<window_title_bar<Backend>> m_title_bar;
    std::unique_ptr<window_content_area<Backend>> m_content_area;
    std::vector<std::unique_ptr<window_resize_handle<Backend>>> m_resize_handles;

    // Window manager integration (auto-registers on construction)
    // Note: window_manager is accessed via ui_services<Backend>::window_manager()

    // Drag state
    bool m_is_dragging = false;
    point_type m_drag_start_pos;
    rect_type m_drag_start_bounds;

    // Resize state
    bool m_is_resizing = false;
    resize_direction m_resize_dir;
    point_type m_resize_start_pos;
    rect_type m_resize_start_bounds;
};
```

#### 2. **`window_title_bar<Backend>`** - Composite Title Bar
```cpp
template<UIBackend Backend>
class window_title_bar : public widget_container<Backend> {
public:
    explicit window_title_bar(std::string title, const window::window_flags& flags);

    // Title
    void set_title(const std::string& title);
    const std::string& get_title() const;

    // Button access (for connecting signals)
    button<Backend>* get_menu_button();
    button<Backend>* get_minimize_button();
    button<Backend>* get_maximize_button();
    button<Backend>* get_close_button();

    // Signals (forwarded from buttons)
    signal<> menu_clicked;
    signal<> minimize_clicked;
    signal<> maximize_clicked;
    signal<> close_clicked;
    signal<> drag_started;   // User clicked empty title bar area
    signal<int, int> dragging;  // Mouse delta while dragging

protected:
    void do_render(render_context<Backend>& ctx) const override;
    bool handle_event(const ui_event& event, event_phase phase) override;

private:
    label<Backend>* m_title_label;
    button<Backend>* m_menu_button;      // Optional
    button<Backend>* m_minimize_button;
    button<Backend>* m_maximize_button;
    button<Backend>* m_close_button;

    // Drag state
    bool m_dragging = false;
    point_type m_drag_start;
};
```

#### 3. **`window_content_area<Backend>`** - Content Container
```cpp
template<UIBackend Backend>
class window_content_area : public widget_container<Backend> {
public:
    explicit window_content_area(bool scrollable = false);

    void set_content(std::unique_ptr<ui_element<Backend>> content);
    ui_element<Backend>* get_content();

    void set_scrollable(bool scrollable);
    bool is_scrollable() const;

private:
    bool m_scrollable;
    scroll_view<Backend>* m_scroll_view;  // Only if scrollable
    ui_element<Backend>* m_content;
};
```

#### 4. **`window_resize_handle<Backend>`** - Resize Hit Areas
```cpp
enum class resize_direction : uint8_t {
    none = 0,
    top = 1,
    bottom = 2,
    left = 4,
    right = 8,
    top_left = top | left,
    top_right = top | right,
    bottom_left = bottom | left,
    bottom_right = bottom | right
};

template<UIBackend Backend>
class window_resize_handle : public widget<Backend> {
public:
    explicit window_resize_handle(resize_direction direction);

    resize_direction get_direction() const;

    // Signals
    signal<> resize_started;
    signal<int, int> resizing;  // Mouse delta while resizing
    signal<> resize_ended;

protected:
    bool handle_event(const ui_event& event, event_phase phase) override;

private:
    resize_direction m_direction;
    bool m_resizing = false;
    point_type m_resize_start;
};
```

#### 5. **`window_manager`** - Service for Window Tracking

```cpp
/**
 * @brief Service that tracks all windows and provides window list functionality
 *
 * @details
 * The window_manager is a ui_services service that maintains a registry of all
 * windows in the application. It provides:
 * - Window enumeration (all, visible, minimized)
 * - Window list dialog (Turbo Vision style)
 * - Ctrl+W hotkey registration
 * - Custom minimize behavior override support
 */
template<UIBackend Backend>
class window_manager {
public:
    // Register/unregister windows
    void register_window(window<Backend>* win);
    void unregister_window(window<Backend>* win);

    // Window enumeration
    std::vector<window<Backend>*> get_all_windows() const;
    std::vector<window<Backend>*> get_visible_windows() const;
    std::vector<window<Backend>*> get_minimized_windows() const;
    std::vector<window<Backend>*> get_modal_windows() const;

    // Window list dialog
    void show_window_list();  // Shows Turbo Vision style dialog

    // Custom minimize handler (override default behavior)
    void set_minimize_handler(std::function<void(window<Backend>*)> handler);
    void clear_minimize_handler();  // Restore default behavior

    // Hotkey registration
    void register_hotkeys();  // Ctrl+W for window list

    // Signals
    signal<window<Backend>*> window_registered;
    signal<window<Backend>*> window_unregistered;
    signal<window<Backend>*> window_minimized;
    signal<window<Backend>*> window_restored;

private:
    std::vector<window<Backend>*> m_windows;
    std::function<void(window<Backend>*)> m_custom_minimize_handler;
};
```

#### 6. **`window_list_dialog<Backend>`** - Turbo Vision Window List

```cpp
/**
 * @brief Turbo Vision style window list dialog
 *
 * @details
 * Modal dialog that shows all windows with state indicators.
 * Allows user to select window to activate/restore.
 *
 * Appearance:
 * ┌─ Windows ─────────────────────────┐
 * │ [1] Editor - document.txt         │
 * │ [2] Calculator                    │
 * │ [3] Settings (minimized)          │
 * │ [4] Help (modal)                  │
 * │ [5] File Manager (maximized)      │
 * └───────────────────────────────────┘
 */
template<UIBackend Backend>
class window_list_dialog : public dialog<Backend> {
public:
    enum class filter_mode {
        all,            // Show all windows
        visible_only,   // Show only visible (not minimized)
        minimized_only  // Show only minimized
    };

    explicit window_list_dialog(filter_mode filter = filter_mode::all);

    // Add window to list
    void add_window(window<Backend>* win);

    // Filter control
    void set_filter(filter_mode filter);
    filter_mode get_filter() const;

    // Signals
    signal<window<Backend>*> window_selected;  // User selected window

protected:
    void do_render(render_context<Backend>& ctx) const override;
    bool handle_event(const ui_event& event, event_phase phase) override;

private:
    std::vector<window<Backend>*> m_windows;
    filter_mode m_filter;
    int m_selected_index = 0;

    void refresh_list();  // Rebuild list based on filter
    std::string get_window_label(window<Backend>* win) const;
};
```

---

## Theme Integration

### Window Theme Structure

Add to `include/onyxui/theming/theme.hh`:

```cpp
/**
 * @brief Window styling configuration
 */
struct window_style {
    // Title bar (focused)
    color_type title_bg_focused;
    color_type title_fg_focused;
    box_style title_border_focused;

    // Title bar (unfocused)
    color_type title_bg_unfocused;
    color_type title_fg_unfocused;
    box_style title_border_unfocused;

    // Title bar dimensions
    int title_height = 24;
    int title_padding_horizontal = 8;
    int title_padding_vertical = 4;

    // Content area
    color_type content_bg;
    box_style content_border;

    // Window buttons (menu, minimize, maximize, close)
    button_style menu_button;
    button_style minimize_button;
    button_style maximize_button;
    button_style close_button;

    // Resize handles
    int resize_handle_size = 4;
    color_type resize_handle_hover;  // Visual feedback

    // Window border (when focused/unfocused)
    box_style window_border_focused;
    box_style window_border_unfocused;

    // Sizing
    int min_width = 200;
    int min_height = 100;
    int default_width = 600;
    int default_height = 400;
};
```

### Default Theme Values

```cpp
// Norton Blue theme window styling
window_style window = {
    .title_bg_focused = {0, 0, 170},      // Blue
    .title_fg_focused = {255, 255, 255},  // White
    .title_border_focused = box_style::single_border,

    .title_bg_unfocused = {128, 128, 128}, // Gray
    .title_fg_unfocused = {0, 0, 0},       // Black
    .title_border_unfocused = box_style::single_border,

    .content_bg = {255, 255, 255},  // White
    .content_border = box_style::single_border,

    .resize_handle_hover = {255, 255, 0},  // Yellow highlight

    .window_border_focused = box_style::double_border,
    .window_border_unfocused = box_style::single_border,
};
```

---

## Integration with Existing Systems

### 1. Window Manager Service (NEW)

The `window_manager` will be added to `ui_services<Backend>` as a new service:

```cpp
// In include/onyxui/services/ui_services.hh
template<UIBackend Backend>
class ui_services {
public:
    static window_manager<Backend>* window_manager();  // NEW
    // ... existing services ...
};
```

Windows automatically register/unregister on construction/destruction:

```cpp
template<UIBackend Backend>
window<Backend>::window(std::string title, window_flags flags)
    : m_title(std::move(title)), m_flags(flags), m_state(window_state::normal)
{
    // Auto-register with window manager
    auto* mgr = ui_services<Backend>::window_manager();
    if (mgr) {
        mgr->register_window(this);
    }

    // ... rest of initialization ...
}

template<UIBackend Backend>
window<Backend>::~window() {
    // Auto-unregister from window manager
    auto* mgr = ui_services<Backend>::window_manager();
    if (mgr) {
        mgr->unregister_window(this);
    }
}
```

### 2. Layer Manager Integration

```cpp
void window<Backend>::show() {
    auto* layers = ui_services<Backend>::layers();
    if (layers) {
        layer_type type = m_flags.is_modal ? layer_type::modal : layer_type::dialog;
        layers->add_layer(type, this, 100);  // z-index
    }
}

void window<Backend>::hide() {
    auto* layers = ui_services<Backend>::layers();
    if (layers) {
        layers->remove_layer(this);
    }
}
```

### 3. Focus Manager Integration

```cpp
bool window<Backend>::handle_event(const ui_event& event, event_phase phase) {
    // Request focus when clicked
    if (auto* mouse_evt = std::get_if<mouse_event>(&event)) {
        if (mouse_evt->act == mouse_event::action::press) {
            auto* input = ui_services<Backend>::input();
            if (input) {
                input->set_focus(this);
                focus_gained.emit();

                // Bring to front in layer manager
                auto* layers = ui_services<Backend>::layers();
                if (layers) {
                    layers->bring_to_front(this);
                }
            }
        }
    }

    return base::handle_event(event, phase);
}
```

### 4. Event Handling - Drag and Resize

```cpp
bool window<Backend>::handle_event(const ui_event& event, event_phase phase) {
    if (auto* mouse_evt = std::get_if<mouse_event>(&event)) {
        // Dragging
        if (m_is_dragging) {
            if (mouse_evt->act == mouse_event::action::move) {
                int dx = mouse_evt->x - m_drag_start_pos.x;
                int dy = mouse_evt->y - m_drag_start_pos.y;

                auto new_bounds = m_drag_start_bounds;
                rect_utils::offset(new_bounds, dx, dy);
                set_bounds(new_bounds);

                moved.emit();
                return true;
            }
            if (mouse_evt->act == mouse_event::action::release) {
                m_is_dragging = false;
                return true;
            }
        }

        // Resizing
        if (m_is_resizing) {
            if (mouse_evt->act == mouse_event::action::move) {
                int dx = mouse_evt->x - m_resize_start_pos.x;
                int dy = mouse_evt->y - m_resize_start_pos.y;

                auto new_bounds = calculate_resized_bounds(dx, dy, m_resize_dir);
                set_bounds(new_bounds);

                resized.emit();
                return true;
            }
            if (mouse_evt->act == mouse_event::action::release) {
                m_is_resizing = false;
                return true;
            }
        }
    }

    return base::handle_event(event, phase);
}
```

### 5. Keyboard Shortcuts

```cpp
void register_window_hotkeys() {
    auto* hotkeys = ui_services<Backend>::hotkeys();

    // Alt+F4 - Close focused window
    auto close_action = std::make_shared<action<Backend>>();
    close_action->set_shortcut(key_code::f4, key_modifier::alt);
    close_action->triggered.connect([this]() {
        this->close();
    });

    // Alt+Space - Show window menu
    auto menu_action = std::make_shared<action<Backend>>();
    menu_action->set_shortcut(key_code::space, key_modifier::alt);
    menu_action->triggered.connect([this]() {
        show_system_menu();
    });
}
```

---

## Implementation Phases

### Phase 1: Core Window Widget (Week 1)
**Goal:** Basic window with title bar and content area

**Files to Create:**
- `include/onyxui/widgets/window/window.hh`
- `include/onyxui/widgets/window/window_title_bar.hh`
- `include/onyxui/widgets/window/window_content_area.hh`

**Tasks:**
1. Create `window<Backend>` base class
2. Create `window_title_bar<Backend>` composite widget
3. Create `window_content_area<Backend>` container
4. Basic rendering (static, no dragging yet)
5. Title bar with close button
6. Content area with arbitrary widget

**Tests:**
- Window creation and rendering
- Title bar rendering with buttons
- Content area rendering
- Close button functionality

**Estimated Effort:** 2-3 days

---

### Phase 2: Dragging and Basic Positioning (Week 1-2)
**Goal:** Windows can be moved by dragging title bar

**Tasks:**
1. Implement drag state in `window_title_bar`
2. Mouse event handling (press, move, release)
3. Coordinate translation during drag
4. Bounds clamping (keep window on screen)
5. Signal emission (drag_started, dragging, drag_ended)

**Tests:**
- Drag window by title bar
- Window stays within screen bounds
- Release stops dragging
- Multiple windows don't interfere

**Estimated Effort:** 1-2 days

---

### Phase 3: Resize Handles (Week 2)
**Goal:** Windows can be resized from edges/corners

**Files to Create:**
- `include/onyxui/widgets/window/window_resize_handle.hh`

**Tasks:**
1. Create `window_resize_handle<Backend>` widget
2. Create 8 resize handles (4 edges + 4 corners)
3. Hit testing for resize handles
4. Resize state machine
5. Bounds calculation during resize
6. Min/max size constraints
7. Cursor changes (visual feedback)

**Tests:**
- Resize from each edge
- Resize from each corner
- Min/max size enforcement
- Resize while dragging

**Estimated Effort:** 2-3 days

---

### Phase 4: Window States and Window Manager (Week 2-3)
**Goal:** Minimize, maximize, restore functionality + window tracking service

**Files to Create:**
- `include/onyxui/services/window_manager.hh`
- `include/onyxui/widgets/window/window_list_dialog.hh`

**Tasks:**
1. Add minimize/maximize buttons to title bar
2. Implement state machine (normal, minimized, maximized)
3. Save/restore bounds when changing states
4. Maximize fills parent container
5. Create `window_manager` service for window tracking
6. Register/unregister windows with window_manager
7. Implement minimize behavior (window becomes invisible, tracked as minimized)
8. Create `window_list_dialog` (Turbo Vision style)
9. Restore button (replaces maximize when maximized)
10. Ctrl+W hotkey for window list dialog

**Tests:**
- Maximize window
- Restore from maximized
- Minimize window
- Window manager tracks minimized windows
- Window list dialog shows all windows
- Restore from minimized via dialog
- Ctrl+W shows window list
- State persistence

**Estimated Effort:** 3-4 days

---

### Phase 5: Modal Support (Week 3)
**Goal:** Modal and non-modal window support

**Tasks:**
1. Add `is_modal` flag to window
2. Integrate with `layer_manager::layer_type::modal`
3. Block events to windows below modal
4. Dim background behind modal (optional)
5. Enforce single modal at a time
6. Auto-focus modal window

**Tests:**
- Show modal window
- Events blocked to background
- Close modal restores focus
- Multiple non-modal windows work

**Estimated Effort:** 1-2 days

---

### Phase 6: Scrollable Content (Week 3)
**Goal:** Optional scrolling for window content

**Tasks:**
1. Add `is_scrollable` flag to window
2. Wrap content in `scroll_view` when scrollable
3. Automatic scrollbar visibility
4. Coordinate content size vs window size
5. Scroll position preservation on resize

**Tests:**
- Scrollable window with large content
- Resize updates scrollbars
- Non-scrollable window clips content
- Scroll position stable

**Estimated Effort:** 1 day

---

### Phase 7: Window Menu (Week 3-4)
**Goal:** Optional system menu (Restore, Move, Size, Minimize, Maximize, Close)

**Files to Create:**
- `include/onyxui/widgets/window/window_system_menu.hh`

**Tasks:**
1. Add menu button to title bar (optional)
2. Create system menu popup
3. Menu items trigger window actions
4. Alt+Space keyboard shortcut
5. Disable irrelevant items based on state

**Tests:**
- Menu button shows menu
- Menu items work
- Alt+Space shortcut
- Disabled items when maximized

**Estimated Effort:** 1-2 days

---

### Phase 8: Theme Integration (Week 4)
**Goal:** Complete theme customization

**Tasks:**
1. Add `window_style` to theme.hh
2. Implement `get_theme_style()` for window
3. Focus/unfocus color changes
4. Customize all window components
5. Default themes (Norton Blue, Windows 95, etc.)

**Tests:**
- Theme switching updates all windows
- Focus changes title bar color
- Buttons use window theme
- Custom themes work

**Estimated Effort:** 2-3 days

---

### Phase 9: Z-Order and Focus Management (Week 4)
**Goal:** Multiple windows with proper focus and stacking

**Tasks:**
1. Click to focus and bring to front
2. Tab switching between windows (Ctrl+F6 pattern)
3. Focus indicators (border changes)
4. Z-order preservation in layer_manager
5. Active window tracking

**Tests:**
- Click unfocused window brings to front
- Focus follows mouse clicks
- Keyboard focus switching
- Multiple windows maintain order

**Estimated Effort:** 2 days

---

### Phase 10: Keyboard Navigation (Week 4-5)
**Goal:** Full keyboard support for windows

**Tasks:**
1. Alt+F4 closes focused window
2. Alt+Space shows system menu
3. Alt+F9 minimizes
4. Alt+F10 maximizes
5. Ctrl+F6 cycles windows
6. Escape closes modal window

**Tests:**
- All hotkeys work
- Modal closes on Escape
- Window cycling
- Menu navigation

**Estimated Effort:** 1-2 days

---

### Phase 11: Helper Functions and Convenience API (Week 5)
**Goal:** Easy-to-use window creation patterns

**Files to Create:**
- `include/onyxui/widgets/window/window_presets.hh`

**Tasks:**
1. Create preset window configurations
2. Builder pattern for window creation
3. Common dialog types (message_box, input_dialog, etc.)
4. Examples and documentation

**Example API:**
```cpp
// Simple window
auto win = modern_window<Backend>("My App");
win->set_content(create_my_content());
win->show();

// Modal dialog
auto dialog = modal_dialog<Backend>("Save Changes?",
    "Do you want to save your changes before closing?");
dialog->show_modal();

// Scrollable window
auto scroll_win = scrollable_window<Backend>("Large Content");
scroll_win->set_content(large_content);
scroll_win->show();
```

**Estimated Effort:** 2-3 days

---

### Phase 12: Documentation and Examples (Week 5)
**Goal:** Complete user-facing documentation

**Files to Create:**
- `docusaurus/docs/guides/window-system.md`
- `docusaurus/docs/api-reference/widgets/window.md`
- `examples/window_demo.cc`

**Tasks:**
1. User guide with examples
2. API reference documentation
3. Demo application showcasing all features
4. Migration notes (if any breaking changes)

**Estimated Effort:** 2-3 days

---

## File Organization

```
include/onyxui/widgets/window/
├── window.hh                      # Main window widget
├── window_title_bar.hh            # Composite title bar
├── window_content_area.hh         # Content container
├── window_resize_handle.hh        # Resize hit areas
├── window_system_menu.hh          # System menu popup
├── window_list_dialog.hh          # Turbo Vision window list
└── window_presets.hh              # Convenience functions

include/onyxui/services/
└── window_manager.hh              # Window tracking service

docusaurus/docs/guides/
└── window-system.md               # User guide

docusaurus/docs/api-reference/widgets/
└── window.md                      # API reference

examples/
└── window_demo.cc                 # Complete demo

unittest/widgets/
├── test_window.cc                 # Core window tests
├── test_window_title_bar.cc       # Title bar tests
├── test_window_resize.cc          # Resize tests
├── test_window_modal.cc           # Modal behavior tests
└── test_window_list_dialog.cc     # Window list tests

unittest/services/
└── test_window_manager.cc         # Window manager tests
```

---

## Testing Strategy

### Unit Tests
- **Window creation** and destruction
- **Title bar** rendering and event handling
- **Drag** behavior with bounds
- **Resize** from all directions
- **State transitions** (minimize, maximize, restore)
- **Modal blocking** event propagation
- **Scrolling** content updates
- **Theme** application and switching
- **Focus** changes and z-order
- **Window manager** registration/unregistration
- **Window manager** enumeration (all, visible, minimized)
- **Window list dialog** rendering and navigation
- **Window list dialog** filtering modes
- **Ctrl+W hotkey** shows window list

### Integration Tests
- **Multiple windows** interaction
- **Modal over non-modal** stacking
- **Layer manager** integration
- **Focus manager** integration
- **Event routing** through window hierarchy
- **Theme switching** updates all windows
- **Window manager** tracks all windows correctly
- **Minimize/restore** through window list dialog
- **Custom minimize handler** override

### Visual Tests
- **Manual demo** application
- **Screenshot comparisons** for themes
- **Resize smoothness** (no flicker)
- **Drag smoothness**

---

## API Examples

### Basic Window
```cpp
// Create window
auto window = std::make_unique<window<Backend>>("My Application");

// Add content
auto content = std::make_unique<vbox<Backend>>();
content->emplace_child<label>("Hello, Window!");
content->emplace_child<button>("Click Me");

window->set_content(std::move(content));

// Show window
window->show();
```

### Modal Dialog
```cpp
// Create modal window
window::window_flags flags;
flags.is_modal = true;
flags.has_minimize_button = false;
flags.has_maximize_button = false;

auto dialog = std::make_unique<window<Backend>>("Save Changes?", flags);

// Add content
auto content = std::make_unique<vbox<Backend>>();
content->emplace_child<label>("Do you want to save your changes?");

auto buttons = content->emplace_child<hbox<Backend>>();
auto* yes_btn = buttons->emplace_child<button>("Yes");
auto* no_btn = buttons->emplace_child<button>("No");
auto* cancel_btn = buttons->emplace_child<button>("Cancel");

yes_btn->clicked.connect([&]() { dialog->close(); save(); });
no_btn->clicked.connect([&]() { dialog->close(); discard(); });
cancel_btn->clicked.connect([&]() { dialog->close(); });

dialog->set_content(std::move(content));
dialog->show_modal();
```

### Scrollable Window
```cpp
window::window_flags flags;
flags.is_scrollable = true;

auto window = std::make_unique<window<Backend>>("Large Content", flags);

// Add large content
auto content = std::make_unique<vbox<Backend>>();
for (int i = 0; i < 100; ++i) {
    content->emplace_child<label>("Item " + std::to_string(i));
}

window->set_content(std::move(content));
window->show();
```

### Window with Custom Menu
```cpp
window::window_flags flags;
flags.has_menu_button = true;

auto window = std::make_unique<window<Backend>>("My App", flags);

// Connect to menu button
auto* title_bar = window->get_title_bar();
title_bar->menu_clicked.connect([&]() {
    // Show custom menu
    auto menu = std::make_unique<menu<Backend>>();
    menu->add_item("Settings");
    menu->add_item("About");
    // ... show menu
});

window->show();
```

### Window List Dialog (Turbo Vision Style)
```cpp
// User presses Ctrl+W - automatically handled by window_manager
// Or programmatically show window list:
auto* mgr = ui_services<Backend>::window_manager();
if (mgr) {
    mgr->show_window_list();  // Shows Turbo Vision style dialog
}

// The dialog automatically lists all windows:
// ┌─ Windows ─────────────────────────┐
// │ [1] Editor - document.txt         │
// │ [2] Calculator                    │
// │ [3] Settings (minimized)          │
// │ [4] Help (modal)                  │
// │ [5] File Manager (maximized)      │
// └───────────────────────────────────┘

// User navigates with arrow keys, Enter restores/activates window
```

### Custom Minimize Handler
```cpp
// Override default minimize behavior (e.g., custom taskbar)
auto* mgr = ui_services<Backend>::window_manager();
if (mgr) {
    mgr->set_minimize_handler([](window<Backend>* win) {
        // Custom behavior: add to taskbar, show notification, etc.
        my_taskbar->add_minimized_window(win);
    });
}

// Restore default behavior
mgr->clear_minimize_handler();
```

---

## Timeline

**Total Estimated Time:** 4-5 weeks

### Week 1
- Phase 1: Core window widget (2-3 days)
- Phase 2: Dragging and positioning (1-2 days)

### Week 2
- Phase 3: Resize handles (2-3 days)
- Phase 4: Window states (2 days)

### Week 3
- Phase 5: Modal support (1-2 days)
- Phase 6: Scrollable content (1 day)
- Phase 7: Window menu (1-2 days)

### Week 4
- Phase 8: Theme integration (2-3 days)
- Phase 9: Z-order and focus (2 days)
- Phase 10: Keyboard navigation (1-2 days)

### Week 5
- Phase 11: Helper functions (2-3 days)
- Phase 12: Documentation (2-3 days)

---

## Dependencies

### External (Already Available)
- ✅ layer_manager (modal/dialog support)
- ✅ focus_manager (window focus)
- ✅ event system (drag/resize events)
- ✅ theming system (visual customization)
- ✅ scroll_view (scrollable content)
- ✅ button widget (title bar buttons)
- ✅ label widget (title text)
- ✅ menu widget (system menu)

### New (To Be Created)
- ⏳ window widget
- ⏳ window_title_bar widget
- ⏳ window_content_area widget
- ⏳ window_resize_handle widget
- ⏳ window_system_menu widget
- ⏳ window_list_dialog widget
- ⏳ window_manager service
- ⏳ window theme styles

---

## Risk Assessment

### Low Risk
- ✅ Layer manager integration (existing system)
- ✅ Basic rendering (well-understood patterns)
- ✅ Theme integration (established framework)

### Medium Risk
- ⚠️ Drag and resize smoothness (requires careful coordinate math)
- ⚠️ Z-order management with multiple windows
- ⚠️ Modal event blocking (ensure no leaks)

### High Risk
- ❌ Performance with many windows (need profiling)
- ❌ Complex resize constraints (aspect ratios, snapping)
- ❌ Window animations (optional, future enhancement)

---

## Future Enhancements (Out of Scope)

1. **Animations** (fade in/out, slide, etc.)
2. **Window snapping** (to screen edges, other windows)
3. **Taskbar** integration (minimized windows list)
4. **MDI** (Multiple Document Interface) support
5. **Tabbed windows** (browser-style tabs)
6. **Window transparency** (alpha channel support)
7. **Window shapes** (non-rectangular windows)
8. **Aspect ratio** locking during resize
9. **Custom title bar** content (beyond standard buttons)
10. **Window grouping** (application windows together)

---

## Design Decisions

1. ✅ **Architecture:** Component hierarchy with composite title bar + content area + resize handles
2. ✅ **API:** Intuitive creation with `window<Backend>("Title", flags)` pattern
3. ✅ **Theme Integration:** Comprehensive `window_style` in theme with focused/unfocused states
4. ⏳ **Modal Behavior:** Should modal windows dim the background? (Optional feature)
5. ✅ **Scrolling:** Window property (`is_scrollable` flag) that wraps content in scroll_view
6. ✅ **Minimize:** **Turbo Vision approach** - Window list dialog (Ctrl+W) + override support ⭐
7. ⏳ **Maximize:** Fill entire screen or parent container? (TBD)
8. ⏳ **System Menu:** Standard items or customizable? (TBD)
9. ✅ **Hotkeys:** Ctrl+W (window list), Alt+F4 (close), Alt+Space (menu)
10. ✅ **Z-Order:** Automatic on click (bring to front)

---

## Conclusion

This implementation plan provides a comprehensive, phased approach to implementing a full-featured windowing system for OnyxUI. The design leverages existing framework capabilities (layer management, theming, events) while adding specialized window-specific functionality.

**Recommended Starting Point:** Phase 1 (Core Window Widget)

**Next Steps:**
1. Review this plan with stakeholders
2. Get approval on architecture and API
3. Begin Phase 1 implementation
4. Iterate based on feedback

---

**Status:** AWAITING REVIEW AND APPROVAL

**Last Updated:** 2025-11-08
