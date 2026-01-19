# Arrays in Sindarin

Sindarin provides powerful array support with type-safe operations, slicing, and modern syntax features like range literals and spread operators.

## Declaration and Initialization

```sindarin
// Empty array
var arr: int[] = {}

// Array with initial values
var numbers: int[] = {1, 2, 3, 4, 5}

// Different types
var doubles: double[] = {1.5, 2.5, 3.5}
var chars: char[] = {'H', 'e', 'l', 'l', 'o'}
var bools: bool[] = {true, false, true}
var strings: str[] = {"hello", "world"}
```

### Multi-line Array Literals

Array literals can span multiple lines for better readability:

```sindarin
var numbers: int[] = {
    1, 2, 3,
    4, 5, 6,
    7, 8, 9
}

var bytes: byte[] = {
    72, 101, 108, 108, 111,   // "Hello"
    44, 32,                    // ", "
    87, 111, 114, 108, 100, 33 // "World!"
}

var names: str[] = {
    "Alice",
    "Bob",
    "Charlie"
}
```

Nested arrays also support multi-line formatting:

```sindarin
var matrix: int[][] = {
    {1, 2, 3},
    {4, 5, 6},
    {7, 8, 9}
}
```

## Array Properties

### Length

```sindarin
var arr: int[] = {10, 20, 30}
print(arr.length)   // 3
print(len(arr))     // 3 (alternative syntax)
```

## Array Methods

### push(value)
Adds an element to the end of the array.

```sindarin
var arr: int[] = {1, 2}
arr.push(3)  // arr is now {1, 2, 3}
```

### pop()
Removes and returns the last element.

```sindarin
var arr: int[] = {1, 2, 3}
var last: int = arr.pop()  // last = 3, arr is now {1, 2}
```

### insert(value, index)
Inserts an element at the specified index.

```sindarin
var arr: int[] = {1, 2, 3}
arr.insert(99, 1)  // arr is now {1, 99, 2, 3}
```

### remove(index)
Removes the element at the specified index.

```sindarin
var arr: int[] = {1, 2, 3}
arr.remove(1)  // arr is now {1, 3}
```

### reverse()
Reverses the array in-place.

```sindarin
var arr: int[] = {1, 2, 3}
arr.reverse()  // arr is now {3, 2, 1}
```

### clone()
Creates a shallow copy of the array.

```sindarin
var original: int[] = {1, 2, 3}
var copy: int[] = original.clone()
copy.push(4)  // original is unchanged
```

### concat(other)
Returns a new array with elements from both arrays.

```sindarin
var a: int[] = {1, 2}
var b: int[] = {3, 4}
var c: int[] = a.concat(b)  // c is {1, 2, 3, 4}
```

### indexOf(value)
Returns the index of the first occurrence, or -1 if not found.

```sindarin
var arr: int[] = {10, 20, 30}
print(arr.indexOf(20))  // 1
print(arr.indexOf(99))  // -1
```

### contains(value)
Returns true if the array contains the value.

```sindarin
var arr: int[] = {10, 20, 30}
print(arr.contains(20))  // true
print(arr.contains(99))  // false
```

### join(separator)
Joins array elements into a string.

```sindarin
var words: str[] = {"apple", "banana", "cherry"}
print(words.join(", "))  // "apple, banana, cherry"

var nums: int[] = {1, 2, 3}
print(nums.join("-"))    // "1-2-3"
```

### clear()
Removes all elements from the array.

```sindarin
var arr: int[] = {1, 2, 3}
arr.clear()  // arr is now {}
```

## Indexing

### Positive Indexing
Access elements from the start (0-based).

```sindarin
var arr: int[] = {10, 20, 30, 40, 50}
print(arr[0])  // 10
print(arr[2])  // 30
```

### Negative Indexing
Access elements from the end (-1 is last element).

```sindarin
var arr: int[] = {10, 20, 30, 40, 50}
print(arr[-1])  // 50
print(arr[-2])  // 40
```

## Slicing

Slicing creates a new array from a portion of the original.

### Basic Syntax: `arr[start..end]`
- `start`: inclusive start index (default: 0)
- `end`: exclusive end index (default: length)

```sindarin
var arr: int[] = {10, 20, 30, 40, 50}

arr[1..4]   // {20, 30, 40}
arr[..3]    // {10, 20, 30}
arr[2..]    // {30, 40, 50}
arr[..]     // {10, 20, 30, 40, 50} (full copy)
```

### Step Slicing: `arr[start..end:step]`

```sindarin
var arr: int[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9}

arr[..:2]    // {0, 2, 4, 6, 8} (every 2nd element)
arr[1..:2]   // {1, 3, 5, 7, 9} (odd indices)
arr[..:3]    // {0, 3, 6, 9} (every 3rd element)
```

### Negative Indices in Slices

```sindarin
var arr: int[] = {10, 20, 30, 40, 50}

arr[-2..]    // {40, 50} (last two)
arr[..-1]    // {10, 20, 30, 40} (all but last)
arr[-3..-1]  // {30, 40} (from -3 to -1, exclusive)
```

## Range Literals

Range literals create integer arrays using the `..` operator.

### Syntax: `start..end`
Creates an array from `start` to `end` (exclusive).

```sindarin
var r: int[] = 1..6    // {1, 2, 3, 4, 5}
var r2: int[] = 0..10  // {0, 1, 2, 3, 4, 5, 6, 7, 8, 9}
```

### Range in Array Literals
Ranges can be used inside array literals.

```sindarin
var arr: int[] = {0, 1..4, 10}     // {0, 1, 2, 3, 10}
var arr2: int[] = {1..3, 10..13}   // {1, 2, 10, 11, 12}
```

## Spread Operator

The spread operator (`...`) expands an array inside an array literal.

### Clone with Spread

```sindarin
var original: int[] = {1, 2, 3}
var copy: int[] = {...original}  // {1, 2, 3}
```

### Prepend and Append

```sindarin
var arr: int[] = {1, 2, 3}
var extended: int[] = {0, ...arr, 4, 5}  // {0, 1, 2, 3, 4, 5}
```

### Combine Arrays

```sindarin
var a: int[] = {1, 2}
var b: int[] = {3, 4}
var merged: int[] = {...a, ...b}  // {1, 2, 3, 4}
```

### Mix Spread and Range

```sindarin
var arr: int[] = {1, 2, 3}
var mixed: int[] = {...arr, 10..13}  // {1, 2, 3, 10, 11, 12}
```

## Arrays of Lambdas (Function Types)

Arrays can hold lambda/function types, but **parentheses are required** to disambiguate the syntax.

### The Ambiguity Problem

Without parentheses, the syntax is ambiguous:

```sindarin
// AMBIGUOUS - don't do this!
var arr: fn(str, str): str[]   // Does this mean:
                                //   1. Lambda returning str[] ?
                                //   2. Array of lambdas returning str ?
```

### Solution: Use Parentheses

```sindarin
// Array of lambdas (parentheses required)
var handlers: (fn(str): int)[] = {}
var transforms: (fn(int, int): int)[] = {}

// Lambda returning an array (no parentheses needed - this is the default interpretation)
var getNames: fn(): str[] = () => { return {"Alice", "Bob"} }
```

### Parser Behavior

The parser binds `[]` to the innermost type first. So:
- `fn(str): str[]` = function returning `str[]` (array binds to return type)
- `(fn(str): str)[]` = array of functions returning `str` (array binds to whole function type)

### Example Usage

```sindarin
// Declare array of lambdas
var operations: (fn(int, int): int)[] = {}

// Add lambdas to the array
var add: fn(int, int): int = (a: int, b: int) => a + b
var mul: fn(int, int): int = (a: int, b: int) => a * b
operations.push(add)
operations.push(mul)

// Call lambdas from the array
var result1: int = operations[0](10, 5)  // 15
var result2: int = operations[1](10, 5)  // 50
```

### Compiler Error

If the compiler detects a potentially ambiguous declaration, it may emit an error:

```
error: Ambiguous array declaration. Use parentheses to clarify:
  - For array of functions: (fn(str): str)[]
  - For function returning array: fn(str): str[]
```

## Array Equality

Arrays can be compared for equality.

```sindarin
var a: int[] = {1, 2, 3}
var b: int[] = {1, 2, 3}
var c: int[] = {1, 2, 4}

print(a == b)  // true
print(a == c)  // false
print(a != c)  // true
```

## For-Each Iteration

```sindarin
var arr: int[] = {10, 20, 30}

for x in arr =>
  print($"value: {x}\n")

// Calculate sum
var sum: int = 0
for n in arr =>
  sum = sum + n
print($"Sum: {sum}\n")
```

## Complete Example

```sindarin
fn main(): void =>
  // Create array with range
  var numbers: int[] = 1..6
  print("Initial: ")
  print(numbers)
  print("\n")

  // Add more elements with spread
  var extended: int[] = {0, ...numbers, 6..9}
  print("Extended: ")
  print(extended)
  print("\n")

  // Slice and iterate
  var middle: int[] = extended[2..6]
  var sum: int = 0
  for n in middle =>
    sum = sum + n
  print($"Sum of middle elements: {sum}\n")
```
