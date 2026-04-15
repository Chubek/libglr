#include <glr/forest.h>
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

void
glr_forest_destroy (glr_forest_t *forest)
{
  if (forest == NULL)
    {
      return;
    }

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
  new_node->children = NULL;
  new_node->child_count = 0;
  new_node->capacity = 0;
  new_node->next = forest->nodes[position];

  forest->nodes[position] = new_node;

  return new_node;
}

int
glr_forest_add_child (glr_forest_node_t *parent, glr_forest_node_t *child)
{
  if (parent == NULL || child == NULL || parent->type != GLR_NODE_NONTERMINAL)
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
  if (node == NULL || node->type != GLR_NODE_NONTERMINAL)
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
