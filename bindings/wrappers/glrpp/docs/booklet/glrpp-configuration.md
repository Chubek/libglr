Configuration in glrpp is lightweight and mostly architectural rather than centralized in one giant settings object. That is a good match for a header-only wrapper around a dynamic runtime.

## Main configuration surfaces

You configure glrpp through:

- build-system include paths and runtime library availability
- parser construction choices such as whether a scanner is attached
- scanner rule sets and priorities
- grammar design choices such as literal versus named terminals
- application-level diagnostic and semantic policies

## Build-time configuration

At build time, the important questions are:

- where are the headers?
- where are the third-party includes?
- will `libltdl` and libglr be available at runtime?

Because the wrapper is header-only, there are few compiled wrapper settings to worry about.

## Runtime configuration

At runtime, configuration is often embodied in which parser you construct. For example:

```cpp
glrpp::glr::parser scannerless(grammar);
glrpp::glr::parser hybrid(grammar, scanner);
```

That single choice changes how text input is interpreted and whether lexer hooks are installed.

## Scanner policy as configuration

Scanner priorities, skip rules, and token naming conventions are all configuration choices. They define the lexical contract for the rest of the parser pipeline.

## Semantic and diagnostic configuration

Most user-facing configuration lives above the parser core:

- strict versus permissive error handling
- how much ambiguity to tolerate
- whether to keep raw forests or build custom ASTs immediately
- how richly to format diagnostics

A good glrpp configuration strategy keeps the parser core small and pushes policy to explicit, testable application code.
