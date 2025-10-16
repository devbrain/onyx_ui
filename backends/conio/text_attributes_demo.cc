//
// Created by igor on 16/10/2025.
//
// Demo showcasing vram text attribute support
//
// Demonstrates:
// - Bold text
// - Underlined text
// - Reverse video
// - Italic text (if terminal supports it)
// - Combined attributes (bold + underline, etc.)

#include "vram.hh"
#include "colors.hh"
#include <termbox2.h>
#include <string>

using namespace onyxui::conio;

static void draw_text(vram* v, int x, int y, const std::string& text,
                      const color& fg, const color& bg,
                      text_attribute attr = text_attribute::none) {
    for (size_t i = 0; i < text.length() && x + static_cast<int>(i) < v->get_width(); ++i) {
        v->put(x + static_cast<int>(i), y, text[i], fg, bg, attr);
    }
}

int main() {
    try {
        // Create vram (initializes termbox2)
        vram v;

        const int width = v.get_width();
        const int height = v.get_height();

        // Colors
        const color white{255, 255, 255};
        const color black{0, 0, 0};
        const color cyan{0, 255, 255};
        const color yellow{255, 255, 0};
        const color green{0, 255, 0};
        const color red{255, 0, 0};

        int y = 1;

        // Title
        draw_text(&v, 2, y, "Text Attribute Showcase", cyan, black, text_attribute::bold);
        y += 2;

        // Normal text
        draw_text(&v, 2, y, "Normal text:      ", white, black);
        draw_text(&v, 20, y, "The quick brown fox", white, black);
        y += 2;

        // Bold
        draw_text(&v, 2, y, "Bold:             ", white, black);
        draw_text(&v, 20, y, "The quick brown fox", white, black, text_attribute::bold);
        y += 2;

        // Underline
        draw_text(&v, 2, y, "Underline:        ", white, black);
        draw_text(&v, 20, y, "The quick brown fox", white, black, text_attribute::underline);
        y += 2;

        // Reverse
        draw_text(&v, 2, y, "Reverse:          ", white, black);
        draw_text(&v, 20, y, "The quick brown fox", white, black, text_attribute::reverse);
        y += 2;

        // Italic (may not work on all terminals)
        draw_text(&v, 2, y, "Italic:           ", white, black);
        draw_text(&v, 20, y, "The quick brown fox", white, black, text_attribute::italic);
        y += 2;

        // Bold + Underline
        draw_text(&v, 2, y, "Bold + Underline: ", white, black);
        draw_text(&v, 20, y, "The quick brown fox", white, black,
                  text_attribute::bold | text_attribute::underline);
        y += 2;

        // Bold + Reverse
        draw_text(&v, 2, y, "Bold + Reverse:   ", white, black);
        draw_text(&v, 20, y, "The quick brown fox", white, black,
                  text_attribute::bold | text_attribute::reverse);
        y += 2;

        // All attributes
        draw_text(&v, 2, y, "All combined:     ", white, black);
        draw_text(&v, 20, y, "The quick brown fox", white, black,
                  text_attribute::bold | text_attribute::underline |
                  text_attribute::reverse | text_attribute::italic);
        y += 2;

        // Color examples with attributes
        y += 1;
        draw_text(&v, 2, y, "Colors with attributes:", cyan, black, text_attribute::bold);
        y += 2;

        draw_text(&v, 2, y, "Bold Cyan:        ", white, black);
        draw_text(&v, 20, y, "Colored text", cyan, black, text_attribute::bold);
        y += 1;

        draw_text(&v, 2, y, "Bold Underlined:  ", white, black);
        draw_text(&v, 20, y, "Colored text", yellow, black,
                  text_attribute::bold | text_attribute::underline);
        y += 1;

        draw_text(&v, 2, y, "Reverse Green:    ", white, black);
        draw_text(&v, 20, y, "Colored text", green, black, text_attribute::reverse);
        y += 1;

        draw_text(&v, 2, y, "Bold Red:         ", white, black);
        draw_text(&v, 20, y, "Colored text", red, black, text_attribute::bold);
        y += 2;

        // Instructions
        draw_text(&v, 2, height - 2, "Press any key to exit", white, black, text_attribute::reverse);

        // Present and wait
        v.present();

        tb_event ev;
        tb_poll_event(&ev);

        return 0;

    } catch (const std::exception& e) {
        return 1;
    }
}
