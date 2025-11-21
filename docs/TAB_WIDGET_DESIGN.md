# Tab Widget Design Document

**Version:** 1.0
**Date:** 2025-11-20
**Status:** Design Phase
**Author:** Architecture Team

---

## Table of Contents

1. [Overview](#overview)
2. [Requirements](#requirements)
3. [Architecture](#architecture)
4. [Visual Design](#visual-design)
5. [API Design](#api-design)
6. [Implementation Details](#implementation-details)
7. [Theme Integration](#theme-integration)
8. [Event Handling](#event-handling)
9. [Testing Strategy](#testing-strategy)
10. [Usage Examples](#usage-examples)
11. [Open Questions](#open-questions)

---

## Overview

### Purpose

The `tab_widget` provides a multi-page container widget that allows users to organize content into multiple tabbed pages. Users can switch between pages by clicking on tabs or using keyboard shortcuts (Ctrl+Tab, Ctrl+Shift+Tab).

### Key Features

- **Multiple Pages**: Each tab contains its own widget content
- **Tab Bar**: Visual tab selector at top/bottom/left/right
- **Tab Labels**: Text labels for each tab
- **Closeable Tabs**: Optional close button (X) on each tab
- **Keyboard Navigation**: Ctrl+Tab to cycle tabs
- **Current Tab Highlighting**: Visual indication of active tab
- **Dynamic Management**: Add/remove tabs at runtime

### Use Cases

1. **Settings Dialog**: Multiple preference pages (General, Advanced, About)
2. **Code Editor**: Multiple open files in tabs
3. **Browser**: Multiple web pages in tabs
4. **Document Viewer**: Multiple documents in tabs
5. **Multi-View Application**: Different views in tabs (Files, Search, Debug)

### Non-Goals (Out of Scope)

- Tab reordering via drag-and-drop (future enhancement)
- Tab icons (future enhancement, requires icon system)
- Tab scrolling for overflow (future enhancement)
- Detachable tabs (future enhancement)
- Tab groups/nesting (future enhancement)

---

## Requirements

### Functional Requirements

**FR1: Tab Management**
- Add new tabs with label and content widget
- Remove tabs by index
- Clear all tabs
- Insert tabs at specific position
- Get tab count

**FR2: Tab Selection**
- Set current tab by index
- Get current tab index
- Get current tab widget
- Navigate to next/previous tab

**FR3: Tab Labels**
- Set/get tab text by index
- Support for long tab labels (truncation or wrapping)

**FR4: Tab Position**
- Support tab bar at top (default), bottom, left, or right
- Content area adjusts based on tab position

**FR5: Closeable Tabs**
- Enable/disable close buttons globally
- Close button on each tab
- Emit signal when close requested (doesn't auto-remove)

**FR6: Keyboard Navigation**
- Ctrl+Tab: Next tab
- Ctrl+Shift+Tab: Previous tab
- Ctrl+W: Close current tab (if closeable)
- Alt+Number: Jump to tab by index (Alt+1, Alt+2, etc.)

**FR7: Signals**
- `current_changed(int index)` - Tab switched
- `tab_close_requested(int index)` - Close button clicked

### Non-Functional Requirements

**NFR1: Performance**
- Only render current tab content (don't render hidden tabs)
- Tab switching should be instant (no lag)

**NFR2: Theme Integration**
- Tab colors (active, inactive, hover)
- Tab borders and spacing
- Close button colors

**NFR3: Focus Management**
- Tab bar itself is focusable
- Tab content widgets can have focus
- Tab key cycles through widgets in current tab

**NFR4: Layout Integration**
- Proper measure/arrange integration
- Tab bar size adapts to tab count
- Content area fills remaining space

---

## Architecture

### Class Hierarchy

```
ui_element<Backend>
  └── widget<Backend>
        └── widget_container<Backend>
              └── tab_widget<Backend>

ui_element<Backend>
  └── widget<Backend>
        └── widget_container<Backend>
              └── tab_bar<Backend>  (internal helper)
```

### Composition

The `tab_widget` is composed of:
1. **tab_bar** - Visual tab selector (internal child widget)
   - **scroll_arrow_left** - Left scroll button (optional, shown on overflow)
   - **scroll_arrow_right** - Right scroll button (optional, shown on overflow)
   - **tab_buttons** - Clickable tab buttons (rendered directly)
2. **content_area** - Container for current tab content (internal child widget)
3. **tab_info** - Metadata for each tab (label, widget pointer)

```
┌─────────────────────────────────────────────┐
│ tab_widget                                  │
│ ┌─────────────────────────────────────────┐ │
│ │ tab_bar (child widget)                  │ │
│ │  ┌─┬──────┬──────┬──────┬─┐             │ │
│ │  │<│Tab 1 │Tab 2*│Tab 3 │>│             │ │
│ │  └─┴──────┴──────┴──────┴─┘             │ │
│ │   ↑                      ↑               │ │
│ │   Scroll arrows (if overflow)            │ │
│ └─────────────────────────────────────────┘ │
│ ┌─────────────────────────────────────────┐ │
│ │ content_area (child widget)             │ │
│ │                                         │ │
│ │  Current tab's content widget           │ │
│ │                                         │ │
│ └─────────────────────────────────────────┘ │
└─────────────────────────────────────────────┘
```

### Data Structures

```cpp
template<UIBackend Backend>
class tab_widget : public widget_container<Backend> {
private:
    struct tab_info {
        std::string label;                           // Tab text
        std::unique_ptr<ui_element<Backend>> widget; // Tab content
        bool closeable = true;                       // Can this tab be closed?
    };

    std::vector<tab_info> m_tabs;         // All tabs
    int m_current_index = -1;             // Active tab (-1 = none)
    tab_position m_tab_position = tab_position::top;
    bool m_tabs_closable = false;         // Global close button enable

    tab_bar<Backend>* m_tab_bar = nullptr;    // Non-owning pointer to child
    panel<Backend>* m_content_area = nullptr; // Non-owning pointer to child
};
```

### Ownership Model

**Ownership Chain:**
- `tab_widget` owns `m_tabs` vector
- Each `tab_info` owns its content widget via `std::unique_ptr`
- `tab_widget` (as `widget_container`) owns `tab_bar` and `content_area` as children
- `content_area` has non-owning pointer to current tab's content widget

**Current Tab Display:**
- Only the current tab's widget is a child of `content_area`
- When switching tabs:
  1. Remove old tab's widget from `content_area` (releases child)
  2. Add new tab's widget to `content_area` (borrows from `tab_info`)
  3. Invalidate layout

---

## Visual Design

### Tab Positions

#### Top (Default)
```
┌──────┬──────┬──────┬──────────────────────┐
│ Tab1 │ Tab2*│ Tab3 │                      │ ← Tab bar
├──────┴──────┴──────┴──────────────────────┤
│                                           │
│           Content Area                    │
│                                           │
└───────────────────────────────────────────┘
```

#### Bottom
```
┌───────────────────────────────────────────┐
│                                           │
│           Content Area                    │
│                                           │
├──────┬──────┬──────┬──────────────────────┤
│ Tab1 │ Tab2*│ Tab3 │                      │ ← Tab bar
└──────┴──────┴──────┴──────────────────────┘
```

#### Left
```
┌─────┬───────────────────────────────────┐
│ T1  │                                   │
├─────┤                                   │
│ T2* │         Content Area              │
├─────┤                                   │
│ T3  │                                   │
└─────┴───────────────────────────────────┘
  ↑
Tab bar
```

#### Right
```
┌───────────────────────────────────┬─────┐
│                                   │ T1  │
│                                   ├─────┤
│         Content Area              │ T2* │
│                                   ├─────┤
│                                   │ T3  │
└───────────────────────────────────┴─────┘
                                      ↑
                                   Tab bar
```

### Tab States

Each tab can be in one of these visual states:

1. **Normal** - Inactive tab
2. **Active** - Current tab (highlighted)
3. **Hovered** - Mouse over inactive tab
4. **Pressed** - Mouse down on tab
5. **Disabled** - Tab cannot be selected (future)

### Tab Anatomy

```
Horizontal Tab (top/bottom):
┌─────────────────┐
│ Tab Label    [X]│  ← Close button (optional)
└─────────────────┘
  ↑           ↑
  Label      Close button

Vertical Tab (left/right):
┌────┐
│ T  │
│ a  │
│ b  │  ← Rotated text (or abbreviated)
│    │
│[X] │  ← Close button (optional)
└────┘
```

### Close Button

**Position:**
- Horizontal tabs: Right side of tab
- Vertical tabs: Bottom of tab

**Behavior:**
- Click emits `tab_close_requested(index)`
- Does NOT automatically remove tab
- Application decides whether to close (e.g., prompt for unsaved changes)

---

## API Design

### Enums

```cpp
/**
 * @brief Position of tab bar relative to content area
 */
enum class tab_position : std::uint8_t {
    top,     ///< Tabs at top (default)
    bottom,  ///< Tabs at bottom
    left,    ///< Tabs on left side
    right    ///< Tabs on right side
};
```

### Main Class

```cpp
template<UIBackend Backend>
class tab_widget : public widget_container<Backend> {
public:
    /**
     * @brief Construct tab widget
     * @param parent Parent widget
     */
    explicit tab_widget(ui_element<Backend>* parent = nullptr);

    // ================================================================
    // Tab Management
    // ================================================================

    /**
     * @brief Add tab at end with label and content
     * @param widget Content widget (ownership transferred)
     * @param label Tab label text
     * @return Index of new tab
     */
    int add_tab(std::unique_ptr<ui_element<Backend>> widget, const std::string& label);

    /**
     * @brief Insert tab at specific position
     * @param index Position to insert (0-based)
     * @param widget Content widget (ownership transferred)
     * @param label Tab label text
     * @return Index of inserted tab
     */
    int insert_tab(int index, std::unique_ptr<ui_element<Backend>> widget, const std::string& label);

    /**
     * @brief Remove tab by index
     * @param index Tab index (0-based)
     * @warning Invalidates indices >= index
     */
    void remove_tab(int index);

    /**
     * @brief Remove all tabs
     */
    void clear();

    /**
     * @brief Get number of tabs
     * @return Tab count
     */
    [[nodiscard]] int count() const noexcept { return static_cast<int>(m_tabs.size()); }

    // ================================================================
    // Current Tab Selection
    // ================================================================

    /**
     * @brief Set current tab by index
     * @param index Tab index (0-based, -1 = none)
     */
    void set_current_index(int index);

    /**
     * @brief Get current tab index
     * @return Current index (-1 if no tabs)
     */
    [[nodiscard]] int current_index() const noexcept { return m_current_index; }

    /**
     * @brief Get current tab's content widget
     * @return Widget pointer or nullptr if no tabs
     */
    [[nodiscard]] ui_element<Backend>* current_widget() const;

    /**
     * @brief Navigate to next tab (wraps around)
     */
    void next_tab();

    /**
     * @brief Navigate to previous tab (wraps around)
     */
    void previous_tab();

    // ================================================================
    // Tab Properties
    // ================================================================

    /**
     * @brief Set tab label text
     * @param index Tab index
     * @param text New label text
     */
    void set_tab_text(int index, const std::string& text);

    /**
     * @brief Get tab label text
     * @param index Tab index
     * @return Label text
     */
    [[nodiscard]] std::string tab_text(int index) const;

    /**
     * @brief Get tab's content widget
     * @param index Tab index
     * @return Widget pointer or nullptr if invalid index
     */
    [[nodiscard]] ui_element<Backend>* widget(int index) const;

    // ================================================================
    // Tab Bar Configuration
    // ================================================================

    /**
     * @brief Set tab bar position
     * @param position Where to place tab bar (top/bottom/left/right)
     */
    void set_tab_position(tab_position position);

    /**
     * @brief Get tab bar position
     * @return Current position
     */
    [[nodiscard]] tab_position get_tab_position() const noexcept { return m_tab_position; }

    // ================================================================
    // Close Buttons
    // ================================================================

    /**
     * @brief Enable/disable close buttons on all tabs
     * @param closable True to show close buttons
     */
    void set_tabs_closable(bool closable);

    /**
     * @brief Check if tabs have close buttons
     * @return True if close buttons enabled
     */
    [[nodiscard]] bool tabs_closable() const noexcept { return m_tabs_closable; }

    /**
     * @brief Set whether specific tab can be closed
     * @param index Tab index
     * @param closeable True if this tab can be closed
     */
    void set_tab_closeable(int index, bool closeable);

    /**
     * @brief Check if specific tab can be closed
     * @param index Tab index
     * @return True if tab is closeable
     */
    [[nodiscard]] bool is_tab_closeable(int index) const;

    // ================================================================
    // Signals
    // ================================================================

    /**
     * @brief Emitted when current tab changes
     * @param index New current index
     */
    signal<int> current_changed;

    /**
     * @brief Emitted when close button clicked
     * @param index Tab index to close
     * @note Does not automatically remove tab - application must call remove_tab()
     */
    signal<int> tab_close_requested;

protected:
    void do_render(render_context<Backend>& ctx) const override;
    bool handle_event(const ui_event& event, event_phase phase) override;

private:
    struct tab_info {
        std::string label;
        std::unique_ptr<ui_element<Backend>> widget;
        bool closeable = true;
    };

    std::vector<tab_info> m_tabs;
    int m_current_index = -1;
    tab_position m_tab_position = tab_position::top;
    bool m_tabs_closable = false;

    // Child widgets (owned via widget_container)
    tab_bar<Backend>* m_tab_bar = nullptr;      // Non-owning
    panel<Backend>* m_content_area = nullptr;   // Non-owning

    // Helpers
    void update_tab_bar();
    void update_content_area();
    void switch_to_tab(int index);
};
```

### Helper Class: tab_bar

```cpp
/**
 * @brief Internal widget that renders tab buttons with optional scroll arrows
 * @note Not part of public API - used only by tab_widget
 */
template<UIBackend Backend>
class tab_bar : public widget_container<Backend> {
public:
    explicit tab_bar(ui_element<Backend>* parent = nullptr);

    // Tab button management
    void set_tab_count(int count);
    void set_tab_text(int index, const std::string& text);
    void set_current_index(int index);
    void set_closable(bool closable);
    void set_orientation(tab_position position);

    // Tab scrolling (for overflow)
    void scroll_left();              // Scroll to show previous tabs
    void scroll_right();             // Scroll to show next tabs
    [[nodiscard]] bool can_scroll_left() const noexcept;
    [[nodiscard]] bool can_scroll_right() const noexcept;

    // Signals
    signal<int> tab_clicked;         // User clicked tab
    signal<int> close_clicked;       // User clicked close button

protected:
    void do_render(render_context<Backend>& ctx) const override;
    bool handle_event(const ui_event& event, event_phase phase) override;

private:
    struct tab_button {
        std::string text;
        bool is_active = false;
        bool is_hovered = false;
    };

    std::vector<tab_button> m_buttons;
    int m_current_index = -1;
    bool m_closable = false;
    tab_position m_orientation = tab_position::top;

    // Tab scrolling state (for overflow)
    int m_scroll_offset = 0;         // First visible tab index
    int m_visible_tab_count = 0;     // How many tabs fit in visible area

    // Child widgets for scroll arrows (similar to scrollbar_arrow)
    tab_scroll_arrow<Backend>* m_arrow_left = nullptr;   // Non-owning
    tab_scroll_arrow<Backend>* m_arrow_right = nullptr;  // Non-owning

    // Hit testing
    int tab_at_position(int x, int y) const;
    bool close_button_at_position(int x, int y, int& out_tab_index) const;
    typename Backend::rect_type tab_rect(int index) const;
    typename Backend::rect_type close_button_rect(int index) const;

    // Scrolling helpers
    void update_scroll_arrows_visibility();
    int calculate_visible_tab_count(int available_width) const;
};

/**
 * @brief Scroll arrow button for tab overflow
 * @note Similar to scrollbar_arrow, but for horizontal tab scrolling
 */
template<UIBackend Backend>
class tab_scroll_arrow : public widget<Backend> {
public:
    enum class direction { left, right };

    explicit tab_scroll_arrow(direction dir);

    void set_state(arrow_state state);
    [[nodiscard]] arrow_state get_state() const noexcept { return m_state; }

    // Signals
    signal<> clicked;

protected:
    void do_render(render_context<Backend>& ctx) const override;
    bool handle_event(const ui_event& event, event_phase phase) override;

private:
    direction m_direction;
    arrow_state m_state = arrow_state::normal;
};
```

---

## Implementation Details

### Tab Overflow and Scrolling

When there are too many tabs to fit in the available tab bar width, the widget automatically switches to scroll mode:

**Overflow Detection:**
```cpp
template<UIBackend Backend>
int tab_bar<Backend>::calculate_visible_tab_count(int available_width) const
{
    auto* theme = this->get_theme();
    if (!theme) return 0;

    int const min_tab_width = theme->tab_widget.min_tab_width;
    int const tab_spacing = theme->tab_widget.tab_spacing;
    int const scroll_arrow_width = theme->tab_widget.scroll_arrow_width;

    // Reserve space for scroll arrows if needed
    int available_for_tabs = available_width;
    if (m_buttons.size() * (min_tab_width + tab_spacing) > available_width) {
        // Overflow detected - reserve space for arrows
        available_for_tabs -= 2 * scroll_arrow_width;
    }

    // Calculate how many tabs fit
    int visible_count = 0;
    int used_width = 0;
    for (const auto& button : m_buttons) {
        int tab_width = std::max(min_tab_width,
                                 static_cast<int>(button.text.length()) + 2 * tab_padding);

        if (used_width + tab_width > available_for_tabs) {
            break;
        }

        used_width += tab_width + tab_spacing;
        ++visible_count;
    }

    return visible_count;
}
```

**Scroll Arrow Visibility:**
```cpp
template<UIBackend Backend>
void tab_bar<Backend>::update_scroll_arrows_visibility()
{
    bool const has_overflow = (m_visible_tab_count < static_cast<int>(m_buttons.size()));

    // Show/hide arrows based on overflow
    m_arrow_left->set_visible(has_overflow);
    m_arrow_right->set_visible(has_overflow);

    // Disable left arrow if at start
    if (m_scroll_offset == 0) {
        m_arrow_left->set_state(arrow_state::disabled);
    } else {
        m_arrow_left->set_state(arrow_state::normal);
    }

    // Disable right arrow if at end
    int const last_visible = m_scroll_offset + m_visible_tab_count;
    if (last_visible >= static_cast<int>(m_buttons.size())) {
        m_arrow_right->set_state(arrow_state::disabled);
    } else {
        m_arrow_right->set_state(arrow_state::normal);
    }
}
```

**Scrolling Behavior:**
```cpp
template<UIBackend Backend>
void tab_bar<Backend>::scroll_left()
{
    if (m_scroll_offset > 0) {
        --m_scroll_offset;
        update_scroll_arrows_visibility();
        this->invalidate_arrange();  // Re-layout with new scroll offset
    }
}

template<UIBackend Backend>
void tab_bar<Backend>::scroll_right()
{
    int const max_offset = static_cast<int>(m_buttons.size()) - m_visible_tab_count;
    if (m_scroll_offset < max_offset) {
        ++m_scroll_offset;
        update_scroll_arrows_visibility();
        this->invalidate_arrange();
    }
}
```

**Visual Layout with Scroll Arrows:**
```
No overflow (all tabs visible):
┌──────┬──────┬──────┬──────┐
│ Tab1 │ Tab2 │ Tab3 │ Tab4 │
└──────┴──────┴──────┴──────┘

Overflow (tabs 5-7 hidden):
┌─┬──────┬──────┬──────┬─┐
│<│ Tab1 │ Tab2 │ Tab3 │>│
└─┴──────┴──────┴──────┴─┘
  ↑                      ↑
  Left                   Right
  (disabled)             (enabled)

After scrolling right:
┌─┬──────┬──────┬──────┬─┐
│<│ Tab2 │ Tab3 │ Tab4 │>│
└─┴──────┴──────┴──────┴─┘
  ↑                      ↑
  (enabled)              (enabled)
```

### Construction

```cpp
template<UIBackend Backend>
tab_widget<Backend>::tab_widget(ui_element<Backend>* parent)
    : widget_container<Backend>(parent)
{
    // Create internal tab bar
    auto tab_bar_widget = std::make_unique<tab_bar<Backend>>(this);
    m_tab_bar = tab_bar_widget.get();
    this->add_child(std::move(tab_bar_widget));

    // Create internal content area
    auto content_widget = std::make_unique<panel<Backend>>(this);
    m_content_area = content_widget.get();
    this->add_child(std::move(content_widget));

    // Connect tab bar signals
    m_tab_bar->tab_clicked.connect([this](int index) {
        set_current_index(index);
    });

    m_tab_bar->close_clicked.connect([this](int index) {
        tab_close_requested.emit(index);
    });

    // Tab widget is focusable
    this->set_focusable(true);
}
```

### Adding Tabs

```cpp
template<UIBackend Backend>
int tab_widget<Backend>::add_tab(std::unique_ptr<ui_element<Backend>> widget,
                                   const std::string& label)
{
    tab_info info;
    info.label = label;
    info.widget = std::move(widget);
    info.closeable = m_tabs_closable;

    m_tabs.push_back(std::move(info));
    int const new_index = static_cast<int>(m_tabs.size()) - 1;

    // Update tab bar
    update_tab_bar();

    // If first tab, make it current
    if (m_tabs.size() == 1) {
        set_current_index(0);
    }

    this->invalidate_layout();
    return new_index;
}
```

### Switching Tabs

```cpp
template<UIBackend Backend>
void tab_widget<Backend>::set_current_index(int index)
{
    // Validate index
    if (index < -1 || index >= static_cast<int>(m_tabs.size())) {
        return;
    }

    // Already current?
    if (index == m_current_index) {
        return;
    }

    int const old_index = m_current_index;
    m_current_index = index;

    // Update content area
    update_content_area();

    // Update tab bar highlighting
    m_tab_bar->set_current_index(index);

    // Emit signal
    current_changed.emit(index);

    this->invalidate_layout();
}

template<UIBackend Backend>
void tab_widget<Backend>::update_content_area()
{
    // Remove all children from content area
    m_content_area->clear_children();

    // If valid tab selected, add its widget to content area
    if (m_current_index >= 0 && m_current_index < static_cast<int>(m_tabs.size())) {
        // Borrow widget from tab_info (don't transfer ownership)
        auto* current_widget = m_tabs[m_current_index].widget.get();
        if (current_widget) {
            // Temporarily transfer ownership to content area
            // NOTE: When switching tabs, we must remove it first
            m_content_area->add_child_borrowed(current_widget);
        }
    }
}
```

### Layout Algorithm

```cpp
template<UIBackend Backend>
typename Backend::size_type tab_widget<Backend>::measure(
    int available_width,
    int available_height) const
{
    int tab_bar_width = 0;
    int tab_bar_height = 0;
    int content_width = available_width;
    int content_height = available_height;

    // Measure tab bar based on position
    switch (m_tab_position) {
        case tab_position::top:
        case tab_position::bottom: {
            // Horizontal tab bar
            auto tab_bar_size = m_tab_bar->measure(available_width, available_height);
            tab_bar_height = size_utils::get_height(tab_bar_size);
            content_height = std::max(0, available_height - tab_bar_height);
            break;
        }
        case tab_position::left:
        case tab_position::right: {
            // Vertical tab bar
            auto tab_bar_size = m_tab_bar->measure(available_width, available_height);
            tab_bar_width = size_utils::get_width(tab_bar_size);
            content_width = std::max(0, available_width - tab_bar_width);
            break;
        }
    }

    // Measure content area with remaining space
    m_content_area->measure(content_width, content_height);

    // Return full size
    return size_utils::make_size(available_width, available_height);
}

template<UIBackend Backend>
void tab_widget<Backend>::arrange(rect_type final_rect)
{
    widget_container<Backend>::arrange(final_rect);

    int const widget_width = rect_utils::get_width(final_rect);
    int const widget_height = rect_utils::get_height(final_rect);

    // Measure tab bar to get its size
    auto tab_bar_size = m_tab_bar->desired_size();
    int const tab_bar_w = size_utils::get_width(tab_bar_size);
    int const tab_bar_h = size_utils::get_height(tab_bar_size);

    rect_type tab_bar_rect;
    rect_type content_rect;

    // Position tab bar and content based on tab position
    switch (m_tab_position) {
        case tab_position::top:
            // Tab bar at top
            tab_bar_rect = rect_utils::make_rect(0, 0, widget_width, tab_bar_h);
            content_rect = rect_utils::make_rect(0, tab_bar_h, widget_width,
                                                  widget_height - tab_bar_h);
            break;

        case tab_position::bottom:
            // Tab bar at bottom
            content_rect = rect_utils::make_rect(0, 0, widget_width,
                                                  widget_height - tab_bar_h);
            tab_bar_rect = rect_utils::make_rect(0, widget_height - tab_bar_h,
                                                  widget_width, tab_bar_h);
            break;

        case tab_position::left:
            // Tab bar on left
            tab_bar_rect = rect_utils::make_rect(0, 0, tab_bar_w, widget_height);
            content_rect = rect_utils::make_rect(tab_bar_w, 0,
                                                  widget_width - tab_bar_w, widget_height);
            break;

        case tab_position::right:
            // Tab bar on right
            content_rect = rect_utils::make_rect(0, 0, widget_width - tab_bar_w,
                                                  widget_height);
            tab_bar_rect = rect_utils::make_rect(widget_width - tab_bar_w, 0,
                                                  tab_bar_w, widget_height);
            break;
    }

    // Arrange children
    m_tab_bar->arrange(tab_bar_rect);
    m_content_area->arrange(content_rect);
}
```

### Keyboard Navigation

```cpp
template<UIBackend Backend>
bool tab_widget<Backend>::handle_event(const ui_event& event, event_phase phase)
{
    if (phase != event_phase::target) {
        return widget_container<Backend>::handle_event(event, phase);
    }

    // Handle keyboard navigation
    if (auto* key = std::get_if<key_event>(&event)) {
        if (key->act == key_event::action::press) {

            // Ctrl+Tab: Next tab
            if (key->code == key_code::tab &&
                (key->mods & key_modifier::ctrl) != key_modifier::none) {

                if ((key->mods & key_modifier::shift) != key_modifier::none) {
                    previous_tab();  // Ctrl+Shift+Tab
                } else {
                    next_tab();      // Ctrl+Tab
                }
                return true;
            }

            // Ctrl+W: Close current tab
            if (key->code == key_code::w &&
                (key->mods & key_modifier::ctrl) != key_modifier::none &&
                m_tabs_closable) {

                if (m_current_index >= 0 &&
                    m_current_index < static_cast<int>(m_tabs.size()) &&
                    m_tabs[m_current_index].closeable) {
                    tab_close_requested.emit(m_current_index);
                }
                return true;
            }

            // Alt+Number: Jump to tab (Alt+1, Alt+2, etc.)
            if ((key->mods & key_modifier::alt) != key_modifier::none) {
                int tab_index = -1;

                // Check for number keys 1-9
                if (key->code >= key_code::n1 && key->code <= key_code::n9) {
                    tab_index = static_cast<int>(key->code) - static_cast<int>(key_code::n1);
                }

                if (tab_index >= 0 && tab_index < static_cast<int>(m_tabs.size())) {
                    set_current_index(tab_index);
                    return true;
                }
            }
        }
    }

    return widget_container<Backend>::handle_event(event, phase);
}

template<UIBackend Backend>
void tab_widget<Backend>::next_tab()
{
    if (m_tabs.empty()) return;

    int next_index = m_current_index + 1;
    if (next_index >= static_cast<int>(m_tabs.size())) {
        next_index = 0;  // Wrap around
    }
    set_current_index(next_index);
}

template<UIBackend Backend>
void tab_widget<Backend>::previous_tab()
{
    if (m_tabs.empty()) return;

    int prev_index = m_current_index - 1;
    if (prev_index < 0) {
        prev_index = static_cast<int>(m_tabs.size()) - 1;  // Wrap around
    }
    set_current_index(prev_index);
}
```

---

## Theme Integration

### Theme Structure

Add to `theme.hh`:

```cpp
template<typename ColorType, typename FontType, typename IconType>
struct theme {
    // ... existing styles ...

    /**
     * @brief Tab widget styling
     */
    struct tab_widget_style {
        // Tab bar background
        ColorType tab_bar_background;

        // Tab button colors
        ColorType tab_normal_background;      ///< Inactive tab background
        ColorType tab_normal_text;            ///< Inactive tab text
        ColorType tab_active_background;      ///< Active tab background
        ColorType tab_active_text;            ///< Active tab text
        ColorType tab_hover_background;       ///< Hovered tab background
        ColorType tab_hover_text;             ///< Hovered tab text

        // Tab borders
        ColorType tab_border;                 ///< Tab border color
        ColorType tab_active_border;          ///< Active tab border color

        // Close button
        ColorType close_button_normal;        ///< Close button (X) color
        ColorType close_button_hover;         ///< Close button hover color

        // Fonts
        FontType tab_font;                    ///< Tab label font
        FontType tab_active_font;             ///< Active tab label font (may be bold)

        // Scroll arrows (for tab overflow)
        ColorType scroll_arrow_normal;        ///< Scroll arrow normal color
        ColorType scroll_arrow_hover;         ///< Scroll arrow hover color
        ColorType scroll_arrow_pressed;       ///< Scroll arrow pressed color
        ColorType scroll_arrow_disabled;      ///< Scroll arrow disabled color

        // Spacing
        int tab_padding_horizontal = 4;       ///< Horizontal padding inside tab
        int tab_padding_vertical = 1;         ///< Vertical padding inside tab
        int tab_spacing = 1;                  ///< Space between tabs
        int close_button_spacing = 2;         ///< Space between label and close button
        int min_tab_width = 10;               ///< Minimum tab width in characters
        int scroll_arrow_width = 3;           ///< Width of scroll arrow buttons
    } tab_widget;
};
```

### Theme Colors (Norton Blue Example)

```yaml
# themes/examples/norton_blue.yaml

tab_widget:
  tab_bar_background: { r: 0, g: 0, b: 85 }           # Dark blue

  # Tab button colors
  tab_normal_background: { r: 0, g: 0, b: 85 }        # Dark blue (inactive)
  tab_normal_text: { r: 170, g: 170, b: 170 }         # Gray text
  tab_active_background: { r: 0, g: 85, b: 170 }      # Bright blue (active)
  tab_active_text: { r: 255, g: 255, b: 255 }         # White text
  tab_hover_background: { r: 0, g: 51, b: 128 }       # Medium blue
  tab_hover_text: { r: 255, g: 255, b: 255 }          # White text

  # Borders
  tab_border: { r: 85, g: 85, b: 85 }                 # Gray border
  tab_active_border: { r: 170, g: 170, b: 170 }       # Light gray border

  # Close button
  close_button_normal: { r: 170, g: 170, b: 170 }     # Gray X
  close_button_hover: { r: 255, g: 85, b: 85 }        # Red X on hover

  # Fonts
  tab_font: { bold: false, reverse: false, underline: false }
  tab_active_font: { bold: true, reverse: false, underline: false }

  # Scroll arrows
  scroll_arrow_normal: { r: 170, g: 170, b: 170 }     # Gray arrows
  scroll_arrow_hover: { r: 255, g: 255, b: 255 }      # White on hover
  scroll_arrow_pressed: { r: 255, g: 255, b: 0 }      # Yellow when pressed
  scroll_arrow_disabled: { r: 85, g: 85, b: 85 }      # Dark gray when disabled

  # Spacing
  tab_padding_horizontal: 4
  tab_padding_vertical: 1
  tab_spacing: 1
  close_button_spacing: 2
  min_tab_width: 10             # Minimum 10 characters
  scroll_arrow_width: 3         # 3 characters wide
```

### Programmatic Theme

Add to `conio_themes.hh` in `create_base_theme()`:

```cpp
// ====================================================================
// Tab Widget Configuration
// ====================================================================
color dark_blue{0, 0, 85};
color bright_blue{0, 85, 170};
color medium_blue{0, 51, 128};

theme.tab_widget.tab_bar_background = dark_blue;

theme.tab_widget.tab_normal_background = dark_blue;
theme.tab_widget.tab_normal_text = light_gray;
theme.tab_widget.tab_active_background = bright_blue;
theme.tab_widget.tab_active_text = white;
theme.tab_widget.tab_hover_background = medium_blue;
theme.tab_widget.tab_hover_text = white;

theme.tab_widget.tab_border = dark_gray;
theme.tab_widget.tab_active_border = light_gray;

theme.tab_widget.close_button_normal = light_gray;
theme.tab_widget.close_button_hover = color{255, 85, 85};  // Red

theme.tab_widget.tab_font = conio_renderer::font{.bold = false};
theme.tab_widget.tab_active_font = conio_renderer::font{.bold = true};

theme.tab_widget.scroll_arrow_normal = light_gray;
theme.tab_widget.scroll_arrow_hover = white;
theme.tab_widget.scroll_arrow_pressed = color{255, 255, 0};  // Yellow
theme.tab_widget.scroll_arrow_disabled = dark_gray;

theme.tab_widget.tab_padding_horizontal = 4;
theme.tab_widget.tab_padding_vertical = 1;
theme.tab_widget.tab_spacing = 1;
theme.tab_widget.close_button_spacing = 2;
theme.tab_widget.min_tab_width = 10;
theme.tab_widget.scroll_arrow_width = 3;
```

---

## Event Handling

### Mouse Events

**Tab Bar Click Detection:**

```cpp
template<UIBackend Backend>
bool tab_bar<Backend>::handle_event(const ui_event& event, event_phase phase)
{
    if (phase != event_phase::target) {
        return widget_container<Backend>::handle_event(event, phase);
    }

    if (auto* mouse = std::get_if<mouse_event>(&event)) {
        if (mouse->act == mouse_event::action::press) {
            int const mouse_x = mouse->x;
            int const mouse_y = mouse->y;

            // Check if close button clicked first (higher priority)
            int close_tab_index = -1;
            if (m_closable && close_button_at_position(mouse_x, mouse_y, close_tab_index)) {
                close_clicked.emit(close_tab_index);
                return true;
            }

            // Check if tab clicked
            int const tab_index = tab_at_position(mouse_x, mouse_y);
            if (tab_index >= 0) {
                tab_clicked.emit(tab_index);
                return true;
            }
        }

        // Track hover state for visual feedback
        if (mouse->act == mouse_event::action::move) {
            int const hovered_tab = tab_at_position(mouse->x, mouse->y);

            // Update hover state for all tabs
            for (int i = 0; i < static_cast<int>(m_buttons.size()); ++i) {
                bool const should_hover = (i == hovered_tab && i != m_current_index);
                if (m_buttons[i].is_hovered != should_hover) {
                    m_buttons[i].is_hovered = should_hover;
                    this->mark_dirty();
                }
            }
            return true;
        }
    }

    return widget_container<Backend>::handle_event(event, phase);
}
```

### Focus Management

**Focus Behavior:**
1. Tab widget itself can receive focus (for keyboard navigation)
2. Content widgets in tabs can also have focus
3. Tab key cycles through focusable widgets in current tab
4. Ctrl+Tab changes tabs without affecting focus

```cpp
// When tab widget receives focus, optionally focus first widget in current tab
template<UIBackend Backend>
void tab_widget<Backend>::on_focus_gained()
{
    widget_container<Backend>::on_focus_gained();

    // Optionally: Set focus to first focusable child in current tab
    if (auto* current = current_widget()) {
        if (auto* focusable = find_first_focusable(current)) {
            auto* input = ui_services<Backend>::input();
            if (input) {
                input->set_focus(focusable);
            }
        }
    }
}
```

---

## Testing Strategy

### Unit Tests

**File:** `unittest/widgets/test_tab_widget.cc`

```cpp
TEST_CASE("tab_widget - Construction") {
    ui_context_fixture<test_canvas_backend> fixture;

    auto tabs = std::make_unique<tab_widget<test_canvas_backend>>();

    REQUIRE(tabs->count() == 0);
    REQUIRE(tabs->current_index() == -1);
    REQUIRE(tabs->current_widget() == nullptr);
    REQUIRE_FALSE(tabs->tabs_closable());
    REQUIRE(tabs->get_tab_position() == tab_position::top);
}

TEST_CASE("tab_widget - Add tabs") {
    ui_context_fixture<test_canvas_backend> fixture;

    auto tabs = std::make_unique<tab_widget<test_canvas_backend>>();

    auto page1 = std::make_unique<panel<test_canvas_backend>>();
    auto page2 = std::make_unique<panel<test_canvas_backend>>();

    int idx1 = tabs->add_tab(std::move(page1), "Page 1");
    int idx2 = tabs->add_tab(std::move(page2), "Page 2");

    REQUIRE(idx1 == 0);
    REQUIRE(idx2 == 1);
    REQUIRE(tabs->count() == 2);
    REQUIRE(tabs->current_index() == 0);  // First tab auto-selected
}

TEST_CASE("tab_widget - Switch tabs") {
    ui_context_fixture<test_canvas_backend> fixture;

    auto tabs = std::make_unique<tab_widget<test_canvas_backend>>();
    tabs->add_tab(std::make_unique<panel<test_canvas_backend>>(), "Tab 1");
    tabs->add_tab(std::make_unique<panel<test_canvas_backend>>(), "Tab 2");

    int signal_index = -1;
    tabs->current_changed.connect([&signal_index](int index) {
        signal_index = index;
    });

    tabs->set_current_index(1);

    REQUIRE(tabs->current_index() == 1);
    REQUIRE(signal_index == 1);
}

TEST_CASE("tab_widget - Remove tab") {
    ui_context_fixture<test_canvas_backend> fixture;

    auto tabs = std::make_unique<tab_widget<test_canvas_backend>>();
    tabs->add_tab(std::make_unique<panel<test_canvas_backend>>(), "Tab 1");
    tabs->add_tab(std::make_unique<panel<test_canvas_backend>>(), "Tab 2");
    tabs->add_tab(std::make_unique<panel<test_canvas_backend>>(), "Tab 3");

    tabs->set_current_index(1);  // Select middle tab
    tabs->remove_tab(1);

    REQUIRE(tabs->count() == 2);
    REQUIRE(tabs->current_index() == 1);  // Now points to what was Tab 3
    REQUIRE(tabs->tab_text(0) == "Tab 1");
    REQUIRE(tabs->tab_text(1) == "Tab 3");
}

TEST_CASE("tab_widget - Next/previous tab") {
    ui_context_fixture<test_canvas_backend> fixture;

    auto tabs = std::make_unique<tab_widget<test_canvas_backend>>();
    tabs->add_tab(std::make_unique<panel<test_canvas_backend>>(), "Tab 1");
    tabs->add_tab(std::make_unique<panel<test_canvas_backend>>(), "Tab 2");
    tabs->add_tab(std::make_unique<panel<test_canvas_backend>>(), "Tab 3");

    tabs->set_current_index(0);

    tabs->next_tab();
    REQUIRE(tabs->current_index() == 1);

    tabs->next_tab();
    REQUIRE(tabs->current_index() == 2);

    tabs->next_tab();  // Wrap around
    REQUIRE(tabs->current_index() == 0);

    tabs->previous_tab();  // Wrap around
    REQUIRE(tabs->current_index() == 2);
}

TEST_CASE("tab_widget - Close button signal") {
    ui_context_fixture<test_canvas_backend> fixture;

    auto tabs = std::make_unique<tab_widget<test_canvas_backend>>();
    tabs->set_tabs_closable(true);
    tabs->add_tab(std::make_unique<panel<test_canvas_backend>>(), "Tab 1");
    tabs->add_tab(std::make_unique<panel<test_canvas_backend>>(), "Tab 2");

    int close_requested_index = -1;
    tabs->tab_close_requested.connect([&close_requested_index](int index) {
        close_requested_index = index;
    });

    // Simulate close button click
    // (This would require simulating mouse event on close button)
    // For now, just emit signal directly for testing
    tabs->tab_close_requested.emit(1);

    REQUIRE(close_requested_index == 1);
    REQUIRE(tabs->count() == 2);  // Tab not auto-removed
}

TEST_CASE("tab_widget - Tab positions") {
    ui_context_fixture<test_canvas_backend> fixture;

    auto tabs = std::make_unique<tab_widget<test_canvas_backend>>();

    tabs->set_tab_position(tab_position::top);
    REQUIRE(tabs->get_tab_position() == tab_position::top);

    tabs->set_tab_position(tab_position::bottom);
    REQUIRE(tabs->get_tab_position() == tab_position::bottom);

    tabs->set_tab_position(tab_position::left);
    REQUIRE(tabs->get_tab_position() == tab_position::left);

    tabs->set_tab_position(tab_position::right);
    REQUIRE(tabs->get_tab_position() == tab_position::right);
}

TEST_CASE("tab_widget - Clear all tabs") {
    ui_context_fixture<test_canvas_backend> fixture;

    auto tabs = std::make_unique<tab_widget<test_canvas_backend>>();
    tabs->add_tab(std::make_unique<panel<test_canvas_backend>>(), "Tab 1");
    tabs->add_tab(std::make_unique<panel<test_canvas_backend>>(), "Tab 2");

    REQUIRE(tabs->count() == 2);

    tabs->clear();

    REQUIRE(tabs->count() == 0);
    REQUIRE(tabs->current_index() == -1);
}

TEST_CASE("tab_widget - Insert tab at position") {
    ui_context_fixture<test_canvas_backend> fixture;

    auto tabs = std::make_unique<tab_widget<test_canvas_backend>>();
    tabs->add_tab(std::make_unique<panel<test_canvas_backend>>(), "Tab 1");
    tabs->add_tab(std::make_unique<panel<test_canvas_backend>>(), "Tab 3");

    int idx = tabs->insert_tab(1, std::make_unique<panel<test_canvas_backend>>(), "Tab 2");

    REQUIRE(idx == 1);
    REQUIRE(tabs->count() == 3);
    REQUIRE(tabs->tab_text(0) == "Tab 1");
    REQUIRE(tabs->tab_text(1) == "Tab 2");
    REQUIRE(tabs->tab_text(2) == "Tab 3");
}

TEST_CASE("tab_widget - Layout with different positions") {
    ui_context_fixture<test_canvas_backend> fixture;

    auto tabs = std::make_unique<tab_widget<test_canvas_backend>>();
    tabs->add_tab(std::make_unique<panel<test_canvas_backend>>(), "Tab 1");

    // Test layout at each position
    tabs->set_tab_position(tab_position::top);
    tabs->measure(80, 25);
    tabs->arrange(rect_utils::make_rect<test_canvas_backend>(0, 0, 80, 25));

    // Verify tab bar is at top
    // (Would check actual bounds of children)

    tabs->set_tab_position(tab_position::bottom);
    tabs->measure(80, 25);
    tabs->arrange(rect_utils::make_rect<test_canvas_backend>(0, 0, 80, 25));

    // Verify tab bar is at bottom
}

TEST_CASE("tab_widget - Tab overflow scrolling") {
    ui_context_fixture<test_canvas_backend> fixture;

    auto tabs = std::make_unique<tab_widget<test_canvas_backend>>();

    // Add many tabs to force overflow
    for (int i = 0; i < 20; ++i) {
        tabs->add_tab(std::make_unique<panel<test_canvas_backend>>(),
                      "Very Long Tab Name " + std::to_string(i + 1));
    }

    // Measure with limited width
    tabs->measure(40, 25);  // Narrow width forces overflow
    tabs->arrange(rect_utils::make_rect<test_canvas_backend>(0, 0, 40, 25));

    // Verify scroll arrows are visible
    // (Would check arrow widgets visibility)

    // Test scrolling
    // (Would simulate arrow clicks and verify scroll offset changes)
}

TEST_CASE("tab_widget - Scroll arrows auto-hide") {
    ui_context_fixture<test_canvas_backend> fixture;

    auto tabs = std::make_unique<tab_widget<test_canvas_backend>>();
    tabs->add_tab(std::make_unique<panel<test_canvas_backend>>(), "Tab 1");
    tabs->add_tab(std::make_unique<panel<test_canvas_backend>>(), "Tab 2");

    // Measure with plenty of width
    tabs->measure(80, 25);  // Wide enough for all tabs
    tabs->arrange(rect_utils::make_rect<test_canvas_backend>(0, 0, 80, 25));

    // Verify scroll arrows are NOT visible (no overflow)
    // (Would check arrow widgets visibility == false)
}
```

### Integration Tests

```cpp
TEST_CASE("tab_widget - Integration with forms") {
    ui_context_fixture<test_canvas_backend> fixture;

    auto tabs = std::make_unique<tab_widget<test_canvas_backend>>();

    // General settings tab
    auto general_page = std::make_unique<vbox<test_canvas_backend>>();
    general_page->template emplace_child<label>("General Settings");
    auto* username_edit = general_page->template emplace_child<line_edit>();

    // Advanced settings tab
    auto advanced_page = std::make_unique<vbox<test_canvas_backend>>();
    advanced_page->template emplace_child<label>("Advanced Settings");
    auto* debug_check = advanced_page->template emplace_child<checkbox>("Enable Debug");

    tabs->add_tab(std::move(general_page), "General");
    tabs->add_tab(std::move(advanced_page), "Advanced");

    // Test switching tabs
    tabs->set_current_index(0);
    REQUIRE(tabs->current_index() == 0);

    tabs->set_current_index(1);
    REQUIRE(tabs->current_index() == 1);
}
```

---

## Usage Examples

### Simple Settings Dialog

```cpp
// Create tab widget
auto tabs = std::make_unique<tab_widget<Backend>>();

// General tab
auto general_page = std::make_unique<vbox<Backend>>(2);
general_page->template emplace_child<label>("Username:");
auto* username = general_page->template emplace_child<line_edit>();
general_page->template emplace_child<label>("Email:");
auto* email = general_page->template emplace_child<line_edit>();
tabs->add_tab(std::move(general_page), "General");

// Advanced tab
auto advanced_page = std::make_unique<vbox<Backend>>(2);
auto* debug = advanced_page->template emplace_child<checkbox>("Enable Debug Mode");
auto* logging = advanced_page->template emplace_child<checkbox>("Enable Logging");
tabs->add_tab(std::move(advanced_page), "Advanced");

// About tab
auto about_page = std::make_unique<vbox<Backend>>(1);
about_page->template emplace_child<label>("MyApp v1.0");
about_page->template emplace_child<label>("Copyright 2025");
tabs->add_tab(std::move(about_page), "About");

// Handle tab changes
tabs->current_changed.connect([](int index) {
    std::cout << "Switched to tab " << index << "\n";
});
```

### Code Editor with Closeable Tabs

```cpp
auto editor = std::make_unique<tab_widget<Backend>>();
editor->set_tabs_closable(true);

// Open files
auto file1 = std::make_unique<text_view<Backend>>();
file1->set_text(load_file("main.cpp"));
editor->add_tab(std::move(file1), "main.cpp");

auto file2 = std::make_unique<text_view<Backend>>();
file2->set_text(load_file("utils.h"));
editor->add_tab(std::move(file2), "utils.h");

// Handle close requests
editor->tab_close_requested.connect([&editor](int index) {
    // Check for unsaved changes
    if (has_unsaved_changes(index)) {
        // Show confirmation dialog
        if (confirm_close()) {
            editor->remove_tab(index);
        }
    } else {
        editor->remove_tab(index);
    }
});

// Handle Ctrl+S to save current file
// (via hotkey system)
```

### Vertical Tabs (Sidebar Style)

```cpp
auto sidebar = std::make_unique<tab_widget<Backend>>();
sidebar->set_tab_position(tab_position::left);

// Files tab
auto files_page = std::make_unique<vbox<Backend>>();
files_page->template emplace_child<label>("File Browser");
sidebar->add_tab(std::move(files_page), "Files");

// Search tab
auto search_page = std::make_unique<vbox<Backend>>();
search_page->template emplace_child<label>("Search Results");
sidebar->add_tab(std::move(search_page), "Search");

// Git tab
auto git_page = std::make_unique<vbox<Backend>>();
git_page->template emplace_child<label>("Git Changes");
sidebar->add_tab(std::move(git_page), "Git");
```

---

## Design Decisions

The following design decisions have been finalized:

### 1. Tab Overflow Handling ✅

**Decision:** Scroll arrows (like scrollbar widget)

When tabs overflow the available tab bar width:
- Add left/right scroll arrow buttons (similar to `scrollbar_arrow`)
- Clicking arrows scrolls tab bar horizontally
- Tabs shrink to minimum width before scrolling activates
- Arrow buttons auto-hide when all tabs visible

**Rationale:** Consistent with scrollbar design pattern, provides intuitive navigation.

### 2. Minimum Tab Width ✅

**Decision:** Theme-configurable minimum

- Theme property: `tab_widget.min_tab_width` (default: 10 characters)
- Tabs shrink from natural size down to minimum
- Once minimum reached, scroll arrows appear
- Prevents tabs from becoming unreadable

**Rationale:** Different themes may have different aesthetics for compact vs spacious tabs.

### 3. Tab Close Confirmation ✅

**Decision:** Signal-based (application decides)

- Widget emits `tab_close_requested(int index)` signal
- Does NOT automatically remove tab
- Application connects to signal and decides whether to close
- Allows custom confirmation dialogs, save prompts, etc.

**Rationale:** Maximum flexibility for applications with different close behaviors.

### 4. Empty Tab Widget ✅

**Decision:** Simple empty area

- When `count() == 0`, show empty content area
- No placeholder text or widgets
- Tab bar still visible (for consistency)

**Rationale:** Simple, clean, consistent with other container widgets.

### 5. Borrowed Widget Ownership

**Decision:** `unique_ptr` with careful borrowing

- `tab_info` owns widget via `unique_ptr`
- When tab becomes current, widget is "borrowed" by content_area
- When switching tabs, widget is returned to tab_info
- Requires careful ownership management in `switch_to_tab()`

**Rationale:** Clear ownership semantics, avoids shared_ptr overhead.

### 6. Tab Bar Architecture

**Decision:** Separate `tab_bar` widget

- `tab_bar` is internal helper widget, not public API
- Handles rendering and mouse interaction
- Cleaner separation of concerns
- Easier to test independently

**Rationale:** Better encapsulation and testability.

---

## Implementation Checklist

**Last Updated:** 2025-11-21

### Core Functionality
- [x] `tab_widget` class skeleton ✅ (extends panel<Backend>)
- [ ] `tab_bar` helper class (deferred - using inline rendering)
- [ ] `tab_scroll_arrow` helper class (deferred - future enhancement)
- [x] `add_tab()` implementation ✅
- [x] `remove_tab()` implementation ✅
- [x] `set_current_index()` implementation ✅
- [x] `next_tab()` / `previous_tab()` implementation ✅
- [x] Tab label management ✅ (set_tab_text, tab_text)
- [x] Tab overflow detection ✅
- [x] Scroll arrow visibility logic ✅
- [x] `scroll_left()` / `scroll_right()` implementation ✅

### Layout
- [x] Measure pass (delegates to panel base) ✅
- [x] Arrange pass (delegates to panel base) ✅
- [x] Content area sizing ✅ (via panel)
- [x] Custom tab bar height calculation ✅ (get_tab_bar_height())

### Rendering
- [x] Tab bar rendering (normal, active, hover states) ✅
- [x] Close button rendering ([X] on tabs) ✅
- [x] Scroll arrow rendering ✅
- [x] Border rendering ✅ (via panel)
- [x] Content area rendering ✅
- [x] Bottom position rendering ✅
- [x] Left/right position rendering (horizontal fallback) ✅
- [x] Tab truncation when at minimum width ✅

### Event Handling
- [x] Mouse click on tabs ✅
- [x] Mouse click on close buttons ✅
- [x] Mouse click on scroll arrows ✅
- [x] Mouse hover for visual feedback ✅
- [x] Keyboard navigation (Ctrl+Tab, Ctrl+Shift+Tab) ✅
- [x] Alt+Number shortcuts (Alt+1 to Alt+9) ✅
- [x] Ctrl+W to close tab ✅

### Theme Integration
- [x] Add `tab_widget_style` to theme.hh ✅
- [ ] Update all theme YAML files
- [x] Update theme_builder.hh ✅
- [x] Update conio_themes.hh ✅

### Signals
- [x] `current_changed` emission ✅
- [x] `tab_close_requested` emission ✅

### Testing
- [x] Unit tests (96 assertions) ✅
- [x] Integration tests ✅
- [x] Overflow/scrolling edge case tests ✅
- [x] Demo integration ✅

### Documentation
- [x] API documentation (header comments) ✅
- [x] Usage examples in demo ✅
- [x] Update WIDGET_ROADMAP.md ✅
- [x] Update CLAUDE.md ✅

---

## Timeline Estimate

**Week 1: Core Implementation**
- Days 1-2: `tab_widget` and `tab_bar` class structure, add/remove/switch logic
- Days 3-4: Layout algorithm (measure/arrange for all 4 positions)
- Day 5: Basic rendering

**Week 2: Polish & Testing**
- Days 1-2: Event handling (mouse, keyboard)
- Day 3: Theme integration (all files)
- Day 4: Comprehensive testing
- Day 5: Demo integration, documentation

**Total:** 2 weeks for complete implementation

---

## Success Criteria

After implementation, users should be able to:
- ✅ Create tab widget with multiple pages
- ✅ Click tabs to switch pages
- ✅ Use Ctrl+Tab for keyboard navigation
- ✅ Add/remove tabs dynamically
- ✅ Close tabs with close button (if enabled)
- ✅ Position tab bar at top/bottom/left/right
- ✅ Customize tab colors via themes
- ✅ Handle tab overflow with scroll arrows
- ✅ Configure minimum tab width via theme
- ✅ Build settings dialogs, code editors, multi-view apps

---

## References

- **Similar Widgets:**
  - Qt: `QTabWidget`
  - GTK: `GtkNotebook`
  - Windows: Tab Control
  - Web: `<tabs>` component

- **Related OnyxUI Widgets:**
  - `menu_bar` - Similar tab-like selection
  - `button` - Tab rendering similar to button states
  - `panel` - Content area container

- **Architecture Documents:**
  - `docs/CLAUDE/ARCHITECTURE.md` - Layout system
  - `docs/CLAUDE/THEMING.md` - Theme integration
  - `docs/WIDGET_ROADMAP.md` - Implementation plan

---

**End of Design Document**
