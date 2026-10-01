#include <glr/forest.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

glr_forest_t *
glr_forest_create (void)
{
  glr_forest_t *forest = calloc (1, sizeof (glr_forest_t));
  if (forest == NULL)
    {
      return NULL;
    }

  forest->nodes = NULL;
  forest->node_count = 0;
  forest->edges = NULL;
  forest->edge_count = 0;

  return forest;
}

static bool
glr_forest_node_is_owned (glr_forest_node_t *const *owned, size_t owned_count,
                           glr_forest_node_t *candidate)
{
  for (size_t i = 0; i < owned_count; i++)
    {
      if (owned[i] == candidate)
        {
          return true;
        }
    }
  return false;
}

/* Free a node subtree that is not part of the forest's position table.
   Deserialization materializes child subtrees inline, so those nodes are
   owned by the container without being registered as top-level nodes. */
static void
glr_forest_free_detached_subtree (glr_forest_node_t *const *owned,
                                  size_t owned_count,
                                  glr_forest_node_t *node)
{
  if (node == NULL || glr_forest_node_is_owned (owned, owned_count, node))
    {
      return;
    }
  for (size_t c = 0; c < node->child_count; c++)
    {
      glr_forest_free_detached_subtree (owned, owned_count, node->children[c]);
    }
  free (node->children);
  free (node);
}

void
glr_forest_destroy (glr_forest_t *forest)
{
  glr_forest_node_t **owned;
  size_t owned_count;
  size_t owned_capacity;

  if (forest == NULL)
    {
      return;
    }

  /* Collect every node owned through the position table so child
     pointers can be classified as borrowed (aliasing an owned node)
     or owned subtrees (materialized by deserialization, which
     duplicates child subtrees inline). Borrowed children are never
     freed here; owned subtrees are freed exactly once. */
  owned = NULL;
  owned_count = 0;
  owned_capacity = 0;
  for (size_t pos = 0; pos < forest->node_count; pos++)
    {
      for (glr_forest_node_t *node = forest->nodes[pos]; node != NULL;
           node = node->next)
        {
          if (owned_count >= owned_capacity)
            {
              size_t new_cap = owned_capacity == 0 ? 32 : owned_capacity * 2;
              glr_forest_node_t **grown
                  = realloc (owned, new_cap * sizeof (*grown));
              if (grown == NULL)
                {
                  break;
                }
              owned = grown;
              owned_capacity = new_cap;
            }
          if (owned_count < owned_capacity)
            {
              owned[owned_count++] = node;
            }
        }
    }

  /* Free owned subtrees reachable only through child pointers. */
  for (size_t i = 0; i < owned_count; i++)
    {
      glr_forest_node_t *node = owned[i];
      for (size_t c = 0; c < node->child_count; c++)
        {
          glr_forest_node_t *child = node->children[c];
          bool known = false;

          if (child == NULL)
            {
              continue;
            }
          for (size_t k = 0; k < owned_count; k++)
            {
              if (owned[k] == child)
                {
                  known = true;
                  break;
                }
            }
          if (!known)
            {
              /* Owned duplicate: free the subtree without touching
                 nodes owned through the position table. The recursive
                 helper below only frees nodes outside `owned`. */
              glr_forest_free_detached_subtree (owned, owned_count, child);
              node->children[c] = NULL;
            }
        }
    }
  free (owned);

  /* Free all nodes */
  for (size_t pos = 0; pos < forest->node_count; pos++)
    {
      glr_forest_node_t *node = forest->nodes[pos];
      while (node != NULL)
        {
          glr_forest_node_t *next = node->next;
          free (node->children);
          free (node);
          node = next;
        }
    }
  free (forest->nodes);

  /* Free all edges */
  for (size_t pos = 0; pos < forest->edge_count; pos++)
    {
      glr_forest_edge_t *edge = forest->edges[pos];
      while (edge != NULL)
        {
          glr_forest_edge_t *next = edge->next;
          free (edge);
          edge = next;
        }
    }
  free (forest->edges);

  free (forest);
}

glr_forest_node_t *
glr_forest_get_node (glr_forest_t *forest, glr_forest_node_type_t type,
                     int symbol_id, size_t position)
{
  if (forest == NULL)
    {
      return NULL;
    }

  /* Expand nodes array if needed */
  if (position >= forest->node_count)
    {
      size_t new_count = position + 1;
      glr_forest_node_t **new_nodes
          = realloc (forest->nodes, new_count * sizeof (glr_forest_node_t *));
      if (new_nodes == NULL)
        {
          return NULL;
        }

      /* Initialize new positions to NULL */
      memset (new_nodes + forest->node_count, 0,
              (new_count - forest->node_count) * sizeof (glr_forest_node_t *));

      forest->nodes = new_nodes;
      forest->node_count = new_count;
    }

  /* Search for existing node with matching properties */
  glr_forest_node_t *node = forest->nodes[position];
  while (node != NULL)
    {
      if (node->type == type && node->symbol_id == symbol_id)
        {
          return node;
        }
      node = node->next;
    }

  /* Create new node */
  glr_forest_node_t *new_node = calloc (1, sizeof (glr_forest_node_t));
  if (new_node == NULL)
    {
      return NULL;
    }

  new_node->type = type;
  new_node->symbol_id = symbol_id;
  new_node->position = position;
  /* Spans default to an empty range at the start position; the parser
     widens them as it consumes and reduces terminals. */
  new_node->end_position = position;
  new_node->children = NULL;
  new_node->child_count = 0;
  new_node->capacity = 0;
  new_node->next = forest->nodes[position];

  forest->nodes[position] = new_node;

  return new_node;
}

void
glr_forest_node_destroy (glr_forest_node_t *node)
{
  if (node == NULL)
    {
      return;
    }
  free (node->children);
  node->children = NULL;
  node->child_count = 0;
  node->capacity = 0;
  free (node);
}

int
glr_forest_add_child (glr_forest_node_t *parent, glr_forest_node_t *child)
{
  if (parent == NULL || child == NULL || parent == child)
    {
      return -1;
    }
  /* Non-terminal and constructor nodes both pack children; terminals are
     leaves. */
  if (parent->type == GLR_NODE_TERMINAL)
    {
      return -1;
    }

  /* Expand children array if needed */
  if (parent->child_count >= parent->capacity)
    {
      size_t new_capacity = parent->capacity == 0 ? 4 : parent->capacity * 2;
      glr_forest_node_t **new_children = realloc (
          parent->children, new_capacity * sizeof (glr_forest_node_t *));
      if (new_children == NULL)
        {
          return -1;
        }

      parent->capacity = new_capacity;
      parent->children = new_children;
    }

  parent->children[parent->child_count++] = child;

  return 0;
}

glr_forest_node_t **
glr_forest_get_children (glr_forest_node_t *node)
{
  if (node == NULL || node->type == GLR_NODE_TERMINAL)
    {
      return NULL;
    }

  return node->children;
}

int
glr_forest_add_edge (glr_forest_t *forest, glr_forest_edge_t *edge)
{
  glr_forest_edge_t *stored_edge;

  if (forest == NULL || edge == NULL)
    {
      return -1;
    }

  /* Expand edges array if needed */
  if (edge->end_position >= forest->edge_count)
    {
      size_t new_count = edge->end_position + 1;
      glr_forest_edge_t **new_edges
          = realloc (forest->edges, new_count * sizeof (glr_forest_edge_t *));
      if (new_edges == NULL)
        {
          return -1;
        }

      memset (new_edges + forest->edge_count, 0,
              (new_count - forest->edge_count) * sizeof (glr_forest_edge_t *));

      forest->edges = new_edges;
      forest->edge_count = new_count;
    }

  stored_edge = calloc (1, sizeof (*stored_edge));
  if (stored_edge == NULL)
    {
      return -1;
    }

  *stored_edge = *edge;
  stored_edge->next = forest->edges[edge->end_position];
  forest->edges[edge->end_position] = stored_edge;

  return 0;
}

glr_forest_edge_t *
glr_forest_get_edges (glr_forest_t *forest, size_t position)
{
  if (forest == NULL || position >= forest->edge_count)
    {
      return NULL;
    }

  return forest->edges[position];
}

size_t
glr_forest_node_count_at (const glr_forest_t *forest, size_t position)
{
  const glr_forest_node_t *node;
  size_t count = 0;

  if (forest == NULL || position >= forest->node_count)
    {
      return 0;
    }

  for (node = forest->nodes[position]; node != NULL; node = node->next)
    {
      count++;
    }

  return count;
}

size_t
glr_forest_total_nodes (const glr_forest_t *forest)
{
  size_t total = 0;
  size_t pos;

  if (forest == NULL)
    {
      return 0;
    }

  for (pos = 0; pos < forest->node_count; pos++)
    {
      total += glr_forest_node_count_at (forest, pos);
    }

  return total;
}

typedef struct
{
  const glr_forest_node_t *old_node;
  glr_forest_node_t *new_node;
} glr_forest_clone_entry_t;

static glr_forest_node_t *
glr_forest_clone_lookup (glr_forest_clone_entry_t *map, size_t map_size,
                         const glr_forest_node_t *old_node)
{
  size_t i;

  for (i = 0; i < map_size; i++)
    {
      if (map[i].old_node == old_node)
        {
          return map[i].new_node;
        }
    }

  return NULL;
}

glr_forest_t *
glr_forest_clone (const glr_forest_t *source)
{
  glr_forest_t *copy;
  glr_forest_clone_entry_t *map = NULL;
  size_t map_size = 0;
  size_t map_capacity = 0;
  size_t pos;
  size_t i;

  if (source == NULL)
    {
      return NULL;
    }

  copy = glr_forest_create ();
  if (copy == NULL)
    {
      return NULL;
    }

  if (source->node_count > 0)
    {
      copy->nodes = calloc (source->node_count, sizeof (*copy->nodes));
      if (copy->nodes == NULL)
        {
          glr_forest_destroy (copy);
          return NULL;
        }
      copy->node_count = source->node_count;
    }

  /* First pass: duplicate every node object without children. */
  for (pos = 0; pos < source->node_count; pos++)
    {
      const glr_forest_node_t *node;
      glr_forest_node_t **slot = &copy->nodes[pos];

      for (node = source->nodes[pos]; node != NULL; node = node->next)
        {
          glr_forest_node_t *fresh = calloc (1, sizeof (*fresh));
          glr_forest_clone_entry_t *grown;

          if (fresh == NULL)
            {
              free (map);
              glr_forest_destroy (copy);
              return NULL;
            }

          fresh->type = node->type;
          fresh->symbol_id = node->symbol_id;
          fresh->position = node->position;
          fresh->end_position = node->end_position;
          fresh->child_count = node->child_count;
          fresh->capacity = node->child_count;
          fresh->data = NULL;
          fresh->next = NULL;
          fresh->children = NULL;

          if (node->child_count > 0)
            {
              fresh->children
                  = calloc (node->child_count, sizeof (*fresh->children));
              if (fresh->children == NULL)
                {
                  free (fresh);
                  free (map);
                  glr_forest_destroy (copy);
                  return NULL;
                }
            }

          if (map_size >= map_capacity)
            {
              size_t new_cap = map_capacity == 0 ? 32 : map_capacity * 2;
              grown = realloc (map, new_cap * sizeof (*grown));
              if (grown == NULL)
                {
                  free (fresh->children);
                  free (fresh);
                  free (map);
                  glr_forest_destroy (copy);
                  return NULL;
                }
              map = grown;
              map_capacity = new_cap;
            }
          map[map_size].old_node = node;
          map[map_size].new_node = fresh;
          map_size++;

          *slot = fresh;
          slot = &fresh->next;
        }
    }

  /* Second pass: remap child pointers to the duplicated nodes. When a
     child lives outside the source forest (foreign pointer), keep the
     original pointer so no information is lost. */
  for (i = 0; i < map_size; i++)
    {
      const glr_forest_node_t *old_node = map[i].old_node;
      glr_forest_node_t *new_node = map[i].new_node;

      for (size_t c = 0; c < old_node->child_count; c++)
        {
          glr_forest_node_t *remapped = glr_forest_clone_lookup (
              map, map_size, old_node->children[c]);
          new_node->children[c]
              = remapped != NULL ? remapped : old_node->children[c];
        }
    }

  free (map);

  if (source->edge_count > 0)
    {
      copy->edges = calloc (source->edge_count, sizeof (*copy->edges));
      if (copy->edges == NULL)
        {
          glr_forest_destroy (copy);
          return NULL;
        }
      copy->edge_count = source->edge_count;

      for (pos = 0; pos < source->edge_count; pos++)
        {
          const glr_forest_edge_t *edge;
          glr_forest_edge_t **slot = &copy->edges[pos];

          for (edge = source->edges[pos]; edge != NULL; edge = edge->next)
            {
              glr_forest_edge_t *fresh = calloc (1, sizeof (*fresh));
              if (fresh == NULL)
                {
                  glr_forest_destroy (copy);
                  return NULL;
                }
              *fresh = *edge;
              fresh->next = NULL;
              *slot = fresh;
              slot = &fresh->next;
            }
        }
    }

  return copy;
}

void
glr_forest_clear (glr_forest_t *forest)
{
  size_t pos;

  if (forest == NULL)
    {
      return;
    }
  for (pos = 0; pos < forest->node_count; pos++)
    {
      glr_forest_node_t *node = forest->nodes[pos];
      while (node != NULL)
        {
          glr_forest_node_t *next = node->next;
          free (node->children);
          free (node);
          node = next;
        }
      forest->nodes[pos] = NULL;
    }
  free (forest->nodes);
  forest->nodes = NULL;
  forest->node_count = 0;
  forest->root = NULL;

  for (pos = 0; pos < forest->edge_count; pos++)
    {
      glr_forest_edge_t *edge = forest->edges[pos];
      while (edge != NULL)
        {
          glr_forest_edge_t *next = edge->next;
          free (edge);
          edge = next;
        }
    }
  free (forest->edges);
  forest->edges = NULL;
  forest->edge_count = 0;
}

/* ============================================================================
 * Traversal
 * ========================================================================== */

typedef struct
{
  const glr_forest_node_t *node;
  size_t depth;
} lr_visit_frame_t;

typedef struct
{
  const glr_forest_node_t *node;
} lr_visit_entry_t;

static bool
lr_visit_seen (const lr_visit_entry_t *seen, size_t seen_count,
               const glr_forest_node_t *node)
{
  for (size_t i = 0; i < seen_count; i++)
    {
      if (seen[i].node == node)
        {
          return true;
        }
    }
  return false;
}

static int
lr_visit_push (lr_visit_frame_t **queue, size_t *count, size_t *capacity,
               const glr_forest_node_t *node, size_t depth)
{
  if (*count >= *capacity)
    {
      size_t new_cap = *capacity == 0 ? 32 : *capacity * 2;
      lr_visit_frame_t *grown = realloc (*queue, new_cap * sizeof (*grown));
      if (grown == NULL)
        {
          return -1;
        }
      *queue = grown;
      *capacity = new_cap;
    }
  (*queue)[*count].node = node;
  (*queue)[*count].depth = depth;
  (*count)++;
  return 0;
}

size_t
glr_forest_visit (const glr_forest_t *forest, const glr_forest_node_t *root,
                  glr_forest_visit_fn visit, void *user_data)
{
  lr_visit_entry_t *seen = NULL;
  size_t seen_count = 0;
  size_t seen_capacity = 0;
  lr_visit_frame_t *queue = NULL;
  size_t queue_count = 0;
  size_t queue_capacity = 0;
  size_t visited = 0;
  int rc = 0;

  if (visit == NULL)
    {
      return 0;
    }

  if (root != NULL)
    {
      rc = lr_visit_push (&queue, &queue_count, &queue_capacity, root, 0);
    }
  else if (forest != NULL)
    {
      for (size_t pos = 0; pos < forest->node_count && rc == 0; pos++)
        {
          for (const glr_forest_node_t *node = forest->nodes[pos];
               node != NULL; node = node->next)
            {
              rc = lr_visit_push (&queue, &queue_count, &queue_capacity, node,
                                  0);
              if (rc != 0)
                {
                  break;
                }
            }
        }
    }

  while (rc == 0 && queue_count > 0)
    {
      lr_visit_frame_t frame = queue[--queue_count];
      glr_forest_node_t *node = (glr_forest_node_t *) frame.node;
      size_t c;

      if (node == NULL || lr_visit_seen (seen, seen_count, node))
        {
          continue;
        }

      if (seen_count >= seen_capacity)
        {
          size_t new_cap = seen_capacity == 0 ? 32 : seen_capacity * 2;
          lr_visit_entry_t *grown
              = realloc (seen, new_cap * sizeof (*grown));
          if (grown == NULL)
            {
              rc = -1;
              break;
            }
          seen = grown;
          seen_capacity = new_cap;
        }
      seen[seen_count++].node = node;

      visit (node, frame.depth, user_data);
      visited++;

      for (c = 0; c < node->child_count; c++)
        {
          rc = lr_visit_push (&queue, &queue_count, &queue_capacity,
                              node->children[c], frame.depth + 1);
          if (rc != 0)
            {
              break;
            }
        }
    }

  free (queue);
  free (seen);
  return visited;
}

/* Does any sub-forest reachable from `node` pack two alternative
   derivations? The visited set makes the walk terminate on the cyclic graphs
   that left recursion produces. */
static bool
lr_ambig_walk (const glr_forest_node_t *node, const glr_forest_node_t **seen,
               size_t seen_count, size_t seen_capacity)
{
  if (node == NULL)
    {
      return false;
    }
  for (size_t i = 0; i < seen_count; i++)
    {
      if (seen[i] == node)
        {
          return false; /* already inspected */
        }
    }

  if (seen_count >= seen_capacity)
    {
      return false; /* visited set exhausted: stay conservative */
    }
  seen[seen_count++] = node;

  /* More than one constructor under a symbol node is two derivations of the
     same non-terminal occurrence. */
  if (node->type != GLR_NODE_CONSTRUCTOR && node->child_count > 1)
    {
      return true;
    }

  for (size_t c = 0; c < node->child_count; c++)
    {
      if (lr_ambig_walk (node->children[c], seen, seen_count, seen_capacity))
        {
          return true;
        }
    }

  return false;
}

bool
glr_forest_is_ambiguous (const glr_forest_node_t *node)
{
  const glr_forest_node_t **seen;
  size_t capacity = 64;
  bool result;

  if (node == NULL)
    {
      return false;
    }

  seen = calloc (capacity, sizeof (*seen));
  if (seen == NULL)
    {
      return false;
    }
  result = lr_ambig_walk (node, seen, 0, capacity);
  free (seen);
  return result;
}

/* Get or create a packed node keyed by (type, id, start, end). A node over a
   span that is not already packed is appended to the position's chain, so
   nodes of different spans at one position coexist without overwriting each
   other. */
static glr_forest_node_t *
lr_forest_get_packed (glr_forest_t *forest, glr_forest_node_type_t type,
                      int id, size_t start, size_t end)
{
  glr_forest_node_t *node;
  glr_forest_node_t **chain;

  if (forest == NULL)
    {
      return NULL;
    }
  if (end < start)
    {
      end = start;
    }

  /* Grow the position table so the node can be linked at `start`. */
  if (start >= forest->node_count)
    {
      size_t new_count = start + 1;
      glr_forest_node_t **grown
          = realloc (forest->nodes, new_count * sizeof (*grown));
      if (grown == NULL)
        {
          return NULL;
        }
      memset (grown + forest->node_count, 0,
              (new_count - forest->node_count) * sizeof (*grown));
      forest->nodes = grown;
      forest->node_count = new_count;
    }

  chain = &forest->nodes[start];

  for (node = *chain; node != NULL; node = node->next)
    {
      if (node->type == type && node->symbol_id == id
          && node->position == start && node->end_position == end)
        {
          return node;
        }
    }

  node = calloc (1, sizeof (*node));
  if (node == NULL)
    {
      return NULL;
    }
  node->type = type;
  node->symbol_id = id;
  node->position = start;
  node->end_position = end;
  node->next = *chain;
  *chain = node;

  return node;
}

glr_forest_node_t *
glr_forest_get_constructor (glr_forest_t *forest, int production_id,
                            size_t start, size_t end)
{
  return lr_forest_get_packed (forest, GLR_NODE_CONSTRUCTOR, production_id,
                               start, end);
}

glr_forest_node_t *
glr_forest_get_symbol (glr_forest_t *forest, int nonterminal_id, size_t start,
                       size_t end)
{
  return lr_forest_get_packed (forest, GLR_NODE_NONTERMINAL, nonterminal_id,
                               start, end);
}
