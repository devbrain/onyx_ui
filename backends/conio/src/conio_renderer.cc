//
// Created by igor on 16/10/2025.
//

#include <onyxui/conio/conio_backend.hh>
#include "dos_chars.h"
#include <utf8.h>
#include "vram.hh"
#include <algorithm>
#include <iostream>

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

    struct conio_renderer::impl {
        vram m_vram;
        // Default white foreground
        color m_fg{color::MAX_COMPONENT, color::MAX_COMPONENT, color::MAX_COMPONENT};
        // Default black background
        color m_bg{color::MIN_COMPONENT, color::MIN_COMPONENT, color::MIN_COMPONENT};
    };

    // ======================================================================
    // Box Drawing
    // ======================================================================

    conio_renderer::conio_renderer()
        : m_pimpl(std::make_unique<impl>()){
    }

    conio_renderer::~conio_renderer() = default;

    void conio_renderer::draw_box(const rect& r, const box_style& style) {
        const rect clip = get_clip_rect();

        // Fill interior if solid
        if (style.is_solid) {
            for (int y = r.y; y < r.y + r.h; ++y) {
                for (int x = r.x; x < r.x + r.w; ++x) {
                    // Check if position is within clip region
                    if (x >= clip.x && x < clip.x + clip.w &&
                        y >= clip.y && y < clip.y + clip.h) {
                        // Fill with space character using background color
                        m_pimpl->m_vram.put(x, y, ' ', m_pimpl->m_fg, m_pimpl->m_bg);
                    }
                }
            }
        }

        // Draw border if not none
        if (style.style == border_style::none) return;

        // Box drawing characters based on border style
        int tl = 0;
        int tr = 0;
        int bl = 0;
        int br = 0;
        int h = 0;
        int v = 0;

        switch (style.style) {
            case border_style::single_line:
                tl = DOS_TL; tr = DOS_TR; bl = DOS_BL; br = DOS_BR;
                h = DOS_H; v = DOS_V;
                break;
            case border_style::double_line:
                tl = DOS_TL2; tr = DOS_TR2; bl = DOS_BL2; br = DOS_BR2;
                h = DOS_H2; v = DOS_V2;
                break;
            case border_style::rounded:
                tl = BOX_ROUND_TL; tr = BOX_ROUND_TR;
                bl = BOX_ROUND_BL; br = BOX_ROUND_BR;
                h = BOX_ROUND_H; v = BOX_ROUND_V;
                break;
            case border_style::heavy:
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
            m_pimpl->m_vram.put(r.x, r.y, tl, m_pimpl->m_fg, m_pimpl->m_bg);  // Top-left
        }

        if (right_edge >= clip.x && right_edge < clip.x + clip.w &&
            r.y >= clip.y && r.y < clip.y + clip.h) {
            m_pimpl->m_vram.put(right_edge, r.y, tr, m_pimpl->m_fg, m_pimpl->m_bg);  // Top-right
        }

        if (r.x >= clip.x && r.x < clip.x + clip.w &&
            bottom_edge >= clip.y && bottom_edge < clip.y + clip.h) {
            m_pimpl->m_vram.put(r.x, bottom_edge, bl, m_pimpl->m_fg, m_pimpl->m_bg);  // Bottom-left
        }

        if (right_edge >= clip.x && right_edge < clip.x + clip.w &&
            bottom_edge >= clip.y && bottom_edge < clip.y + clip.h) {
            m_pimpl->m_vram.put(right_edge, bottom_edge, br, m_pimpl->m_fg, m_pimpl->m_bg);  // Bottom-right
        }

        // Draw horizontal lines
        for (int x = r.x + RECT_EDGE_OFFSET; x < r.x + r.w - RECT_EDGE_OFFSET; ++x) {
            if (x >= clip.x && x < clip.x + clip.w) {
                // Top edge
                if (r.y >= clip.y && r.y < clip.y + clip.h) {
                    m_pimpl->m_vram.put(x, r.y, h, m_pimpl->m_fg, m_pimpl->m_bg);
                }
                // Bottom edge
                if (r.y + r.h - RECT_EDGE_OFFSET >= clip.y && r.y + r.h - RECT_EDGE_OFFSET < clip.y + clip.h) {
                    m_pimpl->m_vram.put(x, r.y + r.h - RECT_EDGE_OFFSET, h, m_pimpl->m_fg, m_pimpl->m_bg);
                }
            }
        }

        // Draw vertical lines
        for (int y = r.y + RECT_EDGE_OFFSET; y < r.y + r.h - RECT_EDGE_OFFSET; ++y) {
            if (y >= clip.y && y < clip.y + clip.h) {
                // Left edge
                if (r.x >= clip.x && r.x < clip.x + clip.w) {
                    m_pimpl->m_vram.put(r.x, y, v, m_pimpl->m_fg, m_pimpl->m_bg);
                }
                // Right edge
                if (r.x + r.w - RECT_EDGE_OFFSET >= clip.x && r.x + r.w - RECT_EDGE_OFFSET < clip.x + clip.w) {
                    m_pimpl->m_vram.put(r.x + r.w - RECT_EDGE_OFFSET, y, v, m_pimpl->m_fg, m_pimpl->m_bg);
                }
            }
        }
    }

    // ======================================================================
    // Text Drawing
    // ======================================================================

    void conio_renderer::draw_text(const rect& r, std::string_view text, const font& f) {
        if (text.empty()) return;

        const rect clip = get_clip_rect();

        // Convert font attributes to text_attribute flags
        text_attribute attr = text_attribute::none;
        if (f.bold) {
            attr |= text_attribute::bold;
        }
        if (f.underline) {
            attr |= text_attribute::underline;
        }
        if (f.reverse) {
            attr |= text_attribute::reverse;
        }

        int x = r.x;
        int y = r.y;

        // Iterate through UTF-8 string
        const auto *it = text.begin();
        const auto *end = text.end();

        while (it != end && x < r.x + r.w) {
            // Check if current position is within clip region
            if (x >= clip.x && x < clip.x + clip.w &&
                y >= clip.y && y < clip.y + clip.h) {

                // Get next UTF-8 codepoint
                uint32_t const codepoint = utf8::next(it, end);

                // Draw the character with attributes
                m_pimpl->m_vram.put(x, y, static_cast<int>(codepoint), m_pimpl->m_fg, m_pimpl->m_bg, attr);
            } else {
                // Still need to advance iterator even if not drawing
                utf8::next(it, end);
            }

            ++x;
        }
    }

    // ======================================================================
    // Background Drawing (Backend-Specific Optimization)
    // ======================================================================

    void conio_renderer::draw_background(const rect& viewport, const background_style& style) {
        const rect clip = get_clip_rect();

        // Set colors from background_style
        set_background(style.bg_color);
        set_foreground(style.bg_color);  // Use same for consistency

        // Fill entire viewport with the background style
        for (int y = viewport.y; y < viewport.y + viewport.h; ++y) {
            for (int x = viewport.x; x < viewport.x + viewport.w; ++x) {
                // Check if position is within clip region
                if (x >= clip.x && x < clip.x + clip.w &&
                    y >= clip.y && y < clip.y + clip.h) {
                    // Use fill_char from style (space = solid, others = patterns)
                    m_pimpl->m_vram.put(x, y, style.fill_char, m_pimpl->m_fg, m_pimpl->m_bg);
                }
            }
        }
    }

    void conio_renderer::draw_background(const rect& viewport,
                                         const background_style& style,
                                         const std::vector<rect>& dirty_regions) {
        // If no dirty regions, fall back to full viewport
        if (dirty_regions.empty()) {
            draw_background(viewport, style);
            return;
        }

        const rect clip = get_clip_rect();

        // Set colors from background_style
        set_background(style.bg_color);
        set_foreground(style.bg_color);

        // Fill only dirty regions (optimization)
        for (const auto& region : dirty_regions) {
            for (int y = region.y; y < region.y + region.h; ++y) {
                for (int x = region.x; x < region.x + region.w; ++x) {
                    // Check if position is within clip region
                    if (x >= clip.x && x < clip.x + clip.w &&
                        y >= clip.y && y < clip.y + clip.h) {
                        // Use fill_char from style
                        m_pimpl->m_vram.put(x, y, style.fill_char, m_pimpl->m_fg, m_pimpl->m_bg);
                    }
                }
            }
        }
    }

    // ======================================================================
    // Region Clearing
    // ======================================================================

    void conio_renderer::clear_region(const rect& r) {

        const rect clip = get_clip_rect();

        // Clear by filling with spaces using current background color
        for (int y = r.y; y < r.y + r.h; ++y) {
            for (int x = r.x; x < r.x + r.w; ++x) {
                // Check if position is within clip region
                if (x >= clip.x && x < clip.x + clip.w &&
                    y >= clip.y && y < clip.y + clip.h) {
                    // Put a space character with current background
                    m_pimpl->m_vram.put(x, y, ' ', m_pimpl->m_fg, m_pimpl->m_bg);
                }
            }
        }
    }

    // ======================================================================
    // Line Drawing
    // ======================================================================

    void conio_renderer::draw_horizontal_line(const rect& r, const line_style& style) {
        const rect clip = get_clip_rect();

        // Select line character based on style
        int line_char = 0;
        switch (style.style) {
            case border_style::single_line:
                line_char = DOS_H;  // ─
                break;
            case border_style::double_line:
                line_char = DOS_H2;  // ═
                break;
            case border_style::rounded:
                line_char = BOX_ROUND_H;  // ─ (same as single)
                break;
            case border_style::heavy:
                line_char = BOX_HEAVY_H;  // ━
                break;
            case border_style::none:
            default:
                return;  // Don't draw anything
        }

        // Draw horizontal line at y position
        const int y = r.y;
        for (int x = r.x; x < r.x + r.w; ++x) {
            // Check if position is within clip region
            if (x >= clip.x && x < clip.x + clip.w &&
                y >= clip.y && y < clip.y + clip.h) {
                m_pimpl->m_vram.put(x, y, line_char, m_pimpl->m_fg, m_pimpl->m_bg);
            }
        }
    }

    void conio_renderer::draw_vertical_line(const rect& r, const line_style& style) {
        const rect clip = get_clip_rect();

        // Select line character based on style
        int line_char = 0;
        switch (style.style) {
            case border_style::single_line:
                line_char = DOS_V;  // │
                break;
            case border_style::double_line:
                line_char = DOS_V2;  // ║
                break;
            case border_style::rounded:
                line_char = BOX_ROUND_V;  // │ (same as single)
                break;
            case border_style::heavy:
                line_char = BOX_HEAVY_V;  // ┃
                break;
            case border_style::none:
            default:
                return;  // Don't draw anything
        }

        // Draw vertical line at x position
        const int x = r.x;
        for (int y = r.y; y < r.y + r.h; ++y) {
            // Check if position is within clip region
            if (x >= clip.x && x < clip.x + clip.w &&
                y >= clip.y && y < clip.y + clip.h) {
                m_pimpl->m_vram.put(x, y, line_char, m_pimpl->m_fg, m_pimpl->m_bg);
            }
        }
    }

    // ======================================================================
    // Icon Drawing
    // ======================================================================

    void conio_renderer::draw_icon(const rect& r, icon_style style) {

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

        m_pimpl->m_vram.put(r.x, r.y, ch, m_pimpl->m_fg, m_pimpl->m_bg);
    }

    // ======================================================================
    // Clipping
    // ======================================================================

    void conio_renderer::push_clip(const rect& r) {
        // DEBUG: Detect bad clip rects
        if (r.y < 0 || r.y > 1000 || r.x < 0 || r.x > 1000) {
            std::cerr << "PUSH_CLIP: Bad clip rect requested!"
                      << " {x=" << r.x << ", y=" << r.y
                      << ", w=" << r.w << ", h=" << r.h << "}" << std::endl;
            std::cerr.flush();
        }
        m_pimpl->m_vram.set_clip(r);
    }

    void conio_renderer::pop_clip() {
        // Reset to full screen clip (origin at 0,0, full vram dimensions)
        constexpr int SCREEN_ORIGIN_X = 0;
        constexpr int SCREEN_ORIGIN_Y = 0;
        const rect full{SCREEN_ORIGIN_X, SCREEN_ORIGIN_Y, m_pimpl->m_vram.get_width(), m_pimpl->m_vram.get_height()};
        m_pimpl->m_vram.set_clip(full);
    }

    rect conio_renderer::get_clip_rect() const {
        return m_pimpl->m_vram.get_clip();
    }

    // ======================================================================
    // Presentation
    // ======================================================================

    void conio_renderer::present() {
        m_pimpl->m_vram.present();
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
        m_pimpl->m_fg = c;
    }

    void conio_renderer::set_background(const color& c) {
        m_pimpl->m_bg = c;
    }

    color conio_renderer::get_foreground() const { return m_pimpl->m_fg; }

    color conio_renderer::get_background() const { return m_pimpl->m_bg; }

    // ======================================================================
    // Resize Handling
    // ======================================================================

    void conio_renderer::on_resize() {
        m_pimpl->m_vram.resize();
        // Reset clip rect to full new vram dimensions
        // This ensures any previously set clip is updated to the new size
        pop_clip();
    }

    // ======================================================================
    // Viewport
    // ======================================================================

    rect conio_renderer::get_viewport() const {
        int w = m_pimpl->m_vram.get_width();
        int h = m_pimpl->m_vram.get_height();

        // DEBUG: Check vram dimensions
        if (w < 0 || w > 1000 || h < 0 || h > 1000) {
            std::cerr << "CONIO_RENDERER::get_viewport(): Bad vram dimensions!"
                      << " width=" << w << " height=" << h << std::endl;
            std::cerr.flush();
        }

        return rect{0, 0, w, h};
    }
}
