#pragma once

#include <onyxui/theming/theme.hh>
#include <onyxui/theming/theme_registry.hh>
#include <onyxui/theming/theme_defaults.hh>
#include <onyxui/utils/color_utils.hh>
#include <onyxui/layout/layout_strategy.hh>
#include <string>
#include <string_view>
#include <unordered_map>
#include <optional>
#include <initializer_list>
#include <utility>
#include <stdexcept>

namespace onyxui {

/**
 * @file theme_builder.hh
 * @brief Fluent builder API for creating OnyxUI themes programmatically
 *
 * @details
 * Provides a type-safe, ergonomic API for theme creation with:
 * - Named color palettes (DRY principle)
 * - Theme inheritance (create variants easily)
 * - Smart defaults (auto-generate missing states)
 * - Nested widget builders (scoped configuration)
 * - Backend-agnostic (works with any UIBackend)
 *
 * Design Philosophy:
 * - Simple over clever (direct field access, no abstraction layers)
 * - Copy-paste friendly (mechanical patterns, easy to extend)
 * - Intentional coupling (builder's job is to build themes)
 *
 * @example Minimal Theme (3 colors)
 * @code
 * auto theme = theme_builder<Backend>::create("My Theme", "Description")
 *     .with_palette({
 *         {"window_bg", 0x0000AA},
 *         {"text_fg", 0xFFFFFF},
 *         {"border_color", 0xFFFF00}
 *     })
 *     .build();
 * // Smart defaults auto-generate 50+ values from these 3 colors!
 * @endcode
 *
 * @example Customized Theme
 * @code
 * auto theme = theme_builder<Backend>::create("Norton Blue", "Classic Norton")
 *     .with_palette({
 *         {"bg", 0x0000AA},
 *         {"fg", 0xFFFFFF},
 *         {"accent", 0xFFFF00}
 *     })
 *     .with_button()
 *         .padding(2, 0)
 *         .box_style(border_style::single_line, true)
 *         .normal()
 *             .foreground("fg")
 *             .background("bg")
 *             .end()
 *         .hover()
 *             .foreground("accent")
 *             .end()
 *     .build();
 * @endcode
 *
 * @example Theme Inheritance
 * @code
 * auto variant = theme_builder<Backend>::extend("Norton Blue", ctx.themes())
 *     .rename("Norton Blue Dark", "Darker variant")
 *     .override_color("bg", 0x000055)
 *     .build();
 * @endcode
 */
template<UIBackend Backend>
class theme_builder {
public:
    using theme_type = ui_theme<Backend>;
    using color_type = typename Backend::color_type;
    using renderer_type = typename Backend::renderer_type;

    // Forward declarations for nested builders
    class button_builder;
    class label_builder;
    class panel_builder;
    class menu_builder;
    class menu_item_builder;
    class menu_bar_builder;
    class separator_builder;
    class scrollbar_builder;
    class state_builder;

    // ===================================================================
    // Creation Strategies
    // ===================================================================

    /**
     * @brief Create new theme from scratch
     * @param name Theme name (required)
     * @param description Theme description (required)
     * @return Builder instance
     */
    static theme_builder create(std::string name, std::string description) {
        theme_builder builder;
        builder.m_theme.name = std::move(name);
        builder.m_theme.description = std::move(description);
        return builder;
    }

    /**
     * @brief Extend existing theme instance
     * @param base_theme Theme to copy and modify
     * @return Builder instance (inherits name/description from base)
     *
     * @details Extracts palette from base theme for reference.
     * Use with_name()/with_description() to customize.
     */
    static theme_builder from(theme_type base_theme) {
        theme_builder builder;
        builder.m_theme = std::move(base_theme);
        builder.extract_palette_from_theme();
        return builder;
    }

    /**
     * @brief Extend theme from registry by name
     * @param base_name Theme name in registry
     * @param registry Theme registry
     * @return Builder instance (inherits name/description from base)
     * @throws std::runtime_error if theme not found
     *
     * @details Looks up theme by name and extends it.
     * Use with_name()/with_description() to customize.
     */
    static theme_builder extend(std::string base_name,
                                 const theme_registry<Backend>& registry) {
        auto const* base = registry.get_theme(base_name);
        if (!base) {
            throw std::runtime_error("Base theme '" + base_name + "' not found in registry");
        }

        auto builder = from(*base);
        builder.m_base_theme_name = std::move(base_name);
        return builder;
    }

    // ===================================================================
    // Name/Description Setters
    // ===================================================================

    /**
     * @brief Set theme name
     * @param name New theme name
     * @return *this (for chaining)
     */
    theme_builder& with_name(std::string name) {
        m_theme.name = std::move(name);
        return *this;
    }

    /**
     * @brief Set theme description
     * @param description New description
     * @return *this (for chaining)
     */
    theme_builder& with_description(std::string description) {
        m_theme.description = std::move(description);
        return *this;
    }

    /**
     * @brief Set both name and description
     * @param name New theme name
     * @param description New description
     * @return *this (for chaining)
     */
    theme_builder& rename(std::string name, std::string description) {
        m_theme.name = std::move(name);
        m_theme.description = std::move(description);
        return *this;
    }

    // ===================================================================
    // Palette Management
    // ===================================================================

    /**
     * @brief Define named color palette
     * @param colors Initializer list of {name, hex} pairs
     * @return *this (for chaining)
     *
     * @details Reserved names auto-populate theme globals:
     * - "window_bg" → theme.window_bg
     * - "text_fg" → theme.text_fg
     * - "border_color" → theme.border_color
     *
     * @example
     * @code
     * .with_palette({
     *     {"bg", 0x0000AA},
     *     {"fg", 0xFFFFFF},
     *     {"accent", 0xFFFF00}
     * })
     * @endcode
     */
    theme_builder& with_palette(std::initializer_list<std::pair<std::string, std::uint32_t>> colors) {
        for (auto const& [name, hex] : colors) {
            auto rgb = color_utils::parse_hex_rgb(hex);
            m_palette[name] = color_type{rgb.r, rgb.g, rgb.b};
        }

        update_global_colors_from_palette();
        return *this;
    }

    /**
     * @brief Simple 3-color palette (auto-names: "window_bg", "text_fg", "border_color")
     * @param window_bg Window background hex (0xRRGGBB)
     * @param text_fg Text foreground hex (0xRRGGBB)
     * @param border Border/accent hex (0xRRGGBB)
     * @return *this (for chaining)
     */
    theme_builder& with_palette(std::uint32_t window_bg, std::uint32_t text_fg, std::uint32_t border) {
        return with_palette({
            {"window_bg", window_bg},
            {"text_fg", text_fg},
            {"border_color", border}
        });
    }

    /**
     * @brief Add/override single color in palette
     * @param name Color name
     * @param hex Hex color value (0xRRGGBB)
     * @return *this (for chaining)
     */
    theme_builder& add_color(std::string name, std::uint32_t hex) {
        auto rgb = color_utils::parse_hex_rgb(hex);
        m_palette[name] = color_type{rgb.r, rgb.g, rgb.b};
        update_global_colors_from_palette();
        return *this;
    }

    /**
     * @brief Override specific palette color
     * @param name Color name
     * @param hex New hex value (0xRRGGBB)
     * @return *this (for chaining)
     */
    theme_builder& override_color(std::string name, std::uint32_t hex) {
        return add_color(std::move(name), hex);
    }

    /**
     * @brief Resolve color from palette by name
     * @param name Color name
     * @return Color value
     * @throws std::runtime_error if color not found
     */
    [[nodiscard]] color_type resolve_color(std::string_view name) const {
        auto it = m_palette.find(std::string(name));
        if (it == m_palette.end()) {
            throw std::runtime_error("Color '" + std::string(name) + "' not found in palette");
        }
        return it->second;
    }

    /**
     * @brief Resolve color from hex value
     * @param hex Hex color (0xRRGGBB)
     * @return Color value
     */
    [[nodiscard]] color_type resolve_color(std::uint32_t hex) const {
        auto rgb = color_utils::parse_hex_rgb(hex);
        return color_type{rgb.r, rgb.g, rgb.b};
    }

    // ===================================================================
    // Widget Builders
    // ===================================================================

    /**
     * @brief Configure button widget
     * @return button_builder (implicit conversion back to theme_builder)
     */
    button_builder with_button();

    /**
     * @brief Configure label widget
     * @return label_builder (implicit conversion back to theme_builder)
     */
    label_builder with_label();

    /**
     * @brief Configure panel widget
     * @return panel_builder (implicit conversion back to theme_builder)
     */
    panel_builder with_panel();

    /**
     * @brief Configure menu widget
     * @return menu_builder (implicit conversion back to theme_builder)
     */
    menu_builder with_menu();

    /**
     * @brief Configure menu item widget
     * @return menu_item_builder (implicit conversion back to theme_builder)
     */
    menu_item_builder with_menu_item();

    /**
     * @brief Configure menu bar widget
     * @return menu_bar_builder (implicit conversion back to theme_builder)
     */
    menu_bar_builder with_menu_bar();

    /**
     * @brief Configure separator widget
     * @return separator_builder (implicit conversion back to theme_builder)
     */
    separator_builder with_separator();

    /**
     * @brief Configure scrollbar widget
     * @return scrollbar_builder (implicit conversion back to theme_builder)
     */
    scrollbar_builder with_scrollbar();

    // ===================================================================
    // Build
    // ===================================================================

    /**
     * @brief Build final theme with smart defaults
     * @return Complete ui_theme<Backend>
     *
     * @details Applies theme_defaults::apply_defaults() to auto-generate:
     * - Button states (normal, hover, pressed, disabled)
     * - Label styles
     * - Panel/menu styles
     * - Menu item states
     * - Scrollbar components
     *
     * Also sets non-color defaults (mnemonic fonts, padding, etc.)
     */
    theme_type build();

private:
    theme_type m_theme;
    std::unordered_map<std::string, color_type> m_palette;
    std::optional<std::string> m_base_theme_name;

    /**
     * @brief Extract palette from existing theme (best-effort)
     * @details Called by from() to populate palette from base theme
     */
    void extract_palette_from_theme() {
        m_palette["window_bg"] = m_theme.window_bg;
        m_palette["text_fg"] = m_theme.text_fg;
        m_palette["border_color"] = m_theme.border_color;
    }

    /**
     * @brief Update theme global colors from reserved palette names
     */
    void update_global_colors_from_palette() {
        if (auto it = m_palette.find("window_bg"); it != m_palette.end()) {
            m_theme.window_bg = it->second;
        }
        if (auto it = m_palette.find("text_fg"); it != m_palette.end()) {
            m_theme.text_fg = it->second;
        }
        if (auto it = m_palette.find("border_color"); it != m_palette.end()) {
            m_theme.border_color = it->second;
        }
    }

    // Friend all nested builders for direct theme access
    friend class button_builder;
    friend class label_builder;
    friend class panel_builder;
    friend class menu_builder;
    friend class menu_item_builder;
    friend class menu_bar_builder;
    friend class separator_builder;
    friend class scrollbar_builder;
    friend class state_builder;
};

// ===========================================================================
// State Builder (for visual states: normal, hover, pressed, disabled)
// ===========================================================================

template<UIBackend Backend>
class theme_builder<Backend>::state_builder {
    using visual_state = typename ui_theme<Backend>::visual_state;
    using parent_type = void*;  // Type-erased parent (button_builder, menu_item_builder, etc.)

    theme_builder& m_root;
    visual_state& m_state;
    parent_type m_parent;

public:
    /**
     * @brief Construct state builder
     * @param root Root theme_builder (for palette access)
     * @param state Reference to visual_state to modify
     * @param parent Parent builder (for .end() chaining)
     */
    template<typename ParentBuilder>
    state_builder(ParentBuilder& parent, visual_state& state)
        : m_root(parent.m_parent)
        , m_state(state)
        , m_parent(static_cast<void*>(&parent))
    {}

    /**
     * @brief Set foreground color by palette name
     * @param color_name Color name from palette
     * @return *this (for chaining)
     */
    state_builder& foreground(std::string_view color_name) {
        m_state.foreground = m_root.resolve_color(color_name);
        return *this;
    }

    /**
     * @brief Set foreground color by hex value
     * @param hex Hex color (0xRRGGBB)
     * @return *this (for chaining)
     */
    state_builder& foreground(std::uint32_t hex) {
        m_state.foreground = m_root.resolve_color(hex);
        return *this;
    }

    /**
     * @brief Set background color by palette name
     * @param color_name Color name from palette
     * @return *this (for chaining)
     */
    state_builder& background(std::string_view color_name) {
        m_state.background = m_root.resolve_color(color_name);
        return *this;
    }

    /**
     * @brief Set background color by hex value
     * @param hex Hex color (0xRRGGBB)
     * @return *this (for chaining)
     */
    state_builder& background(std::uint32_t hex) {
        m_state.background = m_root.resolve_color(hex);
        return *this;
    }

    /**
     * @brief Set font (perfect forwarding to backend font constructor)
     * @param args Constructor arguments for Backend::renderer_type::font
     * @return *this (for chaining)
     *
     * @example Conio (struct with bools)
     * @code
     * .font(true, false, false)  // bold, no underline, no reverse
     * @endcode
     */
    template<typename... Args>
    state_builder& font(Args&&... args) {
        m_state.font = typename Backend::renderer_type::font{std::forward<Args>(args)...};
        return *this;
    }

    /**
     * @brief Return to parent builder (explicit end)
     * @return Parent builder
     *
     * @note Type-erased, so returns void* - caller must know parent type
     * In practice, this is only called from known contexts (button_builder, etc.)
     */
    template<typename ParentBuilder>
    ParentBuilder& end() {
        return *static_cast<ParentBuilder*>(m_parent);
    }
};

// ===========================================================================
// Button Builder
// ===========================================================================

template<UIBackend Backend>
class theme_builder<Backend>::button_builder {
public:
    theme_builder& m_parent;  // Public for state_builder access

    explicit button_builder(theme_builder& parent) : m_parent(parent) {}

    // ===== Widget-Level Settings =====

    /**
     * @brief Set button padding
     * @param horizontal Horizontal padding
     * @param vertical Vertical padding
     * @return *this (for chaining)
     */
    button_builder& padding(int horizontal, int vertical) {
        m_parent.m_theme.button.padding_horizontal = horizontal;
        m_parent.m_theme.button.padding_vertical = vertical;
        return *this;
    }

    /**
     * @brief Set box style (perfect forwarding)
     * @param args Constructor arguments for Backend::renderer_type::box_style
     * @return *this (for chaining)
     */
    template<typename... Args>
    button_builder& box_style(Args&&... args) {
        m_parent.m_theme.button.box_style =
            typename Backend::renderer_type::box_style{std::forward<Args>(args)...};
        m_parent.m_theme.panel.box_style = m_parent.m_theme.button.box_style;
        m_parent.m_theme.menu.box_style = m_parent.m_theme.button.box_style;
        return *this;
    }

    /**
     * @brief Set text alignment
     * @param align Horizontal alignment
     * @return *this (for chaining)
     */
    button_builder& text_align(horizontal_alignment align) {
        m_parent.m_theme.button.text_align = align;
        return *this;
    }

    /**
     * @brief Set mnemonic font (perfect forwarding)
     * @param args Constructor arguments for Backend::renderer_type::font
     * @return *this (for chaining)
     */
    template<typename... Args>
    button_builder& mnemonic_font(Args&&... args) {
        m_parent.m_theme.button.mnemonic_font =
            typename Backend::renderer_type::font{std::forward<Args>(args)...};
        return *this;
    }

    // ===== Quick Color Helpers =====

    /**
     * @brief Set normal state colors by palette name
     * @param fg_name Foreground color name
     * @param bg_name Background color name
     * @return *this (for chaining)
     */
    button_builder& colors(std::string_view fg_name, std::string_view bg_name) {
        m_parent.m_theme.button.normal.foreground = m_parent.resolve_color(fg_name);
        m_parent.m_theme.button.normal.background = m_parent.resolve_color(bg_name);
        return *this;
    }

    /**
     * @brief Set hover state colors by palette name
     * @param fg_name Foreground color name
     * @param bg_name Background color name
     * @return *this (for chaining)
     */
    button_builder& hover_colors(std::string_view fg_name, std::string_view bg_name) {
        m_parent.m_theme.button.hover.foreground = m_parent.resolve_color(fg_name);
        m_parent.m_theme.button.hover.background = m_parent.resolve_color(bg_name);
        m_parent.m_theme.button.hover.font.bold = true;
        return *this;
    }

    // ===== State Builders (Advanced) =====

    /**
     * @brief Configure normal state
     * @return state_builder (explicit .end() required)
     */
    state_builder normal() {
        return state_builder{*this, m_parent.m_theme.button.normal};
    }

    /**
     * @brief Configure hover state
     * @return state_builder (explicit .end() required)
     */
    state_builder hover() {
        return state_builder{*this, m_parent.m_theme.button.hover};
    }

    /**
     * @brief Configure pressed state
     * @return state_builder (explicit .end() required)
     */
    state_builder pressed() {
        return state_builder{*this, m_parent.m_theme.button.pressed};
    }

    /**
     * @brief Configure disabled state
     * @return state_builder (explicit .end() required)
     */
    state_builder disabled() {
        return state_builder{*this, m_parent.m_theme.button.disabled};
    }

    // ===== Implicit Conversion =====

    /**
     * @brief Implicit conversion back to theme_builder
     * @return Parent builder
     */
    operator theme_builder&() {
        return m_parent;
    }
};

// ===========================================================================
// Label Builder
// ===========================================================================

template<UIBackend Backend>
class theme_builder<Backend>::label_builder {
public:
    theme_builder& m_parent;  // Public for state_builder access

    explicit label_builder(theme_builder& parent) : m_parent(parent) {}

    /**
     * @brief Set label text color by palette name
     * @param color_name Color name
     * @return *this (for chaining)
     */
    label_builder& text_color(std::string_view color_name) {
        m_parent.m_theme.label.text = m_parent.resolve_color(color_name);
        return *this;
    }

    /**
     * @brief Set label background color by palette name
     * @param color_name Color name
     * @return *this (for chaining)
     */
    label_builder& background_color(std::string_view color_name) {
        m_parent.m_theme.label.background = m_parent.resolve_color(color_name);
        return *this;
    }

    /**
     * @brief Set font (perfect forwarding)
     * @param args Constructor arguments for Backend::renderer_type::font
     * @return *this (for chaining)
     */
    template<typename... Args>
    label_builder& font(Args&&... args) {
        m_parent.m_theme.label.font =
            typename Backend::renderer_type::font{std::forward<Args>(args)...};
        return *this;
    }

    /**
     * @brief Set mnemonic font (perfect forwarding)
     * @param args Constructor arguments for Backend::renderer_type::font
     * @return *this (for chaining)
     */
    template<typename... Args>
    label_builder& mnemonic_font(Args&&... args) {
        m_parent.m_theme.label.mnemonic_font =
            typename Backend::renderer_type::font{std::forward<Args>(args)...};
        return *this;
    }

    /**
     * @brief Implicit conversion back to theme_builder
     */
    operator theme_builder&() {
        return m_parent;
    }
};

// ===========================================================================
// Panel Builder
// ===========================================================================

template<UIBackend Backend>
class theme_builder<Backend>::panel_builder {
public:
    theme_builder& m_parent;  // Public for state_builder access

    explicit panel_builder(theme_builder& parent) : m_parent(parent) {}

    /**
     * @brief Set panel background color by palette name
     * @param color_name Color name
     * @return *this (for chaining)
     */
    panel_builder& background_color(std::string_view color_name) {
        m_parent.m_theme.panel.background = m_parent.resolve_color(color_name);
        return *this;
    }

    /**
     * @brief Set panel border color by palette name
     * @param color_name Color name
     * @return *this (for chaining)
     */
    panel_builder& border_color(std::string_view color_name) {
        m_parent.m_theme.panel.border_color = m_parent.resolve_color(color_name);
        return *this;
    }

    /**
     * @brief Set box style (perfect forwarding)
     * @param args Constructor arguments for Backend::renderer_type::box_style
     * @return *this (for chaining)
     */
    template<typename... Args>
    panel_builder& box_style(Args&&... args) {
        m_parent.m_theme.panel.box_style =
            typename Backend::renderer_type::box_style{std::forward<Args>(args)...};
        return *this;
    }

    /**
     * @brief Implicit conversion back to theme_builder
     */
    operator theme_builder&() {
        return m_parent;
    }
};

// ===========================================================================
// Menu Builder
// ===========================================================================

template<UIBackend Backend>
class theme_builder<Backend>::menu_builder {
public:
    theme_builder& m_parent;  // Public for state_builder access

    explicit menu_builder(theme_builder& parent) : m_parent(parent) {}

    /**
     * @brief Set menu background color by palette name
     * @param color_name Color name
     * @return *this (for chaining)
     */
    menu_builder& background_color(std::string_view color_name) {
        m_parent.m_theme.menu.background = m_parent.resolve_color(color_name);
        return *this;
    }

    /**
     * @brief Set menu border color by palette name
     * @param color_name Color name
     * @return *this (for chaining)
     */
    menu_builder& border_color(std::string_view color_name) {
        m_parent.m_theme.menu.border_color = m_parent.resolve_color(color_name);
        return *this;
    }

    /**
     * @brief Set box style (perfect forwarding)
     * @param args Constructor arguments for Backend::renderer_type::box_style
     * @return *this (for chaining)
     */
    template<typename... Args>
    menu_builder& box_style(Args&&... args) {
        m_parent.m_theme.menu.box_style =
            typename Backend::renderer_type::box_style{std::forward<Args>(args)...};
        return *this;
    }

    /**
     * @brief Enable/disable shadow with offsets
     * @param enabled Enable shadow rendering
     * @param offset_x Horizontal shadow offset (cells right)
     * @param offset_y Vertical shadow offset (cells down)
     * @return *this (for chaining)
     *
     * @example
     * @code
     * builder.with_menu()
     *     .shadow(true, 1, 1);  // Classic DOS shadow
     * @endcode
     */
    menu_builder& shadow(bool enabled, int offset_x = 1, int offset_y = 1) {
        m_parent.m_theme.menu.shadow.enabled = enabled;
        m_parent.m_theme.menu.shadow.offset_x = offset_x;
        m_parent.m_theme.menu.shadow.offset_y = offset_y;
        return *this;
    }

    /**
     * @brief Implicit conversion back to theme_builder
     */
    operator theme_builder&() {
        return m_parent;
    }
};

// ===========================================================================
// Menu Item Builder
// ===========================================================================

template<UIBackend Backend>
class theme_builder<Backend>::menu_item_builder {
public:
    theme_builder& m_parent;  // Public for state_builder access

    explicit menu_item_builder(theme_builder& parent) : m_parent(parent) {}

    /**
     * @brief Set menu item padding
     * @param horizontal Horizontal padding
     * @param vertical Vertical padding
     * @return *this (for chaining)
     */
    menu_item_builder& padding(int horizontal, int vertical) {
        m_parent.m_theme.menu_item.padding_horizontal = horizontal;
        m_parent.m_theme.menu_item.padding_vertical = vertical;
        return *this;
    }

    /**
     * @brief Set mnemonic font (perfect forwarding)
     * @param args Constructor arguments for Backend::renderer_type::font
     * @return *this (for chaining)
     */
    template<typename... Args>
    menu_item_builder& mnemonic_font(Args&&... args) {
        m_parent.m_theme.menu_item.mnemonic_font =
            typename Backend::renderer_type::font{std::forward<Args>(args)...};
        return *this;
    }

    /**
     * @brief Configure normal state
     * @return state_builder (explicit .end() required)
     */
    state_builder normal() {
        return state_builder{*this, m_parent.m_theme.menu_item.normal};
    }

    /**
     * @brief Configure highlighted state
     * @return state_builder (explicit .end() required)
     */
    state_builder highlighted() {
        return state_builder{*this, m_parent.m_theme.menu_item.highlighted};
    }

    /**
     * @brief Configure disabled state
     * @return state_builder (explicit .end() required)
     */
    state_builder disabled() {
        return state_builder{*this, m_parent.m_theme.menu_item.disabled};
    }

    /**
     * @brief Configure shortcut hint state
     * @return state_builder (explicit .end() required)
     */
    state_builder shortcut() {
        return state_builder{*this, m_parent.m_theme.menu_item.shortcut};
    }

    /**
     * @brief Implicit conversion back to theme_builder
     */
    operator theme_builder&() {
        return m_parent;
    }
};

// ===========================================================================
// Menu Bar Builder
// ===========================================================================

template<UIBackend Backend>
class theme_builder<Backend>::menu_bar_builder {
public:
    theme_builder& m_parent;  // Public for state_builder access

    explicit menu_bar_builder(theme_builder& parent) : m_parent(parent) {}

    /**
     * @brief Set item spacing
     * @param spacing Spacing between menu bar items
     * @return *this (for chaining)
     */
    menu_bar_builder& item_spacing(int spacing) {
        m_parent.m_theme.menu_bar.item_spacing = spacing;
        return *this;
    }

    /**
     * @brief Set item padding
     * @param horizontal Horizontal padding
     * @param vertical Vertical padding
     * @return *this (for chaining)
     */
    menu_bar_builder& item_padding(int horizontal, int vertical) {
        m_parent.m_theme.menu_bar.item_padding_horizontal = horizontal;
        m_parent.m_theme.menu_bar.item_padding_vertical = vertical;
        return *this;
    }

    /**
     * @brief Implicit conversion back to theme_builder
     */
    operator theme_builder&() {
        return m_parent;
    }
};

// ===========================================================================
// Separator Builder
// ===========================================================================

template<UIBackend Backend>
class theme_builder<Backend>::separator_builder {
public:
    theme_builder& m_parent;  // Public for state_builder access

    explicit separator_builder(theme_builder& parent) : m_parent(parent) {}

    /**
     * @brief Set separator foreground color (line color)
     * @param color_name Palette color name or hex value
     * @return *this (for chaining)
     */
    separator_builder& foreground(std::string_view color_name) {
        m_parent.m_theme.separator.foreground = m_parent.resolve_color(color_name);
        return *this;
    }

    /**
     * @brief Set separator foreground color (line color) via hex
     * @param hex Hex color value (e.g., 0xFF0000 for red)
     * @return *this (for chaining)
     */
    separator_builder& foreground(std::uint32_t hex) {
        m_parent.m_theme.separator.foreground = m_parent.resolve_color(hex);
        return *this;
    }

    /**
     * @brief Implicit conversion back to theme_builder
     */
    operator theme_builder&() {
        return m_parent;
    }
};

// ===========================================================================
// Scrollbar Builder
// ===========================================================================

template<UIBackend Backend>
class theme_builder<Backend>::scrollbar_builder {
public:
    theme_builder& m_parent;  // Public for state_builder access

    explicit scrollbar_builder(theme_builder& parent) : m_parent(parent) {}

    /**
     * @brief Set scrollbar width (thickness)
     * @param width Width in pixels/chars
     * @return *this (for chaining)
     */
    scrollbar_builder& width(int width) {
        m_parent.m_theme.scrollbar.width = width;
        return *this;
    }

    /**
     * @brief Set minimum thumb size
     * @param size Minimum thumb size in pixels/chars
     * @return *this (for chaining)
     */
    scrollbar_builder& min_thumb_size(int size) {
        m_parent.m_theme.scrollbar.min_thumb_size = size;
        return *this;
    }

    /**
     * @brief Implicit conversion back to theme_builder
     */
    operator theme_builder&() {
        return m_parent;
    }
};

// ===========================================================================
// Widget Builder Factory Methods (inline implementations)
// ===========================================================================

template<UIBackend Backend>
typename theme_builder<Backend>::button_builder theme_builder<Backend>::with_button() {
    return button_builder{*this};
}

template<UIBackend Backend>
typename theme_builder<Backend>::label_builder theme_builder<Backend>::with_label() {
    return label_builder{*this};
}

template<UIBackend Backend>
typename theme_builder<Backend>::panel_builder theme_builder<Backend>::with_panel() {
    return panel_builder{*this};
}

template<UIBackend Backend>
typename theme_builder<Backend>::menu_builder theme_builder<Backend>::with_menu() {
    return menu_builder{*this};
}

template<UIBackend Backend>
typename theme_builder<Backend>::menu_item_builder theme_builder<Backend>::with_menu_item() {
    return menu_item_builder{*this};
}

template<UIBackend Backend>
typename theme_builder<Backend>::menu_bar_builder theme_builder<Backend>::with_menu_bar() {
    return menu_bar_builder{*this};
}

template<UIBackend Backend>
typename theme_builder<Backend>::separator_builder theme_builder<Backend>::with_separator() {
    return separator_builder{*this};
}

template<UIBackend Backend>
typename theme_builder<Backend>::scrollbar_builder theme_builder<Backend>::with_scrollbar() {
    return scrollbar_builder{*this};
}

// ===========================================================================
// Build Method (inline implementation)
// ===========================================================================

template<UIBackend Backend>
ui_theme<Backend> theme_builder<Backend>::build() {
    // Apply Phase 4 smart defaults (auto-generate missing states)
    theme_defaults::apply_defaults(m_theme);

    // Set remaining non-color defaults (if not already set)

    // Button defaults
    // Note: box_style setup is backend-specific and handled by theme_defaults

    if (m_theme.button.text_align == horizontal_alignment{}) {
        m_theme.button.text_align = horizontal_alignment::center;
    }

    // Mnemonic fonts (underlined)
    typename Backend::renderer_type::font mnemonic_font{};
    mnemonic_font.underline = true;

    m_theme.button.mnemonic_font = mnemonic_font;
    m_theme.label.mnemonic_font = mnemonic_font;
    m_theme.menu_item.mnemonic_font = mnemonic_font;
    m_theme.menu_bar_item.mnemonic_font = mnemonic_font;

    // Menu bar spacing defaults to 2 (from theme struct)
    // No override needed - 0 is a valid value for continuous menu bars

    // Menu item padding (if not set)
    if (m_theme.menu_item.padding_horizontal == 0) {
        m_theme.menu_item.padding_horizontal = 8;
        m_theme.menu_item.padding_vertical = 1;
    }

    // Menu bar item padding (if not set)
    if (m_theme.menu_bar_item.padding_horizontal == 0) {
        m_theme.menu_bar_item.padding_horizontal = 4;
        m_theme.menu_bar_item.padding_vertical = 0;
    }

    // Note: box_style defaults (borders, etc.) are backend-specific and handled by theme_defaults

    return std::move(m_theme);
}

} // namespace onyxui
