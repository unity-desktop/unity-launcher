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

#include <libdex.h>

#define RESULTS_PER_PROVIDER 5

struct _UnitySearch
{
  GObject  parent_instance;

  GList   *providers;
  guint    generation;
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

typedef struct
{
  UnitySearchProvider *provider;
  guint                generation;
} ReplyCtx;

static void
reply_ctx_free (gpointer data)
{
  ReplyCtx *ctx = data;
  g_object_unref (ctx->provider);
  g_free (ctx);
}

static DexFuture *
deliver_reply (DexFuture *future, gpointer user_data)
{
  ReplyCtx    *ctx  = user_data;
  UnitySearch *self = unity_search_get_default ();

  if (ctx->generation != self->generation)
    return NULL;

  const GValue *value = dex_future_get_value (future, NULL);
  if (value == NULL)
    return NULL;

  GPtrArray *results = g_value_get_boxed (value);
  if (results == NULL || results->len == 0)
    return NULL;

  g_signal_emit (self, signals[SIG_PROVIDER_RESULTS], 0, ctx->provider, results);
  return NULL;
}

static void
dispatch_query (UnitySearch *self, UnitySearchProvider *provider, const gchar *const *terms)
{
  ReplyCtx *ctx  = g_new0 (ReplyCtx, 1);
  ctx->provider  = g_object_ref (provider);
  ctx->generation = self->generation;

  dex_future_disown (dex_future_finally (
    unity_search_provider_query (provider, terms, RESULTS_PER_PROVIDER),
    deliver_reply, ctx, reply_ctx_free));
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

  self->generation++;

  g_auto (GStrv) terms = query ? split_terms (query) : NULL;
  if (terms == NULL)
    {
      for (GList *l = self->providers; l != NULL; l = l->next)
        unity_search_provider_reset (l->data);
      return;
    }

  for (GList *l = self->providers; l != NULL; l = l->next)
    dispatch_query (self, l->data, (const gchar *const *) terms);
}

static void
unity_search_dispose (GObject *object)
{
  UnitySearch *self = UNITY_SEARCH (object);
  self->generation++;
  g_list_free_full (g_steal_pointer (&self->providers), g_object_unref);
  G_OBJECT_CLASS (unity_search_parent_class)->dispose (object);
}

static void
unity_search_class_init (UnitySearchClass *klass)
{
  G_OBJECT_CLASS (klass)->dispose = unity_search_dispose;

  signals[SIG_PROVIDER_RESULTS] = g_signal_new (
    "provider-results", G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST, 0, NULL, NULL, NULL,
    G_TYPE_NONE, 2, UNITY_TYPE_SEARCH_PROVIDER, G_TYPE_PTR_ARRAY);
}

static void
unity_search_init (UnitySearch *self)
{
  dex_init ();
  self->providers = unity_search_provider_discover ();
}
