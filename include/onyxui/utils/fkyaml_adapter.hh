//
// fkYAML Adapter for reflect-cpp
// Bridges reflect-cpp reflection capabilities with fkYAML YAML parsing
//

#pragma once

#ifdef ONYXUI_ENABLE_YAML_THEMES

#include <fkYAML/node.hpp>
#include <rfl.hpp>
#include <onyxui/concepts/color_like.hh>
#include <onyxui/utils/color_utils.hh>
#include <string>
#include <string_view>
#include <type_traits>
#include <stdexcept>
#include <sstream>
#include <iomanip>
#include <cctype>

namespace onyxui::yaml {

    // ===========================================================================
    // Forward Declarations
    // ===========================================================================

    template<typename T>
    fkyaml::node to_yaml(const T& value);

    template<typename T>
    T from_yaml(const fkyaml::node& node);

    // ===========================================================================
    // Helper: Check if type is reflectable
    // ===========================================================================

    template<typename T>
    concept Reflectable = requires(T t) {
        { rfl::to_named_tuple(t) };
    };

    // Helper for static_assert(false) in if constexpr
    template<typename T>
    struct always_false : std::false_type {};

    // Helper to satisfy control flow analysis after static_assert
    [[noreturn]] inline void unreachable_type_error(const char* context) {
        throw std::runtime_error(std::string("Unsupported type in ") + context);
    }

    // ===========================================================================
    // Serialization: C++ → YAML
    // ===========================================================================

    namespace detail {
        // ===========================================================================
        // Color Helpers
        // ===========================================================================

        // Convert color to hex string "#RRGGBB"
        inline std::string color_to_hex(uint8_t r, uint8_t g, uint8_t b) {
            std::ostringstream oss;
            oss << '#'
                << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(r)
                << std::setw(2) << static_cast<int>(g)
                << std::setw(2) << static_cast<int>(b);
            return oss.str();
        }

        // Parse hex color: "#RRGGBB", "RRGGBB", "0xRRGGBB", "0xRRGGBBAA"
        inline bool parse_hex_color(std::string_view hex, uint8_t& r, uint8_t& g, uint8_t& b, uint8_t& a) {
            // Handle legacy "#RRGGBB" format
            if (!hex.empty() && hex[0] == '#') {
                hex = hex.substr(1);

                // Must be exactly 6 hex digits after '#'
                if (hex.length() != 6) {
                    return false;
                }

                // Validate all characters are hex digits
                for (char const c : hex) {
                    if (!std::isxdigit(static_cast<unsigned char>(c))) {
                        return false;
                    }
                }

                // Parse hex values manually for '#' format
                unsigned int rr = 0;
                unsigned int gg = 0;
                unsigned int bb = 0;
                std::istringstream(std::string(hex.substr(0, 2))) >> std::hex >> rr;
                std::istringstream(std::string(hex.substr(2, 2))) >> std::hex >> gg;
                std::istringstream(std::string(hex.substr(4, 2))) >> std::hex >> bb;

                r = static_cast<uint8_t>(rr);
                g = static_cast<uint8_t>(gg);
                b = static_cast<uint8_t>(bb);
                a = 255; // Full opacity for #RRGGBB format
                return true;
            }

            // Handle "0xRRGGBB" or "0xRRGGBBAA" format using color_utils
            auto const hex_value = color_utils::parse_hex_string(hex);
            if (!hex_value) {
                return false;
            }

            // Determine format based on digit count (after removing 0x prefix)
            std::size_t digit_count = hex.size();
            if (hex.size() >= 2 && hex[0] == '0' && (hex[1] == 'x' || hex[1] == 'X')) {
                digit_count -= 2;
            }

            color_utils::rgb_components components;
            if (digit_count == 6) {
                // 6 digits: 0xRRGGBB (RGB, alpha=255)
                components = color_utils::parse_hex_rgb(*hex_value);
            } else if (digit_count == 8) {
                // 8 digits: 0xRRGGBBAA (RGBA)
                components = color_utils::parse_hex_rgba(*hex_value);
            } else {
                return false;
            }

            r = components.r;
            g = components.g;
            b = components.b;
            a = components.a;
            return true;
        }

        // Forward declaration
        template<typename T>
        fkyaml::node to_yaml_value(const T& value);

        // Helper to detect if a type has a custom *_to_string function in parent namespace
        // Used for types like box_style that should serialize as strings, not structs
        template<typename T>
        concept HasCustomToString = requires(const T& value) {
            // Check for box_style_to_string, icon_style_to_string, etc.
            { box_style_to_string(value) } -> std::convertible_to<std::string>;
        } || requires(const T& value) {
            { icon_style_to_string(value) } -> std::convertible_to<std::string_view>;
        } || requires(const T& value) {
            { border_style_to_string(value) } -> std::convertible_to<std::string_view>;
        };

        // Unified serialization function using if constexpr dispatch
        template<typename T>
        fkyaml::node to_yaml_value(const T& value) {
            // Handle ColorLike types (use object notation {r, g, b} to avoid YAML comment issues with # in hex)
            if constexpr (ColorLike<T>) {
                fkyaml::node obj = fkyaml::node::mapping();
                obj["r"] = static_cast<int>(color_utils::get_r(value));
                obj["g"] = static_cast<int>(color_utils::get_g(value));
                obj["b"] = static_cast<int>(color_utils::get_b(value));
                return obj;
            }
            // Handle strings first (most specific)
            else if constexpr (std::is_same_v<T, std::string>) {
                return fkyaml::node(value);
            }
            else if constexpr (std::is_same_v<T, std::string_view>) {
                return fkyaml::node(std::string(value));
            }
            else if constexpr (std::is_same_v<T, const char*> || std::is_same_v<T, char*>) {
                return fkyaml::node(std::string(value));
            }
            // Handle arithmetic types (including bool)
            else if constexpr (std::is_arithmetic_v<T>) {
                return fkyaml::node(value);
            }
            // Handle enums
            else if constexpr (std::is_enum_v<T>) {
                if constexpr (requires { rfl::enum_to_string(value); }) {
                    return fkyaml::node(std::string(rfl::enum_to_string(value)));
                } else {
                    return fkyaml::node(static_cast<std::underlying_type_t<T>>(value));
                }
            }
            // Handle types with custom *_to_string functions (BEFORE Reflectable check!)
            // This ensures box_style, icon_style, etc. serialize as strings, not structs
            else if constexpr (HasCustomToString<T>) {
                if constexpr (requires { box_style_to_string(value); }) {
                    return fkyaml::node(box_style_to_string(value));
                } else if constexpr (requires { icon_style_to_string(value); }) {
                    return fkyaml::node(std::string(icon_style_to_string(value)));
                } else if constexpr (requires { border_style_to_string(value); }) {
                    return fkyaml::node(std::string(border_style_to_string(value)));
                }
            }
            // Handle reflectable structs
            else if constexpr (Reflectable<T>) {
                return to_yaml(value);
            }
            else {
                static_assert(always_false<T>::value, "Type not supported for YAML serialization");
                unreachable_type_error("YAML serialization");
            }
        }

    }

    /// @brief Convert a reflectable struct to fkYAML node
    template<typename T>
    fkyaml::node to_yaml(const T& value) {
        // Handle ColorLike types specially BEFORE checking Reflectable
        if constexpr (ColorLike<T>) {
            return detail::to_yaml_value(value);
        }
        // Handle reflectable structs
        else if constexpr (Reflectable<T>) {
            fkyaml::node node(fkyaml::node::mapping());

            // Use reflect-cpp to introspect fields
            auto named_tuple = rfl::to_named_tuple(value);

            // Use apply() to iterate over fields (NOT visit, which is for Literals/TaggedUnions)
            named_tuple.apply([&](const auto& field) {
                std::string field_name(field.name());  // field.name() returns string_view
                const auto& field_value = field.value();
                node[field_name] = detail::to_yaml_value(field_value);
            });

            return node;
        } else {
            // For non-reflectable types, use direct conversion
            return detail::to_yaml_value(value);
        }
    }

    /// @brief Convert reflectable struct to YAML string
    template<typename T>
    std::string to_yaml_string(const T& value) {
        fkyaml::node node = to_yaml(value);
        return fkyaml::node::serialize(node);
    }

    // ===========================================================================
    // Deserialization: YAML → C++
    // ===========================================================================

    namespace detail {
        // Forward declaration
        template<typename T>
        T from_yaml_value(const fkyaml::node& node);

        // Unified deserialize function using if constexpr dispatch
        template<typename T>
        T from_yaml_value(const fkyaml::node& node) {
            // Handle ColorLike types (supports hex string, array, or object)
            if constexpr (ColorLike<T>) {
                uint8_t r = 0;
                uint8_t g = 0;
                uint8_t b = 0;
                uint8_t a = 255;

                // Format 1: Hex string "#RRGGBB", "0xRRGGBB", "0xRRGGBBAA"
                if (node.is_string()) {
                    std::string hex = node.get_value<std::string>();
                    if (!parse_hex_color(hex, r, g, b, a)) {
                        throw std::runtime_error("Invalid hex color format: " + hex);
                    }
                    if constexpr (requires { T{r, g, b, a}; }) {
                        return T{r, g, b, a};  // RGBA color
                    } else {
                        return T{r, g, b};  // RGB color (ignore alpha)
                    }
                }
                // Format 2: Array [R, G, B] or [R, G, B, A]
                else if (node.is_sequence()) {
                    if (node.size() == 3) {
                        r = static_cast<uint8_t>(node[0].get_value<int>());
                        g = static_cast<uint8_t>(node[1].get_value<int>());
                        b = static_cast<uint8_t>(node[2].get_value<int>());
                        if constexpr (requires { T{r, g, b}; }) {
                            return T{r, g, b};  // RGB color
                        } else {
                            return T{r, g, b, 255};  // RGBA color
                        }
                    } else if (node.size() == 4) {
                        r = static_cast<uint8_t>(node[0].get_value<int>());
                        g = static_cast<uint8_t>(node[1].get_value<int>());
                        b = static_cast<uint8_t>(node[2].get_value<int>());
                        uint8_t a = static_cast<uint8_t>(node[3].get_value<int>());
                        if constexpr (requires { T{r, g, b, a}; }) {
                            return T{r, g, b, a};  // RGBA color
                        } else {
                            // RGB-only color, ignore alpha channel
                            return T{r, g, b};
                        }
                    } else {
                        throw std::runtime_error("Color array must have 3 or 4 elements [R, G, B] or [R, G, B, A]");
                    }
                }
                // Format 3: Object {r: R, g: G, b: B, a: A (optional)}
                else if (node.is_mapping()) {
                    // Use exception-based field access instead of deprecated contains()
                    try {
                        r = static_cast<uint8_t>(node["r"].get_value<int>());
                        g = static_cast<uint8_t>(node["g"].get_value<int>());
                        b = static_cast<uint8_t>(node["b"].get_value<int>());
                        // Check if alpha is present
                        try {
                            uint8_t a = static_cast<uint8_t>(node["a"].get_value<int>());
                            if constexpr (requires { T{r, g, b, a}; }) {
                                return T{r, g, b, a};  // RGBA color
                            } else {
                                // RGB-only color, ignore alpha channel
                                return T{r, g, b};
                            }
                        } catch (...) {
                            // No alpha present
                            if constexpr (requires { T{r, g, b}; }) {
                                return T{r, g, b};  // RGB color
                            } else {
                                return T{r, g, b, 255};  // RGBA color with default alpha
                            }
                        }
                    } catch (const std::exception&) {
                        throw std::runtime_error("Color object must have r, g, b fields (a is optional)");
                    }
                }
                else {
                    throw std::runtime_error("Invalid YAML node type for color (expected string, array, or object)");
                }
            }
            // Handle custom enum types (via enum_from_yaml_impl helper if enum_reflection.hh is included)
            else if constexpr (requires(const fkyaml::node& n) { enum_from_yaml_impl(n, static_cast<T*>(nullptr)); }) {
                return enum_from_yaml_impl(node, static_cast<T*>(nullptr));
            }
            // Handle bool
            else if constexpr (std::is_same_v<T, bool>) {
                if (!node.is_boolean()) {
                    throw std::runtime_error("YAML node is not a boolean");
                }
                return node.get_value<bool>();
            }
            // Handle strings
            else if constexpr (std::is_same_v<T, std::string>) {
                if (!node.is_string()) {
                    throw std::runtime_error("YAML node is not a string");
                }
                return node.get_value<std::string>();
            }
            // Handle arithmetic types (excluding bool)
            else if constexpr (std::is_arithmetic_v<T>) {
                if (node.is_integer()) {
                    return static_cast<T>(node.get_value<int64_t>());
                } else if (node.is_float_number()) {
                    return static_cast<T>(node.get_value<double>());
                }
                throw std::runtime_error("YAML node is not a numeric type");
            }
            // Handle enums
            else if constexpr (std::is_enum_v<T>) {
                if constexpr (requires { rfl::string_to_enum<T>(""); }) {
                    // Use reflect-cpp's string to enum conversion
                    if (!node.is_string()) {
                        throw std::runtime_error("YAML node for enum must be a string");
                    }
                    std::string str = node.get_value<std::string>();
                    auto result = rfl::string_to_enum<T>(str);
                    if (!result) {
                        throw std::runtime_error("Invalid enum value: " + str);
                    }
                    return *result;
                } else {
                    // Fallback: assume integer
                    if (!node.is_integer()) {
                        throw std::runtime_error("YAML node for enum must be an integer");
                    }
                    return static_cast<T>(node.get_value<std::underlying_type_t<T>>());
                }
            }
            // Handle reflectable structs
            else if constexpr (Reflectable<T>) {
                return from_yaml<T>(node);
            }
            else {
                static_assert(always_false<T>::value, "Type not supported for YAML deserialization");
                unreachable_type_error("YAML deserialization");
            }
        }
    }

    /// @brief Convert fkYAML node to a reflectable struct
    template<typename T>
    T from_yaml(const fkyaml::node& node) {
        // Handle ColorLike types specially BEFORE checking Reflectable
        if constexpr (ColorLike<T>) {
            return detail::from_yaml_value<T>(node);
        }
        // Handle reflectable structs
        else if constexpr (Reflectable<T>) {
            if (!node.is_mapping()) {
                throw std::runtime_error("YAML node must be a mapping for struct deserialization");
            }

            // Start with a default-constructed object and extract its named tuple
            auto named_tuple = rfl::to_named_tuple(T{});

            // Update each field from the YAML node
            // Note: apply() passes fields by value, so we need to capture and modify a mutable copy
            auto update_fields = [&](auto field) {
                std::string field_name(field.name());  // field.name() returns string_view
                // Use exception-based field access instead of deprecated contains()
                try {
                    // Extract the field type and deserialize from YAML
                    using FieldType = typename std::remove_cvref_t<decltype(field)>::Type;
                    return rfl::make_field<decltype(field)::name_>(
                        detail::from_yaml_value<FieldType>(node[field_name])
                    );
                } catch (...) {
                    // If field is missing in YAML, keep default value
                    return field;
                }
            };

            named_tuple = named_tuple.transform(update_fields);

            // Convert named tuple back to original struct
            return rfl::from_named_tuple<T>(named_tuple);
        } else {
            // For non-reflectable types, use direct conversion
            return detail::from_yaml_value<T>(node);
        }
    }

    /// @brief Parse YAML string and convert to struct
    template<typename T>
    T from_yaml_string(std::string_view yaml_str) {
        fkyaml::node node = fkyaml::node::deserialize(std::string(yaml_str));
        return from_yaml<T>(node);
    }

} // namespace onyxui::yaml

#endif // ONYXUI_ENABLE_YAML_THEMES
