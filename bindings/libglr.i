%module libglr

%{
#include <glr/glr.h>
%}

%include <stdint.i>
%include <stddef.i>
%include <cstring.i>

/* Export the public C API as a thin wrapper surface. */
%include "glr/grammar.h"
%include "glr/forest.h"
%include "glr/fork.h"
%include "glr/graph.h"
%include "glr/reduction.h"
%include "glr/disambiguate.h"
%include "glr/rewrite.h"
%include "glr/parser.h"
%include "glr/glr.h"
