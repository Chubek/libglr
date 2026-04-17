The reader layer is where glrpp turns raw character data into the event stream expected by native lexer hooks. If the parser is the orchestrator, the reader is the adapter between textual encodings and token decisions.

## Reader responsibilities

`glrpp::glr::reader` is responsible for:

- storing the input buffer
- interpreting UTF-16 byte order correctly
- exposing token events through a hook-aware API
- preserving native byte-consumption semantics

It is not merely a file reader. It is a Unicode-aware lexical event bridge.

## Setting input

The reader accepts input in multiple forms depending on the wrapper surface:

```cpp
glrpp::glr::reader reader(scanner);
reader.set_input(u"42");
reader.set_input(std::string{"\xFE\xFF\x00\x34\x00\x32", 6});
```

The wrapper contains helpers to detect BOMs, choose little-endian or big-endian decoding, and interpret surrogate pairs.

## UTF-16 event windows

A reader event is not just a code unit. The bridge logic needs:

- a UTF-8 view of the next readable window
- a mapping from UTF-8 match boundaries back to UTF-16 byte counts
- the default runtime consumption size

This is why the reader helpers build an offset table when converting windows.

## Hook cooperation

With a scanner attached, the reader does not classify tokens itself. Instead, it prepares input for the lexer hook callback. The callback decides which terminal to accept. This separation is healthy:

- the reader understands encodings
- the scanner understands token classes
- the parser understands grammar structure

## Endianness and BOM handling

The wrapper includes logic that can distinguish UTF-16LE and UTF-16BE using a BOM when present. This matters for tests, cross-platform input fixtures, and interoperability with external tools.

## Example: native-reader probing

```cpp
auto scanner = std::make_shared<glrpp::scanner>(std::vector<glrpp::dsl::scan_rule>{
    glrpp::token_rule<"[0-9]+">("number", 10),
});

glrpp::glr::reader reader(scanner);
reader.set_input(u"42");
auto next = reader.next();
```

If `next` succeeds, it typically reports terminal name, hook origin, and consumed byte count in UTF-16 units.

## When to customize reading behavior

You will care about the reader directly when:

- testing Unicode and encoding correctness
- implementing preprocessing before lexical classification
- validating hook behavior without running a full parse
- integrating non-file sources such as editor buffers or network payloads

The reader layer is where raw bytes become parser-visible structure. Understanding it pays off whenever Unicode and contextual lexing enter the picture.
