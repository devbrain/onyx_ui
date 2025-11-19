---
sidebar_position: 2
---

# Line Edit

The `onyxui::line_edit` is a single-line text input widget for entering and editing text. It's essential for forms, dialogs, and any UI that requires user text input.

## Overview

A line edit widget provides a single-line text entry field with full editing capabilities including cursor movement, text selection, undo/redo, and horizontal scrolling for long text.

## Key Features

-   **Single-Line Input:** Text entry limited to one line (Enter key submits rather than inserting newline)
-   **Full Text Editing:** Insert/overwrite modes, cursor movement, selection, deletion
-   **Horizontal Scrolling:** Auto-scrolls to keep cursor visible when text exceeds widget width
-   **Cursor Blinking:** Visual cursor with theme-configurable blink interval
-   **Semantic Actions:** 20+ keyboard shortcuts (Windows/Norton Commander schemes)
-   **Password Mode:** Displays asterisks instead of actual text
-   **Read-Only Mode:** Display-only text field
-   **Placeholder Text:** Hint text shown when empty
-   **Validation:** Custom validators for input constraints
-   **Undo/Redo:** Stack-based text editing history
-   **Themable:** Full theme support with line_edit-specific styles

## Usage

Here's a simple example of how to create a line_edit and use it in a form:

```cpp
#include <onyxui/widgets/input/line_edit.hh>
#include <iostream>

// Simple text input
auto name_input = std::make_unique<line_edit<Backend>>();
name_input->set_placeholder("Enter your name...");
name_input->text_changed.connect([](const std::string& text) {
    std::cout << "Name: " << text << "\n";
});

// Password field
auto password_input = std::make_unique<line_edit<Backend>>();
password_input->set_password_mode(true);
password_input->set_placeholder("Password");
password_input->return_pressed.connect([]() {
    // Enter pressed - submit form
    submit_login();
});

// Read-only display
auto readonly_field = std::make_unique<line_edit<Backend>>("Read-only text");
readonly_field->set_read_only(true);

// Validated email field
auto email_input = std::make_unique<line_edit<Backend>>();
email_input->set_validator([](const std::string& text) {
    return text.find('@') != std::string::npos;
});
```

## Keyboard Interaction

The line_edit widget responds to semantic hotkey actions, allowing full keyboard control:

### Cursor Movement
- **Left/Right Arrows:** Move cursor one character
- **Ctrl+Left/Right:** Move cursor one word
- **Home/End:** Move to start/end of line

### Text Selection
- **Shift+Left/Right:** Extend selection one character
- **Ctrl+Shift+Left/Right:** Extend selection one word
- **Shift+Home/End:** Extend selection to start/end
- **Ctrl+A:** Select all text

### Text Deletion
- **Backspace:** Delete character before cursor
- **Delete:** Delete character after cursor
- **Ctrl+Backspace:** Delete word before cursor
- **Ctrl+Delete:** Delete word after cursor

### Clipboard Operations (API ready, pending OS integration)
- **Ctrl+C:** Copy selected text
- **Ctrl+X:** Cut selected text
- **Ctrl+V:** Paste from clipboard

### Edit Modes
- **Insert Key:** Toggle between insert and overwrite modes
- **Enter:** Emit `editing_finished` and `return_pressed` signals (does NOT insert newline)
- **Escape:** Cancel editing (future)

### Customizing Keyboard Shortcuts

All keyboard shortcuts use semantic actions and can be customized via hotkey schemes:

```cpp
auto& schemes = ctx.hotkey_schemes();
auto* scheme = schemes.get_scheme("Windows");

// Customize cursor movement (example: Emacs-style)
scheme->set_binding(hotkey_action::cursor_move_left,
                    parse_key_sequence("Ctrl+B"));  // backward
scheme->set_binding(hotkey_action::cursor_move_right,
                    parse_key_sequence("Ctrl+F"));  // forward
```

## Insert vs Overwrite Mode

The line_edit supports two text entry modes:

### Insert Mode (Default)
- New characters are inserted at cursor position
- Existing text shifts right
- Cursor displayed as thin vertical line (|)

### Overwrite Mode
- New characters replace existing characters
- Text does not shift
- Cursor displayed as block (█)
- Toggle with Insert key

## Horizontal Scrolling

When text exceeds the widget's visible width:
- Text automatically scrolls horizontally to keep cursor visible
- Scroll offset adjusts when cursor moves beyond visible area
- Only visible portion of text is rendered
- Cursor always remains in view

## API Reference

### Template Parameters

-   `Backend`: The backend traits class.

### Public Signals

-   **`signal<const std::string&> text_changed`:** Emitted when text is modified. Receives the new text.
-   **`signal<> editing_finished`:** Emitted when Enter is pressed or focus is lost (submit form).
-   **`signal<> return_pressed`:** Emitted specifically when Enter key is pressed.

### Public Methods

#### Text Management
-   **`void set_text(const std::string& text)`:** Sets the text content.
-   **`const std::string& text() const`:** Returns current text.

#### Placeholder
-   **`void set_placeholder(const std::string& placeholder)`:** Sets hint text shown when empty.
-   **`const std::string& placeholder() const`:** Returns placeholder text.

#### Modes
-   **`void set_password_mode(bool password)`:** Enable/disable password mode (shows asterisks).
-   **`bool is_password_mode() const`:** Returns true if password mode enabled.
-   **`void set_read_only(bool read_only)`:** Enable/disable read-only mode.
-   **`bool is_read_only() const`:** Returns true if read-only.
-   **`void set_overwrite_mode(bool overwrite)`:** Set insert (false) or overwrite (true) mode.
-   **`bool is_overwrite_mode() const`:** Returns true if overwrite mode.

#### Cursor and Selection
-   **`void set_cursor_position(int position)`:** Move cursor to specific position.
-   **`int cursor_position() const`:** Returns current cursor position.
-   **`void select_all()`:** Select all text.
-   **`void clear_selection()`:** Remove selection.
-   **`bool has_selection() const`:** Returns true if text is selected.
-   **`std::string selected_text() const`:** Returns selected text (or empty string).

#### Clipboard (API ready, pending OS integration)
-   **`void copy() const`:** Copy selected text to clipboard.
-   **`void paste()`:** Paste from clipboard.
-   **`void cut()`:** Cut selected text to clipboard.

#### Undo/Redo
-   **`void undo()`:** Undo last change.
-   **`void redo()`:** Redo last undone change.
-   **`bool can_undo() const`:** Returns true if undo available.
-   **`bool can_redo() const`:** Returns true if redo available.

#### Validation
-   **`void set_validator(std::function<bool(const std::string&)> validator)`:** Set custom validation function.
-   **`bool is_valid() const`:** Returns true if current text passes validation.

#### Border
-   **`void set_has_border(bool border)`:** Enable/disable border rendering.
-   **`bool has_border() const`:** Returns true if border enabled.

### Theming

The appearance of the line_edit is controlled by the `line_edit` section in the theme. You can customize the following properties:

```yaml
line_edit:
  text: "#FFFFFF"                    # Normal text color
  background: "#000080"               # Background color
  placeholder_text: "#808080"         # Placeholder hint color
  cursor: "#FFFF00"                   # Cursor color
  cursor_insert_icon: "│"             # Insert mode cursor icon
  cursor_overwrite_icon: "█"          # Overwrite mode cursor icon
  cursor_blink_interval_ms: 500       # Cursor blink rate
  selection_background: "#0000FF"     # Selected text background
  border_normal: single_line          # Border style
  border_focused: double_line         # Border when focused
```

### Theme Methods

Override these methods to customize appearance:

```cpp
[[nodiscard]] typename Backend::color_type get_theme_background_color() const override;
[[nodiscard]] typename Backend::color_type get_theme_foreground_color() const override;
```

## Complete Example

Here's a complete login form example using line_edit:

```cpp
#include <onyxui/ui_context.hh>
#include <onyxui/widgets/containers/vbox.hh>
#include <onyxui/widgets/input/line_edit.hh>
#include <onyxui/widgets/button.hh>
#include <onyxui/widgets/label.hh>

template<typename Backend>
class login_form : public vbox<Backend> {
public:
    login_form() {
        // Username field
        this->template emplace_child<label<Backend>>("Username:");
        auto* username = this->template emplace_child<line_edit<Backend>>();
        username->set_placeholder("Enter username");
        username->set_has_border(true);

        // Password field
        this->template emplace_child<label<Backend>>("Password:");
        auto* password = this->template emplace_child<line_edit<Backend>>();
        password->set_password_mode(true);
        password->set_placeholder("Enter password");
        password->set_has_border(true);

        // Login button
        auto* login_btn = this->template emplace_child<button<Backend>>("Login");

        // Submit on Enter in password field
        password->return_pressed.connect([this]() {
            handle_login();
        });

        // Submit on Login button click
        login_btn->clicked.connect([this]() {
            handle_login();
        });
    }

private:
    void handle_login() {
        // Validate and submit login credentials
        std::cout << "Logging in...\n";
    }
};
```

## Differences from text_edit

It's important to understand the difference between `line_edit` and the future `text_edit` widget:

| Feature | line_edit | text_edit (future) |
|---------|-----------|-------------------|
| **Lines** | Single line only | Multiple lines |
| **Enter key** | Emits `editing_finished` | Inserts `\n` |
| **Scrolling** | Horizontal only | Vertical + horizontal |
| **Use cases** | Forms, dialogs, search | Notes, comments, code |

## Best Practices

1. **Always set placeholder text** - Helps users understand what to enter
2. **Use password mode for sensitive data** - Displays asterisks instead of plain text
3. **Connect to return_pressed for forms** - Submit form when Enter is pressed
4. **Enable borders for visual clarity** - Makes input fields easily identifiable
5. **Use validators for constraints** - Validate email, phone numbers, etc.
6. **Set read-only for display fields** - Show non-editable information
7. **Use semantic signals** - Connect to `text_changed` for live validation, `editing_finished` for submission

## See Also

-   **[Button](./button.md)** - Often used with line_edit in forms
-   **[Label](./label.md)** - For labeling input fields
-   **[VBox](./vbox-hbox.md)** - For arranging form fields vertically
