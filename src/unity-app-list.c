/* unity-app-list.c
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

#include "unity-app-list.h"


#include <astal-apps.h>
#include <astal-wlr.h>
#include <gtk/gtk.h>

#include "unity-app-catalog.h"

struct _UnityAppList
{
  GObject        parent_instance;

  AstalAppsApps *catalog;
  GListModel    *toplevels;
  GStrv          pinned;

  GListStore    *entries;
  GHashTable    *cache;
  GHashTable    *id_canonical;
};

static void list_model_iface_init (GListModelInterface *iface);

/* Raw app ids (desktop id, compositor app id, or wm class) resolve to one
 * canonical desktop id, so variants of the same app collapse to one entry. */
G_DEFINE_FINAL_TYPE_WITH_CODE (UnityAppList, unity_app_list, G_TYPE_OBJECT,
                               G_IMPLEMENT_INTERFACE (G_TYPE_LIST_MODEL, list_model_iface_init))

static GType    list_model_get_item_type (GListModel *m) { (void) m; return UNITY_TYPE_APP_ENTRY; }
static guint    list_model_get_n_items   (GListModel *m)
{
  return g_list_model_get_n_items (G_LIST_MODEL (UNITY_APP_LIST (m)->entries));
}
static gpointer list_model_get_item      (GListModel *m, guint pos)
{
  return g_list_model_get_item (G_LIST_MODEL (UNITY_APP_LIST (m)->entries), pos);
}

static void
list_model_iface_init (GListModelInterface *iface)
{
  iface->get_item_type = list_model_get_item_type;
  iface->get_n_items   = list_model_get_n_items;
  iface->get_item      = list_model_get_item;
}

static void
on_entries_items_changed (GListModel *entries, guint pos, guint removed, guint added,
                          UnityAppList *self)
{
  (void) entries;
  g_list_model_items_changed (G_LIST_MODEL (self), pos, removed, added);
}

static gchar *
canonicalize_token (const gchar *token)
{
  gchar *out = g_ascii_strdown (token, -1);
  g_strdelimit (out, " ", '-');
  return out;
}

static gchar *
resolve_canonical_for_token (UnityAppList *self, const gchar *token)
{
  if (token == NULL || *token == '\0')
    return NULL;

  GList            *list = astal_apps_apps_get_list (self->catalog);
  g_autofree gchar *with_suffix = g_strconcat (token, ".desktop", NULL);
  gchar            *result = NULL;

  for (GList *l = list; l != NULL && result == NULL; l = l->next)
    {
      const gchar *entry = astal_apps_application_get_entry (l->data);
      if (entry != NULL &&
          (g_ascii_strcasecmp (entry, token) == 0 ||
           g_ascii_strcasecmp (entry, with_suffix) == 0))
        result = g_strdup (entry);
    }
  for (GList *l = list; l != NULL && result == NULL; l = l->next)
    {
      const gchar *wm = astal_apps_application_get_wm_class (l->data);
      if (wm != NULL && *wm != '\0' && g_ascii_strcasecmp (wm, token) == 0)
        result = g_strdup (astal_apps_application_get_entry (l->data));
    }
  g_list_free (list);
  if (result != NULL)
    return result;

  GList *fuzzy = astal_apps_apps_fuzzy_query (self->catalog, token);
  if (fuzzy != NULL)
    result = g_strdup (astal_apps_application_get_entry (fuzzy->data));
  g_list_free (fuzzy);
  return result;
}

static const gchar *
canonical_id_for (UnityAppList *self, const gchar *raw_id)
{
  if (raw_id == NULL || *raw_id == '\0')
    return NULL;

  const gchar *cached = g_hash_table_lookup (self->id_canonical, raw_id);
  if (cached != NULL)
    return cached;

  gchar *canonical = NULL;
  g_auto (GStrv) tokens = g_strsplit_set (raw_id, " \t", -1);
  for (gchar **t = tokens; *t != NULL && canonical == NULL; t++)
    if (**t != '\0')
      canonical = resolve_canonical_for_token (self, *t);
  if (canonical == NULL)
    canonical = canonicalize_token (raw_id);

  g_hash_table_insert (self->id_canonical, g_strdup (raw_id), canonical);
  return canonical;
}

typedef struct
{
  UnityAppList *list;
  gchar        *canonical;
} AppIdMatch;

static void
app_id_match_free (gpointer p)
{
  AppIdMatch *m = p;
  g_free (m->canonical);
  g_free (m);
}

static gboolean
match_app_id (gpointer item, gpointer user_data)
{
  AstalWlrToplevel *tl   = item;
  AppIdMatch         *m    = user_data;
  const gchar        *tlid = astal_wlr_toplevel_get_app_id (tl);

  if (tlid == NULL || m->canonical == NULL)
    return FALSE;
  return g_strcmp0 (canonical_id_for (m->list, tlid), m->canonical) == 0;
}

static GListModel *
build_per_app_filter (UnityAppList *self, const gchar *canonical)
{
  AppIdMatch *m = g_new0 (AppIdMatch, 1);
  m->list      = self;
  m->canonical = g_strdup (canonical);

  GtkCustomFilter *filter = gtk_custom_filter_new (match_app_id, m, app_id_match_free);
  GListModel      *base   = self->toplevels ? g_object_ref (self->toplevels) : NULL;
  return G_LIST_MODEL (gtk_filter_list_model_new (base, GTK_FILTER (filter)));
}

static AstalAppsApplication *
application_for (UnityAppList *self, const gchar *entry_id)
{
  GList                *list  = astal_apps_apps_get_list (self->catalog);
  AstalAppsApplication *found = NULL;

  for (GList *l = list; l != NULL && found == NULL; l = l->next)
    if (g_strcmp0 (astal_apps_application_get_entry (l->data), entry_id) == 0)
      found = l->data;
  g_list_free (list);

  return found;
}

static UnityAppEntry *
ensure_entry (UnityAppList *self, const gchar *canonical)
{
  if (canonical == NULL)
    return NULL;

  UnityAppEntry *entry = g_hash_table_lookup (self->cache, canonical);
  if (entry != NULL)
    return entry;

  g_autoptr (GListModel) toplevels = build_per_app_filter (self, canonical);
  entry = _unity_app_entry_new (canonical, application_for (self, canonical), toplevels);

  g_hash_table_insert (self->cache, g_strdup (canonical), entry);
  return entry;
}

static void
append_unique_entry (UnityAppList *self, const gchar *raw_id, gboolean pinned,
                     GHashTable *seen, GPtrArray *out)
{
  const gchar *canonical = canonical_id_for (self, raw_id);
  if (canonical == NULL || g_hash_table_contains (seen, canonical))
    return;
  UnityAppEntry *entry = ensure_entry (self, canonical);
  if (entry == NULL)
    return;
  g_hash_table_add (seen, (gpointer) canonical);
  _unity_app_entry_set_pinned (entry, pinned);
  g_ptr_array_add (out, entry);
}

static GPtrArray *
compute_desired_order (UnityAppList *self)
{
  GPtrArray *out = g_ptr_array_new ();
  g_autoptr (GHashTable) seen = g_hash_table_new (g_str_hash, g_str_equal);

  if (self->pinned != NULL)
    for (gchar **id = self->pinned; *id != NULL; id++)
      append_unique_entry (self, *id, TRUE, seen, out);

  if (self->toplevels != NULL)
    {
      guint n = g_list_model_get_n_items (self->toplevels);
      for (guint i = 0; i < n; i++)
        {
          g_autoptr (AstalWlrToplevel) tl = g_list_model_get_item (self->toplevels, i);
          append_unique_entry (self, astal_wlr_toplevel_get_app_id (tl), FALSE, seen, out);
        }
    }

  return out;
}

static void
sync_to_desired (UnityAppList *self)
{
  g_autoptr (GPtrArray) desired = compute_desired_order (self);
  GListStore           *current = self->entries;
  guint                 cur_len = g_list_model_get_n_items (G_LIST_MODEL (current));

  g_autoptr (GHashTable) desired_set = g_hash_table_new (NULL, NULL);
  for (guint i = 0; i < desired->len; i++)
    g_hash_table_add (desired_set, g_ptr_array_index (desired, i));

  for (guint i = cur_len; i-- > 0;)
    {
      g_autoptr (GObject) item = g_list_model_get_item (G_LIST_MODEL (current), i);
      if (!g_hash_table_contains (desired_set, item))
        {
          g_list_store_remove (current, i);
          cur_len--;
        }
    }

  for (guint i = 0; i < desired->len; i++)
    {
      gpointer want = g_ptr_array_index (desired, i);
      if (i < cur_len)
        {
          g_autoptr (GObject) here = g_list_model_get_item (G_LIST_MODEL (current), i);
          if (here == want)
            continue;
        }

      gboolean moved = FALSE;
      for (guint j = i + 1; j < cur_len; j++)
        {
          g_autoptr (GObject) at = g_list_model_get_item (G_LIST_MODEL (current), j);
          if (at == want)
            {
              g_list_store_remove (current, j);
              g_list_store_insert (current, i, want);
              moved = TRUE;
              break;
            }
        }
      if (moved)
        continue;

      g_list_store_insert (current, i, want);
      cur_len++;
    }
}

static void
invalidate_entry_filter (UnityAppList *self, const gchar *canonical)
{
  if (canonical == NULL)
    return;

  UnityAppEntry *entry = g_hash_table_lookup (self->cache, canonical);
  if (entry == NULL)
    return;

  GListModel *toplevels = unity_app_entry_get_toplevels (entry);
  if (!GTK_IS_FILTER_LIST_MODEL (toplevels))
    return;

  GtkFilter *filter = gtk_filter_list_model_get_filter (GTK_FILTER_LIST_MODEL (toplevels));
  if (filter != NULL)
    gtk_filter_changed (filter, GTK_FILTER_CHANGE_DIFFERENT);
}

static void
on_toplevel_app_id_notify (AstalWlrToplevel *toplevel, GParamSpec *pspec,
                           UnityAppList *self)
{
  (void) pspec;
  const gchar *new_canonical = canonical_id_for (self, astal_wlr_toplevel_get_app_id (toplevel));
  const gchar *old_canonical = g_object_get_data (G_OBJECT (toplevel), "unity-canonical");

  sync_to_desired (self);

  /* Only the entries the window left and joined need re-filtering, not every one. */
  if (g_strcmp0 (old_canonical, new_canonical) != 0)
    {
      invalidate_entry_filter (self, old_canonical);
      invalidate_entry_filter (self, new_canonical);
      g_object_set_data_full (G_OBJECT (toplevel), "unity-canonical",
                              g_strdup (new_canonical), g_free);
    }
}

static void
on_toplevel_activated_notify (AstalWlrToplevel *toplevel, GParamSpec *pspec,
                              UnityAppList *self)
{
  (void) pspec;
  const gchar   *canonical = g_object_get_data (G_OBJECT (toplevel), "unity-canonical");
  UnityAppEntry *entry     = canonical ? g_hash_table_lookup (self->cache, canonical) : NULL;
  if (entry != NULL)
    _unity_app_entry_recompute (entry);
}

static void
hook_toplevel (UnityAppList *self, AstalWlrToplevel *tl)
{
  const gchar *canonical = canonical_id_for (self, astal_wlr_toplevel_get_app_id (tl));
  g_object_set_data_full (G_OBJECT (tl), "unity-canonical", g_strdup (canonical), g_free);
  g_signal_connect_object (tl, "notify::app-id",
                           G_CALLBACK (on_toplevel_app_id_notify), self, G_CONNECT_DEFAULT);
  g_signal_connect_object (tl, "notify::activated",
                           G_CALLBACK (on_toplevel_activated_notify), self, G_CONNECT_DEFAULT);
}

static void
on_toplevels_items_changed (GListModel *model, guint position, guint removed,
                            guint added, UnityAppList *self)
{
  (void) removed;

  for (guint i = 0; i < added; i++)
    {
      g_autoptr (AstalWlrToplevel) tl = g_list_model_get_item (model, position + i);
      if (tl != NULL)
        hook_toplevel (self, tl);
    }
  sync_to_desired (self);
}

static void
on_catalog_changed (AstalAppsApps *catalog, GParamSpec *pspec, UnityAppList *self)
{
  (void) catalog; (void) pspec;
  g_hash_table_remove_all (self->id_canonical);
  g_hash_table_remove_all (self->cache);
  sync_to_desired (self);
}

static void
unity_app_list_dispose (GObject *object)
{
  UnityAppList *self = UNITY_APP_LIST (object);

  g_clear_object (&self->entries);
  g_clear_pointer (&self->cache,   g_hash_table_unref);
  g_clear_object  (&self->toplevels);

  G_OBJECT_CLASS (unity_app_list_parent_class)->dispose (object);
}

static void
unity_app_list_finalize (GObject *object)
{
  UnityAppList *self = UNITY_APP_LIST (object);

  g_clear_pointer (&self->id_canonical, g_hash_table_unref);
  g_clear_pointer (&self->pinned,       g_strfreev);

  G_OBJECT_CLASS (unity_app_list_parent_class)->finalize (object);
}

static void
unity_app_list_class_init (UnityAppListClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose  = unity_app_list_dispose;
  object_class->finalize = unity_app_list_finalize;
}

static void
unity_app_list_init (UnityAppList *self)
{
  self->entries      = g_list_store_new (UNITY_TYPE_APP_ENTRY);
  g_signal_connect_object (self->entries, "items-changed",
                           G_CALLBACK (on_entries_items_changed), self, G_CONNECT_DEFAULT);
  self->cache        = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_object_unref);
  self->id_canonical = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);

  self->catalog = unity_app_catalog_get_default ();
  g_signal_connect_object (self->catalog, "notify::list",
                           G_CALLBACK (on_catalog_changed), self, G_CONNECT_DEFAULT);

  self->toplevels = g_object_ref (G_LIST_MODEL (astal_wlr_toplevels_get_default ()));
  guint n = g_list_model_get_n_items (self->toplevels);
  for (guint i = 0; i < n; i++)
    {
      g_autoptr (AstalWlrToplevel) tl = g_list_model_get_item (self->toplevels, i);
      hook_toplevel (self, tl);
    }
  g_signal_connect_object (self->toplevels, "items-changed",
                           G_CALLBACK (on_toplevels_items_changed), self, G_CONNECT_DEFAULT);

  sync_to_desired (self);
}

UnityAppList *
unity_app_list_new (void)
{
  return g_object_new (UNITY_TYPE_APP_LIST, NULL);
}

void
unity_app_list_set_pinned_app_ids (UnityAppList *self, const gchar *const *app_ids)
{
  g_return_if_fail (UNITY_IS_APP_LIST (self));

  g_strfreev (self->pinned);
  self->pinned = app_ids ? g_strdupv ((gchar **) app_ids) : NULL;
  sync_to_desired (self);
}

UnityAppEntry *
unity_app_list_get_entry (UnityAppList *self, const gchar *app_id)
{
  g_return_val_if_fail (UNITY_IS_APP_LIST (self), NULL);
  if (app_id == NULL || *app_id == '\0')
    return NULL;

  const gchar *canonical = canonical_id_for (self, app_id);
  return canonical ? g_hash_table_lookup (self->cache, canonical) : NULL;
}
