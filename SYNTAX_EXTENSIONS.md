# DaScript Syntax Extensions for GDScript Parity

## Changes Made

### 1. Added `#` as Alternative Single-Line Comment

Both `//` and `#` can now be used for single-line comments, making DaScript more GDScript-like:

```das
# This is a comment using #
// This is also a comment using //

var value : int = 42  # inline comment
var other : int = 10  // inline comment
```

### 2. Added `func` as Alternative to `def`

You can now use either `func` or `def` to define functions:

```das
# Using func (GDScript-style)
func add(a: int; b: int) : int
    return a + b

# Using def (traditional DaScript-style)
def multiply(a: int; b: int) : int
    return a * b
```

## Modified Files

### Lexer Source Files (`.lpp`)
- `extensions/dascript/lib/DarkScript/src/parser/ds_lexer.lpp`
- `extensions/dascript/lib/DarkScript/src/parser/ds2_lexer.lpp`

These are the Flex lexer specification files. Changes added:

1. **Comment pattern for `#`**: Added rules to recognize `#` as a single-line comment in both `<indent>` and `<normal>` modes
2. **Keyword `func`**: Added `<normal>"func"` pattern that returns `DAS_DEF` token

### To Regenerate Lexer Files

The lexer `.cpp` files are generated from `.lpp` files using Flex. To regenerate them after modifications:

#### On Linux/macOS:
```bash
cd extensions/dascript/lib/DarkScript/src/parser
flex ds_lexer.lpp
flex ds2_lexer.lpp
```

#### Using CMake (recommended):
```bash
cd extensions/dascript/lib/DarkScript
mkdir build
cd build
cmake ..
make
```

#### On Windows with winflexbison:
```cmd
cd extensions\dascript\lib\DarkScript\src\parser
flex ds_lexer.lpp
flex ds2_lexer.lpp
```

**Note**: Install `winflexbison` via scoop:
```cmd
scoop install winflexbison
```

The command is just `flex` (not `win_flex`). Version 2.5.25 or 2.6.4+ works fine.

## Test Script

A test script has been created at `godot_project/test_new_syntax.das` to demonstrate the new syntax features.

## Building

After regenerating the lexer files, rebuild the extension:

```bash
cd godot_extensions
./build.bat dascript
```

## Compatibility

These changes are backward compatible:
- Existing scripts using `//` comments will continue to work
- Existing scripts using `def` will continue to work
- The new `#` comments and `func` keyword are additional alternatives

## Implementation Details

### Comment Handling

The lexer now recognizes both `//` and `#` patterns and transitions to the `cpp_comment` state, where it reads until end-of-line. The comment content is passed to any registered comment readers (for documentation generation, etc.).

### Function Definition

Both `func` and `def` keywords are mapped to the same `DAS_DEF` token, so the parser treats them identically. This is a lexical-level alias - the actual parsing and AST generation is unchanged.
