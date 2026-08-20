/* unity-search-result.c
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

#include "dash/search/unity-search-result.h"

#include "dash/search/unity-search-provider.h"

struct _UnitySearchResult
{
  GObject                   parent_instance;

  UnitySearchProvider *provider;
  gchar                    *id;
  gchar                    *name;
  gchar                    *description;
  gchar                    *clipboard_text;
  GIcon                    *gicon;
  GStrv                     terms;
};

G_DEFINE_FINAL_TYPE (UnitySearchResult, unity_search_result, G_TYPE_OBJECT)

UnitySearchResult *
unity_search_result_new (UnitySearchProvider *provider,
                               const gchar              *id,
                               const gchar              *name,
                               const gchar              *description,
                               const gchar              *clipboard_text,
                               GIcon                    *gicon,
                               const gchar *const       *terms)
{
  UnitySearchResult *self = g_object_new (UNITY_TYPE_SEARCH_RESULT, NULL);
  self->provider       = provider ? g_object_ref (provider) : NULL;
  self->id             = g_strdup (id);
  self->name           = g_strdup (name);
  self->description    = g_strdup (description);
  self->clipboard_text = g_strdup (clipboard_text);
  self->gicon          = gicon ? g_object_ref (gicon) : NULL;
  self->terms          = g_strdupv ((gchar **) terms);
  return self;
}

const gchar *
unity_search_result_get_id (UnitySearchResult *self)
{
  g_return_val_if_fail (UNITY_IS_SEARCH_RESULT (self), NULL);
  return self->id;
}

const gchar *
unity_search_result_get_name (UnitySearchResult *self)
{
  g_return_val_if_fail (UNITY_IS_SEARCH_RESULT (self), NULL);
  return self->name;
}

const gchar *
unity_search_result_get_description (UnitySearchResult *self)
{
  g_return_val_if_fail (UNITY_IS_SEARCH_RESULT (self), NULL);
  return self->description;
}

const gchar *
unity_search_result_get_clipboard_text (UnitySearchResult *self)
{
  g_return_val_if_fail (UNITY_IS_SEARCH_RESULT (self), NULL);
  return self->clipboard_text;
}

GIcon *
unity_search_result_get_gicon (UnitySearchResult *self)
{
  g_return_val_if_fail (UNITY_IS_SEARCH_RESULT (self), NULL);
  return self->gicon;
}

const gchar *const *
unity_search_result_get_terms (UnitySearchResult *self)
{
  g_return_val_if_fail (UNITY_IS_SEARCH_RESULT (self), NULL);
  return (const gchar *const *) self->terms;
}

void
unity_search_result_activate (UnitySearchResult *self, guint32 timestamp)
{
  g_return_if_fail (UNITY_IS_SEARCH_RESULT (self));
  if (self->provider != NULL)
    unity_search_provider_activate_result (
      self->provider, self->id, (const gchar *const *) self->terms, timestamp);
}

static void
unity_search_result_dispose (GObject *object)
{
  UnitySearchResult *self = UNITY_SEARCH_RESULT (object);
  g_clear_object (&self->provider);
  g_clear_object (&self->gicon);
  G_OBJECT_CLASS (unity_search_result_parent_class)->dispose (object);
}

static void
unity_search_result_finalize (GObject *object)
{
  UnitySearchResult *self = UNITY_SEARCH_RESULT (object);
  g_free (self->id);
  g_free (self->name);
  g_free (self->description);
  g_free (self->clipboard_text);
  g_strfreev (self->terms);
  G_OBJECT_CLASS (unity_search_result_parent_class)->finalize (object);
}

static void
unity_search_result_class_init (UnitySearchResultClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  object_class->dispose  = unity_search_result_dispose;
  object_class->finalize = unity_search_result_finalize;
}

static void
unity_search_result_init (UnitySearchResult *self)
{
  (void) self;
}
