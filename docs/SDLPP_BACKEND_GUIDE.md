# SDL++ Backend Implementation Guide

This document provides comprehensive guidance for implementing an SDL++ (lib_sdlpp) based backend for OnyxUI. The SDL++ library is a modern C++20 wrapper for SDL3, providing RAII-managed interfaces and type-safe operations.

---

## Table of Contents

- [Overview](#overview)
- [Prerequisites](#prerequisites)
- [CMake Configuration](#cmake-configuration)
- [Directory Structure](#directory-structure)
- [Backend Definition](#backend-definition)
- [Geometry Types](#geometry-types)
- [Color Type](#color-type)
- [Event System](#event-system)
- [Renderer Implementation](#renderer-implementation)
- [Theme Registration](#theme-registration)
- [Complete Implementation Checklist](#complete-implementation-checklist)
- [Testing](#testing)
- [Example Application](#example-application)

---

## Overview

The SDL++ backend provides a graphical UI backend using SDL3 for rendering. Unlike the text-based conio backend, this backend supports:

- **Pixel-based rendering** with hardware acceleration
- **TrueType font support** via lib_sdlpp's built-in font system
- **Full mouse tracking** with motion events
- **Smooth animations** and effects
- **Image/texture support** for icons and backgrounds

### lib_sdlpp Features

[lib_sdlpp](https://github.com/devbrain/lib_sdlpp) provides:

- **RAII wrappers** - Automatic resource management
- **std::expected error handling** - No exceptions, explicit error states
- **Concept-based geometry** - Works with any math library satisfying point_like/rect_like concepts
- **Type-safe API** - Modern C++20 design
- **Built-in font rendering** - TTF, FON, BGI, raw BIOS fonts via `sdlpp::font::font`
- **Built-in image loading** - 30+ image formats via `sdlpp::image::load()`
- **Comprehensive renderer** - DDA algorithms for antialiased drawing

---

## Prerequisites

### Dependencies

1. **lib_sdlpp** - C++20 SDL3 wrapper (fetched via neutrino-cmake)

**Note:** lib_sdlpp manages all dependencies internally:
- **SDL3** - Fetched and linked automatically by lib_sdlpp
- **Font rendering** - Built-in via onyx_font library
- **Image loading** - Built-in via onyx_image library (30+ formats)

No separate SDL3, SDL3_ttf, or SDL3_image dependencies are needed.

### Compiler Requirements

- C++20 support required
- GCC 10+, Clang 12+, or MSVC 2019+

---

## CMake Configuration

### Backend CMakeLists.txt

Create `backends/sdlpp/CMakeLists.txt`:

```cmake
# =============================================================================
# SDL++ Backend CMake Configuration
# =============================================================================

# Fetch lib_sdlpp via neutrino-cmake
# (automatically fetches SDL3 and includes font/image support)
include(${NEUTRINO_CMAKE_DIR}/deps/sdlpp.cmake)
neutrino_fetch_sdlpp()

# Create SDL++ backend library using the helper function
add_backend_library(
    NAME sdlpp
    SOURCES
        src/sdlpp_renderer.cc
        src/sdlpp_backend.cc
    PUBLIC_HEADERS
        include/onyxui/sdlpp/colors.hh
        include/onyxui/sdlpp/geometry.hh
        include/onyxui/sdlpp/sdlpp_backend.hh
        include/onyxui/sdlpp/sdlpp_events.hh
        include/onyxui/sdlpp/sdlpp_renderer.hh
    PRIVATE_SOURCES
        src/sdlpp_themes.hh
    DEPENDENCIES
        neutrino::sdlpp
)

# Demo application
add_executable(sdlpp_demo demo.cc)
target_link_libraries(sdlpp_demo PRIVATE onyxui_sdlpp_backend)
```

### Main CMakeLists.txt Option

The main CMakeLists.txt already has (after removing SDL2):

```cmake
option(NEUTRINO_ONYX_UI_BUILD_BACKEND_SDLPP "Build SDL++ (graphical) backend" OFF)

if(NEUTRINO_ONYX_UI_BUILD_BACKEND_SDLPP)
    add_subdirectory(backends/sdlpp)
endif()
```

---

## Directory Structure

```
backends/sdlpp/
├── CMakeLists.txt
├── demo.cc                          # Demo application
├── include/
│   └── onyxui/
│       └── sdlpp/
│           ├── colors.hh            # Color type definition
│           ├── geometry.hh          # rect, size, point types
│           ├── sdlpp_backend.hh     # Backend definition + event conversion
│           ├── sdlpp_events.hh      # Event type aliases
│           └── sdlpp_renderer.hh    # Renderer with styles and methods
├── src/
│   ├── sdlpp_backend.cc             # register_themes() implementation
│   ├── sdlpp_renderer.cc            # Rendering implementation
│   └── sdlpp_themes.hh              # Built-in theme definitions
└── unittest/                        # Backend-specific tests (optional)
    └── CMakeLists.txt
```

---

## Backend Definition

### `include/onyxui/sdlpp/sdlpp_backend.hh`

```cpp
/**
 * @file sdlpp_backend.hh
 * @brief SDL++ backend implementation using lib_sdlpp for SDL3
 */

#pragma once

#include <onyxui/concepts/backend.hh>
#include <onyxui/sdlpp/geometry.hh>
#include <onyxui/sdlpp/colors.hh>
#include <onyxui/sdlpp/sdlpp_renderer.hh>
#include <onyxui/sdlpp/sdlpp_events.hh>
#include <onyxui/events/ui_event.hh>
#include <onyxui/theming/theme_registry.hh>
#include <onyxui/backends/sdlpp/onyxui_sdlpp_export.h>
#include <optional>

namespace onyxui::sdlpp {

/**
 * @struct sdlpp_backend
 * @brief SDL++ backend for graphical UIs using SDL3 via lib_sdlpp
 *
 * This backend provides all required types for the UIBackend concept:
 * - Geometric types: rect, size, point (pixel-based)
 * - Event types: SDL3 events via lib_sdlpp
 * - Renderer: Hardware-accelerated 2D rendering
 *
 * @example Usage
 * @code
 * using element = ui_element<sdlpp_backend>;
 * auto root = std::make_unique<element>();
 *
 * sdlpp_renderer renderer(sdl_renderer_ptr);
 *
 * root->measure(800, 600);
 * root->arrange(sdlpp_backend::rect{0, 0, 800, 600});
 * root->render(&renderer);
 * @endcode
 */
struct sdlpp_backend {
    // Required geometric types (pixel-based)
    using rect_type = rect;
    using size_type = size;
    using point_type = point;

    // Required event types (SDL3 events)
    using event_type = sdl_event;
    using keyboard_event_type = sdl_keyboard_event;
    using mouse_button_event_type = sdl_mouse_button_event;
    using mouse_motion_event_type = sdl_mouse_motion_event;
    using mouse_wheel_event_type = sdl_mouse_wheel_event;
    using text_input_event_type = sdl_text_input_event;
    using window_event_type = sdl_window_event;

    // Required rendering types
    using color_type = color;
    using renderer_type = sdlpp_renderer;

    // Backend identification
    static constexpr const char* name() { return "SDL++"; }

    // GUI backends have full mouse tracking
    static constexpr bool has_mouse_tracking = true;

    /**
     * @brief Convert SDL3 event to framework-level ui_event
     * @param native SDL event from event queue
     * @return Framework event, or nullopt if unknown/unhandled event type
     */
    [[nodiscard]] static std::optional<onyxui::ui_event> create_event(
        const sdl_event& native) noexcept;

    /**
     * @brief Register backend-provided themes
     * @param registry Theme registry to populate
     *
     * Registered themes:
     * 1. "Windows 3.11" (default) - Classic Windows 3.11 style
     */
    ONYXUI_SDLPP_EXPORT static void register_themes(
        onyxui::theme_registry<sdlpp_backend>& registry);
};

} // namespace onyxui::sdlpp

// Static assertion to verify backend compliance
static_assert(onyxui::UIBackend<onyxui::sdlpp::sdlpp_backend>,
              "sdlpp_backend must satisfy UIBackend concept");
```

---

## Geometry Types

### `include/onyxui/sdlpp/geometry.hh`

```cpp
/**
 * @file geometry.hh
 * @brief Pixel-based geometric types for SDL++ backend
 *
 * Note: lib_sdlpp provides its own geometry types (sdlpp::point, sdlpp::rect,
 * sdlpp::size) that can be used directly. However, we define OnyxUI-specific
 * types to ensure concept compliance and maintain consistency with other
 * backends.
 *
 * lib_sdlpp's concept-based API accepts any types satisfying point_like,
 * rect_like, etc., so these types work seamlessly with lib_sdlpp functions.
 */

#pragma once

#include <cstdint>

namespace onyxui::sdlpp {

/**
 * @struct rect
 * @brief Rectangle with pixel coordinates
 *
 * Satisfies both OnyxUI RectLike and lib_sdlpp rect_like concepts.
 */
struct rect {
    int x = 0;      ///< X coordinate (pixels from left)
    int y = 0;      ///< Y coordinate (pixels from top)
    int w = 0;      ///< Width in pixels
    int h = 0;      ///< Height in pixels

    constexpr bool operator==(const rect&) const noexcept = default;
};

/**
 * @struct size
 * @brief Size with pixel dimensions
 *
 * Satisfies both OnyxUI SizeLike and lib_sdlpp size_like concepts.
 */
struct size {
    int w = 0;      ///< Width in pixels
    int h = 0;      ///< Height in pixels

    constexpr bool operator==(const size&) const noexcept = default;
};

/**
 * @struct point
 * @brief 2D point with pixel coordinates
 *
 * Satisfies both OnyxUI PointLike and lib_sdlpp point_like concepts.
 */
struct point {
    int x = 0;      ///< X coordinate
    int y = 0;      ///< Y coordinate

    constexpr bool operator==(const point&) const noexcept = default;
};

} // namespace onyxui::sdlpp
```

---

## Color Type

### `include/onyxui/sdlpp/colors.hh`

```cpp
/**
 * @file colors.hh
 * @brief RGBA color type for SDL++ backend
 *
 * Note: lib_sdlpp provides sdlpp::color with similar structure.
 * We define our own for OnyxUI consistency.
 */

#pragma once

#include <cstdint>

namespace onyxui::sdlpp {

/**
 * @struct color
 * @brief RGBA color with 8-bit components
 *
 * Standard RGBA color representation for graphical rendering.
 * Compatible with sdlpp::color for seamless conversion.
 */
struct color {
    uint8_t r = 0;      ///< Red component (0-255)
    uint8_t g = 0;      ///< Green component (0-255)
    uint8_t b = 0;      ///< Blue component (0-255)
    uint8_t a = 255;    ///< Alpha component (0=transparent, 255=opaque)

    constexpr color() noexcept = default;

    constexpr color(uint8_t red, uint8_t green, uint8_t blue,
                    uint8_t alpha = 255) noexcept
        : r(red), g(green), b(blue), a(alpha) {}

    constexpr bool operator==(const color&) const noexcept = default;

    // Conversion to sdlpp::color
    [[nodiscard]] constexpr auto to_sdlpp() const noexcept {
        return ::sdlpp::color{r, g, b, a};
    }

    // Common color constants
    static constexpr color black() { return {0, 0, 0}; }
    static constexpr color white() { return {255, 255, 255}; }
    static constexpr color red() { return {255, 0, 0}; }
    static constexpr color green() { return {0, 255, 0}; }
    static constexpr color blue() { return {0, 0, 255}; }
    static constexpr color transparent() { return {0, 0, 0, 0}; }
};

} // namespace onyxui::sdlpp
```

---

## Event System

### `include/onyxui/sdlpp/sdlpp_events.hh`

```cpp
/**
 * @file sdlpp_events.hh
 * @brief Event type aliases for SDL++ backend
 *
 * lib_sdlpp provides comprehensive event handling via sdlpp::event_queue
 * and type-safe event variants.
 */

#pragma once

#include <sdlpp/events/events.hh>

namespace onyxui::sdlpp {

// SDL3 event types via lib_sdlpp
using sdl_event = sdlpp::event;

// Specific event types for UIBackend concept compliance
using sdl_keyboard_event = sdlpp::event;
using sdl_mouse_button_event = sdlpp::event;
using sdl_mouse_motion_event = sdlpp::event;
using sdl_mouse_wheel_event = sdlpp::event;
using sdl_text_input_event = sdlpp::event;
using sdl_window_event = sdlpp::event;

} // namespace onyxui::sdlpp
```

### Event Conversion Implementation

In `src/sdlpp_backend.cc`:

```cpp
#include <onyxui/sdlpp/sdlpp_backend.hh>
#include "sdlpp_themes.hh"

namespace onyxui::sdlpp {

std::optional<ui_event> sdlpp_backend::create_event(
    const sdl_event& native) noexcept
{
    using namespace onyxui;

    // Use lib_sdlpp's type-safe event handling
    return std::visit([](const auto& evt) -> std::optional<ui_event> {
        using T = std::decay_t<decltype(evt)>;

        // Keyboard events
        if constexpr (std::is_same_v<T, sdlpp::keyboard_event>) {
            keyboard_event kbd{};
            kbd.key = map_sdl_keycode(evt.keycode);
            kbd.pressed = evt.state == sdlpp::key_state::pressed;

            key_modifier mods = key_modifier::none;
            if (evt.mod & sdlpp::key_mod::ctrl) mods |= key_modifier::ctrl;
            if (evt.mod & sdlpp::key_mod::shift) mods |= key_modifier::shift;
            if (evt.mod & sdlpp::key_mod::alt) mods |= key_modifier::alt;
            kbd.modifiers = mods;

            return ui_event{kbd};
        }

        // Mouse button events
        if constexpr (std::is_same_v<T, sdlpp::mouse_button_event>) {
            mouse_event mouse{};
            mouse.x = static_cast<int>(evt.x);
            mouse.y = static_cast<int>(evt.y);

            switch (evt.button) {
                case sdlpp::mouse_button::left:
                    mouse.btn = mouse_event::button::left;
                    break;
                case sdlpp::mouse_button::right:
                    mouse.btn = mouse_event::button::right;
                    break;
                case sdlpp::mouse_button::middle:
                    mouse.btn = mouse_event::button::middle;
                    break;
                default:
                    mouse.btn = mouse_event::button::none;
            }

            mouse.act = evt.state == sdlpp::button_state::pressed
                ? mouse_event::action::press
                : mouse_event::action::release;

            return ui_event{mouse};
        }

        // Mouse motion events
        if constexpr (std::is_same_v<T, sdlpp::mouse_motion_event>) {
            mouse_event mouse{};
            mouse.x = static_cast<int>(evt.x);
            mouse.y = static_cast<int>(evt.y);
            mouse.btn = mouse_event::button::none;
            mouse.act = mouse_event::action::move;
            return ui_event{mouse};
        }

        // Mouse wheel events
        if constexpr (std::is_same_v<T, sdlpp::mouse_wheel_event>) {
            mouse_event mouse{};
            mouse.x = static_cast<int>(evt.mouse_x);
            mouse.y = static_cast<int>(evt.mouse_y);
            mouse.btn = mouse_event::button::none;

            if (evt.y > 0) {
                mouse.act = mouse_event::action::wheel_up;
            } else if (evt.y < 0) {
                mouse.act = mouse_event::action::wheel_down;
            } else {
                return std::nullopt;
            }
            return ui_event{mouse};
        }

        // Window resize events
        if constexpr (std::is_same_v<T, sdlpp::window_event>) {
            if (evt.event_id == sdlpp::window_event_id::resized) {
                resize_event resize{};
                resize.width = evt.data1;
                resize.height = evt.data2;
                return ui_event{resize};
            }
        }

        return std::nullopt;
    }, native);
}

void sdlpp_backend::register_themes(
    theme_registry<sdlpp_backend>& registry)
{
    registry.register_theme(create_windows311_theme());
}

} // namespace onyxui::sdlpp
```

---

## Renderer Implementation

### `include/onyxui/sdlpp/sdlpp_renderer.hh`

```cpp
/**
 * @file sdlpp_renderer.hh
 * @brief SDL++ renderer implementing RenderLike concept
 *
 * Uses lib_sdlpp's built-in:
 * - sdlpp::renderer for hardware-accelerated 2D rendering
 * - sdlpp::font::font for text measurement and rendering
 * - sdlpp::image for icon/texture loading
 */

#pragma once

#include <cstdint>
#include <string_view>
#include <memory>
#include <vector>
#include <filesystem>

#include <onyxui/sdlpp/geometry.hh>
#include <onyxui/sdlpp/colors.hh>
#include <onyxui/backends/sdlpp/onyxui_sdlpp_export.h>

// Forward declarations
namespace sdlpp {
    class renderer;
    namespace font {
        class font;
    }
}

namespace onyxui::sdlpp {

/**
 * @class sdlpp_renderer
 * @brief Hardware-accelerated 2D renderer using lib_sdlpp
 *
 * Implements the RenderLike concept for graphical UI rendering.
 * Uses lib_sdlpp's built-in font and image support.
 */
class ONYXUI_SDLPP_EXPORT sdlpp_renderer {
public:
    // =================================================================
    // Required Types for RenderLike Concept
    // =================================================================

    enum class border_style_type : uint8_t {
        none,
        flat,
        raised,
        sunken,
        etched,
        thick_raised,
        thick_sunken
    };

    struct box_style {
        border_style_type style = border_style_type::none;
        bool is_solid = true;

        constexpr box_style() noexcept = default;
        constexpr explicit box_style(border_style_type s, bool solid = true) noexcept
            : style(s), is_solid(solid) {}
        constexpr bool operator==(const box_style&) const noexcept = default;
    };

    struct line_style {
        border_style_type style = border_style_type::flat;
        int thickness = 1;

        constexpr line_style() noexcept = default;
        constexpr explicit line_style(border_style_type s, int t = 1) noexcept
            : style(s), thickness(t) {}
        constexpr bool operator==(const line_style&) const noexcept = default;
    };

    /**
     * @struct font
     * @brief Font specification for text rendering
     *
     * lib_sdlpp's font system supports:
     * - TTF fonts (TrueType)
     * - Windows FON fonts
     * - BGI vector fonts
     * - Raw BIOS font dumps
     */
    struct font {
        std::filesystem::path path;     ///< Font file path
        float size_px = 16.0f;          ///< Size in pixels
        bool bold = false;
        bool italic = false;
        bool underline = false;
        bool strikethrough = false;

        [[nodiscard]] size_t hash() const noexcept;
    };

    enum class icon_style : uint8_t {
        none,
        check, cross, bullet, folder, file,
        arrow_up, arrow_down, arrow_left, arrow_right,
        menu, minimize, maximize, restore, close_x,
        cursor_insert, cursor_overwrite,
        checkbox_unchecked, checkbox_checked, checkbox_indeterminate,
        radio_unchecked, radio_checked,
        progress_filled, progress_empty,
        slider_filled, slider_empty, slider_thumb
    };

    struct background_style {
        color bg_color;
        bool use_gradient = false;
        color gradient_end;

        constexpr background_style() noexcept = default;
        constexpr explicit background_style(const color& c) noexcept
            : bg_color(c) {}
        constexpr bool operator==(const background_style&) const noexcept = default;
    };

    using size_type = size;
    using color_type = color;

    // =================================================================
    // Construction / Destruction
    // =================================================================

    /**
     * @brief Construct renderer from lib_sdlpp renderer
     * @param sdl_renderer Reference to sdlpp::renderer
     * @param default_font_path Path to default font file
     */
    explicit sdlpp_renderer(sdlpp::renderer& sdl_renderer,
                            const std::filesystem::path& default_font_path = {});

    ~sdlpp_renderer();

    sdlpp_renderer(const sdlpp_renderer&) = delete;
    sdlpp_renderer& operator=(const sdlpp_renderer&) = delete;
    sdlpp_renderer(sdlpp_renderer&&) noexcept;
    sdlpp_renderer& operator=(sdlpp_renderer&&) noexcept;

    // =================================================================
    // Drawing Methods (RenderLike Concept)
    // =================================================================

    void draw_box(const rect& r, const box_style& style,
                  const color& fg, const color& bg);

    void draw_text(const rect& r, std::string_view text,
                   const font& f, const color& fg, const color& bg);

    void draw_icon(const rect& r, icon_style style,
                   const color& fg, const color& bg);

    void draw_horizontal_line(const rect& r, const line_style& style,
                              const color& fg, const color& bg);

    void draw_vertical_line(const rect& r, const line_style& style,
                            const color& fg, const color& bg);

    void draw_background(const rect& viewport, const background_style& style);

    void draw_background(const rect& viewport, const background_style& style,
                         const std::vector<rect>& dirty_regions);

    void clear_region(const rect& r, const color& bg);

    void draw_shadow(const rect& widget_bounds, int offset_x, int offset_y);

    // =================================================================
    // Clipping Methods
    // =================================================================

    void push_clip(const rect& r);
    void pop_clip();
    [[nodiscard]] rect get_clip_rect() const;

    // =================================================================
    // Viewport and Presentation
    // =================================================================

    [[nodiscard]] rect get_viewport() const;
    void present();
    void on_resize();
    void take_screenshot(std::ostream& sink) const;

    // =================================================================
    // Static Measurement Methods
    // =================================================================

    /**
     * @brief Measure text dimensions using lib_sdlpp's font system
     * @param text UTF-8 text to measure
     * @param f Font specification
     * @return Size in pixels
     *
     * Uses sdlpp::font::font::measure() internally.
     */
    [[nodiscard]] static size measure_text(std::string_view text, const font& f);

    [[nodiscard]] static size get_icon_size(icon_style icon) noexcept;

    [[nodiscard]] static constexpr int get_border_thickness(
        const box_style& style) noexcept
    {
        switch (style.style) {
            case border_style_type::none: return 0;
            case border_style_type::flat: return 1;
            case border_style_type::raised:
            case border_style_type::sunken:
            case border_style_type::etched: return 2;
            case border_style_type::thick_raised:
            case border_style_type::thick_sunken: return 3;
        }
        return 0;
    }

private:
    struct impl;
    std::unique_ptr<impl> m_pimpl;
};

} // namespace onyxui::sdlpp
```

### Renderer Implementation

In `src/sdlpp_renderer.cc`:

```cpp
#include <onyxui/sdlpp/sdlpp_renderer.hh>

#include <sdlpp/video/renderer.hh>
#include <sdlpp/font/font.hh>
#include <sdlpp/image/image.hh>
#include <stack>
#include <unordered_map>

namespace onyxui::sdlpp {

// =================================================================
// Font Cache using lib_sdlpp's font system
// =================================================================

class font_cache {
public:
    static font_cache& instance() {
        static font_cache cache;
        return cache;
    }

    ::sdlpp::font::font* get_font(const sdlpp_renderer::font& f) {
        size_t key = f.hash();
        auto it = m_fonts.find(key);
        if (it != m_fonts.end()) {
            return it->second.get();
        }

        // Load font using lib_sdlpp
        auto result = ::sdlpp::font::font::load(f.path);
        if (!result) {
            // Try fallback font
            result = ::sdlpp::font::font::load(get_fallback_path());
            if (!result) return nullptr;
        }

        auto font_ptr = std::make_unique<::sdlpp::font::font>(std::move(*result));

        // Set size and style
        font_ptr->set_size(f.size_px);

        ::sdlpp::font::text_style style{};
        if (f.bold) style |= ::sdlpp::font::text_style::bold;
        if (f.italic) style |= ::sdlpp::font::text_style::italic;
        if (f.underline) style |= ::sdlpp::font::text_style::underline;
        if (f.strikethrough) style |= ::sdlpp::font::text_style::strikethrough;
        font_ptr->set_style(style);

        auto* ptr = font_ptr.get();
        m_fonts[key] = std::move(font_ptr);
        return ptr;
    }

private:
    std::filesystem::path get_fallback_path() const {
        #if defined(_WIN32)
        return "C:\\Windows\\Fonts\\segoeui.ttf";
        #elif defined(__APPLE__)
        return "/System/Library/Fonts/Helvetica.ttc";
        #else
        return "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf";
        #endif
    }

    std::unordered_map<size_t, std::unique_ptr<::sdlpp::font::font>> m_fonts;
};

// =================================================================
// Renderer Implementation
// =================================================================

struct sdlpp_renderer::impl {
    ::sdlpp::renderer* renderer = nullptr;
    std::stack<rect> clip_stack;
    rect viewport{0, 0, 800, 600};

    explicit impl(::sdlpp::renderer& r) : renderer(&r) {
        // Get viewport from renderer
        auto vp = r.get_viewport();
        viewport = {vp.x, vp.y, vp.w, vp.h};
    }
};

sdlpp_renderer::sdlpp_renderer(::sdlpp::renderer& sdl_renderer,
                               const std::filesystem::path& default_font_path)
    : m_pimpl(std::make_unique<impl>(sdl_renderer))
{
}

sdlpp_renderer::~sdlpp_renderer() = default;
sdlpp_renderer::sdlpp_renderer(sdlpp_renderer&&) noexcept = default;
sdlpp_renderer& sdlpp_renderer::operator=(sdlpp_renderer&&) noexcept = default;

void sdlpp_renderer::draw_box(const rect& r, const box_style& style,
                               const color& fg, const color& bg)
{
    auto& rend = *m_pimpl->renderer;

    // Fill background
    if (style.is_solid) {
        rend.set_draw_color(::sdlpp::color{bg.r, bg.g, bg.b, bg.a});
        rend.fill_rect(::sdlpp::rect_i{r.x, r.y, r.w, r.h});
    }

    // Draw border
    switch (style.style) {
        case border_style_type::none:
            break;

        case border_style_type::flat:
            rend.set_draw_color(::sdlpp::color{fg.r, fg.g, fg.b, fg.a});
            rend.draw_rect(::sdlpp::rect_i{r.x, r.y, r.w, r.h});
            break;

        case border_style_type::raised:
            // Light edge (top-left)
            rend.set_draw_color(::sdlpp::color{255, 255, 255, 255});
            rend.draw_line({r.x, r.y}, {r.x + r.w - 1, r.y});
            rend.draw_line({r.x, r.y}, {r.x, r.y + r.h - 1});
            // Dark edge (bottom-right)
            rend.set_draw_color(::sdlpp::color{64, 64, 64, 255});
            rend.draw_line({r.x + r.w - 1, r.y}, {r.x + r.w - 1, r.y + r.h - 1});
            rend.draw_line({r.x, r.y + r.h - 1}, {r.x + r.w - 1, r.y + r.h - 1});
            break;

        case border_style_type::sunken:
            // Dark edge (top-left)
            rend.set_draw_color(::sdlpp::color{64, 64, 64, 255});
            rend.draw_line({r.x, r.y}, {r.x + r.w - 1, r.y});
            rend.draw_line({r.x, r.y}, {r.x, r.y + r.h - 1});
            // Light edge (bottom-right)
            rend.set_draw_color(::sdlpp::color{255, 255, 255, 255});
            rend.draw_line({r.x + r.w - 1, r.y}, {r.x + r.w - 1, r.y + r.h - 1});
            rend.draw_line({r.x, r.y + r.h - 1}, {r.x + r.w - 1, r.y + r.h - 1});
            break;

        default:
            break;
    }
}

void sdlpp_renderer::draw_text(const rect& r, std::string_view text,
                                const font& f, const color& fg, const color& bg)
{
    if (text.empty()) return;

    auto* fnt = font_cache::instance().get_font(f);
    if (!fnt) return;

    // Render text to texture using lib_sdlpp's font system
    auto result = fnt->render_texture(
        *m_pimpl->renderer, text,
        ::sdlpp::color{fg.r, fg.g, fg.b, fg.a});

    if (!result) return;

    auto& texture = *result;
    auto tex_size = texture.get_size();

    // Copy texture to renderer
    ::sdlpp::rect_i dst{r.x, r.y,
                        std::min(r.w, tex_size.w),
                        std::min(r.h, tex_size.h)};
    m_pimpl->renderer->copy(texture, std::nullopt, dst);
}

void sdlpp_renderer::draw_icon(const rect& r, icon_style style,
                                const color& fg, const color& bg)
{
    auto& rend = *m_pimpl->renderer;

    // Draw icons procedurally using lib_sdlpp's DDA renderer
    switch (style) {
        case icon_style::check: {
            rend.set_draw_color(::sdlpp::color{fg.r, fg.g, fg.b, fg.a});
            int cx = r.x + r.w / 4;
            int cy = r.y + r.h / 2;
            // Use antialiased line drawing
            rend.draw_line_aa({cx, cy}, {cx + r.w / 4, cy + r.h / 4});
            rend.draw_line_aa({cx + r.w / 4, cy + r.h / 4},
                              {cx + r.w / 2, cy - r.h / 4});
            break;
        }

        case icon_style::cross:
        case icon_style::close_x: {
            rend.set_draw_color(::sdlpp::color{fg.r, fg.g, fg.b, fg.a});
            int m = 3;
            rend.draw_line({r.x + m, r.y + m}, {r.x + r.w - m, r.y + r.h - m});
            rend.draw_line({r.x + r.w - m, r.y + m}, {r.x + m, r.y + r.h - m});
            break;
        }

        case icon_style::arrow_down: {
            rend.set_draw_color(::sdlpp::color{fg.r, fg.g, fg.b, fg.a});
            // Draw filled triangle using polygon
            std::vector<::sdlpp::point_i> points = {
                {r.x + r.w / 2, r.y + r.h - 2},
                {r.x + 2, r.y + 2},
                {r.x + r.w - 2, r.y + 2}
            };
            rend.fill_polygon(points);
            break;
        }

        case icon_style::arrow_up: {
            rend.set_draw_color(::sdlpp::color{fg.r, fg.g, fg.b, fg.a});
            std::vector<::sdlpp::point_i> points = {
                {r.x + r.w / 2, r.y + 2},
                {r.x + 2, r.y + r.h - 2},
                {r.x + r.w - 2, r.y + r.h - 2}
            };
            rend.fill_polygon(points);
            break;
        }

        default:
            break;
    }
}

void sdlpp_renderer::draw_horizontal_line(const rect& r, const line_style& style,
                                           const color& fg, const color& bg)
{
    auto& rend = *m_pimpl->renderer;
    rend.set_draw_color(::sdlpp::color{fg.r, fg.g, fg.b, fg.a});

    int y = r.y + r.h / 2;
    if (style.thickness == 1) {
        rend.draw_line({r.x, y}, {r.x + r.w, y});
    } else {
        rend.draw_line_thick({r.x, y}, {r.x + r.w, y}, style.thickness);
    }
}

void sdlpp_renderer::draw_vertical_line(const rect& r, const line_style& style,
                                         const color& fg, const color& bg)
{
    auto& rend = *m_pimpl->renderer;
    rend.set_draw_color(::sdlpp::color{fg.r, fg.g, fg.b, fg.a});

    int x = r.x + r.w / 2;
    if (style.thickness == 1) {
        rend.draw_line({x, r.y}, {x, r.y + r.h});
    } else {
        rend.draw_line_thick({x, r.y}, {x, r.y + r.h}, style.thickness);
    }
}

void sdlpp_renderer::draw_background(const rect& viewport,
                                      const background_style& style)
{
    auto& rend = *m_pimpl->renderer;
    rend.set_draw_color(::sdlpp::color{
        style.bg_color.r, style.bg_color.g,
        style.bg_color.b, style.bg_color.a});
    rend.fill_rect(::sdlpp::rect_i{viewport.x, viewport.y, viewport.w, viewport.h});
}

void sdlpp_renderer::draw_background(const rect& viewport,
                                      const background_style& style,
                                      const std::vector<rect>& dirty_regions)
{
    if (dirty_regions.empty()) {
        draw_background(viewport, style);
        return;
    }

    auto& rend = *m_pimpl->renderer;
    rend.set_draw_color(::sdlpp::color{
        style.bg_color.r, style.bg_color.g,
        style.bg_color.b, style.bg_color.a});

    for (const auto& r : dirty_regions) {
        rend.fill_rect(::sdlpp::rect_i{r.x, r.y, r.w, r.h});
    }
}

void sdlpp_renderer::clear_region(const rect& r, const color& bg)
{
    auto& rend = *m_pimpl->renderer;
    rend.set_draw_color(::sdlpp::color{bg.r, bg.g, bg.b, bg.a});
    rend.fill_rect(::sdlpp::rect_i{r.x, r.y, r.w, r.h});
}

void sdlpp_renderer::draw_shadow(const rect& wb, int offset_x, int offset_y)
{
    auto& rend = *m_pimpl->renderer;
    rend.set_draw_blend_mode(::sdlpp::blend_mode::blend);
    rend.set_draw_color(::sdlpp::color{0, 0, 0, 64});

    // Right shadow
    rend.fill_rect(::sdlpp::rect_i{
        wb.x + wb.w, wb.y + offset_y, offset_x, wb.h});
    // Bottom shadow
    rend.fill_rect(::sdlpp::rect_i{
        wb.x + offset_x, wb.y + wb.h, wb.w, offset_y});
}

void sdlpp_renderer::push_clip(const rect& r)
{
    m_pimpl->clip_stack.push(r);
    m_pimpl->renderer->set_clip_rect(::sdlpp::rect_i{r.x, r.y, r.w, r.h});
}

void sdlpp_renderer::pop_clip()
{
    if (!m_pimpl->clip_stack.empty()) {
        m_pimpl->clip_stack.pop();
    }

    if (m_pimpl->clip_stack.empty()) {
        m_pimpl->renderer->set_clip_rect(std::nullopt);
    } else {
        const auto& r = m_pimpl->clip_stack.top();
        m_pimpl->renderer->set_clip_rect(::sdlpp::rect_i{r.x, r.y, r.w, r.h});
    }
}

rect sdlpp_renderer::get_clip_rect() const
{
    if (m_pimpl->clip_stack.empty()) {
        return m_pimpl->viewport;
    }
    return m_pimpl->clip_stack.top();
}

rect sdlpp_renderer::get_viewport() const
{
    return m_pimpl->viewport;
}

void sdlpp_renderer::present()
{
    m_pimpl->renderer->present();
}

void sdlpp_renderer::on_resize()
{
    auto vp = m_pimpl->renderer->get_viewport();
    m_pimpl->viewport = {vp.x, vp.y, vp.w, vp.h};
}

void sdlpp_renderer::take_screenshot(std::ostream& sink) const
{
    sink << "SDL++ Renderer Screenshot\n";
    sink << "Viewport: " << m_pimpl->viewport.w << "x"
         << m_pimpl->viewport.h << "\n";
}

// =================================================================
// Static Measurement Methods
// =================================================================

size sdlpp_renderer::measure_text(std::string_view text, const font& f)
{
    if (text.empty()) {
        return {0, static_cast<int>(f.size_px)};
    }

    auto* fnt = font_cache::instance().get_font(f);
    if (!fnt) {
        // Fallback estimate
        return {static_cast<int>(text.length() * f.size_px * 0.6f),
                static_cast<int>(f.size_px)};
    }

    // Use lib_sdlpp's font measurement
    auto metrics = fnt->measure(text);
    return {static_cast<int>(metrics.width), static_cast<int>(metrics.height)};
}

size sdlpp_renderer::get_icon_size(icon_style icon) noexcept
{
    switch (icon) {
        case icon_style::checkbox_unchecked:
        case icon_style::checkbox_checked:
        case icon_style::checkbox_indeterminate:
        case icon_style::radio_unchecked:
        case icon_style::radio_checked:
            return {16, 16};

        case icon_style::minimize:
        case icon_style::maximize:
        case icon_style::restore:
        case icon_style::close_x:
            return {12, 12};

        default:
            return {16, 16};
    }
}

size_t sdlpp_renderer::font::hash() const noexcept
{
    size_t h = std::hash<std::string>{}(path.string());
    h ^= std::hash<float>{}(size_px) << 1;
    h ^= std::hash<bool>{}(bold) << 2;
    h ^= std::hash<bool>{}(italic) << 3;
    h ^= std::hash<bool>{}(underline) << 4;
    return h;
}

} // namespace onyxui::sdlpp
```

---

## Theme Registration

### `src/sdlpp_themes.hh`

```cpp
/**
 * @file sdlpp_themes.hh
 * @brief Built-in theme definitions for SDL++ backend
 */

#pragma once

#include <onyxui/theming/theme.hh>
#include <onyxui/sdlpp/sdlpp_backend.hh>

namespace onyxui::sdlpp {

/**
 * @brief Create Windows 3.11 style theme (default)
 *
 * Classic Windows 3.11 color scheme with 3D beveled controls.
 */
inline theme<sdlpp_backend> create_windows311_theme()
{
    theme<sdlpp_backend> t;
    t.name = "Windows 3.11";

    // Windows 3.11 system colors
    constexpr color button_face{192, 192, 192};      // btnface
    constexpr color button_shadow{128, 128, 128};    // btnshadow
    constexpr color button_highlight{255, 255, 255}; // btnhighlight
    constexpr color window_bg{192, 192, 192};        // window background
    constexpr color window_text{0, 0, 0};            // windowtext
    constexpr color highlight{0, 0, 128};            // highlight (navy)
    constexpr color highlight_text{255, 255, 255};   // highlighttext
    constexpr color window_frame{0, 0, 0};           // windowframe

    // Window
    t.window_bg = window_bg;
    t.window_fg = window_text;

    // Button - 3D raised style
    t.button.background_color = button_face;
    t.button.foreground_color = window_text;
    t.button.box_style = sdlpp_renderer::box_style{
        sdlpp_renderer::border_style_type::raised, true};
    t.button.hover_background = button_face;
    t.button.pressed_background = button_face;

    // Menu
    t.menu.background_color = window_bg;
    t.menu.foreground_color = window_text;
    t.menu.highlight_background = highlight;
    t.menu.highlight_foreground = highlight_text;

    // Text input - 3D sunken style
    t.text_input.background_color = color{255, 255, 255};
    t.text_input.foreground_color = window_text;
    t.text_input.box_style = sdlpp_renderer::box_style{
        sdlpp_renderer::border_style_type::sunken, true};

    // Scrollbar
    t.scrollbar.track_color = button_face;
    t.scrollbar.thumb_color = button_face;
    t.scrollbar.thumb_box_style = sdlpp_renderer::box_style{
        sdlpp_renderer::border_style_type::raised, true};

    return t;
}

} // namespace onyxui::sdlpp
```

---

## Complete Implementation Checklist

### Required Components

- [ ] **Geometry types** (`geometry.hh`)
  - [x] `rect` satisfying RectLike
  - [x] `size` satisfying SizeLike
  - [x] `point` satisfying PointLike

- [ ] **Color type** (`colors.hh`)
  - [x] RGBA color struct

- [ ] **Event types** (`sdlpp_events.hh`)
  - [x] Event type aliases from lib_sdlpp

- [ ] **Backend struct** (`sdlpp_backend.hh`)
  - [x] Type aliases for all required types
  - [x] `create_event()` static method
  - [x] `register_themes()` static method
  - [x] `name()` constexpr method
  - [x] `has_mouse_tracking` constexpr bool

- [ ] **Renderer** (`sdlpp_renderer.hh` / `.cc`)
  - [x] All style types
  - [x] All drawing methods using lib_sdlpp
  - [x] Static `measure_text()` using `sdlpp::font::font::measure()`
  - [x] Static `get_icon_size()`

- [ ] **CMake** (`CMakeLists.txt`)
  - [x] Fetch lib_sdlpp via neutrino-cmake
  - [x] Use `add_backend_library()` helper
  - [x] **Only `neutrino::sdlpp` dependency needed** (SDL3, fonts, images all included)

---

## Testing

```cpp
// unittest/backends/test_sdlpp_backend.cc

#include <doctest/doctest.h>
#include <onyxui/sdlpp/sdlpp_backend.hh>

TEST_CASE("sdlpp_backend satisfies UIBackend concept") {
    static_assert(onyxui::UIBackend<onyxui::sdlpp::sdlpp_backend>);
}

TEST_CASE("Static text measurement works without renderer") {
    using renderer = onyxui::sdlpp::sdlpp_renderer;

    renderer::font f{};
    f.path = "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf";
    f.size_px = 16.0f;

    auto sz = renderer::measure_text("Hello", f);

    CHECK(sz.w > 0);
    CHECK(sz.h > 0);
}
```

---

## Example Application

```cpp
#include <onyxui/sdlpp/sdlpp_backend.hh>
#include <onyxui/services/ui_context.hh>
#include <onyxui/widgets/containers/vbox.hh>
#include <onyxui/widgets/button.hh>
#include <onyxui/widgets/label.hh>

#include <sdlpp/core/core.hh>
#include <sdlpp/video/window.hh>
#include <sdlpp/video/renderer.hh>
#include <sdlpp/events/events.hh>
#include <iostream>

using namespace onyxui;
using namespace onyxui::sdlpp;

int main()
{
    // Initialize SDL via lib_sdlpp
    auto sdl = sdlpp::init(sdlpp::init_flags::video);
    if (!sdl) {
        std::cerr << "Failed to initialize SDL: " << sdl.error() << "\n";
        return 1;
    }

    // Create window
    auto window = sdlpp::window::create("OnyxUI SDL++ Demo", 800, 600,
        sdlpp::window_flags::shown | sdlpp::window_flags::resizable);
    if (!window) {
        std::cerr << "Failed to create window: " << window.error() << "\n";
        return 1;
    }

    // Create renderer
    auto sdl_renderer = window->create_renderer();
    if (!sdl_renderer) {
        std::cerr << "Failed to create renderer\n";
        return 1;
    }

    // Create OnyxUI context
    scoped_ui_context<sdlpp_backend> ctx;

    // Create OnyxUI renderer wrapper
    sdlpp_renderer renderer(*sdl_renderer);

    // Create UI hierarchy
    auto root = std::make_unique<vbox<sdlpp_backend>>(10);
    root->apply_theme("Windows 3.11", ctx.themes());

    root->emplace_child<label<sdlpp_backend>>("Hello, SDL++ OnyxUI!");

    auto* btn = root->emplace_child<button<sdlpp_backend>>("Click Me");
    btn->clicked.connect([]() {
        std::cout << "Button clicked!\n";
    });

    auto* quit_btn = root->emplace_child<button<sdlpp_backend>>("Quit");
    bool running = true;
    quit_btn->clicked.connect([&running]() { running = false; });

    // Main loop
    while (running) {
        while (auto event = sdlpp::poll_event()) {
            if (event->is<sdlpp::quit_event>()) {
                running = false;
                break;
            }

            if (auto ui_evt = sdlpp_backend::create_event(*event)) {
                // Route to UI
            }
        }

        auto viewport = renderer.get_viewport();
        root->measure(viewport.w, viewport.h);
        root->arrange(viewport);

        renderer.draw_background(viewport,
            sdlpp_renderer::background_style{color{192, 192, 192}});
        root->render(&renderer, ctx.themes().current_theme());
        renderer.present();
    }

    return 0;
}
```

---

## Windows 3.11 Assets

All Windows 3.11 assets are located in the `win311/` folder at the project root.

### Available Assets

```
win311/
├── SSERIFE.FON      # MS Sans Serif font (default UI font)
├── MORICONS.DLL     # Icon library (~100 icons)
├── SHELL.DLL        # Shell icons (folder, file, drive)
└── PROGMAN.EXE      # Program Manager icons
```

### Font File

| File | Description | Usage |
|------|-------------|-------|
| `SSERIFE.FON` | MS Sans Serif (proportional) | All UI elements |

lib_sdlpp's font system supports Windows `.FON` bitmap fonts natively.

**Usage:**
```cpp
sdlpp_renderer::font ui_font{};
ui_font.path = "win311/SSERIFE.FON";  // MS Sans Serif
ui_font.size_px = 16.0f;
```

### Icon Files

| File | Description | Icons |
|------|-------------|-------|
| `MORICONS.DLL` | More Icons library | ~100 general-purpose icons |
| `SHELL.DLL` | Shell icons | Folder, file, drive icons |
| `PROGMAN.EXE` | Program Manager | Program group icons |

lib_sdlpp's `image::load_atlas()` can extract icons from `.DLL` and `.EXE` files.

**Usage:**
```cpp
#include <sdlpp/image/image.hh>

// Load icon atlas from MORICONS.DLL
auto atlas = sdlpp::image::load_atlas("win311/MORICONS.DLL");
if (atlas) {
    // Find best size for 32x32 display
    auto icon = atlas->find_best_size({32, 32});

    // Or extract specific icon by index
    auto folder_icon = atlas->extract(0);
}
```

### Backend Initialization

```cpp
// Path to Windows 3.11 assets
const std::filesystem::path WIN311_ASSETS = "win311";

// Load default UI font
auto default_font = sdlpp::font::font::load(WIN311_ASSETS / "SSERIFE.FON");
if (default_font) {
    default_font->set_size(16.0f);
}

// Load shell icons
auto shell_icons = sdlpp::image::load_atlas(WIN311_ASSETS / "SHELL.DLL");
```

---

## lib_sdlpp Key API Reference

### Font System (`sdlpp::font::font`)

```cpp
#include <sdlpp/font/font.hh>

// Load font from file (auto-detects TTF, FON, BGI, raw)
auto font = sdlpp::font::font::load("path/to/font.ttf");

// Set rendering size
font->set_size(16.0f);  // pixels

// Set style
font->set_style(sdlpp::font::text_style::bold |
                sdlpp::font::text_style::italic);

// Measure text
auto metrics = font->measure("Hello World");
// metrics.width, metrics.height, metrics.ascent, metrics.descent

// Render to texture
auto texture = font->render_texture(renderer, "Hello", fg_color);

// Render to surface
auto surface = font->render_text("Hello", fg_color, bg_color);
```

### Image System (`sdlpp::image`)

```cpp
#include <sdlpp/image/image.hh>

// Load image to surface
auto surface = sdlpp::image::load("path/to/image.png");

// Load image directly to texture
auto texture = sdlpp::image::load_texture(renderer, "icon.png");

// Load from memory
auto surface = sdlpp::image::load(std::span<const uint8_t>{data});

// Load icon atlas (ICO, DCX)
auto atlas = sdlpp::image::load_atlas("icons.ico");
auto best = atlas->find_best_size({32, 32});
```

### Renderer DDA Algorithms

```cpp
// Antialiased drawing
renderer.draw_line_aa(start, end);
renderer.draw_circle(center, radius);
renderer.fill_circle(center, radius);
renderer.draw_ellipse(rect);
renderer.fill_polygon(points);
renderer.draw_bezier_cubic(p0, p1, p2, p3);
```

---

## Additional Resources

- [lib_sdlpp GitHub](https://github.com/devbrain/lib_sdlpp) - SDL3 C++20 wrapper
- [OnyxUI Architecture Guide](CLAUDE/ARCHITECTURE.md) - Framework architecture
- [OnyxUI Theming Guide](CLAUDE/THEMING.md) - Theme system documentation

---

*Last updated: December 2025*
