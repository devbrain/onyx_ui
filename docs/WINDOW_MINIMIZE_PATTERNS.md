# Minimized Window Patterns - Cross-Framework Analysis

**Date:** 2025-11-08
**Purpose:** Research how different UI frameworks handle minimized windows to inform OnyxUI design

---

## Desktop Windowing Systems

### 1. **Windows (Win32, WPF, WinForms)**

**Approach:** Taskbar Integration

```
Minimized State:
- Window hidden from screen
- Icon appears in taskbar
- Window still exists in memory
- Can be restored by clicking taskbar icon
- Alt+Tab shows minimized windows
- Task manager shows all windows (minimized or not)
```

**Key Characteristics:**
- ✅ Centralized location (taskbar) for all minimized windows
- ✅ Visual preview on hover (Windows 7+)
- ✅ Grouped by application
- ✅ Persistent across virtual desktops
- ❌ Requires taskbar widget
- ❌ Complex interaction model

**Implementation:**
```cpp
// Win32 API
ShowWindow(hwnd, SW_MINIMIZE);  // Hide and iconify
ShowWindow(hwnd, SW_RESTORE);   // Restore from minimized

// WPF
window.WindowState = WindowState.Minimized;
window.WindowState = WindowState.Normal;
```

---

### 2. **macOS (Cocoa, AppKit)**

**Approach:** Dock + Application Hide

```
Minimize (Cmd+M):
- Window slides into Dock with animation
- Icon shows in right side of Dock (separate from app icon)
- Window thumbnail on hover
- Click to restore

Hide Application (Cmd+H):
- All application windows hide
- No Dock icons (just app icon)
- Reactivate app to restore
```

**Key Characteristics:**
- ✅ Two-tier approach (minimize vs hide app)
- ✅ Beautiful minimize animation (genie effect)
- ✅ Dock provides visual preview
- ✅ Per-window minimize to Dock
- ❌ Dock can get cluttered
- ❌ Requires Dock widget

**Implementation:**
```objc
// Cocoa/AppKit
[window miniaturize:nil];  // Minimize to Dock
[window deminiaturize:nil]; // Restore

// Or programmatically
window.isMiniaturized = YES;
```

---

### 3. **X11 / Linux Window Managers**

**Approach:** Varies by Window Manager

**GNOME (Modern):**
```
- Window hidden completely
- Shows in "Activities" overview
- Shows in Alt+Tab switcher
- No taskbar by default (uses top bar activities)
- Can add extensions for taskbar
```

**KDE Plasma:**
```
- Window hidden
- Icon in taskbar panel
- Similar to Windows approach
- Configurable behavior
```

**i3/Sway (Tiling WMs):**
```
- No traditional minimize
- Windows moved to "scratchpad"
- Scratchpad is a hidden workspace
- Keyboard shortcut to toggle scratchpad windows
```

**Key Characteristics:**
- ✅ Highly configurable
- ✅ Some WMs have innovative approaches (scratchpad)
- ❌ No standard behavior
- ❌ Inconsistent UX across distros

---

## UI Frameworks (Cross-Platform)

### 4. **Qt Framework**

**Approach:** Platform-Native + Optional Widget

```cpp
// Platform-native minimize
window->setWindowState(Qt::WindowMinimized);
window->setWindowState(Qt::WindowNoState);  // Restore

// Custom minimize (stays in app)
window->hide();  // Just hide
window->show();  // Show again

// Or with system tray
QSystemTrayIcon *tray = new QSystemTrayIcon(icon);
window->hide();  // Minimize to tray instead
```

**Key Characteristics:**
- ✅ Follows platform conventions (taskbar on Windows, Dock on macOS)
- ✅ Fallback to simple hide() for custom behavior
- ✅ System tray as alternative
- ✅ Flexible - can override default behavior
- ❌ Behavior differs per platform

**Custom Minimize Example:**
```cpp
// Qt: Custom "minimize" to internal list
class WindowManager : public QObject {
    QList<QWidget*> minimizedWindows;

    void minimizeWindow(QWidget* window) {
        window->hide();
        minimizedWindows.append(window);
        updateTaskList();  // Custom taskbar widget
    }

    void restoreWindow(QWidget* window) {
        window->show();
        minimizedWindows.removeOne(window);
        updateTaskList();
    }
};
```

---

### 5. **GTK (GIMP Toolkit)**

**Approach:** Platform-Native + Window List

```c
// GTK 3/4
gtk_window_iconify(window);   // Minimize (platform-specific)
gtk_window_deiconify(window); // Restore

// Or hide completely
gtk_widget_hide(widget);
gtk_widget_show(widget);
```

**Key Characteristics:**
- ✅ Platform-native behavior on Windows/macOS
- ✅ Respects window manager on Linux
- ✅ Can track minimized state
- ❌ No built-in taskbar widget (relies on platform)

---

### 6. **Electron (Chromium-Based)**

**Approach:** Platform-Native + Tray

```javascript
// Electron
win.minimize();  // Platform-native minimize
win.restore();   // Restore

// Or minimize to tray
win.on('minimize', (event) => {
    event.preventDefault();
    win.hide();  // Hide instead of minimize
    tray.displayBalloon({
        title: 'App minimized',
        content: 'Running in background'
    });
});

// Restore from tray
tray.on('click', () => {
    win.show();
});
```

**Key Characteristics:**
- ✅ Platform-native minimize available
- ✅ Easy to override with tray behavior
- ✅ Popular pattern: minimize = hide to tray
- ❌ Tray icons can be confusing for users

---

## Terminal UI Frameworks

### 7. **ncurses / dialog / whiptail**

**Approach:** No True Minimize

```
Traditional TUI behavior:
- Windows are modal or stacked
- No minimize concept
- Background windows are "covered" not "minimized"
- Can switch between windows with keys (like screen/tmux)
```

**Key Characteristics:**
- ✅ Simple model (no minimize needed)
- ❌ Limited to terminal constraints
- ❌ No persistent window list

---

### 8. **tmux / GNU Screen**

**Approach:** Window List / Session Management

```bash
# tmux approach
Ctrl+b c    # Create new window
Ctrl+b n/p  # Next/previous window
Ctrl+b w    # List all windows (like taskbar)
Ctrl+b &    # Close window

# Windows shown in status bar at bottom:
[0:bash] [1:vim*] [2:htop-]
          ^current  ^activity
```

**Key Characteristics:**
- ✅ Status bar shows all "windows" (like taskbar)
- ✅ Keyboard-driven switching
- ✅ Activity indicators
- ✅ Clean, text-based approach
- ❌ Not traditional minimize (windows always exist)

---

### 9. **Turbo Vision (Borland)**

**Approach:** Desktop Manager + Window List

```
Desktop Manager:
┌─ Windows ────────┐
│ 1. Editor        │
│ 2. Calculator    │
│ 3. File Manager  │
│ 4. [Minimized]   │  ← Shows as item in list
└──────────────────┘

Features:
- Alt+0 shows window list
- Minimized windows shown in list with icon
- Can restore by selecting from list
- Windows can be "closed to desktop" (minimized)
```

**Key Characteristics:**
- ✅ Simple window list (no taskbar needed)
- ✅ Keyboard-driven (Alt+0, Alt+1-9 for direct access)
- ✅ Clear visual indication of minimized state
- ✅ Fits terminal UI constraints
- ✅ **This is the classic DOS/TUI approach**

---

### 10. **Midnight Commander (mc) / Far Manager**

**Approach:** No True Windows (Panels)

```
- Two-panel file manager (not multi-window)
- Background jobs list instead of minimized windows
- F9 menu shows running operations
- More of a task manager than window manager
```

**Key Characteristics:**
- ✅ Simple (no window management complexity)
- ❌ Not applicable (different paradigm)

---

### 11. **Norton Commander / DOS Navigator**

**Approach:** Limited Multi-Window

```
Norton Commander:
- Two fixed panels (not movable/minimizable)
- Background viewer can be "hidden" (Ctrl+O)
- Temporary screens pushed to background
- F12 toggles between screens

DOS Navigator:
- Desktop with icons for minimized windows
- Windows minimize to desktop icons
- Click icon to restore
- Similar to Windows 3.1 approach
```

**Key Characteristics:**
- ✅ Desktop icons for minimized windows
- ✅ Visual metaphor (icon = minimized window)
- ✅ Clean when minimized (just icon)
- ❌ Requires icon rendering
- ❌ Desktop can get cluttered

---

## Web-Based / Embedded Frameworks

### 12. **jQuery UI / Kendo UI (Window Widgets)**

**Approach:** JavaScript Window Management

```javascript
// jQuery UI
$("#window").dialog("minimize");  // Custom method
$("#window").dialog("restore");

// Implementation:
minimize: function() {
    this.element.hide();
    this.uiDialog.hide();
    // Add to taskbar widget
    taskbar.addMinimizedWindow(this);
}
```

**Approaches vary:**
1. **Hide + Taskbar Widget** (most common)
2. **Minimize to title bar only** (collapse content)
3. **Icon in corner** (like desktop icons)
4. **No minimize** (just close/hide)

---

## Pattern Summary

### Common Approaches

| Pattern | Used By | Description | Pros | Cons |
|---------|---------|-------------|------|------|
| **Taskbar Integration** | Windows, KDE | Minimized = icon in taskbar | Centralized, familiar | Requires taskbar widget |
| **Dock Integration** | macOS | Slide to dock with preview | Beautiful, visual | Requires dock widget |
| **Window List Dialog** | Turbo Vision | Alt+0 shows all windows | Simple, keyboard-driven | Extra step to restore |
| **Desktop Icons** | DOS Navigator, Win 3.1 | Minimize to icon on desktop | Visual, clickable | Can clutter desktop |
| **Hide to Tray** | Electron apps | Minimize = hide to system tray | Clean main UI | Tray can be confusing |
| **Scratchpad** | i3/Sway | Hidden workspace for minimized | Powerful, keyboard-driven | Complex for beginners |
| **Simple Hide** | Many frameworks | Just hide the window | Simple implementation | No visual indicator |
| **Status Bar List** | tmux/screen | Show all windows in status bar | Always visible, compact | Limited space |

---

## Recommendations for OnyxUI

### Option 1: **Window List Dialog (Turbo Vision Style)** ⭐ RECOMMENDED

**Implementation:**
```cpp
class window_list_dialog : public dialog<Backend> {
    // Shows all windows with state indicators
    // [1] Editor.txt
    // [2] Calculator
    // [3] Settings (minimized)  ← Special indicator
    // [4] Help
};

// Hotkey: Alt+0 or Ctrl+W
void show_window_list() {
    auto list = std::make_unique<window_list_dialog<Backend>>();
    list->show_modal();
    // User selects window to restore
}

// Window minimize implementation
void window::minimize() {
    m_state = window_state::minimized;
    this->set_visible(false);  // Hide from rendering
    minimized.emit();

    // Window stays in layer_manager but hidden
    // Window list dialog can enumerate all windows
}
```

**Pros:**
- ✅ Clean, text-based approach (fits TUI paradigm)
- ✅ No taskbar widget needed
- ✅ Keyboard-driven (Alt+0 to see all windows)
- ✅ Simple to implement
- ✅ Minimal screen space usage
- ✅ Follows Turbo Vision/DOS Navigator conventions
- ✅ Works well with existing layer_manager

**Cons:**
- ⚠️ Extra step to see minimized windows (must open dialog)
- ⚠️ No always-visible indicator

---

### Option 2: **Status Bar Integration**

**Implementation:**
```cpp
class status_bar : public widget<Backend> {
    // Right side shows window count
    // "Windows: 3 (1 minimized)"

    // Or clickable indicators
    // [Win1] [Win2] [Win3*]  ← * = minimized

    // Click to show window list
};
```

**Pros:**
- ✅ Always visible
- ✅ Compact
- ✅ Similar to tmux/screen approach
- ✅ No popup needed (click status bar)

**Cons:**
- ⚠️ Requires status bar widget
- ⚠️ Limited space for many windows
- ⚠️ May clutter status bar

---

### Option 3: **Desktop Icons (DOS Navigator Style)**

**Implementation:**
```cpp
// When minimized, window becomes an icon on desktop
class desktop_manager : public widget<Backend> {
    std::vector<window_icon<Backend>*> m_minimized_icons;

    void add_minimized_window(window<Backend>* win) {
        auto icon = std::make_unique<window_icon<Backend>>(win);
        icon->set_position(next_icon_position());
        m_minimized_icons.push_back(icon.get());
        add_child(std::move(icon));
    }
};

class window_icon : public button<Backend> {
    // Small button with window title
    // Click to restore window
};
```

**Pros:**
- ✅ Visual metaphor (icon = minimized window)
- ✅ Direct restore (click icon)
- ✅ Familiar from Windows 3.1/DOS Navigator

**Cons:**
- ⚠️ Desktop can get cluttered
- ⚠️ Requires desktop background widget
- ⚠️ Icon positioning logic needed

---

### Option 4: **Taskbar Widget (Windows/KDE Style)**

**Implementation:**
```cpp
class taskbar : public widget<Backend> {
    hbox<Backend>* m_window_buttons;

    void add_window(window<Backend>* win) {
        auto btn = std::make_unique<button<Backend>>(win->get_title());
        btn->clicked.connect([win]() {
            if (win->is_minimized()) {
                win->restore();
            }
            win->bring_to_front();
        });
        m_window_buttons->add_child(std::move(btn));
    }
};
```

**Pros:**
- ✅ Always visible
- ✅ Familiar to users
- ✅ Shows all windows (not just minimized)
- ✅ Can show window state visually

**Cons:**
- ⚠️ Requires taskbar widget
- ⚠️ Takes screen space
- ⚠️ Complex for many windows

---

### Option 5: **Simple State Tracking (No Visual)**

**Implementation:**
```cpp
void window::minimize() {
    m_state = window_state::minimized;
    this->set_visible(false);
    minimized.emit();

    // That's it - just hide and track state
    // Application manages restoration
}

// Application code
std::vector<window<Backend>*> minimized_windows;

void on_window_minimized(window<Backend>* win) {
    minimized_windows.push_back(win);
}

void restore_first_minimized() {
    if (!minimized_windows.empty()) {
        minimized_windows.front()->restore();
        minimized_windows.erase(minimized_windows.begin());
    }
}
```

**Pros:**
- ✅ Extremely simple
- ✅ No UI overhead
- ✅ Application has full control

**Cons:**
- ❌ No built-in way to restore
- ❌ No visual feedback
- ❌ User must remember minimized windows exist

---

## Recommendation: Hybrid Approach ⭐

**Combine Option 1 + Option 5:**

```cpp
// 1. Simple state tracking (always)
void window::minimize() {
    m_state = window_state::minimized;
    this->set_visible(false);
    minimized.emit();
}

// 2. Built-in window list dialog (keyboard shortcut)
class window_manager : public service {
    std::vector<window<Backend>*> m_all_windows;

    void show_window_list() {
        auto dialog = std::make_unique<window_list_dialog<Backend>>();

        for (auto* win : m_all_windows) {
            std::string label = win->get_title();
            if (win->is_minimized()) label += " (minimized)";
            if (win->is_maximized()) label += " (maximized)";

            dialog->add_item(label, [win]() {
                if (win->is_minimized()) win->restore();
                win->bring_to_front();
            });
        }

        dialog->show_modal();
    }

    // Register hotkey: Ctrl+W or Alt+0
    void register_hotkeys() {
        auto list_action = std::make_shared<action<Backend>>();
        list_action->set_shortcut(key_code::w, key_modifier::ctrl);
        list_action->triggered.connect([this]() {
            show_window_list();
        });
    }
};

// 3. Optional: Applications can add taskbar/status bar integration
```

**Why This Works:**
- ✅ Core is simple (minimize = hide + state tracking)
- ✅ Built-in window list for discoverability (Ctrl+W)
- ✅ Applications can optionally add taskbar/status bar
- ✅ Flexible - supports multiple UI patterns
- ✅ Keyboard-driven (TUI-friendly)
- ✅ No mandatory UI widgets
- ✅ Follows Turbo Vision conventions

---

## Implementation Plan Update

Add to Phase 4 (Window States):

```cpp
// Phase 4: Window States
// 1. Add window_state enum (normal, minimized, maximized)
// 2. Implement minimize() - sets state, hides window
// 3. Implement restore() - shows window, restores bounds
// 4. Add minimized/restored signals
// 5. Create window_list_dialog widget (simple list)
// 6. Register Ctrl+W hotkey for window list
// 7. Tests for state transitions

// Optional (later phase):
// 8. Create taskbar widget (example implementation)
// 9. Create status_bar integration example
// 10. Create desktop icon example
```

---

## Conclusion

**Recommended Approach for OnyxUI:**

**Primary:** Window List Dialog (Ctrl+W shows all windows)
**Secondary:** Simple hide/show state tracking
**Optional:** Applications can implement taskbar/status bar/desktop icons

**Rationale:**
1. Fits TUI paradigm (keyboard-driven, no GUI chrome)
2. Simple core implementation
3. Flexible for applications to extend
4. Follows classic TUI conventions (Turbo Vision)
5. No mandatory screen space usage
6. Works well with existing layer_manager architecture

---

**Status:** RECOMMENDATION COMPLETE
**Next Step:** Update WINDOW_SYSTEM_IMPLEMENTATION_PLAN.md with minimize decision
