//
// Created by igor on 16/10/2025.
//

#include <onyxui/conio/conio_backend.hh>
#include "dos_chars.h"
#include <utf8.h>
#include "vram.hh"
#include <algorithm>
#include <iostream>
#include <stack>
#include <memory>       // for std::make_unique
#include <vector>       // for std::vector
#include <string_view>  // for std::string_view
#include <cstdint>      // for uint32_t
#include "onyxui/conio/colors.hh"       // for color
#include "onyxui/conio/geometry.hh"     // for rect
#include "onyxui/conio/conio_renderer.hh"  // for conio_renderer

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
        std::stack<rect> m_clip_stack;  // Clip rectangle stack for proper nesting
    };

    // ======================================================================
    // Box Drawing
    // ======================================================================

    conio_renderer::conio_renderer()
        : m_pimpl(std::make_unique<impl>()){
    }

    conio_renderer::~conio_renderer() = default;

    void conio_renderer::draw_box(const rect& r, const box_style& style, const color& fg, const color& bg) {
        const rect clip = get_clip_rect();

        // Fill interior if solid
        if (style.is_solid) {
            for (int y = r.y; y < r.y + r.h; ++y) {
                for (int x = r.x; x < r.x + r.w; ++x) {
                    // Check if position is within clip region
                    if (x >= clip.x && x < clip.x + clip.w &&
                        y >= clip.y && y < clip.y + clip.h) {
                        // Fill with space character using background color
                        m_pimpl->m_vram.put(x, y, ' ', fg, bg);
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
            m_pimpl->m_vram.put(r.x, r.y, tl, fg, bg);  // Top-left
        }

        if (right_edge >= clip.x && right_edge < clip.x + clip.w &&
            r.y >= clip.y && r.y < clip.y + clip.h) {
            m_pimpl->m_vram.put(right_edge, r.y, tr, fg, bg);  // Top-right
        }

        if (r.x >= clip.x && r.x < clip.x + clip.w &&
            bottom_edge >= clip.y && bottom_edge < clip.y + clip.h) {
            m_pimpl->m_vram.put(r.x, bottom_edge, bl, fg, bg);  // Bottom-left
        }

        if (right_edge >= clip.x && right_edge < clip.x + clip.w &&
            bottom_edge >= clip.y && bottom_edge < clip.y + clip.h) {
            m_pimpl->m_vram.put(right_edge, bottom_edge, br, fg, bg);  // Bottom-right
        }

        // Draw horizontal lines
        for (int x = r.x + RECT_EDGE_OFFSET; x < r.x + r.w - RECT_EDGE_OFFSET; ++x) {
            if (x >= clip.x && x < clip.x + clip.w) {
                // Top edge
                if (r.y >= clip.y && r.y < clip.y + clip.h) {
                    m_pimpl->m_vram.put(x, r.y, h, fg, bg);
                }
                // Bottom edge
                if (r.y + r.h - RECT_EDGE_OFFSET >= clip.y && r.y + r.h - RECT_EDGE_OFFSET < clip.y + clip.h) {
                    m_pimpl->m_vram.put(x, r.y + r.h - RECT_EDGE_OFFSET, h, fg, bg);
                }
            }
        }

        // Draw vertical lines
        for (int y = r.y + RECT_EDGE_OFFSET; y < r.y + r.h - RECT_EDGE_OFFSET; ++y) {
            if (y >= clip.y && y < clip.y + clip.h) {
                // Left edge
                if (r.x >= clip.x && r.x < clip.x + clip.w) {
                    m_pimpl->m_vram.put(r.x, y, v, fg, bg);
                }
                // Right edge
                if (r.x + r.w - RECT_EDGE_OFFSET >= clip.x && r.x + r.w - RECT_EDGE_OFFSET < clip.x + clip.w) {
                    m_pimpl->m_vram.put(r.x + r.w - RECT_EDGE_OFFSET, y, v, fg, bg);
                }
            }
        }
    }

    // ======================================================================
    // Text Drawing
    // ======================================================================

    void conio_renderer::draw_text(const rect& r, std::string_view text, const font& f, const color& fg, const color& bg) {
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
        const int y = r.y;

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
                m_pimpl->m_vram.put(x, y, static_cast<int>(codepoint), fg, bg, attr);
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

        // Use colors from background_style directly
        const color& bg = style.bg_color;
        const color& fg = style.bg_color;  // Use same for consistency

        // Fill entire viewport with the background style
        for (int y = viewport.y; y < viewport.y + viewport.h; ++y) {
            for (int x = viewport.x; x < viewport.x + viewport.w; ++x) {
                // Check if position is within clip region
                if (x >= clip.x && x < clip.x + clip.w &&
                    y >= clip.y && y < clip.y + clip.h) {
                    // Use fill_char from style (space = solid, others = patterns)
                    m_pimpl->m_vram.put(x, y, style.fill_char, fg, bg);
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

        // Use colors from background_style directly
        const color& bg = style.bg_color;
        const color& fg = style.bg_color;

        // Fill only dirty regions (optimization)
        for (const auto& region : dirty_regions) {
            for (int y = region.y; y < region.y + region.h; ++y) {
                for (int x = region.x; x < region.x + region.w; ++x) {
                    // Check if position is within clip region
                    if (x >= clip.x && x < clip.x + clip.w &&
                        y >= clip.y && y < clip.y + clip.h) {
                        // Use fill_char from style
                        m_pimpl->m_vram.put(x, y, style.fill_char, fg, bg);
                    }
                }
            }
        }
    }

    // ======================================================================
    // Region Clearing
    // ======================================================================

    void conio_renderer::clear_region(const rect& r, const color& bg) {

        const rect clip = get_clip_rect();

        // Clear by filling with spaces using specified background color
        for (int y = r.y; y < r.y + r.h; ++y) {
            for (int x = r.x; x < r.x + r.w; ++x) {
                // Check if position is within clip region
                if (x >= clip.x && x < clip.x + clip.w &&
                    y >= clip.y && y < clip.y + clip.h) {
                    // Put a space character with specified background
                    m_pimpl->m_vram.put(x, y, ' ', bg, bg);
                }
            }
        }
    }

    // ======================================================================
    // Shadow Drawing
    // ======================================================================

    void conio_renderer::draw_shadow(const rect& widget_bounds, int offset_x, int offset_y) {
        constexpr float SHADOW_DARKEN_FACTOR = 0.5f;  // Backend-specific constant

        // Shadow to the right (vertical strip) - stops before corner to avoid double-darkening
        const rect shadow_right{
            widget_bounds.x + widget_bounds.w,
            widget_bounds.y + offset_y,
            offset_x,
            widget_bounds.h - offset_y  // Don't extend into corner
        };

        // Shadow at the bottom (horizontal strip) - includes corner area
        const rect shadow_bottom{
            widget_bounds.x + offset_x,
            widget_bounds.y + widget_bounds.h,
            widget_bounds.w,
            offset_y
        };

        // Darken both shadow regions (they don't overlap now)
        m_pimpl->m_vram.darken_region(shadow_right, SHADOW_DARKEN_FACTOR);
        m_pimpl->m_vram.darken_region(shadow_bottom, SHADOW_DARKEN_FACTOR);
    }

    // ======================================================================
    // Line Drawing
    // ======================================================================

    void conio_renderer::draw_horizontal_line(const rect& r, const line_style& style, const color& fg, const color& bg) {
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
                m_pimpl->m_vram.put(x, y, line_char, fg, bg);
            }
        }
    }

    void conio_renderer::draw_vertical_line(const rect& r, const line_style& style, const color& fg, const color& bg) {
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
                m_pimpl->m_vram.put(x, y, line_char, fg, bg);
            }
        }
    }

    // ======================================================================
    // Icon Drawing
    // ======================================================================

    void conio_renderer::draw_icon(const rect& r, icon_style style, const color& fg, const color& bg) {

        if (style == icon_style::none) return;

        const rect clip = get_clip_rect();

        // Check if icon position is within clip region
        if (r.x < clip.x || r.x >= clip.x + clip.w ||
            r.y < clip.y || r.y >= clip.y + clip.h) {
            return;
        }

        // Handle multi-character checkbox icons (3 characters wide)
        if (style == icon_style::checkbox_unchecked ||
            style == icon_style::checkbox_checked ||
            style == icon_style::checkbox_indeterminate) {

            // Classic DOS-style 3-character checkbox rendering
            const char* checkbox_str;
            switch (style) {
                case icon_style::checkbox_unchecked:      checkbox_str = "[ ]"; break;
                case icon_style::checkbox_checked:        checkbox_str = "[X]"; break;
                case icon_style::checkbox_indeterminate:  checkbox_str = "[-]"; break;
                default: checkbox_str = "[ ]"; break;
            }

            // Render 3 characters horizontally
            for (int i = 0; i < 3 && r.x + i < clip.x + clip.w; ++i) {
                if (r.x + i >= clip.x) {
                    m_pimpl->m_vram.put(r.x + i, r.y, checkbox_str[i], fg, bg);
                }
            }
            return;
        }

        // Map single-character icon style to character
        int ch;
        switch (style) {
            // General purpose icons
            case icon_style::check:       ch = DOS_CHECK; break;
            case icon_style::cross:       ch = DOS_CROSS; break;
            case icon_style::bullet:      ch = DOS_BULLET; break;
            case icon_style::folder:      ch = DOS_TRIANGLE_RIGHT; break;
            case icon_style::file:        ch = DOS_SQUARE_FILLED; break;

            // Navigation arrows
            case icon_style::arrow_up:    ch = DOS_ARROW_UP; break;
            case icon_style::arrow_down:  ch = DOS_ARROW_DOWN; break;
            case icon_style::arrow_left:  ch = DOS_ARROW_LEFT; break;
            case icon_style::arrow_right: ch = DOS_ARROW_RIGHT; break;

            // Window management icons
            case icon_style::menu:        ch = DOS_MENU; break;
            case icon_style::minimize:    ch = DOS_MINIMIZE; break;
            case icon_style::maximize:    ch = DOS_MAXIMIZE; break;
            case icon_style::restore:     ch = DOS_RESTORE; break;
            case icon_style::close_x:     ch = DOS_CLOSE_X; break;

            // Text editing cursors
            case icon_style::cursor_insert:    ch = DOS_CURSOR_INSERT; break;
            case icon_style::cursor_overwrite: ch = DOS_CURSOR_OVERWRITE; break;

            default:                      return;
        }

        m_pimpl->m_vram.put(r.x, r.y, ch, fg, bg);
    }

    // ======================================================================
    // Clipping
    // ======================================================================

    void conio_renderer::push_clip(const rect& r) {
        // Get current clip rect (or full screen if stack is empty)
        rect current;
        if (m_pimpl->m_clip_stack.empty()) {
            // No current clip - use full screen
            current = rect{0, 0, m_pimpl->m_vram.get_width(), m_pimpl->m_vram.get_height()};
        } else {
            current = m_pimpl->m_clip_stack.top();
        }

        // Intersect new clip rect with current clip rect
        const int x1 = std::max(r.x, current.x);
        const int y1 = std::max(r.y, current.y);
        const int x2 = std::min(r.x + r.w, current.x + current.w);
        const int y2 = std::min(r.y + r.h, current.y + current.h);

        rect intersected;
        intersected.x = x1;
        intersected.y = y1;
        intersected.w = std::max(0, x2 - x1);
        intersected.h = std::max(0, y2 - y1);

        // Push intersected clip rect onto stack
        m_pimpl->m_clip_stack.push(intersected);
        m_pimpl->m_vram.set_clip(intersected);
    }

    void conio_renderer::pop_clip() {
        // Pop current clip rect from stack
        if (!m_pimpl->m_clip_stack.empty()) {
            m_pimpl->m_clip_stack.pop();
        }

        // Restore previous clip rect (or full screen if stack is now empty)
        if (m_pimpl->m_clip_stack.empty()) {
            // Reset to full screen
            const rect full{0, 0, m_pimpl->m_vram.get_width(), m_pimpl->m_vram.get_height()};
            m_pimpl->m_vram.set_clip(full);
        } else {
            // Restore previous clip rect
            m_pimpl->m_vram.set_clip(m_pimpl->m_clip_stack.top());
        }
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

        return rect{0, 0, w, h};
    }

    // ======================================================================
    // Screenshot
    // ======================================================================

    void conio_renderer::take_screenshot(std::ostream& sink) const {
        m_pimpl->m_vram.take_screenshot(sink);
    }
}
