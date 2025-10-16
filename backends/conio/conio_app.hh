//
// Created by igor on 14/10/2025.
//

#pragma once

#include "conio_backend.hh"
#include "vram.hh"
#include "drawing_utils.hh"
#include <memory>
#include <onyxui/element.hh>
#include <onyxui/event_target.hh>

// Forward declare tb_event from termbox2
struct tb_event;

namespace onyxui::conio {

    /**
     * @class conio_app
     * @brief Application class for terminal UI applications
     *
     * Manages the main event loop, rendering, and UI element tree.
     */
    class conio_app {
    public:
        conio_app();
        virtual ~conio_app();

        // Initialize the application
        bool init(int width = 0, int height = 0);  // 0 = auto-detect

        // Main event loop
        int run();

        // Get the root element for UI construction
        ui_element<conio_backend>* root() { return m_root.get(); }

        // Get renderer for custom drawing
        conio_renderer* renderer() { return m_renderer.get(); }

        // Request a redraw
        void invalidate() { m_needs_redraw = true; }

        // Exit the application
        void quit() { m_running = false; }

    protected:
        // Override these in derived classes
        virtual void on_init() {}
        virtual void on_update(float delta_time) {}
        virtual void on_render(conio_renderer* renderer) {}
        virtual bool on_event(const tb_event& event) { return false; }

    private:
        void process_events();
        void render();
        void handle_resize();

        std::shared_ptr<vram> m_vram;
        std::unique_ptr<conio_renderer> m_renderer;
        std::unique_ptr<ui_element<conio_backend>> m_root;

        bool m_running = false;
        bool m_needs_redraw = true;
        int m_width = 0;
        int m_height = 0;
    };

    /**
     * @class conio_window
     * @brief A window/panel element for the terminal
     *
     * Provides a framed area with optional title bar.
     */
    class conio_window : public ui_element<conio_backend> {
    public:
        explicit conio_window(ui_element<conio_backend>* parent = nullptr);

        void set_title(const std::string& title) {
            m_title = title;
            invalidate_arrange();
        }

        void set_frame_style(box_style style) {
            m_frame_style = style;
            invalidate_arrange();
        }

        void set_title_color(color fg, color bg) {
            m_title_fg = fg;
            m_title_bg = bg;
            invalidate_arrange();
        }

        void set_frame_color(color fg, color bg) {
            m_frame_fg = fg;
            m_frame_bg = bg;
            invalidate_arrange();
        }

        void set_background_color(color bg) {
            m_background = bg;
            invalidate_arrange();
        }

    protected:
        virtual void on_render(conio_renderer* renderer) override;

    private:
        std::string m_title;
        box_style m_frame_style = box_style::single;
        color m_title_fg{255, 255, 255};
        color m_title_bg{0, 0, 128};
        color m_frame_fg{192, 192, 192};
        color m_frame_bg{0, 0, 0};
        color m_background{0, 0, 64};
    };

    /**
     * @class conio_label
     * @brief A text label element
     */
    class conio_label : public ui_element<conio_backend> {
    public:
        explicit conio_label(ui_element<conio_backend>* parent = nullptr);

        void set_text(const std::string& text) {
            m_text = text;
            invalidate_measure();
        }

        void set_color(color fg, color bg) {
            m_fg = fg;
            m_bg = bg;
            invalidate_arrange();
        }

        void set_alignment(int align) {
            m_alignment = align;
            invalidate_arrange();
        }

    protected:
        virtual size measure_content(int available_width, int available_height) override;
        virtual void on_render(conio_renderer* renderer) override;

    private:
        std::string m_text;
        color m_fg{255, 255, 255};
        color m_bg{0, 0, 0};
        int m_alignment = 0;  // 0=left, 1=center, 2=right
    };

    /**
     * @class conio_button
     * @brief A clickable button element
     */
    class conio_button : public ui_element<conio_backend> {
    public:
        explicit conio_button(ui_element<conio_backend>* parent = nullptr);

        void set_text(const std::string& text) {
            m_text = text;
            invalidate_measure();
        }

        void set_enabled(bool enabled) {
            m_enabled = enabled;
            invalidate_arrange();
        }

        // Callback for click events
        std::function<void()> on_click;

    protected:
        virtual size measure_content(int available_width, int available_height) override;
        virtual void on_render(conio_renderer* renderer) override;
        virtual bool on_mouse_button(const tb_event& event) override;
        virtual bool on_keyboard(const tb_event& event) override;

    private:
        std::string m_text;
        bool m_enabled = true;
        bool m_pressed = false;
        bool m_hover = false;
    };

    /**
     * @class conio_listbox
     * @brief A scrollable list of items
     */
    class conio_listbox : public ui_element<conio_backend> {
    public:
        explicit conio_listbox(ui_element<conio_backend>* parent = nullptr);

        void add_item(const std::string& item);
        void remove_item(int index);
        void clear_items();

        int get_selected_index() const { return m_selected_index; }
        void set_selected_index(int index);

        // Callback for selection changes
        std::function<void(int)> on_selection_changed;

    protected:
        virtual size measure_content(int available_width, int available_height) override;
        virtual void on_render(conio_renderer* renderer) override;
        virtual bool on_mouse_button(const tb_event& event) override;
        virtual bool on_keyboard(const tb_event& event) override;

    private:
        std::vector<std::string> m_items;
        int m_selected_index = -1;
        int m_scroll_offset = 0;
        int m_visible_items = 0;
    };
}