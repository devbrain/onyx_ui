//
// Created by igor on 14/10/2025.
//

#pragma once

#include <optional>
#include <utility>  // for std::exchange
#include <onyxui/theme.hh>

namespace onyxui {
    /**
     * @class themeable
     * @brief Provides CSS-style property inheritance for visual styling
     *
     * ## Inheritance Model
     *
     * Properties resolve in this order:
     * 1. **Explicit override** (set_background_color, etc.)
     * 2. **Parent inheritance** (walks up the tree)
     * 3. **Theme default** (widget-type specific)
     * 4. **Global fallback** (theme globals)
     *
     * ## Inheritable Properties (CSS-style)
     *
     * These inherit from parent to child:
     * - **Colors**: background, foreground/text
     * - **Renderer styles**: box_style (borders), font, icon_style
     * - **Effects**: opacity/alpha (multiplicative)
     *
     * ## Non-Inheritable Properties
     *
     * Layout properties don't inherit:
     * - Padding, margin, size constraints
     * - State colors (hover, pressed) - widget-specific
     *
     * @example Norton Utilities style
     * @code
     * // Window with dark blue background
     * window->set_background_color({0, 0, 170});
     * window->set_foreground_color({255, 255, 255});
     * window->set_box_style(single_border);
     *
     * auto group = create_group_box("System Info");
     * window->add_child(group);
     * // group automatically inherits colors and box style!
     * @endcode
     */
    template<UIBackend Backend>
    class themeable {
        public:
            using theme_type = ui_theme<Backend>;
            using color_type = typename Backend::color_type;
            using box_style_type = typename Backend::renderer_type::box_style;
            using font_type = typename Backend::renderer_type::font;
            using icon_style_type = typename Backend::renderer_type::icon_style;

            virtual ~themeable() = default;

            // Delete copy operations (theme binding is per-instance)
            themeable(const themeable&) = delete;
            themeable& operator=(const themeable&) = delete;

            // Move operations (safe: transfers theme pointer and overrides)
            themeable(themeable&& other) noexcept
                : m_theme(std::exchange(other.m_theme, nullptr))
                , m_background_override(std::move(other.m_background_override))
                , m_foreground_override(std::move(other.m_foreground_override))
                , m_box_style_override(std::move(other.m_box_style_override))
                , m_font_override(std::move(other.m_font_override))
                , m_icon_style_override(std::move(other.m_icon_style_override))
                , m_opacity_override(std::move(other.m_opacity_override)) {}

            themeable& operator=(themeable&& other) noexcept {
                if (this != &other) {
                    m_theme = std::exchange(other.m_theme, nullptr);
                    m_background_override = std::move(other.m_background_override);
                    m_foreground_override = std::move(other.m_foreground_override);
                    m_box_style_override = std::move(other.m_box_style_override);
                    m_font_override = std::move(other.m_font_override);
                    m_icon_style_override = std::move(other.m_icon_style_override);
                    m_opacity_override = std::move(other.m_opacity_override);
                }
                return *this;
            }

            void apply_theme(const theme_type& theme) {
                before_apply_theme(theme);
                set_theme_internal(&theme);
                this->do_apply_theme(theme);
                after_apply_theme(theme);
            }

            // ===================================================================
            // Background Color (Inheritable)
            // ===================================================================

            /**
             * @brief Set explicit background color (overrides inheritance)
             */
            void set_background_color(color_type color) {
                m_background_override = color;
                invalidate_visual();
            }

            /**
             * @brief Clear background color override (back to inheritance)
             */
            void clear_background_color() {
                m_background_override.reset();
                invalidate_visual();
            }

            /**
             * @brief Get effective background color with inheritance
             *
             * Resolution: override → parent → theme → global fallback
             */
            [[nodiscard]] color_type get_effective_background_color() const {
                if (m_background_override) {
                    return *m_background_override;
                }

                if (auto* p = get_themeable_parent()) {
                    return p->get_effective_background_color();
                }

                if (auto* theme = get_theme()) {
                    return get_theme_background_color(*theme);
                }

                return color_type{};  // Fallback: default-constructed color
            }

            // ===================================================================
            // Foreground/Text Color (Inheritable)
            // ===================================================================

            void set_foreground_color(color_type color) {
                m_foreground_override = color;
                invalidate_visual();
            }

            void clear_foreground_color() {
                m_foreground_override.reset();
                invalidate_visual();
            }

            [[nodiscard]] color_type get_effective_foreground_color() const {
                if (m_foreground_override) {
                    return *m_foreground_override;
                }

                if (auto* p = get_themeable_parent()) {
                    return p->get_effective_foreground_color();
                }

                if (auto* theme = get_theme()) {
                    return get_theme_foreground_color(*theme);
                }

                return color_type{};
            }

            // ===================================================================
            // Box Style (Inheritable) - from renderer
            // ===================================================================

            /**
             * @brief Set box style for borders (single/double/rounded)
             *
             * @details Uses renderer's box_style type. This defines how
             *          borders and boxes are drawn (line style, corners, etc.)
             */
            void set_box_style(box_style_type style) {
                m_box_style_override = style;
                invalidate_visual();
            }

            void clear_box_style() {
                m_box_style_override.reset();
                invalidate_visual();
            }

            [[nodiscard]] box_style_type get_effective_box_style() const {
                if (m_box_style_override) {
                    return *m_box_style_override;
                }

                if (auto* p = get_themeable_parent()) {
                    return p->get_effective_box_style();
                }

                if (auto* theme = get_theme()) {
                    return get_theme_box_style(*theme);
                }

                return box_style_type{};
            }

            // ===================================================================
            // Font (Inheritable) - from renderer
            // ===================================================================

            /**
             * @brief Set font (normal, bold, underline, etc.)
             *
             * @details Uses renderer's font type. In TUI: controls attributes
             *          like bold, underline, reverse. In GUI: actual font face.
             */
            void set_font(font_type font) {
                m_font_override = font;
                invalidate_visual();
            }

            void clear_font() {
                m_font_override.reset();
                invalidate_visual();
            }

            [[nodiscard]] font_type get_effective_font() const {
                if (m_font_override) {
                    return *m_font_override;
                }

                if (auto* p = get_themeable_parent()) {
                    return p->get_effective_font();
                }

                if (auto* theme = get_theme()) {
                    return get_theme_font(*theme);
                }

                return font_type{};
            }

            // ===================================================================
            // Icon Style (Inheritable) - from renderer
            // ===================================================================

            /**
             * @brief Set icon style
             *
             * @details Uses renderer's icon_style type for drawing icons/glyphs.
             */
            void set_icon_style(icon_style_type style) {
                m_icon_style_override = style;
                invalidate_visual();
            }

            void clear_icon_style() {
                m_icon_style_override.reset();
                invalidate_visual();
            }

            [[nodiscard]] icon_style_type get_effective_icon_style() const {
                if (m_icon_style_override) {
                    return *m_icon_style_override;
                }

                if (auto* p = get_themeable_parent()) {
                    return p->get_effective_icon_style();
                }

                if (auto* theme = get_theme()) {
                    return get_theme_icon_style(*theme);
                }

                return icon_style_type{};
            }

            // ===================================================================
            // Opacity (Inheritable, Multiplicative)
            // ===================================================================

            /**
             * @brief Set opacity (0.0 = transparent, 1.0 = opaque)
             *
             * @details Useful for dimming disabled widgets. Opacity is
             *          multiplicative: parent=0.5, child=0.8 → effective=0.4
             */
            void set_opacity(float opacity) {
                m_opacity_override = opacity;
                invalidate_visual();
            }

            void clear_opacity() {
                m_opacity_override.reset();
                invalidate_visual();
            }

            /**
             * @brief Get effective opacity (multiplies with parent's)
             */
            [[nodiscard]] float get_effective_opacity() const {
                float opacity = m_opacity_override.value_or(1.0f);

                // Multiply with parent's opacity (CSS-style)
                if (auto* p = get_themeable_parent()) {
                    opacity *= p->get_effective_opacity();
                }

                return opacity;
            }

            // ===================================================================
            // Theme Access (Public API)
            // ===================================================================

            /**
             * @brief Get the effective theme for this element
             *
             * @details Implements CSS-style inheritance by walking up the parent
             *          chain to find the nearest theme. This ensures widgets without
             *          an explicitly set theme can still access theme data.
             *
             * @return Pointer to theme, or nullptr if no theme in hierarchy
             */
            [[nodiscard]] const theme_type* get_theme() const {
                // First check if we have a theme directly set
                if (m_theme) {
                    return m_theme;
                }

                // Walk up the parent chain to find a theme
                if (auto* parent = get_themeable_parent()) {
                    return parent->get_theme();
                }

                // No theme found in hierarchy
                return nullptr;
            }

            /**
             * @brief Check if this element has a theme (either direct or inherited)
             */
            [[nodiscard]] bool has_theme() const {
                return get_theme() != nullptr;
            }

        protected:
            themeable() = default;

            // Internal theme setter (for apply_theme only)
            void set_theme_internal(const theme_type* theme) {
                m_theme = theme;
            }

            // Theme application hooks
            virtual void do_apply_theme(const theme_type& theme) = 0;
            virtual void before_apply_theme([[maybe_unused]] const theme_type& theme) {}
            virtual void after_apply_theme([[maybe_unused]] const theme_type& theme) {}

            // ===================================================================
            // Inheritance Hooks (override in derived classes)
            // ===================================================================

            /**
             * @brief Get parent for inheritance chain
             *
             * @details Override in ui_element to provide parent pointer
             */
            [[nodiscard]] virtual const themeable* get_themeable_parent() const {
                return nullptr;
            }

            /**
             * @brief Get widget-type-specific background from theme
             *
             * @details Override in widgets:
             * - button: theme.button.bg_normal
             * - panel: theme.panel.background
             * - label: theme.label.background
             */
            [[nodiscard]] virtual color_type get_theme_background_color(const theme_type& theme) const {
                return theme.window_bg;
            }

            [[nodiscard]] virtual color_type get_theme_foreground_color(const theme_type& theme) const {
                return theme.text_fg;
            }

            [[nodiscard]] virtual box_style_type get_theme_box_style(const theme_type& theme) const {
                return theme.panel.box_style;  // Reasonable default
            }

            [[nodiscard]] virtual font_type get_theme_font(const theme_type& theme) const {
                return theme.label.font;  // Most widgets display text
            }

            [[nodiscard]] virtual icon_style_type get_theme_icon_style([[maybe_unused]] const theme_type& theme) const {
                return icon_style_type{};  // Not all widgets use icons
            }

            /**
             * @brief Called when visual properties change
             *
             * @details Override in ui_element to trigger redraw
             */
            virtual void invalidate_visual() {}

            // NOLINTBEGIN(cppcoreguidelines-non-private-member-variables-in-classes)
            // Note: Protected members are intentional for this base class design.
            // Derived classes (ui_element) need direct access for CSS-style inheritance.

        private:
            // Theme reference (access via get_theme() for proper inheritance)
            const theme_type* m_theme = nullptr;

        protected:

            // Property overrides (optional = use inheritance)
            std::optional<color_type> m_background_override;
            std::optional<color_type> m_foreground_override;
            std::optional<box_style_type> m_box_style_override;
            std::optional<font_type> m_font_override;
            std::optional<icon_style_type> m_icon_style_override;
            std::optional<float> m_opacity_override;

            // NOLINTEND(cppcoreguidelines-non-private-member-variables-in-classes)
    };
}
