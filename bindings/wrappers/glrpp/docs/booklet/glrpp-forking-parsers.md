Forking is native to GLR parsing. When the runtime encounters a conflict, it conceptually forks the parser state so multiple hypotheses can proceed in parallel. This chapter explains that idea from the wrapper user’s point of view.

## Why parser forking happens

Forking occurs when the current state admits more than one valid action, typically because the grammar is ambiguous or intentionally broad. Instead of choosing prematurely, the runtime explores all viable paths.

## Graph-structured stacks

Naively, forking would duplicate the full parse stack for every branch. GLR avoids that cost with a graph-structured stack. Shared prefixes remain shared, and only diverging suffixes branch.

## User-visible consequences

As a glrpp user, you usually notice forking indirectly through:

- a parse forest rather than a single tree
- more memory use on ambiguous input
- a need for later disambiguation

## Speculative parsing use cases

You can also use the idea of forking at a higher application level:

- try different start symbols against the same token stream
- parse with and without a scanner to compare behavior
- branch semantic interpretation after one broad parse

These are not the same as the runtime’s internal forks, but they follow the same philosophy: delay commitment until you have evidence.

## Example scenario

Consider a language where `<` might begin a generic argument list or a comparison. An LL parser often needs ad hoc lookahead tricks. A GLR parser can fork naturally when it reaches the ambiguous point and carry both readings until later context resolves the issue.

## Operational advice

- do not fear forking merely because it sounds expensive
- do fear uncontrolled ambiguity in large grammars without a pruning plan
- use targeted tests on known ambiguous constructs to understand branch growth

Forking is not a failure mode. It is generalized parsing doing exactly what it was designed to do.
