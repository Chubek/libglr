Real parsers rarely live in one file. Grammar fragments, scanner rules, semantic utilities, and domain models all depend on one another. Managing those dependencies well keeps a glrpp codebase readable.

## Organize by language layer

A practical project split is:

- grammar definitions
- scanning rules
- parser assembly
- semantic AST and actions
- diagnostics and tooling

Each layer should depend downward more than sideways.

## Cross-grammar composition

When a language has embedded sublanguages or reusable fragments, prefer composition through factory functions and helper expressions rather than copy-pasting rule sets.

Example:

```cpp
glrpp::dsl::grammar sql_core_grammar();
glrpp::dsl::grammar expr_fragment();
```

A higher-level builder can merge or adapt these pieces before parser construction.

## Avoid cyclic header dependencies

Because glrpp is header-only, include hygiene matters. Keep grammar declarations in headers and place heavyweight helper logic in source files where possible in your own project. If everything becomes templates, document the include structure carefully.

## Third-party dependencies

glrpp itself relies on bundled libraries such as CTRE and libltdl. Application code should avoid reaching into those dependencies directly unless it has a clear reason. Prefer the wrapper’s surface area first.

## Dependency advice for tests

Keep tests narrow:

- grammar tests should not need the full semantic layer
- scanner tests should not require the parser when avoidable
- metaprogramming tests should compile headers without dragging runtime concerns in unnecessarily

Good dependency management keeps each failure local and understandable.
