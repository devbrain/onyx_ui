//
// Created by igor on 12/10/2025.
//
// Norton Utilities 8-style UI demo using drawing_utils abstraction

#include "vram.hh"
#include "src/drawing_utils.hh"
#include "include/onyxui/conio/colors.hh"
#include "rect.hh"
#include "src/dos_chars.h"
#include <time.h>
#include <unistd.h>
#include <string>
#include <cstring>
#include <vector>
#include <algorithm>

using namespace onyxui::conio;

// Use the Norton Utilities theme
static const auto& g_theme = color_schemes::norton_utilities;

// ---- Helper Functions ----

static void fill_rect_solid(drawing_context& ctx, int x, int y, int w, int h, color fg, color bg) {
    for (int j = 0; j < h; ++j) {
        for (int i = 0; i < w; ++i) {
            ctx.draw_text(x + i, y + j, " ", fg, bg);
        }
    }
}

static void draw_title_bar(drawing_context& ctx, int x, int y, int w,
                           const std::string& title, color fg, color bg) {
    // Fill background
    fill_rect_solid(ctx, x, y, w, 1, fg, bg);

    // Center title
    int title_len = static_cast<int>(title.length());
    int tx = x + (w - title_len) / 2;
    if (tx < x) tx = x;
    ctx.draw_text(tx, y, title, fg, bg);
}

static void draw_shadow_inset(vram* v, const rect& bounds, const rect& clip_bounds) {
    constexpr int SH = DOS_BLOCK_FULL; // █

    int cx0 = clip_bounds.x;
    int cy0 = clip_bounds.y;
    int cx1 = clip_bounds.x + clip_bounds.w - 1;
    int cy1 = clip_bounds.y + clip_bounds.h - 1;

    // Right shadow
    int sx = bounds.x + bounds.w;
    int sy = bounds.y + 1;
    int sh = bounds.h - 1;

    if (sx >= cx0 && sx <= cx1) {
        for (int yy = sy; yy < sy + sh && yy <= cy1; ++yy) {
            v->put(sx, yy, SH, g_theme.shadow, g_theme.window_bg);
        }
    }

    // Bottom shadow
    int bx = bounds.x + 1;
    int by = bounds.y + bounds.h;
    int bw = bounds.w - 1;

    if (by >= cy0 && by <= cy1) {
        for (int xx = bx; xx < bx + bw && xx <= cx1; ++xx) {
            v->put(xx, by, SH, g_theme.shadow, g_theme.window_bg);
        }
    }
}

// ---- UI Content ----
static const char* LEFT_ITEMS[] = {
    " Disk Doctor", " Speed Disk", " UnErase", " System Information",
    " Norton Commander", " WipeInfo", " Disk Editor", " NDOS Shell",
    " File Find", " SmartCan"
};

static void draw_shell(vram& v, drawing_context& ctx) {
    int W = v.get_width();
    int H = v.get_height();

    if (W < 80 || H < 25) {
        ctx.draw_text(2, 2, "Enlarge terminal to at least 80x25.",
                     g_theme.text_fg, g_theme.window_bg);
        return;
    }

    // Caption (gray) - use window title colors
    draw_title_bar(ctx, 0, 0, W, "The Norton Utilities 8.0",
                  g_theme.window_title_fg, g_theme.window_title_bg);

    // Menu bar with items
    std::vector<drawing_context::menu_item> menu_items = {
        {"Menu", true, false, 0},
        {"Configuration", true, false, 0},
        {"Help", true, true, 0}  // "Help" is selected/highlighted
    };
    ctx.draw_menu_bar(1, menu_items, g_theme.menu_fg, g_theme.menu_bg,
                     g_theme.menu_selected_fg, g_theme.menu_selected_bg);

    // Main frame
    int outer_x = 2, outer_y = 3;
    int outer_w = W - 4, outer_h = H - 5;
    rect outer_rect(outer_x, outer_y, outer_w, outer_h);
    ctx.draw_box(outer_rect, g_theme.window_frame_fg, g_theme.window_bg, box_style::single);

    rect clip_rect(outer_x + 1, outer_y + 1, outer_w - 2, outer_h - 2);

    // Left list box
    int left_x = outer_x + 2, left_y = outer_y + 2;
    int left_w = 22, left_h = outer_h - 6;
    rect left_frame(left_x - 1, left_y - 1, left_w + 2, left_h + 2);

    ctx.draw_box(left_frame, g_theme.window_frame_fg, g_theme.window_bg, box_style::single);
    fill_rect_solid(ctx, left_x, left_y, left_w, left_h, g_theme.text_fg, g_theme.window_bg);
    draw_shadow_inset(&v, left_frame, clip_rect);

    // Draw list items
    for (int i = 0; i < 10 && i < left_h; ++i) {
        bool selected = (i == 0);
        ctx.draw_list_item(left_x, left_y + i, left_w, LEFT_ITEMS[i],
                          selected, selected,
                          g_theme.text_fg, g_theme.window_bg,
                          g_theme.text_selected_fg, g_theme.text_selected_bg);
    }

    // Right window (workspace) - use cyan workspace color
    int work_x = left_x + left_w + 3;
    int work_y = outer_y + 2;
    int work_w = outer_x + outer_w - 3 - work_x;
    int work_h = outer_y + outer_h - 3 - work_y;
    rect work_frame(work_x - 1, work_y - 1, work_w + 2, work_h + 2);

    // Custom cyan workspace background
    color workspace_bg(0x00, 0xC0, 0xC0);
    ctx.draw_window(work_frame, " System Information ",
                   g_theme.window_frame_fg, g_theme.window_bg,  // frame colors
                   g_theme.shadow, g_theme.text_fg,              // title colors
                   workspace_bg,                             // fill color
                   box_style::single);

    draw_shadow_inset(&v, work_frame, clip_rect);

    // Example blocks inside workspace
    int bx = work_x + 2, by = work_y + 2;
    int bw = work_w - 4, bh = 5;

    // "Registered To" block
    rect reg_block(bx, by, bw, bh);
    ctx.draw_box(reg_block, g_theme.shadow, workspace_bg, box_style::single);
    color red_header(0xFF, 0x30, 0x30);
    ctx.draw_text(bx + 2, by, " Registered To ", red_header, workspace_bg);
    ctx.draw_text(bx + 2, by + 2, "User", g_theme.shadow, workspace_bg);
    ctx.draw_text(bx + 2, by + 3, "Company", g_theme.shadow, workspace_bg);

    // "System Info" block
    int by2 = by + bh + 2;
    rect sys_block(bx, by2, bw, 7);
    ctx.draw_box(sys_block, g_theme.shadow, workspace_bg, box_style::single);
    ctx.draw_text(bx + 2, by2, " System Info ", red_header, workspace_bg);

    ctx.draw_text(bx + 2, by2 + 2, "DOS Version:", g_theme.shadow, workspace_bg);
    ctx.draw_text(bx + 18, by2 + 2, "6.22", red_header, workspace_bg);

    ctx.draw_text(bx + 2, by2 + 3, "Free Conventional Memory:", g_theme.shadow, workspace_bg);
    ctx.draw_text(bx + 28, by2 + 3, "556 K", red_header, workspace_bg);

    ctx.draw_text(bx + 2, by2 + 4, "Free Extended Memory:", g_theme.shadow, workspace_bg);
    ctx.draw_text(bx + 28, by2 + 4, "65,535 K", red_header, workspace_bg);

    ctx.draw_text(bx + 2, by2 + 5, "Free Expanded Memory:", g_theme.shadow, workspace_bg);
    ctx.draw_text(bx + 28, by2 + 5, "0 K", red_header, workspace_bg);

    // Status bar
    ctx.draw_status_bar(H - 1, "F1 Help   Alt-X Exit", "",
                       g_theme.status_fg, g_theme.status_bg);
}

int main() {
    try {
        // Create VRAM instance
        vram v;
        drawing_context ctx(&v);

        // Draw the shell
        draw_shell(v, ctx);

        // Show for ~10 seconds with live clock
        for (int i = 0; i < 10; ++i) {
            // Update clock
            time_t now = time(nullptr);
            struct tm* lt = localtime(&now);
            char buf[32];
            strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", lt);

            int W = v.get_width();
            int x = std::max(1, W - static_cast<int>(strlen(buf)) - 2);
            ctx.draw_text(x, 0, buf, g_theme.window_title_fg, g_theme.window_title_bg);

            // Present to screen
            v.present();

            // Sleep for 1 second
            usleep(1000000);
        }

        return 0;

    } catch (const std::exception&) {
        // Error handling
        return 1;
    }
}
