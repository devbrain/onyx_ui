# Hotkey Framework - Quick Reference

## New Features (October 2025)

### Multi-Key Sequences (Chords)

```cpp
// Emacs-style: Ctrl+X, Ctrl+C to exit
ctx.hotkeys().register_chord(
    make_emacs_chord({
        {'x', key_modifier::ctrl},
        {'c', key_modifier::ctrl}
    }),
    []() { exit(0); }
);

// Vim-style: dd to delete line
ctx.hotkeys().register_chord(
    make_vim_chord("dd"),
    []() { delete_current_line(); }
);
```

### Modifier-Only Activation

```cpp
// Alt key alone opens menu (QBasic/MS-DOS style)
ctx.hotkeys().register_modifier_activation(
    key_modifier::alt,
    [&menu]() { menu->activate(); }
);
```

### Priority System

1. **Modifier-only** (highest) - Alt alone
2. **Multi-key chords** - Ctrl+X, Ctrl+C
3. **Semantic actions** - F10, arrows
4. **Element-scoped** - Widget shortcuts
5. **Global actions** (lowest) - App shortcuts

## Common Tasks

### Access Hotkey Manager

```cpp
// Via ui_context (most common)
scoped_ui_context<Backend> ctx;
auto* hotkeys = ctx.hotkeys();
auto* schemes = ctx.hotkey_schemes();

// Via service locator (from widgets)
if (auto* hotkeys = ui_services<Backend>::hotkeys()) {
    // Use hotkeys...
}
```

### Get Current Scheme

```cpp
auto* current_scheme = schemes->get_current_scheme();
if (current_scheme) {
    std::cout << "Current: " << current_scheme->name << "\n";
}

auto name = schemes->get_current_scheme_name();  // std::optional<string>
```

### Switch Schemes

```cpp
// List available schemes
auto names = schemes->get_scheme_names();  // ["Windows", "Norton Commander"]

// Switch to a scheme
if (schemes->set_current_scheme("Norton Commander")) {
    std::cout << "Switched to Norton Commander (F9 for menu)\n";
}
```

### Register Semantic Action Handler

```cpp
ctx.hotkeys().register_semantic_action(
    hotkey_action::activate_menu_bar,
    [&menu]() { menu->activate(); }
);

// Now F10 (Windows) or F9 (Norton) will trigger this handler
```

### All Semantic Actions

```cpp
enum class hotkey_action : uint8_t {
    // Menu navigation
    activate_menu_bar,   // F10 (Windows), F9 (Norton)
    menu_up,             // Up arrow
    menu_down,           // Down arrow
    menu_left,           // Left arrow
    menu_right,          // Right arrow
    menu_select,         // Enter
    menu_cancel,         // Escape

    // Focus management
    focus_next,          // Tab
    focus_previous,      // Shift+Tab

    // Dialog actions
    dialog_ok,          // Enter
    dialog_cancel,      // Escape

    // Standard actions
    undo,               // Ctrl+Z
    redo,               // Ctrl+Y or Ctrl+Shift+Z
    cut,                // Ctrl+X
    copy,               // Ctrl+C
    paste,              // Ctrl+V
    select_all          // Ctrl+A
};
```

### Create Custom Scheme

```cpp
hotkey_scheme custom;
custom.name = "My Custom Scheme";
custom.description = "Custom keyboard layout";

// Set bindings
custom.set_binding(hotkey_action::activate_menu_bar, parse_key_sequence("F12"));
custom.set_binding(hotkey_action::menu_up, parse_key_sequence("k"));
custom.set_binding(hotkey_action::menu_down, parse_key_sequence("j"));
custom.set_binding(hotkey_action::menu_left, parse_key_sequence("h"));
custom.set_binding(hotkey_action::menu_right, parse_key_sequence("l"));
custom.set_binding(hotkey_action::menu_select, parse_key_sequence("Enter"));
custom.set_binding(hotkey_action::menu_cancel, parse_key_sequence("Escape"));
custom.set_binding(hotkey_action::focus_next, parse_key_sequence("Tab"));
custom.set_binding(hotkey_action::focus_previous, parse_key_sequence("Shift+Tab"));

// Register
schemes->register_scheme(std::move(custom));
schemes->set_current_scheme("My Custom Scheme");
```

### Key Sequence Parsing

```cpp
// Simple keys
auto s = parse_key_sequence("s");           // → {key:'s', modifiers:none}
auto ctrl_s = parse_key_sequence("Ctrl+S"); // → {key:'s', modifiers:ctrl}

// Modifiers
auto combo = parse_key_sequence("Ctrl+Shift+A");  // Multiple modifiers

// F-keys
auto f10 = parse_key_sequence("F10");  // → {f_key:10}

// Special keys
auto up = parse_key_sequence("Up");        // Arrow keys
auto enter = parse_key_sequence("Enter");
auto esc = parse_key_sequence("Escape");
auto tab = parse_key_sequence("Tab");
auto shift_tab = parse_key_sequence("Shift+Tab");

// Format back to string
std::string str = format_key_sequence(ctrl_s);  // → "Ctrl+S"
```

### Chord Patterns

```cpp
// Emacs-style with timeout
auto chord = make_emacs_chord({
    {'x', key_modifier::ctrl},
    {'s', key_modifier::ctrl}
});
chord.timeout_ms = std::chrono::milliseconds{2000};  // Custom timeout

// Vim-style modal sequences
auto chord = make_vim_chord("gg");  // Go to top
auto chord = make_vim_chord("dd");  // Delete line
auto chord = make_vim_chord(":w");  // Save command

// QBasic-style modifier-only
auto chord = make_modifier_chord(key_modifier::alt);

// Check chord state
chord_matcher matcher;
if (!matcher.process_key(seq, chord)) {
    if (matcher.has_partial_match()) {
        show_status("Ctrl+X-");  // Visual feedback
    }
}
```

### Query Scheme Bindings

```cpp
auto* scheme = schemes->get_scheme("Windows");
if (scheme) {
    // Get key for action
    auto key = scheme->get_binding(hotkey_action::activate_menu_bar);
    if (key) {
        std::cout << "Menu activates with: " << format_key_sequence(*key) << "\n";
    }

    // Find action for key
    auto action = scheme->find_action_for_key(parse_key_sequence("F10"));
    if (action) {
        std::cout << "F10 triggers: " << to_string(*action) << "\n";
    }

    // Check if action has binding
    bool has = scheme->has_binding(hotkey_action::menu_up);
}
```

### Unregister Semantic Action

```cpp
// Remove handler
ctx.hotkeys().unregister_semantic_action(hotkey_action::activate_menu_bar);

// Check if handler exists
bool has_handler = ctx.hotkeys().has_semantic_handler(hotkey_action::menu_down);
```

### Application Actions (Still Work!)

```cpp
// Old application action system still works unchanged
auto save_action = std::make_shared<action<Backend>>();
save_action->set_shortcut('s', key_modifier::ctrl);
save_action->triggered.connect([]() { save_document(); });
ctx.hotkeys().register_action(save_action);

// Priority: Semantic actions checked FIRST, then application actions
```

## Built-In Schemes

### Windows Scheme
- **F10**: Activate menu bar
- **Up/Down/Left/Right**: Navigate menus
- **Enter**: Select item
- **Escape**: Cancel/close
- **Tab**: Focus next
- **Shift+Tab**: Focus previous

### Norton Commander Scheme
- **F9**: Activate menu bar (main difference from Windows)
- **Up/Down/Left/Right**: Navigate menus
- **Enter**: Select item
- **Escape**: Cancel/close
- **Tab**: Focus next
- **Shift+Tab**: Focus previous

## Common Patterns

### Settings Dialog

```cpp
void show_keyboard_settings() {
    auto* schemes = ui_services<Backend>::hotkey_schemes();

    // Get available schemes
    auto names = schemes->get_scheme_names();

    // Current selection
    auto current = schemes->get_current_scheme_name();

    // User selects from dropdown
    schemes->set_current_scheme(user_selected_name);
}
```

### Menu Widget Integration

```cpp
class menu_bar : public widget<Backend> {
    void initialize() {
        auto* hotkeys = ui_services<Backend>::hotkeys();
        if (hotkeys) {
            hotkeys->register_semantic_action(
                hotkey_action::activate_menu_bar,
                [this]() { this->activate(); }
            );
        }
    }
};
```

### Minimal Implementation

```cpp
// No handlers registered - mouse-only UI still works!
scoped_ui_context<Backend> ctx;
// Schemes registered but no handlers
// User can only use mouse (graceful fallback)
```

### Progressive Enhancement

```cpp
// Start minimal
scoped_ui_context<Backend> ctx;

// Add keyboard support incrementally
ctx.hotkeys().register_semantic_action(
    hotkey_action::menu_select,
    [&]() { /* handle Enter */ }
);

// Add more as needed
ctx.hotkeys().register_semantic_action(
    hotkey_action::menu_cancel,
    [&]() { /* handle Escape */ }
);
```

## Error Handling

```cpp
// No binding? Returns nullopt (not an error!)
auto key = scheme->get_binding(hotkey_action::some_action);
if (!key) {
    // No keyboard shortcut - mouse still works
}

// No handler? Returns false (graceful fallback)
bool handled = hotkeys->handle_key_event(event);
if (!handled) {
    // Event not handled - widget receives keyboard event
}

// Invalid scheme name? Returns false
if (!schemes->set_current_scheme("NonExistent")) {
    std::cerr << "Scheme not found\n";
}
```

## Testing

```cpp
TEST_CASE("Hotkey scheme test") {
    hotkey_scheme_registry registry;
    registry.register_scheme(builtin_hotkey_schemes::windows());

    hotkey_manager<Backend> manager(&registry);

    bool called = false;
    manager.register_semantic_action(
        hotkey_action::activate_menu_bar,
        [&]() { called = true; }
    );

    // Simulate F10 press
    test_backend::test_keyboard_event event;
    event.key_code = KEY_F10;
    event.pressed = true;

    CHECK(manager.handle_key_event(event));
    CHECK(called);
}
```

## Backend Integration

### Event Traits Requirements

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

```cpp
// Backend should verify assumptions at compile time
// Example from conio backend
static_assert(TB_KEY_F1 > TB_KEY_F12,
    "termbox2 F-key ordering assumption violated");
static_assert(TB_KEY_F1 - TB_KEY_F12 == 11,
    "termbox2 F-key spacing assumption violated");

// In to_f_key() implementation
[[nodiscard]] static int to_f_key(const tb_event& e) noexcept {
    // Termbox2 uses reverse order: F1=65535, F12=65524
    if (e.key <= TB_KEY_F1 && e.key >= TB_KEY_F12) {
        return (TB_KEY_F1 - e.key) + 1;  // Convert to 1-12
    }
    return 0;
}
```

## Files Reference

### Headers
- `include/onyxui/hotkeys/hotkey_action.hh` - Semantic action enum
- `include/onyxui/hotkeys/hotkey_scheme.hh` - Scheme data structure
- `include/onyxui/hotkeys/hotkey_scheme_registry.hh` - Registry management
- `include/onyxui/hotkeys/builtin_hotkey_schemes.hh` - Built-in schemes
- `include/onyxui/hotkeys/key_sequence.hh` - Key parsing/formatting
- `include/onyxui/hotkeys/key_chord.hh` - Multi-key sequences & chords
- `include/onyxui/hotkeys/hotkey_manager.hh` - Manager with chord support

### Tests
- `unittest/hotkeys/test_hotkey_scheme.cc` - Scheme tests (8 cases, 148 assertions)
- `unittest/hotkeys/test_hotkey_manager.cc` - Manager tests (9 semantic action cases)
- `unittest/hotkeys/test_key_chord.cc` - Chord tests (multi-key, modifier-only)
- `backends/conio/unittest/test_conio_f_keys.cc` - Backend-specific tests

## Migration from Hardcoded Keys

### Before (Hardcoded)
```cpp
// Widget hardcodes F10
void on_key_press(int key) {
    if (key == KEY_F10) {
        activate_menu();
    }
}
```

### After (Scheme-Based)
```cpp
// Widget uses semantic actions
void initialize() {
    ui_services<Backend>::hotkeys()->register_semantic_action(
        hotkey_action::activate_menu_bar,
        [this]() { this->activate_menu(); }
    );
}
// Now respects user's chosen scheme (F10 or F9)
```

## Performance

- **Memory**: < 1 KB total for entire scheme system
- **Scheme switch**: O(1) pointer update
- **Key lookup**: O(n) where n ≈ 9 bindings
- **Handler dispatch**: O(1) hash map lookup
- **Overhead per key event**: 1-3 map lookups (negligible)

## Complete Example

```cpp
template<UIBackend Backend>
class text_editor {
public:
    text_editor() {
        setup_hotkeys();
    }

private:
    void setup_hotkeys() {
        auto* hotkeys = ui_services<Backend>::hotkeys();
        if (!hotkeys) return;

        // 1. QBasic-style: Alt alone opens menu
        hotkeys->register_modifier_activation(
            key_modifier::alt,
            [this]() { this->menu_bar->activate(); }
        );

        // 2. Emacs-style chord: Ctrl+X, Ctrl+S to save
        hotkeys->register_chord(
            make_emacs_chord({
                {'x', key_modifier::ctrl},
                {'s', key_modifier::ctrl}
            }),
            [this]() { this->save_file(); }
        );

        // 3. Vim-style: dd to delete line (in command mode)
        if (vim_mode_active) {
            hotkeys->register_chord(
                make_vim_chord("dd"),
                [this]() { this->delete_line(); }
            );
        }

        // 4. Traditional: Ctrl+S also saves
        auto save = std::make_shared<action<Backend>>();
        save->set_shortcut('s', key_modifier::ctrl);
        save->triggered.connect([this]() { this->save_file(); });
        hotkeys->register_action(save);

        // 5. Semantic action: Menu navigation
        hotkeys->register_semantic_action(
            hotkey_action::activate_menu_bar,
            [this]() { this->menu_bar->activate(); }
        );
    }
};
```

## Thread Safety

- **NOT thread-safe** - All operations must be on UI thread
- **ui_context**: Thread-local stack (safe for multi-threaded rendering)
- **Registry**: No concurrent access protection (UI thread only)
