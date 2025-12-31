/**
 * @file demo.cc
 * @brief SDL++ backend demo application
 *
 * Demonstrates font rendering using lib_sdlpp with the pixel format fix.
 */

#include <sdlpp/core/core.hh>
#include <sdlpp/video/window.hh>
#include <sdlpp/video/renderer.hh>
#include <sdlpp/events/events.hh>

#include <onyxui/sdlpp/sdlpp_renderer.hh>

#include <filesystem>
#include <iostream>

int main()
{
    try {
        // Initialize SDL
        ::sdlpp::init sdl(::sdlpp::init_flags::video);

        // Create window
        auto window_result = ::sdlpp::window::create(
            "OnyxUI - Windows 3.11 Theme Demo",
            800, 600,
            ::sdlpp::window_flags::resizable);

        if (!window_result) {
            std::cerr << "Failed to create window: " << window_result.error() << "\n";
            return 1;
        }

        auto& window = *window_result;

        // Create SDL renderer
        auto renderer_result = window.create_renderer();
        if (!renderer_result) {
            std::cerr << "Failed to create renderer: " << renderer_result.error() << "\n";
            return 1;
        }

        auto& sdl_renderer = *renderer_result;

        // Create OnyxUI renderer wrapping SDL renderer
        onyxui::sdlpp::sdlpp_renderer renderer(sdl_renderer);

        // Use Windows 3.11 MS Sans Serif font
        std::filesystem::path font_path = "/home/igor/proj/neutrino/onyx_ui/win311/SSERIFE.FON";

        // Define fonts for rendering
        onyxui::sdlpp::sdlpp_renderer::font title_font;
        title_font.path = font_path;
        title_font.size_px = 16.0f;  // Bitmap fonts have fixed sizes
        title_font.bold = true;

        onyxui::sdlpp::sdlpp_renderer::font normal_font;
        normal_font.path = font_path;
        normal_font.size_px = 13.0f;

        onyxui::sdlpp::sdlpp_renderer::font italic_font;
        italic_font.path = font_path;
        italic_font.size_px = 13.0f;

        // Define colors
        onyxui::sdlpp::color black{0, 0, 0, 255};
        onyxui::sdlpp::color white{255, 255, 255, 255};
        onyxui::sdlpp::color gray{192, 192, 192, 255};
        onyxui::sdlpp::color dark_gray{128, 128, 128, 255};
        onyxui::sdlpp::color blue{0, 0, 128, 255};

        std::cerr << "Starting main loop...\n";

        // Main event loop
        bool running = true;
        while (running) {
            // Process events
            while (auto event = ::sdlpp::event_queue::poll()) {
                if (event->is<::sdlpp::quit_event>()) {
                    running = false;
                    break;
                }

                if (auto* key_event = event->as<::sdlpp::keyboard_event>()) {
                    if (key_event->is_pressed()) {
                        if (key_event->scan == ::sdlpp::scancode::escape) {
                            running = false;
                            break;
                        }
                    }
                }

                if (auto* window_event = event->as<::sdlpp::window_event>()) {
                    if (window_event->is_resized()) {
                        renderer.on_resize();
                        auto new_viewport = renderer.get_viewport();
                        std::cerr << "Resized to: " << new_viewport.w << "x" << new_viewport.h << "\n";
                    }
                }
            }

            // Clear background to Windows 3.11 gray
            sdl_renderer.set_draw_color(::sdlpp::color{192, 192, 192, 255});
            sdl_renderer.clear();

            // Draw title bar simulation
            onyxui::sdlpp::rect title_bar{10, 10, 780, 30};
            onyxui::sdlpp::sdlpp_renderer::box_style raised_style{
                onyxui::sdlpp::sdlpp_renderer::border_style_type::raised, true};

            renderer.draw_box(title_bar, raised_style, white, blue);
            renderer.draw_text({15, 12, 770, 26}, "Windows 3.11 Style Demo", title_font, white, blue);

            // Draw a panel with raised border
            onyxui::sdlpp::rect panel{10, 50, 780, 540};
            renderer.draw_box(panel, raised_style, black, gray);

            // Draw text labels
            renderer.draw_text({30, 80, 740, 40}, "This is the OnyxUI SDL++ Backend Demo", title_font, black, gray);

            renderer.draw_text({30, 140, 740, 20}, "Font rendering is now working correctly!", normal_font, black, gray);
            renderer.draw_text({30, 170, 740, 20}, "The pixel format was fixed from RGBA8888 to ABGR8888.", normal_font, black, gray);
            renderer.draw_text({30, 200, 740, 20}, "This ensures correct byte order on little-endian systems.", italic_font, dark_gray, gray);

            // Draw a group box simulation
            onyxui::sdlpp::rect group_box{30, 250, 400, 150};
            onyxui::sdlpp::sdlpp_renderer::box_style sunken_style{
                onyxui::sdlpp::sdlpp_renderer::border_style_type::sunken, true};
            renderer.draw_box(group_box, sunken_style, dark_gray, white);

            renderer.draw_text({50, 270, 360, 20}, "Sample Group Box", normal_font, black, white);
            renderer.draw_text({50, 300, 360, 20}, "- Item 1: Text rendering", normal_font, black, white);
            renderer.draw_text({50, 320, 360, 20}, "- Item 2: Box styles", normal_font, black, white);
            renderer.draw_text({50, 340, 360, 20}, "- Item 3: Color blending", normal_font, black, white);

            // Draw buttons
            onyxui::sdlpp::rect button1{30, 430, 120, 30};
            renderer.draw_box(button1, raised_style, black, gray);
            renderer.draw_text({45, 435, 90, 20}, "OK", normal_font, black, gray);

            onyxui::sdlpp::rect button2{170, 430, 120, 30};
            renderer.draw_box(button2, raised_style, black, gray);
            renderer.draw_text({185, 435, 90, 20}, "Cancel", normal_font, black, gray);

            // Draw icons showcase
            renderer.draw_text({460, 260, 300, 20}, "Icons:", normal_font, black, white);

            // Row 1: Arrows
            renderer.draw_icon({460, 285, 16, 16}, onyxui::sdlpp::sdlpp_renderer::icon_style::arrow_up, black, white);
            renderer.draw_icon({480, 285, 16, 16}, onyxui::sdlpp::sdlpp_renderer::icon_style::arrow_down, black, white);
            renderer.draw_icon({500, 285, 16, 16}, onyxui::sdlpp::sdlpp_renderer::icon_style::arrow_left, black, white);
            renderer.draw_icon({520, 285, 16, 16}, onyxui::sdlpp::sdlpp_renderer::icon_style::arrow_right, black, white);

            // Row 2: Checkboxes
            renderer.draw_icon({460, 310, 13, 13}, onyxui::sdlpp::sdlpp_renderer::icon_style::checkbox_unchecked, black, white);
            renderer.draw_icon({480, 310, 13, 13}, onyxui::sdlpp::sdlpp_renderer::icon_style::checkbox_checked, black, white);
            renderer.draw_icon({500, 310, 13, 13}, onyxui::sdlpp::sdlpp_renderer::icon_style::checkbox_indeterminate, black, white);

            // Row 3: Radio buttons
            renderer.draw_icon({460, 335, 13, 13}, onyxui::sdlpp::sdlpp_renderer::icon_style::radio_unchecked, black, white);
            renderer.draw_icon({480, 335, 13, 13}, onyxui::sdlpp::sdlpp_renderer::icon_style::radio_checked, black, white);

            // Row 4: Window controls
            renderer.draw_icon({460, 360, 16, 16}, onyxui::sdlpp::sdlpp_renderer::icon_style::minimize, black, white);
            renderer.draw_icon({480, 360, 16, 16}, onyxui::sdlpp::sdlpp_renderer::icon_style::maximize, black, white);
            renderer.draw_icon({500, 360, 16, 16}, onyxui::sdlpp::sdlpp_renderer::icon_style::restore, black, white);
            renderer.draw_icon({520, 360, 16, 16}, onyxui::sdlpp::sdlpp_renderer::icon_style::close_x, black, white);

            // Row 5: Misc
            renderer.draw_icon({460, 385, 16, 16}, onyxui::sdlpp::sdlpp_renderer::icon_style::check, black, white);
            renderer.draw_icon({480, 385, 16, 16}, onyxui::sdlpp::sdlpp_renderer::icon_style::bullet, black, white);
            renderer.draw_icon({500, 385, 16, 16}, onyxui::sdlpp::sdlpp_renderer::icon_style::folder, black, white);
            renderer.draw_icon({520, 385, 16, 16}, onyxui::sdlpp::sdlpp_renderer::icon_style::menu, black, white);

            // Status bar
            renderer.draw_text({30, 550, 740, 20}, "Press ESC to exit", italic_font, dark_gray, gray);

            // Present
            sdl_renderer.present();
        }

        std::cerr << "Shutting down...\n";

        // CRITICAL: Clear font cache before SDL shuts down
        // This prevents SIGSEGV from fonts being destroyed after FreeType shutdown
        onyxui::sdlpp::sdlpp_renderer::shutdown();

        std::cerr << "Clean exit.\n";
        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}
