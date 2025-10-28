/**
 * @file backend.hh
 * @brief Backend traits and concept for unified template parameter
 * @author igor
 * @date 10/10/2025
 *
 * This file defines the Backend concept and traits structure that unifies
 * all backend-specific types into a single template parameter.
 *
 * ## Architecture Overview
 *
 * The Backend pattern provides a clean separation between the UI layout logic
 * and platform-specific types. Instead of templating UI components on multiple
 * parameters (TRect, TSize, TEvent, etc.), components now take a single Backend
 * parameter that provides all necessary types through a traits structure.
 *
 * ## Benefits
 *
 * - **Simplified API**: Single template parameter instead of multiple
 * - **Type Consistency**: All types come from the same backend
 * - **Easy Extension**: Add new backends by implementing the required types
 * - **Better Error Messages**: Concept constraints provide clear requirements
 *
 * ## Creating a Custom Backend
 *
 * To create a custom backend, define a struct with the required type aliases:
 *
 * @code
 * struct my_backend {
 *     // Required geometric types
 *     using rect_type = MyRect;    // Must satisfy RectLike concept
 *     using size_type = MySize;    // Must satisfy SizeLike concept
 *     using point_type = MyPoint;  // Must satisfy PointLike concept
 *
 *     // Required event types
 *     using event_type = MyEvent;
 *     using keyboard_event_type = MyKeyboardEvent;
 *     using mouse_button_event_type = MyMouseButtonEvent;
 *     using mouse_motion_event_type = MyMouseMotionEvent;
 *
 *     // Optional rendering types
 *     using color_type = MyColor;
 *     using renderer_type = MyRenderer*;
 *     using texture_type = MyTexture*;
 *     using font_type = MyFont*;
 *
 *     // Backend name for debugging
 *     static constexpr const char* name() { return "MyBackend"; }
 * };
 * @endcode
 *
 * Then specialize event_traits for your event types to provide event handling:
 *
 * @code
 * template<>
 * struct event_traits<MyKeyboardEvent> {
 *     static bool is_key_press(const MyKeyboardEvent& e) { return e.pressed; }
 *     static int key_code(const MyKeyboardEvent& e) { return e.keycode; }
 *     // ... other required methods
 * };
 * @endcode
 */

#pragma once

#include <string_view>
#include <type_traits>
#include <variant>
#include <optional>

#include <onyxui/concepts/point_like.hh>
#include <onyxui/concepts/size_like.hh>
#include <onyxui/concepts/rect_like.hh>
#include <onyxui/concepts/render_like.hh>
#include <onyxui/events/ui_event.hh>

namespace onyxui {
    /**
     * @concept UIBackend
     * @brief Concept that defines requirements for a UI backend
     *
     * A backend must provide all the types needed for the UI system:
     * - Geometric types (rect, size, point)
     * - Event types
     * - Renderer type with text measurement capability (both instance and static)
     *
     * ## Renderer Static measure_text() Rationale
     *
     * Renderers must provide a **static** measure_text() method because widgets
     * need to calculate their size during layout (in get_content_size()) but
     * don't have access to a renderer instance at that point.
     *
     * This approach is correct because:
     * - **Proper abstraction**: Renderer knows how to measure text (it draws it!)
     * - **No renderer needed**: Static method works during layout calculations
     * - **Accurate**: Not using text.length() (wrong for UTF-8, proportional fonts)
     * - **Simple**: No complex caching or invalidation logic needed
     *
     * @example Label auto-sizing
     * @code
     * size_type get_content_size() const override {
     *     typename Backend::renderer_type::font default_font{};
     *     return Backend::renderer_type::measure_text(m_text, default_font);
     * }
     * @endcode
     */
    template<typename T>
    concept UIBackend = requires(
        std::string_view text,
        const typename T::renderer_type::font& font,
        const typename T::event_type& native_event
    )
    {
        // Required geometric types
        typename T::rect_type;
        typename T::size_type;
        typename T::point_type;

        // Required event types
        typename T::event_type;
        typename T::keyboard_event_type;
        typename T::mouse_button_event_type;
        typename T::mouse_motion_event_type;

        // Renderer
        typename T::renderer_type;

        // Static text measurement on renderer (for layout without renderer instance)
        { T::renderer_type::measure_text(text, font) } -> std::same_as<typename T::size_type>;

        // Event conversion to unified ui_event (NEW Phase 5 requirement)
        { T::create_event(native_event) } -> std::same_as<std::optional<ui_event>>;

        requires RenderLike<typename T::renderer_type, typename T::rect_type>;
        // The geometric types must satisfy our concepts
        requires RectLike <typename T::rect_type>;
        requires SizeLike <typename T::size_type>;
        requires PointLike <typename T::point_type>;
    };

    /**
     * @struct backend_traits
     * @brief Base template for backend traits
     *
     * Specialize this for each backend to provide type mappings
     */
    template<typename Backend>
    struct backend_traits {
        // No default implementation - must specialize
    };

    // ======================================================================
    // Backend Utilities
    // ======================================================================

    /**
     * @brief Helper to extract types from a backend
     */
    template<UIBackend Backend>
    struct backend_types {
        using rect = typename Backend::rect_type;
        using size = typename Backend::size_type;
        using point = typename Backend::point_type;
        using event = typename Backend::event_type;
        using keyboard_event = typename Backend::keyboard_event_type;
        using mouse_button_event = typename Backend::mouse_button_event_type;
        using mouse_motion_event = typename Backend::mouse_motion_event_type;
        using renderer = typename Backend::renderer_type;
    };

    /**
     * @brief Convenience alias for backend types
     */
    template<UIBackend Backend>
    using backend_rect_t = typename Backend::rect_type;

    template<UIBackend Backend>
    using backend_size_t = typename Backend::size_type;

    template<UIBackend Backend>
    using backend_point_t = typename Backend::point_type;

    template<UIBackend Backend>
    using backend_event_t = typename Backend::event_type;
}
