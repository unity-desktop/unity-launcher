/* unity-dash-apps.c
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

#include "dash/unity-dash-apps.h"

#include <astal-apps.h>

#include "components/unity-dash-tile.h"
#include "dash/unity-dash-grid-layout.h"
#include "unity-app-catalog.h"
#include "unity-settings.h"

struct _UnityDashApps
{
  AdwBin         parent_instance;

  AstalAppsApps *catalog;
  GSettings     *settings;
  GtkBox        *cells;
};

G_DEFINE_FINAL_TYPE (UnityDashApps, unity_dash_apps, ADW_TYPE_BIN)

/* The tile keeps itself square and dismisses the dash on its own. Here we only
 * bind its icon size to the setting. */
static GtkWidget *
make_tile (UnityDashApps *self, AstalAppsApplication *app)
{
  GtkWidget *tile = unity_dash_tile_new (app);
  g_settings_bind (self->settings, UNITY_LAUNCHER_KEY_DASH_ICON_SIZE,
                   tile, "icon-size", G_SETTINGS_BIND_GET);
  return tile;
}

static gint
cmp_by_name (gconstpointer a, gconstpointer b, gpointer user_data)
{
  (void) user_data;
  const gchar *na = astal_apps_application_get_name ((AstalAppsApplication *) a);
  const gchar *nb = astal_apps_application_get_name ((AstalAppsApplication *) b);
  return g_utf8_collate (na ? na : "", nb ? nb : "");
}

static void
fill (UnityDashApps *self)
{
  GList *apps = g_list_sort_with_data (astal_apps_apps_get_list (self->catalog),
                                       cmp_by_name, NULL);

  GtkWidget *child;
  while ((child = gtk_widget_get_first_child (GTK_WIDGET (self->cells))) != NULL)
    gtk_box_remove (self->cells, child);

  for (GList *l = apps; l != NULL; l = l->next)
    gtk_box_append (self->cells, make_tile (self, l->data));
  g_list_free (apps);
}

static void
on_catalog_changed (AstalAppsApps *catalog, GParamSpec *pspec, gpointer user_data)
{
  (void) catalog; (void) pspec;
  fill (user_data);
}

GtkWidget *
unity_dash_apps_new (void)
{
  return g_object_new (UNITY_TYPE_DASH_APPS, NULL);
}

void
unity_dash_apps_reset (UnityDashApps *self)
{
  g_return_if_fail (UNITY_IS_DASH_APPS (self));
  GtkWidget *sw = gtk_widget_get_ancestor (GTK_WIDGET (self->cells), GTK_TYPE_SCROLLED_WINDOW);
  if (sw != NULL)
    {
      GtkAdjustment *adj = gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (sw));
      gtk_adjustment_set_value (adj, gtk_adjustment_get_lower (adj));
    }
}

static void
unity_dash_apps_dispose (GObject *object)
{
  gtk_widget_dispose_template (GTK_WIDGET (object), UNITY_TYPE_DASH_APPS);
  G_OBJECT_CLASS (unity_dash_apps_parent_class)->dispose (object);
}

static void
unity_dash_apps_class_init (UnityDashAppsClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  G_OBJECT_CLASS (klass)->dispose = unity_dash_apps_dispose;

  gtk_widget_class_set_template_from_resource (
    widget_class, "/org/unity/launcher/dash/unity-dash-apps.ui");
  gtk_widget_class_bind_template_child (widget_class, UnityDashApps, cells);
}

static void
unity_dash_apps_init (UnityDashApps *self)
{
  self->catalog  = unity_app_catalog_get_default ();
  self->settings = unity_settings_get_default ();

  gtk_widget_init_template (GTK_WIDGET (self));
  gtk_widget_set_layout_manager (GTK_WIDGET (self->cells), unity_dash_grid_layout_new ());

  g_signal_connect_object (self->catalog, "notify::list",
                           G_CALLBACK (on_catalog_changed), self, G_CONNECT_DEFAULT);
  fill (self);
}
