# Bug Fix: Arrow Key Navigation Not Working

**Date**: 28/10/2025
**Status**: Fixed ✅

## Issue

Menu navigation with arrow keys did not work in widgets_demo, despite the quit button working correctly after the previous fix.

## Symptoms

- Arrow Up/Down keys had no effect in menus
- ESC key worked
- Quit button worked with mouse clicks
- Enter key also failed in menus

## Root Cause

**Triple Constructor Mismatch**: Three different code paths were creating incompatible `key_sequence` objects for special keys.

### The Problem

`key_sequence` has two fields:
```cpp
struct key_sequence {
    char key = '\0';   // ASCII character
    int f_key = 0;     // F-keys (positive 1-12) OR special keys (negative -1,-2,-3,-4)
};
```

Arrow keys use **negative codes** stored in `f_key`:
- Up = -1
- Down = -2
- Left = -3
- Right = -4

However, three bugs caused mismatches:

### Bug 1: `parse_key_sequence()` (String Parser)

**File**: `include/onyxui/hotkeys/key_sequence.hh:496`

**Before**:
```cpp
// Parse "Down" → -2
return key_sequence{static_cast<char>(it->second), mods};  // WRONG: char(-2) = 254
```

Creates: `key = (char)(-2)`, `f_key = 0`

**After**:
```cpp
int code = it->second;
if (code < 0) {
    return key_sequence{code, mods};  // CORRECT: int constructor
} else {
    return key_sequence{static_cast<char>(code), mods};  // char for Enter/Escape
}
```

Creates: `key = '\0'`, `f_key = -2`

### Bug 2: `hotkey_manager::event_to_sequence()` (Event Converter)

**File**: `include/onyxui/hotkeys/hotkey_manager.hh:738`

**Before**:
```cpp
int const special = traits::to_special_key(event);
if (special != 0) {
    return key_sequence{static_cast<char>(special), mods};  // WRONG: char(-2) = 254
}
```

**After**:
```cpp
int const special = traits::to_special_key(event);
if (special != 0) {
    return key_sequence{special, mods};  // CORRECT: int constructor
}
```

### Bug 3: `format_key_sequence()` (Formatter)

**File**: `include/onyxui/hotkeys/key_sequence.hh:541`

**Before**:
```cpp
if (seq.f_key != 0) {
    result += "F" + std::to_string(seq.f_key);  // WRONG: "F-2" for Down arrow
}
```

Formatted Down arrow as "F-2" instead of "Down".

**After**:
```cpp
if (seq.f_key > 0) {
    // F-keys (F1-F12): positive values
    result += "F" + std::to_string(seq.f_key);
} else if (seq.f_key < 0) {
    // Arrow keys: negative values
    for (const auto& [name, code] : detail::special_keys) {
        if (seq.f_key == code) {
            result += name;  // "Down", "Up", etc.
            return result;
        }
    }
}
```

## Why This Caused Arrow Keys to Fail

1. **Hotkey scheme registration** (at startup):
   ```cpp
   scheme.bindings = {
       {hotkey_action::menu_down, parse_key_sequence("Down")},  // Bug 1: Created key=(char)-2
   };
   ```

2. **Event from backend** (when user presses Down):
   ```cpp
   auto seq = event_to_sequence(down_event);  // Bug 2: Created key=(char)-2 too
   ```

3. **Comparison**:
   ```cpp
   if (scheme_key == event_key) { ... }  // MATCHED! But WRONG values
   ```

Despite both being wrong, they matched initially... but then `create_key_sequence()` (from `event_conversion.hh`) was introduced in Phase 6 and it was **correct**:

```cpp
case keyboard_event::key_type::special:
    if (kbd.special_key != 0) {
        return key_sequence{kbd.special_key, mods};  // CORRECT: int constructor
    }
```

This created: `key = '\0'`, `f_key = -2`

Now comparison failed:
- Scheme binding: `key = (char)(-2)`, `f_key = 0`
- Event from backend: `key = '\0'`, `f_key = -2`
- **NO MATCH** → Arrow keys ignored!

## Impact

All arrow key navigation in menus was broken:
- Up/Down to navigate menu items
- Left/Right to switch between menus
- Enter to select items
- Escape also failed (but is '\0' in some events, so sometimes worked)

## Fix Applied

**Files modified** (3):
1. `include/onyxui/hotkeys/key_sequence.hh:493-504` - Fixed `parse_key_sequence()`
2. `include/onyxui/hotkeys/hotkey_manager.hh:735-740` - Fixed `event_to_sequence()`
3. `include/onyxui/hotkeys/key_sequence.hh:540-565` - Fixed `format_key_sequence()`
4. `unittest/hotkeys/test_hotkey_scheme.cc:70-81` - Fixed tests to check `f_key` not `key`

**Key principle**: Negative codes (arrow keys) use `int` constructor, positive codes (Enter/Escape) use `char` constructor.

## Verification

✅ All 794 tests passing (was 788 passing, 6 failing)
✅ Hotkey scheme tests now correctly check `f_key` for arrow keys
✅ Round-trip parsing test passing (Down → parse → format → "Down")
✅ Menu navigation tests passing
✅ widgets_demo rebuilt successfully

## Testing

Menu navigation should now work in widgets_demo:
- F10 (or F9 in Norton Commander scheme) to activate menu
- Arrow Up/Down to navigate items
- Arrow Left/Right to switch menus
- Enter to select
- Escape to close

## Related Bugs

This is part 2 of the widgets_demo bug fixes:
- **Part 1**: Mouse click generation (event_target.hh:837-901) - FIXED
- **Part 2**: Arrow key navigation (this fix) - FIXED

Both bugs were introduced during Phase 6 event system refactoring when switching from backend-specific events to unified `ui_event`.
