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

#include <cstdint>
#include <string>
#include <utility>

namespace onyxui {

/// Checkbox state enumeration (two-state or tri-state)
enum class tri_state : std::uint8_t {
    unchecked = 0,      ///< Box is empty
    checked = 1,        ///< Box has checkmark
    indeterminate = 2   ///< Box has partial indicator (tri-state mode only)
};

/// Checkbox widget - toggleable boolean input control
///
/// The checkbox provides a two-state or tri-state toggle control with:
/// - Two-state mode (default): Unchecked ⟷ Checked
/// - Tri-state mode (optional): Unchecked ⟷ Indeterminate ⟷ Checked
/// - Keyboard interaction: Space key toggles
/// - Mouse interaction: Click anywhere on widget toggles
/// - Text label with optional mnemonic support (Alt+key)
/// - Theme integration for colors and box characters
///
/// Visual appearance (using themed icons - 3 characters wide):
/// @code
/// [ ] Unchecked       (checkbox_unchecked icon)
/// [X] Checked         (checkbox_checked icon)
/// [-] Indeterminate   (checkbox_indeterminate icon, tri-state only)
/// @endcode
///
/// Usage examples:
/// @code
/// // Simple checkbox
/// auto remember_me = std::make_unique<checkbox<Backend>>("Remember me");
/// remember_me->toggled.connect([](bool checked) {
///     std::cout << "Remember: " << (checked ? "Yes" : "No") << "\n";
/// });
///
/// // Tri-state checkbox (select all)
/// auto select_all = std::make_unique<checkbox<Backend>>("Select All");
/// select_all->set_tri_state_enabled(true);
/// select_all->state_changed.connect([](tri_state state) {
///     // Handle unchecked/checked/indeterminate states
/// });
///
/// // Checkbox with mnemonic (Alt+E toggles)
/// auto enable = std::make_unique<checkbox<Backend>>("&Enable feature");
/// enable->set_mnemonic('e');
/// @endcode
///
/// @tparam Backend The backend traits class
template<UIBackend Backend>
class checkbox : public widget<Backend> {
public:
    using base = widget<Backend>;
    using typename base::color_type;
    using typename base::rect_type;
    using typename base::size_type;
    using typename base::renderer_type;

    // ===== Construction =====

    /// Create checkbox with optional text label
    ///
    /// @param text Label text displayed to the right of the checkbox box
    /// @param parent Parent element (optional)
    explicit checkbox(std::string text = "", ui_element<Backend>* parent = nullptr)
        : base(parent)
        , m_text(std::move(text))
    {
        // Checkboxes are focusable by default
        this->set_focusable(true);
    }

    /// Create checkbox with text label and initial checked state
    ///
    /// @param text Label text
    /// @param checked Initial checked state (true = checked, false = unchecked)
    /// @param parent Parent element (optional)
    checkbox(std::string text, bool checked, ui_element<Backend>* parent = nullptr)
        : base(parent)
        , m_text(std::move(text))
        , m_state(checked ? tri_state::checked : tri_state::unchecked)
    {
        this->set_focusable(true);
    }

    // ===== State Management =====

    /// Set checked state (two-state mode)
    ///
    /// This method sets the checkbox to either checked or unchecked state.
    /// It emits both toggled(bool) and state_changed(tri_state) signals.
    ///
    /// @param checked true to check, false to uncheck
    void set_checked(bool checked) {
        const tri_state new_state = checked ? tri_state::checked : tri_state::unchecked;
        if (m_state == new_state) {
            return;  // No change
        }

        m_state = new_state;
        this->mark_dirty();

        // Emit signals
        toggled.emit(checked);
        state_changed.emit(m_state);
    }

    /// Get checked state
    ///
    /// @return true if state == tri_state::checked, false otherwise
    [[nodiscard]] bool is_checked() const noexcept {
        return m_state == tri_state::checked;
    }

    /// Set tri-state value
    ///
    /// This method allows setting any tri-state value, including indeterminate.
    /// If tri-state mode is disabled and you try to set indeterminate, the call is ignored.
    ///
    /// Note: The toggled(bool) signal is only emitted for unchecked ⟷ checked transitions.
    /// The state_changed(tri_state) signal is always emitted.
    ///
    /// @param state New tri-state value
    void set_tri_state(tri_state state) {
        // Validate: cannot set indeterminate if tri-state is disabled
        if (!m_tri_state_enabled && state == tri_state::indeterminate) {
            return;  // Silently ignore invalid state
        }

        if (m_state == state) {
            return;  // No change
        }

        const tri_state old_state = m_state;
        m_state = state;
        this->mark_dirty();

        // Emit state_changed always
        state_changed.emit(m_state);

        // Emit toggled only for unchecked ⟷ checked transitions
        if ((old_state == tri_state::unchecked && state == tri_state::checked) ||
            (old_state == tri_state::checked && state == tri_state::unchecked)) {
            toggled.emit(state == tri_state::checked);
        }
    }

    /// Get current tri-state value
    ///
    /// @return Current state (unchecked, checked, or indeterminate)
    [[nodiscard]] tri_state get_tri_state() const noexcept {
        return m_state;
    }

    /// Enable or disable tri-state mode
    ///
    /// When enabled, the checkbox can be in unchecked, indeterminate, or checked state.
    /// When disabled, only unchecked and checked states are allowed.
    ///
    /// Note: Disabling tri-state mode does NOT automatically clear indeterminate state.
    /// You must explicitly set a valid state if the checkbox is currently indeterminate.
    ///
    /// @param enabled true to enable tri-state mode, false to disable
    void set_tri_state_enabled(bool enabled) {
        m_tri_state_enabled = enabled;
    }

    /// Check if tri-state mode is enabled
    ///
    /// @return true if tri-state mode is enabled
    [[nodiscard]] bool is_tri_state_enabled() const noexcept {
        return m_tri_state_enabled;
    }

    // ===== Text Label =====

    /// Set label text
    ///
    /// The text is displayed to the right of the checkbox box.
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
    /// When the user presses Alt+key, the checkbox is toggled immediately.
    /// The mnemonic character is typically underlined in the label text.
    ///
    /// Example: set_mnemonic('e') with text "Enable feature" → Alt+E toggles checkbox
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

    /// Emitted when checkbox toggles between unchecked and checked (two-state logic)
    ///
    /// Parameter: true if checked, false if unchecked
    /// Note: NOT emitted when entering or leaving indeterminate state
    signal<bool> toggled;

    /// Emitted on ANY state change (including indeterminate)
    ///
    /// Parameter: new tri_state value
    signal<tri_state> state_changed;

protected:
    // ===== Rendering =====

    void do_render(render_context<Backend>& ctx) const override {
        auto* theme = ctx.theme();

        // Font for measurement
        typename renderer_type::font const default_font{};

        // Get checkbox icon based on state
        const auto& box_icon = get_box_icon(theme);

        // Measure icon and text
        auto box_size = renderer_type::get_icon_size(box_icon);
        const int box_width = size_utils::get_width(box_size);
        const int box_height = size_utils::get_height(box_size);

        int text_width = 0;
        if (!m_text.empty()) {
            auto text_size = renderer_type::measure_text(m_text, default_font);
            text_width = size_utils::get_width(text_size);
        }

        // Get spacing from theme (defaults to 1 if no theme)
        // Only include spacing if there's text to separate
        const int spacing = (!m_text.empty() && theme) ? theme->checkbox.spacing : 0;

        // Calculate natural size: icon + spacing + text
        const int natural_width = box_width + spacing + text_width;
        const int natural_height = box_height;

        // Get final dimensions and position from context
        auto const [final_width, final_height] = ctx.get_final_dims(natural_width, natural_height);
        auto const& pos = ctx.position();
        const int x = point_utils::get_x(pos);
        const int y = point_utils::get_y(pos);

        // Create widget rectangle
        typename Backend::rect_type widget_rect;
        rect_utils::set_bounds(widget_rect, x, y, final_width, final_height);

        // Fill background (no borders for checkbox)
        ctx.fill_rect(widget_rect);

        // If no theme, skip rendering
        if (!theme) return;

        // Get colors from resolved style
        auto fg = ctx.style().foreground_color;

        // Draw checkbox icon
        const typename Backend::point_type box_pos{x, y};
        ctx.draw_icon(box_icon, box_pos);

        // Draw label text (if any)
        if (!m_text.empty()) {
            const typename Backend::point_type text_pos{x + box_width + spacing, y};
            ctx.draw_text(m_text, text_pos, default_font, fg);
        }
    }

    // ===== Event Handling =====

    bool handle_event(const ui_event& event, event_phase phase) override {
        // Handle mouse press (request focus in capture phase, toggle in target phase)
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
                    // Toggle in target phase
                    if (this->is_enabled()) {
                        toggle();
                        return true;  // Event handled
                    }
                }
            }
        }

        return base::handle_event(event, phase);
    }

    /// Handle semantic action (Space key to toggle)
    bool handle_semantic_action(hotkey_action action) override {
        switch (action) {
            case hotkey_action::activate_widget:
                // Space key toggles checkbox
                if (this->is_enabled()) {
                    toggle();
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
    tri_state m_state = tri_state::unchecked;     ///< Current state
    bool m_tri_state_enabled = false;             ///< Allow indeterminate state?
    char m_mnemonic = '\0';                       ///< Mnemonic character (or '\0')

    // ===== Internal Helpers =====

    /// Toggle to next state
    ///
    /// Two-state mode: Unchecked ⟷ Checked
    /// Tri-state mode: Indeterminate → Checked (user click exits indeterminate)
    ///                 Unchecked ⟷ Checked (normal toggle)
    void toggle() {
        if (m_tri_state_enabled && m_state == tri_state::indeterminate) {
            // Indeterminate → Checked (user click exits indeterminate state)
            set_tri_state(tri_state::checked);
        } else if (m_state == tri_state::unchecked) {
            // Unchecked → Checked
            set_checked(true);
        } else {
            // Checked → Unchecked
            set_checked(false);
        }
    }

    /// Get checkbox icon for current state from theme
    [[nodiscard]] typename Backend::renderer_type::icon_style get_box_icon(const ui_theme<Backend>* theme) const {
        using icon_type = typename Backend::renderer_type::icon_style;

        if (!theme) {
            // Fallback to checkbox icon enums when no theme
            switch (m_state) {
                case tri_state::unchecked:
                    return icon_type::checkbox_unchecked;
                case tri_state::checked:
                    return icon_type::checkbox_checked;
                case tri_state::indeterminate:
                    return icon_type::checkbox_indeterminate;
            }
            return icon_type::checkbox_unchecked;
        }

        switch (m_state) {
            case tri_state::unchecked:
                return theme->checkbox.unchecked_icon;
            case tri_state::checked:
                return theme->checkbox.checked_icon;
            case tri_state::indeterminate:
                return theme->checkbox.indeterminate_icon;
        }

        return theme->checkbox.unchecked_icon;  // Fallback
    }
};

}  // namespace onyxui
