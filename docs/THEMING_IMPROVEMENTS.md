# OnyxUI Theming System Improvements

**Status**: Planning
**Author**: Claude Code
**Date**: 2025-10-30

## Problem Statement

The current theming system requires massive repetition and verbosity:
- Colors defined as `{r: 255, g: 255, b: 255}` repeated 20+ times per theme
- Same colors duplicated across widgets (no palette system)
- No theme inheritance or derivation
- No templates for common visual state patterns
- ~200-300 lines per theme, mostly repetition

**Example Pain Point**:
```yaml
# Current: Repeat this 20+ times
button:
  normal:
    foreground: { r: 255, g: 255, b: 255 }
    background: { r: 0, g: 0, b: 170 }
  hover:
    foreground: { r: 255, g: 255, b: 255 }
    background: { r: 0, g: 0, b: 170 }
```

## Solution Overview

Implement five major improvements in testable phases:

1. **Phase 1**: Hex Color Notation (Week 1) - Foundation
2. **Phase 2**: Color Palette System (Week 1-2) - Core feature
3. **Phase 3**: Theme Inheritance (Week 2-3) - Variants
4. **Phase 4**: Smart Defaults (Week 3-4) - Minimal themes
5. **Phase 5**: Visual State Templates (Week 4) - Pattern reuse

**Expected Impact**:
- Reduce theme size from ~300 lines to ~50 lines (83% reduction)
- Change a color in 1 place instead of 20+
- Create theme variants in 5-10 lines
- Type-safe, maintainable, extensible

## Implementation Phases

See separate documents:
- [Phase 1: Hex Color Notation](THEMING_PHASE1_HEX_COLORS.md)
- [Phase 2: Color Palette System](THEMING_PHASE2_PALETTE.md)
- [Phase 3: Theme Inheritance](THEMING_PHASE3_INHERITANCE.md)
- [Phase 4: Smart Defaults](THEMING_PHASE4_DEFAULTS.md)
- [Phase 5: Visual State Templates](THEMING_PHASE5_TEMPLATES.md)

## Success Metrics

### Phase 1 (Hex Colors)
- ✅ `0xFFFFFF` parses to white color
- ✅ `0xFF0000FF` parses to blue with full alpha
- ✅ Backward compatible with `{r, g, b}` notation
- ✅ All 1042+ tests still pass

### Phase 2 (Palette)
- ✅ Define colors once in `palette:` section
- ✅ Reference via `$color_name` anywhere
- ✅ Reduce theme size by ~60-70%
- ✅ Color changes propagate automatically

### Phase 3 (Inheritance)
- ✅ `extends: "Norton Blue"` loads base theme
- ✅ Override only specific properties
- ✅ Create 5+ theme variants from 1 base theme
- ✅ Deep merging of nested structures

### Phase 4 (Defaults)
- ✅ Themes can omit optional fields
- ✅ Intelligent defaults filled in (hover = accent, etc.)
- ✅ Minimal themes in <20 lines
- ✅ Validation ensures completeness

### Phase 5 (Templates)
- ✅ Define visual state patterns once
- ✅ Reuse across widgets via YAML anchors
- ✅ Further 30% size reduction
- ✅ Consistent state behavior

## Example: Before vs After

### Before (300+ lines)
```yaml
name: "Norton Blue"

window_bg: { r: 0, g: 0, b: 170 }
text_fg: { r: 255, g: 255, b: 255 }
border_color: { r: 255, g: 255, b: 0 }

button:
  normal:
    font: { bold: false, underline: false, reverse: false }
    foreground: { r: 255, g: 255, b: 255 }
    background: { r: 0, g: 0, b: 170 }
  hover:
    font: { bold: true, underline: false, reverse: false }
    foreground: { r: 255, g: 255, b: 0 }
    background: { r: 0, g: 0, b: 255 }
  # ... 250 more lines
```

### After Phase 2 (50 lines)
```yaml
name: "Norton Blue"

palette:
  bg: 0x0000AA          # Dark blue
  fg: 0xFFFFFF          # White
  accent: 0xFFFF00      # Yellow
  hover_bg: 0x0000FF    # Bright blue
  disabled: 0x555555    # Dark gray

button:
  normal: { foreground: $fg, background: $bg }
  hover: { foreground: $accent, background: $hover_bg, font: {bold: true} }
  pressed: { foreground: 0x000000, background: 0xAAAAAA }
  disabled: { foreground: $disabled, background: $bg }
  # ... rest uses same palette
```

### After Phase 3 (10 lines for variant)
```yaml
extends: "Norton Blue"
name: "Norton Blue Dark"

palette:
  bg: 0x000055          # Darker blue (only override)
```

### After Phase 5 (30 lines total)
```yaml
name: "Norton Blue"

palette:
  bg: 0x0000AA
  fg: 0xFFFFFF
  accent: 0xFFFF00

state_templates:
  standard: &std
    normal: { foreground: $fg, background: $bg }
    hover: { foreground: $accent, background: $hover_bg, font: {bold: true} }
    pressed: { foreground: 0x000000, background: 0xAAAAAA }
    disabled: { foreground: $disabled, background: $bg }

button: *std
label: *std
# All widgets reuse template
```

## Architecture

### Color Parsing (Phase 1)

```
YAML File
    ↓
parse_color_value(string)
    ↓
Backend::parse_color_hex(uint32_t)
    ↓
Backend::color_type
```

**Key**: Backends own color parsing, framework provides hex utilities.

### Palette Resolution (Phase 2)

```
YAML with $references
    ↓
extract_palette()
    ↓
resolve_references(tree, palette)
    ↓
Standard ui_theme<Backend>
```

**Key**: Preprocessing step before deserialization.

### Theme Inheritance (Phase 3)

```
Child YAML with "extends"
    ↓
load_base_theme()
    ↓
deep_merge(base, child)
    ↓
Resolved theme
```

**Key**: Recursive loading, deep merge algorithm.

## Testing Strategy

Each phase has 3 test categories:

1. **Unit Tests**: Core functionality
   - Color parsing
   - Palette resolution
   - Merge algorithms

2. **Integration Tests**: YAML to theme
   - Load themes with new features
   - Verify backward compatibility
   - Error handling

3. **Visual Tests**: Actual rendering
   - Themes render correctly
   - Colors match expectations
   - No regressions

## Migration Path

1. **Phase 1**: Opt-in (both notations work)
2. **Phase 2**: Opt-in (palette optional)
3. **Phase 3**: Opt-in (extends optional)
4. **Deprecation**: After 6 months, mark old notation deprecated
5. **Migration Tool**: Provide script to convert old → new

## Related Documents

- [Current Theme System](CLAUDE/THEMING.md)
- [YAML Theme Format](../themes/examples/norton_blue.yaml)
- [Backend Pattern](CLAUDE/ARCHITECTURE.md#backend-pattern)

## Implementation Status

- [ ] Phase 1: Hex Color Notation
- [ ] Phase 2: Color Palette System
- [ ] Phase 3: Theme Inheritance
- [ ] Phase 4: Smart Defaults
- [ ] Phase 5: Visual State Templates

---

**Next Steps**: Begin Phase 1 implementation (see THEMING_PHASE1_HEX_COLORS.md)
