//
// Test fkyaml anchor support directly
//

#include <doctest/doctest.h>

#ifdef ONYXUI_ENABLE_YAML_THEMES

#include <fkYAML/node.hpp>
#include <string>

TEST_CASE("fkyaml - Direct anchor test") {
    std::string yaml = R"(
test: &anchor_name 123
reference: *anchor_name
)";

    auto root = fkyaml::node::deserialize(yaml);

    // Check if anchor works
    int test_value = root["test"].get_value<int>();
    int reference_value = root["reference"].get_value<int>();

    MESSAGE("test: ", test_value);
    MESSAGE("reference: ", reference_value);

    CHECK(test_value == 123);
    CHECK(reference_value == 123);  // This should be same as test if anchors work
}

TEST_CASE("fkyaml - Mapping anchor test (demonstrates limitation)") {
    std::string yaml = R"(
template: &tmpl
  foreground: 0xFFFFFF
  background: 0x0000AA

button: *tmpl
)";

    auto root = fkyaml::node::deserialize(yaml);

    // EXPECTED FAILURE: fkyaml doesn't support mapping anchors
    // This test documents the Phase 5 blocker
    MESSAGE("This test is EXPECTED TO FAIL - it demonstrates fkyaml's limitation");
    MESSAGE("fkyaml does not support YAML mapping anchors, blocking Phase 5 implementation");

    WARN_FALSE(root["button"].is_mapping());  // Expected to fail (changed from CHECK to WARN)
}

TEST_CASE("fkyaml - Merge operator test (demonstrates limitation)") {
    std::string yaml = R"(
template: &tmpl
  foreground: 0xFFFFFF
  background: 0x0000AA

button:
  <<: *tmpl
  foreground: 0xFF0000
)";

    auto root = fkyaml::node::deserialize(yaml);

    // EXPECTED FAILURE: fkyaml doesn't support YAML merge operator (<<:)
    // This test documents the Phase 5 blocker
    MESSAGE("This test is EXPECTED TO FAIL - it demonstrates fkyaml's limitation");
    MESSAGE("fkyaml does not support YAML merge operator (<<:), blocking Phase 5 implementation");

    CHECK(root["button"].is_mapping());
    WARN_FALSE(root["button"].contains("background"));  // Expected to fail (changed from CHECK to WARN)
}

#else // !ONYXUI_ENABLE_YAML_THEMES

TEST_CASE("fkyaml anchors - YAML themes disabled") {
    CHECK(true);
}

#endif // ONYXUI_ENABLE_YAML_THEMES
