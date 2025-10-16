//
// Created by igor on 16/10/2025.
//

#include "conio_backend.hh"
#include "dos_chars.h"
#include <utf8.h>
#include <algorithm>

namespace onyxui::conio {
    // ======================================================================
    // Constants
    // ======================================================================

    namespace {
        // TUI character cell dimensions
        constexpr int TUI_CELL_HEIGHT = 1;  // Each character is 1 row high

        // Rectangle boundary offsets
        constexpr int RECT_EDGE_OFFSET = 1;  // Offset for inner edges (exclude corners)
    } // anonymous namespace

    // ======================================================================
    // Box Drawing
    // ======================================================================

    void conio_renderer::draw_box(const rect& r, box_style style) {
        if (!m_vram) return;
        if (style == box_style::none) return;

        // Get clip region
        const rect clip = get_clip_rect();

        // Box drawing characters based on style
        int tl, tr, bl, br, h, v;

        switch (style) {
            case box_style::single_line:
                tl = DOS_TL; tr = DOS_TR; bl = DOS_BL; br = DOS_BR;
                h = DOS_H; v = DOS_V;
                break;
            case box_style::double_line:
                tl = DOS_TL2; tr = DOS_TR2; bl = DOS_BL2; br = DOS_BR2;
                h = DOS_H2; v = DOS_V2;
                break;
            case box_style::rounded:
                tl = BOX_ROUND_TL; tr = BOX_ROUND_TR;
                bl = BOX_ROUND_BL; br = BOX_ROUND_BR;
                h = BOX_ROUND_H; v = BOX_ROUND_V;
                break;
            case box_style::heavy:
                tl = BOX_HEAVY_ROUND_TL; tr = BOX_HEAVY_ROUND_TR;
                bl = BOX_HEAVY_ROUND_BL; br = BOX_HEAVY_ROUND_BR;
                h = BOX_HEAVY_H; v = BOX_HEAVY_V;
                break;
            default:
                return;
        }

        // Calculate corner positions (last cell in each dimension)
        const int right_edge = r.x + r.w - RECT_EDGE_OFFSET;
        const int bottom_edge = r.y + r.h - RECT_EDGE_OFFSET;

        // Draw corners (if visible)
        if (r.x >= clip.x && r.x < clip.x + clip.w &&
            r.y >= clip.y && r.y < clip.y + clip.h) {
            m_vram->put(r.x, r.y, tl, m_fg, m_bg);  // Top-left
        }

        if (right_edge >= clip.x && right_edge < clip.x + clip.w &&
            r.y >= clip.y && r.y < clip.y + clip.h) {
            m_vram->put(right_edge, r.y, tr, m_fg, m_bg);  // Top-right
        }

        if (r.x >= clip.x && r.x < clip.x + clip.w &&
            bottom_edge >= clip.y && bottom_edge < clip.y + clip.h) {
            m_vram->put(r.x, bottom_edge, bl, m_fg, m_bg);  // Bottom-left
        }

        if (right_edge >= clip.x && right_edge < clip.x + clip.w &&
            bottom_edge >= clip.y && bottom_edge < clip.y + clip.h) {
            m_vram->put(right_edge, bottom_edge, br, m_fg, m_bg);  // Bottom-right
        }

        // Draw horizontal lines
        for (int x = r.x + RECT_EDGE_OFFSET; x < r.x + r.w - RECT_EDGE_OFFSET; ++x) {
            if (x >= clip.x && x < clip.x + clip.w) {
                // Top edge
                if (r.y >= clip.y && r.y < clip.y + clip.h) {
                    m_vram->put(x, r.y, h, m_fg, m_bg);
                }
                // Bottom edge
                if (r.y + r.h - RECT_EDGE_OFFSET >= clip.y && r.y + r.h - RECT_EDGE_OFFSET < clip.y + clip.h) {
                    m_vram->put(x, r.y + r.h - RECT_EDGE_OFFSET, h, m_fg, m_bg);
                }
            }
        }

        // Draw vertical lines
        for (int y = r.y + RECT_EDGE_OFFSET; y < r.y + r.h - RECT_EDGE_OFFSET; ++y) {
            if (y >= clip.y && y < clip.y + clip.h) {
                // Left edge
                if (r.x >= clip.x && r.x < clip.x + clip.w) {
                    m_vram->put(r.x, y, v, m_fg, m_bg);
                }
                // Right edge
                if (r.x + r.w - RECT_EDGE_OFFSET >= clip.x && r.x + r.w - RECT_EDGE_OFFSET < clip.x + clip.w) {
                    m_vram->put(r.x + r.w - RECT_EDGE_OFFSET, y, v, m_fg, m_bg);
                }
            }
        }
    }

    // ======================================================================
    // Text Drawing
    // ======================================================================

    void conio_renderer::draw_text(const rect& r, std::string_view text, const font& /*f*/) {
        if (!m_vram) return;
        if (text.empty()) return;

        const rect clip = get_clip_rect();

        int x = r.x;
        int y = r.y;

        // Iterate through UTF-8 string
        auto it = text.begin();
        auto end = text.end();

        while (it != end && x < r.x + r.w) {
            // Check if current position is within clip region
            if (x >= clip.x && x < clip.x + clip.w &&
                y >= clip.y && y < clip.y + clip.h) {

                // Get next UTF-8 codepoint
                uint32_t codepoint = utf8::next(it, end);

                // Draw the character
                m_vram->put(x, y, static_cast<int>(codepoint), m_fg, m_bg);
            } else {
                // Still need to advance iterator even if not drawing
                utf8::next(it, end);
            }

            ++x;
        }
    }

    // ======================================================================
    // Icon Drawing
    // ======================================================================

    void conio_renderer::draw_icon(const rect& r, icon_style style) {
        if (!m_vram) return;
        if (style == icon_style::none) return;

        const rect clip = get_clip_rect();

        // Check if icon position is within clip region
        if (r.x < clip.x || r.x >= clip.x + clip.w ||
            r.y < clip.y || r.y >= clip.y + clip.h) {
            return;
        }

        // Map icon style to character
        int ch;
        switch (style) {
            case icon_style::check:       ch = DOS_CHECK; break;
            case icon_style::cross:       ch = DOS_CROSS; break;
            case icon_style::arrow_up:    ch = DOS_ARROW_UP; break;
            case icon_style::arrow_down:  ch = DOS_ARROW_DOWN; break;
            case icon_style::arrow_left:  ch = DOS_ARROW_LEFT; break;
            case icon_style::arrow_right: ch = DOS_ARROW_RIGHT; break;
            case icon_style::bullet:      ch = DOS_BULLET; break;
            case icon_style::folder:      ch = DOS_TRIANGLE_RIGHT; break;
            case icon_style::file:        ch = DOS_SQUARE_FILLED; break;
            default:                      return;
        }

        m_vram->put(r.x, r.y, ch, m_fg, m_bg);
    }

    // ======================================================================
    // Clipping
    // ======================================================================

    void conio_renderer::push_clip(const rect& r) {
        if (!m_vram) return;
        m_vram->set_clip(r);
    }

    void conio_renderer::pop_clip() {
        if (!m_vram) return;
        // Reset to full screen clip (origin at 0,0, full vram dimensions)
        constexpr int SCREEN_ORIGIN_X = 0;
        constexpr int SCREEN_ORIGIN_Y = 0;
        const rect full{SCREEN_ORIGIN_X, SCREEN_ORIGIN_Y, m_vram->get_width(), m_vram->get_height()};
        m_vram->set_clip(full);
    }

    rect conio_renderer::get_clip_rect() const {
        // Return empty rect if no vram
        constexpr int EMPTY_RECT_X = 0;
        constexpr int EMPTY_RECT_Y = 0;
        constexpr int EMPTY_RECT_W = 0;
        constexpr int EMPTY_RECT_H = 0;

        if (!m_vram) return rect{EMPTY_RECT_X, EMPTY_RECT_Y, EMPTY_RECT_W, EMPTY_RECT_H};
        return m_vram->get_clip();
    }

    // ======================================================================
    // Text Measurement (Static)
    // ======================================================================

    size conio_renderer::measure_text(std::string_view text, const font& /*f*/) {
        // Count Unicode code points (visual characters)
        const int glyph_count = static_cast<int>(
            utf8::distance(text.begin(), text.end())
        );

        // In a TUI, each character is 1x1 cell
        return size{glyph_count, TUI_CELL_HEIGHT};
    }

    // ======================================================================
    // Color Management
    // ======================================================================

    void conio_renderer::set_foreground(const color& c) {
        m_fg = c;
    }

    void conio_renderer::set_background(const color& c) {
        m_bg = c;
    }
}
