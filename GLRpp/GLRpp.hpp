#ifndef LIBGLR_GLRPP_GLRPP_HPP
#define LIBGLR_GLRPP_GLRPP_HPP

#include <climits>
#include <functional>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include "DSLUtils.hpp"
#include "Polyfills.hpp"
#include "rewrite/GrammarIR.hpp"
#include "rewrite/RewritePipeline.hpp"

#include <glr/disambiguate.h>
#include <glr/forest.h>
#include <glr/grammar.h>
#include <glr/parser.h>

namespace glrpp
{

namespace detail
{
template <typename T, auto DeleterFunc> class ResourceHandle
{
public:
  explicit ResourceHandle (T handle = nullptr) noexcept : handle_ (handle) {}
  ~ResourceHandle () { reset (); }
  ResourceHandle (ResourceHandle &&other) noexcept
      : handle_ (std::exchange (other.handle_, nullptr))
  {
  }
  ResourceHandle &
  operator= (ResourceHandle &&other) noexcept
  {
    if (this != &other)
      {
        reset ();
        handle_ = std::exchange (other.handle_, nullptr);
      }
    return *this;
  }
  ResourceHandle (const ResourceHandle &) = delete;
  ResourceHandle &operator= (const ResourceHandle &) = delete;

  T
  get () const noexcept
  {
    return handle_;
  }
  explicit
  operator bool () const noexcept
  {
    return handle_ != nullptr;
  }
  void
  reset (T handle = nullptr) noexcept
  {
    if (handle_ != nullptr)
      {
        DeleterFunc (handle_);
      }
    handle_ = handle;
  }
  T
  release () noexcept
  {
    return std::exchange (handle_, nullptr);
  }

private:
  T handle_;
};
} // namespace detail

class Symbol
{
public:
  Symbol () = default;
  Symbol (int id, std::string_view name, bool is_terminal)
      : id_ (id), name_ (name), is_terminal_ (is_terminal)
  {
  }
  int
  id () const noexcept
  {
    return id_;
  }
  std::string_view
  name () const noexcept
  {
    return name_;
  }
  bool
  is_terminal () const noexcept
  {
    return is_terminal_;
  }

private:
  int id_ = -1;
  std::string name_;
  bool is_terminal_ = false;
};

class Production
{
public:
  Production () = default;
  explicit Production (int id) : id_ (id) {}
  int
  id () const noexcept
  {
    return id_;
  }

private:
  int id_ = -1;
};

class DisambiguationContext
{
public:
  explicit DisambiguationContext (glr_disambig_context_t *ctx) : ctx_ (ctx) {}
  size_t
  candidate_count () const noexcept
  {
    return ctx_->candidate_count;
  }
  const glr_disambig_candidate_t &
  candidate (size_t i) const
  {
    return ctx_->candidates[i];
  }
  void
  reject (size_t i)
  {
    glr_disambig_context_reject_candidate (ctx_, i);
  }
  void
  set_score (size_t i, double score)
  {
    ctx_->candidates[i].score = score;
  }
  int
  lookahead_symbol () const noexcept
  {
    return ctx_->lookahead_symbol_id;
  }

private:
  glr_disambig_context_t *ctx_;
};

class DisambiguationHook
{
public:
  DisambiguationHook () = default;
  DisambiguationHook (std::string_view name, int priority, glr_disambig_fn fn,
                      void *user_data = nullptr,
                      glr_disambig_destroy_fn destroy = nullptr)
      : handle_ (glr_disambig_hook_create (std::string (name).c_str (),
                                           priority, fn, user_data, destroy))
  {
    if (!handle_)
      {
        throw std::runtime_error ("glr_disambig_hook_create failed");
      }
  }

  glr_disambig_hook_t *
  release () noexcept
  {
    return handle_.release ();
  }

private:
  friend class Parser;
  using HookHandle = detail::ResourceHandle<glr_disambig_hook_t *,
                                            &glr_disambig_hook_destroy>;
  HookHandle handle_;
};

class ParseTreeNode : public dsl::ASTNode
{
public:
  explicit ParseTreeNode (glr_forest_node_t *node = nullptr) : node_ (node) {}
  int
  symbol_id () const noexcept
  {
    return node_ ? node_->symbol_id : -1;
  }
  std::vector<ParseTreeNode>
  children () const
  {
    std::vector<ParseTreeNode> out;
    if (!node_)
      {
        return out;
      }
    out.reserve (node_->child_count);
    for (size_t i = 0; i < node_->child_count; ++i)
      {
        out.emplace_back (node_->children[i]);
      }
    return out;
  }

private:
  glr_forest_node_t *node_ = nullptr;
};

class ParseTree
{
public:
  explicit ParseTree (glr_forest_t *forest = nullptr) : forest_ (forest) {}
  bool
  is_ambiguous () const
  {
    if (!forest_)
      {
        return false;
      }
    for (size_t i = 0; i < forest_->node_count; ++i)
      {
        for (auto *n = forest_->nodes[i]; n != nullptr; n = n->next)
          {
            if (n->child_count > 1)
              {
                return true;
              }
          }
      }
    return false;
  }
  size_t
  num_parses () const noexcept
  {
    return is_ambiguous () ? 2u : 1u;
  }
  ParseTreeNode
  root () const
  {
    if (!forest_ || forest_->node_count == 0 || !forest_->nodes[0])
      {
        return ParseTreeNode{};
      }
    return ParseTreeNode{ forest_->nodes[0] };
  }

private:
  glr_forest_t *forest_ = nullptr;
};

class Grammar
{
public:
  using Status = dsl::Result<bool, std::string>;

  Grammar () : handle_ (glr_grammar_create ())
  {
    if (!handle_)
      {
        throw std::runtime_error ("glr_grammar_create failed");
      }
  }

  Symbol
  terminal (std::string_view name)
  {
    return add_symbol (name, GLR_SYMBOL_TERMINAL);
  }
  Symbol
  nonterminal (std::string_view name)
  {
    return add_symbol (name, GLR_SYMBOL_NONTERMINAL);
  }
  Production
  add_production (Symbol lhs, const std::vector<Symbol> &rhs)
  {
    auto result = try_add_production (lhs, rhs, std::nullopt, std::nullopt);
    if (result.is_err ())
      {
        throw std::runtime_error ("glr_grammar_add_production failed");
      }
    return result.unwrap ();
  }
  dsl::Result<Production, std::string>
  try_add_production (Symbol lhs, const std::vector<Symbol> &rhs)
  {
    return try_add_production (lhs, rhs, std::nullopt, std::nullopt);
  }
  dsl::Result<Production, std::string>
  try_add_production (Symbol lhs, const std::vector<Symbol> &rhs,
                      std::optional<int> precedence,
                      std::optional<glr_disambig_associativity_t> assoc)
  {
    std::vector<glr_symbol_t *> body;
    std::vector<int> rhs_ids;
    body.reserve (rhs.size ());
    rhs_ids.reserve (rhs.size ());
    for (const auto &s : rhs)
      {
        auto *sym = glr_grammar_get_symbol (handle_.get (), s.id ());
        if (!sym)
          {
            return dsl::Result<Production, std::string>::from_err (
                "invalid rhs symbol id");
          }
        body.push_back (sym);
        rhs_ids.push_back (s.id ());
      }
    int id = glr_grammar_add_production (handle_.get (), lhs.id (),
                                         body.data (), body.size ());
    if (id < 0)
      {
        return dsl::Result<Production, std::string>::from_err (
            "glr_grammar_add_production failed");
      }
    rewrite_ir_.productions.push_back (
        { id, lhs.id (), std::move (rhs_ids), precedence, assoc, {}, {} });
    if (id >= rewrite_ir_.next_production_id)
      {
        rewrite_ir_.next_production_id = id + 1;
      }
    return dsl::Result<Production, std::string>::from_ok (Production (id));
  }
  void
  set_start (Symbol start)
  {
    auto st = try_set_start (start);
    if (st.is_err ())
      {
        throw std::runtime_error ("glr_grammar_set_start_symbol failed");
      }
  }
  Status
  try_set_start (Symbol start)
  {
    if (glr_grammar_set_start_symbol (handle_.get (), start.id ()) != 0)
      {
        return Status::from_err ("glr_grammar_set_start_symbol failed");
      }
    rewrite_ir_.start_symbol_id = start.id ();
    return Status::from_ok (true);
  }
  rewrite::GrammarIR
  to_ir () const
  {
    return rewrite_ir_;
  }
  static Grammar
  from_ir (const rewrite::GrammarIR &ir)
  {
    std::string validation_error;
    if (!ir.validate (&validation_error))
      {
        throw std::runtime_error ("invalid GrammarIR: " + validation_error);
      }

    Grammar out;
    std::unordered_map<int, Symbol> id_map;
    for (const auto &symbol : ir.symbols)
      {
        Symbol added = symbol.is_terminal ? out.terminal (symbol.name)
                                          : out.nonterminal (symbol.name);
        id_map.emplace (symbol.id, added);
      }

    for (const auto &production : ir.productions)
      {
        std::vector<Symbol> rhs_symbols;
        rhs_symbols.reserve (production.rhs.size ());
        for (int rhs : production.rhs)
          {
            rhs_symbols.push_back (id_map.at (rhs));
          }
        out.try_add_production (id_map.at (production.lhs), rhs_symbols,
                                production.precedence,
                                production.associativity);
      }
    if (ir.start_symbol_id)
      {
        out.set_start (id_map.at (*ir.start_symbol_id));
      }
    out.rewrite_ir_ = ir;
    return out;
  }
  Grammar
  rewritten (const rewrite::RewritePipeline &pipeline) const
  {
    auto ir = to_ir ();
    pipeline.run (ir);
    return from_ir (ir);
  }
  glr_grammar_t *
  handle () const noexcept
  {
    return handle_.get ();
  }

private:
  Symbol
  add_symbol (std::string_view name, glr_symbol_type_t type)
  {
    auto key = std::string (name);
    auto it = symbol_map_.find (key);
    if (it != symbol_map_.end ())
      {
        auto *sym = glr_grammar_get_symbol (handle_.get (), it->second);
        return Symbol (it->second, key,
                       sym && sym->type == GLR_SYMBOL_TERMINAL);
      }
    int id = glr_grammar_add_symbol (handle_.get (), type, key.c_str ());
    if (id < 0)
      {
        throw std::runtime_error ("glr_grammar_add_symbol failed");
      }
    symbol_map_.emplace (key, id);
    rewrite_ir_.symbols.push_back (
        { id, key, type == GLR_SYMBOL_TERMINAL, {}, {} });
    if (id >= rewrite_ir_.next_symbol_id)
      {
        rewrite_ir_.next_symbol_id = id + 1;
      }
    return Symbol (id, key, type == GLR_SYMBOL_TERMINAL);
  }

  using GrammarHandle
      = detail::ResourceHandle<glr_grammar_t *, &glr_grammar_destroy>;
  GrammarHandle handle_;
  std::unordered_map<std::string, int> symbol_map_;
  rewrite::GrammarIR rewrite_ir_;
};

class ProductionBuilder
{
public:
  ProductionBuilder (Grammar &grammar, Symbol lhs)
      : grammar_ (grammar), lhs_ (lhs)
  {
  }
  ProductionBuilder &
  operator>> (Symbol rhs)
  {
    rhs_.push_back (rhs);
    return *this;
  }
  ProductionBuilder &
  prec (int p)
  {
    precedence_ = p;
    return *this;
  }
  ProductionBuilder &
  assoc (glr_disambig_associativity_t a)
  {
    assoc_ = a;
    return *this;
  }
  Production
  build ()
  {
    auto result = grammar_.try_add_production (lhs_, rhs_, precedence_, assoc_);
    if (result.is_err ())
      {
        throw std::runtime_error ("glr_grammar_add_production failed");
      }
    return result.unwrap ();
  }

private:
  Grammar &grammar_;
  Symbol lhs_;
  std::vector<Symbol> rhs_;
  std::optional<int> precedence_;
  std::optional<glr_disambig_associativity_t> assoc_;
};

class DisambiguationBuilder
{
public:
  using Predicate = std::function<bool (const DisambiguationContext &)>;
  using Action = std::function<glr_disambig_result_t (DisambiguationContext &,
                                                      size_t &)>;

  DisambiguationBuilder &
  when (Predicate pred)
  {
    clauses_.push_back ({ std::move (pred), {} });
    return *this;
  }
  DisambiguationBuilder &
  otherwise ()
  {
    clauses_.push_back (
        { [] (const DisambiguationContext &) { return true; }, {} });
    return *this;
  }
  DisambiguationBuilder &
  prefer (size_t index)
  {
    clauses_.back ().action = [index] (DisambiguationContext &, size_t &winner)
      {
        winner = index;
        return GLR_DISAMBIG_RESOLVED;
      };
    return *this;
  }
  DisambiguationBuilder &
  reject ()
  {
    clauses_.back ().action = [] (DisambiguationContext &ctx, size_t &)
      {
        for (size_t i = 0; i < ctx.candidate_count (); ++i)
          {
            ctx.reject (i);
          }
        return GLR_DISAMBIG_NO_MATCH;
      };
    return *this;
  }
  DisambiguationHook
  build (std::string_view name = "custom", int priority = 0)
  {
    auto *state = new std::vector<Clause> (std::move (clauses_));
    auto fn = +[] (glr_disambig_context_t *ctx, size_t *winner, void *ud)
                {
                  auto *clauses = static_cast<std::vector<Clause> *> (ud);
                  DisambiguationContext w (ctx);
                  for (const auto &cl : *clauses)
                    {
                      if (cl.predicate && cl.predicate (w))
                        {
                          return cl.action ? cl.action (w, *winner)
                                           : GLR_DISAMBIG_NO_MATCH;
                        }
                    }
                  return GLR_DISAMBIG_NO_MATCH;
                };
    auto destroy
        = +[] (void *ud) { delete static_cast<std::vector<Clause> *> (ud); };
    return DisambiguationHook (name, priority, fn, state, destroy);
  }

private:
  struct Clause
  {
    Predicate predicate;
    Action action;
  };
  std::vector<Clause> clauses_;
};

template <typename Derived>
class GrammarDSL : public dsl::DSL<Derived, dsl::PatternMatch, dsl::Pipeline,
                                   dsl::CustomLiterals, dsl::Rewrite, dsl::AST>
{
public:
  Symbol
  terminal (std::string_view name)
  {
    return static_cast<Derived *> (this)->grammar_.terminal (name);
  }
  Symbol
  nonterminal (std::string_view name)
  {
    return static_cast<Derived *> (this)->grammar_.nonterminal (name);
  }
  ProductionBuilder
  rule (Symbol lhs)
  {
    return ProductionBuilder (static_cast<Derived *> (this)->grammar_, lhs);
  }
  void
  start (Symbol s)
  {
    static_cast<Derived *> (this)->grammar_.set_start (s);
  }
};

class Parser
{
public:
  using ParseResult = dsl::Result<ParseTree, std::string>;
  using Status = dsl::Result<bool, std::string>;

  explicit Parser (Grammar &grammar)
      : grammar_ (&grammar), handle_ (glr_parser_create (grammar.handle ()))
  {
    if (!handle_)
      {
        throw std::runtime_error ("glr_parser_create failed");
      }
  }

  ParseTree
  parse (std::string_view input)
  {
    auto out = try_parse (input);
    if (out.is_err ())
      {
        throw std::runtime_error ("glr_parse failed");
      }
    return out.unwrap ();
  }

  ParseResult
  try_parse (std::string_view input)
  {
    auto result = glr_parse (handle_.get (), input.data (), input.size ());
    if (result.error != GLR_PARSE_SUCCESS)
      {
        return ParseResult::from_err ("glr_parse failed");
      }
    return ParseResult::from_ok (ParseTree (result.forest));
  }

  void
  register_disambiguation (DisambiguationHook hook)
  {
    auto st = try_register_disambiguation (std::move (hook));
    if (st.is_err ())
      {
        throw std::runtime_error ("glr_parser_add_disambiguator failed");
      }
  }

  Status
  try_register_disambiguation (DisambiguationHook hook)
  {
    auto *raw = hook.release ();
    if (glr_parser_add_disambiguator (handle_.get (), raw) != 0)
      {
        glr_disambig_hook_destroy (raw);
        return Status::from_err ("glr_parser_add_disambiguator failed");
      }
    return Status::from_ok (true);
  }

  glr_parser_t *
  handle () const noexcept
  {
    return handle_.get ();
  }

private:
  using ParserHandle
      = detail::ResourceHandle<glr_parser_t *, &glr_parser_destroy>;
  Grammar *grammar_;
  ParserHandle handle_;
};

namespace disambiguators
{
inline DisambiguationHook
by_precedence (int priority = 100)
{
  auto fn = [] (glr_disambig_context_t *ctx, size_t *winner,
                void *) -> glr_disambig_result_t
    {
      int best = INT_MIN;
      size_t idx = 0;
      bool found = false;
      for (size_t i = 0; i < ctx->candidate_count; ++i)
        {
          const auto &c = ctx->candidates[i];
          if (!c.rejected && c.precedence > best)
            {
              best = c.precedence;
              idx = i;
              found = true;
            }
        }
      if (!found)
        {
          return GLR_DISAMBIG_NO_MATCH;
        }
      *winner = idx;
      return GLR_DISAMBIG_RESOLVED;
    };
  return DisambiguationHook ("precedence", priority, fn);
}
inline DisambiguationHook
by_associativity (int priority = 90)
{
  auto fn = [] (glr_disambig_context_t *ctx, size_t *winner,
                void *) -> glr_disambig_result_t
    {
      for (size_t i = 0; i < ctx->candidate_count; ++i)
        {
          const auto &cand = ctx->candidates[i];
          if (cand.rejected)
            {
              continue;
            }
          if (cand.associativity == GLR_DISAMBIG_ASSOC_LEFT)
            {
              *winner = i;
              return GLR_DISAMBIG_RESOLVED;
            }
          if (cand.associativity == GLR_DISAMBIG_ASSOC_RIGHT)
            {
              *winner = ctx->candidate_count - 1 - i;
              return GLR_DISAMBIG_RESOLVED;
            }
        }
      return GLR_DISAMBIG_NO_MATCH;
    };
  return DisambiguationHook ("associativity", priority, fn);
}
inline DisambiguationHook
longest_match (int priority = 80)
{
  auto fn = [] (glr_disambig_context_t *ctx, size_t *winner,
                void *) -> glr_disambig_result_t
    {
      size_t max_len = 0;
      size_t best_idx = 0;
      bool found = false;
      for (size_t i = 0; i < ctx->candidate_count; ++i)
        {
          const auto &cand = ctx->candidates[i];
          if (cand.rejected)
            {
              continue;
            }
          size_t len = cand.end_position - cand.start_position;
          if (len > max_len)
            {
              max_len = len;
              best_idx = i;
              found = true;
            }
        }
      if (!found)
        {
          return GLR_DISAMBIG_NO_MATCH;
        }
      *winner = best_idx;
      return GLR_DISAMBIG_RESOLVED;
    };
  return DisambiguationHook ("longest_match", priority, fn);
}
} // namespace disambiguators

} // namespace glrpp

#endif
