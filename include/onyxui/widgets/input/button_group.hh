// Copyright (c) 2025 OnyxUI Framework
// SPDX-License-Identifier: MIT

#pragma once

#include <onyxui/concepts/backend.hh>
#include <onyxui/core/signal.hh>
#include <onyxui/services/ui_services.hh>

#include <algorithm>
#include <unordered_map>
#include <vector>

namespace onyxui {

// Forward declaration
template<UIBackend Backend>
class radio_button;

/// Manages mutually exclusive radio buttons
///
/// The button_group class enforces single selection among a group of radio buttons.
/// When one radio button is checked, all others in the group are automatically unchecked.
///
/// **Important**: button_group is NOT a widget. It's a manager class that coordinates
/// radio buttons. Radio buttons remain in their normal widget hierarchy.
///
/// Usage:
/// @code
/// auto group = std::make_shared<button_group<Backend>>();
///
/// auto radio1 = std::make_unique<radio_button<Backend>>("Option 1");
/// auto radio2 = std::make_unique<radio_button<Backend>>("Option 2");
/// auto radio3 = std::make_unique<radio_button<Backend>>("Option 3");
///
/// group->add_button(radio1.get(), 1);
/// group->add_button(radio2.get(), 2);
/// group->add_button(radio3.get(), 3);
/// group->set_checked_id(1);  // Select first option
///
/// group->button_toggled.connect([](int id, bool checked) {
///     if (checked) {
///         std::cout << "Selected option: " << id << "\n";
///     }
/// });
/// @endcode
///
/// @tparam Backend The backend traits class
template<UIBackend Backend>
class button_group {
public:
    button_group() = default;

    // ===== Button Management =====

    /// Add radio button to group
    ///
    /// The radio button's group pointer is automatically set to this group.
    /// If id is -1, a unique ID is auto-assigned starting from 0.
    ///
    /// @param button Pointer to radio button (non-owning - widget is owned by parent)
    /// @param id Unique ID for this button, or -1 for auto-assignment
    void add_button(radio_button<Backend>* button, int id = -1);

    /// Remove radio button from group
    ///
    /// The radio button's group pointer is automatically cleared.
    /// If the removed button was checked, no button will be checked afterward.
    ///
    /// @param button Pointer to radio button to remove
    void remove_button(radio_button<Backend>* button);

    /// Get all buttons in group (in no particular order)
    ///
    /// @return Vector of button pointers
    [[nodiscard]] std::vector<radio_button<Backend>*> buttons() const {
        std::vector<radio_button<Backend>*> result;
        result.reserve(m_buttons.size());
        for (const auto& [id, button] : m_buttons) {
            result.push_back(button);
        }
        return result;
    }

    /// Get button by ID
    ///
    /// @param id Button ID
    /// @return Pointer to button, or nullptr if not found
    [[nodiscard]] radio_button<Backend>* button(int id) const {
        auto it = m_buttons.find(id);
        return (it != m_buttons.end()) ? it->second : nullptr;
    }

    /// Get number of buttons in group
    ///
    /// @return Button count
    [[nodiscard]] std::size_t count() const noexcept {
        return m_buttons.size();
    }

    // ===== Selection Management =====

    /// Get checked button ID
    ///
    /// @return ID of checked button, or -1 if none checked
    [[nodiscard]] int checked_id() const noexcept {
        return m_checked_id;
    }

    /// Get checked button pointer
    ///
    /// @return Pointer to checked button, or nullptr if none checked
    [[nodiscard]] radio_button<Backend>* checked_button() const {
        return button(m_checked_id);
    }

    /// Set checked button by ID
    ///
    /// Automatically unchecks all other buttons in the group.
    /// If id is -1, all buttons are unchecked.
    ///
    /// @param id ID of button to check, or -1 to uncheck all
    void set_checked_id(int id);

    /// Set checked button by pointer
    ///
    /// Automatically unchecks all other buttons in the group.
    /// If button is nullptr, all buttons are unchecked.
    ///
    /// @param button Pointer to button to check, or nullptr to uncheck all
    void set_checked_button(radio_button<Backend>* button);

    // ===== Navigation =====

    /// Select next radio button in group (with wrapping)
    ///
    /// Used for arrow key navigation (Down/Right arrows).
    ///
    /// @param current Currently focused radio button
    void select_next(radio_button<Backend>* current);

    /// Select previous radio button in group (with wrapping)
    ///
    /// Used for arrow key navigation (Up/Left arrows).
    ///
    /// @param current Currently focused radio button
    void select_previous(radio_button<Backend>* current);

    // ===== Signals =====

    /// Emitted when any button in group changes state
    ///
    /// Parameters: (button_id, checked)
    ///
    /// Note: When checking a new button, this signal is emitted twice:
    /// 1. Once with (old_id, false) for the button being unchecked
    /// 2. Once with (new_id, true) for the button being checked
    signal<int, bool> button_toggled;

private:
    std::unordered_map<int, radio_button<Backend>*> m_buttons;
    int m_checked_id = -1;
    int m_next_id = 0;

    /// Internal: Called by radio_button when it's checked
    ///
    /// Enforces mutual exclusion by unchecking all other buttons.
    ///
    /// @param checked_button The button that was just checked
    void notify_button_checked(radio_button<Backend>* checked_button);

    /// Internal: Get ID for a button pointer
    ///
    /// @param button Button pointer
    /// @return Button ID, or -1 if not found
    [[nodiscard]] int get_button_id(radio_button<Backend>* button) const {
        for (const auto& [id, btn] : m_buttons) {
            if (btn == button) {
                return id;
            }
        }
        return -1;
    }

    friend class radio_button<Backend>;
};

// ===== Implementation =====

template<UIBackend Backend>
void button_group<Backend>::add_button(radio_button<Backend>* button, int id) {
    if (!button) {
        return;  // Null button
    }

    // Auto-assign ID if needed
    if (id == -1) {
        id = m_next_id++;
    } else {
        // Update next_id if user-provided ID is higher
        if (id >= m_next_id) {
            m_next_id = id + 1;
        }
    }

    // Add button to map
    m_buttons[id] = button;

    // Set button's group pointer
    button->set_group(this);

    // If this button is checked, update checked_id and uncheck others
    if (button->is_checked()) {
        // Uncheck all other buttons first
        for (const auto& [other_id, other_button] : m_buttons) {
            if (other_id != id && other_button->is_checked()) {
                other_button->m_is_checked = false;  // Direct access (friend)
                other_button->mark_dirty();
                other_button->toggled.emit(false);
                button_toggled.emit(other_id, false);
            }
        }
        m_checked_id = id;
    }
}

template<UIBackend Backend>
void button_group<Backend>::remove_button(radio_button<Backend>* button) {
    if (!button) {
        return;
    }

    // Find button ID
    int button_id = get_button_id(button);
    if (button_id == -1) {
        return;  // Button not in this group
    }

    // Clear button's group pointer
    button->set_group(nullptr);

    // Remove from map
    m_buttons.erase(button_id);

    // If this was the checked button, clear checked_id
    if (m_checked_id == button_id) {
        m_checked_id = -1;
    }
}

template<UIBackend Backend>
void button_group<Backend>::set_checked_id(int id) {
    if (id == -1) {
        // Uncheck all buttons
        for (const auto& [button_id, button] : m_buttons) {
            if (button->is_checked()) {
                button->m_is_checked = false;  // Direct access (friend)
                button->mark_dirty();
                button->toggled.emit(false);
                button_toggled.emit(button_id, false);
            }
        }
        m_checked_id = -1;
        return;
    }

    // Find button with this ID
    auto it = m_buttons.find(id);
    if (it == m_buttons.end()) {
        return;  // Invalid ID
    }

    radio_button<Backend>* new_checked = it->second;

    // Already checked?
    if (m_checked_id == id) {
        return;
    }

    // Uncheck currently checked button
    if (m_checked_id != -1) {
        auto old_it = m_buttons.find(m_checked_id);
        if (old_it != m_buttons.end()) {
            radio_button<Backend>* old_checked = old_it->second;
            old_checked->m_is_checked = false;  // Direct access (friend)
            old_checked->mark_dirty();
            old_checked->toggled.emit(false);
            button_toggled.emit(m_checked_id, false);
        }
    }

    // Check new button
    new_checked->m_is_checked = true;  // Direct access (friend)
    new_checked->mark_dirty();
    new_checked->toggled.emit(true);
    button_toggled.emit(id, true);

    m_checked_id = id;
}

template<UIBackend Backend>
void button_group<Backend>::set_checked_button(radio_button<Backend>* button) {
    if (!button) {
        set_checked_id(-1);  // Uncheck all
        return;
    }

    int id = get_button_id(button);
    if (id != -1) {
        set_checked_id(id);
    }
}

template<UIBackend Backend>
void button_group<Backend>::notify_button_checked(radio_button<Backend>* checked_button) {
    // Find the ID of the checked button
    int checked_id = get_button_id(checked_button);
    if (checked_id == -1) {
        return;  // Button not in this group
    }

    // Already the checked button?
    if (m_checked_id == checked_id) {
        return;
    }

    // Uncheck currently checked button
    if (m_checked_id != -1) {
        auto old_it = m_buttons.find(m_checked_id);
        if (old_it != m_buttons.end()) {
            radio_button<Backend>* old_checked = old_it->second;
            old_checked->m_is_checked = false;  // Direct access (friend)
            old_checked->mark_dirty();
            old_checked->toggled.emit(false);
            button_toggled.emit(m_checked_id, false);
        }
    }

    // Update checked ID
    m_checked_id = checked_id;

    // Emit group signal for new checked button
    button_toggled.emit(checked_id, true);
}

template<UIBackend Backend>
void button_group<Backend>::select_next(radio_button<Backend>* current) {
    if (m_buttons.empty()) {
        return;
    }

    // Get current button ID
    int current_id = get_button_id(current);
    if (current_id == -1) {
        return;  // Current button not in group
    }

    // Build sorted list of IDs
    std::vector<int> ids;
    ids.reserve(m_buttons.size());
    for (const auto& [id, _] : m_buttons) {
        ids.push_back(id);
    }
    std::sort(ids.begin(), ids.end());

    // Find current ID position
    auto it = std::find(ids.begin(), ids.end(), current_id);
    if (it == ids.end()) {
        return;
    }

    // Move to next (wrap around if at end)
    ++it;
    if (it == ids.end()) {
        it = ids.begin();  // Wrap to first
    }

    // Check next button
    set_checked_id(*it);

    // Set focus to next button (if possible)
    if (auto* next_button = button(*it)) {
        if (auto* input = ui_services<Backend>::input()) {
            input->set_focus(next_button);
        }
    }
}

template<UIBackend Backend>
void button_group<Backend>::select_previous(radio_button<Backend>* current) {
    if (m_buttons.empty()) {
        return;
    }

    // Get current button ID
    int current_id = get_button_id(current);
    if (current_id == -1) {
        return;  // Current button not in group
    }

    // Build sorted list of IDs
    std::vector<int> ids;
    ids.reserve(m_buttons.size());
    for (const auto& [id, _] : m_buttons) {
        ids.push_back(id);
    }
    std::sort(ids.begin(), ids.end());

    // Find current ID position
    auto it = std::find(ids.begin(), ids.end(), current_id);
    if (it == ids.end()) {
        return;
    }

    // Move to previous (wrap around if at beginning)
    if (it == ids.begin()) {
        it = ids.end();  // Wrap to last
    }
    --it;

    // Check previous button
    set_checked_id(*it);

    // Set focus to previous button (if possible)
    if (auto* prev_button = button(*it)) {
        if (auto* input = ui_services<Backend>::input()) {
            input->set_focus(prev_button);
        }
    }
}

}  // namespace onyxui
