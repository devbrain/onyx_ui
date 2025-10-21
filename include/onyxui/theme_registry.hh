/**
 * @file theme_registry.hh
 * @brief Theme discovery and management
 * @author Assistant
 * @date 2025-10-20
 *
 * @details
 * Provides centralized theme registration and discovery.
 * Themes can be registered by backends or applications and accessed by name.
 *
 * ## Usage Example
 *
 * @code
 * // Register themes at startup
 * theme_registry<Backend> registry;
 *
 * // Register backend themes
 * conio_themes::register_default_themes(registry);
 *
 * // Register custom theme
 * ui_theme<Backend> custom;
 * custom.name = "My Theme";
 * custom.description = "Custom color scheme";
 * registry.register_theme(custom);
 *
 * // List available themes
 * for (const auto& name : registry.list_theme_names()) {
 *     std::cout << "- " << name << "\n";
 * }
 *
 * // Get and apply theme
 * if (auto* theme = registry.get_theme("Norton Blue")) {
 *     window->apply_theme(*theme);
 * }
 * @endcode
 */

#pragma once

#include <onyxui/theme.hh>
#include <unordered_map>
#include <vector>
#include <string>
#include <optional>
#include <algorithm>

namespace onyxui {

    /**
     * @class theme_registry
     * @brief Registry for theme discovery and management
     *
     * @tparam Backend The UI backend type
     *
     * @details
     * Maintains a collection of named themes that can be queried by name.
     * Themes are typically registered at application startup by:
     * - Backend providers (built-in themes)
     * - Application code (custom themes)
     * - User config files (saved themes)
     *
     * ## Thread Safety
     *
     * Not thread-safe. All access must be on UI thread.
     *
     * ## Lifetime
     *
     * Typically registered via ui_services at application startup and
     * cleared at application shutdown.
     */
    template<UIBackend Backend>
    class theme_registry {
    public:
        using theme_type = ui_theme<Backend>;

        /**
         * @brief Register a theme
         * @param theme Theme to register (must have non-empty name)
         *
         * @details
         * If a theme with the same name already exists, it will be replaced.
         * Themes without names (empty string) are ignored.
         *
         * @example
         * @code
         * ui_theme<Backend> dark_theme;
         * dark_theme.name = "Dark Mode";
         * dark_theme.description = "Easy on the eyes";
         * registry.register_theme(dark_theme);
         * @endcode
         */
        void register_theme(const theme_type& theme) {
            if (!theme.name.empty()) {
                m_themes[theme.name] = theme;
            }
        }

        /**
         * @brief Register a theme (move version)
         * @param theme Theme to register (must have non-empty name)
         */
        void register_theme(theme_type&& theme) {
            if (!theme.name.empty()) {
                std::string name = theme.name;  // Copy name before move
                m_themes[std::move(name)] = std::move(theme);
            }
        }

        /**
         * @brief Get a theme by name
         * @param name Theme name (case-sensitive)
         * @return Pointer to theme if found, nullptr otherwise
         *
         * @details
         * Returns a non-owning pointer to the theme stored in the registry.
         * The pointer is valid until the theme is unregistered or the registry is destroyed.
         *
         * @warning Do NOT store this pointer long-term. Use it immediately to apply the theme,
         *          then discard it. The themeable base class will store the pointer safely.
         *
         * @example
         * @code
         * if (auto* theme = registry.get_theme("Norton Blue")) {
         *     window->apply_theme(*theme);  // Safe: apply_theme stores the pointer
         * } else {
         *     std::cerr << "Theme not found\n";
         * }
         * @endcode
         */
        [[nodiscard]] const theme_type* get_theme(const std::string& name) const noexcept {
            auto it = m_themes.find(name);
            return (it != m_themes.end()) ? &it->second : nullptr;
        }

        /**
         * @brief List all registered theme names
         * @return Vector of theme names (sorted alphabetically)
         *
         * @example
         * @code
         * for (const auto& name : registry.list_theme_names()) {
         *     std::cout << "- " << name << "\n";
         * }
         * @endcode
         */
        [[nodiscard]] std::vector<std::string> list_theme_names() const {
            std::vector<std::string> names;
            names.reserve(m_themes.size());
            for (const auto& [name, theme] : m_themes) {
                names.push_back(name);
            }
            std::sort(names.begin(), names.end());
            return names;
        }

        /**
         * @brief Get all registered themes
         * @return Map of theme name → theme
         *
         * @details
         * Returns a const reference to the internal map. Useful for iteration
         * or bulk operations.
         *
         * @example
         * @code
         * for (const auto& [name, theme] : registry.all_themes()) {
         *     std::cout << name << ": " << theme.description << "\n";
         * }
         * @endcode
         */
        [[nodiscard]] const std::unordered_map<std::string, theme_type>& all_themes() const noexcept {
            return m_themes;
        }

        /**
         * @brief Check if a theme is registered
         * @param name Theme name
         * @return True if theme exists
         */
        [[nodiscard]] bool has_theme(const std::string& name) const noexcept {
            return m_themes.find(name) != m_themes.end();
        }

        /**
         * @brief Unregister a theme
         * @param name Theme name
         * @return True if theme was removed, false if not found
         */
        bool unregister_theme(const std::string& name) {
            return m_themes.erase(name) > 0;
        }

        /**
         * @brief Clear all registered themes
         */
        void clear() noexcept {
            m_themes.clear();
        }

        /**
         * @brief Get the number of registered themes
         * @return Theme count
         */
        [[nodiscard]] size_t size() const noexcept {
            return m_themes.size();
        }

        /**
         * @brief Check if registry is empty
         * @return True if no themes registered
         */
        [[nodiscard]] bool empty() const noexcept {
            return m_themes.empty();
        }

    private:
        std::unordered_map<std::string, theme_type> m_themes;
    };

} // namespace onyxui
