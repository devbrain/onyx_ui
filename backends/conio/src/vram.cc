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
#include <iostream>

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
        uint32_t attr {0};  ///< Text attributes (termbox2 format)
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

            // Enable mouse input (if supported by terminal)
            tb_set_input_mode(TB_INPUT_ESC | TB_INPUT_MOUSE);

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

        /**
         * @brief Convert text_attribute flags to termbox2 attribute flags
         */
        static uint32_t to_tb_attr(text_attribute attr) noexcept {
            uint32_t result = 0;
            if ((attr & text_attribute::bold) != text_attribute::none) {
                result |= TB_BOLD;
            }
            if ((attr & text_attribute::underline) != text_attribute::none) {
                result |= TB_UNDERLINE;
            }
            if ((attr & text_attribute::reverse) != text_attribute::none) {
                result |= TB_REVERSE;
            }
            if ((attr & text_attribute::italic) != text_attribute::none) {
                result |= TB_ITALIC;
            }
            return result;
        }
    }

    struct vram::impl {
        impl();
        ~impl();

        bool clip(int x, int y) const;

        cell& get_cell(int x, int y);

        void draw() const;

        void resize();

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

        // DEBUG: Show what termbox returned
        std::cerr << "VRAM::init(): termbox returned width=" << w << " height=" << h << std::endl;
        std::cerr.flush();

        if (w <= 0 || w > 1000) {
            throw std::runtime_error("illegal terminal width");
        }
        if (h <= 0 || h > 1000) {
            throw std::runtime_error("illegal terminal height");
        }
        m_cells.resize(w * h);
        m_width = w;
        m_height = h;
        m_clip_rect.x = 0;
        m_clip_rect.y = 0;
        m_clip_rect.w = w;
        m_clip_rect.h = h;

        // DEBUG: Show initialized clip rect
        std::cerr << "VRAM::init(): clip_rect = {x=" << m_clip_rect.x
                  << ", y=" << m_clip_rect.y
                  << ", w=" << m_clip_rect.w
                  << ", h=" << m_clip_rect.h << "}" << std::endl;
        std::cerr.flush();
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
            std::cerr << "VRAM::get_cell() OUT OF RANGE!"
                      << " x=" << x << " y=" << y
                      << " vram_size=" << m_width << "x" << m_height << std::endl;
            std::cerr << "  Clip rect: {x=" << m_clip_rect.x << ", y=" << m_clip_rect.y
                      << ", w=" << m_clip_rect.w << ", h=" << m_clip_rect.h << "}" << std::endl;
            std::cerr.flush();
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
                // OR attributes with foreground color for termbox2
                tb_set_cell(x, y, c.ch, c.fg | c.attr, c.bg);
            }
        }
    }

    void vram::impl::resize() {
        // Get new dimensions from termbox2
        auto w = tb_width();
        auto h = tb_height();

        // Validate dimensions
        if (w <= 0 || w > 1000) {
            throw std::runtime_error("illegal terminal width");
        }
        if (h <= 0 || h > 1000) {
            throw std::runtime_error("illegal terminal height");
        }

        // Reallocate buffer if dimensions changed
        if (w != m_width || h != m_height) {
            // Clear termbox2 screen to prevent visual artifacts from old content
            tb_clear();

            // Reallocate internal buffer
            m_cells.clear();
            m_cells.resize(w * h);
            m_width = w;
            m_height = h;
            m_clip_rect.x = 0;
            m_clip_rect.y = 0;
            m_clip_rect.w = w;
            m_clip_rect.h = h;
            m_dirty = true;  // Force redraw after resize
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

    void vram::put(int x, int y, int ch, color fg, color bg, text_attribute attr) {
        // DEBUG: Check coordinates before clipping
        auto y_unsigned = static_cast<unsigned int>(y);
        if (y < 0 || y > 1000 || y_unsigned > 1000) {
            std::cerr << "VRAM::put() called with BAD y=" << y
                      << " (unsigned: " << y_unsigned << ") x=" << x
                      << " char='" << static_cast<char>(ch) << "' (" << ch << ")"
                      << " vram_size=" << m_pimpl->m_width << "x" << m_pimpl->m_height << std::endl;
            std::cerr.flush();
        }

        if (!m_pimpl->clip(x, y)) {
            return;
        }
        auto& c = m_pimpl->get_cell(x, y);

        c.ch = ch;
        c.fg = m_pimpl->m_mapper_fn(fg.r, fg.g, fg.b);
        c.bg = m_pimpl->m_mapper_fn(bg.r, bg.g, bg.b);
        c.attr = detail::to_tb_attr(attr);

        m_pimpl->m_dirty = true;
    }

    int vram::get_width() const {
        // Return our internal width, not tb_width()
        // tb_width() returns the CURRENT terminal size which may have changed
        // before resize() was called, causing a race condition
        return m_pimpl->m_width;
    }

    int vram::get_height() const {
        // Return our internal height, not tb_height()
        // tb_height() returns the CURRENT terminal size which may have changed
        // before resize() was called, causing a race condition
        return m_pimpl->m_height;
    }

    void vram::resize() {
        m_pimpl->resize();
    }

    void vram::present() {
        if (m_pimpl->m_dirty) {
            m_pimpl->draw();

            tb_present();
            m_pimpl->m_dirty = false;
        }
    }
}
