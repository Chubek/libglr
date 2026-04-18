UTF-16 conversion is a central part of glrpp’s scanner bridge. Even if your public API uses UTF-8 strings, the native hook path needs UTF-16-aware accounting so that libglr and CTRE can agree on how much input was consumed.

## Why UTF-16 appears at all

The wrapper is built around libglr’s native reader and lexer-hook interfaces. Those interfaces reason about native character windows and byte consumption. To support Unicode correctly and portably, glrpp converts UTF-8 input into BOM-tagged UTF-16LE bytes before calling the runtime when a scanner is attached.

## Key tasks in conversion

The wrapper’s conversion helpers must:

- prepend or detect a BOM
- decode little-endian and big-endian UTF-16 correctly
- handle surrogate pairs
- map UTF-8 match boundaries back to UTF-16 byte counts

That last point is the one people miss most often. A CTRE match length in UTF-8 bytes is not enough. The runtime must know how many native bytes to advance in the original UTF-16 stream.

## Surrogate pairs

Characters outside the Basic Multilingual Plane are represented as surrogate pairs in UTF-16. A scanner that matches a single emoji may consume four UTF-8 bytes but also four UTF-16 bytes, split across two code units. The conversion layer must preserve that exact accounting.

## Endianness

The reader helpers can detect whether a BOM marks the buffer as big-endian or little-endian. This is especially helpful in tests, where you may want to prove that both representations produce identical tokenization results.

## Example mental mapping

Take the UTF-8 string `"hello 🌍"`.

- the word portion maps one byte per ASCII character in UTF-8 and two bytes per code unit in UTF-16
- the globe emoji maps four bytes in UTF-8 and four bytes in UTF-16 via a surrogate pair

When the scanner matches `emoji`, the bridge must return the UTF-16 byte count expected by the runtime, not just the UTF-8 match width.

## Performance considerations

Conversion costs time, but the payoff is correct scanner-hook integration. In practice:

- deterministic ASCII-heavy input stays simple
- non-ASCII input remains correct rather than silently misaligned
- the most expensive part is usually not conversion but ambiguity in the grammar itself

## Testing advice

Always include at least these test cases:

- pure ASCII
- BMP characters outside ASCII
- supplementary-plane characters using surrogate pairs
- UTF-16BE with BOM
- whitespace and newline tracking around non-ASCII text

UTF-16 conversion is not glamorous, but it is the difference between a Unicode-capable parser and a parser that only looks Unicode-capable on paper.
