# OnyxUI YAML Themes

This directory contains YAML theme files that can be loaded dynamically at runtime without recompiling your application.

## Available Example Themes

### Classic DOS/TUI Themes

1. **norton_blue.yaml** - Norton Utilities blue theme
   - Dark blue background with white text
   - Yellow highlights
   - Classic single-line borders
   - Perfect for that vintage Norton Commander look

2. **borland_turbo.yaml** - Borland Turbo Pascal/C++ IDE theme
   - Cyan background with black text
   - Double-line borders (distinctive Turbo Vision style)
   - Light gray buttons

3. **midnight_commander.yaml** - MC file manager theme
   - Dark blue background with yellow text
   - Cyan buttons
   - Single-line borders
   - Two-panel file manager aesthetic

4. **dos_edit.yaml** - MS-DOS Edit text editor theme
   - Light gray background with black text
   - Simple and clean
   - Classic MS-DOS 5.0+ look

### Modern Themes

5. **dark_professional.yaml** - Modern dark theme
   - Dark gray (not pure black) background
   - Light gray text for reduced eye strain
   - Modern blue accent color
   - Rounded borders
   - Professional look inspired by VS Code Dark+

6. **light_modern.yaml** - Modern light theme
   - Very light gray background (softer than pure white)
   - Dark gray text
   - Modern blue accents
   - Rounded borders
   - Inspired by VS Code Light+

### Accessibility Theme

7. **high_contrast.yaml** - High contrast accessibility theme
   - Pure black background
   - Pure white text
   - Maximum contrast
   - Heavy (bold) borders for better visibility
   - Bold fonts throughout
   - Extra padding for better touch targets

## Using Themes in Your Application

### Loading a Single Theme

```cpp
#include <onyxui/theme_loader.hh>
#include <onyxui/conio/conio_backend.hh>

using namespace onyxui;
using namespace onyxui::theme_loader;

// Load a theme
auto theme = load_from_file<conio::conio_backend>("themes/examples/norton_blue.yaml");

// Apply it to your UI context
ctx.themes().register_theme(theme);
ctx.themes().apply_theme("Norton Blue");
```

### Loading All Themes from a Directory

```cpp
// Load all theme files from a directory
auto themes = load_from_directory<conio::conio_backend>("themes/examples");

// Register all loaded themes
for (const auto& theme : themes) {
    ctx.themes().register_theme(theme);
}

// Now all themes are available!
ctx.themes().apply_theme("Dark Professional");
```

### Loading with Error Reporting

```cpp
// Get detailed error information for failed loads
auto results = load_from_directory_with_errors<conio::conio_backend>("themes/");

for (const auto& result : results) {
    if (result.success) {
        std::cout << "✓ Loaded: " << result.theme.name << "\n";
        ctx.themes().register_theme(result.theme);
    } else {
        std::cerr << "✗ Failed: " << result.file_path.filename()
                  << " - " << result.error_message << "\n";
    }
}
```

## Creating Custom Themes

### YAML Theme Format

A theme file has the following structure:

```yaml
name: "My Custom Theme"
description: "Description of your theme"

# Global colors
window_bg: { r: 0, g: 0, b: 170 }     # RGB values 0-255
text_fg: { r: 255, g: 255, b: 255 }
border_color: { r: 255, g: 255, b: 0 }

# Button styling
button:
  fg_normal: { r: 255, g: 255, b: 255 }
  bg_normal: { r: 0, g: 0, b: 170 }
  fg_hover: { r: 255, g: 255, b: 0 }
  bg_hover: { r: 0, g: 0, b: 255 }
  fg_pressed: { r: 0, g: 0, b: 0 }
  bg_pressed: { r: 170, g: 170, b: 170 }
  fg_disabled: { r: 85, g: 85, b: 85 }
  bg_disabled: { r: 0, g: 0, b: 170 }

  box_style: single_line  # none, single_line, double_line, rounded, heavy
  padding_horizontal: 2
  padding_vertical: 0
  text_align: center      # left, center, right, stretch

  font:
    bold: false
    reverse: false
    underline: false

  mnemonic_font:
    bold: false
    reverse: false
    underline: true

# Label styling
label:
  text: { r: 255, g: 255, b: 255 }
  background: { r: 0, g: 0, b: 170 }
  font: { bold: false, reverse: false, underline: false }
  mnemonic_font: { bold: false, reverse: false, underline: true }

# Panel styling
panel:
  background: { r: 0, g: 0, b: 170 }
  border_color: { r: 255, g: 255, b: 0 }
  box_style: single_line
  has_border: true
```

### Color Formats

Colors can be specified in multiple formats:

**RGB Object:**
```yaml
color: { r: 255, g: 128, b: 64 }
```

**RGB Array:**
```yaml
color: [255, 128, 64]
```

**Hex String (in object context):**
```yaml
window:
  background: "#FF8040"   # With # prefix
  foreground: "00FF00"    # Without # prefix
```

### Box Styles

Available border styles:
- `none` - No border
- `single_line` - Single-line ASCII borders (classic)
- `double_line` - Double-line ASCII borders (Borland style)
- `rounded` - Rounded corners (modern)
- `heavy` - Bold/heavy borders (accessibility)

### Text Alignment

Available alignment options:
- `left` - Align text to the left
- `center` - Center text (default for buttons)
- `right` - Align text to the right
- `stretch` - Stretch to fill available space

### Font Styles

Font flags can be combined:
```yaml
font:
  bold: true       # Bold/bright text
  reverse: false   # Inverted colors
  underline: false # Underlined text
```

## IDE Support (VS Code)

For autocomplete and validation in VS Code:

1. Install the YAML extension
2. The JSON schema is automatically generated at `themes/conio-theme.schema.json`
3. Add to your `settings.json`:

```json
{
  "yaml.schemas": {
    "themes/conio-theme.schema.json": "themes/**/*.yaml"
  }
}
```

Now you'll get:
- Autocomplete for field names
- Validation of enum values
- Hover documentation
- Error highlighting

## Tips for Creating Themes

### 1. Start with an Example

Copy an existing theme and modify it:
```bash
cp themes/examples/norton_blue.yaml themes/my_theme.yaml
```

### 2. Test Color Contrast

Ensure good contrast between text and background:
- Light backgrounds → Dark text (e.g., 250,250,250 bg with 30,30,30 text)
- Dark backgrounds → Light text (e.g., 30,30,30 bg with 220,220,220 text)

### 3. Consistent Color Palette

Choose 3-5 main colors and use them consistently:
- Background color
- Text color
- Accent color (for highlights/hover)
- Border color
- Disabled state color (usually grayed out)

### 4. Test Accessibility

- Use sufficient contrast ratios (WCAG guidelines recommend 4.5:1 minimum)
- Test with different terminal emulators
- Consider users with color blindness

### 5. Match Your Application's Mood

- Professional apps → Muted colors, rounded borders
- Retro/DOS apps → Bright colors, single/double-line borders
- Modern apps → Dark themes with subtle accents
- Accessibility apps → High contrast, bold borders, large padding

## File Organization

Recommended directory structure:

```
themes/
  examples/           # Built-in example themes (read-only)
    norton_blue.yaml
    dark_professional.yaml
    ...

  custom/            # Your custom themes
    my_company_theme.yaml
    dark_green.yaml

  community/         # Community-contributed themes
    dracula.yaml
    solarized.yaml

  conio-theme.schema.json  # Auto-generated JSON schema
```

## Theme File Validation

Test your theme files:

```bash
# Run unit tests (validates all example themes)
./build/bin/ui_unittest --test-case="Example Themes*"

# Or test loading programmatically
./your_app --theme my_theme.yaml
```

## Common Issues

### Theme Not Found
**Error:** `Theme file not found: ...`

**Solution:** Check the file path and working directory. Use absolute paths if needed.

### Invalid YAML Syntax
**Error:** `Failed to parse YAML...`

**Solution:** Check for:
- Unclosed quotes
- Missing colons
- Incorrect indentation (use spaces, not tabs)
- Missing commas in array/object notation

### Invalid Enum Value
**Error:** Field uses default value instead of expected

**Solution:** Check that enum values match exactly (case-sensitive):
- Box styles: `none`, `single_line`, `double_line`, `rounded`, `heavy`
- Alignment: `left`, `center`, `right`, `stretch`

### Color Out of Range
RGB values must be 0-255. Values outside this range are truncated.

## Contributing Themes

Want to contribute a theme to the examples collection?

1. Create your theme file
2. Test it thoroughly
3. Add descriptive comments
4. Submit a pull request

Good themes for contribution:
- Classic terminal emulator themes (e.g., Solarized, Dracula)
- Accessibility themes
- Regional/cultural themes
- Application-specific themes

## Support

For issues or questions:
- Check the unit tests in `unittest/reflection/test_example_themes.cc`
- Review the comprehensive test coverage in `test_coverage_analysis.cc`
- See the implementation in `include/onyxui/theme_loader.hh`

## Version Compatibility

These YAML themes work with:
- OnyxUI 1.0+ with YAML themes enabled
- Any backend implementing the theme structure
- C++20 or later

---

**Happy theming!** 🎨
