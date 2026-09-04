/* unity-search-provider.c
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

#include "dash/search/unity-search-provider.h"

#include <gio/gdesktopappinfo.h>

#define SEARCH_PROVIDER_IFACE   "org.gnome.Shell.SearchProvider2"
#define PROVIDER_GROUP          "Shell Search Provider"
#define SEARCH_PROVIDERS_SCHEMA "org.gnome.desktop.search-providers"

struct _UnitySearchProvider
{
  GObject          parent_instance;

  gchar           *bus_name;
  gchar           *object_path;
  gchar           *app_id;
  gboolean         default_enabled;
  GDesktopAppInfo *app_info;

  GStrv            last_terms;
  GStrv            last_ids;
};

G_DEFINE_FINAL_TYPE (UnitySearchProvider, unity_search_provider, G_TYPE_OBJECT)

static GDBusConnection *
session_bus (void)
{
  static GDBusConnection *cached;

  if (g_once_init_enter_pointer (&cached))
    {
      g_autoptr (GError) error = NULL;
      GDBusConnection *bus = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, &error);
      if (bus == NULL)
        g_warning ("search provider: session bus: %s", error->message);
      g_once_init_leave_pointer (&cached, bus);
    }
  return cached;
}

static DexFuture *
call_provider (UnitySearchProvider *self, const gchar *method, GVariant *params)
{
  return dex_dbus_connection_call (
    session_bus (), self->bus_name, self->object_path,
    SEARCH_PROVIDER_IFACE, method, params,
    NULL, G_DBUS_CALL_FLAGS_NONE, -1);
}

static gboolean
terms_extend (GStrv prev, const gchar *const *next)
{
  if (prev == NULL || prev[0] == NULL)
    return FALSE;
  g_autofree gchar *p = g_strjoinv (" ", prev);
  g_autofree gchar *n = g_strjoinv (" ", (gchar **) next);
  return g_str_has_prefix (n, p);
}

static DexFuture *
pick_ids_call (UnitySearchProvider *self, const gchar *const *terms)
{
  if (self->last_ids != NULL && self->last_ids[0] != NULL
      && terms_extend (self->last_terms, terms))
    return call_provider (self, "GetSubsearchResultSet",
                          g_variant_new ("(^as^as)", self->last_ids, terms));

  return call_provider (self, "GetInitialResultSet",
                        g_variant_new ("(^as)", terms));
}

static void
remember_query (UnitySearchProvider *self, GStrv terms, const gchar **ids)
{
  g_strfreev (self->last_terms);
  self->last_terms = g_strdupv (terms);
  g_strfreev (self->last_ids);
  self->last_ids = g_strdupv ((gchar **) ids);
}

static GIcon *
gicon_from_meta (GVariant *icon, GVariant *gicon)
{
  if (gicon != NULL)
    return g_icon_new_for_string (g_variant_get_string (gicon, NULL), NULL);
  if (icon != NULL)
    return g_icon_deserialize (icon);
  return NULL;
}

static UnitySearchResult *
result_from_meta (UnitySearchProvider *self, GVariant *meta, const gchar *const *terms)
{
  g_autoptr (GVariant) v_id    = g_variant_lookup_value (meta, "id",            NULL);
  g_autoptr (GVariant) v_name  = g_variant_lookup_value (meta, "name",          NULL);
  g_autoptr (GVariant) v_desc  = g_variant_lookup_value (meta, "description",   NULL);
  g_autoptr (GVariant) v_clip  = g_variant_lookup_value (meta, "clipboardText", NULL);
  g_autoptr (GVariant) v_icon  = g_variant_lookup_value (meta, "icon",          NULL);
  g_autoptr (GVariant) v_gicon = g_variant_lookup_value (meta, "gicon",         NULL);

  if (v_id == NULL || v_name == NULL)
    return NULL;

  /* Skip the "open-in-" companion; the primary rows already launch the app. */
  const gchar *id = g_variant_get_string (v_id, NULL);
  if (g_str_has_prefix (id, "open-in-"))
    return NULL;

  g_autoptr (GIcon) gicon = gicon_from_meta (v_icon, v_gicon);
  return unity_search_result_new (
    self, id, g_variant_get_string (v_name, NULL),
    v_desc ? g_variant_get_string (v_desc, NULL) : NULL,
    v_clip ? g_variant_get_string (v_clip, NULL) : NULL,
    gicon, terms);
}

static GPtrArray *
results_from_reply (UnitySearchProvider *self, GVariant *metas_reply, const gchar *const *terms)
{
  GPtrArray *results = g_ptr_array_new_with_free_func (g_object_unref);

  g_autoptr (GVariant) metas = g_variant_get_child_value (metas_reply, 0);
  GVariantIter iter;
  g_variant_iter_init (&iter, metas);
  GVariant *meta;
  while ((meta = g_variant_iter_next_value (&iter)))
    {
      UnitySearchResult *r = result_from_meta (self, meta, terms);
      if (r != NULL)
        g_ptr_array_add (results, r);
      g_variant_unref (meta);
    }
  return results;
}

typedef struct
{
  UnitySearchProvider *provider;
  GStrv                terms;
  guint                limit;
} QueryArgs;

static void
query_args_free (gpointer data)
{
  QueryArgs *args = data;
  g_object_unref (args->provider);
  g_strfreev (args->terms);
  g_free (args);
}

static DexFuture *
run_query (gpointer data)
{
  QueryArgs           *args = data;
  UnitySearchProvider *self = args->provider;
  g_autoptr (GError)   error = NULL;

  g_autoptr (GVariant) ids_reply = dex_await_variant (
    pick_ids_call (self, (const gchar *const *) args->terms), &error);
  if (ids_reply == NULL)
    return dex_future_new_for_error (g_steal_pointer (&error));

  g_autoptr (GVariant) v_ids = g_variant_get_child_value (ids_reply, 0);
  gsize n_ids = 0;
  g_autofree const gchar **ids = g_variant_get_strv (v_ids, &n_ids);

  remember_query (self, args->terms, ids);

  if (n_ids == 0)
    return dex_future_new_take_boxed (G_TYPE_PTR_ARRAY,
      g_ptr_array_new_with_free_func (g_object_unref));

  if (n_ids > args->limit)
    ids[args->limit] = NULL;

  g_autoptr (GVariant) metas_reply = dex_await_variant (
    call_provider (self, "GetResultMetas", g_variant_new ("(^as)", ids)), &error);
  if (metas_reply == NULL)
    return dex_future_new_for_error (g_steal_pointer (&error));

  return dex_future_new_take_boxed (G_TYPE_PTR_ARRAY,
    results_from_reply (self, metas_reply, (const gchar *const *) args->terms));
}

DexFuture *
unity_search_provider_query (UnitySearchProvider *self,
                             const gchar *const  *terms,
                             guint                limit)
{
  g_return_val_if_fail (UNITY_IS_SEARCH_PROVIDER (self), NULL);

  QueryArgs *args = g_new0 (QueryArgs, 1);
  args->provider  = g_object_ref (self);
  args->terms     = g_strdupv ((gchar **) terms);
  args->limit     = limit;

  return dex_scheduler_spawn (NULL, 0, run_query, args, query_args_free);
}

void
unity_search_provider_activate_result (UnitySearchProvider *self,
                                       const gchar         *id,
                                       const gchar *const  *terms,
                                       guint32              timestamp)
{
  g_return_if_fail (UNITY_IS_SEARCH_PROVIDER (self));
  if (session_bus () == NULL)
    return;

  dex_future_disown (call_provider (
    self, "ActivateResult", g_variant_new ("(s^asu)", id, terms, timestamp)));
}

void
unity_search_provider_launch_search (UnitySearchProvider *self,
                                     const gchar *const  *terms,
                                     guint32              timestamp)
{
  g_return_if_fail (UNITY_IS_SEARCH_PROVIDER (self));
  if (session_bus () == NULL)
    return;

  dex_future_disown (call_provider (
    self, "LaunchSearch", g_variant_new ("(^asu)", terms, timestamp)));
}

void
unity_search_provider_reset (UnitySearchProvider *self)
{
  g_return_if_fail (UNITY_IS_SEARCH_PROVIDER (self));
  g_clear_pointer (&self->last_terms, g_strfreev);
  g_clear_pointer (&self->last_ids, g_strfreev);
}

const gchar *
unity_search_provider_get_name (UnitySearchProvider *self)
{
  g_return_val_if_fail (UNITY_IS_SEARCH_PROVIDER (self), NULL);
  return self->app_info ? g_app_info_get_display_name (G_APP_INFO (self->app_info)) : NULL;
}

static UnitySearchProvider *
provider_new (const gchar *bus_name, const gchar *object_path,
              const gchar *desktop_id, gboolean default_enabled)
{
  g_autoptr (GDesktopAppInfo) app_info = g_desktop_app_info_new (desktop_id);
  if (app_info == NULL || !g_app_info_should_show (G_APP_INFO (app_info)))
    return NULL;

  UnitySearchProvider *self = g_object_new (UNITY_TYPE_SEARCH_PROVIDER, NULL);
  self->bus_name        = g_strdup (bus_name);
  self->object_path     = g_strdup (object_path);
  self->app_id          = g_strdup (g_app_info_get_id (G_APP_INFO (app_info)));
  self->default_enabled = default_enabled;
  self->app_info        = g_steal_pointer (&app_info);
  return self;
}

static void
discover_in_dir (const gchar *data_dir, GHashTable *seen, GList **out)
{
  g_autofree gchar *dir = g_build_filename (data_dir, "gnome-shell", "search-providers", NULL);
  g_autoptr (GDir) d = g_dir_open (dir, 0, NULL);
  if (d == NULL)
    return;

  const gchar *name;
  while ((name = g_dir_read_name (d)) != NULL)
    {
      if (!g_str_has_suffix (name, ".ini"))
        continue;
      if (!g_hash_table_add (seen, g_strdup (name)))
        continue;

      g_autofree gchar *path = g_build_filename (dir, name, NULL);
      g_autoptr (GKeyFile) kf = g_key_file_new ();
      if (!g_key_file_load_from_file (kf, path, G_KEY_FILE_NONE, NULL))
        continue;
      if (!g_key_file_has_group (kf, PROVIDER_GROUP))
        continue;

      g_autofree gchar *version = g_key_file_get_string (kf, PROVIDER_GROUP, "Version", NULL);
      if (g_strcmp0 (version, "2") != 0)
        continue;

      g_autofree gchar *bus  = g_key_file_get_string (kf, PROVIDER_GROUP, "BusName", NULL);
      g_autofree gchar *obj  = g_key_file_get_string (kf, PROVIDER_GROUP, "ObjectPath", NULL);
      g_autofree gchar *desk = g_key_file_get_string (kf, PROVIDER_GROUP, "DesktopId", NULL);
      if (bus == NULL || obj == NULL || desk == NULL)
        continue;

      gboolean default_enabled =
        !g_key_file_get_boolean (kf, PROVIDER_GROUP, "DefaultDisabled", NULL);

      UnitySearchProvider *p = provider_new (bus, obj, desk, default_enabled);
      if (p != NULL)
        *out = g_list_prepend (*out, p);
    }
}

static gint
strv_index (GStrv strv, const gchar *needle)
{
  if (strv == NULL || needle == NULL)
    return -1;
  for (gint i = 0; strv[i] != NULL; i++)
    if (g_strcmp0 (strv[i], needle) == 0)
      return i;
  return -1;
}

static gint
compare_providers (gconstpointer a, gconstpointer b, gpointer user_data)
{
  UnitySearchProvider *pa = UNITY_SEARCH_PROVIDER ((gpointer) a);
  UnitySearchProvider *pb = UNITY_SEARCH_PROVIDER ((gpointer) b);
  GStrv sort = user_data;

  gint ia = strv_index (sort, pa->app_id);
  gint ib = strv_index (sort, pb->app_id);

  if (ia == -1 && ib == -1)
    return g_utf8_collate (g_app_info_get_name (G_APP_INFO (pa->app_info)),
                           g_app_info_get_name (G_APP_INFO (pb->app_info)));
  if (ia == -1)
    return 1;
  if (ib == -1)
    return -1;
  return ia - ib;
}

GList *
unity_search_provider_discover (void)
{
  GList *out = NULL;
  g_autoptr (GHashTable) seen = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);

  discover_in_dir (g_get_user_data_dir (), seen, &out);
  for (const gchar *const *p = g_get_system_data_dirs (); p && *p; p++)
    discover_in_dir (*p, seen, &out);

  GSettingsSchemaSource *source = g_settings_schema_source_get_default ();
  g_autoptr (GSettingsSchema) schema =
    source ? g_settings_schema_source_lookup (source, SEARCH_PROVIDERS_SCHEMA, TRUE) : NULL;
  if (schema == NULL)
    return out;

  g_autoptr (GSettings) settings = g_settings_new (SEARCH_PROVIDERS_SCHEMA);

  if (g_settings_get_boolean (settings, "disable-external"))
    {
      g_list_free_full (out, g_object_unref);
      return NULL;
    }

  g_auto (GStrv) disabled   = g_settings_get_strv (settings, "disabled");
  g_auto (GStrv) enabled    = g_settings_get_strv (settings, "enabled");
  g_auto (GStrv) sort_order = g_settings_get_strv (settings, "sort-order");

  GList *kept = NULL;
  for (GList *l = out; l != NULL; l = l->next)
    {
      UnitySearchProvider *p = l->data;
      gboolean keep = p->default_enabled
        ? !g_strv_contains ((const gchar *const *) disabled, p->app_id)
        :  g_strv_contains ((const gchar *const *) enabled,  p->app_id);
      if (keep)
        kept = g_list_prepend (kept, p);
      else
        g_object_unref (p);
    }
  g_list_free (out);

  return g_list_sort_with_data (kept, compare_providers, sort_order);
}

static void
unity_search_provider_dispose (GObject *object)
{
  UnitySearchProvider *self = UNITY_SEARCH_PROVIDER (object);
  g_clear_object (&self->app_info);
  G_OBJECT_CLASS (unity_search_provider_parent_class)->dispose (object);
}

static void
unity_search_provider_finalize (GObject *object)
{
  UnitySearchProvider *self = UNITY_SEARCH_PROVIDER (object);
  g_free (self->bus_name);
  g_free (self->object_path);
  g_free (self->app_id);
  g_strfreev (self->last_terms);
  g_strfreev (self->last_ids);
  G_OBJECT_CLASS (unity_search_provider_parent_class)->finalize (object);
}

static void
unity_search_provider_class_init (UnitySearchProviderClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  object_class->dispose  = unity_search_provider_dispose;
  object_class->finalize = unity_search_provider_finalize;
}

static void
unity_search_provider_init (UnitySearchProvider *self)
{
  (void) self;
  dex_init ();
}
