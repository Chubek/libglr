One of the defining implementation ideas of GLR parsing is the graph-structured stack, often abbreviated GSS. Even if glrpp hides the raw structure, understanding it helps you reason about runtime behavior.

## Why a graph-structured stack exists

In deterministic LR parsing, one linear stack is enough. In GLR parsing, conflicts cause branching. Copying a full stack for every branch would be wasteful, so the runtime shares common prefixes in a graph.

## Conceptual model

Each stack node represents a parser state at a certain point. Edges represent predecessor relationships. When two parse paths converge to the same state and viable history, their stack representations can share structure again.

## Benefits

- branching does not imply full stack duplication
- merges become natural and cheap
- ambiguous prefixes stay compact

## Costs

- implementation complexity is higher than for ordinary LR stacks
- debugging requires thinking in graphs rather than simple stack traces
- heavily ambiguous grammars can still produce large structures

## Why wrapper users should care

Even though glrpp does not ask you to manipulate the GSS directly, stack behavior explains:

- why ambiguous inputs remain tractable longer than expected
- why forests can share substructures
- why parser traces are best visualized as graphs, not logs alone

## Practical takeaway

When you see branch growth during parsing, think in terms of graph width rather than duplicate whole-stack explosions. The runtime is already doing the smart thing; your job is to manage ambiguity and disambiguation policy so that the graph stays meaningful.
