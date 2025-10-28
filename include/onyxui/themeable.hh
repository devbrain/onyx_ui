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

    // Forward declarations to break circular dependency
    template<UIBackend Backend>
    class ui_services;
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
     * - **Renderer styles**: font, icon_style
     * - **Effects**: opacity/alpha (multiplicative)
     *
     * ## Non-Inheritable Properties
     *
     * These do NOT inherit from parent (CSS-compliant):
     * - **box_style (borders)** - Each widget has independent borders
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

            // Move operations (safe: transfers overrides only, theme is global)
            themeable(themeable&& other) noexcept
                : m_background_override(std::move(other.m_background_override))
                , m_foreground_override(std::move(other.m_foreground_override))
                , m_box_style_override(std::move(other.m_box_style_override))
                , m_font_override(std::move(other.m_font_override))
                , m_icon_style_override(std::move(other.m_icon_style_override))
                , m_opacity_override(std::move(other.m_opacity_override)) {}

            themeable& operator=(themeable&& other) noexcept {
                if (this != &other) {
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

            [[nodiscard]] box_style_type get_effective_box_style(const theme_type* theme) const {
                if (m_box_style_override) {
                    return *m_box_style_override;
                }

                if (auto* p = get_themeable_parent()) {
                    return p->get_effective_box_style(theme);
                }

                if (theme) {
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

            [[nodiscard]] font_type get_effective_font(const theme_type* theme) const {
                if (m_font_override) {
                    return *m_font_override;
                }

                if (auto* p = get_themeable_parent()) {
                    return p->get_effective_font(theme);
                }

                if (theme) {
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

            [[nodiscard]] icon_style_type get_effective_icon_style(const theme_type* theme) const {
                if (m_icon_style_override) {
                    return *m_icon_style_override;
                }

                if (auto* p = get_themeable_parent()) {
                    return p->get_effective_icon_style(theme);
                }

                if (theme) {
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
            [[nodiscard]] float get_effective_opacity(const resolved_style<Backend>& parent_style) const {
                float opacity = m_opacity_override.value_or(1.0F);

                // Multiply with parent's opacity (CSS-style multiplicative composition, NO RECURSION!)
                opacity *= parent_style.opacity.value;

                return opacity;
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
             *     auto* theme = get_global_theme();  // Get once at entry point
             *     auto style = resolve_style(theme); // Pass down tree
             *     draw_context ctx(r, style);        // Pass resolved style
             *     this->do_render(ctx);              // Widget just draws!
             * }
             * @endcode
             *
             * @param theme Global theme pointer (passed down from root), defaults to nullptr
             * @param parent_style Parent's resolved style (accumulated top-down), default-constructed if not provided
             * @return Fully-resolved style ready for rendering
             */
            /**
             * @brief Resolve complete style from theme, parent, and overrides
             *
             * @param theme Theme reference (REQUIRED - must be valid)
             * @param parent_style Parent's resolved style (passed down from root)
             * @return Fully resolved style with all properties
             *
             * @details
             * Resolution order per-property:
             * 1. Explicit override (set_background_color, etc.) - highest priority
             * 2. Parent inheritance (CSS-style, passed down top-to-bottom)
             * 3. Theme default (widget-specific via get_theme_style())
             *
             * Special case: Opacity is multiplicative through the hierarchy.
             *
             * @note Zero recursion! Parent style is passed down, not looked up.
             */
            [[nodiscard]] virtual resolved_style<Backend> resolve_style(
                const theme_type& theme,
                const resolved_style<Backend>& parent_style
            ) const {
                // Start with theme style (widget-specific via polymorphism)
                resolved_style<Backend> style = get_theme_style(theme);

                // Apply parent inheritance per-property (CSS-style)
                // Parent values override theme values (parent already accumulated from ancestors)
                // Note: We don't check for "default" because backend types are opaque and may not have operator==
                // The root element gets theme values, so this always works correctly.

                // BUG FIX: Stateful widgets (menu_item, menu_bar_item, button) manage their own
                // state-based colors and should NOT inherit from parents. Check if widget wants inheritance.
                if (should_inherit_colors()) {
                    style.background_color = parent_style.background_color.value;
                    style.foreground_color = parent_style.foreground_color.value;
                }
                // box_style is NOT inherited (borders don't inherit in CSS)
                // style.box_style = parent_style.box_style.value;  // REMOVED - breaks CSS compliance

                // Font inheritance (all widgets inherit fonts)
                style.font = parent_style.font.value;

                // Icon style is optional - only inherit if parent has one
                if (parent_style.icon_style.value.has_value()) {
                    style.icon_style = std::make_optional(parent_style.icon_style.value.value());  // Wrap in optional for assignment
                }

                // Layout properties are NOT inherited (widget-specific, set by get_theme_style())
                // padding_horizontal, padding_vertical, mnemonic_font stay as-is from get_theme_style()

                // Apply explicit overrides (highest priority)
                if (m_background_override) {
                    style.background_color = *m_background_override;
                }
                if (m_foreground_override) {
                    style.foreground_color = *m_foreground_override;
                }
                if (m_box_style_override) {
                    style.box_style = *m_box_style_override;
                }
                if (m_font_override) {
                    style.font = *m_font_override;
                }
                if (m_icon_style_override) {
                    style.icon_style = std::make_optional(*m_icon_style_override);
                }

                // Opacity is multiplicative through hierarchy
                style.opacity = get_effective_opacity(parent_style);

                // Border color defaults to foreground
                style.border_color = style.foreground_color.value;

                return style;
            }

        protected:
            themeable() = default;

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
             * @brief Check if widget should inherit colors from parent
             * @return true if colors should inherit (default), false otherwise
             *
             * @details
             * Stateful widgets (menu_item, menu_bar_item, button) manage their own
             * state-based colors (normal/hover/pressed/etc) and should NOT inherit
             * background/foreground from parents, as this would override their state colors.
             *
             * Non-stateful widgets (label, panel) should inherit colors from parents
             * for consistent styling (CSS-style inheritance).
             *
             * @note Override to return false in stateful widgets
             */
            [[nodiscard]] virtual bool should_inherit_colors() const {
                return true;  // Default: inherit colors (normal CSS behavior)
            }

            /**
             * @brief Get widget-type-specific complete style from theme
             *
             * @details Override in widgets to return widget-specific theme properties:
             * - button: returns theme.button colors, box_style, font
             * - panel: returns theme.panel colors, box_style
             * - label: returns theme.label colors, font
             *
             * @param theme Theme to extract properties from
             * @return Complete resolved_style with all widget-specific theme values
             *
             * @note Base implementation returns generic panel/label defaults
             */
            [[nodiscard]] virtual resolved_style<Backend> get_theme_style(const theme_type& theme) const {
                // Base implementation: generic widget style from theme
                return resolved_style<Backend>{
                    .background_color = theme.window_bg,
                    .foreground_color = theme.text_fg,
                    .border_color = theme.border_color,
                    .box_style = theme.panel.box_style,   // Default to panel
                    .font = theme.label.font,              // Default to label
                    .opacity = 1.0f,
                    .icon_style = std::optional<icon_style_type>(std::nullopt),  // No icon by default
                    .padding_horizontal = std::optional<int>{},  // No padding by default
                    .padding_vertical = std::optional<int>{},
                    .mnemonic_font = std::optional<font_type>{}  // No mnemonic by default
                };
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
            // Note: Theme is passed from the outside (via render pipeline)
            // No per-widget theme storage (zero overhead)

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

} // namespace onyxui
