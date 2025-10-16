//
// Created by igor on 14/10/2025.
//

#include "conio_app.hh"
#include "drawing_utils.hh"
#include <termbox2.h>
#include <chrono>

namespace onyxui::conio {

    // ======================================================================
    // conio_app implementation
    // ======================================================================

    conio_app::conio_app()
        : m_vram(nullptr),
          m_renderer(nullptr),
          m_root(nullptr),
          m_running(false),
          m_needs_redraw(true),
          m_width(0),
          m_height(0) {
    }

    conio_app::~conio_app() {
        if (m_running) {
            tb_shutdown();
        }
    }

    bool conio_app::init(int width, int height) {
        // Initialize termbox2
        int ret = tb_init();
        if (ret != 0) {
            return false;
        }

        // Enable mouse support
        tb_set_input_mode(TB_INPUT_ESC | TB_INPUT_MOUSE);
        tb_set_output_mode(TB_OUTPUT_TRUECOLOR);

        // Get actual terminal dimensions
        m_width = (width == 0) ? tb_width() : width;
        m_height = (height == 0) ? tb_height() : height;

        // Create VRAM and renderer
        m_vram = std::make_shared<vram>(m_width, m_height);
        m_renderer = std::make_unique<conio_renderer>(m_vram.get());

        // Create root element with full screen bounds
        m_root = std::make_unique<ui_element<conio_backend>>();
        m_root->set_bounds(rect(0, 0, m_width, m_height));

        // Call user initialization
        on_init();

        return true;
    }

    int conio_app::run() {
        if (!m_vram || !m_renderer) {
            return -1;
        }

        m_running = true;
        auto last_time = std::chrono::steady_clock::now();

        while (m_running) {
            // Calculate delta time
            auto current_time = std::chrono::steady_clock::now();
            float delta_time = std::chrono::duration<float>(current_time - last_time).count();
            last_time = current_time;

            // Process events
            process_events();

            // Update logic
            on_update(delta_time);

            // Render if needed
            if (m_needs_redraw) {
                render();
                m_needs_redraw = false;
            }

            // Small sleep to avoid busy waiting
            tb_set_cursor(-1, -1);
            tb_present();
        }

        tb_shutdown();
        return 0;
    }

    void conio_app::process_events() {
        tb_event event;

        // Poll for events (non-blocking)
        while (tb_peek_event(&event, 0) == TB_OK) {
            // Give user a chance to handle the event first
            if (on_event(event)) {
                continue;
            }

            // Handle built-in events
            switch (event.type) {
                case TB_EVENT_KEY:
                    // ESC or Ctrl+C to quit
                    if (event.key == TB_KEY_ESC ||
                        (event.key == TB_KEY_CTRL_C)) {
                        m_running = false;
                    }
                    m_needs_redraw = true;
                    break;

                case TB_EVENT_RESIZE:
                    handle_resize();
                    break;

                case TB_EVENT_MOUSE:
                    m_needs_redraw = true;
                    break;

                default:
                    break;
            }

            // Propagate event to UI tree
            if (m_root) {
                // TODO: Implement proper event routing through the UI tree
            }
        }
    }

    void conio_app::render() {
        // Clear VRAM
        m_vram->clear(color(0, 0, 0), color(0, 0, 0));

        // Render UI tree
        if (m_root) {
            // Ensure layout is up to date
            m_root->measure(m_width, m_height);
            m_root->arrange(rect(0, 0, m_width, m_height));

            // TODO: Render the UI tree
            // m_root->render(m_renderer.get());
        }

        // Give user a chance to do custom rendering
        on_render(m_renderer.get());

        // Flush VRAM to terminal
        m_vram->present();
    }

    void conio_app::handle_resize() {
        m_width = tb_width();
        m_height = tb_height();

        // Resize VRAM
        m_vram->resize(m_width, m_height);

        // Update root element bounds
        if (m_root) {
            m_root->set_bounds(rect(0, 0, m_width, m_height));
        }

        m_needs_redraw = true;
    }

    // ======================================================================
    // conio_window implementation
    // ======================================================================

    conio_window::conio_window(ui_element<conio_backend>* parent)
        : ui_element<conio_backend>(parent),
          m_title(),
          m_frame_style(box_style::single),
          m_title_fg(255, 255, 255),
          m_title_bg(0, 0, 128),
          m_frame_fg(192, 192, 192),
          m_frame_bg(0, 0, 0),
          m_background(0, 0, 64) {
    }

    void conio_window::on_render(conio_renderer* renderer) {
        if (!renderer) return;

        drawing_context ctx(renderer->vram());
        const auto& bounds = get_bounds();

        // Draw filled background
        ctx.draw_filled_box(bounds, m_frame_fg, m_background, m_frame_style);

        // Draw title if present
        if (!m_title.empty()) {
            // Draw title bar background
            rect title_rect(bounds.x, bounds.y, bounds.w, 1);
            ctx.draw_filled_box(title_rect, m_title_fg, m_title_bg, box_style::single);

            // Draw title text centered
            ctx.draw_text_centered(title_rect, m_title, m_title_fg, m_title_bg);
        }
    }

    // ======================================================================
    // conio_label implementation
    // ======================================================================

    conio_label::conio_label(ui_element<conio_backend>* parent)
        : ui_element<conio_backend>(parent),
          m_text(),
          m_fg(255, 255, 255),
          m_bg(0, 0, 0),
          m_alignment(0) {
    }

    size conio_label::measure_content(int available_width, int available_height) {
        // Measure text dimensions
        int text_width = static_cast<int>(m_text.length());
        int text_height = 1;

        // Account for word wrapping if needed
        if (available_width > 0 && text_width > available_width) {
            text_height = (text_width + available_width - 1) / available_width;
            text_width = available_width;
        }

        return size(text_width, text_height);
    }

    void conio_label::on_render(conio_renderer* renderer) {
        if (!renderer || m_text.empty()) return;

        drawing_context ctx(renderer->vram());
        const auto& bounds = get_bounds();

        // Draw background
        ctx.draw_filled_box(bounds, m_fg, m_bg, box_style::single);

        // Draw text based on alignment
        switch (m_alignment) {
            case 0: // Left
                ctx.draw_text(bounds.x, bounds.y, m_text, m_fg, m_bg);
                break;
            case 1: // Center
                ctx.draw_text_centered(bounds, m_text, m_fg, m_bg);
                break;
            case 2: { // Right
                int x = bounds.x + bounds.w - static_cast<int>(m_text.length());
                ctx.draw_text(x, bounds.y, m_text, m_fg, m_bg);
                break;
            }
        }
    }

    // ======================================================================
    // conio_button implementation
    // ======================================================================

    conio_button::conio_button(ui_element<conio_backend>* parent)
        : ui_element<conio_backend>(parent),
          m_text(),
          m_enabled(true),
          m_pressed(false),
          m_hover(false) {
    }

    size conio_button::measure_content(int available_width, int available_height) {
        // Button needs space for text + border
        int width = static_cast<int>(m_text.length()) + 4;  // 2 chars padding on each side
        int height = 3;  // Top border, text, bottom border

        return size(width, height);
    }

    void conio_button::on_render(conio_renderer* renderer) {
        if (!renderer) return;

        drawing_context ctx(renderer->vram());
        const auto& bounds = get_bounds();

        // Choose colors based on state
        color fg, bg;
        if (!m_enabled) {
            fg = color(128, 128, 128);
            bg = color(64, 64, 64);
        } else if (m_pressed) {
            fg = color(0, 0, 0);
            bg = color(255, 255, 0);
        } else if (m_hover) {
            fg = color(255, 255, 255);
            bg = color(0, 128, 255);
        } else {
            fg = color(0, 0, 0);
            bg = color(192, 192, 192);
        }

        // Draw button box
        ctx.draw_filled_box(bounds, fg, bg, box_style::single);

        // Draw text centered
        ctx.draw_text_centered(bounds, m_text, fg, bg);
    }

    bool conio_button::on_mouse_button(const tb_event& event) {
        if (!m_enabled) return false;

        const auto& bounds = get_bounds();

        // Check if click is within bounds
        if (event.x >= bounds.x && event.x < bounds.x + bounds.w &&
            event.y >= bounds.y && event.y < bounds.y + bounds.h) {

            if (event.key == TB_KEY_MOUSE_LEFT) {
                m_pressed = true;
                invalidate_arrange();

                // Trigger click callback on release
                if (on_click) {
                    on_click();
                }
                return true;
            }
        } else {
            m_pressed = false;
        }

        return false;
    }

    bool conio_button::on_keyboard(const tb_event& event) {
        if (!m_enabled) return false;

        // Activate on Enter or Space when focused
        if (event.key == TB_KEY_ENTER || event.key == TB_KEY_SPACE) {
            if (on_click) {
                on_click();
            }
            return true;
        }

        return false;
    }

    // ======================================================================
    // conio_listbox implementation
    // ======================================================================

    conio_listbox::conio_listbox(ui_element<conio_backend>* parent)
        : ui_element<conio_backend>(parent),
          m_items(),
          m_selected_index(-1),
          m_scroll_offset(0),
          m_visible_items(0) {
    }

    void conio_listbox::add_item(const std::string& item) {
        m_items.push_back(item);
        invalidate_measure();
    }

    void conio_listbox::remove_item(int index) {
        if (index >= 0 && index < static_cast<int>(m_items.size())) {
            m_items.erase(m_items.begin() + index);

            // Adjust selection if needed
            if (m_selected_index >= static_cast<int>(m_items.size())) {
                m_selected_index = static_cast<int>(m_items.size()) - 1;
            }

            invalidate_measure();
        }
    }

    void conio_listbox::clear_items() {
        m_items.clear();
        m_selected_index = -1;
        m_scroll_offset = 0;
        invalidate_measure();
    }

    void conio_listbox::set_selected_index(int index) {
        if (index >= -1 && index < static_cast<int>(m_items.size())) {
            m_selected_index = index;

            // Adjust scroll to keep selection visible
            if (m_selected_index >= 0) {
                if (m_selected_index < m_scroll_offset) {
                    m_scroll_offset = m_selected_index;
                } else if (m_selected_index >= m_scroll_offset + m_visible_items) {
                    m_scroll_offset = m_selected_index - m_visible_items + 1;
                }
            }

            invalidate_arrange();

            // Trigger callback
            if (on_selection_changed) {
                on_selection_changed(m_selected_index);
            }
        }
    }

    size conio_listbox::measure_content(int available_width, int available_height) {
        // Calculate maximum item width
        int max_width = 0;
        for (const auto& item : m_items) {
            max_width = std::max(max_width, static_cast<int>(item.length()));
        }

        // Height is number of items (capped by available height if specified)
        int height = static_cast<int>(m_items.size());
        if (available_height > 0) {
            height = std::min(height, available_height);
        }

        return size(max_width, height);
    }

    void conio_listbox::on_render(conio_renderer* renderer) {
        if (!renderer) return;

        drawing_context ctx(renderer->vram());
        const auto& bounds = get_bounds();

        m_visible_items = bounds.h;

        // Draw each visible item
        for (int i = 0; i < m_visible_items && (m_scroll_offset + i) < static_cast<int>(m_items.size()); ++i) {
            int item_index = m_scroll_offset + i;
            const auto& item = m_items[item_index];

            color fg, bg;
            if (item_index == m_selected_index) {
                fg = color(255, 255, 255);
                bg = color(0, 0, 128);
            } else {
                fg = color(192, 192, 192);
                bg = color(0, 0, 0);
            }

            int y = bounds.y + i;

            // Draw background for this line
            rect line_rect(bounds.x, y, bounds.w, 1);
            ctx.draw_filled_box(line_rect, fg, bg, box_style::single);

            // Draw item text
            ctx.draw_text(bounds.x, y, item, fg, bg);
        }
    }

    bool conio_listbox::on_mouse_button(const tb_event& event) {
        const auto& bounds = get_bounds();

        // Check if click is within bounds
        if (event.x >= bounds.x && event.x < bounds.x + bounds.w &&
            event.y >= bounds.y && event.y < bounds.y + bounds.h) {

            if (event.key == TB_KEY_MOUSE_LEFT) {
                // Calculate which item was clicked
                int clicked_item = m_scroll_offset + (event.y - bounds.y);
                if (clicked_item >= 0 && clicked_item < static_cast<int>(m_items.size())) {
                    set_selected_index(clicked_item);
                }
                return true;
            }
        }

        return false;
    }

    bool conio_listbox::on_keyboard(const tb_event& event) {
        if (m_items.empty()) return false;

        bool handled = false;

        switch (event.key) {
            case TB_KEY_ARROW_UP:
                if (m_selected_index > 0) {
                    set_selected_index(m_selected_index - 1);
                    handled = true;
                }
                break;

            case TB_KEY_ARROW_DOWN:
                if (m_selected_index < static_cast<int>(m_items.size()) - 1) {
                    set_selected_index(m_selected_index + 1);
                    handled = true;
                }
                break;

            case TB_KEY_HOME:
                set_selected_index(0);
                handled = true;
                break;

            case TB_KEY_END:
                set_selected_index(static_cast<int>(m_items.size()) - 1);
                handled = true;
                break;

            case TB_KEY_PGUP:
                if (m_selected_index > 0) {
                    set_selected_index(std::max(0, m_selected_index - m_visible_items));
                    handled = true;
                }
                break;

            case TB_KEY_PGDN:
                if (m_selected_index < static_cast<int>(m_items.size()) - 1) {
                    set_selected_index(std::min(static_cast<int>(m_items.size()) - 1,
                                                m_selected_index + m_visible_items));
                    handled = true;
                }
                break;
        }

        return handled;
    }

} // namespace onyxui::conio
