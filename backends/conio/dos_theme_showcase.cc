//
// Created by igor on 16/10/2025.
//
// Simple showcase of DOS themes using direct vram rendering
//
// This demo demonstrates the onyx_ui action and hotkey system:
// - Actions encapsulate user commands (switching themes, quitting)
// - Keyboard shortcuts are assigned to actions via set_shortcut()
// - hotkey_manager handles event dispatch and conflict detection
// - Actions emit signals when triggered, connected to application logic
//
// Hotkeys:
//   1 - Switch to DOS Blue theme (Classic)
//   2 - Switch to DOS Dark theme (Norton Commander)
//   3 - Switch to DOS Monochrome theme (Hercules)
//   4 - Switch to Norton Utilities theme (Amber)
//   ESC / Ctrl+C - Quit

#include "dos_theme.hh"
#include "vram.hh"
#include "dos_chars.h"
#include <onyxui/widgets/action.hh>
#include <onyxui/hotkeys/hotkey_manager.hh>
#include <termbox2.h>
#include <memory>
#include <vector>
#include <string>
#include <cstring>

using namespace onyxui;
using namespace onyxui::conio;

/**
 * @brief Draw a text line at position with given colors
 */
static void draw_text(vram* v, int x, int y, const std::string& text, const color& fg, const color& bg) {
    for (size_t i = 0; i < text.length() && x + static_cast<int>(i) < v->get_width(); ++i) {
        v->put(x + static_cast<int>(i), y, text[i], fg, bg);
    }
}

/**
 * @brief Draw a filled rectangle
 */
static void fill_rect(vram* v, int x, int y, int w, int h, const color& fg, const color& bg) {
    for (int dy = 0; dy < h; ++dy) {
        for (int dx = 0; dx < w; ++dx) {
            if (x + dx < v->get_width() && y + dy < v->get_height()) {
                v->put(x + dx, y + dy, ' ', fg, bg);
            }
        }
    }
}

/**
 * @brief Draw a simple box using DOS characters
 */
static void draw_box(vram* v, int x, int y, int w, int h, const color& fg, const color& bg, bool double_line = false) {
    int tl, tr, bl, br, horiz, vert;

    if (double_line) {
        tl = DOS_TL2; tr = DOS_TR2; bl = DOS_BL2; br = DOS_BR2;
        horiz = DOS_H2; vert = DOS_V2;
    } else {
        tl = DOS_TL; tr = DOS_TR; bl = DOS_BL; br = DOS_BR;
        horiz = DOS_H; vert = DOS_V;
    }

    // Corners
    v->put(x, y, tl, fg, bg);
    v->put(x + w - 1, y, tr, fg, bg);
    v->put(x, y + h - 1, bl, fg, bg);
    v->put(x + w - 1, y + h - 1, br, fg, bg);

    // Horizontal lines
    for (int i = 1; i < w - 1; ++i) {
        v->put(x + i, y, horiz, fg, bg);
        v->put(x + i, y + h - 1, horiz, fg, bg);
    }

    // Vertical lines
    for (int i = 1; i < h - 1; ++i) {
        v->put(x, y + i, vert, fg, bg);
        v->put(x + w - 1, y + i, vert, fg, bg);
    }
}

/**
 * @brief Render a theme showcase
 */
static void render_theme(vram* v, const onyxui::ui_theme<conio_backend>& theme, const std::string& theme_name) {
    const int width = v->get_width();
    const int height = v->get_height();

    // Clear with window background
    fill_rect(v, 0, 0, width, height, theme.text_fg, theme.window_bg);

    int y = 1;

    // Title
    const std::string title = "DOS Theme Showcase";
    const int title_x = (width - static_cast<int>(title.length())) / 2;
    draw_text(v, title_x, y, title, theme.border_color, theme.window_bg);
    y += 2;

    // Theme name
    const std::string name_prefix = "Current: ";
    const int name_x = (width - static_cast<int>(name_prefix.length() + theme_name.length())) / 2;
    draw_text(v, name_x, y, name_prefix + theme_name, theme.text_fg, theme.window_bg);
    y += 3;

    // Panel demo
    const int panel_x = 2;
    const int panel_w = width - 4;
    const int panel_h = 10;

    draw_box(v, panel_x, y, panel_w, panel_h, theme.panel.border_color, theme.panel.background, false);
    fill_rect(v, panel_x + 1, y + 1, panel_w - 2, panel_h - 2, theme.panel.border_color, theme.panel.background);

    // Panel title
    const std::string panel_title = " Panel Example ";
    draw_text(v, panel_x + 2, y, panel_title, theme.border_color, theme.window_bg);

    // Panel content
    draw_text(v, panel_x + 2, y + 2, "This is a panel with border", theme.text_fg, theme.panel.background);
    draw_text(v, panel_x + 2, y + 3, "Panel background color shown here", theme.text_fg, theme.panel.background);

    y += panel_h + 2;

    // Button demo
    const int btn_y = y;
    const int btn_width = 12;
    const int btn_height = 3;
    const int btn_spacing = 2;

    // Normal button
    int btn_x = 5;
    fill_rect(v, btn_x, btn_y, btn_width, btn_height, theme.button.fg_normal, theme.button.bg_normal);
    draw_box(v, btn_x, btn_y, btn_width, btn_height, theme.button.fg_normal, theme.button.bg_normal, false);
    draw_text(v, btn_x + 2, btn_y + 1, " Normal ", theme.button.fg_normal, theme.button.bg_normal);

    // Hover button
    btn_x += btn_width + btn_spacing;
    fill_rect(v, btn_x, btn_y, btn_width, btn_height, theme.button.fg_hover, theme.button.bg_hover);
    draw_box(v, btn_x, btn_y, btn_width, btn_height, theme.button.fg_hover, theme.button.bg_hover, false);
    draw_text(v, btn_x + 2, btn_y + 1, " Hover  ", theme.button.fg_hover, theme.button.bg_hover);

    // Pressed button
    btn_x += btn_width + btn_spacing;
    fill_rect(v, btn_x, btn_y, btn_width, btn_height, theme.button.fg_pressed, theme.button.bg_pressed);
    draw_box(v, btn_x, btn_y, btn_width, btn_height, theme.button.fg_pressed, theme.button.bg_pressed, false);
    draw_text(v, btn_x + 2, btn_y + 1, "Pressed ", theme.button.fg_pressed, theme.button.bg_pressed);

    // Disabled button
    btn_x += btn_width + btn_spacing;
    fill_rect(v, btn_x, btn_y, btn_width, btn_height, theme.button.fg_disabled, theme.button.bg_disabled);
    draw_box(v, btn_x, btn_y, btn_width, btn_height, theme.button.fg_disabled, theme.button.bg_disabled, false);
    draw_text(v, btn_x + 2, btn_y + 1, "Disabled", theme.button.fg_disabled, theme.button.bg_disabled);

    y = btn_y + btn_height + 2;

    // Label demo
    draw_text(v, 5, y, "Label (normal text):", theme.label.text, theme.label.background);
    draw_text(v, 30, y, "Sample Label Text", theme.label.text, theme.label.background);

    y += 2;

    // Instructions
    const std::string instr = "Press 1-4 to switch themes | ESC or Ctrl+C to quit";
    const int instr_x = (width - static_cast<int>(instr.length())) / 2;
    draw_text(v, instr_x, height - 2, instr, theme.border_color, theme.window_bg);
}

int main() {
    try {
        // Create vram instance (handles termbox2 initialization)
        vram v;

        // Initialize themes
        std::vector<ui_theme<conio_backend>> themes;
        std::vector<std::string> theme_names;

        themes.push_back(create_dos_blue_theme());
        theme_names.push_back("DOS Blue (Classic)");

        themes.push_back(create_dos_dark_theme());
        theme_names.push_back("DOS Dark (Norton Commander)");

        themes.push_back(create_dos_monochrome_theme());
        theme_names.push_back("DOS Monochrome (Hercules)");

        themes.push_back(create_norton_utilities_theme());
        theme_names.push_back("Norton Utilities (Amber)");

        size_t current_theme = 0;

        // Create hotkey manager
        hotkey_manager<conio_backend> hotkeys;

        // Create actions for theme switching
        auto theme1_action = std::make_shared<action<conio_backend>>();
        theme1_action->set_text("Theme 1 - DOS Blue");
        theme1_action->set_shortcut('1');
        theme1_action->triggered.connect([&]() { current_theme = 0; });
        hotkeys.register_action(theme1_action);

        auto theme2_action = std::make_shared<action<conio_backend>>();
        theme2_action->set_text("Theme 2 - DOS Dark");
        theme2_action->set_shortcut('2');
        theme2_action->triggered.connect([&]() { current_theme = 1; });
        hotkeys.register_action(theme2_action);

        auto theme3_action = std::make_shared<action<conio_backend>>();
        theme3_action->set_text("Theme 3 - DOS Monochrome");
        theme3_action->set_shortcut('3');
        theme3_action->triggered.connect([&]() { current_theme = 2; });
        hotkeys.register_action(theme3_action);

        auto theme4_action = std::make_shared<action<conio_backend>>();
        theme4_action->set_text("Theme 4 - Norton Utilities");
        theme4_action->set_shortcut('4');
        theme4_action->triggered.connect([&]() { current_theme = 3; });
        hotkeys.register_action(theme4_action);

        // Create quit action (ESC)
        bool should_quit = false;
        auto quit_action = std::make_shared<action<conio_backend>>();
        quit_action->set_text("Quit");
        quit_action->triggered.connect([&]() { should_quit = true; });
        // Note: ESC is not an ASCII key, handle separately

        // Main loop
        while (!should_quit) {
            // Render current theme
            render_theme(&v, themes[current_theme], theme_names[current_theme]);
            v.present();

            // Wait for event
            tb_event ev;
            if (tb_poll_event(&ev) != TB_OK) {
                break;
            }

            if (ev.type == TB_EVENT_KEY) {
                // Handle ESC and Ctrl+C specially (not through hotkey manager)
                if (ev.key == TB_KEY_ESC || (ev.ch == 'c' && (ev.mod & TB_MOD_CTRL))) {
                    quit_action->trigger();
                    continue;
                }

                // Try to handle through hotkey manager
                hotkeys.handle_key_event(ev, nullptr);
            }
        }

        return 0;

    } catch (const std::exception&) {
        return 1;
    }
}
