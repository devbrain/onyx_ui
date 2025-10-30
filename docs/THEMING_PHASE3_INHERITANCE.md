# Phase 3: Theme Inheritance

**Status**: Planning
**Prerequisite**: Phase 2 (Palette System)
**Duration**: 4-6 days
**Impact**: Create theme variants in 5-10 lines

## Overview

Extend existing themes, override only what changes:

```yaml
extends: "Norton Blue"
name: "Norton Blue Dark"

# Only override what differs
palette:
  bg: 0x000055  # Darker blue

# Everything else inherited
```

## Motivation

**Without inheritance**:
- Create 5 Norton variants = copy/paste 1500 lines
- Change base theme = must update 5 themes
- Hard to maintain consistency

**With inheritance**:
- Norton Dark = 5 lines
- Norton Light = 5 lines
- Norton High Contrast = 10 lines
- Update base = all variants update automatically

## Architecture

```cpp
namespace onyxui::theme_inheritance {
    /**
     * @brief Load theme with inheritance chain
     * @param theme_name Name or path of theme
     * @param registry Theme registry for named themes
     * @return Fully resolved theme
     */
    template<UIBackend Backend>
    ui_theme<Backend> load_with_inheritance(
        const std::string& theme_name,
        theme_registry<Backend>& registry
    );

    /**
     * @brief Deep merge two themes (child overrides parent)
     * @param parent Base theme
     * @param child Overrides
     * @return Merged theme
     */
    template<UIBackend Backend>
    ui_theme<Backend> merge_themes(
        const ui_theme<Backend>& parent,
        const ui_theme<Backend>& child
    );
}
```

## Implementation

### Step 1: Deep Merge Algorithm

```cpp
template<UIBackend Backend>
ui_theme<Backend> merge_themes(
    const ui_theme<Backend>& parent,
    const ui_theme<Backend>& child
) {
    ui_theme<Backend> result = parent;  // Start with base

    // Override scalars if child specifies them
    if (!child.name.empty()) result.name = child.name;
    if (!child.description.empty()) result.description = child.description;

    // Deep merge visual states (if child specifies)
    if (child_has_button_normal()) {
        result.button.normal = child.button.normal;
    }
    // ... repeat for all nested structures

    return result;
}
```

**Key**: Use reflection or manual merge for each field. Consider fkyaml merge operator.

### Step 2: Recursive Loading

```yaml
# Theme A extends B, which extends C
extends: "Theme B"
```

Load chain: C → B → A (each merges into previous).

### Step 3: YAML Extension

```cpp
template<UIBackend Backend>
ui_theme<Backend> load_from_string_with_inheritance(
    const std::string& yaml,
    theme_registry<Backend>& registry
) {
    fkyaml::node root = fkyaml::node::deserialize(yaml);

    // Check for "extends" field
    if (root.contains("extends")) {
        std::string parent_name = root["extends"].get_value<std::string>();

        // Load parent (recursively)
        auto parent = load_theme_by_name(parent_name, registry);

        // Load child
        auto child = load_from_string<Backend>(yaml);

        // Merge
        return merge_themes(parent, child);
    }

    // No inheritance
    return load_from_string<Backend>(yaml);
}
```

## Testing

```cpp
TEST_CASE("Theme inheritance - basic override") {
    const char* parent_yaml = R"(
name: "Parent"
window_bg: 0x0000AA
text_fg: 0xFFFFFF
)";

    const char* child_yaml = R"(
extends: "Parent"
name: "Child"
window_bg: 0x000055  # Override
# text_fg inherited
)";

    // Register parent
    registry.register_theme(load_from_string(parent_yaml));

    // Load child
    auto child = load_with_inheritance(child_yaml, registry);

    CHECK(child.name == "Child");
    CHECK(child.window_bg.b == 85);   // Overridden
    CHECK(child.text_fg.r == 255);    // Inherited
}

TEST_CASE("Theme inheritance - chain") {
    // A extends B extends C
    // Test: A inherits from C through B
}

TEST_CASE("Theme inheritance - palette merge") {
    // Parent defines palette
    // Child overrides 1 color
    // Result: merged palette
}
```

## Acceptance Criteria

- [ ] `extends: "ThemeName"` loads base theme
- [ ] Child overrides work for all field types
- [ ] Palette merging works (child palette overrides parent)
- [ ] Inheritance chains work (A → B → C)
- [ ] Circular inheritance detected and rejected
- [ ] File path and registry name resolution
- [ ] All 1042+ tests still pass

## Example Variants

**Norton Blue Dark** (5 lines):
```yaml
extends: "Norton Blue"
name: "Norton Blue Dark"
palette:
  bg: 0x000044
```

**Norton Blue Light** (6 lines):
```yaml
extends: "Norton Blue"
name: "Norton Blue Light"
palette:
  bg: 0x8888FF
  fg: 0x000000
```

**Norton High Contrast** (7 lines):
```yaml
extends: "Norton Blue"
name: "Norton Blue HC"
palette:
  bg: 0x000000
  fg: 0xFFFFFF
  accent: 0xFF0000
```

---

**Estimated Completion**: 4-6 days
**Lines of Code**: ~300 (merge logic + tests)
**Impact**: Create unlimited variants from base themes
