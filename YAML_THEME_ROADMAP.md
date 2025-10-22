# YAML Theme System - Implementation Roadmap

This document outlines the step-by-step implementation plan for adding YAML-based theme support using reflect-cpp.

## Overview

**Goal**: Enable loading/saving themes from YAML files while keeping C++ themes working.

**Key Technology**: [reflect-cpp](https://github.com/getml/reflect-cpp) for automatic serialization

**Design Philosophy**: Lazy evaluation - backends only need reflectability if they use YAML themes.

---

## Phase 1: Foundation - Add reflect-cpp Dependency ✅ COMPLETED

**Objective**: Add reflect-cpp to the project and verify basic functionality.

**Implementation Steps**:

1. ✅ Updated `/home/igor/proj/ares/ui/CMakeLists.txt` (main UI CMakeLists, NOT backend):
   - Added fkYAML dependency (v0.3.12)
   - Added reflect-cpp dependency (v0.10.0)
   - Linked as INTERFACE dependencies to onyxui target

2. ✅ Created test file: `unittest/reflection/test_reflectcpp_basic.cc`
   - Tests basic reflection using JSON (not YAML - see note below)
   - Verifies struct serialization/deserialization
   - Verifies reflection capabilities (field introspection)

**Test Cases**:
- ✅ Simple struct serialization to JSON
- ✅ Simple struct deserialization from JSON
- ✅ Round-trip preservation
- ✅ Nested struct support
- ✅ Error handling
- ✅ Field introspection via named tuples

**Test Results**:
- 4 test cases passed
- 31 assertions passed
- All 513 existing tests still pass (2532 assertions)
- No regressions introduced

**Success Criteria**:
- ✅ Project builds with reflect-cpp
- ✅ Basic serialization test passes
- ✅ No dependency conflicts

**Files Created**:
- `unittest/reflection/test_reflectcpp_basic.cc`
- `unittest/reflection/` directory

**Files Modified**:
- `/home/igor/proj/ares/ui/CMakeLists.txt` - Added dependencies
- `/home/igor/proj/ares/ui/unittest/CMakeLists.txt` - Added test file

**Important Discovery**:
reflect-cpp's YAML support uses **yaml-cpp**, not **fkYAML**. This creates an architectural decision:

**Options**:
1. ❌ Use yaml-cpp instead of fkYAML (contradicts user requirement)
2. ✅ Create custom integration layer between reflect-cpp and fkYAML
3. ❌ Use reflect-cpp only for reflection, manual YAML handling (loses automation)

**Decision**: Proceed with option 2 - create custom integration using:
- reflect-cpp for struct introspection (field names, types)
- fkYAML for YAML parsing/generation
- Custom serialization layer that bridges the two

This requires a new Phase 1.5 before proceeding to Phase 2.

---

## Phase 1.5: fkYAML Integration Layer (NEW)

**Objective**: Create custom integration layer between reflect-cpp and fkYAML.

**Background**: reflect-cpp natively supports yaml-cpp but NOT fkYAML. Since the user specifically requested fkYAML, we need a bridge layer.

**Implementation Strategy**:

1. Use reflect-cpp's core reflection capabilities:
   - `rfl::to_named_tuple()` - Convert struct to named tuple
   - `rfl::from_named_tuple()` - Convert named tuple to struct
   - Field introspection via named tuple accessors

2. Create custom YAML serialization using fkYAML:
   - Serialize: struct → named tuple → fkYAML node → YAML string
   - Deserialize: YAML string → fkYAML node → named tuple → struct

**Implementation Steps**:

1. Create `include/onyxui/yaml/fkyaml_adapter.hh`:
   - Template functions for struct → YAML conversion
   - Template functions for YAML → struct conversion
   - Use reflect-cpp's field introspection
   - Use fkYAML for YAML parsing/generation

2. Create `include/onyxui/yaml/theme_loader.hh`:
   - `load_from_file<Backend>(path)` - requires reflectable theme
   - `save_to_file<Backend>(theme, path)` - requires reflectable theme
   - `to_yaml<Backend>(theme)` - generate YAML string
   - `from_yaml<Backend>(yaml_str)` - parse YAML string

**Test Cases**:
- Serialize simple struct to YAML using fkYAML
- Deserialize simple struct from YAML using fkYAML
- Round-trip preservation
- Nested struct support
- Error handling (invalid YAML)

**Success Criteria**:
- ✅ Can serialize C++ structs to YAML using fkYAML
- ✅ Can deserialize YAML to C++ structs using fkYAML
- ✅ Works with reflect-cpp's reflection capabilities
- ✅ All tests pass

**Files Created**:
- `include/onyxui/yaml/fkyaml_adapter.hh`
- `include/onyxui/yaml/theme_loader.hh`
- `unittest/reflection/test_fkyaml_integration.cc`

**Alternative Approach** (if custom integration proves too complex):
- Use reflect-cpp's JSON serialization
- Use fkYAML's JSON compatibility mode
- Convert: struct → JSON → YAML (via fkYAML)
- This is simpler but less elegant

---

## Phase 2: Color Reflection - Make conio::color Reflectable

**Objective**: Add custom parser for `conio::color` supporting hex, RGB array, and RGB object formats.

**Implementation Steps**:

1. Create `backends/conio/include/onyxui/conio/color_reflection.hh`
2. Implement `rfl::Parser<onyxui::conio::color>` specialization
3. Support three formats:
   - Hex: `"#RRGGBB"`
   - Array: `[R, G, B]`
   - Object: `{r: R, g: G, b: B}`

**Test Cases**:
- Serialize color to hex string
- Parse hex string
- Parse RGB array
- Parse RGB object
- Reject invalid hex format
- Reject invalid array length
- Round-trip preservation

**Success Criteria**:
- ✅ All color format tests pass
- ✅ Error handling works correctly
- ✅ Serialization round-trip preserves values

**Files Created**:
- `backends/conio/include/onyxui/conio/color_reflection.hh`
- `unittest/reflection/test_color_reflection.cc`

---

## Phase 3: Enum Reflection - Make conio Enums Reflectable

**Objective**: Add reflection for `box_style` and `icon_style` enums.

**Implementation Steps**:

1. Create `backends/conio/include/onyxui/conio/enum_reflection.hh`
2. Implement `rfl::Reflector<box_style>` with:
   - `to_string(E)` method
   - `from_string(string_view)` method
3. Implement `rfl::Reflector<icon_style>` similarly

**Test Cases**:
- Serialize each box_style value
- Deserialize each box_style value
- Serialize each icon_style value
- Deserialize each icon_style value
- Reject invalid enum strings

**Success Criteria**:
- ✅ All enum tests pass
- ✅ String conversion works bidirectionally
- ✅ Invalid values rejected with clear errors

**Files Created**:
- `backends/conio/include/onyxui/conio/enum_reflection.hh`
- `unittest/reflection/test_enum_reflection.cc`

---

## Phase 4: Font Reflection - Make conio::renderer::font Reflectable

**Objective**: Make font struct automatically reflectable.

**Implementation Steps**:

1. Ensure `conio_renderer::font` is a simple struct (no special methods needed)
2. reflect-cpp automatically handles simple structs

**Test Cases**:
- Serialize font with various boolean combinations
- Deserialize font
- Verify default values applied when fields missing

**Success Criteria**:
- ✅ Font serialization works
- ✅ Defaults are applied correctly
- ✅ All boolean combinations work

**Files Created**:
- `unittest/reflection/test_font_reflection.cc`

---

## Phase 5: Widget Style Reflection - Make Widget Styles Reflectable

**Objective**: Make `button_style`, `label_style`, `panel_style` reflectable.

**Implementation Steps**:

1. Verify widget style structs are plain structs (should already be)
2. reflect-cpp handles them automatically once nested types are reflectable

**Test Cases**:
- Serialize button_style
- Deserialize button_style
- Serialize label_style
- Deserialize label_style
- Serialize panel_style
- Deserialize panel_style
- Round-trip preservation

**Success Criteria**:
- ✅ All widget style tests pass
- ✅ Nested types (color, enum) work correctly
- ✅ Round-trip serialization preserves data

**Files Created**:
- `unittest/reflection/test_widget_style_reflection.cc`

---

## Phase 6: Complete Theme Reflection - Make ui_theme Reflectable

**Objective**: Ensure complete `ui_theme<conio_backend>` is reflectable.

**Implementation Steps**:

1. Verify all fields in `ui_theme` are reflectable types
2. Test complete theme serialization

**Test Cases**:
- Serialize complete theme
- Deserialize complete theme
- Verify all fields preserved
- Round-trip test (serialize → deserialize → serialize should be identical)

**Success Criteria**:
- ✅ Complete theme serialization works
- ✅ All fields preserved in round-trip
- ✅ Nested structures work correctly

**Files Created**:
- `unittest/reflection/test_complete_theme.cc`

---

## Phase 7: Theme Loader Implementation

**Objective**: Implement `theme_loader` class with file I/O and lazy reflectability checking.

**Implementation Steps**:

1. Create `include/onyxui/theme_loader.hh`
2. Implement methods with `requires Reflectable<ui_theme<Backend>>` constraints:
   - `load_from_file(path)`
   - `load_from_string(yaml)`
   - `save_to_file(theme, path)`
   - `to_yaml(theme)`
   - `get_json_schema()`
3. Add error handling and helpful error messages

**Test Cases**:
- Save and load theme file
- Load from string
- Convert to YAML string
- Error handling for missing file
- Error handling for invalid YAML
- Error messages are helpful

**Success Criteria**:
- ✅ File save/load works
- ✅ String parsing works
- ✅ Error handling works
- ✅ Exceptions have helpful messages
- ✅ Compile error when backend not reflectable

**Files Created**:
- `include/onyxui/theme_loader.hh`
- `unittest/reflection/test_theme_loader.cc`

---

## Phase 8: JSON Schema Generation

**Objective**: Implement JSON Schema export for IDE tooling.

**Implementation Steps**:

1. Implement `get_json_schema()` using reflect-cpp's schema generation
2. Verify schema is valid JSON
3. Test schema structure

**Test Cases**:
- Schema is valid JSON
- Schema has expected structure
- Schema contains all theme properties
- Export schema to file works

**Success Criteria**:
- ✅ Schema is valid JSON
- ✅ Schema has correct structure
- ✅ Schema can be exported to file
- ✅ IDE can use schema for autocomplete

**Files Created**:
- `unittest/reflection/test_json_schema.cc`

---

## Phase 9: Directory Loading

**Objective**: Load multiple themes from directory.

**Implementation Steps**:

1. Implement `load_from_directory(path)` method
2. Iterate over `.yaml` and `.yml` files
3. Skip invalid files with warnings
4. Return vector of successfully loaded themes

**Test Cases**:
- Load multiple valid themes
- Skip invalid files gracefully
- Warning messages for skipped files
- Empty directory handling

**Success Criteria**:
- ✅ Loads all valid YAML files
- ✅ Skips invalid files with warning
- ✅ Returns vector of themes
- ✅ No exceptions for invalid files in directory

**Files Modified**:
- `include/onyxui/theme_loader.hh`

**Files Created**:
- `unittest/reflection/test_directory_loading.cc`

---

## Phase 10: Integration with dos_theme_showcase

**Objective**: Add YAML theme support to demo application.

**Implementation Steps**:

1. Update `dos_theme_showcase.cc` main() function:
   - Export JSON Schema on startup
   - Load YAML themes from `themes/` directory
   - Register loaded themes
   - Display all available themes
2. Create `themes/` directory
3. Add example YAML theme

**Manual Test Steps**:
1. Run dos_theme_showcase
2. Verify JSON schema generated in `themes/conio-theme.schema.json`
3. Create custom YAML theme in `themes/` directory
4. Restart application
5. Verify custom theme appears in theme list
6. Switch to custom theme using hotkey
7. Verify theme applies correctly

**Success Criteria**:
- ✅ Schema generation works on startup
- ✅ YAML themes load automatically from directory
- ✅ Custom themes can be added without recompiling
- ✅ Application works without YAML themes (built-in C++ themes always work)
- ✅ Error messages are helpful when theme loading fails

**Files Modified**:
- `backends/conio/dos_theme_showcase.cc`

**Files Created**:
- `themes/examples/dark_theme.yaml`
- `themes/conio-theme.schema.json` (generated)

---

## Phase 11: Documentation

**Objective**: Document YAML theme format and provide examples.

**Implementation Steps**:

1. Create example YAML themes in `themes/examples/`:
   - `dark_theme.yaml` - Dark professional theme
   - `light_theme.yaml` - Light theme
   - `high_contrast.yaml` - High contrast theme
2. Create `themes/README.md` explaining:
   - YAML format
   - Color format options
   - Enum values
   - How to create custom themes
3. Update `CLAUDE.md` with YAML theme section
4. Document JSON Schema usage in VS Code

**Deliverables**:
- Example YAML themes
- Theme format documentation
- VS Code integration guide
- Updated CLAUDE.md

**Success Criteria**:
- ✅ Example themes work correctly
- ✅ Documentation is complete and clear
- ✅ VS Code autocomplete works with schema
- ✅ Users can create custom themes easily

**Files Created**:
- `themes/examples/dark_theme.yaml`
- `themes/examples/light_theme.yaml`
- `themes/examples/high_contrast.yaml`
- `themes/README.md`
- `docs/yaml-theme-guide.md`

**Files Modified**:
- `CLAUDE.md`

---

## Testing Strategy

### Per-Phase Testing
- Each phase has dedicated unit tests
- Tests must pass before moving to next phase
- Use doctest framework (existing test infrastructure)

### Integration Testing
- Phase 10 includes manual integration testing
- Verify end-to-end workflow
- Test with real theme files

### Regression Testing
- Run all existing tests after each phase
- Ensure no breaking changes to existing functionality
- C++ themes must continue to work

### Test Coverage Goals
- Color parsing: 100% of formats
- Enum conversion: 100% of values
- Error handling: All error paths tested
- File I/O: Success and failure cases

---

## Rollback Plan

### Phase-Level Rollback
- Each phase is independent
- Can revert individual phase without affecting others
- Git commits per phase for easy rollback

### Feature-Level Rollback
- YAML support is completely optional
- Can disable via CMake option if needed:
  ```cmake
  option(ONYXUI_ENABLE_YAML_THEMES "Enable YAML theme support" ON)
  ```
- Existing C++ themes always work

### Fallback Strategy
- If reflect-cpp causes issues, can switch to manual parsing
- If JSON Schema generation fails, can write schema manually
- Application works without YAML themes

---

## Timeline Estimate

| Phase | Estimated Time | Complexity |
|-------|---------------|------------|
| Phase 1 | 1-2 hours | Low |
| Phase 2 | 2-3 hours | Medium |
| Phase 3 | 1-2 hours | Low |
| Phase 4 | 1 hour | Low |
| Phase 5 | 2-3 hours | Medium |
| Phase 6 | 1-2 hours | Low |
| Phase 7 | 2-3 hours | Medium |
| Phase 8 | 1-2 hours | Low |
| Phase 9 | 1-2 hours | Low |
| Phase 10 | 2-3 hours | Medium |
| Phase 11 | 2-3 hours | Low |
| **Total** | **16-25 hours** | **2-3 days** |

---

## Success Metrics

### Functional Metrics
- ✅ All 11 phases completed
- ✅ All unit tests passing (existing + new)
- ✅ Zero regression in existing functionality
- ✅ YAML themes load successfully
- ✅ JSON Schema enables IDE autocomplete

### Quality Metrics
- ✅ Error messages are clear and actionable
- ✅ Documentation is complete
- ✅ Code follows existing style guidelines
- ✅ No compiler warnings

### User Experience Metrics
- ✅ Users can create themes without C++ knowledge
- ✅ Theme errors are easy to debug
- ✅ VS Code provides autocomplete for theme files
- ✅ Example themes demonstrate all features

---

## Next Steps

1. **Review this roadmap** - Get approval before starting
2. **Start Phase 1** - Add reflect-cpp dependency
3. **Test Phase 1** - Verify basic functionality works
4. **Proceed sequentially** - Don't skip phases
5. **Document progress** - Update this file with completion status

---

## Notes

- This roadmap assumes C++20 support (already required)
- reflect-cpp is header-only (no linking issues)
- YAML support is opt-in (doesn't break existing code)
- Each phase is independently testable
- Can pause/resume at any phase boundary

---

**Last Updated**: 2025-10-20
**Status**: Planning
**Next Phase**: Phase 1 - Foundation
