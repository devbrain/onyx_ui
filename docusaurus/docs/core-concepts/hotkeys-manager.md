---
sidebar_position: 9
---

# Hotkeys Manager

The `hotkey_manager` provides comprehensive keyboard shortcut management with support for single keys, modifier-only activation, multi-key sequences (chords), and customizable hotkey schemes.

## Overview

The hotkey system supports multiple UI paradigms:
- **Single-key hotkeys** - Traditional shortcuts (Ctrl+S for Save)
- **Modifier-only activation** - QBasic-style (Alt alone opens menu)
- **Multi-key sequences** - Emacs-style chords (Ctrl+X, Ctrl+C)
- **Modal sequences** - Vim-style commands (dd, gg)
- **Hotkey schemes** - Customizable key bindings (Windows vs Norton Commander)

## Architecture

### Key Components

```cpp
#include <onyxui/hotkeys/key_sequence.hh>     // Basic key representation
#include <onyxui/hotkeys/key_chord.hh>        // Multi-key sequences
#include <onyxui/hotkeys/hotkey_manager.hh>   // Main manager
#include <onyxui/hotkeys/hotkey_scheme.hh>    // Customizable schemes
#include <onyxui/hotkeys/hotkey_action.hh>    // Semantic actions
```

### Priority System

The hotkey manager processes keyboard events in this priority order:

1. **Modifier-only activation** (highest) - Single modifier keys
2. **Multi-key chords** - Emacs/Vim style sequences
3. **Framework semantic actions** - Menu navigation (F10, arrows)
4. **Element-scoped actions** - Widget-specific shortcuts
5. **Global application actions** (lowest) - App-wide shortcuts

## Basic Usage

### Traditional Hotkeys

```cpp
// Single-key hotkey (Ctrl+S for Save)
auto save_action = std::make_shared<action<Backend>>();
save_action->set_text("&Save");
save_action->set_shortcut('s', key_modifier::ctrl);
save_action->triggered.connect([]() {
    save_document();
});

manager.register_action(save_action);
```

### Modifier-Only Activation (QBasic Style)

```cpp
// Alt key alone activates menu (like MS-DOS apps)
manager.register_modifier_activation(
    key_modifier::alt,
    [&menu_bar]() {
        menu_bar->activate();
        // Immediate feedback when Alt is pressed
    }
);
```

### Multi-Key Sequences (Emacs Style)

```cpp
// Ctrl+X, Ctrl+C to exit (Emacs style)
manager.register_chord(
    make_emacs_chord({
        {'x', key_modifier::ctrl},
        {'c', key_modifier::ctrl}
    }),
    []() {
        if (confirm_exit()) {
            std::exit(0);
        }
    }
);

// Ctrl+X, Ctrl+S to save
manager.register_chord(
    make_emacs_chord({
        {'x', key_modifier::ctrl},
        {'s', key_modifier::ctrl}
    }),
    []() { save_document(); }
);
```

### Modal Sequences (Vim Style)

```cpp
// dd to delete current line
manager.register_chord(
    make_vim_chord("dd"),
    []() { delete_current_line(); }
);

// gg to go to top
manager.register_chord(
    make_vim_chord("gg"),
    []() { go_to_top(); }
);

// 2dd to delete 2 lines (with numeric prefix)
// This requires more complex parsing - see advanced usage
```

## Hotkey Schemes

Hotkey schemes allow users to choose their preferred keyboard layout:

### Using Schemes

```cpp
// Access scheme registry
auto& schemes = ctx.hotkey_schemes();

// List available schemes
for (const auto& name : schemes.get_scheme_names()) {
    std::cout << "Scheme: " << name << "\n";
}

// Set current scheme
schemes.set_current_scheme("Windows");      // F10 for menu
schemes.set_current_scheme("Norton Commander"); // F9 for menu
```

### Built-in Schemes

| Scheme | Menu Activation | Navigation | Exit |
|--------|----------------|------------|------|
| Windows | F10 | Arrow keys | Alt+F4 |
| Norton Commander | F9 | Arrow keys | F10 |
| Minimal | None | Arrow keys | Esc |

### Creating Custom Schemes

```cpp
hotkey_scheme custom;
custom.name = "My Custom Scheme";
custom.description = "Personalized key bindings";

// Map semantic actions to keys
custom.set_binding(hotkey_action::activate_menu_bar,
                   parse_key_sequence("F12"));
custom.set_binding(hotkey_action::menu_down,
                   parse_key_sequence("Down"));
custom.set_binding(hotkey_action::menu_select,
                   parse_key_sequence("Enter"));

// Register the scheme
registry.register_scheme(std::move(custom));
```

## Semantic Actions

Framework-level actions that widgets can implement:

```cpp
enum class hotkey_action : uint8_t {
    // Menu navigation
    activate_menu_bar,   // Open/close menu (F10 or F9)
    menu_left,          // Navigate left in menu
    menu_right,         // Navigate right in menu
    menu_up,            // Navigate up in dropdown
    menu_down,          // Navigate down in dropdown
    menu_select,        // Select menu item (Enter)
    menu_cancel,        // Close menu (Esc)

    // Focus management
    focus_next,         // Tab to next widget
    focus_previous,     // Shift+Tab to previous

    // Dialog actions
    dialog_ok,          // Confirm (Enter)
    dialog_cancel,      // Cancel (Esc)

    // Standard actions
    undo,              // Ctrl+Z
    redo,              // Ctrl+Y or Ctrl+Shift+Z
    cut,               // Ctrl+X
    copy,              // Ctrl+C
    paste,             // Ctrl+V
    select_all,        // Ctrl+A
};
```

### Registering Semantic Handlers

```cpp
// Widgets register handlers for semantic actions
hotkeys->register_semantic_action(
    hotkey_action::activate_menu_bar,
    [this]() {
        if (m_menu_open) {
            close_menu();
        } else {
            open_menu(0);
        }
    }
);
```

## Key Representation

### Key Sequences

```cpp
// ASCII keys (normalized to lowercase)
key_sequence save{'s', key_modifier::ctrl};  // Ctrl+S

// Function keys
key_sequence help{1};  // F1
key_sequence menu{10}; // F10

// Combined modifiers
key_sequence save_as{'s',
    key_modifier::ctrl | key_modifier::shift};  // Ctrl+Shift+S

// Special keys (arrows use negative codes)
key_sequence up{static_cast<char>(-1)};    // Up arrow
key_sequence down{static_cast<char>(-2)};  // Down arrow
```

### Parsing and Formatting

```cpp
// Parse from string
auto key = parse_key_sequence("Ctrl+S");
auto f10 = parse_key_sequence("F10");
auto arrow = parse_key_sequence("Down");

// Format for display
std::string display = format_key_sequence(key); // "Ctrl+S"
```

## Backend Integration

### Event Traits

Backends must implement event traits for hotkey support:

```cpp
template<>
struct event_traits<MyEvent> {
    // Required for HotkeyCapable concept
    static char to_ascii(const MyEvent& e);
    static int to_f_key(const MyEvent& e);
    static int to_special_key(const MyEvent& e);

    // Modifier detection
    static bool ctrl_pressed(const MyEvent& e);
    static bool alt_pressed(const MyEvent& e);
    static bool shift_pressed(const MyEvent& e);
};
```

### Static Assertions

Backends should verify their assumptions at compile time:

```cpp
// Example from conio backend
static_assert(TB_KEY_F1 > TB_KEY_F12,
    "termbox2 F-key ordering assumption violated");
static_assert(TB_KEY_F1 - TB_KEY_F12 == 11,
    "termbox2 F-key spacing assumption violated");
```

## Advanced Features

### Chord State Machine

The `chord_matcher` class provides a state machine for tracking multi-key sequences:

```cpp
chord_matcher matcher;

// Process first key
if (matcher.process_key(seq1, chord)) {
    // Chord complete
} else if (matcher.has_partial_match()) {
    // Waiting for next key
    show_status("Ctrl+X-");  // Visual feedback
}

// Timeout handling (configurable per chord)
chord.timeout_ms = std::chrono::milliseconds{1000};
```

### Conflict Resolution

```cpp
// Set conflict policy
manager.set_conflict_policy(conflict_policy::warn);

// Policies:
// - allow: Last registered wins
// - warn: Log warning but allow
// - error: Reject conflicting registration
```

### Scope Management

```cpp
// Register hotkey for specific widget scope
manager.register_action(action, hotkey_scope::element, widget_ptr);

// Scopes:
// - global: Works everywhere
// - window: Active window only
// - element: Specific widget and descendants
```

## Testing

The framework includes comprehensive tests:

```cpp
// Backend-specific F-key tests
TEST_CASE("F-key ordering assumptions") {
    CHECK(TB_KEY_F1 > TB_KEY_F12);  // termbox2 reverse order
    CHECK(traits::to_f_key(f10_event) == 10);
}

// Chord matching tests
TEST_CASE("Multi-key sequence matching") {
    chord_matcher matcher;
    CHECK_FALSE(matcher.process_key(ctrl_x, chord));  // Partial
    CHECK(matcher.has_partial_match());
    CHECK(matcher.process_key(ctrl_c, chord));  // Complete
}
```

## Complete Example

```cpp
#include <onyxui/ui_context.hh>
#include <onyxui/hotkeys/hotkey_manager.hh>
#include <onyxui/widgets/menu_bar.hh>

class TextEditor {
public:
    TextEditor() : ctx(), manager() {
        setup_hotkeys();
    }

private:
    void setup_hotkeys() {
        // QBasic-style: Alt opens menu
        manager.register_modifier_activation(
            key_modifier::alt,
            [this]() { menu_bar->activate(); }
        );

        // Emacs-style save: Ctrl+X, Ctrl+S
        manager.register_chord(
            make_emacs_chord({
                {'x', key_modifier::ctrl},
                {'s', key_modifier::ctrl}
            }),
            [this]() { save_file(); }
        );

        // Traditional: Ctrl+S also saves
        auto save = std::make_shared<action<Backend>>();
        save->set_shortcut('s', key_modifier::ctrl);
        save->triggered.connect([this]() { save_file(); });
        manager.register_action(save);

        // Vim-style: :w to save (in command mode)
        if (vim_mode_active) {
            manager.register_chord(
                make_vim_chord(":w"),
                [this]() { save_file(); }
            );
        }
    }

    scoped_ui_context<Backend> ctx;
    hotkey_manager<Backend> manager;
    std::unique_ptr<menu_bar<Backend>> menu_bar;
};
```

## Best Practices

1. **Use semantic actions** for framework-level shortcuts
2. **Respect user preferences** with hotkey schemes
3. **Provide visual feedback** for partial chord matches
4. **Document shortcuts** in menus and tooltips
5. **Test backend assumptions** with static assertions
6. **Handle conflicts gracefully** with clear policies
7. **Support multiple paradigms** to accommodate different users

## See Also

- [Event System](./event-system.md)
- [Focus Manager](./focus-manager.md)
- [UI Context](./ui-context.md)
- [Service Locator](./service-locator.md)