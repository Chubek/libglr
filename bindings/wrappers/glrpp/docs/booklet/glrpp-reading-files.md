Parsing usually starts from files, not string literals. glrpp itself focuses on parsing and tokenization, so file reading is intentionally lightweight and explicit. The wrapper expects you to decide how much input to buffer and when.

## Whole-file reading

For many grammars, the simplest strategy is to read the full file into memory before parsing:

```cpp
std::ifstream in(path, std::ios::binary);
std::string source((std::istreambuf_iterator<char>(in)), {});
auto result = parser.parse(source);
```

Whole-file buffering is a good default when:

- inputs are small or moderate
- diagnostics need stable offsets into the full original source
- scannerless parsing benefits from contiguous storage

## Binary versus text mode

Prefer binary mode when reading parser input. It prevents platform newline translation from silently altering byte offsets. That matters because glrpp diagnostics track offsets and the reader bridge reasons about byte counts.

## Memory tradeoffs

Whole-file reads are easy, but not always ideal. Large inputs can inflate memory use in two places:

- the raw source buffer
- the parse forest for ambiguous or deeply nested constructs

If memory is tight, consider splitting work by file sections or grammar units. Even when libglr itself processes contiguous buffers, your higher-level application can still chunk work at logical boundaries.

## Feeding pre-buffered views

The parser accepts `std::string_view` in text mode, so you can parse slices of a larger buffer safely as long as the underlying storage outlives the parse call:

```cpp
std::string source = load_file();
std::string_view header = std::string_view(source).substr(0, 256);
auto result = parser.parse(header);
```

This is useful for embedded languages, configuration preambles, or targeted diagnostics.

## Incremental reading patterns

glrpp does not yet expose a dedicated incremental parser facade, but you can still model incremental workflows by:

- maintaining your own document buffer
- reparsing only the affected region or syntax unit
- caching semantic products above the parse layer

For editor integrations, a common pattern is "small region parse plus surrounding context" rather than true streaming parse state reuse.

## File encodings

The wrapper mostly assumes UTF-8 text at the public API boundary. When a scanner is attached, it converts UTF-8 input to UTF-16 bytes for the native reader hook path. If your files are already UTF-16, decode or normalize them at the file layer unless you are deliberately testing the reader bridge itself.

## Source mapping advice

When you read files, keep these artifacts together:

- the original byte buffer
- the file path
- a line index or newline table if you need random-access diagnostics
- any preprocessor or include expansion maps

This makes it easier to transform raw `parse_diagnostic` values into user-facing messages.

## Example utility

```cpp
struct source_file {
  std::string path;
  std::string text;
};

source_file read_source(const std::string& path) {
  std::ifstream in(path, std::ios::binary);
  return {path, std::string((std::istreambuf_iterator<char>(in)), {})};
}
```

Read deliberately, keep offsets stable, and treat file loading as part of your diagnostic pipeline rather than a mere prelude.
