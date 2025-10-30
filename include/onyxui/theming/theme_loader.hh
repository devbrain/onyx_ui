//
// Theme Loader - File I/O for YAML Themes
// Provides functions to load/save ui_theme from/to YAML files
//

#pragma once

#ifdef ONYXUI_ENABLE_YAML_THEMES

#include <onyxui/utils/fkyaml_adapter.hh>
#include <onyxui/theming/theme.hh>
#include <failsafe/logger.hh>
#include <fstream>
#include <stdexcept>
#include <string>
#include <filesystem>

/**
 * @namespace theme_loader
 * @brief Functions for loading and saving themes from/to YAML files
 *
 * @details
 * This namespace provides a simple API for theme file I/O:
 * - load_from_file() - Load theme from YAML file
 * - save_to_file() - Save theme to YAML file
 * - load_from_string() - Parse theme from YAML string
 * - to_yaml_string() - Convert theme to YAML string
 *
 * All functions provide helpful error messages for common issues:
 * - File not found
 * - Invalid YAML syntax
 * - Missing required fields
 * - Type mismatches
 *
 * Example usage:
 * @code
 * // Load a theme
 * auto theme = theme_loader::load_from_file<conio_backend>("themes/dark.yaml");
 *
 * // Modify it
 * theme.name = "My Custom Dark Theme";
 *
 * // Save it
 * theme_loader::save_to_file(theme, "themes/custom_dark.yaml");
 * @endcode
 */
namespace onyxui::theme_loader {
    /**
     * @brief Load a theme from a YAML file
     *
     * @tparam Backend The UI backend type (e.g., conio_backend)
     * @param file_path Path to the YAML file to load
     * @return The loaded theme
     *
     * @throws std::runtime_error if:
     *   - File does not exist
     *   - File cannot be opened
     *   - YAML syntax is invalid
     *   - Required fields are missing or have wrong types
     *
     * @note Error messages include the file path and specific issue
     *
     * Example:
     * @code
     * try {
     *     auto theme = theme_loader::load_from_file<conio_backend>("dark.yaml");
     *     std::cout << "Loaded theme: " << theme.name << std::endl;
     * } catch (const std::exception& e) {
     *     std::cerr << "Failed to load theme: " << e.what() << std::endl;
     * }
     * @endcode
     */
    template<UIBackend Backend>
    ui_theme <Backend> load_from_file(const std::filesystem::path& file_path) {
        LOG_DEBUG("Loading theme from: ", file_path.string());

        // Check if file exists
        if (!std::filesystem::exists(file_path)) {
            std::string error = "Theme file not found: " + file_path.string();
            LOG_ERROR(error);
            throw std::runtime_error(error);
        }

        // Open file
        std::ifstream file(file_path);
        if (!file.is_open()) {
            std::string error = "Failed to open theme file: " + file_path.string();
            LOG_ERROR(error);
            throw std::runtime_error(error);
        }

        // Read entire file
        std::string yaml_content(
            (std::istreambuf_iterator <char>(file)),
            std::istreambuf_iterator <char>()
        );
        file.close();

        // Parse YAML and deserialize
        try {
            auto theme = yaml::from_yaml_string <ui_theme <Backend>>(yaml_content);
            LOG_INFO("Loaded theme from ", file_path.string(), ": ", theme.name);
            return theme;
        } catch (const std::exception& e) {
            std::string error = "Failed to parse theme file '" + file_path.string() + "': " + std::string(e.what());
            LOG_ERROR(error);
            throw std::runtime_error(error);
        }
    }

    /**
     * @brief Save a theme to a YAML file
     *
     * @tparam Backend The UI backend type
     * @param theme The theme to save
     * @param file_path Path where the YAML file should be saved
     *
     * @throws std::runtime_error if:
     *   - Parent directory does not exist
     *   - File cannot be created
     *   - File cannot be written
     *
     * @note Creates parent directories if they don't exist
     * @note Overwrites existing file
     *
     * Example:
     * @code
     * ui_theme<conio_backend> theme = create_dark_theme();
     * theme_loader::save_to_file(theme, "themes/my_dark.yaml");
     * @endcode
     */
    template<UIBackend Backend>
    void save_to_file(const ui_theme <Backend>& theme, const std::filesystem::path& file_path) {
        LOG_DEBUG("Saving theme to: ", file_path.string());

        // Create parent directories if they don't exist
        auto parent_path = file_path.parent_path();
        if (!parent_path.empty() && !std::filesystem::exists(parent_path)) {
            try {
                std::filesystem::create_directories(parent_path);
                LOG_DEBUG("Created directory: ", parent_path.string());
            } catch (const std::exception& e) {
                std::string error = "Failed to create directory '" + parent_path.string() + "': " + std::string(e.what());
                LOG_ERROR(error);
                throw std::runtime_error(error);
            }
        }

        // Convert theme to YAML string
        std::string yaml_content;
        try {
            yaml_content = yaml::to_yaml_string(theme);
        } catch (const std::exception& e) {
            std::string error = "Failed to serialize theme: " + std::string(e.what());
            LOG_ERROR(error);
            throw std::runtime_error(error);
        }

        // Write to file
        std::ofstream file(file_path);
        if (!file.is_open()) {
            std::string error = "Failed to create theme file: " + file_path.string();
            LOG_ERROR(error);
            throw std::runtime_error(error);
        }

        file << yaml_content;
        file.close();

        if (file.fail()) {
            std::string error = "Failed to write theme file: " + file_path.string();
            LOG_ERROR(error);
            throw std::runtime_error(error);
        }

        LOG_INFO("Saved theme to ", file_path.string(), ": ", theme.name);
    }

    /**
     * @brief Load a theme from a YAML string
     *
     * @tparam Backend The UI backend type
     * @param yaml_string YAML content as a string
     * @return The loaded theme
     *
     * @throws std::runtime_error if YAML is invalid or fields are missing/wrong type
     *
     * Example:
     * @code
     * std::string yaml = R"(
     * name: "Test Theme"
     * description: "A test"
     * # ... rest of theme
     * )";
     * auto theme = theme_loader::load_from_string<conio_backend>(yaml);
     * @endcode
     */
    template<UIBackend Backend>
    ui_theme <Backend> load_from_string(const std::string& yaml_string) {
        try {
            return yaml::from_yaml_string <ui_theme <Backend>>(yaml_string);
        } catch (const std::exception& e) {
            throw std::runtime_error(
                "Failed to parse YAML string: " + std::string(e.what())
            );
        }
    }

    /**
     * @brief Convert a theme to a YAML string
     *
     * @tparam Backend The UI backend type
     * @param theme The theme to convert
     * @return YAML string representation
     *
     * @throws std::runtime_error if serialization fails
     *
     * Example:
     * @code
     * ui_theme<conio_backend> theme = create_my_theme();
     * std::string yaml = theme_loader::to_yaml_string(theme);
     * std::cout << yaml << std::endl;
     * @endcode
     */
    template<UIBackend Backend>
    std::string to_yaml_string(const ui_theme <Backend>& theme) {
        try {
            return yaml::to_yaml_string(theme);
        } catch (const std::exception& e) {
            throw std::runtime_error(
                "Failed to serialize theme: " + std::string(e.what())
            );
        }
    }

    /**
     * @brief Check if a file exists and appears to be a valid YAML file
     *
     * @param file_path Path to check
     * @return true if file exists and has .yaml or .yml extension
     *
     * @note Does not validate YAML content, only checks extension
     */
    inline bool is_theme_file(const std::filesystem::path& file_path) {
        if (!std::filesystem::exists(file_path)) {
            return false;
        }

        auto ext = file_path.extension().string();
        return (ext == ".yaml" || ext == ".yml");
    }

    /**
     * @brief Load result for a single theme file
     */
    template<UIBackend Backend>
    struct theme_load_result {
        std::filesystem::path file_path;
        bool success;
        std::string error_message;
        ui_theme<Backend> theme;  // Only valid if success == true
    };

    /**
     * @brief Load all theme files from a directory
     *
     * @tparam Backend The UI backend type
     * @param directory_path Directory containing theme YAML files
     * @param recursive If true, search subdirectories recursively (default: false)
     * @return Vector of successfully loaded themes
     *
     * @throws std::runtime_error if directory does not exist or cannot be accessed
     *
     * @note Skips invalid files silently - use load_from_directory_with_errors() to get error details
     * @note Only processes files with .yaml or .yml extensions
     *
     * Example:
     * @code
     * auto themes = theme_loader::load_from_directory<conio_backend>("themes/");
     * for (const auto& theme : themes) {
     *     std::cout << "Loaded: " << theme.name << std::endl;
     * }
     * @endcode
     */
    template<UIBackend Backend>
    std::vector<ui_theme<Backend>> load_from_directory(
        const std::filesystem::path& directory_path,
        bool recursive = false
    ) {
        // Check directory exists
        if (!std::filesystem::exists(directory_path)) {
            throw std::runtime_error(
                "Theme directory not found: " + directory_path.string()
            );
        }

        if (!std::filesystem::is_directory(directory_path)) {
            throw std::runtime_error(
                "Path is not a directory: " + directory_path.string()
            );
        }

        std::vector<ui_theme<Backend>> themes;

        // Iterate through directory (handle recursive vs non-recursive separately)
        if (recursive) {
            for (const auto& entry : std::filesystem::recursive_directory_iterator(directory_path)) {
                // Skip non-files
                if (!entry.is_regular_file()) {
                    continue;
                }

                // Skip non-YAML files
                if (!is_theme_file(entry.path())) {
                    continue;
                }

                // Try to load the theme
                try {
                    auto theme = load_from_file<Backend>(entry.path());
                    themes.push_back(std::move(theme));
                } catch (const std::exception&) {
                    // Silently skip invalid files
                }
            }
        } else {
            for (const auto& entry : std::filesystem::directory_iterator(directory_path)) {
                // Skip non-files
                if (!entry.is_regular_file()) {
                    continue;
                }

                // Skip non-YAML files
                if (!is_theme_file(entry.path())) {
                    continue;
                }

                // Try to load the theme
                try {
                    auto theme = load_from_file<Backend>(entry.path());
                    themes.push_back(std::move(theme));
                } catch (const std::exception&) {
                    // Silently skip invalid files
                }
            }
        }

        return themes;
    }

    /**
     * @brief Load all theme files from a directory with detailed error reporting
     *
     * @tparam Backend The UI backend type
     * @param directory_path Directory containing theme YAML files
     * @param recursive If true, search subdirectories recursively (default: false)
     * @return Vector of load results (both successful and failed)
     *
     * @throws std::runtime_error if directory does not exist or cannot be accessed
     *
     * @note Returns results for ALL files, including failures
     * @note Only processes files with .yaml or .yml extensions
     *
     * Example:
     * @code
     * auto results = theme_loader::load_from_directory_with_errors<conio_backend>("themes/");
     * for (const auto& result : results) {
     *     if (result.success) {
     *         std::cout << "✓ Loaded: " << result.theme.name << std::endl;
     *     } else {
     *         std::cerr << "✗ Failed: " << result.file_path.filename()
     *                   << " - " << result.error_message << std::endl;
     *     }
     * }
     * @endcode
     */
    template<UIBackend Backend>
    std::vector<theme_load_result<Backend>> load_from_directory_with_errors(
        const std::filesystem::path& directory_path,
        bool recursive = false
    ) {
        // Check directory exists
        if (!std::filesystem::exists(directory_path)) {
            throw std::runtime_error(
                "Theme directory not found: " + directory_path.string()
            );
        }

        if (!std::filesystem::is_directory(directory_path)) {
            throw std::runtime_error(
                "Path is not a directory: " + directory_path.string()
            );
        }

        std::vector<theme_load_result<Backend>> results;

        // Iterate through directory (handle recursive vs non-recursive separately)
        if (recursive) {
            for (const auto& entry : std::filesystem::recursive_directory_iterator(directory_path)) {
                // Skip non-files
                if (!entry.is_regular_file()) {
                    continue;
                }

                // Skip non-YAML files
                if (!is_theme_file(entry.path())) {
                    continue;
                }

                // Try to load the theme
                theme_load_result<Backend> result;
                result.file_path = entry.path();

                try {
                    result.theme = load_from_file<Backend>(entry.path());
                    result.success = true;
                    result.error_message = "";
                } catch (const std::exception& e) {
                    result.success = false;
                    result.error_message = e.what();
                }

                results.push_back(std::move(result));
            }
        } else {
            for (const auto& entry : std::filesystem::directory_iterator(directory_path)) {
                // Skip non-files
                if (!entry.is_regular_file()) {
                    continue;
                }

                // Skip non-YAML files
                if (!is_theme_file(entry.path())) {
                    continue;
                }

                // Try to load the theme
                theme_load_result<Backend> result;
                result.file_path = entry.path();

                try {
                    result.theme = load_from_file<Backend>(entry.path());
                    result.success = true;
                    result.error_message = "";
                } catch (const std::exception& e) {
                    result.success = false;
                    result.error_message = e.what();
                }

                results.push_back(std::move(result));
            }
        }

        return results;
    }
} // namespace onyxui::theme_loader

#endif // ONYXUI_ENABLE_YAML_THEMES
