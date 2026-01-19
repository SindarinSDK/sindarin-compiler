# Strings in Sindarin

Sindarin provides powerful string manipulation with string interpolation, comprehensive methods, and escape sequences.

## String Literals

Basic string literals use double quotes:

```sindarin
var greeting: str = "Hello, World!"
var empty: str = ""
```

## Multi-line Strings (Pipe Block Syntax)

For multi-line strings, use the pipe block syntax with `|`:

```sindarin
var sql: str = |
    SELECT *
    FROM users
    WHERE active = true

print(sql)
```

Output:
```
SELECT *
FROM users
WHERE active = true
```

### How It Works

- The `|` character introduces a multi-line string block
- Content starts on the line after `|`
- Leading whitespace is automatically stripped based on the minimum indentation
- The string ends when a line with less indentation than the starting `|` is encountered

### Preserving Relative Indentation

Inner indentation relative to the first line is preserved:

```sindarin
var code: str = |
    fn main():
        print("Hello")
        if true:
            print("World")

print(code)
```

Output:
```
fn main():
    print("Hello")
    if true:
        print("World")
```

### Interpolated Multi-line Strings

Use `$|` for multi-line strings with interpolation:

```sindarin
var name: str = "Alice"
var age: int = 30

var greeting: str = $|
    Hello {name}!
    You are {age} years old.
    Next year you'll be {age + 1}.

print(greeting)
```

Output:
```
Hello Alice!
You are 30 years old.
Next year you'll be 31.
```

### Common Use Cases

**SQL Queries:**
```sindarin
var table: str = "users"
var query: str = $|
    SELECT id, name, email
    FROM {table}
    WHERE created_at > '2024-01-01'
    ORDER BY name ASC
```

**HTML Templates:**
```sindarin
var title: str = "Welcome"
var html: str = $|
    <!DOCTYPE html>
    <html>
    <head><title>{title}</title></head>
    <body>
        <h1>{title}</h1>
    </body>
    </html>
```

**JSON Data:**
```sindarin
var name: str = "test"
var value: int = 42
var json: str = $|
    {
        "name": "{name}",
        "value": {value}
    }
```

## String Interpolation

Use the `$` prefix to embed expressions in strings:

```sindarin
var name: str = "World"
var count: int = 42
print($"Hello, {name}! The answer is {count}.\n")
```

Works with all types:

```sindarin
var pi: double = 3.14
var flag: bool = true
print($"Pi is {pi}, flag is {flag}\n")
```

Expressions inside `{}` are evaluated and converted to strings:

```sindarin
var a: int = 5
var b: int = 3
print($"Sum: {a + b}, Product: {a * b}\n")  // "Sum: 8, Product: 15"
```

## Escape Sequences

The following escape sequences are supported in string literals:

| Escape | Character |
|--------|-----------|
| `\n` | Newline |
| `\t` | Tab |
| `\r` | Carriage return |
| `\\` | Backslash |
| `\"` | Double quote |
| `\'` | Single quote |
| `\0` | Null character |

Example:

```sindarin
print("Line 1\nLine 2\n")
print("Column1\tColumn2\tColumn3\n")
print("She said \"Hello!\"\n")
print("Path: C:\\Users\\name\n")
```

## String Properties

### length

Returns the number of characters in the string:

```sindarin
var size: int = "hello".length  // 5
var empty: int = "".length      // 0
```

## String Methods

### Case Conversion

#### toUpper()

Converts all characters to uppercase:

```sindarin
var text: str = "Hello World"
var upper: str = text.toUpper()  // "HELLO WORLD"
```

#### toLower()

Converts all characters to lowercase:

```sindarin
var text: str = "Hello World"
var lower: str = text.toLower()  // "hello world"
```

### Trimming

#### trim()

Removes whitespace from both ends:

```sindarin
var text: str = "  Hello World  "
var trimmed: str = text.trim()  // "Hello World"
```

### Substring Extraction

#### substring(start, end)

Extracts a portion of the string from `start` (inclusive) to `end` (exclusive):

```sindarin
var text: str = "Hello World"
var sub: str = text.substring(0, 5)   // "Hello"
var sub2: str = text.substring(6, 11) // "World"
```

#### charAt(index)

Returns the character at the specified index:

```sindarin
var ch: char = "Hello".charAt(0)  // 'H'
var ch2: char = "Hello".charAt(4) // 'o'
```

### Searching

#### indexOf(substring)

Returns the index of the first occurrence of a substring, or -1 if not found:

```sindarin
var idx: int = "hello".indexOf("ll")   // 2
var notFound: int = "hello".indexOf("x")  // -1
```

#### contains(substring)

Returns true if the string contains the substring:

```sindarin
var has: bool = "hello".contains("ell")  // true
var no: bool = "hello".contains("xyz")   // false
```

#### startsWith(prefix)

Returns true if the string starts with the prefix:

```sindarin
var starts: bool = "hello".startsWith("he")   // true
var no: bool = "hello".startsWith("lo")       // false
```

#### endsWith(suffix)

Returns true if the string ends with the suffix:

```sindarin
var ends: bool = "hello".endsWith("lo")   // true
var no: bool = "hello".endsWith("he")     // false
```

### Replacement

#### replace(old, new)

Replaces all occurrences of a substring:

```sindarin
var replaced: str = "hello".replace("l", "L")  // "heLLo"
var updated: str = "foo bar foo".replace("foo", "baz")  // "baz bar baz"
```

### Appending

#### append(other)

Appends a string to the end of the current string, modifying it in place. Returns the string (which must be reassigned as the pointer may change due to reallocation):

```sindarin
var text: str = "Hello"
text = text.append(" World")  // "Hello World"
text = text.append("!")       // "Hello World!"
```

The `append` method is optimized for efficient string building with amortized O(1) performance. When building strings incrementally, prefer `append` over repeated concatenation:

```sindarin
// Efficient: uses append for incremental building
var result: str = ""
for i in 0..10 =>
  result = result.append($"{i} ")

// Less efficient: creates intermediate strings
var result2: str = ""
for i in 0..10 =>
  result2 = result2 + $"{i} "  // Creates new string each iteration
```

**Important:** Always reassign the result of `append` back to the variable, as the underlying buffer may be reallocated:

```sindarin
var s: str = "start"
s = s.append(" middle")  // Correct: reassign the result
s = s.append(" end")
```

### Splitting

#### split(delimiter)

Splits a string by a delimiter and returns an array:

```sindarin
var parts: str[] = "a,b,c".split(",")   // {"a", "b", "c"}
var words: str[] = "one two three".split(" ")  // {"one", "two", "three"}
```

#### splitWhitespace()

Splits on any whitespace (spaces, tabs, newlines):

```sindarin
var words: str[] = "hello   world\tfoo".splitWhitespace()  // {"hello", "world", "foo"}
```

#### splitLines()

Splits on newline characters (`\n`, `\r\n`, `\r`):

```sindarin
var lines: str[] = "line1\nline2\nline3".splitLines()  // {"line1", "line2", "line3"}
```

### Validation

#### isBlank()

Returns true if the string is empty or contains only whitespace:

```sindarin
var blank: bool = "   ".isBlank()      // true
var empty: bool = "".isBlank()         // true
var notBlank: bool = "hi".isBlank()    // false
var withText: bool = "  x  ".isBlank() // false
```

### Byte Conversion

#### toBytes()

Converts the string to a byte array (UTF-8 encoding):

```sindarin
var bytes: byte[] = "Hello".toBytes()  // {72, 101, 108, 108, 111}
```

## Method Chaining

String methods can be chained together:

```sindarin
var result: str = "  HELLO  ".trim().toLower()  // "hello"
var words: str[] = "  One Two Three  ".trim().toLower().split(" ")  // {"one", "two", "three"}
```

## Common Patterns

### Reading and Processing Lines

```sindarin
var content: str = TextFile.readAll("data.txt")
var lines: str[] = content.splitLines()
for line in lines =>
  if !line.isBlank() =>
    print($"Processing: {line}\n")
```

### Word Counting

```sindarin
var text: str = TextFile.readAll("document.txt")
var words: str[] = text.splitWhitespace()
print($"Word count: {words.length}\n")
```

### CSV Parsing

```sindarin
var line: str = "name,age,city"
var fields: str[] = line.split(",")
for field in fields =>
  print($"Field: {field.trim()}\n")
```

### Case-Insensitive Comparison

```sindarin
fn equalsIgnoreCase(a: str, b: str): bool =>
  return a.toLower() == b.toLower()

var same: bool = equalsIgnoreCase("Hello", "HELLO")  // true
```

### String Building with Join

```sindarin
var parts: str[] = {"apple", "banana", "cherry"}
var csv: str = parts.join(",")          // "apple,banana,cherry"
var sentence: str = parts.join(" and ") // "apple and banana and cherry"
```

## See Also

- [Arrays](arrays.md) - Array operations including string arrays
- [SDK I/O documentation](sdk/io/readme.md) - File I/O for reading and writing text files
