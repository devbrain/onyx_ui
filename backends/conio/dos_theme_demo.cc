//
// Created by igor on 16/10/2025.
//
// Demo showcasing all DOS theme variants for the conio backend

#include "dos_theme.hh"
#include "vram.hh"
#include "drawing_utils.hh"
#include <onyxui/widgets/label.hh>
#include <onyxui/widgets/button.hh>
#include <onyxui/widgets/vbox.hh>
#include <onyxui/widgets/panel.hh>
#include <termbox2.h>
#include <memory>
#include <vector>
#include <string>

using namespace onyxui;
using namespace onyxui::conio;

/**
 * @brief Demo application showing all DOS themes
 *
 * This demo cycles through all available DOS themes every 3 seconds,
 * demonstrating:
 * - DOS Blue (classic DOS applications)
 * - DOS Dark (Norton Commander style)
 * - DOS Monochrome (Hercules monitors)
 * - Norton Utilities (amber on brown)
 */
class dos_theme_demo {
public:
    dos_theme_demo() {
        // Create vram instance (handles termbox2 initialization)
        m_vram = std::make_shared<vram>();

        // Create renderer
        m_renderer = std::make_unique<conio_renderer>(m_vram);

        // Initialize themes
        m_themes.push_back(create_dos_blue_theme());
        m_theme_names.push_back("DOS Blue Theme (Classic)");

        m_themes.push_back(create_dos_dark_theme());
        m_theme_names.push_back("DOS Dark Theme (Norton Commander)");

        m_themes.push_back(create_dos_monochrome_theme());
        m_theme_names.push_back("DOS Monochrome Theme (Hercules)");

        m_themes.push_back(create_norton_utilities_theme());
        m_theme_names.push_back("Norton Utilities Theme (Amber)");

        // Create UI
        build_ui();
    }

    ~dos_theme_demo() {
        // vram destructor handles termbox2 cleanup
    }

    void run() {
        const int THEME_DISPLAY_SECONDS = 3;
        const int FRAME_DELAY_MS = 100;  // 10 FPS
        const int FRAMES_PER_THEME = (THEME_DISPLAY_SECONDS * 1000) / FRAME_DELAY_MS;

        int frame_count = 0;

        while (true) {
            // Check for quit event
            tb_event ev;
            if (tb_peek_event(&ev, FRAME_DELAY_MS) == TB_OK) {
                if (ev.type == TB_EVENT_KEY) {
                    // ESC or Ctrl+C to quit
                    if (ev.key == TB_KEY_ESC ||
                        (ev.ch == 'c' && (ev.mod & TB_MOD_CTRL))) {
                        break;
                    }
                }
            }

            // Cycle themes
            if (frame_count % FRAMES_PER_THEME == 0) {
                m_current_theme = (m_current_theme + 1) % m_themes.size();
                apply_current_theme();
            }

            // Render
            render();

            ++frame_count;
        }
    }

private:
    void build_ui() {
        // Create root panel
        m_root = std::make_unique<panel<conio_backend>>();
        m_root->set_layout_strategy(
            std::make_unique<linear_layout<conio_backend>>(direction::vertical)
        );
        m_root->set_has_border(true);

        // Title label
        auto title = std::make_unique<label<conio_backend>>("DOS Theme Gallery");
        m_title_label = title.get();
        m_root->add_child(std::move(title));

        // Theme name label
        auto theme_name = std::make_unique<label<conio_backend>>("");
        m_theme_name_label = theme_name.get();
        m_root->add_child(std::move(theme_name));

        // Description panel
        auto desc_panel = std::make_unique<panel<conio_backend>>();
        desc_panel->set_layout_strategy(
            std::make_unique<linear_layout<conio_backend>>(direction::vertical)
        );
        desc_panel->set_has_border(true);

        // Add description labels
        m_desc_labels.push_back(
            desc_panel->add_child(std::make_unique<label<conio_backend>>("Button: Normal state"))
        );
        m_desc_labels.push_back(
            desc_panel->add_child(std::make_unique<label<conio_backend>>("Button: Hover state"))
        );
        m_desc_labels.push_back(
            desc_panel->add_child(std::make_unique<label<conio_backend>>("Button: Pressed state"))
        );
        m_desc_labels.push_back(
            desc_panel->add_child(std::make_unique<label<conio_backend>>("Button: Disabled state"))
        );

        m_root->add_child(std::move(desc_panel));

        // Button examples
        auto button_panel = std::make_unique<panel<conio_backend>>();
        button_panel->set_layout_strategy(
            std::make_unique<linear_layout<conio_backend>>(direction::vertical)
        );

        auto btn1 = std::make_unique<button<conio_backend>>("OK");
        auto btn2 = std::make_unique<button<conio_backend>>("Cancel");
        auto btn3 = std::make_unique<button<conio_backend>>("Help");

        m_buttons.push_back(btn1.get());
        m_buttons.push_back(btn2.get());
        m_buttons.push_back(btn3.get());

        button_panel->add_child(std::move(btn1));
        button_panel->add_child(std::move(btn2));
        button_panel->add_child(std::move(btn3));

        m_root->add_child(std::move(button_panel));

        // Instructions
        auto instructions = std::make_unique<label<conio_backend>>("Press ESC or Ctrl+C to quit");
        m_root->add_child(std::move(instructions));

        // Apply initial theme
        apply_current_theme();
    }

    void apply_current_theme() {
        m_root->apply_theme(m_themes[m_current_theme]);
        m_theme_name_label->set_text(m_theme_names[m_current_theme]);
    }

    void render() {
        // Clear screen
        tb_clear();

        // Get terminal size
        const int width = tb_width();
        const int height = tb_height();

        // Layout
        m_root->measure(width, height);
        m_root->arrange(conio_backend::rect{0, 0, width, height});

        // Render (TODO: implement render method in ui_element)
        // For now, just present the vram
        m_vram->present();
    }

private:
    std::shared_ptr<vram> m_vram;
    std::unique_ptr<conio_renderer> m_renderer;
    std::unique_ptr<panel<conio_backend>> m_root;

    std::vector<ui_theme<conio_backend>> m_themes;
    std::vector<std::string> m_theme_names;
    size_t m_current_theme = 0;

    // UI element references
    label<conio_backend>* m_title_label = nullptr;
    label<conio_backend>* m_theme_name_label = nullptr;
    std::vector<label<conio_backend>*> m_desc_labels;
    std::vector<button<conio_backend>*> m_buttons;
};

int main() {
    try {
        dos_theme_demo demo;
        demo.run();
        return 0;
    } catch (const std::exception&) {
        // vram destructor handles cleanup
        return 1;
    }
}
