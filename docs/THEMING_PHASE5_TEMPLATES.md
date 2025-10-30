# Phase 5: Visual State Templates

**Status**: Planning
**Prerequisite**: Phase 2 (Palette), Phase 4 (Defaults)
**Duration**: 2-3 days
**Impact**: Further 30% reduction via pattern reuse

## Overview

Eliminate duplication across widgets using YAML anchors and aliases:

```yaml
name: "Norton Blue"

palette:
  bg: 0x0000AA
  fg: 0xFFFFFF
  accent: 0xFFFF00

# Define pattern ONCE
state_templates:
  standard: &std
    normal:
      foreground: $fg
      background: $bg
    hover:
      foreground: $accent
      background: $bg
      font: {bold: true}
    pressed:
      foreground: 0x000000
      background: 0xAAAAAA
    disabled:
      foreground: 0x555555
      background: $bg

# Reuse everywhere
button: *std
label: *std
menu_item: *std
# All widgets use same pattern!
```

## Motivation

**Without templates**:
- Same visual state pattern repeated for 10+ widgets
- Change hover style = edit 10+ places
- Hard to maintain consistency

**With templates**:
- Define pattern once
- Reference via YAML anchor/alias
- Change in 1 place affects all widgets
- Guaranteed consistency

## YAML Anchor/Alias Syntax

```yaml
# Define anchor with &name
state_templates:
  standard: &std
    normal: {...}
    hover: {...}

# Use alias with *name
button: *std
label: *std
```

**Key**: This is native YAML syntax (YAML 1.2 spec), no custom parsing needed!

## Architecture

Templates leverage existing YAML features - no new parsing code required:

```yaml
# Step 1: Define anchors
state_templates:
  standard: &std
    normal: {foreground: $fg, background: $bg}
    hover: {foreground: $accent, background: $bg, font: {bold: true}}

  inverted: &inv
    normal: {foreground: $bg, background: $fg}
    hover: {foreground: $bg, background: $accent}

# Step 2: Apply aliases
button: *std        # Standard pattern
label: *std         # Standard pattern
status_bar: *inv    # Inverted pattern

# Step 3: Override specific fields
menu_item:
  <<: *std          # Merge operator
  hover:
    font: {bold: false}  # Override: no bold on menu hover
```

## Implementation

### Step 1: YAML Anchor Support

**Good news**: fkyaml already supports anchors/aliases!

```cpp
// No new code needed - this already works:
fkyaml::node root = fkyaml::node::deserialize(yaml);

// Anchors resolved automatically during deserialization
// Aliases point to same node in memory
```

### Step 2: Merge Operator Support

Check if fkyaml supports `<<:` merge operator:

```yaml
button:
  <<: *std          # Merge standard template
  normal:
    foreground: 0xFF0000  # Override normal foreground
```

If not natively supported, implement merge preprocessing:

```cpp
namespace onyxui::theme_templates {
    /**
     * @brief Resolve merge operators in YAML tree
     * @param node Root node
     *
     * Processes `<<:` merge keys, copying aliased content
     * into parent while preserving explicit overrides.
     */
    void resolve_merge_operators(fkyaml::node& node);
}
```

### Step 3: Template Validation

Optional: Validate template usage

```cpp
namespace onyxui::theme_templates {
    /**
     * @brief Validate template references
     * @param root Theme YAML root
     * @return List of undefined template references
     */
    [[nodiscard]] std::vector<std::string> validate_templates(
        const fkyaml::node& root
    );
}
```

## Testing

```cpp
TEST_CASE("Visual state templates - basic alias") {
    const char* yaml = R"(
name: "Template Test"

palette:
  fg: 0xFFFFFF
  bg: 0x0000AA

state_templates:
  standard: &std
    normal:
      foreground: $fg
      background: $bg
    hover:
      foreground: 0xFFFF00
      background: $bg
      font: {bold: true}

button: *std
label: *std
)";

    auto theme = load_from_string_with_templates<Backend>(yaml);

    // Verify button uses template
    CHECK(theme.button.normal.foreground.r == 255);
    CHECK(theme.button.normal.background.b == 170);
    CHECK(theme.button.hover.font.bold == true);

    // Verify label uses same template
    CHECK(theme.label.text.foreground.r == 255);
    CHECK(theme.label.text.background.b == 170);
}

TEST_CASE("Visual state templates - merge with override") {
    const char* yaml = R"(
state_templates:
  standard: &std
    normal:
      foreground: 0xFFFFFF
      background: 0x0000AA

button:
  <<: *std                    # Merge standard
  normal:
    foreground: 0xFF0000      # Override: red text
)";

    auto theme = load_from_string_with_templates<Backend>(yaml);

    // Override applied
    CHECK(theme.button.normal.foreground.r == 255);
    CHECK(theme.button.normal.foreground.g == 0);
    CHECK(theme.button.normal.foreground.b == 0);

    // Template background preserved
    CHECK(theme.button.normal.background.b == 170);
}

TEST_CASE("Visual state templates - multiple templates") {
    const char* yaml = R"(
state_templates:
  standard: &std
    normal: {foreground: 0xFFFFFF, background: 0x0000AA}

  inverted: &inv
    normal: {foreground: 0x0000AA, background: 0xFFFFFF}

button: *std
status_bar: *inv
)";

    auto theme = load_from_string_with_templates<Backend>(yaml);

    // Button uses standard
    CHECK(theme.button.normal.foreground.r == 255);
    CHECK(theme.button.normal.background.b == 170);

    // Status bar uses inverted
    CHECK(theme.status_bar.background.foreground.b == 170);
    CHECK(theme.status_bar.background.background.r == 255);
}

TEST_CASE("Visual state templates - nested anchors") {
    const char* yaml = R"(
palette:
  fg: 0xFFFFFF

state_templates:
  base_font: &font
    bold: true
    underline: false

  standard: &std
    normal:
      foreground: $fg
      font: *font     # Nested anchor
)";

    auto theme = load_from_string_with_templates<Backend>(yaml);

    CHECK(theme.button.normal.font.bold == true);
    CHECK(theme.button.normal.font.underline == false);
}
```

## Acceptance Criteria

- [ ] YAML anchors (`&name`) define reusable templates
- [ ] YAML aliases (`*name`) reference templates
- [ ] Merge operator (`<<: *name`) works with overrides
- [ ] Multiple templates per theme supported
- [ ] Nested anchors work correctly
- [ ] Validation detects undefined template references
- [ ] Combines with palette system (Phase 2)
- [ ] All 1042+ tests still pass

## Example: Before vs After

**Before Phase 5 (120 lines after Phase 2)**:
```yaml
palette:
  fg: 0xFFFFFF
  bg: 0x0000AA
  accent: 0xFFFF00

button:
  normal: {foreground: $fg, background: $bg}
  hover: {foreground: $accent, background: $bg, font: {bold: true}}
  pressed: {foreground: 0x000000, background: 0xAAAAAA}
  disabled: {foreground: 0x555555, background: $bg}

label:
  text:
    foreground: $fg
    background: $bg
  hover:
    foreground: $accent
    background: $bg
    font: {bold: true}

menu_item:
  normal: {foreground: $fg, background: $bg}
  hover: {foreground: $accent, background: $bg, font: {bold: true}}
  pressed: {foreground: 0x000000, background: 0xAAAAAA}
  disabled: {foreground: 0x555555, background: $bg}

# Repeat for 7 more widgets...
```

**After Phase 5 (35 lines)**:
```yaml
palette:
  fg: 0xFFFFFF
  bg: 0x0000AA
  accent: 0xFFFF00

state_templates:
  standard: &std
    normal: {foreground: $fg, background: $bg}
    hover: {foreground: $accent, background: $bg, font: {bold: true}}
    pressed: {foreground: 0x000000, background: 0xAAAAAA}
    disabled: {foreground: 0x555555, background: $bg}

button: *std
label: *std
menu_item: *std
checkbox: *std
radio_button: *std
input_field: *std
dropdown: *std
# 10 widgets, 1 line each!
```

**Size Reduction**: 120 lines → 35 lines (70% reduction from Phase 2, 88% total from original)

## Integration with Other Phases

### Phase 2 (Palette)
Templates can reference palette colors:
```yaml
palette:
  fg: 0xFFFFFF

state_templates:
  standard: &std
    normal:
      foreground: $fg  # Palette reference works in templates!
```

### Phase 3 (Inheritance)
Child themes can override template references:
```yaml
extends: "Norton Blue"

button:
  <<: *custom_template  # Override with different template
```

### Phase 4 (Defaults)
Templates provide patterns, defaults fill gaps:
```yaml
state_templates:
  minimal: &min
    normal: {foreground: $fg, background: $bg}
    # hover/pressed/disabled generated by defaults
```

## Advanced Patterns

### Pattern Composition

```yaml
state_templates:
  base_font: &font
    bold: false
    underline: false
    reverse: false

  bold_font: &bold_font
    <<: *font
    bold: true

  standard: &std
    normal:
      foreground: $fg
      background: $bg
      font: *font
    hover:
      foreground: $accent
      background: $bg
      font: *bold_font
```

### Partial Overrides

```yaml
button:
  <<: *std
  hover:
    foreground: 0xFF0000  # Override just hover color, keep everything else
```

### Multiple Inheritance (via merge keys)

```yaml
button:
  <<: [*std, *accessible]  # Merge multiple templates (right wins)
```

## Known Limitations

1. **YAML Library Support**: Not all YAML libraries support merge keys (`<<:`)
   - fkyaml status: **Need to verify**
   - Workaround: Implement custom merge preprocessing

2. **Template Scope**: Templates defined in one file don't automatically transfer to child themes
   - Workaround: Re-define templates or use inheritance with template section

3. **Deep Overrides**: Overriding nested fields requires repeating parent keys
   - This is YAML limitation, not framework issue

## Migration Path

1. **Existing themes**: Continue working unchanged
2. **New themes**: Can use templates immediately after Phase 5
3. **Hybrid approach**: Mix explicit and template-based definitions
4. **Gradual adoption**: Convert one widget at a time

---

**Estimated Completion**: 2-3 days
**Lines of Code**: ~150 (merge logic + validation + tests)
**Impact**: 70% reduction from Phase 2 baseline, 88% total reduction

## Next Steps

After completing Phase 5:

1. **Documentation**: User guide with template best practices
2. **Migration Tool**: Script to detect common patterns and suggest templates
3. **Standard Templates**: Ship with pre-made templates (standard, accessible, minimal)
4. **Theme Gallery**: Example themes showcasing all features
