# OnyxUI Widget Roadmap

**Status**: Ready for Implementation
**Version**: v1.0
**Created**: 2025-11-18
**Author**: Architecture Team

---

## Table of Contents

1. [Overview](#overview)
2. [Widget Priority Analysis](#widget-priority-analysis)
3. [Implementation Phases](#implementation-phases)
4. [Widget Specifications](#widget-specifications)
5. [Integration Requirements](#integration-requirements)
6. [Testing Strategy](#testing-strategy)
7. [Future Widgets](#future-widgets)

---

## Overview

### Current State

OnyxUI has a solid foundation with container widgets, menus, and windows. However, it lacks essential **input widgets** needed for building real applications.

**Existing Widgets** (Complete):
- ✅ **Containers**: vbox, hbox, grid, panel, anchor_panel, absolute_panel, group_box
- ✅ **Display**: label, spacer, spring, status_bar
- ✅ **Menus**: menu_bar, menu, menu_item
- ✅ **Windows**: window (draggable, resizable, minimize/maximize)
- ✅ **Scrolling**: scroll_view, scrollable, scrollbar, scroll_controller
- ✅ **Buttons**: button (with mnemonics and actions)
- ✅ **Input**: line_edit (single-line text input with scrolling)

**Missing Widgets** (Critical):
- ✅ **checkbox** - Boolean toggle (COMPLETED)
- ✅ **radio_button** - Mutually exclusive options (COMPLETED)
- ✅ **slider** - Numeric range input (COMPLETED)
- ✅ **progress_bar** - Progress indicator (COMPLETED)
- ✅ **tab_widget** - Multi-page container (COMPLETED)

**Missing Widgets** (Future):
- ❌ **text_edit** - Multi-line text editor (notes, comments)

### Why These Widgets Matter

**Without line_edit:**
- Can't build login forms
- Can't build search boxes
- Can't build editable combo_box
- Can't build settings dialogs

**Without checkbox/radio_button:**
- Can't build preference panels
- Can't build option dialogs
- Can't build feature toggles

**Without slider/progress_bar:**
- Can't show loading states
- Can't adjust numeric values visually
- Can't build media players

**With these 6 widgets:** Users can build **complete applications** (forms, dialogs, dashboards, settings).

---

## Widget Priority Analysis

### Tier 1: Critical Input Widgets (Weeks 1-3)

**Highest Priority:**
1. **line_edit** - Enables all forms and dialogs
2. **checkbox** - Enables boolean options
3. **radio_button** - Enables mutually exclusive options

**Why First:**
- These 3 widgets unlock **80% of common UI patterns**
- line_edit is a dependency for editable combo_box (future MVC phase)
- checkbox/radio are simple but essential

### Tier 2: Visual Feedback Widgets (Weeks 4-5)

**High Priority:**
4. **progress_bar** - Visual feedback for operations
5. **slider** - Numeric input with visual feedback

**Why Second:**
- Nice-to-have but not blocking
- Enhance UX but apps can function without them
- Relatively simple to implement

### Tier 3: Container Widgets (Weeks 6-7)

**Medium Priority:**
6. **tab_widget** - Multi-page organization
7. **splitter** - Resizable panels

**Why Third:**
- Can be worked around with existing containers
- More complex implementation
- Needed for advanced layouts

---

## Implementation Phases

### Phase 1: line_edit (Week 1) - **✅ COMPLETED**

**Status**: Fully implemented with all features and horizontal scrolling

**Goal**: Single-line text input field for forms and dialogs

**Files Created**:
- ✅ `include/onyxui/widgets/input/line_edit.hh` - Complete widget (1372 lines)
- ✅ `include/onyxui/hotkeys/hotkey_action.hh` - Added 24 text editing semantic actions
- ✅ `include/onyxui/hotkeys/builtin_hotkey_schemes.hh` - Text editing keybindings
- ✅ `unittest/widgets/test_line_edit.cc` - 14 test cases, 62 assertions (all passing)

---

#### ✅ COMPLETED Features

**Core Architecture:**
- ✅ Widget class structure inheriting from `widget<Backend>`
- ✅ Full API design (text, placeholder, cursor, selection, clipboard, undo/redo)
- ✅ Theme integration using `resolved_style` pattern
- ✅ Three-phase event routing (capture/target/bubble)
- ✅ Focus management integration
- ✅ Render context pattern for measurement and rendering

**Text Management:**
- ✅ Single-line text storage
- ✅ Placeholder text support
- ✅ Password mode (displays asterisks)
- ✅ Read-only mode
- ✅ Insert vs Overwrite mode toggle
- ✅ Text validation with custom validators

**Selection & Cursor:**
- ✅ Cursor positioning API (`set_cursor_position`, `cursor_position`)
- ✅ Text selection API (`select_all`, `clear_selection`, `has_selection`, `selected_text`)
- ✅ Cursor style enum (line/block/underline)
- ✅ Cursor rendering in `do_render` (visual placeholder for actual cursor)

**Clipboard & Undo:**
- ✅ Clipboard operation stubs (`copy`, `paste`, `cut`)
- ✅ Undo/redo stacks and API

**Semantic Actions:**
- ✅ **24 text editing semantic actions defined** in `hotkey_action.hh`:
  - Cursor movement: `cursor_move_left/right/home/end`, word jumps
  - Selection: `cursor_select_left/right/all/home/end`, word selection
  - Deletion: `text_delete_char/backspace`, word deletion, line deletion
  - Clipboard: `text_copy/cut/paste`
  - Undo/Redo: `text_undo/redo`
  - Mode: `text_toggle_overwrite`
  - Completion: `text_accept/cancel`

**Signals:**
- ✅ `text_changed(const std::string& text)` - Text modified
- ✅ `editing_finished()` - Enter pressed or focus lost
- ✅ `return_pressed()` - Enter key pressed

**Testing:**
- ✅ 12 comprehensive test cases covering all APIs
- ✅ Tests for construction, text management, modes, cursor/selection, validation, undo/redo, layout, focus
- ✅ All tests passing (46 assertions)

---

**Additional Features:**
- ✅ **Cursor blinking** - Implemented with theme-configurable interval
- ✅ **Cursor visibility** - Toggles based on blink state and resets on input
- ✅ **Horizontal scrolling** - Auto-scrolls to keep cursor visible
- ✅ **Cursor rendering** - Theme-based cursor icons (insert/overwrite modes)
- ✅ **Backend integration** - Full conio backend support

**Clipboard Integration** (API ready, implementation pending):
- ⏸️ **Platform clipboard** - Stubs implemented, needs OS integration
- ⏸️ Copy/Cut/Paste operations ready for backend clipboard API

**Advanced Input** (future enhancement):
- ⏸️ Mouse click to set cursor position
- ⏸️ Mouse drag to select text
- ⏸️ Double-click to select word
- ⏸️ Triple-click to select all

---

**Key Difference from text_edit:**
- `line_edit`: Single-line, Enter submits → Used in forms, dialogs, search boxes
- `text_edit`: Multi-line, Enter inserts `\n` → Used in notes, comments (future widget)

**✅ All Success Criteria Met:**
- ✅ Widget architecture complete
- ✅ API fully designed and tested (14 test cases, 62 assertions)
- ✅ Theme and focus integration working
- ✅ Semantic actions fully implemented (20+ text editing actions)
- ✅ **Type text and edit** → Full keyboard input with insert/overwrite modes
- ✅ **Navigate with arrow keys** → Complete cursor movement (char/word/line)
- ✅ **Select text with Shift+arrows** → Full text selection support
- ✅ **Cursor blinks** → Implemented with theme-configurable interval
- ✅ **Horizontal scrolling** → Auto-scrolls to keep cursor visible
- ⏸️ **Copy/paste works** → API ready, needs platform clipboard backend

### Phase 2: checkbox + radio_button (Week 2) - **✅ COMPLETED**

**Goal**: Boolean toggles and mutually exclusive options

**Status**: checkbox ✅ COMPLETED | radio_button ✅ COMPLETED

---

#### ✅ checkbox COMPLETED

**Files Created**:
- ✅ `include/onyxui/widgets/input/checkbox.hh` - Complete widget (394 lines)
- ✅ `include/onyxui/hotkeys/hotkey_action.hh` - Added `activate_widget` semantic action
- ✅ `include/onyxui/hotkeys/builtin_hotkey_schemes.hh` - Space key binding
- ✅ `unittest/widgets/test_checkbox.cc` - 28 test cases, 74 assertions (all passing)
- ✅ `examples/demo_ui_builder.hh` - 4 checkbox examples in demo

**Implemented Features**:
- ✅ Two-state mode (checked/unchecked)
- ✅ Tri-state mode (checked/unchecked/indeterminate for "select all" scenarios)
- ✅ Keyboard toggle (Space key via `activate_widget` semantic action)
- ✅ Mouse toggle (click anywhere on widget)
- ✅ Text label with mnemonic support (storage ready)
- ✅ Disabled state
- ✅ Theme integration for colors and styles
- ✅ Focus management

**Signals**:
- ✅ `toggled(bool checked)` - Emitted on checked ⟷ unchecked transitions
- ✅ `state_changed(tri_state state)` - Emitted on any state change

**Testing**:
- ✅ 28 comprehensive test cases covering all features
- ✅ Construction, state management, signals, events, tri-state, mnemonics, layout
- ✅ All tests passing (74 assertions, zero warnings)

**Success Criteria Met**:
- ✅ Click checkbox toggles state
- ✅ Space key toggles checkbox
- ✅ Visual states (checked/unchecked/indeterminate) themed correctly
- ✅ Signals emit on state changes
- ✅ Tri-state mode works correctly
- ✅ Disabled state prevents interaction

---

#### ✅ radio_button COMPLETED

**Files Created**:
- ✅ `include/onyxui/widgets/input/radio_button.hh` - Complete widget (357 lines)
- ✅ `include/onyxui/widgets/input/button_group.hh` - Group manager (439 lines)
- ✅ `include/onyxui/theming/theme.hh` - Added `radio_button_style` struct
- ✅ `backends/conio/include/onyxui/conio/conio_renderer.hh` - Radio icons (radio_unchecked, radio_checked)
- ✅ `backends/conio/src/conio_renderer.cc` - 3-character icon rendering
- ✅ `backends/conio/src/conio_themes.hh` - Radio button theme configuration
- ✅ `themes/examples/norton_blue.yaml` - Radio button YAML theme
- ✅ `unittest/widgets/test_radio_button.cc` - 23 test cases, 47 assertions (all passing)
- ✅ `unittest/widgets/test_button_group.cc` - 23 test cases, 74 assertions (all passing)
- ✅ `examples/demo_ui_builder.hh` - Radio button demo (size selection)

**Implemented Features**:
- ✅ Mutually exclusive selection via `button_group`
- ✅ Keyboard navigation (Arrow keys within group)
- ✅ Mouse selection (click to select, cannot uncheck)
- ✅ Text label with mnemonic support
- ✅ Space key selects focused radio button
- ✅ Themed icons (3-character DOS-style: `( )` and `(*)`)
- ✅ Focus management integration
- ✅ Disabled state
- ✅ Theme integration (normal/hover/checked/disabled visual states)

**Signals**:
- ✅ radio_button: `toggled(bool checked)` - Emitted on state change
- ✅ button_group: `button_toggled(int id, bool checked)` - Emitted for group changes

**Success Criteria Met**:
- ✅ Radio buttons in group are mutually exclusive
- ✅ Arrow keys navigate radio group (select_next/select_previous)
- ✅ Visual states (checked/unchecked) themed correctly
- ✅ Signals emit on state changes
- ✅ Click selects radio button
- ✅ Space key activates radio button
- ✅ Icons render as 3-character glyphs

**Testing**:
- ✅ 23 radio_button test cases covering all features
- ✅ 23 button_group test cases for mutual exclusion and navigation
- ✅ All tests passing (121 total assertions, zero warnings)
- ✅ Demo integrated and working correctly

### Phase 3: progress_bar + slider (Week 3)

**Goal**: Visual feedback and numeric input widgets

**Files to Create**:
- `include/onyxui/widgets/display/progress_bar.hh`
- `include/onyxui/widgets/display/progress_bar.inl`
- `include/onyxui/widgets/input/slider.hh`
- `include/onyxui/widgets/input/slider.inl`
- `unittest/widgets/test_progress_bar.cc`
- `unittest/widgets/test_slider.cc`

**progress_bar Features**:
- Horizontal/vertical orientation
- Determinate mode (0-100%)
- Indeterminate mode (busy indicator, animated)
- Optional text overlay (e.g., "45%")
- Theming support (colors, gradients)

**slider Features**:
- Horizontal/vertical orientation
- Integer or floating-point values
- Min/max range
- Step size (e.g., snap to multiples of 5)
- Tick marks (optional)
- Current value label (optional)
- Keyboard adjustment (arrow keys, Page Up/Down)
- Mouse drag

**Signals**:
- progress_bar: `value_changed(int value)` - For determinate mode
- slider: `value_changed(int value)` or `value_changed(double value)`
- slider: `slider_moved(int value)` - Emitted during drag
- slider: `slider_released()` - Emitted when mouse released

**Success Criteria**:
- ✅ progress_bar shows percentage fill
- ✅ Indeterminate progress_bar animates
- ✅ slider responds to mouse drag
- ✅ slider responds to keyboard (arrows, Page Up/Down)
- ✅ slider snaps to step values
- ✅ Signals emit correctly

### Phase 4: tab_widget (Week 4) - **✅ COMPLETED**

**Goal**: Multi-page container with tab navigation

**Status**: Fully implemented with all core features

**Files Created**:
- ✅ `include/onyxui/widgets/containers/tab_widget.hh` - Complete widget (extends panel<Backend>)
- ✅ `unittest/widgets/test_tab_widget.cc` - 18 test cases, 83 assertions (all passing)
- ✅ `docs/TAB_WIDGET_DESIGN.md` - Comprehensive design document

**Implemented Features**:
- ✅ Add/remove tabs dynamically
- ✅ Tab labels with truncation support
- ✅ Tab position (top/bottom/left/right)
- ✅ Closeable tabs (with X button)
- ✅ Current tab highlighting
- ✅ Keyboard navigation (Ctrl+Tab, Ctrl+Shift+Tab, Alt+1-9, Ctrl+W)
- ✅ Mouse hover visual feedback
- ✅ Theme integration via `tab_widget_style`

**Signals**:
- ✅ `current_changed(int index)` - Tab switched
- ✅ `tab_close_requested(int index)` - X button clicked

**Success Criteria** (All Met):
- ✅ Add tabs with labels
- ✅ Click tab to switch
- ✅ Ctrl+Tab cycles tabs
- ✅ Close button removes tab
- ✅ Current tab visually highlighted

**Implemented Enhancements**:
- ✅ Tab overflow scroll arrows (scroll_left/scroll_right, click arrows)

**Future Enhancements** (Deferred):
- Tab reordering (drag tabs)
- Tab icons

### Phase 5: splitter (Week 5)

**Goal**: Resizable split panels

**Files to Create**:
- `include/onyxui/widgets/containers/splitter.hh`
- `include/onyxui/widgets/containers/splitter.inl`
- `unittest/widgets/test_splitter.cc`

**Features**:
- Horizontal/vertical split
- Drag splitter handle to resize
- Multiple splits (3+ panels)
- Collapse panels (double-click splitter)
- Save/restore split positions
- Min/max size constraints per panel

**Signals**:
- `splitter_moved(int index, int position)` - Handle dragged

**Success Criteria**:
- ✅ Drag splitter handle resizes panels
- ✅ Multiple panels supported
- ✅ Double-click collapses panel
- ✅ Size constraints respected

---

## Widget Specifications

### 1. line_edit (Single-Line Text Input)

#### Visual Design

```
Insert mode (default):
┌────────────────────────────────┐
│ Hello, World!|                 │  ← Thin vertical line cursor
└────────────────────────────────┘

Overwrite mode (Insert key toggles):
┌────────────────────────────────┐
│ Hello, Worl█!                  │  ← Block cursor (covers character)
└────────────────────────────────┘

Placeholder text:
┌────────────────────────────────┐
│ Enter your name...             │  ← Gray placeholder (no cursor)
└────────────────────────────────┘

Password mode:
┌────────────────────────────────┐
│ ••••••••|                      │  ← Dots with cursor
└────────────────────────────────┘

Text selection:
┌────────────────────────────────┐
│ Hello,█World!                  │  ← Highlighted background
└────────────────────────────────┘
```

#### Key Behavior

**Enter Key**: Emits `editing_finished` signal (submits form, does NOT insert newline)
**Line Breaks**: Not allowed (single line only)
**Vertical Scrolling**: None (fixed height)
**Use Cases**: Login forms, search boxes, URL bars, dialog inputs

#### API Design

```cpp
template<UIBackend Backend>
class line_edit : public widget<Backend> {
public:
    explicit line_edit(std::string text = "", ui_element<Backend>* parent = nullptr);

    // Text management
    void set_text(const std::string& text);
    [[nodiscard]] const std::string& text() const noexcept { return m_text; }

    // Placeholder (hint when empty)
    void set_placeholder(const std::string& placeholder);
    [[nodiscard]] const std::string& placeholder() const noexcept { return m_placeholder; }

    // Password mode
    void set_password_mode(bool password);
    [[nodiscard]] bool is_password_mode() const noexcept { return m_is_password; }

    // Read-only mode
    void set_read_only(bool read_only);
    [[nodiscard]] bool is_read_only() const noexcept { return m_is_read_only; }

    // Edit mode (insert vs overwrite)
    void set_overwrite_mode(bool overwrite);
    [[nodiscard]] bool is_overwrite_mode() const noexcept { return m_overwrite_mode; }

    // Cursor and selection
    void set_cursor_position(int position);
    [[nodiscard]] int cursor_position() const noexcept { return m_cursor_pos; }

    void select_all();
    void clear_selection();
    [[nodiscard]] bool has_selection() const noexcept { return m_selection_start != m_selection_end; }
    [[nodiscard]] std::string selected_text() const;

    // Clipboard
    void copy() const;
    void paste();
    void cut();

    // Undo/redo
    void undo();
    void redo();
    [[nodiscard]] bool can_undo() const noexcept { return m_undo_stack.size() > 0; }
    [[nodiscard]] bool can_redo() const noexcept { return m_redo_stack.size() > 0; }

    // Validation
    void set_validator(std::function<bool(const std::string&)> validator);
    [[nodiscard]] bool is_valid() const;

    // Signals
    signal<const std::string&> text_changed;    // Text modified
    signal<> editing_finished;                  // Enter key or focus lost (submit form)
    signal<> return_pressed;                    // Enter key specifically

protected:
    void do_render(render_context<Backend>& ctx) const override;
    bool handle_event(const ui_event& event, event_phase phase) override;

private:
    std::string m_text;
    std::string m_placeholder;
    bool m_is_password = false;
    bool m_is_read_only = false;
    bool m_overwrite_mode = false;  // Insert (false) vs Overwrite (true)

    // Cursor and selection
    int m_cursor_pos = 0;
    int m_selection_start = 0;
    int m_selection_end = 0;
    bool m_cursor_visible = true;  // Blink state (toggles every 500ms)

    // Undo/redo
    std::vector<std::string> m_undo_stack;
    std::vector<std::string> m_redo_stack;

    // Validation
    std::function<bool(const std::string&)> m_validator;

    // Helpers
    void insert_text(const std::string& text);
    void delete_selection();
    void move_cursor(int offset, bool select = false);
    void update_cursor_blink();
};
```

#### Usage Example

```cpp
// Simple text field
auto name_edit = std::make_unique<line_edit<Backend>>();
name_edit->set_placeholder("Enter your name...");
name_edit->text_changed.connect([](const std::string& text) {
    std::cout << "Name: " << text << "\n";
});

// Password field
auto password_edit = std::make_unique<line_edit<Backend>>();
password_edit->set_password_mode(true);
password_edit->set_placeholder("Password");
password_edit->return_pressed.connect([]() {
    submit_login();  // Enter key submits form
});

// Validated email field
auto email_edit = std::make_unique<line_edit<Backend>>();
email_edit->set_validator([](const std::string& text) {
    return text.find('@') != std::string::npos;  // Simple email check
});

// Read-only display
auto readonly_edit = std::make_unique<line_edit<Backend>>("Read-only text");
readonly_edit->set_read_only(true);
```

#### Implementation Notes

**Key Challenges:**
1. **Cursor blinking** - Need timer or frame-based animation
2. **Text selection** - Track start/end, render highlight
3. **Clipboard integration** - Platform-specific (use Backend concept?)
4. **Unicode support** - Handle multi-byte characters correctly (UTF-8)
5. **Undo/redo** - Stack-based history with size limits
6. **Horizontal scrolling** - When text exceeds widget width

**Rendering:**
- Draw background rect (themed)
- Draw text or placeholder (different colors)
- Draw selection highlight (blue rect behind text)
- Draw cursor (vertical line, blinking)
- Clip text if too long (horizontal scroll to keep cursor visible)

**Event Handling:**
- Mouse click → set cursor position
- Mouse drag → select text
- Left/Right arrows → move cursor (no up/down, single line only)
- Shift+arrows → select text
- Ctrl+A → select all
- Ctrl+C/V/X → clipboard operations
- Ctrl+Z/Y → undo/redo
- **Enter → emit editing_finished and return_pressed (does NOT insert newline)**
- **Insert key → toggle overwrite mode** (changes cursor shape)
- Home/End → move to start/end of text
- Delete → delete character after cursor
- Backspace → delete character before cursor
- Typing → insert (push text right) or overwrite (replace character) based on mode

#### Backend Cursor Rendering Requirements

**Cursor rendering is backend-specific.** The `line_edit` widget provides cursor state, but the backend draws it.

**Required Backend Methods:**

```cpp
// Add to Backend::renderer_type concept
template<typename Renderer>
concept UIRenderer = requires(Renderer r, /* ... */) {
    // Existing methods...

    // Cursor rendering
    { r.draw_cursor(rect, cursor_style, color) } -> std::same_as<void>;
};

// Cursor styles
enum class cursor_style : uint8_t {
    line,       // Thin vertical line (insert mode): |
    block,      // Block covering character (overwrite mode): █
    underline   // Underline under character (alternate overwrite): _
};
```

**Usage in line_edit::do_render():**

```cpp
void line_edit<Backend>::do_render(render_context<Backend>& ctx) const {
    // ... draw background, text, selection ...

    // Draw cursor if focused and visible (blinking)
    if (this->has_focus() && m_cursor_visible) {
        // Calculate cursor position in screen coordinates
        int cursor_x = get_cursor_screen_x();
        int cursor_y = rect_utils::get_y(this->bounds());
        int cursor_h = rect_utils::get_height(this->bounds());

        // Determine cursor style based on edit mode
        cursor_style style = m_overwrite_mode ? cursor_style::block : cursor_style::line;

        // Get cursor color from theme
        auto cursor_color = this->get_theme().get_color("line_edit.cursor");

        // Backend renders cursor
        ctx.draw_cursor(
            rect_utils::make_rect(cursor_x, cursor_y, 2, cursor_h),  // width=2 for line, full char for block
            style,
            cursor_color
        );
    }
}
```

**Backend Implementation Examples:**

**1. Terminal Backend (conio):**
```cpp
void conio_renderer::draw_cursor(rect_type rect, cursor_style style, color_type color) {
    int x = rect_utils::get_x(rect);
    int y = rect_utils::get_y(rect);

    switch (style) {
        case cursor_style::line:
            // Draw thin vertical line using box drawing character
            put_char(x, y, '│', color);
            break;

        case cursor_style::block:
            // Draw filled block
            put_char(x, y, '█', color);
            break;

        case cursor_style::underline:
            // Draw underline using box drawing character
            put_char(x, y, '▁', color);
            break;
    }
}
```

**2. GUI Backend (Qt/SDL):**
```cpp
void gui_renderer::draw_cursor(rect_type rect, cursor_style style, color_type color) {
    switch (style) {
        case cursor_style::line:
            // Draw 2px wide vertical line
            fill_rect(rect_utils::make_rect(x, y, 2, h), color);
            break;

        case cursor_style::block:
            // Draw filled rectangle covering entire character
            int char_width = get_char_width();
            fill_rect(rect_utils::make_rect(x, y, char_width, h), color);
            break;

        case cursor_style::underline:
            // Draw horizontal line at bottom
            fill_rect(rect_utils::make_rect(x, y + h - 2, char_width, 2), color);
            break;
    }
}
```

**Cursor Blinking:**

Cursor blink is managed by the widget via a timer or frame counter:

```cpp
void line_edit<Backend>::update_cursor_blink() {
    static auto last_blink = std::chrono::steady_clock::now();
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_blink);

    if (elapsed.count() >= 500) {  // Toggle every 500ms
        m_cursor_visible = !m_cursor_visible;
        last_blink = now;
        this->mark_dirty();  // Request repaint
    }
}
```

**Alternative**: Backend could handle blinking if it provides a frame callback, but widget-side is simpler.

#### Semantic Actions for Text Editing

**CRITICAL**: All keyboard operations must use semantic actions, NOT hardcoded key handling.

This allows users to customize keyboard shortcuts (e.g., Emacs-style Ctrl+F for forward, Vim-style hjkl for navigation).

**New Semantic Actions to Add** (`include/onyxui/hotkeys/hotkey_action.hh`):

```cpp
enum class hotkey_action : std::uint8_t {
    // ... existing actions ...

    // Text Editing - Cursor Movement
    cursor_move_left,           // Move cursor left one character
    cursor_move_right,          // Move cursor right one character
    cursor_move_word_left,      // Move cursor left one word (Ctrl+Left)
    cursor_move_word_right,     // Move cursor right one word (Ctrl+Right)
    cursor_move_home,           // Move cursor to start of line
    cursor_move_end,            // Move cursor to end of line

    // Text Editing - Selection
    cursor_select_left,         // Extend selection left (Shift+Left)
    cursor_select_right,        // Extend selection right (Shift+Right)
    cursor_select_word_left,    // Extend selection left one word (Ctrl+Shift+Left)
    cursor_select_word_right,   // Extend selection right one word (Ctrl+Shift+Right)
    cursor_select_home,         // Extend selection to start (Shift+Home)
    cursor_select_end,          // Extend selection to end (Shift+End)
    cursor_select_all,          // Select all text (Ctrl+A)

    // Text Editing - Deletion
    text_delete_char,           // Delete character after cursor (Delete key)
    text_backspace,             // Delete character before cursor (Backspace)
    text_delete_word,           // Delete word after cursor (Ctrl+Delete)
    text_backspace_word,        // Delete word before cursor (Ctrl+Backspace)
    text_delete_line,           // Delete entire line (Ctrl+K in Emacs)

    // Text Editing - Clipboard
    text_copy,                  // Copy selection (Ctrl+C)
    text_cut,                   // Cut selection (Ctrl+X)
    text_paste,                 // Paste from clipboard (Ctrl+V)

    // Text Editing - Undo/Redo
    text_undo,                  // Undo last change (Ctrl+Z)
    text_redo,                  // Redo last undo (Ctrl+Y or Ctrl+Shift+Z)

    // Text Editing - Mode
    text_toggle_overwrite,      // Toggle insert/overwrite mode (Insert key)

    // Text Editing - Completion
    text_accept,                // Accept input (Enter in line_edit → editing_finished)
    text_cancel,                // Cancel input (Escape)

    action_count
};
```

**Default Hotkey Scheme** (Windows-style):

```cpp
// In hotkey_scheme.cc - windows_scheme
void register_text_editing_actions(hotkey_scheme& scheme) {
    // Cursor movement
    scheme.set_binding(hotkey_action::cursor_move_left, parse_key_sequence("Left"));
    scheme.set_binding(hotkey_action::cursor_move_right, parse_key_sequence("Right"));
    scheme.set_binding(hotkey_action::cursor_move_word_left, parse_key_sequence("Ctrl+Left"));
    scheme.set_binding(hotkey_action::cursor_move_word_right, parse_key_sequence("Ctrl+Right"));
    scheme.set_binding(hotkey_action::cursor_move_home, parse_key_sequence("Home"));
    scheme.set_binding(hotkey_action::cursor_move_end, parse_key_sequence("End"));

    // Selection
    scheme.set_binding(hotkey_action::cursor_select_left, parse_key_sequence("Shift+Left"));
    scheme.set_binding(hotkey_action::cursor_select_right, parse_key_sequence("Shift+Right"));
    scheme.set_binding(hotkey_action::cursor_select_word_left, parse_key_sequence("Ctrl+Shift+Left"));
    scheme.set_binding(hotkey_action::cursor_select_word_right, parse_key_sequence("Ctrl+Shift+Right"));
    scheme.set_binding(hotkey_action::cursor_select_home, parse_key_sequence("Shift+Home"));
    scheme.set_binding(hotkey_action::cursor_select_end, parse_key_sequence("Shift+End"));
    scheme.set_binding(hotkey_action::cursor_select_all, parse_key_sequence("Ctrl+A"));

    // Deletion
    scheme.set_binding(hotkey_action::text_delete_char, parse_key_sequence("Delete"));
    scheme.set_binding(hotkey_action::text_backspace, parse_key_sequence("Backspace"));
    scheme.set_binding(hotkey_action::text_delete_word, parse_key_sequence("Ctrl+Delete"));
    scheme.set_binding(hotkey_action::text_backspace_word, parse_key_sequence("Ctrl+Backspace"));

    // Clipboard
    scheme.set_binding(hotkey_action::text_copy, parse_key_sequence("Ctrl+C"));
    scheme.set_binding(hotkey_action::text_cut, parse_key_sequence("Ctrl+X"));
    scheme.set_binding(hotkey_action::text_paste, parse_key_sequence("Ctrl+V"));

    // Undo/redo
    scheme.set_binding(hotkey_action::text_undo, parse_key_sequence("Ctrl+Z"));
    scheme.set_binding(hotkey_action::text_redo, parse_key_sequence("Ctrl+Y"));

    // Mode
    scheme.set_binding(hotkey_action::text_toggle_overwrite, parse_key_sequence("Insert"));

    // Accept/cancel
    scheme.set_binding(hotkey_action::text_accept, parse_key_sequence("Enter"));
    scheme.set_binding(hotkey_action::text_cancel, parse_key_sequence("Escape"));
}
```

**line_edit Implementation** (using semantic actions):

```cpp
bool line_edit<Backend>::handle_event(const ui_event& event, event_phase phase) override {
    if (phase != event_phase::target) {
        return widget<Backend>::handle_event(event, phase);
    }

    // Let hotkey_manager translate key to semantic action
    auto& hotkeys = ui_context<Backend>::instance().hotkeys();
    std::optional<hotkey_action> action = hotkeys.translate_key_event(event);

    if (!action) {
        // Not a hotkey - check if it's printable character input
        if (event.type == ui_event::event_type::key_press && is_printable(event.key_code)) {
            insert_character(event.character);
            return true;
        }
        return false;
    }

    // Handle semantic action
    switch (*action) {
        case hotkey_action::cursor_move_left:
            move_cursor(-1, false);
            return true;

        case hotkey_action::cursor_move_right:
            move_cursor(1, false);
            return true;

        case hotkey_action::cursor_move_home:
            set_cursor_position(0);
            return true;

        case hotkey_action::cursor_move_end:
            set_cursor_position(m_text.length());
            return true;

        case hotkey_action::cursor_select_left:
            move_cursor(-1, true);  // extend_selection = true
            return true;

        case hotkey_action::cursor_select_right:
            move_cursor(1, true);
            return true;

        case hotkey_action::cursor_select_all:
            select_all();
            return true;

        case hotkey_action::text_delete_char:
            delete_character(1);  // forward
            return true;

        case hotkey_action::text_backspace:
            delete_character(-1);  // backward
            return true;

        case hotkey_action::text_copy:
            copy();
            return true;

        case hotkey_action::text_paste:
            paste();
            return true;

        case hotkey_action::text_cut:
            cut();
            return true;

        case hotkey_action::text_undo:
            undo();
            return true;

        case hotkey_action::text_redo:
            redo();
            return true;

        case hotkey_action::text_toggle_overwrite:
            set_overwrite_mode(!m_overwrite_mode);
            return true;

        case hotkey_action::text_accept:
            // Emit editing_finished signal
            editing_finished.emit();
            return_pressed.emit();
            return true;

        case hotkey_action::text_cancel:
            // TODO: Restore original text? Or just lose focus?
            return true;

        default:
            // Not a text editing action
            return false;
    }
}
```

**Benefits of Semantic Actions:**

1. **Customizable Keybindings** - Users can remap keys without modifying line_edit code
2. **Scheme Support** - Different schemes (Windows, Emacs, Vim) work automatically
3. **Consistent UX** - All text widgets use same keybindings
4. **Testable** - Can test actions independently of key codes
5. **Documentation** - Self-documenting (action names describe intent)

**Example Custom Scheme** (Emacs-style):

```cpp
void register_emacs_text_actions(hotkey_scheme& scheme) {
    scheme.set_binding(hotkey_action::cursor_move_left, parse_key_sequence("Ctrl+B"));  // backward
    scheme.set_binding(hotkey_action::cursor_move_right, parse_key_sequence("Ctrl+F")); // forward
    scheme.set_binding(hotkey_action::cursor_move_home, parse_key_sequence("Ctrl+A"));  // beginning
    scheme.set_binding(hotkey_action::cursor_move_end, parse_key_sequence("Ctrl+E"));   // end
    scheme.set_binding(hotkey_action::text_delete_line, parse_key_sequence("Ctrl+K"));  // kill line
    // ... etc
}
```

---

### 2. checkbox

#### Visual Design

```
☐ Unchecked         ← Empty box + label
☑ Checked           ← Checked box + label
☒ Indeterminate     ← Partial check (tri-state)
□ Disabled          ← Grayed out
```

#### API Design

```cpp
enum class tri_state : uint8_t {
    unchecked,
    checked,
    indeterminate  // For hierarchical checkboxes (e.g., "select all")
};

template<UIBackend Backend>
class checkbox : public widget<Backend> {
public:
    explicit checkbox(std::string text = "", ui_element<Backend>* parent = nullptr);

    // State management
    void set_checked(bool checked);
    [[nodiscard]] bool is_checked() const noexcept { return m_state == tri_state::checked; }

    void set_tri_state(tri_state state);
    [[nodiscard]] tri_state get_tri_state() const noexcept { return m_state; }

    // Enable tri-state mode (allows indeterminate)
    void set_tri_state_enabled(bool enabled);
    [[nodiscard]] bool is_tri_state_enabled() const noexcept { return m_tri_state_enabled; }

    // Text label
    void set_text(const std::string& text);
    [[nodiscard]] const std::string& text() const noexcept { return m_text; }

    // Mnemonic (Alt+key shortcut)
    void set_mnemonic(char key);

    // Signals
    signal<bool> toggled;              // checked → unchecked or vice versa
    signal<tri_state> state_changed;   // Any state change (including indeterminate)

protected:
    void do_render(render_context<Backend>& ctx) const override;
    bool handle_event(const ui_event& event, event_phase phase) override;

private:
    std::string m_text;
    tri_state m_state = tri_state::unchecked;
    bool m_tri_state_enabled = false;
    char m_mnemonic = '\0';

    void toggle();
};
```

#### Usage Example

```cpp
// Simple checkbox
auto remember_me = std::make_unique<checkbox<Backend>>("Remember me");
remember_me->toggled.connect([](bool checked) {
    std::cout << "Remember: " << (checked ? "Yes" : "No") << "\n";
});

// Tri-state checkbox (for "select all" functionality)
auto select_all = std::make_unique<checkbox<Backend>>("Select All");
select_all->set_tri_state_enabled(true);
select_all->state_changed.connect([](tri_state state) {
    switch (state) {
        case tri_state::unchecked: std::cout << "None selected\n"; break;
        case tri_state::checked: std::cout << "All selected\n"; break;
        case tri_state::indeterminate: std::cout << "Some selected\n"; break;
    }
});

// Checkbox with mnemonic (Alt+A toggles)
auto agree = std::make_unique<checkbox<Backend>>("I &Agree");
agree->set_mnemonic('a');
```

---

### 3. radio_button + button_group

#### Visual Design

```
◉ Selected          ← Filled circle + label
◯ Unselected        ← Empty circle + label
○ Disabled          ← Grayed out
```

#### API Design

```cpp
template<UIBackend Backend>
class radio_button : public widget<Backend> {
public:
    explicit radio_button(std::string text = "", ui_element<Backend>* parent = nullptr);

    // State management
    void set_checked(bool checked);
    [[nodiscard]] bool is_checked() const noexcept { return m_is_checked; }

    // Text label
    void set_text(const std::string& text);
    [[nodiscard]] const std::string& text() const noexcept { return m_text; }

    // Mnemonic
    void set_mnemonic(char key);

    // Signals
    signal<bool> toggled;  // State changed

protected:
    void do_render(render_context<Backend>& ctx) const override;
    bool handle_event(const ui_event& event, event_phase phase) override;

private:
    std::string m_text;
    bool m_is_checked = false;
    char m_mnemonic = '\0';
    button_group<Backend>* m_group = nullptr;  // Non-owning

    friend class button_group<Backend>;
};

// Manages mutually exclusive radio buttons
template<UIBackend Backend>
class button_group {
public:
    button_group() = default;

    // Add radio button to group
    void add_button(radio_button<Backend>* button, int id = -1);
    void remove_button(radio_button<Backend>* button);

    // Get checked button
    [[nodiscard]] int checked_id() const noexcept { return m_checked_id; }
    [[nodiscard]] radio_button<Backend>* checked_button() const noexcept;

    // Set checked button
    void set_checked_button(radio_button<Backend>* button);
    void set_checked_id(int id);

    // Signals
    signal<int, bool> button_toggled;  // (id, checked)

private:
    std::unordered_map<int, radio_button<Backend>*> m_buttons;
    int m_checked_id = -1;
    int m_next_id = 0;
};
```

#### Usage Example

```cpp
// Radio button group
auto group = std::make_shared<button_group<Backend>>();

auto radio1 = std::make_unique<radio_button<Backend>>("Option 1");
auto radio2 = std::make_unique<radio_button<Backend>>("Option 2");
auto radio3 = std::make_unique<radio_button<Backend>>("Option 3");

group->add_button(radio1.get(), 1);
group->add_button(radio2.get(), 2);
group->add_button(radio3.get(), 3);
group->set_checked_id(1);  // Select first option

group->button_toggled.connect([](int id, bool checked) {
    if (checked) {
        std::cout << "Selected option " << id << "\n";
    }
});

// Add to layout
auto vbox = std::make_unique<vbox<Backend>>();
vbox->add_child(std::move(radio1));
vbox->add_child(std::move(radio2));
vbox->add_child(std::move(radio3));
```

---

### 4. progress_bar

#### Visual Design

```
Determinate (45%):
┌────────────────────────────────┐
│████████████▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒│ 45%
└────────────────────────────────┘

Indeterminate (animated):
┌────────────────────────────────┐
│▒▒▒▒▒▒▒▒████████▒▒▒▒▒▒▒▒▒▒▒▒▒▒│  ← Moving stripe
└────────────────────────────────┘

Vertical:
┌──┐
│▒▒│ ▲
│▒▒│ │
│██│ │ 60%
│██│ │
│██│ ▼
└──┘
```

#### API Design

```cpp
enum class progress_bar_orientation : uint8_t {
    horizontal,
    vertical
};

template<UIBackend Backend>
class progress_bar : public widget<Backend> {
public:
    explicit progress_bar(ui_element<Backend>* parent = nullptr);

    // Value (0-100 for determinate, ignored for indeterminate)
    void set_value(int value);  // Clamps to [min, max]
    [[nodiscard]] int value() const noexcept { return m_value; }

    // Range
    void set_range(int min, int max);
    [[nodiscard]] int minimum() const noexcept { return m_min; }
    [[nodiscard]] int maximum() const noexcept { return m_max; }

    // Indeterminate mode (busy indicator)
    void set_indeterminate(bool indeterminate);
    [[nodiscard]] bool is_indeterminate() const noexcept { return m_is_indeterminate; }

    // Orientation
    void set_orientation(progress_bar_orientation orientation);
    [[nodiscard]] progress_bar_orientation orientation() const noexcept { return m_orientation; }

    // Text overlay (e.g., "45%" or "3/10 files")
    void set_text_visible(bool visible);
    [[nodiscard]] bool is_text_visible() const noexcept { return m_text_visible; }

    void set_text_format(const std::string& format);  // e.g., "%v% complete"
    [[nodiscard]] std::string formatted_text() const;

    // Signals
    signal<int> value_changed;

protected:
    void do_render(render_context<Backend>& ctx) const override;

private:
    int m_value = 0;
    int m_min = 0;
    int m_max = 100;
    bool m_is_indeterminate = false;
    progress_bar_orientation m_orientation = progress_bar_orientation::horizontal;
    bool m_text_visible = false;
    std::string m_text_format = "%v%";  // %v = value, %p = percentage

    // Animation state for indeterminate mode
    mutable int m_animation_offset = 0;
};
```

#### Usage Example

```cpp
// Simple progress bar (file download)
auto progress = std::make_unique<progress_bar<Backend>>();
progress->set_range(0, 100);
progress->set_value(45);
progress->set_text_visible(true);

// Update progress
download_manager.progress_changed.connect([&progress](int percent) {
    progress->set_value(percent);
});

// Indeterminate progress (loading spinner)
auto busy = std::make_unique<progress_bar<Backend>>();
busy->set_indeterminate(true);

// Vertical progress bar
auto volume_indicator = std::make_unique<progress_bar<Backend>>();
volume_indicator->set_orientation(progress_bar_orientation::vertical);
volume_indicator->set_range(0, 100);
volume_indicator->set_value(75);
```

---

### 5. slider

#### Visual Design

```
Horizontal:
   0     25     50     75    100
   |------●------|------|------|   ← Thumb at 25%

Vertical:
100 ┬──
    │
 75 ┼──
    │
 50 ┼──  ●  ← Thumb at 40%
    │
 25 ┼──
    │
  0 ┴──

With tick marks:
   |------|------|------|------|
   0     25     50     75    100
```

#### API Design

```cpp
enum class slider_orientation : uint8_t {
    horizontal,
    vertical
};

template<UIBackend Backend>
class slider : public widget<Backend> {
public:
    explicit slider(ui_element<Backend>* parent = nullptr);

    // Value
    void set_value(int value);
    [[nodiscard]] int value() const noexcept { return m_value; }

    // Range
    void set_range(int min, int max);
    [[nodiscard]] int minimum() const noexcept { return m_min; }
    [[nodiscard]] int maximum() const noexcept { return m_max; }

    // Step size (snap to multiples)
    void set_single_step(int step);  // Arrow key increment
    void set_page_step(int step);    // Page Up/Down increment
    [[nodiscard]] int single_step() const noexcept { return m_single_step; }
    [[nodiscard]] int page_step() const noexcept { return m_page_step; }

    // Orientation
    void set_orientation(slider_orientation orientation);
    [[nodiscard]] slider_orientation orientation() const noexcept { return m_orientation; }

    // Tick marks
    void set_tick_position(tick_position position);  // above/below/both/none
    void set_tick_interval(int interval);  // E.g., 25 for marks at 0, 25, 50, 75, 100

    // Value label
    void set_value_visible(bool visible);  // Show current value near thumb

    // Signals
    signal<int> value_changed;      // Value changed (any method)
    signal<int> slider_moved;       // User dragged slider
    signal<> slider_pressed;        // Mouse down on thumb
    signal<> slider_released;       // Mouse up on thumb

protected:
    void do_render(render_context<Backend>& ctx) const override;
    bool handle_event(const ui_event& event, event_phase phase) override;

private:
    int m_value = 0;
    int m_min = 0;
    int m_max = 100;
    int m_single_step = 1;
    int m_page_step = 10;
    slider_orientation m_orientation = slider_orientation::horizontal;
    tick_position m_tick_position = tick_position::none;
    int m_tick_interval = 0;
    bool m_value_visible = false;

    // Drag state
    bool m_is_dragging = false;
    int m_drag_start_value = 0;

    // Helpers
    int position_to_value(int x, int y) const;
    typename Backend::rect_type thumb_rect() const;
};
```

#### Usage Example

```cpp
// Volume slider (0-100)
auto volume = std::make_unique<slider<Backend>>();
volume->set_range(0, 100);
volume->set_value(75);
volume->set_value_visible(true);
volume->value_changed.connect([](int value) {
    audio_system::set_volume(value);
});

// Opacity slider (0.0 - 1.0, using int 0-100 scaled)
auto opacity = std::make_unique<slider<Backend>>();
opacity->set_range(0, 100);
opacity->set_single_step(5);
opacity->set_tick_interval(25);
opacity->value_changed.connect([](int value) {
    set_opacity(value / 100.0);
});

// Vertical slider (like mixer fader)
auto fader = std::make_unique<slider<Backend>>();
fader->set_orientation(slider_orientation::vertical);
fader->set_range(-60, 12);  // dB range
fader->set_value(0);  // Unity gain
```

---

### 6. tab_widget

#### Visual Design

```
┌────────┬────────┬────────┬─────────────────────┐
│ Tab 1  │ Tab 2* │ Tab 3  │                     │  ← Tab bar
├────────┴────────┴────────┴─────────────────────┤
│                                                 │
│           Content for Tab 2                     │  ← Active tab content
│                                                 │
│                                                 │
└─────────────────────────────────────────────────┘

* = active tab (highlighted)

With close buttons:
┌──────────┬──────────┬──────────┐
│ Tab 1 ✕  │ Tab 2* ✕ │ Tab 3 ✕  │
└──────────┴──────────┴──────────┘
```

#### API Design

```cpp
enum class tab_position : uint8_t {
    top,
    bottom,
    left,
    right
};

template<UIBackend Backend>
class tab_widget : public widget_container<Backend> {
public:
    explicit tab_widget(ui_element<Backend>* parent = nullptr);

    // Tab management
    int add_tab(std::unique_ptr<ui_element<Backend>> widget, const std::string& label);
    int insert_tab(int index, std::unique_ptr<ui_element<Backend>> widget, const std::string& label);
    void remove_tab(int index);
    void clear();

    [[nodiscard]] int count() const noexcept { return static_cast<int>(m_tabs.size()); }

    // Current tab
    void set_current_index(int index);
    [[nodiscard]] int current_index() const noexcept { return m_current_index; }
    [[nodiscard]] ui_element<Backend>* current_widget() const;

    // Tab labels
    void set_tab_text(int index, const std::string& text);
    [[nodiscard]] std::string tab_text(int index) const;

    // Tab icons (future: when icon support added)
    // void set_tab_icon(int index, const icon& icon);

    // Tab position
    void set_tab_position(tab_position position);
    [[nodiscard]] tab_position get_tab_position() const noexcept { return m_tab_position; }

    // Close buttons
    void set_tabs_closable(bool closable);
    [[nodiscard]] bool tabs_closable() const noexcept { return m_tabs_closable; }

    // Signals
    signal<int> current_changed;        // Tab switched
    signal<int> tab_close_requested;    // Close button clicked (doesn't auto-remove)

protected:
    void do_render(render_context<Backend>& ctx) const override;
    bool handle_event(const ui_event& event, event_phase phase) override;

private:
    struct tab_info {
        std::string label;
        std::unique_ptr<ui_element<Backend>> widget;
    };

    std::vector<tab_info> m_tabs;
    int m_current_index = -1;
    tab_position m_tab_position = tab_position::top;
    bool m_tabs_closable = false;

    // Internal tab bar widget
    tab_bar<Backend>* m_tab_bar = nullptr;  // Owned by children
};
```

#### Usage Example

```cpp
// Simple tab widget
auto tabs = std::make_unique<tab_widget<Backend>>();

// Add tabs
auto page1 = std::make_unique<vbox<Backend>>();
page1->emplace_child<label>("Page 1 content");
tabs->add_tab(std::move(page1), "General");

auto page2 = std::make_unique<vbox<Backend>>();
page2->emplace_child<label>("Page 2 content");
tabs->add_tab(std::move(page2), "Advanced");

auto page3 = std::make_unique<vbox<Backend>>();
page3->emplace_child<label>("Page 3 content");
tabs->add_tab(std::move(page3), "About");

// Handle tab changes
tabs->current_changed.connect([](int index) {
    std::cout << "Switched to tab " << index << "\n";
});

// Closeable tabs
auto editor = std::make_unique<tab_widget<Backend>>();
editor->set_tabs_closable(true);
editor->tab_close_requested.connect([&editor](int index) {
    // Confirm before closing
    if (confirm_close(index)) {
        editor->remove_tab(index);
    }
});
```

---

### 7. splitter

#### Visual Design

```
Horizontal splitter:
┌──────────────────┬───────────────────────┐
│                  │                       │
│   Left Panel     ║   Right Panel         │  ║ = draggable handle
│                  │                       │
└──────────────────┴───────────────────────┘

Vertical splitter:
┌───────────────────────────────────────────┐
│           Top Panel                       │
╠═══════════════════════════════════════════╣  ═ = draggable handle
│           Bottom Panel                    │
└───────────────────────────────────────────┘

Three-way split:
┌──────┬────────────┬────────┐
│  A   │     B      │   C    │
└──────┴────────────┴────────┘
```

#### API Design

```cpp
enum class splitter_orientation : uint8_t {
    horizontal,  // Left/right panels
    vertical     // Top/bottom panels
};

template<UIBackend Backend>
class splitter : public widget_container<Backend> {
public:
    explicit splitter(splitter_orientation orientation = splitter_orientation::horizontal,
                      ui_element<Backend>* parent = nullptr);

    // Add widgets
    void add_widget(std::unique_ptr<ui_element<Backend>> widget);
    void insert_widget(int index, std::unique_ptr<ui_element<Backend>> widget);

    // Orientation
    void set_orientation(splitter_orientation orientation);
    [[nodiscard]] splitter_orientation orientation() const noexcept { return m_orientation; }

    // Splitter sizes (proportions or absolute pixels)
    void set_sizes(const std::vector<int>& sizes);
    [[nodiscard]] std::vector<int> sizes() const;

    // Widget at index
    [[nodiscard]] ui_element<Backend>* widget(int index) const;
    [[nodiscard]] int count() const noexcept { return static_cast<int>(this->children().size()); }

    // Collapsible
    void set_collapsible(int index, bool collapsible);
    [[nodiscard]] bool is_collapsible(int index) const;

    // Handle width
    void set_handle_width(int width);
    [[nodiscard]] int handle_width() const noexcept { return m_handle_width; }

    // Signals
    signal<int, int> splitter_moved;  // (handle_index, new_position)

protected:
    void do_render(render_context<Backend>& ctx) const override;
    bool handle_event(const ui_event& event, event_phase phase) override;

private:
    splitter_orientation m_orientation;
    int m_handle_width = 4;  // Width of draggable handle
    std::vector<int> m_sizes;  // Size of each panel
    std::vector<bool> m_collapsible;  // Which panels can collapse

    // Drag state
    bool m_is_dragging = false;
    int m_drag_handle_index = -1;  // Which handle is being dragged
    int m_drag_start_pos = 0;

    // Helpers
    int handle_at_position(int x, int y) const;  // Which handle (if any) at mouse pos
    typename Backend::rect_type handle_rect(int index) const;
};
```

#### Usage Example

```cpp
// Horizontal splitter (left/right panels)
auto splitter = std::make_unique<splitter<Backend>>(splitter_orientation::horizontal);

auto left_panel = std::make_unique<vbox<Backend>>();
left_panel->emplace_child<label>("File browser");

auto right_panel = std::make_unique<vbox<Backend>>();
right_panel->emplace_child<label>("Editor");

splitter->add_widget(std::move(left_panel));
splitter->add_widget(std::move(right_panel));

// Set initial sizes (30% left, 70% right)
splitter->set_sizes({30, 70});

// Handle splitter moves
splitter->splitter_moved.connect([](int handle, int pos) {
    save_splitter_position(handle, pos);
});

// Three-way split
auto three_way = std::make_unique<splitter<Backend>>(splitter_orientation::horizontal);
three_way->add_widget(create_panel("A"));
three_way->add_widget(create_panel("B"));
three_way->add_widget(create_panel("C"));
three_way->set_sizes({25, 50, 25});  // 25%, 50%, 25%
```

---

## Integration Requirements

### Theme System Integration

All widgets must integrate with OnyxUI's theming system:

```cpp
// Each widget should override get_theme_* methods
[[nodiscard]] typename Backend::color_type get_theme_background_color() const override {
    // Query theme for widget-specific colors
    const auto& theme = this->get_theme();
    if (is_enabled()) {
        return theme.get_color("text_edit.background");
    } else {
        return theme.get_color("text_edit.background_disabled");
    }
}

[[nodiscard]] typename Backend::color_type get_theme_foreground_color() const override {
    const auto& theme = this->get_theme();
    return theme.get_color("text_edit.text");
}
```

**Required theme properties for new widgets:**
- `line_edit.background`, `line_edit.text`, `line_edit.cursor`, `line_edit.selection`, `line_edit.placeholder`
- `checkbox.background`, `checkbox.checkmark`, `checkbox.border`
- `radio_button.background`, `radio_button.dot`, `radio_button.border`
- `progress_bar.background`, `progress_bar.fill`, `progress_bar.text`
- `slider.background`, `slider.thumb`, `slider.groove`
- `tab_widget.tab_background`, `tab_widget.tab_active`, `tab_widget.tab_text`
- `splitter.handle`, `splitter.handle_hover`

### Focus Management Integration

All input widgets must integrate with `focus_manager`:

```cpp
template<UIBackend Backend>
class line_edit : public widget<Backend> {
public:
    line_edit(...) {
        // Input widgets are focusable by default
        this->set_focusable(true);
    }

protected:
    bool handle_event(const ui_event& event, event_phase phase) override {
        if (phase == event_phase::target && event.type == ui_event::event_type::mouse_press) {
            // Request focus on click
            this->request_focus();
            return true;
        }
        return widget<Backend>::handle_event(event, phase);
    }
};
```

**Focus order:**
- Tab key cycles through focusable widgets (managed by focus_manager)
- Shift+Tab cycles backward
- Radio buttons: Arrow keys navigate within group (don't change focus)

### Hotkey Integration

Some widgets should respond to global hotkeys:

```cpp
// Register widget-specific hotkeys
ctx.hotkeys().register_semantic_action(
    hotkey_action::activate_focused,
    [&checkbox]() {
        if (checkbox->has_focus()) {
            checkbox->toggle();
        }
    }
);
```

### Signal System Integration

All widgets use OnyxUI's `signal<>` template:

```cpp
signal<const std::string&> text_changed;  // Typed signal

// Usage
text_edit->text_changed.connect([](const std::string& text) {
    std::cout << text << "\n";
});

// Scoped connection (RAII cleanup)
scoped_connection conn(text_edit->text_changed, my_handler);
```

---

## Testing Strategy

### Unit Test Structure

Each widget gets comprehensive tests:

**File**: `unittest/widgets/test_line_edit.cc`

```cpp
TEST_CASE("line_edit - Construction and basic properties") {
    ui_context_fixture<test_canvas_backend> fixture;

    auto edit = std::make_unique<line_edit<test_canvas_backend>>();

    REQUIRE(edit->text() == "");
    REQUIRE(edit->cursor_position() == 0);
    REQUIRE_FALSE(edit->has_selection());
}

TEST_CASE("line_edit - Set and get text") {
    ui_context_fixture<test_canvas_backend> fixture;

    auto edit = std::make_unique<line_edit<test_canvas_backend>>();
    edit->set_text("Hello");

    REQUIRE(edit->text() == "Hello");
    REQUIRE(edit->cursor_position() == 0);  // Setting text doesn't move cursor
}

TEST_CASE("line_edit - Text changed signal") {
    ui_context_fixture<test_canvas_backend> fixture;

    auto edit = std::make_unique<line_edit<test_canvas_backend>>();

    std::string last_text;
    edit->text_changed.connect([&last_text](const std::string& text) {
        last_text = text;
    });

    edit->set_text("Test");
    REQUIRE(last_text == "Test");
}

TEST_CASE("line_edit - Enter key emits editing_finished") {
    ui_context_fixture<test_canvas_backend> fixture;

    auto edit = std::make_unique<line_edit<test_canvas_backend>>("Test");

    bool editing_finished_called = false;
    edit->editing_finished.connect([&editing_finished_called]() {
        editing_finished_called = true;
    });

    // Simulate Enter key press
    ui_event enter_event;
    enter_event.type = ui_event::event_type::key_press;
    enter_event.key_code = key_code::enter;
    edit->handle_event(enter_event, event_phase::target);

    REQUIRE(editing_finished_called);
    REQUIRE(edit->text() == "Test");  // Text unchanged (no newline inserted)
}

TEST_CASE("line_edit - Newlines not allowed") {
    ui_context_fixture<test_canvas_backend> fixture;

    auto edit = std::make_unique<line_edit<test_canvas_backend>>();
    edit->set_text("Line 1\nLine 2");  // Try to insert newline

    // Newlines should be stripped or rejected
    REQUIRE(edit->text().find('\n') == std::string::npos);
}

TEST_CASE("line_edit - Cursor movement") {
    ui_context_fixture<test_canvas_backend> fixture;

    auto edit = std::make_unique<line_edit<test_canvas_backend>>("Hello");
    edit->set_cursor_position(5);  // End of text

    REQUIRE(edit->cursor_position() == 5);

    // Test clamping
    edit->set_cursor_position(100);
    REQUIRE(edit->cursor_position() == 5);  // Clamped to text length
}

TEST_CASE("line_edit - Password mode") {
    ui_context_fixture<test_canvas_backend> fixture;

    auto edit = std::make_unique<line_edit<test_canvas_backend>>("Secret");
    edit->set_password_mode(true);

    REQUIRE(edit->is_password_mode());
    REQUIRE(edit->text() == "Secret");  // Internal text unchanged

    // TODO: Render and verify dots are displayed instead of text
}

TEST_CASE("line_edit - Read-only mode") {
    ui_context_fixture<test_canvas_backend> fixture;

    auto edit = std::make_unique<line_edit<test_canvas_backend>>("Read-only");
    edit->set_read_only(true);

    // TODO: Simulate keyboard input and verify text doesn't change
}

TEST_CASE("line_edit - Validation") {
    ui_context_fixture<test_canvas_backend> fixture;

    auto edit = std::make_unique<line_edit<test_canvas_backend>>();
    edit->set_validator([](const std::string& text) {
        return text.length() <= 10;  // Max 10 chars
    });

    edit->set_text("Short");
    REQUIRE(edit->is_valid());

    edit->set_text("This is too long");
    REQUIRE_FALSE(edit->is_valid());
}

TEST_CASE("line_edit - Insert vs Overwrite mode") {
    ui_context_fixture<test_canvas_backend> fixture;

    auto edit = std::make_unique<line_edit<test_canvas_backend>>("Hello");
    edit->set_cursor_position(5);  // After "Hello"

    // Default: insert mode
    REQUIRE_FALSE(edit->is_overwrite_mode());

    // TODO: Simulate typing "!" in insert mode
    // Expected: "Hello!" (inserted at end)

    // Toggle to overwrite mode
    edit->set_overwrite_mode(true);
    REQUIRE(edit->is_overwrite_mode());

    // TODO: Simulate Insert key press to toggle back
    // Expected: m_overwrite_mode == false
}

TEST_CASE("line_edit - Cursor rendering state") {
    ui_context_fixture<test_canvas_backend> fixture;

    auto edit = std::make_unique<line_edit<test_canvas_backend>>("Test");

    // Cursor should blink when focused
    edit->request_focus();
    REQUIRE(edit->has_focus());

    // TODO: Verify cursor is rendered during do_render()
    // TODO: Verify cursor style changes based on overwrite mode
    //   - Insert mode: cursor_style::line
    //   - Overwrite mode: cursor_style::block
}
```

### Test Coverage Requirements

Each widget must have:
- ✅ Construction tests
- ✅ Property getter/setter tests
- ✅ Signal emission tests
- ✅ Event handling tests (mouse, keyboard)
- ✅ Focus integration tests
- ✅ Theme integration tests
- ✅ Edge case tests (empty, max values, disabled state)
- ✅ Regression tests for bugs

### Integration Tests

Test widgets in realistic scenarios:

```cpp
TEST_CASE("Form - Complete login form with line_edit and checkbox") {
    ui_context_fixture<test_canvas_backend> fixture;

    auto form = std::make_unique<vbox<test_canvas_backend>>();

    auto username = form->template emplace_child<line_edit<test_canvas_backend>>();
    username->set_placeholder("Username");

    auto password = form->template emplace_child<line_edit<test_canvas_backend>>();
    password->set_password_mode(true);
    password->set_placeholder("Password");

    auto remember = form->template emplace_child<checkbox<test_canvas_backend>>("Remember me");

    auto login_btn = form->template emplace_child<button<test_canvas_backend>>("Login");

    // Test form submission
    bool login_called = false;
    login_btn->clicked.connect([&]() {
        login_called = true;
        REQUIRE(username->text() == "alice");
        REQUIRE(password->text() == "secret123");
        REQUIRE(remember->is_checked());
    });

    // Simulate user input
    username->set_text("alice");
    password->set_text("secret123");
    remember->set_checked(true);
    login_btn->clicked.emit();

    REQUIRE(login_called);
}
```

---

## Future Widgets

These widgets are **lower priority** but may be needed later:

### Nice-to-Have Widgets (Phase 6+)

1. **text_edit** - **Multi-line text editor** (notes, comments, code editing)
   - Enter key inserts `\n` (does NOT submit)
   - Vertical scrolling support
   - Line wrapping (optional)
   - Syntax highlighting (advanced)
2. **spin_box** - Numeric input with up/down buttons
3. **date_edit** - Date picker calendar
4. **time_edit** - Time picker (hours:minutes)
5. **combo_box** - Dropdown (WAIT FOR MVC - uses list_view internally)
6. **list_widget** - Simple list (WAIT FOR MVC - non-editable list_view)
7. **tree_widget** - Hierarchical tree (WAIT FOR MVC)
8. **toolbar** - Horizontal button bar with icons
9. **menu_button** - Button with dropdown menu
10. **color_picker** - Color selection dialog
11. **file_dialog** - File open/save dialog

### Platform-Specific Widgets

These may require backend-specific implementations:

- **native_file_dialog** - OS file picker
- **native_color_picker** - OS color picker
- **native_message_box** - OS message box
- **system_tray_icon** - Taskbar/tray icon

---

## Implementation Timeline

### 5-Week Plan (Critical Path)

**Week 1: line_edit** (Single-line text input)
- Days 1-2: Core text editing (insert, delete, cursor, horizontal scroll)
- Days 3-4: Selection, clipboard, undo/redo
- Day 5: Tests, polish, documentation
- **Key**: Enter key emits editing_finished (no newlines allowed)

**Week 2: checkbox + radio_button**
- Days 1-2: checkbox (basic toggle, tri-state)
- Days 3-4: radio_button + button_group
- Day 5: Tests, polish, documentation

**Week 3: progress_bar + slider**
- Days 1-2: progress_bar (determinate, indeterminate)
- Days 3-4: slider (drag, keyboard, ticks)
- Day 5: Tests, polish, documentation

**Week 4: tab_widget**
- Days 1-3: tab_widget + tab_bar
- Day 4: Close buttons, keyboard nav
- Day 5: Tests, polish, documentation

**Week 5: splitter**
- Days 1-3: splitter (drag, resize, collapse)
- Day 4: Multi-split support
- Day 5: Tests, polish, documentation

**Total**: 5 weeks to complete all critical widgets

---

## Success Criteria

### After Week 3 (line_edit, checkbox, radio_button, progress_bar, slider)

Users can build:
- ✅ Login forms (username/password fields)
- ✅ Settings panels (toggles and text inputs)
- ✅ Preference dialogs (all input types)
- ✅ File upload progress (progress bars)
- ✅ Volume controls (sliders)

### After Week 5 (+ tab_widget, splitter)

Users can build:
- ✅ Multi-page preference dialogs
- ✅ Code editors with split views
- ✅ File managers with resizable panels
- ✅ Complete desktop applications

### Widget Library Completeness

After this roadmap, OnyxUI will have:
- **17 widgets** total (up from 11)
- **100% coverage** of common UI patterns
- **Production-ready** framework for real applications

---

## Next Steps

**Recommended**: Start with **Phase 1: line_edit** (Week 1)

1. Create `include/onyxui/widgets/input/line_edit.hh`
2. Implement core single-line text editing (TDD approach)
3. Add selection, cursor blinking, horizontal scrolling
4. **Ensure Enter key emits editing_finished (no newlines)**
5. Integrate with focus_manager and theming
6. Write comprehensive tests
7. Add to widgets demo for visual testing

**After line_edit is complete**, proceed with checkbox + radio_button in Week 2.

**Multi-line text_edit** (Enter inserts `\n`) deferred to Phase 6+ (future widgets).

**Questions**:
- Should we start implementing line_edit now?
- Any specific features missing from the widget specs?
- Do you have mockups or design preferences for widget appearance?

---

## line_edit vs text_edit: Key Differences

| Feature | line_edit (Week 1) | text_edit (Future) |
|---------|-------------------|-------------------|
| **Lines** | Single line only | Multiple lines |
| **Enter key** | Emits `editing_finished` | Inserts `\n` |
| **Scrolling** | Horizontal only | Vertical + horizontal |
| **Use cases** | Forms, dialogs, search | Notes, comments, code |
| **Complexity** | Simple (Week 1) | Complex (Phase 6+) |
| **Priority** | Critical | Nice-to-have |
