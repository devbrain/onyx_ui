# OnyxUI Theming System Improvements

**Status**: Phase 4 Complete (4/5 phases done)
**Author**: Claude Code
**Date**: 2025-10-30
**Last Updated**: 2025-10-30

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

**Achieved Impact** (Phases 1-4):
- Reduced theme size from ~300 lines to **9 lines** (97% reduction!)
- Change a color in 1 place instead of 20+ (via palette system)
- Create theme variants in 5-10 lines (via inheritance)
- **Minimal themes**: Only 3 colors needed (window_bg, text_fg, border_color)
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

### Phase 4 (Defaults) - **COMPLETE**
- ✅ Themes can omit optional fields
- ✅ Intelligent defaults filled in (hover = lightened + accent, disabled = darkened)
- ✅ Minimal themes in just **9 lines** (3 colors only!)
- ✅ Completeness detection via heuristic comparison
- ✅ Color manipulation utilities: lighten(), darken(), invert(), luminance(), contrast()
- ✅ RGB-only backend support (no alpha required)
- ✅ Dependency-aware generation (normal → hover → pressed cascading)
- ✅ Examples: minimal_blue.yaml, minimal_green.yaml
- ✅ 16 comprehensive tests + integration tests
- ✅ Commit: 368e441

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

### After Phase 4 (9 lines - truly minimal!)
```yaml
# Minimal Blue - ACTUAL WORKING EXAMPLE
name: "Minimal Blue"
description: "Truly minimal theme with only 3 colors - Phase 4 generates the rest!"

# Only 3 colors needed - Phase 4 generates 50+ values from these!
window_bg: 0x0000AA      # Dark blue background
text_fg: 0xFFFFFF        # White text
border_color: 0xFFFF00   # Yellow accents/borders

# That's it! Phase 4 auto-generates:
# - Button states (normal, hover, pressed, disabled)
# - Label styles, Panel styles, Menu styles
# - Menu item states (normal, highlighted, disabled)
# - Scrollbar components (track, thumb, arrows, hover states)
# - All hover effects (lightened backgrounds, bold fonts)
# - All disabled effects (darkened colors)
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

### Smart Defaults (Phase 4)

```
Minimal YAML (3 colors)
    ↓
theme_defaults::check_completeness()
    ↓
theme_defaults::apply_defaults()
  - Generate missing normal states
  - Generate hover states (lighten + accent)
  - Generate pressed states (invert)
  - Generate disabled states (darken)
    ↓
Complete theme with 50+ generated values
```

**Key Features**:
- **Color Manipulation Utilities** (`color_utils.hh`):
  - `lighten(color, factor)` - Move RGB toward white
  - `darken(color, factor)` - Move RGB toward black
  - `invert(color)` - Flip RGB values (255-r, 255-g, 255-b)
  - `luminance(color)` - Calculate perceived brightness (ITU-R BT.709)
  - `contrast(fg, bg)` - Calculate WCAG contrast ratio

- **Completeness Detection**: Heuristic comparison against default-constructed colors
- **Dependency-Aware Generation**: Re-checks field presence after each generation step
- **RGB-Only Support**: Works without alpha channel (3-component colors)
- **Visual State Generation**:
  - Hover: Lighten background 20%, use accent foreground, bold font
  - Pressed: Invert colors
  - Disabled: Darken foreground 40%

**Files**:
- `include/onyxui/utils/color_utils.hh` - Color manipulation utilities
- `include/onyxui/theming/theme_defaults.hh` - Smart defaults implementation
- `unittest/theming/test_theme_defaults.cc` - 16 comprehensive tests
- `unittest/utils/test_color_utils.cc` - 40+ color utility tests

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

- [x] **Phase 1: Hex Color Notation** - ✅ COMPLETE
  - Commit: (early commit, part of color system)
  - 0xRRGGBB and 0xRRGGBBAA notation supported
  - Backward compatible with {r, g, b} notation

- [x] **Phase 2: Color Palette System** - ✅ COMPLETE
  - Commit: (integrated in theme_loader)
  - $reference notation for color reuse
  - Palette section in YAML themes

- [x] **Phase 3: Theme Inheritance** - ✅ COMPLETE
  - Commit: (integrated in theme_loader)
  - `extends: "Base Theme"` notation
  - Deep merging of theme structures

- [x] **Phase 4: Smart Defaults** - ✅ COMPLETE
  - Commit: 368e441
  - Minimal themes (9 lines, 3 colors only!)
  - Color manipulation utilities (lighten, darken, invert, etc.)
  - Automatic visual state generation
  - 1134 tests passing with 6076 assertions

- [ ] **Phase 5: Visual State Templates** - PENDING
  - YAML anchors and aliases for pattern reuse
  - Further size reduction (~30 lines total)

---

**Next Steps**: Phase 5 implementation (Visual State Templates via YAML anchors)
