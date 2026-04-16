String utilities in glrpp exist to support parsing work, not to replace a general-purpose text library. Their role is to help with slicing, view management, and encoding-sensitive operations that appear frequently in wrapper code.

## Why string helpers matter in a parser wrapper

Parsing code is full of boundaries:

- symbol names are strings
- tokens carry lexemes and coordinates
- scanners inspect windows of input
- readers translate between encodings
- diagnostics display expected and found fragments

Small helper functions reduce repeated indexing logic and make byte-versus-code-unit distinctions explicit.

## Typical use cases

Even if you never call a utility directly, you benefit from them in three places:

- scanner matching and diagnostic snippets
- parser serialization of token streams
- reader conversion between UTF-8 and UTF-16 representations

## Views over copies

Whenever possible, prefer `std::string_view` for transient parser work. glrpp follows that style in its public text-facing interfaces. This makes scanning and parse entry cheap and keeps the wrapper predictable.

```cpp
std::string source = load_source();
std::string_view head = std::string_view(source).substr(0, 16);
```

The rule is simple: views are excellent for read-only inspection, but the underlying storage must outlive the parse call.

## Diagnostic excerpts

One of the most common string helper tasks is extracting a short snippet around the failure point. glrpp’s parse diagnostics already carry `found` and `expected`, but application-level tools often augment those with source excerpts, carets, or highlighted ranges.

## Normalization guidance

The wrapper does not attempt aggressive Unicode normalization automatically. That is a deliberate choice. Grammar matching should not silently rewrite source text unless the application has chosen a normalization policy. If your language considers canonically equivalent forms identical, normalize at the ingestion layer and document that decision.

## Symbol names and literals

Grammar authors often mix descriptive terminals and literal punctuation. Keep symbol text stable and ASCII-friendly where possible. This makes grammar dumps, trace logs, and error explanations easier to read.

## Performance advice

- avoid repeated substring allocation in hot lexical paths
- keep token lexemes as lightweight slices until you truly need owned strings
- measure before inventing custom small-string optimizations

In a wrapper like glrpp, clarity about textual boundaries matters more than cleverness. Good string utilities are the quiet infrastructure that keeps the parser precise.
