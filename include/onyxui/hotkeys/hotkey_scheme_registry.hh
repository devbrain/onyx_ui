/**
 * @file hotkey_scheme_registry.hh
 * @brief Registry for hotkey schemes (parallel to theme_registry)
 * @author Claude Code
 * @date 2025-10-26
 *
 * @details
 * Manages hotkey schemes and tracks the current active scheme.
 * NOT backend-specific - schemes work across all backends.
 *
 * ## Design Philosophy
 *
 * Hotkey scheme registry parallels theme_registry:
 * - **Theme Registry**: Manages visual themes (colors, borders, fonts)
 * - **Hotkey Scheme Registry**: Manages keyboard schemes (key bindings)
 *
 * Both are:
 * - User preferences (not technical constraints)
 * - Backend-agnostic (work everywhere)
 * - Switchable at runtime
 * - Auto-registered with built-in defaults
 *
 * ## Differences from theme_registry
 *
 * | Aspect | Theme Registry | Hotkey Scheme Registry |
 * |--------|----------------|------------------------|
 * | Backend-specific? | Yes (per-backend themes) | No (universal schemes) |
 * | Where registered? | Backend::register_themes() | Library (ui_context constructor) |
 * | Template parameter? | Yes (Backend) | No (plain class) |
 *
 * ## Usage Example
 *
 * ```cpp
 * // Automatic registration (happens in ui_context constructor)
 * scoped_ui_context<Backend> ctx;
 *
 * // Schemes already available!
 * auto* schemes = ctx.hotkey_schemes();
 * auto* current = schemes->get_current_scheme();  // "Windows" (default)
 *
 * // Switch schemes at runtime
 * schemes->set_current_scheme("Norton Commander");
 *
 * // Register custom scheme
 * hotkey_scheme custom;
 * custom.name = "Vim-style";
 * custom.set_binding(hotkey_action::menu_down, parse_key_sequence("j"));
 * schemes->register_scheme(std::move(custom));
 * ```
 */

#pragma once

#include <string>
#include <map>
#include <vector>
#include <optional>
#include <onyxui/hotkeys/hotkey_scheme.hh>

namespace onyxui {

    /**
     * @class hotkey_scheme_registry
     * @brief Registry for hotkey schemes (parallel to theme_registry)
     *
     * @details
     * Manages a collection of hotkey schemes and tracks which is active.
     * NOT templated on Backend (schemes are universal).
     *
     * ## Lifecycle
     *
     * The registry is owned by ui_context and lives for the context's lifetime.
     * Built-in schemes are automatically registered in ui_context constructor.
     *
     * ## Thread Safety
     *
     * Not thread-safe. All operations must be on the UI thread.
     * (Could be extended with reader-writer locks like theme_registry if needed)
     *
     * ## First-Registered-Is-Default Convention
     *
     * The first scheme registered becomes the default.
     * Built-in registration order (in ui_context):
     * 1. Windows (default)
     * 2. Norton Commander
     *
     * Applications can override by calling set_current_scheme().
     */
    class hotkey_scheme_registry {
    public:
        /**
         * @brief Default constructor
         */
        hotkey_scheme_registry() = default;

        /**
         * @brief Register a hotkey scheme
         * @param scheme Scheme to register (moved into registry)
         *
         * @details
         * Adds scheme to registry. If this is the first scheme registered,
         * it becomes the default (current) scheme.
         *
         * If a scheme with the same name already exists, it is replaced.
         *
         * @example
         * @code
         * hotkey_scheme_registry registry;
         *
         * hotkey_scheme scheme;
         * scheme.name = "My Scheme";
         * scheme.set_binding(hotkey_action::activate_menu_bar, parse_key_sequence("F10"));
         *
         * registry.register_scheme(std::move(scheme));
         * @endcode
         */
        void register_scheme(hotkey_scheme scheme) {
            std::string name = scheme.name;
            m_schemes[name] = std::move(scheme);

            // First registered is default
            if (!m_current_scheme) {
                m_current_scheme = name;
            }
        }

        /**
         * @brief Get scheme by name
         * @param name Scheme name to retrieve
         * @return Pointer to scheme, or nullptr if not found
         *
         * @details
         * Returns non-owning pointer to scheme stored in registry.
         * Pointer valid until scheme is removed or registry destroyed.
         *
         * @example
         * @code
         * auto* scheme = registry.get_scheme("Windows");
         * if (scheme) {
         *     auto key = scheme->get_binding(hotkey_action::activate_menu_bar);
         * }
         * @endcode
         */
        [[nodiscard]] const hotkey_scheme* get_scheme(const std::string& name) const {
            auto it = m_schemes.find(name);
            return (it != m_schemes.end()) ? &it->second : nullptr;
        }

        /**
         * @brief Get current active scheme
         * @return Pointer to current scheme, or nullptr if none registered
         *
         * @details
         * Returns the scheme currently in use for the application.
         * This is what hotkey_manager queries to resolve key presses.
         *
         * @example
         * @code
         * auto* scheme = registry.get_current_scheme();
         * if (scheme) {
         *     auto action = scheme->find_action_for_key(pressed_key);
         *     // Dispatch action...
         * }
         * @endcode
         */
        [[nodiscard]] const hotkey_scheme* get_current_scheme() const {
            if (!m_current_scheme) return nullptr;
            return get_scheme(*m_current_scheme);
        }

        /**
         * @brief Set current active scheme
         * @param name Name of scheme to activate
         * @return true if scheme exists and was activated, false if not found
         *
         * @details
         * Switches the active scheme. Takes effect immediately on next key press.
         * No invalidation or notification needed - hotkey_manager queries
         * current scheme on each key event.
         *
         * @example
         * @code
         * if (registry.set_current_scheme("Norton Commander")) {
         *     std::cout << "Switched to Norton Commander (F9 for menu)\n";
         * } else {
         *     std::cerr << "Scheme not found\n";
         * }
         * @endcode
         */
        bool set_current_scheme(const std::string& name) {
            if (m_schemes.contains(name)) {
                m_current_scheme = name;
                return true;
            }
            return false;
        }

        /**
         * @brief Get name of current scheme
         * @return Name of current scheme, or nullopt if none
         */
        [[nodiscard]] std::optional<std::string> get_current_scheme_name() const {
            return m_current_scheme;
        }

        /**
         * @brief Get all registered scheme names
         * @return Vector of scheme names (in arbitrary order)
         *
         * @details
         * Useful for populating UI selection lists.
         *
         * @example
         * @code
         * auto names = registry.get_scheme_names();
         * for (const auto& name : names) {
         *     std::cout << "Available scheme: " << name << "\n";
         * }
         * @endcode
         */
        [[nodiscard]] std::vector<std::string> get_scheme_names() const {
            std::vector<std::string> names;
            names.reserve(m_schemes.size());
            for (const auto& [name, _] : m_schemes) {
                names.push_back(name);
            }
            return names;
        }

        /**
         * @brief Get number of registered schemes
         * @return Count of schemes in registry
         */
        [[nodiscard]] size_t scheme_count() const noexcept {
            return m_schemes.size();
        }

        /**
         * @brief Check if a scheme exists
         * @param name Scheme name to check
         * @return true if scheme is registered
         */
        [[nodiscard]] bool has_scheme(const std::string& name) const {
            return m_schemes.contains(name);
        }

        /**
         * @brief Remove a scheme from registry
         * @param name Scheme name to remove
         * @return true if scheme was removed, false if not found
         *
         * @details
         * If removing the current scheme, current scheme becomes nullptr.
         * Consider setting a different current scheme before removing.
         */
        bool remove_scheme(const std::string& name) {
            if (m_current_scheme && *m_current_scheme == name) {
                m_current_scheme = std::nullopt;
            }
            return m_schemes.erase(name) > 0;
        }

        /**
         * @brief Clear all schemes
         *
         * @details
         * Removes all schemes and clears current scheme.
         * Useful for testing or complete reconfiguration.
         */
        void clear() {
            m_schemes.clear();
            m_current_scheme = std::nullopt;
        }

    private:
        std::map<std::string, hotkey_scheme> m_schemes;  ///< Name → scheme mapping
        std::optional<std::string> m_current_scheme;     ///< Current active scheme name
    };

} // namespace onyxui
