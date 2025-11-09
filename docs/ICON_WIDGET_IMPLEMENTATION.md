# Icon Widget Implementation Guide

**Author:** Claude Code
**Date:** 2025-11-09
**Status:** Planning Phase
**Priority:** HIGH - Fixes documented architecture violation (hardcoded symbols in widgets)

---

## Table of Contents

1. [Overview](#overview)
2. [Current State Analysis](#current-state-analysis)
3. [Design Decisions](#design-decisions)
4. [Implementation Tasks](#implementation-tasks)
5. [Testing Strategy](#testing-strategy)
6. [Integration Points](#integration-points)
7. [Migration Guide](#migration-guide)
8. [Future Enhancements](#future-enhancements)

---

## Overview

### Problem Statement

The window title bar currently uses hardcoded symbol strings for buttons:
- Menu button: `"[≡]"`
- Minimize button: `"[_]"`
- Maximize button: `"[□]"`
- Close button: `"[X]"`

This violates the framework's theming architecture where all visual elements should be theme-configurable.

### Goals

1. **Create reusable `icon` widget** following Qt's QIcon pattern
2. **Extend `icon_style` enum** with window button glyphs
3. **Make window buttons theme-configurable**
4. **Enable backend-specific icon rendering** (Unicode, ASCII, graphical)
5. **Maintain backwards compatibility** during migration

### Non-Goals

- Bitmap/raster icon support (future enhancement)
- Icon animation (future enhancement)
- Icon size variants (future enhancement)

---

## Current State Analysis

### Existing Infrastructure

#### 1. Backend Icon Support

**File:** `backends/conio/include/onyxui/conio/conio_renderer.hh:100-111`

```cpp
enum class icon_style : uint8_t {
    none,
    check,         // ✓
    cross,         // ✗
    arrow_up,      // ↑
    arrow_down,    // ↓
    arrow_left,    // ←
    arrow_right,   // →
    bullet,        // •
    folder,        // ▶
    file           // ■
};
```

**Status:** ✅ Enum exists, renderer support functional (used by scrollbar arrows)

#### 2. Renderer Methods

**File:** `include/onyxui/core/rendering/draw_context.hh:303-308`

```cpp
[[nodiscard]] size_type draw_icon(
    const icon_type& icon,
    const point_type& position
) override {
    auto icon_size = renderer_type::get_icon_size(icon);
    // ... implementation exists ...
}
```

**Status:** ✅ Rendering infrastructure in place

#### 3. Current Window Button Implementation

**File:** `include/onyxui/widgets/window/window_title_bar.inl:72-99`

```cpp
// Menu button
if (flags.has_menu_button) {
    m_menu_button = this->template emplace_child<button>("[≡]");
    m_menu_button->clicked.connect([this]() {
        menu_clicked.emit();
    });
}

// Minimize button
if (flags.has_minimize_button) {
    m_minimize_button = this->template emplace_child<button>("[_]");
    // ...
}

// Maximize button
if (flags.has_maximize_button) {
    m_maximize_button = this->template emplace_child<button>("[□]");
    // ...
}

// Close button
if (flags.has_close_button) {
    m_close_button = this->template emplace_child<button>("[X]");
    // ...
}
```

**Status:** ❌ Hardcoded strings - needs refactoring

### Dependencies

- ✅ `icon_style` enum (exists in backends)
- ✅ `draw_icon()` method (exists in draw_context)
- ✅ `get_icon_size()` method (exists in renderers)
- ❌ `icon` widget (needs implementation)
- ❌ Theme properties for window button icons (needs addition)

---

## Design Decisions

### 1. Icon Widget Design

**Pattern:** Follow Qt's QIcon conceptually, adapted to OnyxUI architecture

```cpp
template<UIBackend Backend>
class icon : public widget<Backend> {
public:
    using icon_type = typename Backend::renderer_type::icon_style;

    explicit icon(icon_type style);

    // Accessors
    void set_icon(icon_type style);
    [[nodiscard]] icon_type get_icon() const noexcept;

protected:
    void do_render(render_context<Backend>& ctx) const override;

private:
    icon_type m_icon_style;
};
```

**Rationale:**
- Simple, focused widget with single responsibility
- Backend-agnostic through template parameter
- Consistent with framework's widget architecture
- Minimal state (just icon_style enum value)

### 2. Extended Icon Enum

Add window-specific icons to `icon_style`:

```cpp
enum class icon_style : uint8_t {
    // Existing icons
    none,
    check, cross, arrow_up, arrow_down, arrow_left, arrow_right,
    bullet, folder, file,

    // Window management icons (NEW)
    menu,          // ≡ (hamburger menu)
    minimize,      // _ (minimize to taskbar)
    maximize,      // □ (maximize window)
    restore,       // ▢ (restore from maximized)
    close_x        // × (close window)
};
```

**Rationale:**
- Extends existing infrastructure
- Semantic names (not visual descriptions)
- Backend decides actual glyphs
- `restore` needed for maximize/restore toggle

### 3. Theme Integration

Add to theme structure:

```yaml
window:
  buttons:
    menu_icon: menu
    minimize_icon: minimize
    maximize_icon: maximize
    restore_icon: restore
    close_icon: close_x
```

**Rationale:**
- Theme controls visual representation
- Different themes can use different icons
- Consistent with existing theme architecture

### 4. Button Content Strategy

**Option A (Recommended):** Icon as button content
```cpp
auto icon_widget = std::make_unique<icon<Backend>>(icon_style::minimize);
m_minimize_button = emplace_child<button>();
m_minimize_button->set_content(std::move(icon_widget));
```

**Option B:** Extend button to accept icon directly
```cpp
m_minimize_button = emplace_child<button>(icon_style::minimize);
```

**Decision:** Start with Option A (composability), consider Option B as future convenience API.

---

## Implementation Tasks

### Task 1: Extend icon_style Enum

**Difficulty:** Easy
**Files Modified:** 3
**Estimated Time:** 30 minutes

#### Changes Required

1. **conio backend** - `backends/conio/include/onyxui/conio/conio_renderer.hh`
   - Add 5 new enum values after `file`
   - Update documentation comments

2. **sdl2 backend** - `backends/sdl2/include/onyxui/sdl2/sdl2_renderer.hh`
   - Add matching enum values
   - Mark as TODO for implementation

3. **test backend** - `unittest/test_support/test_types.hh`
   - Add to `test_icon_style` enum for testing

#### Implementation

```cpp
// backends/conio/include/onyxui/conio/conio_renderer.hh:100-111

enum class icon_style : uint8_t {
    none,

    // General purpose icons
    check,         // ✓
    cross,         // ✗
    bullet,        // •
    folder,        // ▶
    file,          // ■

    // Navigation arrows
    arrow_up,      // ↑
    arrow_down,    // ↓
    arrow_left,    // ←
    arrow_right,   // →

    // Window management icons (NEW)
    menu,          // ≡ (hamburger menu)
    minimize,      // _ (minimize to taskbar)
    maximize,      // □ (maximize window)
    restore,       // ▢ (restore from maximized)
    close_x        // × (close window)
};
```

#### Glyph Mapping (conio backend)

Update `conio_renderer.cc` icon rendering:

```cpp
// Map icon_style to Unicode glyphs
switch (icon) {
    // ... existing mappings ...
    case icon_style::menu:     return "≡";
    case icon_style::minimize: return "_";
    case icon_style::maximize: return "□";
    case icon_style::restore:  return "▢";
    case icon_style::close_x:  return "×";
}
```

#### Testing

```cpp
TEST_CASE("icon_style - Window management icons") {
    using icon_style = conio_renderer::icon_style;

    CHECK(enum_to_string(icon_style::menu) == "menu");
    CHECK(enum_to_string(icon_style::minimize) == "minimize");
    CHECK(enum_to_string(icon_style::maximize) == "maximize");
    CHECK(enum_to_string(icon_style::restore) == "restore");
    CHECK(enum_to_string(icon_style::close_x) == "close_x");
}
```

#### Success Criteria

- [ ] All 3 backends have matching enum values
- [ ] Enum reflection tests pass
- [ ] No compilation warnings
- [ ] Glyph rendering works in conio backend

---

### Task 2: Create Icon Widget

**Difficulty:** Medium
**Files Created:** 2
**Estimated Time:** 2 hours

#### Files

1. **Header:** `include/onyxui/widgets/icon.hh`
2. **Tests:** `unittest/widgets/test_icon.cc`

#### Implementation

**File:** `include/onyxui/widgets/icon.hh`

```cpp
/**
 * @file icon.hh
 * @brief Icon widget for rendering backend-specific glyphs
 * @author Claude Code
 * @date 2025-11-09
 *
 * @details
 * Lightweight widget for rendering icons using backend's icon_style enum.
 * Similar to Qt's QIcon but adapted to OnyxUI's architecture.
 *
 * Icons are rendered using the backend's draw_icon() method, which maps
 * icon_style enum values to appropriate glyphs (Unicode, ASCII, bitmaps).
 */

#pragma once

#include <onyxui/widgets/core/widget.hh>
#include <onyxui/core/rendering/render_context.hh>

namespace onyxui {

    /**
     * @class icon
     * @brief Widget for rendering backend-specific icon glyphs
     *
     * @tparam Backend The UI backend type
     *
     * @details
     * The icon widget renders a single glyph using the backend's icon_style enum.
     * The actual visual representation is determined by the backend:
     * - conio: Unicode glyphs (✓, ×, ↑, etc.)
     * - SDL2: Rendered graphics
     * - test: Character representation
     *
     * ## Usage
     *
     * @code
     * // Standalone icon
     * auto icon_widget = std::make_unique<icon<Backend>>(icon_style::check);
     *
     * // Icon in button
     * auto icon_content = std::make_unique<icon<Backend>>(icon_style::minimize);
     * button->set_content(std::move(icon_content));
     *
     * // Change icon dynamically
     * icon_widget->set_icon(icon_style::maximize);
     * @endcode
     *
     * ## Theme Integration
     *
     * Icons respect theme colors through inheritance:
     * - Foreground color from parent or theme.text.fg_color
     * - Background color from parent (typically transparent)
     *
     * ## Size
     *
     * Icon size is determined by backend's get_icon_size() method.
     * Most backends return single-character size (1x1 cells in TUI).
     */
    template<UIBackend Backend>
    class icon : public widget<Backend> {
    public:
        using base = widget<Backend>;
        using icon_type = typename Backend::renderer_type::icon_style;
        using render_context_type = render_context<Backend>;
        using size_type = typename Backend::size_type;

        /**
         * @brief Construct icon widget
         * @param style Icon style to display
         */
        explicit icon(icon_type style)
            : base()
            , m_icon_style(style)
        {
        }

        /**
         * @brief Set icon style
         * @param style New icon style
         */
        void set_icon(icon_type style) {
            if (m_icon_style != style) {
                m_icon_style = style;
                this->invalidate_measure();
            }
        }

        /**
         * @brief Get current icon style
         */
        [[nodiscard]] icon_type get_icon() const noexcept {
            return m_icon_style;
        }

    protected:
        /**
         * @brief Render icon using backend's draw_icon() method
         */
        void do_render(render_context_type& ctx) const override {
            if (!this->is_visible() || m_icon_style == icon_type::none) {
                return;
            }

            // Draw icon at widget's position
            ctx.draw_icon(m_icon_style, {this->x(), this->y()});
        }

    private:
        icon_type m_icon_style;
    };

} // namespace onyxui
```

#### Testing

**File:** `unittest/widgets/test_icon.cc`

```cpp
/**
 * @file test_icon.cc
 * @brief Tests for icon widget
 */

#include <doctest/doctest.h>
#include <onyxui/widgets/icon.hh>
#include <unittest/utils/test_canvas_backend.hh>
#include <unittest/utils/ui_context_fixture.hh>

using namespace onyxui;
using namespace onyxui::testing;

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "Icon - Construction and accessors") {
    using icon_style = test_canvas_backend::renderer_type::icon_style;

    SUBCASE("Construct with icon style") {
        auto icon_widget = std::make_unique<icon<test_canvas_backend>>(icon_style::check);

        CHECK(icon_widget->get_icon() == icon_style::check);
    }

    SUBCASE("Set icon style") {
        auto icon_widget = std::make_unique<icon<test_canvas_backend>>(icon_style::check);

        icon_widget->set_icon(icon_style::cross);
        CHECK(icon_widget->get_icon() == icon_style::cross);
    }

    SUBCASE("Set icon invalidates measure") {
        auto icon_widget = std::make_unique<icon<test_canvas_backend>>(icon_style::arrow_up);

        // Measure once
        icon_widget->measure(100, 100);

        // Change icon - should invalidate
        icon_widget->set_icon(icon_style::arrow_down);

        // Measure again should recalculate
        auto size = icon_widget->measure(100, 100);

        // Icon size is typically 1x1 for TUI backends
        CHECK(size.w >= 1);
        CHECK(size.h >= 1);
    }
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "Icon - Rendering") {
    using icon_style = test_canvas_backend::renderer_type::icon_style;

    SUBCASE("Render check icon") {
        auto icon_widget = std::make_unique<icon<test_canvas_backend>>(icon_style::check);

        icon_widget->measure(100, 100);
        icon_widget->arrange({0, 0, 10, 10});

        test_canvas canvas;
        auto ctx = create_render_context(canvas);
        icon_widget->render(*ctx);

        // Verify icon was rendered (backend-specific verification)
        // test_canvas should record draw_icon() call
    }

    SUBCASE("None icon doesn't render") {
        auto icon_widget = std::make_unique<icon<test_canvas_backend>>(icon_style::none);

        icon_widget->measure(100, 100);
        icon_widget->arrange({0, 0, 10, 10});

        test_canvas canvas;
        auto ctx = create_render_context(canvas);
        icon_widget->render(*ctx);

        // Verify nothing rendered
    }
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "Icon - Window management icons") {
    using icon_style = test_canvas_backend::renderer_type::icon_style;

    SUBCASE("Menu icon") {
        auto icon_widget = std::make_unique<icon<test_canvas_backend>>(icon_style::menu);
        CHECK(icon_widget->get_icon() == icon_style::menu);
    }

    SUBCASE("Minimize icon") {
        auto icon_widget = std::make_unique<icon<test_canvas_backend>>(icon_style::minimize);
        CHECK(icon_widget->get_icon() == icon_style::minimize);
    }

    SUBCASE("Maximize icon") {
        auto icon_widget = std::make_unique<icon<test_canvas_backend>>(icon_style::maximize);
        CHECK(icon_widget->get_icon() == icon_style::maximize);
    }

    SUBCASE("Restore icon") {
        auto icon_widget = std::make_unique<icon<test_canvas_backend>>(icon_style::restore);
        CHECK(icon_widget->get_icon() == icon_style::restore);
    }

    SUBCASE("Close icon") {
        auto icon_widget = std::make_unique<icon<test_canvas_backend>>(icon_style::close_x);
        CHECK(icon_widget->get_icon() == icon_style::close_x);
    }
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "Icon - Size calculation") {
    using icon_style = test_canvas_backend::renderer_type::icon_style;

    auto icon_widget = std::make_unique<icon<test_canvas_backend>>(icon_style::bullet);

    auto size = icon_widget->measure(100, 100);

    // Icon should have minimum size of 1x1
    CHECK(size.w >= 1);
    CHECK(size.h >= 1);

    // Most TUI icons are 1x1 character cells
    // (Actual size depends on backend implementation)
}

TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "Icon - Theme integration") {
    using icon_style = test_canvas_backend::renderer_type::icon_style;

    SUBCASE("Icon inherits theme colors") {
        auto icon_widget = std::make_unique<icon<test_canvas_backend>>(icon_style::check);

        // Apply theme
        icon_widget->apply_theme("Norton Blue", themes);

        // Icon should inherit text color from theme
        // (Verification depends on theme system integration)
    }
}
```

#### Success Criteria

- [ ] Icon widget compiles without warnings
- [ ] All 25+ test cases pass
- [ ] Widget follows framework patterns (measure/arrange/render)
- [ ] Works with all icon_style enum values
- [ ] Properly invalidates layout when icon changes

---

### Task 3: Add Theme Properties for Window Icons

**Difficulty:** Easy
**Files Modified:** 3
**Estimated Time:** 1 hour

#### Changes Required

1. **Theme structure** - `include/onyxui/theming/theme.hh`
   - Add window button icon properties

2. **Example themes** - Update all theme YAML files
   - Add window.buttons section with icon mappings

3. **Theme tests** - `unittest/reflection/test_theme_yaml.cc`
   - Verify theme deserialization

#### Theme Structure Extension

**File:** `include/onyxui/theming/theme.hh`

```cpp
struct theme {
    // ... existing properties ...

    struct {
        // ... existing window properties ...

        struct {
            icon_style menu_icon = icon_style::menu;
            icon_style minimize_icon = icon_style::minimize;
            icon_style maximize_icon = icon_style::maximize;
            icon_style restore_icon = icon_style::restore;
            icon_style close_icon = icon_style::close_x;
        } buttons;
    } window;
};
```

#### Example Theme YAML

**File:** `themes/norton_blue.yaml`

```yaml
name: "Norton Blue"
extends: null

window:
  # ... existing window theme properties ...

  buttons:
    menu_icon: menu
    minimize_icon: minimize
    maximize_icon: maximize
    restore_icon: restore
    close_icon: close_x
```

**Alternative (ASCII-only theme):**

```yaml
name: "ASCII Classic"

window:
  buttons:
    menu_icon: bullet       # Use • instead of ≡
    minimize_icon: arrow_down
    maximize_icon: folder
    restore_icon: folder
    close_icon: cross
```

#### Success Criteria

- [ ] Theme structure includes window.buttons properties
- [ ] All example themes have window button icon mappings
- [ ] Theme deserialization tests pass
- [ ] Default values work without theme

---

### Task 4: Update window_title_bar to Use Icons

**Difficulty:** Medium
**Files Modified:** 2
**Estimated Time:** 2 hours

#### Changes Required

1. **Header** - `include/onyxui/widgets/window/window_title_bar.hh`
   - Add icon widget pointers

2. **Implementation** - `include/onyxui/widgets/window/window_title_bar.inl`
   - Replace hardcoded strings with icon widgets
   - Get icon styles from theme

#### Implementation

**File:** `include/onyxui/widgets/window/window_title_bar.inl`

Replace `create_buttons()` method:

```cpp
template<UIBackend Backend>
void window_title_bar<Backend>::create_buttons(const window_flags& flags) {
    using icon_style = typename Backend::renderer_type::icon_style;

    // Get theme (or use defaults if no theme applied)
    // TODO: Access theme properly through themeable system

    // Menu button (left side, optional)
    if (flags.has_menu_button) {
        m_menu_button = this->template emplace_child<button>();

        // Create icon widget as button content
        auto icon_widget = std::make_unique<icon<Backend>>(icon_style::menu);
        m_menu_icon = icon_widget.get();  // Keep reference for theme updates
        m_menu_button->set_content(std::move(icon_widget));

        m_menu_button->clicked.connect([this]() {
            menu_clicked.emit();
        });
    }

    // Minimize button
    if (flags.has_minimize_button) {
        m_minimize_button = this->template emplace_child<button>();

        auto icon_widget = std::make_unique<icon<Backend>>(icon_style::minimize);
        m_minimize_icon = icon_widget.get();
        m_minimize_button->set_content(std::move(icon_widget));

        m_minimize_button->clicked.connect([this]() {
            minimize_clicked.emit();
        });
    }

    // Maximize button
    if (flags.has_maximize_button) {
        m_maximize_button = this->template emplace_child<button>();

        auto icon_widget = std::make_unique<icon<Backend>>(icon_style::maximize);
        m_maximize_icon = icon_widget.get();
        m_maximize_button->set_content(std::move(icon_widget));

        m_maximize_button->clicked.connect([this]() {
            maximize_clicked.emit();
        });
    }

    // Close button
    if (flags.has_close_button) {
        m_close_button = this->template emplace_child<button>();

        auto icon_widget = std::make_unique<icon<Backend>>(icon_style::close_x);
        m_close_icon = icon_widget.get();
        m_close_button->set_content(std::move(icon_widget));

        m_close_button->clicked.connect([this]() {
            close_clicked.emit();
        });
    }
}
```

**Alternative (if button doesn't support set_content yet):**

Create button subclass or use horizontal composition:

```cpp
// Create hbox with icon and optional label
auto button_content = std::make_unique<hbox<Backend>>(0);
button_content->emplace_child<icon>(icon_style::minimize);
m_minimize_button = this->template emplace_child<button>();
// ... (may need button API extension)
```

#### Header Changes

**File:** `include/onyxui/widgets/window/window_title_bar.hh`

Add icon widget references:

```cpp
template<UIBackend Backend>
class window_title_bar : public widget_container<Backend> {
private:
    // ... existing members ...

    // Icon widget references (non-owning)
    icon<Backend>* m_menu_icon = nullptr;
    icon<Backend>* m_minimize_icon = nullptr;
    icon<Backend>* m_maximize_icon = nullptr;
    icon<Backend>* m_close_icon = nullptr;
};
```

#### Maximize/Restore Toggle

Add method to toggle maximize button icon:

```cpp
template<UIBackend Backend>
void window_title_bar<Backend>::set_maximized(bool maximized) {
    if (m_maximize_icon) {
        using icon_style = typename Backend::renderer_type::icon_style;
        m_maximize_icon->set_icon(
            maximized ? icon_style::restore : icon_style::maximize
        );
    }
}
```

#### Success Criteria

- [ ] All window buttons use icon widgets
- [ ] No hardcoded symbol strings remain
- [ ] Maximize/restore toggle works
- [ ] All existing window tests pass
- [ ] Visual appearance matches previous implementation

---

### Task 5: Update Button to Support Widget Content

**Difficulty:** Medium (if needed)
**Files Modified:** 2
**Estimated Time:** 2 hours
**Status:** OPTIONAL - only if button doesn't support child widgets

#### Current Button Analysis

Check if `button` widget supports child content:

```cpp
// If button is simple widget (not container):
class button : public widget<Backend> {
    std::string m_text;  // Only text supported
};

// OR if button is already container:
class button : public widget_container<Backend> {
    // Can add children
};
```

#### Option 1: Button is Already Container

If button inherits from `widget_container`, just use existing child management:

```cpp
auto icon_widget = std::make_unique<icon<Backend>>(icon_style::minimize);
m_minimize_button->add_child(std::move(icon_widget));
```

#### Option 2: Extend Button to Accept Content Widget

If button is simple widget, add content support:

**File:** `include/onyxui/widgets/button.hh`

```cpp
template<UIBackend Backend>
class button : public widget_container<Backend> {  // Change from widget to widget_container
public:
    // ... existing constructors ...

    /**
     * @brief Set button content widget (replaces text)
     * @param content Widget to display (icon, label, hbox, etc.)
     */
    void set_content(std::unique_ptr<ui_element<Backend>> content);

private:
    std::string m_text;           // Text content (if no widget content)
    ui_element<Backend>* m_content = nullptr;  // Widget content (optional)
};
```

#### Option 3: Composition Approach

Create button content as hbox:

```cpp
auto content = std::make_unique<hbox<Backend>>(2);  // 2px spacing
content->emplace_child<icon>(icon_style::minimize);
// Optionally add text label too
content->emplace_child<label>("Minimize");

m_minimize_button = this->template emplace_child<button>();
m_minimize_button->set_content(std::move(content));
```

#### Success Criteria

- [ ] Button supports icon widgets as content
- [ ] Backwards compatible with text-only buttons
- [ ] All button tests pass
- [ ] Documentation updated

---

### Task 6: Update Tests for Icon-Based Buttons

**Difficulty:** Easy
**Files Modified:** 2+
**Estimated Time:** 1 hour

#### Tests to Update

1. **Window title bar tests** - `unittest/widgets/test_window.cc`
   - Verify buttons contain icons (not text strings)
   - Test icon changes on maximize/restore

2. **Window theming tests** - `unittest/widgets/test_window_theming.cc`
   - Verify theme icon properties apply to buttons

#### Example Test Updates

```cpp
TEST_CASE_FIXTURE(ui_context_fixture<test_canvas_backend>, "Window - Title bar buttons use icons") {
    auto win = std::make_unique<window<test_canvas_backend>>("Test Window");

    // Get title bar
    auto* title_bar = /* ... access title bar ... */;

    SUBCASE("Minimize button has icon") {
        auto* minimize_btn = title_bar->get_minimize_button();
        REQUIRE(minimize_btn != nullptr);

        // Verify button contains icon widget (not text)
        // (Implementation depends on button API)
    }

    SUBCASE("Maximize icon toggles to restore") {
        auto* maximize_btn = title_bar->get_maximize_button();

        // Initially shows maximize icon
        // ... verify icon is maximize ...

        // After maximize, shows restore icon
        win->maximize();
        // ... verify icon is restore ...
    }
}

TEST_CASE("Window - Theme icon properties apply") {
    // Create theme with custom icon mappings
    theme custom_theme;
    custom_theme.window.buttons.minimize_icon = icon_style::arrow_down;

    auto win = std::make_unique<window<test_canvas_backend>>("Test");
    win->apply_theme(custom_theme);

    // Verify minimize button uses arrow_down icon
    // ...
}
```

#### Success Criteria

- [ ] All existing window tests updated and passing
- [ ] New tests for icon functionality added
- [ ] Test coverage remains >95%
- [ ] No test failures or warnings

---

## Testing Strategy

### Unit Tests

#### Icon Widget Tests (25+ cases)
- Construction with various icon styles
- `get_icon()` / `set_icon()` accessors
- Icon change invalidates layout
- Rendering with different icon styles
- None icon doesn't render
- Theme color inheritance
- Window management icons (menu, minimize, maximize, restore, close)

#### Window Button Tests (10+ cases)
- Title bar creates icon-based buttons
- Buttons render correct icons
- Maximize/restore toggle
- Theme icon properties apply
- No hardcoded strings in output

### Integration Tests

#### End-to-End Window Tests
```cpp
TEST_CASE("Window - Complete icon integration") {
    auto ctx = create_ui_context<test_canvas_backend>();

    // Create window with custom theme
    auto win = std::make_unique<window<test_canvas_backend>>("Test");
    win->apply_theme("Norton Blue", ctx.themes());

    // Measure and arrange
    win->measure(80, 25);
    win->arrange({0, 0, 80, 25});

    // Render
    test_canvas canvas;
    auto render_ctx = create_render_context(canvas);
    win->render(*render_ctx);

    // Verify:
    // 1. Title bar buttons visible
    // 2. Icons rendered (not text strings)
    // 3. Theme colors applied
    // 4. Click events work
}
```

### Visual Regression Tests

#### Comparison Screenshots
- Before: Window with text buttons `"[_]"`, `"[□]"`, `"[X]"`
- After: Window with icon buttons (Unicode glyphs)
- Verify: Visual appearance matches (modulo font rendering)

### Backend-Specific Tests

#### Conio Backend
- Verify Unicode glyphs render correctly
- Test ASCII fallback if needed

#### SDL2 Backend
- Mark icons as TODO placeholders
- Document future graphical icon support

---

## Integration Points

### 1. Button Widget

**Current State:** Unknown if button supports child widgets

**Required:**
- Button must support widget content (not just text)
- Alternative: Use composition (hbox with icon + label)

**Action:** Investigate button implementation before Task 4

### 2. Theme System

**Current State:** ✅ Theme inheritance works via themeable

**Required:**
- Add window.buttons properties to theme structure
- Update all example themes
- Ensure icon widgets inherit theme colors

**Action:** Straightforward theme extension

### 3. Renderer Backend

**Current State:** ✅ Icon rendering exists (used by scrollbar)

**Required:**
- Map new icon_style enum values to glyphs
- Update conio renderer glyph table

**Action:** Simple enum extension

### 4. Test Infrastructure

**Current State:** ✅ test_canvas_backend has icon support

**Required:**
- Extend test_icon_style enum
- Verify test renderer handles new icons

**Action:** Mirror production enum changes

---

## Migration Guide

### Phase 1: Non-Breaking Addition

1. **Add icon widget** (Tasks 1-2)
   - Extend icon_style enum
   - Create icon widget
   - Add tests

**Status:** Zero breaking changes, new functionality only

### Phase 2: Theme Extension

2. **Add theme properties** (Task 3)
   - Add window.buttons to theme structure
   - Update example themes
   - Provide sensible defaults

**Status:** Backwards compatible (defaults work if theme incomplete)

### Phase 3: Window Refactoring

3. **Update window_title_bar** (Tasks 4-5)
   - Replace hardcoded strings with icon widgets
   - Update tests

**Status:** Internal refactoring, no API changes

### Phase 4: Documentation

4. **Update docs**
   - Widget library documentation
   - Theming guide
   - Migration notes

**Status:** Documentation only

### Rollback Plan

If issues discovered:
1. Icon widget isolated - can be unused without impact
2. Theme properties have defaults - themes don't break
3. Window changes self-contained - can revert single file

---

## Future Enhancements

### Icon Sizes

Add size variants:
```cpp
enum class icon_size : uint8_t {
    small,   // 1x1 (TUI), 16x16 (GUI)
    medium,  // 2x2 (TUI), 32x32 (GUI)
    large    // 3x3 (TUI), 48x48 (GUI)
};
```

### Icon States

Add state variants:
```cpp
struct icon_state {
    icon_style style;
    bool enabled = true;
    bool highlighted = false;
};
```

### Bitmap Icons

Add raster/vector image support:
```cpp
class bitmap_icon : public icon<Backend> {
    std::filesystem::path m_icon_path;  // PNG, SVG, etc.
};
```

### Icon Themes

Separate icon theme from color theme:
```yaml
icon_theme: "feather"  # or "material", "fontawesome", etc.
color_theme: "norton_blue"
```

### Animated Icons

Add animation support:
```cpp
class animated_icon : public icon<Backend> {
    std::vector<icon_style> m_frames;
    int m_frame_delay_ms;
};
```

---

## Success Metrics

### Code Quality
- [ ] Zero compiler warnings
- [ ] All tests passing (1292+ tests)
- [ ] clang-tidy clean
- [ ] No hardcoded strings in widgets

### Test Coverage
- [ ] Icon widget: 25+ test cases
- [ ] Window buttons: 10+ test cases
- [ ] Integration: 5+ end-to-end tests
- [ ] Coverage >95%

### Performance
- [ ] No measurable performance regression
- [ ] Icon rendering as fast as text rendering
- [ ] No additional allocations in hot paths

### Documentation
- [ ] Icon widget documented
- [ ] Theme guide updated
- [ ] Migration notes complete
- [ ] Example code provided

---

## Timeline Estimate

| Task | Difficulty | Time | Dependencies |
|------|-----------|------|--------------|
| 1. Extend icon_style enum | Easy | 30m | None |
| 2. Create icon widget | Medium | 2h | Task 1 |
| 3. Add theme properties | Easy | 1h | Task 1 |
| 4. Update window_title_bar | Medium | 2h | Tasks 2-3 |
| 5. Extend button (if needed) | Medium | 2h | Task 2 |
| 6. Update tests | Easy | 1h | Task 4 |
| **Total** | | **8-10 hours** | |

**Note:** Times assume familiarity with codebase. Add 2-3 hours for investigation/debugging.

---

## Open Questions

1. **Button API:** Does button support child widgets, or only text?
   - **Action:** Check button implementation before Task 4

2. **Icon sizing:** Should icons respect DPI/scale factors?
   - **Action:** Defer to future enhancement

3. **Backwards compatibility:** Should old text-based buttons still work?
   - **Decision:** Yes, but mark as deprecated

4. **Theme defaults:** What if theme doesn't specify icon styles?
   - **Decision:** Use enum defaults (menu, minimize, maximize, close_x)

---

## References

- **Qt Documentation:** QIcon class design
- **Material Design Icons:** Icon naming conventions
- **Feather Icons:** Minimal icon set examples
- **Framework Patterns:** OnyxUI widget architecture (CLAUDE.md)

---

**End of Document**
