//
// Basic fkYAML Integration Test
// Tests the custom adapter between reflect-cpp and fkYAML
//

#include <doctest/doctest.h>

#ifdef ONYXUI_ENABLE_YAML_THEMES

#include <onyxui/utils/fkyaml_adapter.hh>
#include <string>

using namespace onyxui::yaml;

// Test struct with basic types
struct simple_config {
    int number;
    std::string text;
    bool flag;
};

TEST_CASE("fkYAML Adapter - Basic struct serialization") {
    simple_config config{42, "hello", true};

    SUBCASE("Serialize to YAML string") {
        const std::string yaml_str = to_yaml_string(config);

        CHECK(!yaml_str.empty());
        CHECK(yaml_str.find("number") != std::string::npos);
        CHECK(yaml_str.find("42") != std::string::npos);
        CHECK(yaml_str.find("text") != std::string::npos);
        CHECK(yaml_str.find("hello") != std::string::npos);
        CHECK(yaml_str.find("flag") != std::string::npos);
        CHECK(yaml_str.find("true") != std::string::npos);
    }

    SUBCASE("Convert to YAML node") {
        fkyaml::node node = to_yaml(config);

        CHECK(node.is_mapping());
        CHECK(node.contains("number"));
        CHECK(node.contains("text"));
        CHECK(node.contains("flag"));

        CHECK(node["number"].get_value<int>() == 42);
        CHECK(node["text"].get_value<std::string>() == "hello");
        CHECK(node["flag"].get_value<bool>() == true);
    }
}

TEST_CASE("fkYAML Adapter - Basic struct deserialization") {
    std::string yaml_str = R"(
number: 100
text: "world"
flag: false
)";

    SUBCASE("Parse from YAML string") {
        simple_config config = from_yaml_string<simple_config>(yaml_str);

        CHECK(config.number == 100);
        CHECK(config.text == "world");
        CHECK(config.flag == false);
    }

    SUBCASE("Parse from YAML node") {
        fkyaml::node node = fkyaml::node::deserialize(yaml_str);
        simple_config config = from_yaml<simple_config>(node);

        CHECK(config.number == 100);
        CHECK(config.text == "world");
        CHECK(config.flag == false);
    }
}

TEST_CASE("fkYAML Adapter - Round-trip preservation") {
    simple_config original{777, "test", true};

    SUBCASE("Serialize then deserialize") {
        std::string yaml_str = to_yaml_string(original);
        simple_config restored = from_yaml_string<simple_config>(yaml_str);

        CHECK(restored.number == original.number);
        CHECK(restored.text == original.text);
        CHECK(restored.flag == original.flag);
    }
}

#else // !ONYXUI_ENABLE_YAML_THEMES

TEST_CASE("fkYAML - YAML themes disabled") {
    // When YAML themes are disabled, this test does nothing
    CHECK(true);
}

#endif // ONYXUI_ENABLE_YAML_THEMES
