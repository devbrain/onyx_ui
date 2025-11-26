# Backend-Agnostic Sizing Design

**Status**: Proposal
**Date**: 2025-11-26
**Author**: Claude Code

---

## Problem Statement

Current sizing in OnyxUI uses raw integers that have different meanings across backends:

```cpp
// Current code - concept leakage!
vbox<Backend>(2);                    // 2 cells (TUI) vs 2 pixels (GUI)
set_padding(thickness::all(1));      // 1 cell vs 1 pixel
height_constraint.min_size = 10;     // 10 lines vs 10 pixels
```

This breaks the "write once, compile anywhere" promise. A demo written for conio (TUI) would be unusable when compiled for SDL2 (GUI).

---

## Design Goals

1. **Type-safe**: Compiler prevents mixing incompatible units
2. **Readable**: Code clearly expresses intent
3. **Backend-agnostic**: Same source code works across all backends
4. **Minimal API changes**: Evolutionary, not revolutionary
5. **Theme-driven**: Backends customize via theme files

---

## Solution: Two-Category Approach

### Category A: Spacing (Gaps, Padding, Margins)

Use `enum class spacing` with theme-provided values.

### Category B: Widget Dimensions (Lines, Characters)

Use semantic methods on widgets with internal conversion.

---

## Category A: Spacing System

### API Design

```cpp
namespace onyxui {

    /**
     * @brief Semantic spacing levels
     *
     * Type-safe spacing values resolved by theme.
     * Prevents hardcoded pixel/cell values in application code.
     */
    enum class spacing : std::uint8_t {
        none   = 0,   ///< No spacing (0)
        tiny   = 1,   ///< Minimal separation
        small  = 2,   ///< Tight spacing (default for dense UIs)
        medium = 3,   ///< Standard spacing (default for most cases)
        large  = 4,   ///< Loose spacing (for visual separation)
        xlarge = 5    ///< Very loose (for major sections)
    };

}
```

### Theme Integration

```cpp
// In theme.hh
struct theme {
    struct spacing_values {
        int none   = 0;
        int tiny   = 1;
        int small  = 2;
        int medium = 4;
        int large  = 8;
        int xlarge = 16;

        /// Resolve spacing enum to backend-specific value
        [[nodiscard]] constexpr int resolve(spacing s) const noexcept {
            switch (s) {
                case spacing::none:   return none;
                case spacing::tiny:   return tiny;
                case spacing::small:  return small;
                case spacing::medium: return medium;
                case spacing::large:  return large;
                case spacing::xlarge: return xlarge;
            }
            return medium;  // fallback
        }
    } spacing;
};
```

### Theme YAML Files

```yaml
# themes/conio_default.yaml (TUI - character cells)
spacing:
  none: 0
  tiny: 0
  small: 1
  medium: 2
  large: 3
  xlarge: 4

# themes/sdl2_default.yaml (GUI - pixels)
spacing:
  none: 0
  tiny: 2
  small: 4
  medium: 8
  large: 16
  xlarge: 32
```

### Updated Container APIs

```cpp
// vbox / hbox constructors
template<UIBackend Backend>
class vbox : public widget_container<Backend> {
public:
    /// Construct with semantic spacing
    explicit vbox(spacing gap = spacing::none);

    /// @deprecated Use spacing enum instead
    [[deprecated("Use spacing enum for backend-agnostic code")]]
    explicit vbox(int gap_pixels);
};

// Usage
auto layout = std::make_unique<vbox<Backend>>(spacing::medium);
```

### Updated Padding/Margin APIs

```cpp
// thickness with spacing support
struct thickness {
    spacing top    = spacing::none;
    spacing right  = spacing::none;
    spacing bottom = spacing::none;
    spacing left   = spacing::none;

    static constexpr thickness all(spacing s) {
        return {s, s, s, s};
    }

    static constexpr thickness symmetric(spacing vertical, spacing horizontal) {
        return {vertical, horizontal, vertical, horizontal};
    }

    static constexpr thickness only_top(spacing s) {
        return {s, spacing::none, spacing::none, spacing::none};
    }

    // ... other helpers
};

// Usage
panel->set_padding(thickness::all(spacing::medium));
panel->set_padding(thickness::symmetric(spacing::small, spacing::large));
```

### Resolution at Layout Time

```cpp
// In linear_layout::arrange()
void arrange(/*...*/) {
    const auto* theme = ui_services<Backend>::themes()->get_current_theme();
    int gap_pixels = theme->spacing.resolve(m_spacing);

    // Use gap_pixels for actual layout calculations
    // ...
}
```

---

## Category B: Widget Semantic Sizing

### Design Principle

Each widget exposes **domain-appropriate** sizing methods. The widget internally converts to backend units using theme values.

### Text-Based Widgets

```cpp
template<UIBackend Backend>
class text_view : public widget_container<Backend> {
public:
    /// Set visible height in text lines
    void set_visible_lines(int lines) {
        m_visible_lines = lines;
        invalidate_measure();
    }

    /// Set visible width in characters
    void set_visible_chars(int chars) {
        m_visible_chars = chars;
        invalidate_measure();
    }

protected:
    size_type do_measure(int available_w, int available_h) override {
        const auto* theme = get_theme();
        int line_height = theme->text.line_height;  // 1 (TUI) or 16 (GUI)
        int char_width = theme->text.char_width;    // 1 (TUI) or 8 (GUI)

        int height = m_visible_lines * line_height + border_size();
        int width = m_visible_chars * char_width + border_size() + scrollbar_width();

        return {width, height};
    }

private:
    int m_visible_lines = 10;  // default
    int m_visible_chars = 40;  // default
};
```

```cpp
template<UIBackend Backend>
class line_edit : public widget<Backend> {
public:
    /// Set visible width in characters
    void set_visible_chars(int chars) {
        m_visible_chars = chars;
        invalidate_measure();
    }

protected:
    size_type do_measure(int available_w, int available_h) override {
        const auto* theme = get_theme();
        int char_width = theme->text.char_width;
        int line_height = theme->text.line_height;

        return {
            m_visible_chars * char_width + padding(),
            line_height + padding()
        };
    }

private:
    int m_visible_chars = 20;  // default
};
```

### Slider Widget

```cpp
template<UIBackend Backend>
class slider : public widget<Backend> {
public:
    /// Set track length in logical units
    /// For horizontal slider: width in "thumb widths"
    /// For vertical slider: height in "thumb heights"
    void set_track_length(int units) {
        m_track_length = units;
        invalidate_measure();
    }

protected:
    size_type do_measure(int available_w, int available_h) override {
        const auto* theme = get_theme();
        int unit_size = theme->slider.unit_size;  // 1 (TUI) or 8 (GUI)
        int thickness = theme->slider.thickness;

        if (m_orientation == horizontal) {
            return {m_track_length * unit_size, thickness};
        } else {
            return {thickness, m_track_length * unit_size};
        }
    }

private:
    int m_track_length = 20;
};
```

### Progress Bar Widget

```cpp
template<UIBackend Backend>
class progress_bar : public widget<Backend> {
public:
    /// Set bar length in logical units
    void set_bar_length(int units) {
        m_bar_length = units;
        invalidate_measure();
    }

protected:
    size_type do_measure(int available_w, int available_h) override {
        const auto* theme = get_theme();
        int unit_size = theme->progress_bar.unit_size;
        int thickness = theme->progress_bar.thickness;

        return {m_bar_length * unit_size, thickness};
    }

private:
    int m_bar_length = 30;
};
```

### Theme Text/Widget Settings

```yaml
# themes/conio_default.yaml
text:
  line_height: 1      # 1 character cell
  char_width: 1       # 1 character cell

slider:
  unit_size: 1        # 1 cell per unit
  thickness: 1        # 1 cell thick

progress_bar:
  unit_size: 1
  thickness: 1

# themes/sdl2_default.yaml
text:
  line_height: 20     # 20 pixels (with line spacing)
  char_width: 10      # 10 pixels (monospace)

slider:
  unit_size: 8        # 8 pixels per unit
  thickness: 20       # 20 pixels thick

progress_bar:
  unit_size: 8
  thickness: 24
```

---

## Window Sizing

Windows use **fit-to-content** model - no explicit dimensions.

```cpp
template<UIBackend Backend>
class window : public widget_container<Backend> {
public:
    /// Size window to fit its content
    void fit_content() {
        m_sizing_mode = sizing_mode::fit_content;
        invalidate_measure();
    }

    /// Set minimum size in lines/chars (for content area)
    void set_min_content_size(int chars, int lines) {
        m_min_chars = chars;
        m_min_lines = lines;
    }

    /// Set maximum size in lines/chars (for content area)
    void set_max_content_size(int chars, int lines) {
        m_max_chars = chars;
        m_max_lines = lines;
    }

    /// Constrain to workspace bounds
    void constrain_to_workspace() {
        m_constrain_to_workspace = true;
    }

protected:
    size_type do_measure(int available_w, int available_h) override {
        // Measure content
        auto content_size = m_content->measure(available_w, available_h);

        // Apply min/max constraints (converted via theme)
        const auto* theme = get_theme();
        int min_w = m_min_chars * theme->text.char_width;
        int min_h = m_min_lines * theme->text.line_height;
        int max_w = m_max_chars * theme->text.char_width;
        int max_h = m_max_lines * theme->text.line_height;

        content_size.w = std::clamp(content_size.w, min_w, max_w);
        content_size.h = std::clamp(content_size.h, min_h, max_h);

        // Add chrome (title bar, borders)
        return add_chrome(content_size);
    }
};
```

---

## Implementation Plan

### Step 1: Core Infrastructure
1. Add `enum class spacing` to `layout_strategy.hh`
2. Add `spacing_values` to theme with `resolve()` method
3. Update theme YAML schema

### Step 2: Containers
1. Update `vbox`, `hbox` constructors to take `spacing` enum
2. Update `grid` constructor for column/row gaps
3. Update `group_box`, `panel` for padding
4. Update `button_group`, `tab_widget`

### Step 3: Text-Based Widgets
1. Add `set_visible_lines()`, `set_visible_chars()` to `text_view`
2. Add `set_visible_chars()` to `line_edit`
3. Update `do_measure()` to use theme's `line_height`, `char_width`

### Step 4: Input Widgets
1. Add `set_track_length()` to `slider`
2. Add `set_bar_length()` to `progress_bar`
3. Add `set_visible_items()` to `combo_box`

### Step 5: Window System
1. Add `fit_content()` to `window`
2. Add `set_min/max_content_size(chars, lines)`
3. Update window positioning to relative anchors

### Step 6: Layout Helpers & MVC
1. Update `spacer` to take `spacing` enum
2. Add `set_visible_items()` to `list_view`

### Step 7: Update All Examples
1. Update `examples/` to use new APIs
2. Remove all hardcoded integer sizes

---

## Complete Example: Backend-Agnostic Dialog

```cpp
template<UIBackend Backend>
std::unique_ptr<window<Backend>> create_settings_dialog() {
    // Create window
    auto dialog = std::make_unique<window<Backend>>("Settings");

    // Content layout with semantic spacing
    auto content = std::make_unique<vbox<Backend>>(spacing::medium);
    content->set_padding(thickness::all(spacing::large));

    // Group: Display Settings
    auto display_group = std::make_unique<group_box<Backend>>("Display");
    display_group->set_vbox_layout(spacing::small);

    auto* brightness_row = display_group->template emplace_child<hbox>(spacing::medium);
    brightness_row->template emplace_child<label>("Brightness:");
    auto* brightness = brightness_row->template emplace_child<slider>(slider_orientation::horizontal);
    brightness->set_track_length(25);  // 25 logical units
    brightness->set_range(0, 100);

    content->add_child(std::move(display_group));

    // Group: Text Settings
    auto text_group = std::make_unique<group_box<Backend>>("Text");
    text_group->set_vbox_layout(spacing::small);

    auto* font_row = text_group->template emplace_child<hbox>(spacing::medium);
    font_row->template emplace_child<label>("Font size:");
    auto* font_input = font_row->template emplace_child<line_edit>();
    font_input->set_visible_chars(10);  // 10 characters wide
    font_input->set_text("12");

    content->add_child(std::move(text_group));

    // Preview area
    auto* preview = content->template emplace_child<text_view>();
    preview->set_visible_lines(5);   // 5 lines tall
    preview->set_visible_chars(40);  // 40 chars wide
    preview->set_text("Preview text appears here...\nLine 2\nLine 3");

    // Buttons
    auto button_row = std::make_unique<hbox<Backend>>(spacing::medium);
    button_row->template emplace_child<spring>();  // push buttons right
    button_row->template emplace_child<button>("Cancel");
    button_row->template emplace_child<button>("OK");
    content->add_child(std::move(button_row));

    // Configure window
    dialog->set_content(std::move(content));
    dialog->fit_content();
    dialog->set_min_content_size(50, 15);  // min 50 chars x 15 lines
    dialog->constrain_to_workspace();

    return dialog;
}
```

This code compiles and runs correctly on **both** conio (TUI) and SDL2 (GUI) backends with appropriate sizing.

---

## Summary

| Category | Solution | Type Safety | Conversion Location |
|----------|----------|-------------|---------------------|
| Spacing | `enum class spacing` | Compile-time | Theme at layout time |
| Widget sizes | Semantic methods | Runtime (domain-specific) | Widget at measure time |
| Window sizes | fit_content() + min/max | Runtime | Widget at measure time |

---

## Complete Widget Inventory

### Containers - Need `spacing` Enum

| Widget | Current API | New API |
|--------|-------------|---------|
| `vbox` | `vbox(int spacing)` | `vbox(spacing gap)` |
| `hbox` | `hbox(int spacing)` | `hbox(spacing gap)` |
| `grid` | `grid(cols, rows, col_spacing, row_spacing)` | `grid(cols, rows, spacing col_gap, spacing row_gap)` |
| `group_box` | `set_vbox_layout(int)` | `set_vbox_layout(spacing)` |
| `button_group` | `button_group(orientation, int spacing)` | `button_group(orientation, spacing gap)` |
| `tab_widget` | internal spacing | `set_tab_spacing(spacing)` |

### Containers - Need `spacing` for Padding

| Widget | Current API | New API |
|--------|-------------|---------|
| `panel` | `set_padding(thickness)` | `set_padding(spacing)` or `set_padding(thickness<spacing>)` |
| `group_box` | `set_padding(thickness)` | `set_padding(spacing)` |
| All containers | `set_margin(...)` | `set_margin(spacing)` |

### Text-Based Widgets - Need Semantic Line/Char Sizing

| Widget | New Methods |
|--------|-------------|
| `text_view` | `set_visible_lines(int)`, `set_visible_chars(int)` |
| `line_edit` | `set_visible_chars(int)` |
| `label` | `set_max_chars(int)` (optional, for truncation) |
| `styled_text` | `set_max_chars(int)` (optional) |

### Input Widgets - Need Semantic Track/Bar Sizing

| Widget | New Methods |
|--------|-------------|
| `slider` | `set_track_length(int units)` |
| `progress_bar` | `set_bar_length(int units)` |
| `combo_box` | `set_visible_items(int)` (dropdown height) |

### MVC Views - Need Semantic Sizing

| Widget | New Methods |
|--------|-------------|
| `list_view` | `set_visible_items(int)`, `set_item_width_chars(int)` |

### Layout Helpers - Need Semantic Sizing

| Widget | Current API | New API |
|--------|-------------|---------|
| `spacer` | `spacer(int size)` | `spacer(spacing size)` |
| `spring` | No size needed | No change |

### Window System - Need Content-Based Sizing

| Widget | New Methods |
|--------|-------------|
| `window` | `fit_content()`, `set_min_content_size(chars, lines)`, `set_max_content_size(chars, lines)` |
| `dialog` | Same as window |
| `main_window` | Fills workspace - no explicit sizing |

### Widgets That Don't Need Changes

| Widget | Reason |
|--------|--------|
| `button` | Sizes to content + theme padding |
| `checkbox` | Size from theme (icon-based) |
| `radio_button` | Size from theme (icon-based) |
| `icon` | Size from theme |
| `menu`, `menu_item`, `menu_bar` | Auto-sized from content |
| `scrollbar`, `scrollbar_thumb`, `scrollbar_arrow` | Size from theme |
| `status_bar` | Fills width, height from theme |
| `separator` | Thickness from theme |

---

## Files to Modify

### Core Infrastructure
1. `include/onyxui/layout/layout_strategy.hh` - Add `enum class spacing`
2. `include/onyxui/theming/theme.hh` - Add `spacing_values` struct with `resolve()`

### Containers (spacing in constructors)
3. `include/onyxui/widgets/containers/vbox.hh` - Constructor takes `spacing`
4. `include/onyxui/widgets/containers/hbox.hh` - Constructor takes `spacing`
5. `include/onyxui/widgets/containers/grid.hh` - Constructor takes `spacing` for gaps
6. `include/onyxui/widgets/containers/group_box.hh` - `set_vbox_layout(spacing)`
7. `include/onyxui/widgets/containers/panel.hh` - `set_padding(spacing)`
8. `include/onyxui/widgets/input/button_group.hh` - Constructor takes `spacing`
9. `include/onyxui/widgets/containers/tab_widget.hh` - `set_tab_spacing(spacing)`

### Text-Based Widgets (semantic sizing)
10. `include/onyxui/widgets/text_view.hh` - `set_visible_lines()`, `set_visible_chars()`
11. `include/onyxui/widgets/input/line_edit.hh` - `set_visible_chars()`
12. `include/onyxui/widgets/label.hh` - `set_max_chars()` (optional)

### Input Widgets (semantic sizing)
13. `include/onyxui/widgets/input/slider.hh` - `set_track_length()`
14. `include/onyxui/widgets/progress_bar.hh` - `set_bar_length()`
15. `include/onyxui/widgets/input/combo_box.hh` - `set_visible_items()`

### MVC Views
16. `include/onyxui/mvc/views/list_view.hh` - `set_visible_items()`

### Layout Helpers
17. `include/onyxui/widgets/layout/spacer.hh` - Constructor takes `spacing`

### Window System
18. `include/onyxui/widgets/window/window.hh` - `fit_content()`, semantic min/max
19. `include/onyxui/widgets/window/dialog.hh` - Same as window

### Theme Files
20. `themes/*.yaml` - Add spacing values and text metrics per backend

---

## References

- [Qt Size Policies](https://doc.qt.io/qt-6/qsizepolicy.html)
- [CSS Units](https://developer.mozilla.org/en-US/docs/Learn/CSS/Building_blocks/Values_and_units)
- [Material Design Spacing](https://m3.material.io/foundations/layout/understanding-layout/spacing)
