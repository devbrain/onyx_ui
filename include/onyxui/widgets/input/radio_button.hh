// Copyright (c) 2025 OnyxUI Framework
// SPDX-License-Identifier: MIT

#pragma once

#include <onyxui/concepts/backend.hh>
#include <onyxui/core/rendering/render_context.hh>
#include <onyxui/core/signal.hh>
#include <onyxui/events/ui_event.hh>
#include <onyxui/hotkeys/hotkey_action.hh>
#include <onyxui/services/ui_services.hh>
#include <onyxui/widgets/core/widget.hh>
#include <onyxui/widgets/input/button_group.hh>

#include <string>
#include <utility>

namespace onyxui {

/// Radio button widget - mutually exclusive selection control
///
/// The radio_button provides a toggle control for mutually exclusive options.
/// Unlike checkboxes (which allow multiple independent selections), radio buttons
/// enforce single selection within a button_group.
///
/// **Key Characteristics:**
/// - Two-state only: checked or unchecked (no indeterminate state)
/// - Must be child of a button_group for mutual exclusion
/// - Clicking a checked radio button does NOT uncheck it
/// - Arrow keys navigate within group and select
/// - Space key checks the focused button
///
/// **Design:** Radio buttons are created by button_group via add_option().
/// The group owns the radio buttons through the widget tree, ensuring proper
/// lifetime management and automatic mutual exclusion.
///
/// Visual appearance (using themed icons - 3 characters wide):
/// @code
/// ( ) Unchecked       (radio_unchecked icon)
/// (*) Checked         (radio_checked icon)
/// @endcode
///
/// Usage examples:
/// @code
/// // Create button group (owns radio buttons)
/// auto* group = container->emplace_child<button_group<Backend>>();
/// group->add_option("Small", 1);
/// group->add_option("Medium", 2);
/// group->add_option("Large", 3);
/// group->set_checked_id(2);  // Default to medium
///
/// // Listen for changes
/// group->button_toggled.connect([](int id, bool checked) {
///     if (checked) {
///         std::cout << "Selected option: " << id << "\n";
///     }
/// });
/// @endcode
///
/// @tparam Backend The backend traits class
template<UIBackend Backend>
class radio_button : public widget<Backend> {
public:
    using base = widget<Backend>;
    using typename base::color_type;
    using typename base::rect_type;
    using typename base::size_type;
    using typename base::renderer_type;

    // ===== Construction =====

    /// Create radio button with optional text label
    ///
    /// @param text Label text displayed to the right of the radio icon
    /// @param parent Parent element (optional)
    explicit radio_button(std::string text = "", ui_element<Backend>* parent = nullptr)
        : base(parent)
        , m_text(std::move(text))
    {
        // Radio buttons are focusable by default
        this->set_focusable(true);
    }

    /// Create radio button with text label and initial checked state
    ///
    /// @param text Label text
    /// @param checked Initial checked state
    /// @param parent Parent element (optional)
    radio_button(std::string text, bool checked, ui_element<Backend>* parent = nullptr)
        : base(parent)
        , m_text(std::move(text))
        , m_is_checked(checked)
    {
        this->set_focusable(true);
    }

    // ===== State Management =====

    /// Set checked state
    ///
    /// If this radio button is in a button_group, checking it will automatically
    /// uncheck all other radio buttons in the group via notify_button_checked().
    ///
    /// Note: Unchecking a radio button directly is allowed, but typically the
    /// group manages this when checking a different button.
    ///
    /// @param checked true to check, false to uncheck
    void set_checked(bool checked) {
        if (m_is_checked == checked) {
            return;  // No change
        }

        m_is_checked = checked;
        this->mark_dirty();

        // If checking this button and parent is a button_group, notify group (mutual exclusion)
        if (checked) {
            if (auto* parent_elem = this->parent()) {
                if (auto* group = dynamic_cast<button_group<Backend>*>(parent_elem)) {
                    group->notify_button_checked(this);
                }
            }
        }

        // Emit signal
        toggled.emit(m_is_checked);
    }

    /// Get checked state
    ///
    /// @return true if checked
    [[nodiscard]] bool is_checked() const noexcept {
        return m_is_checked;
    }

    // ===== Text Label =====

    /// Set label text
    ///
    /// The text is displayed to the right of the radio icon.
    /// Changes to text invalidate layout and trigger re-measure.
    ///
    /// @param text New label text
    void set_text(const std::string& text) {
        if (m_text == text) {
            return;
        }

        m_text = text;
        this->invalidate_measure();
    }

    /// Get label text
    ///
    /// @return Current label text
    [[nodiscard]] const std::string& text() const noexcept {
        return m_text;
    }

    // ===== Mnemonic Support =====

    /// Set mnemonic character for keyboard shortcut
    ///
    /// When the user presses Alt+key, the radio button is checked immediately.
    /// The mnemonic character is typically underlined in the label text.
    ///
    /// Example: set_mnemonic('m') with text "Medium" → Alt+M checks button
    ///
    /// @param key Mnemonic character (case-insensitive), or '\0' to clear
    void set_mnemonic(char key) {
        m_mnemonic = key;
    }

    /// Get mnemonic character
    ///
    /// @return Mnemonic character, or '\0' if none set
    [[nodiscard]] char mnemonic() const noexcept {
        return m_mnemonic;
    }

    // ===== Signals =====

    /// Emitted when checked state changes
    ///
    /// Parameter: true if checked, false if unchecked
    signal<bool> toggled;

protected:
    // ===== Rendering =====

    void do_render(render_context<Backend>& ctx) const override {
        auto* theme = ctx.theme();

        // Font for measurement
        typename renderer_type::font const default_font{};

        // Get radio icon based on state
        const auto& icon = get_radio_icon(theme);

        // Measure icon and text
        auto icon_size = renderer_type::get_icon_size(icon);
        const int icon_width = size_utils::get_width(icon_size);
        const int icon_height = size_utils::get_height(icon_size);

        int text_width = 0;
        if (!m_text.empty()) {
            auto text_size = renderer_type::measure_text(m_text, default_font);
            text_width = size_utils::get_width(text_size);
        }

        // Get spacing from theme (defaults to 1 if no theme)
        // Only include spacing if there's text to separate
        const int spacing = (!m_text.empty() && theme) ? theme->radio_button.spacing : 0;

        // Calculate natural size: icon + spacing + text
        const int natural_width = icon_width + spacing + text_width;
        const int natural_height = icon_height;

        // Get final dimensions and position from context
        auto const [final_width, final_height] = ctx.get_final_dims(natural_width, natural_height);
        auto const& pos = ctx.position();
        const int x = point_utils::get_x(pos);
        const int y = point_utils::get_y(pos);

        // Create widget rectangle
        typename Backend::rect_type widget_rect;
        rect_utils::set_bounds(widget_rect, x, y, final_width, final_height);

        // Fill background (no borders for radio button)
        ctx.fill_rect(widget_rect);

        // If no theme, skip rendering
        if (!theme) return;

        // Get colors from resolved style
        auto fg = ctx.style().foreground_color;

        // Draw radio icon
        const typename Backend::point_type icon_pos{x, y};
        ctx.draw_icon(icon, icon_pos);

        // Draw label text (if any)
        if (!m_text.empty()) {
            const typename Backend::point_type text_pos{x + icon_width + spacing, y};
            ctx.draw_text(m_text, text_pos, default_font, fg);
        }
    }

    // ===== Event Handling =====

    bool handle_event(const ui_event& event, event_phase phase) override {
        // Handle mouse press (request focus in capture phase, check in target phase)
        if (auto* mouse_evt = std::get_if<mouse_event>(&event)) {
            if (mouse_evt->act == mouse_event::action::press) {
                if (phase == event_phase::capture) {
                    // Request focus in capture phase
                    auto* input = ui_services<Backend>::input();
                    if (input && this->is_focusable()) {
                        input->set_focus(this);
                    }
                    return false;  // Continue to target phase
                }

                if (phase == event_phase::target) {
                    // Check radio button on click
                    // Note: Cannot uncheck by clicking - must select different option
                    if (this->is_enabled() && !m_is_checked) {
                        set_checked(true);
                        return true;  // Event handled
                    }
                }
            }
        }

        return base::handle_event(event, phase);
    }

    /// Handle semantic action (Space key checks, arrow keys navigate group)
    bool handle_semantic_action(hotkey_action action) override {
        // Find parent button_group if it exists
        button_group<Backend>* group = nullptr;
        if (auto* parent_elem = this->parent()) {
            group = dynamic_cast<button_group<Backend>*>(parent_elem);
        }

        switch (action) {
            case hotkey_action::activate_widget:
                // Space key checks radio button
                if (this->is_enabled() && !m_is_checked) {
                    set_checked(true);
                    return true;
                }
                return false;

            case hotkey_action::menu_up:
            case hotkey_action::menu_left:
                // Navigate to previous radio button in group (reusing menu navigation)
                if (group) {
                    group->select_previous(this);
                    return true;
                }
                return false;

            case hotkey_action::menu_down:
            case hotkey_action::menu_right:
                // Navigate to next radio button in group (reusing menu navigation)
                if (group) {
                    group->select_next(this);
                    return true;
                }
                return false;

            default:
                return base::handle_semantic_action(action);
        }
    }

private:
    // ===== State =====

    std::string m_text;                           ///< Label text
    bool m_is_checked = false;                    ///< Current state
    char m_mnemonic = '\0';                       ///< Mnemonic character (or '\0')

    // ===== Internal Helpers =====

    /// Get radio icon for current state from theme
    [[nodiscard]] typename Backend::renderer_type::icon_style get_radio_icon(const ui_theme<Backend>* theme) const {
        using icon_type = typename Backend::renderer_type::icon_style;

        if (!theme) {
            // Fallback to radio icon enums when no theme
            return m_is_checked ? icon_type::radio_checked : icon_type::radio_unchecked;
        }

        return m_is_checked ? theme->radio_button.checked_icon : theme->radio_button.unchecked_icon;
    }

    friend class button_group<Backend>;
};

}  // namespace onyxui
