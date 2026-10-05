---------------------------------------------------------------------
-- Custom rewriter primitives for Kestrel.
--
-- A primitive is a pure function: rules-table -> rules-table.
-- Once registered it can be called by name from any .grl file,
-- e.g.  (rules (expand-repetition) (left-factor))
-- or from Lua via JGRM.Rewrite:Define { rules = { "expand-repetition" } }.
--
-- `ctx:synthetic(name, shape)` tells the engine that `name` was
-- invented by a rewrite. When the parse tree is mapped back to the
-- original grammar (preserve_tree = true), synthetic "list" rules are
-- flattened into one list node and "opt" rules into present/absent,
-- so semantic actions never see the generated helpers.
---------------------------------------------------------------------

local function slug(s)
    return (s:gsub("[^%w]+", "_"):gsub("^_+", ""):gsub("_+$", ""))
end

elkg.rewrite.primitive
{
    name = "expand-repetition",
    doc  = "Desugar EBNF helpers: X*  X+  X?  sep_by(X,S)  sep_by1(X,S)",

    apply = function(rules, ctx)
        local function define(name, alts, shape)
            if rules[name] == nil then
                rules[name] = alts
                ctx:synthetic(name, shape)
            end
            return name
        end

        -- Returns the nonterminal implementing a (possibly decorated) symbol.
        local expand
        expand = function(sym)
            -- sep_by1(X, S) / sep_by(X, S)
            local kind, args = sym:match("^(sep_by1?)%((.+)%)$")
            if kind then
                local x, s = args:match("^%s*([^,]+)%s*,%s*([^,]+)%s*$")
                assert(x and s, "malformed separator list: " .. sym)
                x = expand(x:match("^%s*(.-)%s*$"))
                s = s:match("^%s*(.-)%s*$")
                local one = "sep1_" .. slug(x) .. "_" .. slug(s)
                define(one, { { x }, { one, s, x } }, "list")
                if kind == "sep_by1" then return one end
                return define("sep0_" .. slug(x) .. "_" .. slug(s),
                              { {}, { one } }, "list")
            end

            -- X*  X+  X?
            local base, op = sym:match("^([%w_]+)([%*%+%?])$")
            if not base then return sym end -- plain symbol

            if op == "*" then
                local n = base .. "_star"
                return define(n, { {}, { n, base } }, "list")
            elseif op == "+" then
                local n = base .. "_plus"
                return define(n, { { base }, { n, base } }, "list")
            else
                local n = base .. "_opt"
                return define(n, { {}, { base } }, "opt")
            end
        end

        -- Snapshot names first: we add rules while rewriting.
        local names = {}
        for name in pairs(rules) do names[#names + 1] = name end

        for _, name in ipairs(names) do
            for _, alt in ipairs(rules[name]) do
                for i, sym in ipairs(alt) do   -- ipairs: skips `label = ...`
                    alt[i] = expand(sym)
                end
            end
        end

        return rules
    end,
}
