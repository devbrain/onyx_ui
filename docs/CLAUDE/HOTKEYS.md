# OnyxUI Hotkey System

This document describes the comprehensive hotkey system with customizable keyboard layouts.

## Table of Contents

- [Overview](#overview)
- [Hotkey Schemes](#hotkey-schemes-customizable-keyboard-layouts)
- [Application Actions](#application-actions)
- [Priority System](#priority-system)
- [Features](#features)

---

## Overview

The hotkey system has two layers:

1. **Framework Semantic Actions** - Scheme-based keyboard layouts (NEW)
2. **Application Actions** - User-defined shortcuts

---

## Hotkey Schemes (Customizable Keyboard Layouts)

Users can switch between different keyboard layouts at runtime:

### Basic Usage

```cpp
// Access via ui_context (auto-configured)
scoped_ui_context<Backend> ctx;

// Current scheme is "Windows" (F10 for menu)
auto* current = ctx.hotkey_schemes().get_current_scheme();

// Switch to Norton Commander (F9 for menu)
ctx.hotkey_schemes().set_current_scheme("Norton Commander");

// Register framework semantic action handlers
ctx.hotkeys().register_semantic_action(
    hotkey_action::activate_menu_bar,
    [&menu]() { menu->activate(); }
);

// Now F10 (Windows) or F9 (Norton) will activate menu!
```

### Built-in Schemes

- **Windows**: F10 for menu (standard modern UI convention)
- **Norton Commander**: F9 for menu (classic DOS feel)

### Custom Schemes

```cpp
hotkey_scheme vim_scheme;
vim_scheme.name = "Vim-style";
vim_scheme.set_binding(hotkey_action::menu_down, parse_key_sequence("j"));
vim_scheme.set_binding(hotkey_action::menu_up, parse_key_sequence("k"));
// ... etc

ctx.hotkey_schemes().register_scheme(std::move(vim_scheme));
ctx.hotkey_schemes().set_current_scheme("Vim-style");
```

---

## Application Actions

Application-defined keyboard shortcuts:

```cpp
hotkey_manager<Backend> hotkeys;

// Register hotkeys
auto save_action = std::make_shared<action<Backend>>();
save_action->set_shortcut('s', key_modifier::ctrl);
hotkeys.register_action(save_action);

// Process events
hotkeys.handle_key_event(event);  // Triggers matching actions
```

---

## Priority System

1. **Framework semantic actions** (from current scheme) - FIRST
2. **Element-scoped application actions**
3. **Global application actions**
4. **Widget keyboard events** - LAST

This ensures framework-level shortcuts take precedence over application shortcuts, which take precedence over widget-specific handling.

---

## Features

- **Conflict detection** with policy enforcement
- **Scope management** (global, element-scoped)
- **Key sequence parsing** ("Ctrl+Shift+A", "Alt+F4", "F10", etc.)
- **Integration with action system**
- **Graceful fallback** (no binding = mouse still works)
- **Runtime scheme switching** (user can choose keyboard layout)
- **Auto-configuration** (built-in schemes pre-registered in ui_context)
- **Library-level** (NOT backend-specific - keys are universal)

---

## Key Reference Files

- `include/onyxui/hotkeys/key_sequence.hh` - Key combination parsing
- `include/onyxui/hotkeys/hotkey_manager.hh` - Global hotkey management
- `include/onyxui/hotkeys/hotkey_scheme.hh` - Keyboard layout schemes
- `unittest/hotkeys/test_hotkeys.cc` - Hotkey system tests
- `unittest/hotkeys/test_hotkey_manager.cc` - Manager tests
