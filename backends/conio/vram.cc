//
// Created by igor on 13/10/2025.
//
#ifdef _WIN32
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#include <vector>
#include <stdexcept>
#include <cstdlib>
#include <cstring>

#include "vram.hh"

#define TB_IMPL
#define TB_OPT_ATTR_W 32
#include <termbox2.h>

namespace onyxui::conio {

    using color_t = uint32_t;

    constexpr std::uint32_t rgb24(std::uint8_t r, std::uint8_t g, std::uint8_t b) noexcept {
        return (static_cast <std::uint32_t>(r) << 16) |
               (static_cast <std::uint32_t>(g) << 8) |
               (static_cast <std::uint32_t>(b));
    }

    struct cell {
        int ch {' '};
        color_t fg {rgb24(0,0,0)};
        color_t bg {rgb24(0,0,0)};
    };

    enum colorcap_t {
        COLORCAP_TRUECOLOR,
        COLORCAP_256,
        COLORCAP_ANSI
    };

    using color_mapper_fn = std::uint32_t(*)(std::uint8_t, std::uint8_t, std::uint8_t);

    namespace detail {
        static colorcap_t detect_color_capability() {
            const char* colorterm = getenv("COLORTERM");
            const char* term = getenv("TERM");

            if (colorterm && strstr(colorterm, "truecolor"))
                return COLORCAP_TRUECOLOR;
            if (term && (strstr(term, "direct") || strstr(term, "truecolor")))
                return COLORCAP_TRUECOLOR;
            if (term && strstr(term, "256color"))
                return COLORCAP_256;
            return COLORCAP_ANSI;
        }

        static uint32_t rgb_to_xterm256(uint8_t r, uint8_t g, uint8_t b) {
            if ((r == g && g == b)) {
                if (r < 8) return 16;
                if (r > 248) return 231;
                return ((r - 8) / 10) + 232;
            }
            return 16 + 36 * (r / 51) + 6 * (g / 51) + (b / 51);
        }

        static uint32_t rgb_to_truecolor(uint8_t r, uint8_t g, uint8_t b) {
            return rgb24(r, g, b);
        }

        static uint32_t rgb_to_ansi(uint8_t r, uint8_t g, uint8_t b) {
            if (r > 128 && g > 128 && b > 128) return TB_WHITE;
            if (r > 128 && g > 128 && b < 128) return TB_YELLOW;
            if (r > 128 && g < 128 && b < 128) return TB_RED;
            if (r < 128 && g > 128 && b < 128) return TB_GREEN;
            if (r < 128 && g < 128 && b > 128) return TB_BLUE;
            if (r < 128 && g > 128 && b > 128) return TB_CYAN;
            if (r > 128 && g < 128 && b > 128) return TB_MAGENTA;
            return TB_BLACK;
        }

        static color_mapper_fn init_termbox() {
            if (TB_OK != tb_init()) {
                throw std::runtime_error("Failed to initialize console");
            }
#ifdef _WIN32
            SetConsoleOutputCP(65001);
            SetConsoleCP(65001);
#endif

            color_mapper_fn mapper{};

            switch (detect_color_capability()) {
                case COLORCAP_TRUECOLOR:
                    mapper = rgb_to_truecolor;
                    tb_set_output_mode(TB_OUTPUT_TRUECOLOR);
                    break;
                case COLORCAP_256:
                    tb_set_output_mode(TB_OUTPUT_256);
                    mapper = rgb_to_xterm256;
                    break;
                case COLORCAP_ANSI:
                    tb_set_output_mode(TB_OUTPUT_NORMAL);
                    mapper = rgb_to_ansi;
                    break;
                default:
                    throw std::runtime_error("unknown color spec.");
            }
            return mapper;
        }

        static void fini_termbox() {
            tb_shutdown();
        }
    }

    struct vram::impl {
        impl();
        ~impl();

        bool clip(int x, int y) const;

        cell& get_cell(int x, int y);

        void draw() const;

        color_mapper_fn m_mapper_fn;
        std::vector <cell> m_cells;
        int m_width;
        int m_height;
        rect m_clip_rect;
        bool m_dirty;
    };

    vram::impl::impl()
        : m_mapper_fn(detail::init_termbox()),
          m_dirty(false) {
        auto w = tb_width();
        auto h = tb_height();

        if (w <= 0 || w > 1000) {
            throw std::runtime_error("illegal terminal width");
        }
        if (h <= 0 || h > 1000) {
            throw std::runtime_error("illegal terminal height");
        }
        m_cells.resize(w * h);
        m_width = w;
        m_height = h;
        m_clip_rect.w = w;
        m_clip_rect.h = h;

    }

    vram::impl::~impl() {
        detail::fini_termbox();
    }

    bool vram::impl::clip(int x, int y) const {
        if (x < m_clip_rect.x) {
            return false;
        }
        if (y < m_clip_rect.y) {
            return false;
        }
        if (x >= m_clip_rect.w + m_clip_rect.x) {
            return false;
        }
        if (y >= m_clip_rect.h + m_clip_rect.y) {
            return false;
        }
        return true;
    }

    cell& vram::impl::get_cell(int x, int y) {
        if (x < 0 || x >= m_width || y < 0 || y >= m_height) {
            throw std::runtime_error("VRAM coords are out of range");
        }
        std::size_t idx = static_cast<std::size_t>(x) + static_cast<std::size_t>(y * m_width);
        return m_cells[idx];
    }

    void vram::impl::draw() const {
        std::size_t idx = 0;
        for (int y=0; y<m_height; y++) {
            for (int x=0; x<m_width; x++) {
                const auto& c = m_cells[idx++];
                tb_set_cell(x, y, c.ch, c.fg, c.bg);
            }
        }
    }

    vram::vram()
        : m_pimpl(std::make_unique<impl>()){

    }

    vram::~vram() = default;

    void vram::set_clip(const rect& r) {
        m_pimpl->m_clip_rect = r;
    }

    rect vram::get_clip() const noexcept {
        return m_pimpl->m_clip_rect;
    }

    void vram::put(int x, int y, int ch, color fg, color bg) {
        if (!m_pimpl->clip(x, y)) {
            return;
        }
        auto& c = m_pimpl->get_cell(x, y);

        c.ch = ch;
        c.fg = m_pimpl->m_mapper_fn(fg.r, fg.g, fg.b);
        c.bg = m_pimpl->m_mapper_fn(bg.r, bg.g, bg.b);

        m_pimpl->m_dirty = true;
    }

    int vram::get_width() const {
        return tb_width();
    }

    int vram::get_height() const {
        return tb_height();
    }

    void vram::present() {
        if (m_pimpl->m_dirty) {
            m_pimpl->draw();

            tb_present();
            m_pimpl->m_dirty = false;
        }
    }
}
