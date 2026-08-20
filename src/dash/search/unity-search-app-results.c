/* unity-search-app-results.c
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

#include "dash/search/unity-search-app-results.h"

#include <astal-apps.h>

#include "components/unity-dash-tile.h"
#include "dash/unity-dash-grid-layout.h"
#include "unity-app-catalog.h"
#include "unity-settings.h"

struct _UnitySearchAppResults
{
  AdwBin         parent_instance;

  AstalAppsApps *catalog;
  GSettings     *settings;

  GtkBox        *tiles;
};

G_DEFINE_FINAL_TYPE (UnitySearchAppResults, unity_search_app_results, ADW_TYPE_BIN)

static void
box_clear (GtkBox *box)
{
  GtkWidget *c;
  while ((c = gtk_widget_get_first_child (GTK_WIDGET (box))) != NULL)
    gtk_box_remove (box, c);
}

guint
unity_search_app_results_fill (UnitySearchAppResults *self, const gchar *query)
{
  g_return_val_if_fail (UNITY_IS_SEARCH_APP_RESULTS (self), 0);

  box_clear (self->tiles);

  GList *apps = astal_apps_apps_fuzzy_query (self->catalog, query);
  guint i = 0;
  for (GList *l = apps; l != NULL; l = l->next, i++)
    {
      GtkWidget *tile = unity_dash_tile_new (l->data);
      g_settings_bind (self->settings, UNITY_LAUNCHER_KEY_DASH_ICON_SIZE,
                       tile, "icon-size", G_SETTINGS_BIND_GET);
      gtk_box_append (self->tiles, tile);
    }
  g_list_free (apps);

  gtk_widget_set_visible (GTK_WIDGET (self), i > 0);
  return i;
}

void
unity_search_app_results_clear (UnitySearchAppResults *self)
{
  g_return_if_fail (UNITY_IS_SEARCH_APP_RESULTS (self));
  box_clear (self->tiles);
  gtk_widget_set_visible (GTK_WIDGET (self), FALSE);
}

void
unity_search_app_results_activate_selected (UnitySearchAppResults *self)
{
  g_return_if_fail (UNITY_IS_SEARCH_APP_RESULTS (self));
  GtkWidget *tile = gtk_widget_get_first_child (GTK_WIDGET (self->tiles));
  if (tile != NULL)
    gtk_widget_activate (tile);
}

void
unity_search_app_results_focus (UnitySearchAppResults *self)
{
  g_return_if_fail (UNITY_IS_SEARCH_APP_RESULTS (self));
  gtk_widget_child_focus (GTK_WIDGET (self->tiles), GTK_DIR_DOWN);
}

GtkWidget *
unity_search_app_results_new (void)
{
  return g_object_new (UNITY_TYPE_SEARCH_APP_RESULTS, NULL);
}

static void
unity_search_app_results_dispose (GObject *object)
{
  gtk_widget_dispose_template (GTK_WIDGET (object), UNITY_TYPE_SEARCH_APP_RESULTS);
  G_OBJECT_CLASS (unity_search_app_results_parent_class)->dispose (object);
}

static void
unity_search_app_results_class_init (UnitySearchAppResultsClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  G_OBJECT_CLASS (klass)->dispose = unity_search_app_results_dispose;

  gtk_widget_class_set_template_from_resource (
    widget_class, "/org/unity/launcher/dash/search/unity-search-app-results.ui");
  gtk_widget_class_bind_template_child (widget_class, UnitySearchAppResults, tiles);
}

static void
unity_search_app_results_init (UnitySearchAppResults *self)
{
  self->catalog  = unity_app_catalog_get_default ();
  self->settings = unity_settings_get_default ();

  gtk_widget_init_template (GTK_WIDGET (self));

  GtkLayoutManager *grid = unity_dash_grid_layout_new ();
  unity_dash_grid_layout_set_max_rows (grid, 1);
  gtk_widget_set_layout_manager (GTK_WIDGET (self->tiles), grid);
}
