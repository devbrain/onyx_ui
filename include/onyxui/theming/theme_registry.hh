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

#include <onyxui/theming/theme.hh>
#include <onyxui/core/signal.hh>
#include <failsafe/logger.hh>
#include <unordered_map>
#include <vector>
#include <string>
#include <optional>
#include <algorithm>
#include <shared_mutex>
#include <mutex>

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
     * **Thread-safe** (as of v2.0). Uses std::shared_mutex for reader-writer locking:
     * - Multiple threads can read simultaneously (get_theme, has_theme, list_theme_names)
     * - Write operations (register_theme, unregister_theme, clear) are exclusive
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
         * @brief Signal emitted when the current theme changes
         *
         * @details
         * Emitted by set_current_theme() when a new theme is activated.
         * Subscribers receive a pointer to the new theme (can be nullptr if theme is cleared).
         *
         * **Use Case:** Automatic synchronization between theme and background renderer.
         *
         * @example
         * @code
         * // Subscribe to theme changes
         * ctx.themes().theme_changed.connect([&bg](const ui_theme<Backend>* theme) {
         *     if (theme) {
         *         bg.set_color(theme->window_bg);
         *     }
         * });
         * @endcode
         */
        signal<const theme_type*> theme_changed;

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
            if (theme.name.empty()) {
                LOG_WARN("Ignoring theme registration with empty name");
                return;
            }

            std::unique_lock lock(m_mutex);

            if (m_themes.find(theme.name) != m_themes.end()) {
                LOG_INFO("Overwriting existing theme: ", theme.name);
            } else {
                LOG_DEBUG("Registered theme: ", theme.name);
            }

            m_themes[theme.name] = theme;
        }

        /**
         * @brief Register a theme (move version)
         * @param theme Theme to register (must have non-empty name)
         */
        void register_theme(theme_type&& theme) {
            if (theme.name.empty()) {
                LOG_WARN("Ignoring theme registration with empty name");
                return;
            }

            std::string name = theme.name;  // Copy name before move

            std::unique_lock lock(m_mutex);

            if (m_themes.find(name) != m_themes.end()) {
                LOG_INFO("Overwriting existing theme: ", name);
            } else {
                LOG_DEBUG("Registered theme: ", name);
            }

            m_themes[std::move(name)] = std::move(theme);
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
            std::shared_lock lock(m_mutex);

            auto it = m_themes.find(name);
            if (it != m_themes.end()) {
                LOG_DEBUG("Found theme: ", name);
                return &it->second;
            }

            LOG_DEBUG("Theme not found: ", name);
            return nullptr;
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
            std::shared_lock lock(m_mutex);

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
        [[nodiscard]] std::unordered_map<std::string, theme_type> all_themes() const {
            std::shared_lock lock(m_mutex);
            return m_themes;  // Return by value for thread safety
        }

        /**
         * @brief Check if a theme is registered
         * @param name Theme name
         * @return True if theme exists
         */
        [[nodiscard]] bool has_theme(const std::string& name) const noexcept {
            std::shared_lock lock(m_mutex);
            return m_themes.find(name) != m_themes.end();
        }

        /**
         * @brief Unregister a theme
         * @param name Theme name
         * @return True if theme was removed, false if not found
         */
        bool unregister_theme(const std::string& name) {
            std::unique_lock lock(m_mutex);

            bool removed = m_themes.erase(name) > 0;
            if (removed) {
                LOG_INFO("Unregistered theme: ", name);
            } else {
                LOG_WARN("Cannot unregister theme (not found): ", name);
            }
            return removed;
        }

        /**
         * @brief Clear all registered themes
         */
        void clear() noexcept {
            std::unique_lock lock(m_mutex);
            size_t count = m_themes.size();
            m_themes.clear();
            LOG_INFO("Cleared all themes (", count, " removed)");
        }

        /**
         * @brief Get the number of registered themes
         * @return Theme count
         */
        [[nodiscard]] size_t size() const noexcept {
            std::shared_lock lock(m_mutex);
            return m_themes.size();
        }

        /**
         * @brief Check if registry is empty
         * @return True if no themes registered
         */
        [[nodiscard]] bool empty() const noexcept {
            std::shared_lock lock(m_mutex);
            return m_themes.empty();
        }

        /**
         * @brief Set the currently active theme by name
         * @param name Theme name (must be registered)
         * @return True if theme was found and set, false otherwise
         *
         * @details
         * Sets the global active theme for all widgets in this context.
         * The theme must already be registered in the registry.
         *
         * @example
         * @code
         * ctx.themes().register_theme(norton_blue_theme);
         * ctx.themes().set_current_theme("Norton Blue");  // All widgets now use this
         * @endcode
         */
        bool set_current_theme(const std::string& name) {
            const theme_type* new_theme_ptr = nullptr;

            {
                std::unique_lock lock(m_mutex);

                auto it = m_themes.find(name);
                if (it != m_themes.end()) {
                    m_current_theme = std::make_shared<const theme_type>(it->second);
                    new_theme_ptr = m_current_theme.get();
                    LOG_DEBUG("Set current theme: ", name);
                } else {
                    LOG_WARN("Cannot set current theme (not found): ", name);
                    return false;
                }
            }

            // Emit signal outside the lock to avoid deadlock
            theme_changed.emit(new_theme_ptr);
            return true;
        }

        /**
         * @brief Set the currently active theme by value
         * @param theme Theme to set as current
         *
         * @details
         * Sets the global active theme for all widgets in this context.
         * The theme does NOT need to be registered first.
         *
         * @example
         * @code
         * ui_theme<Backend> custom_theme = create_my_theme();
         * ctx.themes().set_current_theme(std::move(custom_theme));
         * @endcode
         */
        void set_current_theme(std::shared_ptr<const theme_type> theme) {
            const theme_type* new_theme_ptr = nullptr;

            {
                std::unique_lock lock(m_mutex);
                m_current_theme = std::move(theme);
                new_theme_ptr = m_current_theme.get();

                if (m_current_theme) {
                    LOG_DEBUG("Set current theme: ", m_current_theme->name);
                } else {
                    LOG_DEBUG("Cleared current theme");
                }
            }

            // Emit signal outside the lock to avoid deadlock
            theme_changed.emit(new_theme_ptr);
        }

        /**
         * @brief Get the currently active theme
         * @return Pointer to current theme, or nullptr if none set
         *
         * @details
         * Returns the theme that was set via set_current_theme().
         * This is used by widgets to resolve their styles.
         *
         * @example
         * @code
         * if (auto* theme = ctx.themes().get_current_theme()) {
         *     // Use theme colors
         * }
         * @endcode
         */
        [[nodiscard]] const theme_type* get_current_theme() const noexcept {
            std::shared_lock lock(m_mutex);
            return m_current_theme.get();
        }

    private:
        std::unordered_map<std::string, theme_type> m_themes;
        std::shared_ptr<const theme_type> m_current_theme;  ///< Currently active theme
        mutable std::shared_mutex m_mutex;  ///< Reader-writer lock for thread safety
    };

} // namespace onyxui
