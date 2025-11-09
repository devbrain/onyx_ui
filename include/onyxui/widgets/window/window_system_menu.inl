/**
 * @file window_system_menu.inl
 * @brief Implementation of window_system_menu class
 */

#pragma once

#include <onyxui/widgets/window/window.hh>
#include <onyxui/widgets/menu/separator.hh>

namespace onyxui {

    template<UIBackend Backend>
    window_system_menu<Backend>::window_system_menu(window<Backend>* owner)
        : m_owner(owner)
        , m_menu(std::make_unique<menu<Backend>>())
    {
        create_menu_items();
    }

    template<UIBackend Backend>
    void window_system_menu<Backend>::create_menu_items() {
        // Create actions
        m_restore_action = std::make_shared<action<Backend>>();
        m_restore_action->set_text("Restore");
        m_restore_action->triggered.connect([this]() {
            m_owner->restore();
        });

        m_minimize_action = std::make_shared<action<Backend>>();
        m_minimize_action->set_text("Minimize");
        m_minimize_action->triggered.connect([this]() {
            m_owner->minimize();
        });

        m_maximize_action = std::make_shared<action<Backend>>();
        m_maximize_action->set_text("Maximize");
        m_maximize_action->triggered.connect([this]() {
            m_owner->maximize();
        });

        m_close_action = std::make_shared<action<Backend>>();
        m_close_action->set_text("Close");
        m_close_action->set_shortcut(key_sequence{4, key_modifier::alt});  // Alt+F4
        m_close_action->triggered.connect([this]() {
            m_owner->close();
        });

        // Create menu items
        auto restore_item = std::make_unique<menu_item<Backend>>();
        restore_item->set_mnemonic_text("&Restore");
        restore_item->set_action(m_restore_action);
        m_restore_item = m_menu->add_item(std::move(restore_item));

        // Optional: Move and Size (commented out for Phase 7, can be added later)
        // auto move_item = std::make_unique<menu_item<Backend>>();
        // move_item->set_mnemonic_text("&Move");
        // m_move_item = m_menu->add_item(std::move(move_item));

        // auto size_item = std::make_unique<menu_item<Backend>>();
        // size_item->set_mnemonic_text("&Size");
        // m_size_item = m_menu->add_item(std::move(size_item));

        auto minimize_item = std::make_unique<menu_item<Backend>>();
        minimize_item->set_mnemonic_text("Mi&nimize");
        minimize_item->set_action(m_minimize_action);
        m_minimize_item = m_menu->add_item(std::move(minimize_item));

        auto maximize_item = std::make_unique<menu_item<Backend>>();
        maximize_item->set_mnemonic_text("Ma&ximize");
        maximize_item->set_action(m_maximize_action);
        m_maximize_item = m_menu->add_item(std::move(maximize_item));

        // Separator before Close
        m_menu->add_separator();

        auto close_item = std::make_unique<menu_item<Backend>>();
        close_item->set_mnemonic_text("&Close");
        close_item->set_action(m_close_action);
        m_close_item = m_menu->add_item(std::move(close_item));

        // Initial state update
        update_menu_states();
    }

    template<UIBackend Backend>
    void window_system_menu<Backend>::update_menu_states() {
        using window_state = typename window<Backend>::window_state;

        window_state state = m_owner->get_state();

        // Restore: Enabled if minimized or maximized
        bool can_restore = (state == window_state::minimized || state == window_state::maximized);
        m_restore_action->set_enabled(can_restore);

        // Minimize: Enabled if not already minimized
        bool can_minimize = (state != window_state::minimized);
        m_minimize_action->set_enabled(can_minimize);

        // Maximize: Enabled if not already maximized
        bool can_maximize = (state != window_state::maximized);
        m_maximize_action->set_enabled(can_maximize);

        // Close: Always enabled
        m_close_action->set_enabled(true);
    }

} // namespace onyxui
