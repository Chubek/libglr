#ifndef GLR_H
#define GLR_H

/**
 * @mainpage LibGLR: Generalized LR Parser Library
 *
 * LibGLR is a C library that provides facilities for implementing
 * Generalized LR (GLR) parsers. It is part of the TwinBooks project.
 *
 * @section features Key Features
 * - Grammar data structure with symbols and productions
 * - DAG-based stack implementation for efficient state management
 * - Forking mechanism for handling ambiguity
 * - SPPF (Shared Parse Forest) data structure
 * - Disambiguation hooks and standard policies for SPPF selection
 * - Reduction operations for GLR parsing
 *
 * @section modules Library Modules
 * - @ref grammar.h - Grammar data structures
 * - @ref stack.h - DAG-based stack implementation
 * - @ref fork.h - Forking mechanism
 * - @ref forest.h - SPPF forest representation
 * - @ref reduction.h - Reduction operations
 * - @ref graph.h - Graph operations for forests
 * - @ref parser.h - Parser core infrastructure
 * - @ref disambiguation_api - Disambiguation API and standard strategies
 *
 * @note All public APIs are documented with Doxygen comments.
 *       The library uses the "3tb" manpage section (e.g., libglr.3tb).
 */

#include "forest.h"
#include "disambiguate.h"
#include "fork.h"
#include "grammar.h"
#include "graph.h"
#include "parser.h"
#include "reduction.h"
#include "stack.h"

#ifdef __cplusplus
extern "C"
{
#endif

  /**
   * @brief Get the library version string
   *
   * @return Version string in format "major.minor.patch"
   */
  const char *glr_version (void);

  /**
   * @brief Get the library name
   *
   * @return Library name string
   */
  const char *glr_name (void);

#ifdef __cplusplus
}
#endif

#endif /* GLR_H */
