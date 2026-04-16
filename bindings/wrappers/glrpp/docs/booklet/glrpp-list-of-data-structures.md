This chapter is a compact reference to the most important data structures exposed by glrpp or conceptually central to its operation.

## DSL structures

### `glrpp::dsl::symbol`
Fields:

- `std::string name`
- `symbol_kind kind`

Role: names terminals, nonterminals, literals, or epsilon.

### `glrpp::dsl::expression`
Fields:

- `expr_kind kind`
- `symbol atom`
- `std::vector<expression> children`

Role: runtime grammar expression tree.

### `glrpp::dsl::rule`
Fields:

- `std::string lhs`
- `expression rhs`
- optional reducer action

Role: one production rule.

### `glrpp::dsl::grammar`
Fields:

- start symbol string
- vector of rules

Role: validated runtime grammar container.

## Lexical structures

### `glrpp::dsl::token`
Fields:

- `kind`, `lexeme`
- `offset`, `line`, `column`
- `bytes_consumed`, `codepoint`, `from_hook`

Role: lexical unit consumed by token-stream parsing or emitted by the scanner.

### `glrpp::dsl::scan_rule`
Fields:

- `name`
- `priority`
- `skip`
- matcher function pointer

Role: one CTRE-backed lexical rule.

### `glrpp::dsl::scanner`
Fields:

- vector of scan rules

Role: performs longest-match tokenization and hook-time classification.

## Semantic helper structures

### `glrpp::dsl::ast_node`
Fields:

- `kind`
- variant payload

Role: lightweight generic semantic tree node.

### `glrpp::dsl::ast_array`, `ast_object`
Role: aggregate payload containers for `ast_node`.

## Runtime wrapper structures

### `glrpp::glr::parser`
Holds:

- runtime grammar value
- optional scanner
- native grammar handle
- native parser handle
- optional lexer hook registry

Role: owns parser assembly and parse entry points.

### `glrpp::glr::reader`
Role: bridges raw text, UTF-16 decoding, and lexer hook events.

### `glrpp::glr::forest`
Role: wrapper over the native parse forest.

### `glrpp::glr::node`
Role: wrapper over individual forest nodes.

### `glrpp::glr::context`
Role: dynamic loader and symbol resolver for libglr.

## Meta structures

### `glrpp::meta::type_list<Ts...>`
Role: compile-time list of types.

### `glrpp::meta::fields<T>`
Role: customization point for reflection field names.

## Error structures

### `glrpp::util::parse_diagnostic`
Fields:

- `message`, `expected`, `found`
- source position
- consumed count

Role: structured error report.

### `glrpp::util::expected<Value, Error>`
Role: explicit success-or-error carrier used throughout the wrapper.

Understanding these structures gives you a mental map of the entire library: grammar front-end, lexical bridge, runtime parser, semantic helpers, and meta support.
