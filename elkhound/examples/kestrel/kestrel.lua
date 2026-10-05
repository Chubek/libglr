--[[
Kestrel: a small ML-flavoured language with C-style braces.

    infixl 6 +++;                         # user-defined operator + fixity
    fn id<a>(x: a) -> a = x;
    fn map<a, b>(f: a -> b, xs: List<a>) -> List<b> =
        match xs with
            Nil        => Nil,
            Cons(h, t) => Cons(f h, map<a, b>(f, t));
    fn main() {
        let r = { x = 1, y = 2 };         # record, not block
        let n = id<Int>(3) +++ 4;         # generic call, not  (id<Int)>(3)
        show -n;                          # show (-n)   application
        show - n;                         # show - n    subtraction
    }

Why this language needs both machinery sets
-------------------------------------------
REWRITING (grammar -> grammar, before parsing)
  * EBNF helpers (X*, sep_by...)          custom primitive expand-repetition
  * epsilon / unit productions / factoring stock .grl pipeline
  * Type arrows and tuples                static ambiguity, removed outright
                                          by eliminate-ambiguity

DISAMBIGUATION (parse forest -> one tree, during/after parsing)
  * f -x vs f - x vs f-x                  predicate   (whitespace rule)
  * f<a>(b): call or (f<a)>(b)?           predicate + semantic (symbol table)
  * user-defined operator fixity          semantic -> precedence/associativity
  * lambda / if / match extent            precedence (open-ended)
  * `{ x }`, `{}`: block or record?       probability
  * best tree in the packed forest        dynamic programming (Viterbi)
--]]

require "kestrel_rewriters"

JGRM = elkg.grammar.new "Kestrel"

JGRM:Meta
{
    language = {
        name = "Kestrel",
        type = "Programming Language",
        standard = "none (experimental)",
    },

    grammar = {
        author = "Chubak Bidpaa <chubakbidpaa@riseup.net>",
        license = "MIT",
    },

    uses = {
        elkg.patterns.cstring,
        elkg.rewrite.stdlib,
        elkg.disamb.predicate,
        elkg.disamb.semantic,
        elkg.disamb.associativity,
        elkg.disamb.precedence,
        elkg.disamb.probability,
        elkg.disamb.dynamic_programming,
    },

    lexer = {
        main = elkg.lexer.PCRE2,
        fallback = elkg.lexer.SCANNERLESS,
    },

    encoding = elkg.encoding.UTF8,

    revhist = {
        "05-Oct-2026: First revision",
    },
}

---------------------------------------------------------------------
-- Lexemes
---------------------------------------------------------------------
JGRM.Lexeme:Define
{
    FN       = elkg.lexeme.keyword("fn"),
    LET      = elkg.lexeme.keyword("let"),
    TYPE     = elkg.lexeme.keyword("type"),
    IF       = elkg.lexeme.keyword("if"),
    THEN     = elkg.lexeme.keyword("then"),
    ELSE     = elkg.lexeme.keyword("else"),
    MATCH    = elkg.lexeme.keyword("match"),
    WITH     = elkg.lexeme.keyword("with"),
    INFIX    = elkg.lexeme.keyword("infix"),
    INFIXL   = elkg.lexeme.keyword("infixl"),
    INFIXR   = elkg.lexeme.keyword("infixr"),
    TRUE     = elkg.lexeme.keyword("true"),
    FALSE    = elkg.lexeme.keyword("false"),
    WILDCARD = elkg.lexeme.keyword("_"),

    IDENT       = elkg.lexeme.pattern("[a-z_][A-Za-z0-9_']*"),
    UPPER_IDENT = elkg.lexeme.pattern("[A-Z][A-Za-z0-9_']*"),
    FLOAT       = elkg.lexeme.pattern("[0-9]+\\.[0-9]+(?:[eE][+-]?[0-9]+)?"),
    INT         = elkg.lexeme.pattern("[0-9]+"),
    STRING      = elkg.patterns.cstring,

    -- Reserved symbols (literals beat OPERATOR on equal-length matches)
    ARROW     = elkg.lexeme.literal("->"),
    FATARROW  = elkg.lexeme.literal("=>"),
    ASSIGN    = elkg.lexeme.literal("="),
    PIPE      = elkg.lexeme.literal("|"),
    COLON     = elkg.lexeme.literal(":"),
    DOT       = elkg.lexeme.literal("."),
    BACKSLASH = elkg.lexeme.literal("\\"),
    LPAREN    = elkg.lexeme.literal("("),
    RPAREN    = elkg.lexeme.literal(")"),
    LBRACE    = elkg.lexeme.literal("{"),
    RBRACE    = elkg.lexeme.literal("}"),
    COMMA     = elkg.lexeme.literal(","),
    SEMI      = elkg.lexeme.literal(";"),
    STAR      = elkg.lexeme.literal("*"),

    -- '<' and '>' are lexed singly: they open/close type arguments AND
    -- act as comparison operators. That overlap is the generics ambiguity.
    LT = elkg.lexeme.literal("<"),
    GT = elkg.lexeme.literal(">"),

    -- Any other run of symbol characters (user-definable operators)
    OPERATOR = elkg.lexeme.pattern("[!$%&*+/<=>?@^|~:.\\-]+"),
}

JGRM.Lexeme:Skip
{
    WS      = elkg.lexeme.pattern("[ \\t\\r\\n]+"),
    COMMENT = elkg.lexeme.pattern("#[^\\n]*"),
}

---------------------------------------------------------------------
-- Rules
--
-- EBNF helpers X*  X+  X?  sep_by(X,S)  sep_by1(X,S) are legal here;
-- they are desugared by the `expand-repetition` rewriter.
--
-- Every alternative that disambiguation must refer to carries a
-- `label`. Labels survive rewriting; positional indexes would not.
---------------------------------------------------------------------
JGRM.Rule:Define
{
    Program = { { "Decl*" } },

    Decl = {
        { "FixityDecl" },
        { "FnDecl"     },
        { "TypeDecl"   },
        { "LetDecl"    },
    },

    ----------------------------------------------------------------
    -- Declarations
    ----------------------------------------------------------------
    FixityDecl = {
        { "Fixity", "INT", "sep_by1(Op, COMMA)", "SEMI", label = "fixity" },
    },
    Fixity = { { "INFIX" }, { "INFIXL" }, { "INFIXR" } },

    FnDecl = {
        { "FN", "IDENT", "TypeParams?", "LPAREN", "sep_by(Param, COMMA)", "RPAREN",
          "RetType?", "FnBody", label = "fn" },
    },
    RetType = { { "ARROW", "Type" } },
    FnBody = {
        { "ASSIGN", "Expr", "SEMI", label = "fn_body_expr"  },
        { "Block",                  label = "fn_body_block" },
    },

    TypeParams = { { "LT", "sep_by1(TypeParam, COMMA)", "GT" } },
    TypeParam  = { { "IDENT" } },

    Param = {                                   -- left-factoring target
        { "IDENT", "COLON", "Type" },
        { "IDENT" },
    },

    TypeDecl = {
        { "TYPE", "UPPER_IDENT", "TypeParams?", "ASSIGN",
          "sep_by1(Variant, PIPE)", "SEMI" },
    },
    Variant = {                                 -- left-factoring target
        { "UPPER_IDENT" },
        { "UPPER_IDENT", "LPAREN", "sep_by1(Type, COMMA)", "RPAREN" },
    },

    LetDecl = {
        { "LET", "Pattern", "TypeAnnot?", "ASSIGN", "Expr", "SEMI", label = "let" },
    },
    TypeAnnot = { { "COLON", "Type" } },

    ----------------------------------------------------------------
    -- Types: statically ambiguous, removed by `eliminate-ambiguity`
    ----------------------------------------------------------------
    Type = {
        { "Type", "ARROW", "Type", label = "fn_type"    },
        { "Type", "STAR",  "Type", label = "tuple_type" },
        { "TypeAtom" },
    },
    TypeAtom = {
        { "UPPER_IDENT", "TypeArgs?" },
        { "IDENT" },
        { "LPAREN", "Type", "RPAREN" },
    },
    TypeArgs = { { "LT", "sep_by1(Type, COMMA)", "GT" } },

    ----------------------------------------------------------------
    -- Patterns
    ----------------------------------------------------------------
    Pattern = {
        { "WILDCARD" },
        { "IDENT" },
        { "Literal" },
        { "UPPER_IDENT" },
        { "UPPER_IDENT", "LPAREN", "sep_by1(Pattern, COMMA)", "RPAREN" },
        { "LPAREN", "sep_by(Pattern, COMMA)", "RPAREN" },
    },

    ----------------------------------------------------------------
    -- Expressions: dynamically ambiguous, resolved by the pipeline
    ----------------------------------------------------------------
    Expr = {
        { "Expr", "Op", "Expr",                                  label = "binary" },
        { "PrefixOp", "Expr",                                    label = "prefix" },
        { "Expr", "Expr",                                        label = "apply"  },
        { "Expr", "LT", "sep_by1(Type, COMMA)", "GT",
          "LPAREN", "sep_by(Expr, COMMA)", "RPAREN",             label = "generic_call" },
        { "Expr", "DOT", "IDENT",                                label = "field"  },
        { "BACKSLASH", "sep_by1(Param, COMMA)", "ARROW", "Expr", label = "lambda" },
        { "IF", "Expr", "THEN", "Expr", "ELSE", "Expr",          label = "if_else" },
        { "IF", "Expr", "THEN", "Expr",                          label = "if_then" },
        { "MATCH", "Expr", "WITH", "sep_by1(Arm, COMMA)",        label = "match"  },
        { "Block",                                               label = "block"  },
        { "Record",                                              label = "record" },
        { "Atom",                                                label = "atom"   },
    },

    Op       = { { "OPERATOR" }, { "LT" }, { "GT" } },
    PrefixOp = { { "OPERATOR" } },   -- restricted to - ! ~ by a predicate

    Arm = { { "Pattern", "FATARROW", "Expr" } },

    -- `{}` and `{ x }` are both a valid Block and a valid Record.
    Block = {
        { "LBRACE", "Stmt*", "Expr?", "RBRACE", label = "block_body" },
    },
    Stmt = {
        { "LetDecl" },
        { "Expr", "SEMI", label = "expr_stmt" },
        { "SEMI" },
    },
    Record = {
        { "LBRACE", "sep_by(FieldInit, COMMA)", "RBRACE", label = "record_body" },
    },
    FieldInit = {
        { "IDENT", "ASSIGN", "Expr", label = "field_init" },
        { "IDENT",                   label = "pun"        },
    },

    Atom = {
        { "IDENT" },
        { "UPPER_IDENT" },
        { "Literal" },
        { "Paren" },
    },
    Literal = {
        { "INT" }, { "FLOAT" }, { "STRING" }, { "TRUE" }, { "FALSE" },
    },
    Paren = {                                   -- left-factoring target
        { "LPAREN", "RPAREN" },
        { "LPAREN", "Expr", "RPAREN" },
        { "LPAREN", "Expr", "COMMA", "sep_by1(Expr, COMMA)", "RPAREN" },
        { "LPAREN", "Op", "RPAREN" },           -- operator section: (+)
    },
}

---------------------------------------------------------------------
-- Rewriters
---------------------------------------------------------------------
JGRM.Rewrite:Path "rewrite/"                   -- where the stock .grl files live

-- (1) Defined in Lua: a composition of primitives.
--     Equivalent .grl would be:
--       (rewrite (name "kestrel-desugar") (rules (expand-repetition)))
JGRM.Rewrite:Define
{
    name  = "kestrel-desugar",
    rules = { "expand-repetition" },
}

-- (2) Defined in .grl, loaded from disk.
JGRM.Rewrite:Load "kestrel-normalize.grl"

-- (3) Stock rewriter. NOTE: your file is spelled `eiminate-ambguity.grl`;
--     I assumed its (name ...) is the corrected spelling. Adjust if not.
JGRM.Rewrite:Apply
{
    -- Order is significant; each step sees the previous step's output.
    { "kestrel-desugar" },

    -- Expr is excluded: its alternatives carry dynamic disambiguation
    -- annotations, and factoring/unit-removal would fuse the very
    -- derivations the pipeline needs to compare.
    { "kestrel-normalize", except = { "Expr" } },

    -- Types have fixed precedence, so remove the ambiguity from the
    -- grammar itself instead of filtering at parse time.
    --   Type  -> Type1 ARROW Type | Type1
    --   Type1 -> Type1 STAR TypeAtom | TypeAtom
    { "eliminate-ambiguity", scope = { "Type" },
      levels = {
          { label = "fn_type",    prec = 1, assoc = "right" },
          { label = "tuple_type", prec = 2, assoc = "left"  },
      } },

    -- Map parse trees back to the grammar as written above, so actions
    -- and disambiguators see `list`/`opt` nodes and original labels.
    preserve_tree = true,
}

-- (4) Only meaningful if the SCANNERLESS fallback is top-down:
-- JGRM.Rewrite:Apply
-- {
--     target = elkg.lexer.SCANNERLESS,
--     { "remove-left-recursion" },
--     { "left-factor" },
-- }
-- make-lr-compat is not used: GLR accepts any CFG.

---------------------------------------------------------------------
-- Semantic state: fixity table and symbol table
-- (kept per GLR branch; the engine forks it along with the stack)
---------------------------------------------------------------------
local ASSOC = { infix = "none", infixl = "left", infixr = "right" }

local PRELUDE_FIXITY = {
    ["*"]  = { prec = 7, assoc = "left"  }, ["/"]  = { prec = 7, assoc = "left"  },
    ["+"]  = { prec = 6, assoc = "left"  }, ["-"]  = { prec = 6, assoc = "left"  },
    ["=="] = { prec = 4, assoc = "none"  }, ["!="] = { prec = 4, assoc = "none"  },
    ["<"]  = { prec = 4, assoc = "none"  }, [">"]  = { prec = 4, assoc = "none"  },
    ["<="] = { prec = 4, assoc = "none"  }, [">="] = { prec = 4, assoc = "none"  },
    ["&&"] = { prec = 3, assoc = "right" }, ["||"] = { prec = 2, assoc = "right" },
}
local DEFAULT_FIXITY = { prec = 9, assoc = "left" }   -- as in Haskell

JGRM.Action:Define
{
    __init = function(ctx)
        ctx.fixity  = elkg.persistent.copy(PRELUDE_FIXITY)
        ctx.default_fixity = DEFAULT_FIXITY
        ctx.symtab  = elkg.symtab.new()
    end,

    fixity = function(ctx, n)
        local assoc = ASSOC[n.Fixity.token.text]
        for _, op in ipairs(n.ops) do
            ctx.fixity[op.token.text] = { prec = tonumber(n.INT.text), assoc = assoc }
        end
    end,

    fn = function(ctx, n)
        ctx.symtab:define(n.IDENT.text, {
            kind     = "fn",
            generic  = n.TypeParams ~= nil,
            tparams  = n.TypeParams and #n.TypeParams.list or 0,
        })
    end,
}

---------------------------------------------------------------------
-- Disambiguation pipeline
--
-- Stages run in order; each removes derivations from the packed forest.
-- Stages 1-3 are cheap, local, and run at reduction time.
-- Stage 4 scores whatever is left and runs once over the finished forest.
---------------------------------------------------------------------
local PREFIX_OPS = { ["-"] = true, ["!"] = true, ["~"] = true }

-- One shared level table: precedence reads `prec`, associativity reads `assoc`.
local LEVELS = {
    lambda       = { prec =  0, assoc = "right" },
    if_then      = { prec =  0, assoc = "right" },
    if_else      = { prec =  0, assoc = "right" },
    match        = { prec =  0, assoc = "right" },
    prefix       = { prec = 10, assoc = "right" },
    apply        = { prec = 11, assoc = "left"  },
    generic_call = { prec = 12, assoc = "left"  },
    field        = { prec = 12, assoc = "left"  },
    -- binary: looked up in ctx.fixity at parse time (user-declared)
    binary = elkg.disamb.dynamic_level(function(ctx, n)
        local f = ctx.fixity[n.Op.token.text] or ctx.default_fixity
        return f.prec, f.assoc
    end),
}

-- Where is this Expr, relative to its parent? ("apply.2", "let.5", ...)
local function context_of(n)
    local p = n.parent
    if not p then return "top" end
    return (p.label or p.rule) .. "." .. n.slot
end

local MODEL = elkg.disamb.probability.model
{
    -- Placeholder priors; train them with
    --   elkg.disamb.probability.train(corpus_dir)
    -- Keys are "<label>|<context>", in ORIGINAL-tree terms.
    table = {
        ["block|expr_stmt.1"]      = 0.93, ["record|expr_stmt.1"]      = 0.07,
        ["block|apply.2"]          = 0.25, ["record|apply.2"]          = 0.75,
        ["block|fn_body_expr.2"]   = 0.65, ["record|fn_body_expr.2"]   = 0.35,
        ["block|let.5"]            = 0.20, ["record|let.5"]            = 0.80,
    },
    default = 0.5,
    key = function(n) return n.label .. "|" .. context_of(n) end,
}

-- Structural penalties for the Viterbi pass (lower is better).
local PENALTY = { apply = 2, binary = 1, prefix = 1, generic_call = 0 }

JGRM.Disamb:Pipeline
{
    --------------------------------------------------------------
    -- Stage 1: predicate.c  -- lexical, purely local
    --------------------------------------------------------------
    elkg.disamb.predicate.stage
    {
        -- Only - ! ~ may be prefix operators.
        { rule = "PrefixOp",
          accept_if = function(n) return PREFIX_OPS[n.token.text] end },

        -- Whitespace rule: `f -x` applies f to (-x); `f - x` and `f-x`
        -- are subtraction.
        { rule = "Expr", label = "apply",
          reject_if = function(n)
              local arg = n[2]
              if arg.label ~= "prefix" then return false end
              local op = arg.PrefixOp.token
              return not (op.ws_before and not op.ws_after)
          end },
        { rule = "Expr", label = "binary",
          reject_if = function(n)
              local op = n.Op.token
              return PREFIX_OPS[op.text] and op.ws_before and not op.ws_after
          end },

        -- Generic calls are written `f<T>(x)`: no space before '<' or
        -- between '>' and '('.
        { rule = "Expr", label = "generic_call",
          reject_if = function(n)
              return n.LT.token.ws_before or n.GT.token.ws_after
          end },
    },

    --------------------------------------------------------------
    -- Stage 2: semantic.c  -- consults the symbol table
    --------------------------------------------------------------
    elkg.disamb.semantic.stage
    {
        -- `f<a>(b)` is a generic call only if f is a generic function
        -- that accepts that many type arguments.
        { rule = "Expr", label = "generic_call",
          accept_if = function(ctx, n)
              local callee = n[1]
              local name = callee.label == "atom" and callee:ident()
              local sym  = name and ctx.symtab:lookup(name)
              return sym ~= nil and sym.kind == "fn" and sym.generic
                     and #n.targs <= sym.tparams
          end },
    },

    --------------------------------------------------------------
    -- Stage 3: precedence.c + associativity.c
    --------------------------------------------------------------
    elkg.disamb.precedence.stage
    {
        rule = "Expr",
        levels = LEVELS,
        -- These extend as far right as possible (nearest-if, greedy match).
        open_ended = { "lambda", "if_then", "if_else", "match" },
    },
    elkg.disamb.associativity.stage
    {
        rule = "Expr",
        levels = LEVELS,
        none = "reject",        -- `a == b == c` is a syntax error
    },

    --------------------------------------------------------------
    -- Stage 4: probability.c + dynamic_programming.c
    --   probability supplies the model; DP finds the cheapest tree in
    --   the packed forest in O(|forest|) without enumerating trees.
    --------------------------------------------------------------
    elkg.disamb.dynamic.stage
    {
        forest   = "sppf",
        semiring = "tropical",                -- cost = -log P + penalty
        cost = function(n)
            local c = PENALTY[n.label] or 0
            if n.label == "block" or n.label == "record" then
                c = c - math.log(MODEL:p(n))
            end
            return c
        end,
    },

    -- Anything still ambiguous after stage 4 is a grammar bug: report it.
    elkg.disamb.finalize { on_remaining = "error", report = "diagnostic" },
}

JGRM:Start "Program"

return JGRM
