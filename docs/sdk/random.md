# Random Number Generation

Sindarin provides secure random number generation through the `Random` type. By default, all random operations use operating system entropy for cryptographically suitable randomness, with an optional seeded mode for reproducible sequences.

## Quick Start

```sindarin
// Generate random values - no setup required
var dice: int = Random.int(1, 6)
var coin: bool = Random.bool()
var percentage: double = Random.double(0.0, 100.0)

// Pick from a collection
var colors: str[] = {"red", "green", "blue"}
var pick: str = Random.choice(colors)
```

## Design Philosophy

### Secure by Default

Unlike many languages that require explicit seeding or initialization, Sindarin's `Random` type uses OS entropy by default. Every call to a static method draws from the operating system's random number generator (`getrandom()` on Linux, equivalent facilities on other platforms).

This design choice means:
- No initialization ceremony required
- No risk of predictable sequences from forgotten or weak seeds
- Cryptographically suitable for security-sensitive applications
- Simpler mental model - just call the method and get a random value

### Reproducible When Needed

For testing, simulations, and procedural generation where reproducibility matters, create a seeded `Random` instance:

```sindarin
var rng: Random = Random.createWithSeed(42)
var value: int = rng.int(1, 100)  // Same value every time with seed 42
```

## Static Methods

Static methods use OS entropy and require no initialization.

### Factory Methods

| Method | Signature | Description |
|--------|-----------|-------------|
| `create` | `(): Random` | Create an OS-entropy backed instance |
| `createWithSeed` | `(seed: long): Random` | Create a seeded PRNG instance for reproducible sequences |

### Value Generation

| Method | Signature | Description |
|--------|-----------|-------------|
| `int` | `(min: int, max: int): int` | Random integer in range [min, max] inclusive |
| `long` | `(min: long, max: long): long` | Random long in range [min, max] inclusive |
| `double` | `(min: double, max: double): double` | Random double in range [min, max) |
| `bool` | `(): bool` | Random boolean (50/50) |
| `byte` | `(): byte` | Random byte (0-255) |
| `bytes` | `(count: int): byte[]` | Array of random bytes |
| `gaussian` | `(mean: double, stddev: double): double` | Sample from normal distribution |

### Batch Generation

Generate multiple values in a single call for performance-sensitive loops:

| Method | Signature | Description |
|--------|-----------|-------------|
| `intMany` | `(min: int, max: int, count: int): int[]` | Array of random integers |
| `longMany` | `(min: long, max: long, count: int): long[]` | Array of random longs |
| `doubleMany` | `(min: double, max: double, count: int): double[]` | Array of random doubles |
| `boolMany` | `(count: int): bool[]` | Array of random booleans |
| `gaussianMany` | `(mean: double, stddev: double, count: int): double[]` | Array of gaussian samples |

### Collection Operations

| Method | Signature | Description |
|--------|-----------|-------------|
| `choice` | `(array: T[]): T` | Random element from array |
| `weightedChoice` | `(array: T[], weights: double[]): T` | Random element with probability weights |
| `shuffle` | `(array: T[]): void` | Shuffle array in place |
| `sample` | `(array: T[], count: int): T[]` | Random sample without replacement |

## Instance Methods

When you have a `Random` instance (from `create()` or `createWithSeed()`), the same operations are available as instance methods. This allows functions to accept a `Random` parameter and work identically whether it uses OS entropy or a seed.

### Value Generation

| Method | Signature | Description |
|--------|-----------|-------------|
| `.int` | `(min: int, max: int): int` | Next random integer |
| `.long` | `(min: long, max: long): long` | Next random long |
| `.double` | `(min: double, max: double): double` | Next random double |
| `.bool` | `(): bool` | Next random boolean |
| `.byte` | `(): byte` | Next random byte |
| `.bytes` | `(count: int): byte[]` | Next random bytes |
| `.gaussian` | `(mean: double, stddev: double): double` | Sample from normal distribution |

### Batch Generation

| Method | Signature | Description |
|--------|-----------|-------------|
| `.intMany` | `(min: int, max: int, count: int): int[]` | Array of random integers |
| `.longMany` | `(min: long, max: long, count: int): long[]` | Array of random longs |
| `.doubleMany` | `(min: double, max: double, count: int): double[]` | Array of random doubles |
| `.boolMany` | `(count: int): bool[]` | Array of random booleans |
| `.gaussianMany` | `(mean: double, stddev: double, count: int): double[]` | Array of gaussian samples |

### Collection Operations

| Method | Signature | Description |
|--------|-----------|-------------|
| `.choice` | `(array: T[]): T` | Random element |
| `.weightedChoice` | `(array: T[], weights: double[]): T` | Random element with weights |
| `.shuffle` | `(array: T[]): void` | Shuffle with this generator |
| `.sample` | `(array: T[], count: int): T[]` | Random sample without replacement |

## Examples

### Basic Random Values

```sindarin
fn main(): void =>
  // Dice roll
  var roll: int = Random.int(1, 6)
  print($"You rolled: {roll}\n")

  // Coin flip
  if Random.bool() =>
    print("Heads!\n")
  else =>
    print("Tails!\n")

  // Percentage (0.0 to 100.0)
  var chance: double = Random.double(0.0, 100.0)
  print($"Random percentage: {chance}%\n")
```

### Working with Collections

```sindarin
fn main(): void =>
  // Pick a random color
  var colors: str[] = {"red", "green", "blue", "yellow"}
  var picked: str = Random.choice(colors)
  print($"Selected color: {picked}\n")

  // Shuffle a deck
  var cards: int[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10}
  Random.shuffle(cards)
  print($"Shuffled: {cards.join(\", \")}\n")

  // Sample without replacement
  var lottery: int[] = Random.sample(cards, 3)
  print($"Winning numbers: {lottery.join(\", \")}\n")
```

### Weighted Selection

```sindarin
fn main(): void =>
  // Loot drop with rarity weights
  var items: str[] = {"common", "rare", "legendary"}
  var weights: double[] = {0.7, 0.25, 0.05}

  var drop: str = Random.weightedChoice(items, weights)
  print($"You found: {drop}\n")
```

### Statistical Sampling

```sindarin
fn main(): void =>
  // Generate heights from normal distribution
  // mean=170cm, standard deviation=10cm
  var height: double = Random.gaussian(170.0, 10.0)
  print($"Generated height: {height} cm\n")

  // Batch generation for simulations
  var samples: double[] = Random.gaussianMany(0.0, 1.0, 1000)
  print($"Generated {samples.length} standard normal samples\n")
```

### Reproducible Sequences

```sindarin
fn main(): void =>
  // Create seeded generator for reproducibility
  var rng: Random = Random.createWithSeed(12345)

  // These values will be identical every run
  var a: int = rng.int(1, 100)
  var b: int = rng.int(1, 100)
  var c: int = rng.int(1, 100)

  print($"Sequence: {a}, {b}, {c}\n")
```

### Procedural Generation

```sindarin
fn generateTerrain(seed: long, width: int, height: int): int[][] =>
  var rng: Random = Random.createWithSeed(seed)
  var terrain: int[][] = {}

  for var y: int = 0; y < height; y++ =>
    var row: int[] = rng.intMany(0, 9, width)
    terrain.push(row)

  return terrain

fn main(): void =>
  // Same seed = same terrain
  var map: int[][] = generateTerrain(42, 10, 5)
  for row in map =>
    print($"{row.join(\"\")}\n")
```

### Cryptographic Bytes

```sindarin
fn main(): void =>
  // Generate a 32-byte key (256 bits)
  var key: byte[] = Random.bytes(32)
  print($"Generated {key.length}-byte key\n")

  // Convert to hex for display
  var hex: str = key.toHex()
  print($"Key (hex): {hex}\n")
```

## Notes

- **Range bounds**: `int`, `long`, and related methods use inclusive ranges [min, max]. `double` uses half-open range [min, max).
- **Empty arrays**: `choice` and `sample` will panic if called on an empty array.
- **Weight validation**: `weightedChoice` requires the weights array to have the same length as the items array, with all weights being non-negative.
- **Sample size**: `sample` will panic if `count` exceeds the array length (since it samples without replacement).

## See Also

- [UUID](uuid.md) - UUID generation (uses random internally)
- [SDK Overview](readme.md) - All SDK modules
- [Arrays](../arrays.md) - Array operations
