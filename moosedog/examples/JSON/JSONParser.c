#include "Moosedog.h"
#include "JSON.ast.h"
#include "JSON.lexer.h"
#include "JSON.parser.h"

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>


int main(int argc, char **argv)
{
     MooseDogAST(JSON) json_ast;
     MoosedogLexer(JSON) json_lexer;
     MoosedogParser(JSON) json_parser;
     MoosedogListener(JSON) json_listener;
     MoosedogVisitor(JSON) json_visitor;
     MoosedogIO(JSON) json_input;
     MoosedogQuery(JSON, "PrettyPrint.lua") json_query_pprint;
     MoosedogQuery(JSON, "Linter.lua") json_query_lint;

}
