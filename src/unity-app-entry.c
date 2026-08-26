/* unity-app-entry.c
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

#include "unity-app-entry.h"

#include <astal-wlr.h>

#include "unity-app-catalog.h"

struct _UnityAppEntry
{
  GObject     parent_instance;

  gchar                *app_id;
  AstalAppsApplication *app;
  GListModel           *toplevels;

  gboolean    pinned;
  gboolean    running;
  gboolean    activated;
};

G_DEFINE_FINAL_TYPE (UnityAppEntry, unity_app_entry, G_TYPE_OBJECT)

typedef enum
{
  PROP_RUNNING = 1,
  PROP_ACTIVATED,
} UnityAppEntryProperty;
static GParamSpec *properties[PROP_ACTIVATED + 1];

static void
recompute_derived (UnityAppEntry *self)
{
  guint    n         = g_list_model_get_n_items (self->toplevels);
  gboolean running   = (n > 0);
  gboolean activated = FALSE;

  for (guint i = 0; i < n; i++)
    {
      g_autoptr (AstalWlrToplevel) tl = g_list_model_get_item (self->toplevels, i);
      if (astal_wlr_toplevel_get_activated (tl))
        {
          activated = TRUE;
          break;
        }
    }

  if (self->running != running)
    {
      self->running = running;
      g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_RUNNING]);
    }
  if (self->activated != activated)
    {
      self->activated = activated;
      g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_ACTIVATED]);
    }
}

static void
on_toplevels_items_changed (GListModel *model, guint position, guint removed,
                            guint added, UnityAppEntry *self)
{
  (void) model; (void) position; (void) removed; (void) added;
  recompute_derived (self);
}

static void
unity_app_entry_dispose (GObject *object)
{
  UnityAppEntry *self = UNITY_APP_ENTRY (object);

  g_clear_object (&self->toplevels);
  g_clear_object (&self->app);

  G_OBJECT_CLASS (unity_app_entry_parent_class)->dispose (object);
}

static void
unity_app_entry_finalize (GObject *object)
{
  UnityAppEntry *self = UNITY_APP_ENTRY (object);

  g_clear_pointer (&self->app_id, g_free);

  G_OBJECT_CLASS (unity_app_entry_parent_class)->finalize (object);
}

static void
unity_app_entry_get_property (GObject *object, guint id, GValue *value, GParamSpec *pspec)
{
  UnityAppEntry *self = UNITY_APP_ENTRY (object);
  switch ((UnityAppEntryProperty) id)
    {
    case PROP_RUNNING:   g_value_set_boolean (value, self->running);   break;
    case PROP_ACTIVATED: g_value_set_boolean (value, self->activated); break;
    default: G_OBJECT_WARN_INVALID_PROPERTY_ID (object, id, pspec);
    }
}

static void
unity_app_entry_class_init (UnityAppEntryClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose      = unity_app_entry_dispose;
  object_class->finalize     = unity_app_entry_finalize;
  object_class->get_property = unity_app_entry_get_property;

  properties[PROP_RUNNING] = g_param_spec_boolean (
    "running", NULL, NULL, FALSE, G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);
  properties[PROP_ACTIVATED] = g_param_spec_boolean (
    "activated", NULL, NULL, FALSE, G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, G_N_ELEMENTS (properties), properties);
}

static void
unity_app_entry_init (UnityAppEntry *self)
{
  (void) self;
}

UnityAppEntry *
_unity_app_entry_new (const gchar *app_id, AstalAppsApplication *app, GListModel *toplevels)
{
  UnityAppEntry *self;

  g_return_val_if_fail (app_id != NULL, NULL);
  g_return_val_if_fail (G_IS_LIST_MODEL (toplevels), NULL);

  self = g_object_new (UNITY_TYPE_APP_ENTRY, NULL);
  self->app_id    = g_strdup (app_id);
  self->app       = app ? g_object_ref (app) : NULL;
  self->toplevels = g_object_ref (toplevels);

  g_signal_connect_object (toplevels, "items-changed",
                           G_CALLBACK (on_toplevels_items_changed), self, G_CONNECT_DEFAULT);

  recompute_derived (self);
  return self;
}

void
_unity_app_entry_recompute (UnityAppEntry *self)
{
  g_return_if_fail (UNITY_IS_APP_ENTRY (self));
  recompute_derived (self);
}

void
_unity_app_entry_set_pinned (UnityAppEntry *self, gboolean pinned)
{
  g_return_if_fail (UNITY_IS_APP_ENTRY (self));
  pinned = !!pinned;
  if (self->pinned == pinned)
    return;
  self->pinned = pinned;
}

const gchar *
unity_app_entry_get_app_id (UnityAppEntry *self)
{
  g_return_val_if_fail (UNITY_IS_APP_ENTRY (self), NULL);
  return self->app_id;
}

GAppInfo *
unity_app_entry_get_app_info (UnityAppEntry *self)
{
  g_return_val_if_fail (UNITY_IS_APP_ENTRY (self), NULL);
  if (self->app == NULL)
    return NULL;
  return G_APP_INFO (astal_apps_application_get_app (self->app));
}

AstalAppsApplication *
unity_app_entry_get_application (UnityAppEntry *self)
{
  g_return_val_if_fail (UNITY_IS_APP_ENTRY (self), NULL);
  return self->app;
}

GListModel *
unity_app_entry_get_toplevels (UnityAppEntry *self)
{
  g_return_val_if_fail (UNITY_IS_APP_ENTRY (self), NULL);
  return self->toplevels;
}

gboolean
unity_app_entry_get_pinned (UnityAppEntry *self)
{
  g_return_val_if_fail (UNITY_IS_APP_ENTRY (self), FALSE);
  return self->pinned;
}

gboolean
unity_app_entry_get_running (UnityAppEntry *self)
{
  g_return_val_if_fail (UNITY_IS_APP_ENTRY (self), FALSE);
  return self->running;
}

gboolean
unity_app_entry_get_activated (UnityAppEntry *self)
{
  g_return_val_if_fail (UNITY_IS_APP_ENTRY (self), FALSE);
  return self->activated;
}

void
unity_app_entry_activate_or_launch (UnityAppEntry *self)
{
  g_return_if_fail (UNITY_IS_APP_ENTRY (self));

  guint n = g_list_model_get_n_items (self->toplevels);
  if (n == 0)
    {
      if (self->app != NULL)
        unity_app_catalog_launch (self->app);
      return;
    }

  for (guint i = 0; i < n; i++)
    {
      g_autoptr (AstalWlrToplevel) tl = g_list_model_get_item (self->toplevels, i);
      if (astal_wlr_toplevel_get_activated (tl))
        {
          astal_wlr_toplevel_minimize (tl, TRUE);
          return;
        }
    }

  g_autoptr (AstalWlrToplevel) first = g_list_model_get_item (self->toplevels, 0);
  if (first != NULL)
    astal_wlr_toplevel_activate (first);
}

void
unity_app_entry_close_all (UnityAppEntry *self)
{
  g_return_if_fail (UNITY_IS_APP_ENTRY (self));

  guint n = g_list_model_get_n_items (self->toplevels);
  g_autoptr (GPtrArray) snapshot = g_ptr_array_new_full (n, g_object_unref);
  for (guint i = 0; i < n; i++)
    g_ptr_array_add (snapshot, g_list_model_get_item (self->toplevels, i));

  for (guint i = 0; i < snapshot->len; i++)
    astal_wlr_toplevel_close (g_ptr_array_index (snapshot, i));
}
