//
// Created by igor on 14/10/2025.
//

#pragma once

#include <optional>
#include <utility>  // for std::exchange
#include <memory>
#include <string>
#include <onyxui/theme.hh>
#include <onyxui/resolved_style.hh>
#include <failsafe/logger.hh>

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

            // Move operations (safe: transfers theme ownership and overrides)
            themeable(themeable&& other) noexcept
                : m_theme_raw(std::exchange(other.m_theme_raw, nullptr))
                , m_owned_theme(std::move(other.m_owned_theme))
                , m_theme_ptr(std::move(other.m_theme_ptr))
                , m_theme_name(std::move(other.m_theme_name))
                , m_background_override(std::move(other.m_background_override))
                , m_foreground_override(std::move(other.m_foreground_override))
                , m_box_style_override(std::move(other.m_box_style_override))
                , m_font_override(std::move(other.m_font_override))
                , m_icon_style_override(std::move(other.m_icon_style_override))
                , m_opacity_override(std::move(other.m_opacity_override)) {}

            themeable& operator=(themeable&& other) noexcept {
                if (this != &other) {
                    m_theme_raw = std::exchange(other.m_theme_raw, nullptr);
                    m_owned_theme = std::move(other.m_owned_theme);
                    m_theme_ptr = std::move(other.m_theme_ptr);
                    m_theme_name = std::move(other.m_theme_name);
                    m_background_override = std::move(other.m_background_override);
                    m_foreground_override = std::move(other.m_foreground_override);
                    m_box_style_override = std::move(other.m_box_style_override);
                    m_font_override = std::move(other.m_font_override);
                    m_icon_style_override = std::move(other.m_icon_style_override);
                    m_opacity_override = std::move(other.m_opacity_override);
                }
                return *this;
            }

            // ===================================================================
            // Theme Application - Three Safe Options
            // ===================================================================

            /**
             * @brief Apply theme by name from registry (Option 1 - Recommended)
             *
             * @param name Theme name (must be registered in theme_registry)
             * @param registry Theme registry to look up the theme
             * @return true if theme was found and applied, false otherwise
             *
             * @details
             * This is the safest and most efficient option:
             * - Zero overhead (registry owns the theme)
             * - Automatic lifetime management
             * - Perfect for app-wide standard themes
             *
             * @example
             * @code
             * if (widget->apply_theme("Norton Blue", ctx.themes())) {
             *     // Theme applied successfully
             * } else {
             *     std::cerr << "Theme not found\n";
             * }
             * @endcode
             */
            template<typename ThemeRegistry>
            bool apply_theme(const std::string& name, ThemeRegistry& registry) {
                const theme_type* theme = registry.get_theme(name);
                if (!theme) {
                    LOG_WARN("Theme not found in registry: ", name);
                    return false;
                }

                LOG_DEBUG("Applying theme by name: ", name);
                before_apply_theme(*theme);

                // Clear owned/shared themes (switching to registry theme)
                m_owned_theme.reset();
                m_theme_ptr.reset();
                m_theme_raw = theme;
                m_theme_name = name;

                this->do_apply_theme(*theme);
                after_apply_theme(*theme);
                return true;
            }

            /**
             * @brief Apply theme by value (Option 2 - Copy)
             *
             * @param theme Theme to apply (will be copied/moved)
             *
             * @details
             * Widget takes ownership of a copy:
             * - Pass by value (use std::move for zero-copy)
             * - Widget owns its own theme copy
             * - Perfect for per-widget customization
             *
             * @example
             * @code
             * // Copy
             * auto my_theme = create_theme();
             * widget->apply_theme(my_theme);
             *
             * // Move (zero-copy)
             * widget->apply_theme(std::move(my_theme));
             * @endcode
             */
            void apply_theme(theme_type theme) {  // Pass by value!
                LOG_DEBUG("Applying theme by value: ", theme.name.empty() ? "(unnamed)" : theme.name);
                before_apply_theme(theme);

                // Clear shared theme (switching to owned theme)
                m_theme_ptr.reset();
                m_owned_theme = std::make_unique<theme_type>(std::move(theme));
                m_theme_raw = m_owned_theme.get();
                m_theme_name = m_theme_raw->name;

                this->do_apply_theme(*m_owned_theme);
                after_apply_theme(*m_owned_theme);
            }

            /**
             * @brief Apply theme by shared_ptr (Option 3 - Shared)
             *
             * @param theme Shared pointer to theme
             *
             * @details
             * Multiple widgets share the same theme:
             * - Shared ownership with reference counting
             * - Efficient memory usage for shared themes
             * - Perfect for sharing custom themes across few widgets
             *
             * @example
             * @code
             * auto shared_theme = std::make_shared<ui_theme<Backend>>(create_theme());
             * widget1->apply_theme(shared_theme);
             * widget2->apply_theme(shared_theme);  // Both share same instance
             * @endcode
             */
            void apply_theme(std::shared_ptr<const theme_type> theme) {
                if (!theme) {
                    LOG_WARN("Cannot apply null shared_ptr theme");
                    return;
                }

                LOG_DEBUG("Applying theme by shared_ptr: ", theme->name.empty() ? "(unnamed)" : theme->name);
                before_apply_theme(*theme);

                // Clear owned theme (switching to shared theme)
                m_owned_theme.reset();
                m_theme_ptr = theme;
                m_theme_raw = theme.get();
                m_theme_name = m_theme_raw->name;

                this->do_apply_theme(*theme);
                after_apply_theme(*theme);
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
             * @brief Get effective opacity (multiplies with parent's opacity)
             * @details Opacity is multiplicative in CSS: parent=0.5, child=0.8 → effective=0.4
             *          This represents the VISUAL opacity, not just the CSS property value.
             */
            [[nodiscard]] float get_effective_opacity() const {
                float opacity = m_opacity_override.value_or(1.0F);

                // Multiply with parent's opacity (CSS-style multiplicative composition)
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
                if (m_theme_raw) {
                    return m_theme_raw;
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

            // ===================================================================
            // Style Resolution (v2.0)
            // ===================================================================

            /**
             * @brief Resolve all visual properties into a complete style
             *
             * @details
             * This is the core of the style-based rendering architecture.
             * It resolves ALL visual properties through CSS inheritance ONCE:
             * - Override → Parent → Theme → Default
             *
             * The resulting `resolved_style` contains everything a widget needs
             * to render, eliminating the need for repeated `get_effective_*()`
             * calls during rendering.
             *
             * ## Performance
             *
             * - Resolution: O(tree_height) - walks inheritance chain
             * - Rendering: O(1) - just uses resolved values
             * - Net win: Style resolved once, used many times during rendering
             *
             * ## Usage Pattern
             *
             * @code
             * // In ui_element::render()
             * void render(renderer_type& r) {
             *     auto style = resolve_style();     // CSS inheritance happens HERE
             *     draw_context ctx(r, style);       // Pass resolved style
             *     this->do_render(ctx);             // Widget just draws!
             * }
             * @endcode
             *
             * @return Fully-resolved style ready for rendering
             */
            [[nodiscard]] resolved_style<Backend> resolve_style() const {
                resolved_style<Backend> style;

                // Resolve all properties through CSS inheritance
                style.background_color = get_effective_background_color();
                style.foreground_color = get_effective_foreground_color();
                style.box_style = get_effective_box_style();
                style.font = get_effective_font();
                style.opacity = get_effective_opacity();
                style.icon_style = get_effective_icon_style();

                // Border color (defaults to foreground if not specified)
                style.border_color = get_effective_foreground_color();

                return style;
            }

        protected:
            themeable() = default;

            // Internal theme setter (for apply_theme only - deprecated, kept for compatibility)
            void set_theme_internal(const theme_type* theme) {
                m_theme_raw = theme;
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
            // Theme ownership (three-way API)
            const theme_type* m_theme_raw = nullptr;              ///< Non-owning pointer (always points to active theme)
            std::unique_ptr<theme_type> m_owned_theme;            ///< Option 2: owned copy
            std::shared_ptr<const theme_type> m_theme_ptr;        ///< Option 3: shared ownership
            std::string m_theme_name;                             ///< Track applied theme name

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
