/* unity-search.c
 *
 * Copyright 2026 Muqtadir
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "dash/search/unity-search.h"

#define RESULTS_PER_PROVIDER 5

struct _UnitySearch
{
  GObject       parent_instance;

  GList        *providers;
  GCancellable *cancellable;
};

G_DEFINE_FINAL_TYPE (UnitySearch, unity_search, G_TYPE_OBJECT)

enum { SIG_PROVIDER_RESULTS, N_SIGNALS };
static guint signals[N_SIGNALS];

UnitySearch *
unity_search_get_default (void)
{
  static UnitySearch *instance;
  if (g_once_init_enter_pointer (&instance))
    g_once_init_leave_pointer (&instance, g_object_new (UNITY_TYPE_SEARCH, NULL));
  return instance;
}

static void
on_provider_query_done (GObject *source, GAsyncResult *res, gpointer user_data)
{
  UnitySearch         *self     = user_data;
  UnitySearchProvider *provider = UNITY_SEARCH_PROVIDER (source);
  g_autoptr (GError)        error    = NULL;

  g_autoptr (GPtrArray) results =
    unity_search_provider_query_finish (provider, res, &error);

  if (results == NULL || results->len == 0)
    return;

  g_signal_emit (self, signals[SIG_PROVIDER_RESULTS], 0, provider, results);
}

static GStrv
split_terms (const gchar *query)
{
  g_auto (GStrv) raw = g_strsplit_set (query, " \t\n", -1);
  g_autoptr (GPtrArray) terms = g_ptr_array_new_with_free_func (g_free);
  for (gchar **p = raw; p && *p; p++)
    if (**p != '\0')
      g_ptr_array_add (terms, g_strdup (*p));
  if (terms->len == 0)
    return NULL;
  g_ptr_array_add (terms, NULL);
  return (GStrv) g_ptr_array_free (g_steal_pointer (&terms), FALSE);
}

void
unity_search_query (UnitySearch *self, const gchar *query)
{
  g_return_if_fail (UNITY_IS_SEARCH (self));

  g_cancellable_cancel (self->cancellable);
  g_clear_object (&self->cancellable);

  g_auto (GStrv) terms = query ? split_terms (query) : NULL;
  if (terms == NULL)
    {
      /* Cleared search: forget each provider's last query so the next one starts
       * fresh rather than as a subsearch of stale results. */
      for (GList *l = self->providers; l != NULL; l = l->next)
        unity_search_provider_reset (l->data);
      return;
    }

  self->cancellable = g_cancellable_new ();
  for (GList *l = self->providers; l != NULL; l = l->next)
    unity_search_provider_query_async (
      l->data, (const gchar *const *) terms,
      RESULTS_PER_PROVIDER,
      self->cancellable, on_provider_query_done, self);
}

static void
unity_search_dispose (GObject *object)
{
  UnitySearch *self = UNITY_SEARCH (object);
  g_cancellable_cancel (self->cancellable);
  g_clear_object (&self->cancellable);
  g_list_free_full (g_steal_pointer (&self->providers), g_object_unref);
  G_OBJECT_CLASS (unity_search_parent_class)->dispose (object);
}

static void
unity_search_class_init (UnitySearchClass *klass)
{
  G_OBJECT_CLASS (klass)->dispose = unity_search_dispose;

  /* provider-results (provider, results): emitted when a provider replies. */
  signals[SIG_PROVIDER_RESULTS] = g_signal_new (
    "provider-results", G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST, 0, NULL, NULL, NULL,
    G_TYPE_NONE, 2, UNITY_TYPE_SEARCH_PROVIDER, G_TYPE_PTR_ARRAY);
}

static void
unity_search_init (UnitySearch *self)
{
  self->providers = unity_search_provider_discover ();
}
