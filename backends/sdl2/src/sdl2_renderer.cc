//
// SDL2 Renderer Implementation (Stub)
//

#include <onyxui/sdl2/sdl2_renderer.hh>
#include <stack>
#include <stdexcept>

namespace onyxui::sdl2 {

struct sdl2_renderer::impl {
    SDL_Renderer* sdl_renderer = nullptr;
    int width = 0;
    int height = 0;
    std::stack<rect> clip_stack;
    color fg_color{0, 0, 0};
    color bg_color{192, 192, 192};  // Windows gray

    impl(SDL_Window* window, int w, int h)
        : width(w), height(h) {
        sdl_renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
        if (!sdl_renderer) {
            throw std::runtime_error(std::string("Failed to create SDL renderer: ") + SDL_GetError());
        }
        clip_stack.push(rect{0, 0, w, h});
    }

    ~impl() {
        if (sdl_renderer) {
            SDL_DestroyRenderer(sdl_renderer);
        }
    }
};

sdl2_renderer::sdl2_renderer(SDL_Window* window, int width, int height)
    : m_pimpl(std::make_unique<impl>(window, width, height)) {
}

sdl2_renderer::~sdl2_renderer() = default;

void sdl2_renderer::draw_box(const rect& r, box_style style) {
    // TODO: Implement Windows 3.x style box drawing
    // - For raised: draw white on top/left, dark gray on bottom/right
    // - For sunken: reverse the colors
    // - For flat: just draw a single-pixel border
    SDL_SetRenderDrawColor(m_pimpl->sdl_renderer, m_pimpl->fg_color.r, m_pimpl->fg_color.g, m_pimpl->fg_color.b, 255);
    SDL_Rect sdl_rect{r.x, r.y, r.w, r.h};
    SDL_RenderDrawRect(m_pimpl->sdl_renderer, &sdl_rect);
}

void sdl2_renderer::draw_text(const rect& r, std::string_view text, const font& f) {
    // TODO: Implement TTF font rendering using SDL_ttf
    // For now, just clear the region
    (void)text;
    (void)f;
    clear_region(r);
}

void sdl2_renderer::draw_icon(const rect& r, icon_style style) {
    // TODO: Implement icon drawing
    // For now, just draw a placeholder rectangle
    (void)style;
    SDL_SetRenderDrawColor(m_pimpl->sdl_renderer, m_pimpl->fg_color.r, m_pimpl->fg_color.g, m_pimpl->fg_color.b, 255);
    SDL_Rect sdl_rect{r.x, r.y, r.w, r.h};
    SDL_RenderFillRect(m_pimpl->sdl_renderer, &sdl_rect);
}

void sdl2_renderer::clear_region(const rect& r) {
    SDL_SetRenderDrawColor(m_pimpl->sdl_renderer, m_pimpl->bg_color.r, m_pimpl->bg_color.g, m_pimpl->bg_color.b, 255);
    SDL_Rect sdl_rect{r.x, r.y, r.w, r.h};
    SDL_RenderFillRect(m_pimpl->sdl_renderer, &sdl_rect);
}

void sdl2_renderer::push_clip(const rect& r) {
    m_pimpl->clip_stack.push(r);
    SDL_Rect sdl_rect{r.x, r.y, r.w, r.h};
    SDL_RenderSetClipRect(m_pimpl->sdl_renderer, &sdl_rect);
}

void sdl2_renderer::pop_clip() {
    if (!m_pimpl->clip_stack.empty()) {
        m_pimpl->clip_stack.pop();
    }
    if (!m_pimpl->clip_stack.empty()) {
        const auto& r = m_pimpl->clip_stack.top();
        SDL_Rect sdl_rect{r.x, r.y, r.w, r.h};
        SDL_RenderSetClipRect(m_pimpl->sdl_renderer, &sdl_rect);
    }
}

rect sdl2_renderer::get_clip_rect() const {
    return m_pimpl->clip_stack.empty() ? rect{} : m_pimpl->clip_stack.top();
}

void sdl2_renderer::present() {
    SDL_RenderPresent(m_pimpl->sdl_renderer);
}

void sdl2_renderer::on_resize(int new_width, int new_height) {
    m_pimpl->width = new_width;
    m_pimpl->height = new_height;
    // Clear clip stack and reset
    while (!m_pimpl->clip_stack.empty()) {
        m_pimpl->clip_stack.pop();
    }
    m_pimpl->clip_stack.push(rect{0, 0, new_width, new_height});
}

rect sdl2_renderer::get_viewport() const {
    return rect{0, 0, m_pimpl->width, m_pimpl->height};
}

size sdl2_renderer::measure_text(std::string_view text, const font& f) {
    // TODO: Implement proper TTF text measurement
    // For now, use a simple approximation: 8px width per character, font size for height
    (void)f;
    return size{static_cast<int>(text.length() * 8), f.size};
}

void sdl2_renderer::set_foreground(const color& c) {
    m_pimpl->fg_color = c;
}

void sdl2_renderer::set_background(const color& c) {
    m_pimpl->bg_color = c;
}

color sdl2_renderer::get_foreground() const {
    return m_pimpl->fg_color;
}

color sdl2_renderer::get_background() const {
    return m_pimpl->bg_color;
}

} // namespace onyxui::sdl2
