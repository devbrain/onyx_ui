// Copyright (c) 2025 OnyxUI Framework
// SPDX-License-Identifier: MIT

#pragma once

#include <cstdint>
#include <onyxui/concepts/backend.hh>
#include <onyxui/core/signal.hh>
#include <onyxui/layout/linear_layout.hh>
#include <onyxui/services/ui_services.hh>
#include <onyxui/widgets/core/widget.hh>

#include <algorithm>
#include <unordered_map>
#include <vector>

namespace onyxui {

// Forward declaration
template<UIBackend Backend>
class radio_button;

/// Button group orientation
///
/// Specifies whether radio buttons are arranged vertically or horizontally.
enum class button_group_orientation : std::uint8_t {
    vertical,    ///< Stack radio buttons vertically (top-to-bottom)
    horizontal   ///< Arrange radio buttons horizontally (left-to-right)
};

// Import spacing enum from layout
using onyxui::spacing;

/// Manages mutually exclusive radio buttons as a widget container
///
/// The button_group class is a widget container that owns and manages
/// radio buttons. It enforces single selection among its radio button children.
/// When one radio button is checked, all others are automatically unchecked.
///
/// **Design:** button_group is a widget container that owns its radio buttons.
/// This ensures proper lifetime management - radio buttons can't outlive their group.
///
/// **Layout:** Supports both vertical (default) and horizontal arrangements.
///
/// Usage:
/// @code
/// // Vertical group (default)
/// auto* v_group = container->emplace_child<button_group<Backend>>();
/// v_group->add_option("Small", 1);
/// v_group->add_option("Medium", 2);
/// v_group->add_option("Large", 3);
///
/// // Horizontal group
/// auto* h_group = container->emplace_child<button_group<Backend>>(
///     button_group_orientation::horizontal, 2  // 2px spacing
/// );
/// h_group->add_option("Red", 1);
/// h_group->add_option("Green", 2);
/// h_group->add_option("Blue", 3);
/// h_group->set_checked_id(2);  // Select green
///
/// h_group->button_toggled.connect([](int id, bool checked) {
///     if (checked) {
///         std::cout << "Selected option: " << id << "\n";
///     }
/// });
/// @endcode
///
/// @tparam Backend The backend traits class
template<UIBackend Backend>
class button_group : public widget<Backend> {
public:
    using base = widget<Backend>;
    using horizontal_alignment = onyxui::horizontal_alignment;
    using vertical_alignment = onyxui::vertical_alignment;

    /// Create button group with configurable layout orientation
    ///
    /// @param orientation Layout direction (vertical or horizontal)
    /// @param spacing_value Semantic spacing between radio buttons (resolved via theme)
    /// @param parent Parent element (optional)
    explicit button_group(
        button_group_orientation orientation = button_group_orientation::vertical,
        spacing spacing_value = spacing::small,
        ui_element<Backend>* parent = nullptr
    )
        : base(parent)
        , m_orientation(orientation)
        , m_spacing(spacing_value)
    {
        // Set up linear layout based on orientation
        if (m_orientation == button_group_orientation::vertical) {
            this->set_layout_strategy(
                std::make_unique<linear_layout<Backend>>(
                    direction::vertical, resolve_spacing(),
                    horizontal_alignment::stretch,
                    vertical_alignment::top
                )
            );
        } else {
            this->set_layout_strategy(
                std::make_unique<linear_layout<Backend>>(
                    direction::horizontal, resolve_spacing(),
                    horizontal_alignment::left,
                    vertical_alignment::stretch
                )
            );
        }

        this->set_focusable(false);  // Container, not focusable
    }

    // ===== Option Management =====

    /// Add a radio button option to the group
    ///
    /// Creates a radio_button widget as a child and registers it with the given ID.
    /// If id is -1, a unique ID is auto-assigned starting from 0.
    ///
    /// @param text Label text for the radio button
    /// @param id Unique ID for this option, or -1 for auto-assignment
    /// @return Pointer to the created radio_button (for further customization if needed)
    radio_button<Backend>* add_option(const std::string& text, int id = -1);

    /// Get button by ID
    ///
    /// @param id Button ID
    /// @return Pointer to button, or nullptr if not found
    [[nodiscard]] radio_button<Backend>* button(int id) const {
        auto it = m_buttons.find(id);
        return (it != m_buttons.end()) ? it->second : nullptr;
    }

    /// Get all buttons (in no particular order)
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

    /// Get number of options in group
    ///
    /// @return Option count
    [[nodiscard]] std::size_t count() const noexcept {
        return m_buttons.size();
    }

    /// Get group orientation
    ///
    /// @return Current orientation (vertical or horizontal)
    [[nodiscard]] button_group_orientation orientation() const noexcept {
        return m_orientation;
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
    button_group_orientation m_orientation;
    spacing m_spacing;
    std::unordered_map<int, radio_button<Backend>*> m_buttons;
    int m_checked_id = -1;
    int m_next_id = 0;

    /**
     * @brief Resolve semantic spacing to backend-specific integer via theme
     * @return Resolved spacing value in logical units
     */
    [[nodiscard]] int resolve_spacing() const {
        // Get current theme from ui_services
        auto* themes = ui_services<Backend>::themes();
        if (!themes) {
            // No theme available, use default small spacing (1)
            return 1;
        }

        auto* theme = themes->get_current_theme();
        if (!theme) {
            // No current theme, use default small spacing (1)
            return 1;
        }

        // Resolve spacing enum via theme
        return theme->spacing.resolve(m_spacing);
    }

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
radio_button<Backend>* button_group<Backend>::add_option(const std::string& text, int id) {
    // Auto-assign ID if needed
    if (id == -1) {
        id = m_next_id++;
    } else {
        // Update next_id if user-provided ID is higher
        if (id >= m_next_id) {
            m_next_id = id + 1;
        }
    }

    // Check for duplicate ID
    if (m_buttons.find(id) != m_buttons.end()) {
        // ID already exists - skip (could throw exception instead)
        return nullptr;
    }

    // Create radio button as child
    auto* rb = this->template emplace_child<radio_button>(text);

    // Register in map
    m_buttons[id] = rb;

    return rb;
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
