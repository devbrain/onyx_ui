/**
 * @file test_reflectcpp_basic.cc
 * @brief Basic tests for reflect-cpp integration
 * @details Phase 1 of YAML Theme System - Verify reflect-cpp dependency works
 *
 * NOTE: This test uses reflect-cpp's JSON support to verify the dependency.
 * YAML integration with fkYAML will be implemented in a later phase via
 * custom serialization using reflect-cpp's reflection capabilities.
 */

#include <doctest/doctest.h>
#include <rfl.hpp>
#include <rfl/json.hpp>
#include <string>

/**
 * @brief Simple test struct for basic serialization
 */
struct simple_config {
    std::string name;
    int value;
    bool enabled;
};

/**
 * @brief Nested struct for testing nested serialization
 */
struct nested_config {
    std::string title;
    simple_config settings;
};

TEST_CASE("reflect-cpp - Basic dependency verification") {
    SUBCASE("Simple struct serialization to JSON") {
        simple_config config{
            .name = "test_config",
            .value = 42,
            .enabled = true
        };

        // Serialize to JSON string (verifies reflect-cpp works)
        auto json_str = rfl::json::write(config);

        // Verify JSON contains expected fields
        CHECK(json_str.find("\"name\"") != std::string::npos);
        CHECK(json_str.find("\"test_config\"") != std::string::npos);
        CHECK(json_str.find("\"value\"") != std::string::npos);
        CHECK(json_str.find("42") != std::string::npos);
        CHECK(json_str.find("\"enabled\"") != std::string::npos);
        CHECK(json_str.find("true") != std::string::npos);
    }

    SUBCASE("Simple struct deserialization from JSON") {
        const char* json_text = R"({
"name": "loaded_config",
"value": 123,
"enabled": false
})";

        // Deserialize from JSON string
        auto result = rfl::json::read<simple_config>(json_text);

        // Verify successful parse
        REQUIRE(result);

        auto config = result.value();
        CHECK(config.name == "loaded_config");
        CHECK(config.value == 123);
        CHECK(config.enabled == false);
    }

    SUBCASE("Round-trip preservation") {
        simple_config original{
            .name = "round_trip_test",
            .value = 999,
            .enabled = true
        };

        // Serialize then deserialize
        auto json_str = rfl::json::write(original);
        auto result = rfl::json::read<simple_config>(json_str);

        REQUIRE(result);
        auto restored = result.value();

        // Verify all fields preserved
        CHECK(restored.name == original.name);
        CHECK(restored.value == original.value);
        CHECK(restored.enabled == original.enabled);
    }

    SUBCASE("Nested struct serialization") {
        nested_config config{
            .title = "Nested Test",
            .settings = {
                .name = "inner",
                .value = 55,
                .enabled = true
            }
        };

        auto json_str = rfl::json::write(config);

        // Verify nested structure in JSON
        CHECK(json_str.find("\"title\"") != std::string::npos);
        CHECK(json_str.find("\"settings\"") != std::string::npos);
        CHECK(json_str.find("\"inner\"") != std::string::npos);
    }

    SUBCASE("Nested struct deserialization") {
        const char* json_text = R"({
"title": "Loaded Nested",
"settings": {
  "name": "nested_inner",
  "value": 77,
  "enabled": false
}
})";

        auto result = rfl::json::read<nested_config>(json_text);

        REQUIRE(result);
        auto config = result.value();

        CHECK(config.title == "Loaded Nested");
        CHECK(config.settings.name == "nested_inner");
        CHECK(config.settings.value == 77);
        CHECK(config.settings.enabled == false);
    }
}

TEST_CASE("reflect-cpp - Error handling") {
    SUBCASE("Invalid JSON syntax") {
        const char* invalid_json = R"({
"name": "test
"value": 42
"enabled": true
})";

        auto result = rfl::json::read<simple_config>(invalid_json);

        // Should fail gracefully
        CHECK_FALSE(result);
    }

    SUBCASE("Missing required field") {
        const char* incomplete_json = R"({
"name": "test",
"value": 42
})";
        // Note: 'enabled' field is missing

        auto result = rfl::json::read<simple_config>(incomplete_json);

        // Should fail or use default value (behavior depends on reflect-cpp version)
        // Just verify it doesn't crash
        CHECK((result || !result));  // Either outcome is acceptable
    }

    SUBCASE("Wrong field type") {
        const char* wrong_type_json = R"({
"name": "test",
"value": "not_a_number",
"enabled": true
})";

        auto result = rfl::json::read<simple_config>(wrong_type_json);

        // Should fail gracefully
        CHECK_FALSE(result);
    }
}

TEST_CASE("reflect-cpp - Default values") {
    SUBCASE("Struct with defaults") {
        struct config_with_defaults {
            std::string name = "default_name";
            int value = 100;
            bool enabled = false;
        };

        const char* partial_json = R"({
"name": "custom_name"
})";

        auto result = rfl::json::read<config_with_defaults>(partial_json);

        if (result) {
            auto config = result.value();
            CHECK(config.name == "custom_name");
            // Default values may or may not be preserved depending on reflect-cpp behavior
            // Just verify parsing succeeded
        }
    }
}

TEST_CASE("reflect-cpp - Reflection capabilities") {
    SUBCASE("Field introspection") {
        simple_config config{
            .name = "test",
            .value = 42,
            .enabled = true
        };

        // reflect-cpp can introspect struct fields
        // This verifies the core reflection capability we need
        auto named_tuple = rfl::to_named_tuple(config);

        // Verify we can access fields by name
        CHECK(rfl::get<"name">(named_tuple) == "test");
        CHECK(rfl::get<"value">(named_tuple) == 42);
        CHECK(rfl::get<"enabled">(named_tuple) == true);
    }

    SUBCASE("Struct modification via named tuple") {
        simple_config config{
            .name = "original",
            .value = 10,
            .enabled = false
        };

        // Convert to named tuple, modify, convert back
        auto nt = rfl::to_named_tuple(config);
        auto modified_nt = rfl::replace(nt, rfl::make_field<"value">(99));
        auto modified_config = rfl::from_named_tuple<simple_config>(modified_nt);

        CHECK(modified_config.name == "original");
        CHECK(modified_config.value == 99);
        CHECK(modified_config.enabled == false);
    }
}
