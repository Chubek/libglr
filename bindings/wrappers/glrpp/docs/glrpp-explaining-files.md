A parser that only says "syntax error" is rarely enough. File-level explanation is about turning parse outcomes into something a human can act on quickly.

## What a good file explanation includes

- the file path or logical source name
- a concise primary error message
- line and column coordinates
- a short source excerpt
- a note about expected versus found constructs
- optional follow-up hints or related notes

## glrpp diagnostic inputs

`parse_diagnostic` already gives you the structured core:

- `message`
- `expected`
- `found`
- `position`
- `consumed`

That is enough to build richer file explanations in your application layer.

## Example formatter shape

```cpp
void explain_file(const std::string& path,
                  const std::string& source,
                  const glrpp::util::parse_diagnostic& d);
```

A good implementation would compute the relevant line slice, print a caret under the offending column, and append any grammar-specific hint.

## Partial parses and resilience

In editor tools, file explanations should often be tolerant rather than dramatic. A parse failure may coexist with a useful partial forest or previously cached semantic information. Design the explanation layer so it can report what is known without pretending the whole file is lost.

## Multi-file projects

When files include or embed other files, keep origin metadata available. A helpful error report can then say not only where the failure occurred, but also how that source became part of the current parse job.

Good file-level explanations are where a parsing library becomes a developer tool.
