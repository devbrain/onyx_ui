//
// Created by igor on 12/10/2025.
//

#pragma once

// Comprehensive DOS-like pseudographics enum (code page 437 → Unicode)

enum dos_char {
    // ──────────────────────────────────────
    // Single line box drawing
    DOS_H = 0x2500, // ─
    DOS_V = 0x2502, // │
    DOS_TL = 0x250C, // ┌
    DOS_TR = 0x2510, // ┐
    DOS_BL = 0x2514, // └
    DOS_BR = 0x2518, // ┘
    DOS_T = 0x252C, // ┬
    DOS_B = 0x2534, // ┴
    DOS_L = 0x251C, // ├
    DOS_R = 0x2524, // ┤
    DOS_X = 0x253C, // ┼

    // ──────────────────────────────────────
    // Double line box drawing
    DOS_H2 = 0x2550, // ═
    DOS_V2 = 0x2551, // ║
    DOS_TL2 = 0x2554, // ╔
    DOS_TR2 = 0x2557, // ╗
    DOS_BL2 = 0x255A, // ╚
    DOS_BR2 = 0x255D, // ╝
    DOS_T2 = 0x2566, // ╦
    DOS_B2 = 0x2569, // ╩
    DOS_L2 = 0x2560, // ╠
    DOS_R2 = 0x2563, // ╣
    DOS_X2 = 0x256C, // ╬

    // ──────────────────────────────────────
    // Mixed single/double junctions
    DOS_T_SD = 0x2565, // ╥ (double top, single down)
    DOS_T_DS = 0x2564, // ╤ (single top, double down)
    DOS_B_SD = 0x2568, // ╨ (double bottom, single up)
    DOS_B_DS = 0x2567, // ╧ (single bottom, double up)
    DOS_L_SD = 0x255E, // ╞ (double left, single right)
    DOS_L_DS = 0x255F, // ╟ (single left, double right)
    DOS_R_SD = 0x2561, // ╡ (double right, single left)
    DOS_R_DS = 0x2562, // ╢ (single right, double left)
    DOS_X_SD = 0x256B, // ╫ (double vertical, single horizontal)
    DOS_X_DS = 0x256A, // ╪ (single vertical, double horizontal)

    // ──────────────────────────────────────
    // Rounded box drawing (light line)
    BOX_ROUND_TL = 0x256D, // ╭
    BOX_ROUND_TR = 0x256E, // ╮
    BOX_ROUND_BL = 0x2570, // ╰
    BOX_ROUND_BR = 0x256F, // ╯
    BOX_ROUND_H = 0x2500, // ─
    BOX_ROUND_V = 0x2502, // │

    // ──────────────────────────────────────
    // Heavy rounded corners (for bold UI)
    BOX_HEAVY_ROUND_TL = 0x2552, // ╒
    BOX_HEAVY_ROUND_TR = 0x2555, // ╕
    BOX_HEAVY_ROUND_BL = 0x2558, // ╘
    BOX_HEAVY_ROUND_BR = 0x255B, // ╛
    BOX_HEAVY_H = 0x2501, // ━
    BOX_HEAVY_V = 0x2503, // ┃

    // ──────────────────────────────────────
    // Mixed weight rounded (light horiz / heavy vert)
    BOX_MIXED_TL = 0x2553, // ╓
    BOX_MIXED_TR = 0x2556, // ╖
    BOX_MIXED_BL = 0x2559, // ╙
    BOX_MIXED_BR = 0x255C, // ╜

    // ──────────────────────────────────────
    // Blocks and shading
    DOS_BLOCK_FULL = 0x2588, // █
    DOS_BLOCK_3Q = 0x2593, // ▓
    DOS_BLOCK_HALF = 0x2592, // ▒
    DOS_BLOCK_1Q = 0x2591, // ░
    DOS_BLOCK_LEFT = 0x258C, // ▌
    DOS_BLOCK_RIGHT = 0x2590, // ▐
    DOS_BLOCK_LOW = 0x2584, // ▄
    DOS_BLOCK_HIGH = 0x2580, // ▀

    // ──────────────────────────────────────
    // Arrows
    DOS_ARROW_UP = 0x2191, // ↑
    DOS_ARROW_DOWN = 0x2193, // ↓
    DOS_ARROW_LEFT = 0x2190, // ←
    DOS_ARROW_RIGHT = 0x2192, // →
    DOS_ARROW_HORIZ = 0x2194, // ↔
    DOS_ARROW_VERT = 0x2195, // ↕
    DOS_ARROW_DBLUP = 0x21D1, // ⇑
    DOS_ARROW_DBLDN = 0x21D3, // ⇓
    DOS_ARROW_DBLLT = 0x21D0, // ⇐
    DOS_ARROW_DBLRT = 0x21D2, // ⇒

    // ──────────────────────────────────────
    // Corners and diagonals
    DOS_DIAG_UL = 0x2571, // ╱
    DOS_DIAG_UR = 0x2572, // ╲
    DOS_DIAG_CROSS = 0x2573, // ╳

    // ──────────────────────────────────────
    // Misc UI symbols
    DOS_BULLET = 0x2022, // •
    DOS_DEGREE = 0x00B0, // °
    DOS_PLUSMINUS = 0x00B1, // ±
    DOS_CHECK = 0x221A, // √
    DOS_COPYRIGHT = 0x00A9, // ©
    DOS_TRADEMARK = 0x2122, // ™
    DOS_SMILEY = 0x263A, // ☺
    DOS_SMILEY_INV = 0x263B, // ☻
    DOS_HEART = 0x2665, // ♥
    DOS_DIAMOND = 0x2666, // ♦
    DOS_CLUB = 0x2663, // ♣
    DOS_SPADE = 0x2660, // ♠
    DOS_NOTE = 0x266B, // ♫
    DOS_STAR = 0x2605, // ★
    DOS_DOT = 0x00B7, // ·
    DOS_SECTION = 0x00A7, // §

    // ──────────────────────────────────────
    // Graph aids
    DOS_VERT_DOTTED = 0x250A, // ┊
    DOS_HORIZ_DOTTED = 0x2508, // ┈
    DOS_CROSS_DOTTED = 0x254B, // ┋ (approx)

    // ──────────────────────────────────────
    // Additional icons for renderer
    DOS_CROSS = 0x2717, // ✗ (ballot X)
    DOS_TRIANGLE_RIGHT = 0x25B6, // ▶ (black right-pointing triangle)
    DOS_SQUARE_FILLED = 0x25A0, // ■ (black square)

    // ──────────────────────────────────────
    // Window management icons
    DOS_MENU = 0x2261, // ≡ (hamburger menu / identical to)
    DOS_MINIMIZE = 0x2581, // ▁ (lower one eighth block)
    DOS_MAXIMIZE = 0x25A1, // □ (white square)
    DOS_RESTORE = 0x25A2, // ▢ (white square with rounded corners)
    DOS_CLOSE_X = 0x00D7, // × (multiplication sign)

    // ──────────────────────────────────────
    // Text editing cursors
    DOS_CURSOR_INSERT = 0x2502, // │ (vertical bar for insert mode)
    DOS_CURSOR_OVERWRITE = 0x2588, // █ (full block for overwrite mode)
};
