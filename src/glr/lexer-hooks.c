#include <glr/lexer-hooks.h>

#include <stdlib.h>
#include <string.h>

#include "../../third_party/unicode_names/unicode_names.h"

typedef struct glr_lexer_hook_entry
{
  char *name;
  int priority;
  glr_lexer_hook_fn fn;
  void *user_data;
  glr_lexer_hook_destroy_fn destroy_fn;
  struct glr_lexer_hook_entry *next;
} glr_lexer_hook_entry_t;

struct glr_lexer_hooks
{
  glr_lexer_hook_entry_t *head;
  void *user_data;
};

static void
free_entry (glr_lexer_hook_entry_t *entry)
{
  if (entry == NULL)
    {
      return;
    }

  if (entry->destroy_fn != NULL)
    {
      entry->destroy_fn (entry->user_data);
    }
  free (entry->name);
  free (entry);
}

glr_lexer_hooks_t *
glr_lexer_hooks_create (void)
{
  return calloc (1, sizeof (glr_lexer_hooks_t));
}

void
glr_lexer_hooks_clear (glr_lexer_hooks_t *hooks)
{
  glr_lexer_hook_entry_t *entry;
  glr_lexer_hook_entry_t *next;

  if (hooks == NULL)
    {
      return;
    }

  entry = hooks->head;
  while (entry != NULL)
    {
      next = entry->next;
      free_entry (entry);
      entry = next;
    }
  hooks->head = NULL;
}

void
glr_lexer_hooks_destroy (glr_lexer_hooks_t *hooks)
{
  if (hooks == NULL)
    {
      return;
    }

  glr_lexer_hooks_clear (hooks);
  free (hooks);
}

int
glr_lexer_hooks_add (glr_lexer_hooks_t *hooks, const char *name, int priority,
                     glr_lexer_hook_fn fn, void *hook_user_data,
                     glr_lexer_hook_destroy_fn destroy_fn)
{
  glr_lexer_hook_entry_t *entry;
  glr_lexer_hook_entry_t **slot;

  if (hooks == NULL || fn == NULL)
    {
      return -1;
    }

  entry = calloc (1, sizeof (*entry));
  if (entry == NULL)
    {
      return -1;
    }

  if (name != NULL)
    {
      entry->name = strdup (name);
      if (entry->name == NULL)
        {
          free (entry);
          return -1;
        }
    }

  entry->priority = priority;
  entry->fn = fn;
  entry->user_data = hook_user_data;
  entry->destroy_fn = destroy_fn;

  slot = &hooks->head;
  while (*slot != NULL && (*slot)->priority >= priority)
    {
      slot = &(*slot)->next;
    }

  entry->next = *slot;
  *slot = entry;
  return 0;
}

void
glr_lexer_hooks_set_user_data (glr_lexer_hooks_t *hooks, void *user_data)
{
  if (hooks != NULL)
    {
      hooks->user_data = user_data;
    }
}

void *
glr_lexer_hooks_get_user_data (glr_lexer_hooks_t *hooks)
{
  return hooks != NULL ? hooks->user_data : NULL;
}

const char *
glr_lexer_unicode_name (uint32_t codepoint)
{
  size_t low = 0;
  size_t high = sizeof (unicode_name_table) / sizeof (unicode_name_table[0]);

  while (low < high)
    {
      size_t mid = low + (high - low) / 2;
      if (unicode_name_table[mid].cp == codepoint)
        {
          return unicode_name_table[mid].name;
        }
      if (unicode_name_table[mid].cp < codepoint)
        {
          low = mid + 1;
        }
      else
        {
          high = mid;
        }
    }

  return NULL;
}

bool
glr_lexer_hooks_dispatch (glr_lexer_hooks_t *hooks,
                          const glr_lexer_event_t *event,
                          glr_lexer_response_t *response)
{
  glr_lexer_hook_entry_t *entry;

  if (response == NULL)
    {
      return false;
    }

  glr_lexer_response_reset (response);
  if (hooks == NULL || event == NULL)
    {
      return false;
    }

  for (entry = hooks->head; entry != NULL; entry = entry->next)
    {
      glr_lexer_response_reset (response);
      if (entry->fn (event, response, entry->user_data) && response->accepted
          && response->terminal_name != NULL
          && response->bytes_consumed >= event->default_bytes_consumed)
        {
          return true;
        }
    }

  glr_lexer_response_reset (response);
  return false;
}
