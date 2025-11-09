//
// SDL2 Backend Renderer
//

#pragma once

#include <cstdint>
#include <string_view>
#include <memory>

#include <onyxui/sdl2/geometry.hh>
#include <onyxui/sdl2/colors.hh>

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

namespace onyxui::sdl2 {
    /**
    * @class sdl2_renderer
    * @brief SDL2 renderer for pixel-based UI rendering
    *
    * This renderer uses SDL2 for hardware-accelerated rendering
    * with TTF font support. It provides all required methods
    * for the RenderLike concept.
    */
    class sdl2_renderer {
        public:
            // ===================================================================
            // Required Types for RenderLike Concept
            // ===================================================================

            /**
             * @enum box_style
             * @brief Defines different box drawing styles
             */
            enum class box_style : uint8_t {
                none,           // No border
                flat,           // Flat border (1px solid line)
                raised,         // Windows 3.x raised border (beveled outward)
                sunken,         // Windows 3.x sunken border (beveled inward)
                thick_raised,   // Thick raised border (2px bevel)
                thick_sunken    // Thick sunken border (2px bevel)
            };

            /**
             * @struct font
             * @brief Font attributes for text rendering
             *
             * Uses SDL_ttf for TrueType font rendering.
             */
            struct font {
                std::string family = "MS Sans Serif";  // Windows 3.x default
                int size = 8;                           // Point size
                bool bold = false;
                bool italic = false;
                bool underline = false;
            };

            /**
             * @enum icon_style
             * @brief Predefined icon/glyph styles
             */
            enum class icon_style : uint8_t {
                none,

                // General purpose icons
                check,         // ✓ checkmark
                cross,         // ✗ cross
                bullet,        // •
                folder,        // folder icon
                file,          // file icon

                // Navigation arrows
                arrow_up,      // ↑
                arrow_down,    // ↓
                arrow_left,    // ←
                arrow_right,   // →

                // Window management icons
                menu,          // ≡ (hamburger menu)
                minimize,      // ▁ (minimize to taskbar)
                maximize,      // □ (maximize window)
                restore,       // ▢ (restore from maximized)
                close_x        // × (close window - renamed from 'close' for consistency)
            };

            using size_type = size; // Required by RenderLike concept

        public:
            /**
             * @brief Construct SDL2 renderer
             * @param window SDL window for rendering
             * @param width Initial viewport width
             * @param height Initial viewport height
             */
            sdl2_renderer(SDL_Window* window, int width, int height);
            ~sdl2_renderer();

            sdl2_renderer(const sdl2_renderer&) = delete;
            sdl2_renderer& operator=(const sdl2_renderer&) = delete;

            // ===================================================================
            // Required Drawing Methods (RenderLike Concept)
            // ===================================================================

            /**
             * @brief Draw a box with the specified style
             * @param r Rectangle defining the box bounds
             * @param style Box drawing style
             */
            void draw_box(const rect& r, box_style style);

            /**
             * @brief Draw text within a rectangle
             * @param r Rectangle defining text bounds
             * @param text Text to draw (UTF-8 encoded)
             * @param f Font attributes
             */
            void draw_text(const rect& r, std::string_view text, const font& f);

            /**
             * @brief Draw an icon/glyph
             * @param r Rectangle defining icon bounds
             * @param style Icon to draw
             */
            void draw_icon(const rect& r, icon_style style);

            /**
             * @brief Clear a rectangular region
             * @param r Rectangle to clear
             */
            void clear_region(const rect& r);

            // ===================================================================
            // Required Clipping Methods (RenderLike Concept)
            // ===================================================================

            /**
             * @brief Push a clipping rectangle onto the stack
             * @param r Clipping rectangle
             */
            void push_clip(const rect& r);

            /**
             * @brief Pop the current clipping rectangle from the stack
             */
            void pop_clip();

            /**
             * @brief Get the current clipping rectangle
             * @return Current clip rect
             */
            [[nodiscard]] rect get_clip_rect() const;

            // ===================================================================
            // Presentation Method (Required by RenderLike Concept)
            // ===================================================================

            /**
             * @brief Present the rendered frame to the display
             */
            void present();

            /**
             * @brief Handle window resize event
             */
            void on_resize(int new_width, int new_height);

            /**
             * @brief Get the current viewport
             * @return Rectangle covering the entire window
             */
            [[nodiscard]] rect get_viewport() const;

            // ===================================================================
            // Static Text Measurement (Required by UIBackend Concept)
            // ===================================================================

            /**
             * @brief Measure text dimensions (static - no instance needed)
             * @param text Text to measure (UTF-8 encoded)
             * @param f Font attributes
             * @return Size with width/height in pixels
             */
            static size measure_text(std::string_view text, const font& f);

            /**
             * @brief Get icon size (static - no instance needed)
             * @param icon The icon style
             * @return Size with width/height in pixels (typically 16x16)
             */
            [[nodiscard]] static constexpr size get_icon_size([[maybe_unused]] const icon_style& icon) noexcept {
                return size{16, 16}; // Standard Windows 3.x icon size
            }

            /**
             * @brief Get border thickness for a box style (static - no instance needed)
             * @param style The box style to query
             * @return Border thickness in pixels
             */
            [[nodiscard]] static constexpr int get_border_thickness(box_style style) noexcept {
                switch (style) {
                    case box_style::none:
                        return 0;
                    case box_style::flat:
                        return 1;
                    case box_style::raised:
                    case box_style::sunken:
                        return 2; // Windows 3.x bevel is 2px
                    case box_style::thick_raised:
                    case box_style::thick_sunken:
                        return 4; // Thick bevel is 4px total
                    default:
                        return 0;
                }
            }

            // ===================================================================
            // Color Management
            // ===================================================================

            /**
             * @brief Set current foreground color
             */
            void set_foreground(const color& c);

            /**
             * @brief Set current background color
             */
            void set_background(const color& c);

            /**
             * @brief Get current foreground color
             */
            [[nodiscard]] color get_foreground() const;

            /**
             * @brief Get current background color
             */
            [[nodiscard]] color get_background() const;

        private:
            struct impl;
            std::unique_ptr<impl> m_pimpl;
    };
}
