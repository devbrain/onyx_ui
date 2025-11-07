//
// Created by igor on 13/10/2025.
//
#include "onyxui/conio/geometry.hh"
#ifdef _WIN32
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#include <cstdint>
#include <vector>
#include <stdexcept>
#include <cstdlib>
#include <cstring>

#include "vram.hh"

#define TB_IMPL
#define TB_OPT_ATTR_W 32
#include <onyxui/conio/termbox2_wrappers.hh>

namespace onyxui::conio {

    using color_t = uint32_t;

    static constexpr std::uint32_t rgb24(std::uint8_t r, std::uint8_t g, std::uint8_t b) noexcept {
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

    enum colorcap_t : uint8_t {
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
                return static_cast<uint32_t>(((r - 8) / 10) + 232);
            }
            return static_cast<uint32_t>(16 + 36 * (r / 51) + 6 * (g / 51) + (b / 51));
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

        if (w <= 0 || w > 1000) {
            throw std::runtime_error("illegal terminal width");
        }
        if (h <= 0 || h > 1000) {
            throw std::runtime_error("illegal terminal height");
        }
        m_cells.resize(static_cast<std::size_t>(w) * static_cast<std::size_t>(h));
        m_width = w;
        m_height = h;
        m_clip_rect.x = 0;
        m_clip_rect.y = 0;
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
                // OR attributes with foreground color for termbox2
                tb_set_cell(x, y, static_cast<uint32_t>(c.ch), c.fg | c.attr, c.bg);
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
            m_cells.resize(static_cast<std::size_t>(w) * static_cast<std::size_t>(h));
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

    void vram::darken_region(const rect& shadow_rect, float factor) {
        // Helper to extract RGB from mapped color (works for all color modes)
        auto extract_rgb = [](uint32_t mapped) -> color {
            // For truecolor mode: rgb24(r,g,b) = (r<<16)|(g<<8)|b
            // For 256/ANSI modes: approximate back to RGB (lossy but good enough)
            uint8_t r = static_cast<uint8_t>((mapped >> 16) & 0xFF);
            uint8_t g = static_cast<uint8_t>((mapped >> 8) & 0xFF);
            uint8_t b = static_cast<uint8_t>(mapped & 0xFF);
            return color{r, g, b};
        };

        // Darken each cell in the shadow region
        for (int y = shadow_rect.y; y < shadow_rect.y + shadow_rect.h; ++y) {
            for (int x = shadow_rect.x; x < shadow_rect.x + shadow_rect.w; ++x) {
                if (!m_pimpl->clip(x, y)) {
                    continue;  // Respect clipping
                }

                auto& c = m_pimpl->get_cell(x, y);

                // Extract current background RGB
                color bg = extract_rgb(c.bg);

                // Darken by multiplying each component
                uint8_t darkened_r = static_cast<uint8_t>(static_cast<float>(bg.r) * factor);
                uint8_t darkened_g = static_cast<uint8_t>(static_cast<float>(bg.g) * factor);
                uint8_t darkened_b = static_cast<uint8_t>(static_cast<float>(bg.b) * factor);

                // Re-map to current color mode
                c.bg = m_pimpl->m_mapper_fn(darkened_r, darkened_g, darkened_b);
            }
        }

        m_pimpl->m_dirty = true;
    }

    void vram::lighten_region(const rect& highlight_rect, float factor) {
        // Helper to extract RGB from mapped color (works for all color modes)
        auto extract_rgb = [](uint32_t mapped) -> color {
            // For truecolor mode: rgb24(r,g,b) = (r<<16)|(g<<8)|b
            // For 256/ANSI modes: approximate back to RGB (lossy but good enough)
            uint8_t r = static_cast<uint8_t>((mapped >> 16) & 0xFF);
            uint8_t g = static_cast<uint8_t>((mapped >> 8) & 0xFF);
            uint8_t b = static_cast<uint8_t>(mapped & 0xFF);
            return color{r, g, b};
        };

        // Lighten each cell in the highlight region
        for (int y = highlight_rect.y; y < highlight_rect.y + highlight_rect.h; ++y) {
            for (int x = highlight_rect.x; x < highlight_rect.x + highlight_rect.w; ++x) {
                if (!m_pimpl->clip(x, y)) {
                    continue;  // Respect clipping
                }

                auto& c = m_pimpl->get_cell(x, y);

                // Extract current background RGB
                color bg = extract_rgb(c.bg);

                // Lighten by multiplying each component (clamp to 255)
                auto lighten_component = [factor](uint8_t component) -> uint8_t {
                    float lightened = static_cast<float>(component) * factor;
                    return static_cast<uint8_t>(std::min(255.0f, lightened));
                };

                uint8_t lightened_r = lighten_component(bg.r);
                uint8_t lightened_g = lighten_component(bg.g);
                uint8_t lightened_b = lighten_component(bg.b);

                // Re-map to current color mode
                c.bg = m_pimpl->m_mapper_fn(lightened_r, lightened_g, lightened_b);
            }
        }

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

    void vram::take_screenshot(std::ostream& sink) const {
        // Output plain text representation of the vram buffer
        std::size_t idx = 0;
        for (int y = 0; y < m_pimpl->m_height; ++y) {
            for (int x = 0; x < m_pimpl->m_width; ++x) {
                const auto& c = m_pimpl->m_cells[idx++];
                // Convert Unicode code point to UTF-8
                if (c.ch < 0x80) {
                    // ASCII character (1 byte)
                    sink.put(static_cast<char>(c.ch));
                } else if (c.ch < 0x800) {
                    // 2-byte UTF-8
                    sink.put(static_cast<char>(0xC0 | (c.ch >> 6)));
                    sink.put(static_cast<char>(0x80 | (c.ch & 0x3F)));
                } else if (c.ch < 0x10000) {
                    // 3-byte UTF-8
                    sink.put(static_cast<char>(0xE0 | (c.ch >> 12)));
                    sink.put(static_cast<char>(0x80 | ((c.ch >> 6) & 0x3F)));
                    sink.put(static_cast<char>(0x80 | (c.ch & 0x3F)));
                } else {
                    // 4-byte UTF-8
                    sink.put(static_cast<char>(0xF0 | (c.ch >> 18)));
                    sink.put(static_cast<char>(0x80 | ((c.ch >> 12) & 0x3F)));
                    sink.put(static_cast<char>(0x80 | ((c.ch >> 6) & 0x3F)));
                    sink.put(static_cast<char>(0x80 | (c.ch & 0x3F)));
                }
            }
            sink.put('\n');
        }
    }
}
