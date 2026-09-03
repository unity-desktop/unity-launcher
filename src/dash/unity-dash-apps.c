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

#include "components/unity-dash-app-tile.h"
#include "dash/unity-dash-grid-layout.h"
#include "unity-app-catalog.h"
#include "unity-settings.h"

#define FREQUENT_ROWS       1
#define FREQUENT_CANDIDATES 16

struct _UnityDashApps
{
  AdwBin         parent_instance;

  AstalAppsApps     *catalog;
  GSettings         *settings;
  GtkScrolledWindow *scroller;
  GtkBox        *frequent;
  GtkWidget     *divider;
  GtkBox        *cells;
};

G_DEFINE_FINAL_TYPE (UnityDashApps, unity_dash_apps, ADW_TYPE_BIN)

/* The tile keeps itself square and dismisses the dash on its own. Here we only
 * bind its icon size to the setting. */
static GtkWidget *
make_tile (UnityDashApps *self, AstalAppsApplication *app)
{
  GtkWidget *tile = unity_dash_app_tile_new (app);
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

static gint
launch_count (gconstpointer app)
{
  return astal_apps_application_get_frequency ((AstalAppsApplication *) app);
}

static gint
cmp_by_frequency (gconstpointer a, gconstpointer b, gpointer user_data)
{
  gint ca = launch_count (a);
  gint cb = launch_count (b);

  if (ca != cb)
    return (ca > cb) ? -1 : 1;
  return cmp_by_name (a, b, user_data);
}

static void
box_clear (GtkBox *box)
{
  GtkWidget *child;
  while ((child = gtk_widget_get_first_child (GTK_WIDGET (box))) != NULL)
    gtk_box_remove (box, child);
}

static void
fill_frequent (UnityDashApps *self)
{
  box_clear (self->frequent);

  gboolean enabled = g_settings_get_boolean (self->settings,
                                             UNITY_LAUNCHER_KEY_SHOW_FREQUENT_APPS);
  if (!enabled)
    {
      gtk_widget_set_visible (GTK_WIDGET (self->frequent), FALSE);
      gtk_widget_set_visible (self->divider, FALSE);
      return;
    }

  GList *apps     = astal_apps_apps_get_list (self->catalog);
  GList *launched = NULL;

  for (GList *l = apps; l != NULL; l = l->next)
    if (launch_count (l->data) > 0)
      launched = g_list_prepend (launched, l->data);
  launched = g_list_sort_with_data (launched, cmp_by_frequency, NULL);
  g_list_free (apps);

  guint added = 0;
  for (GList *l = launched; l != NULL && added < FREQUENT_CANDIDATES; l = l->next, added++)
    gtk_box_append (self->frequent, make_tile (self, l->data));
  g_list_free (launched);

  gtk_widget_set_visible (GTK_WIDGET (self->frequent), added > 0);
  gtk_widget_set_visible (self->divider, added > 0);
}

static void
fill (UnityDashApps *self)
{
  GList *apps = g_list_sort_with_data (astal_apps_apps_get_list (self->catalog),
                                       cmp_by_name, NULL);

  fill_frequent (self);

  box_clear (self->cells);
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

void
unity_dash_apps_reset (UnityDashApps *self)
{
  g_return_if_fail (UNITY_IS_DASH_APPS (self));

  fill_frequent (self);

  GtkAdjustment *adj = gtk_scrolled_window_get_vadjustment (self->scroller);
  gtk_adjustment_set_value (adj, gtk_adjustment_get_lower (adj));
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
  gtk_widget_class_bind_template_child (widget_class, UnityDashApps, scroller);
  gtk_widget_class_bind_template_child (widget_class, UnityDashApps, frequent);
  gtk_widget_class_bind_template_child (widget_class, UnityDashApps, divider);
  gtk_widget_class_bind_template_child (widget_class, UnityDashApps, cells);
}

static void
unity_dash_apps_init (UnityDashApps *self)
{
  self->catalog  = unity_app_catalog_get_default ();
  self->settings = unity_settings_get_default ();

  gtk_widget_init_template (GTK_WIDGET (self));
  gtk_widget_set_layout_manager (GTK_WIDGET (self->cells), unity_dash_grid_layout_new ());

  GtkLayoutManager *row = unity_dash_grid_layout_new ();
  unity_dash_grid_layout_set_max_rows (row, FREQUENT_ROWS);
  gtk_widget_set_layout_manager (GTK_WIDGET (self->frequent), row);

  g_signal_connect_object (self->catalog, "notify::list",
                           G_CALLBACK (on_catalog_changed), self, G_CONNECT_DEFAULT);
  g_signal_connect_object (self->settings,
                           "changed::" UNITY_LAUNCHER_KEY_SHOW_FREQUENT_APPS,
                           G_CALLBACK (fill_frequent), self, G_CONNECT_SWAPPED);
  fill (self);
}
