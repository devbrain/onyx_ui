/**
 * @file main_window.inl
 * @brief Implementation of main_window class
 * @author Claude Code
 * @date 2025-11-11
 */

#pragma once

namespace onyxui {

    template<UIBackend Backend>
    main_window<Backend>::main_window()
        : base(0)  // vbox with 0 spacing
    {
        // Don't create central widget yet - will be created lazily
        // This allows menu bar to be added first (child 0), then central (child 1), then status (child 2)
    }

    template<UIBackend Backend>
    void main_window<Backend>::set_menu_bar(std::unique_ptr<menu_bar_type> menu) {
        // For Phase 2, simple implementation: menu must be set before status
        // Children order in vbox: menu (if set), central, status (if set)
        // Menu is always added as first child
        if (m_menu_bar) {
            // Menu bar already exists - not supported in Phase 2
            // TODO: Support menu bar replacement in future
            return;
        }

        m_menu_bar = menu.get();

        // Insert menu at beginning by adding it first, then re-adding central
        // Actually, we can use insert_child at index 0
        // For now, just add as first child (central is already child 0, so this won't work)
        // Simplified: menu bar must be set BEFORE central widget is used
        this->add_child(std::move(menu));
        this->invalidate_measure();
    }

    template<UIBackend Backend>
    void main_window<Backend>::set_status_bar(std::unique_ptr<status_bar_type> status) {
        // Status bar is always added as last child
        if (m_status_bar) {
            // Status bar already exists - not supported in Phase 2
            return;
        }

        m_status_bar = status.get();
        this->add_child(std::move(status));
        this->invalidate_measure();
    }

    template<UIBackend Backend>
    void main_window<Backend>::set_central_widget(std::unique_ptr<ui_element<Backend>> central) {
        if (m_central_widget) {
            // Central widget already exists - not supported in Phase 2
            return;
        }

        m_central_widget = central.get();

        // Central widget should expand to fill available space between menu and status bar
        m_central_widget->set_width_constraint({size_policy::expand});
        m_central_widget->set_height_constraint({size_policy::expand});

        this->add_child(std::move(central));
        this->invalidate_measure();
    }

    template<UIBackend Backend>
    template<typename... Args>
    std::shared_ptr<typename main_window<Backend>::window_type>
    main_window<Backend>::create_window(Args&&... args) {
        ensure_central_widget();

        // Create window with central_widget as last parameter (parent)
        // Window constructor signature: window(title, flags, parent)
        auto win = std::make_shared<window_type>(
            std::forward<Args>(args)...,
            m_central_widget
        );

        return win;
    }

    template<UIBackend Backend>
    void main_window<Backend>::ensure_central_widget() {
        if (!m_central_widget) {
            // Create default empty panel as central widget
            set_central_widget(std::make_unique<panel_type>());
        }
    }

} // namespace onyxui
