/* unity-dash-app-tile.c
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

#include "components/unity-dash-app-tile.h"

struct _UnityDashAppTile
{
  UnityAppTile  parent_instance;

  GtkLabel     *label;
};

G_DEFINE_FINAL_TYPE (UnityDashAppTile, unity_dash_app_tile, UNITY_TYPE_APP_TILE)

static void
on_clicked (GtkButton *button, gpointer user_data)
{
  (void) user_data;
  unity_app_tile_launch (UNITY_APP_TILE (button));
}

static void
sync_label (UnityDashAppTile *self)
{
  AstalAppsApplication *app  = unity_app_tile_get_application (UNITY_APP_TILE (self));
  const gchar          *name = app ? astal_apps_application_get_name (app) : NULL;

  gtk_label_set_text (self->label, name != NULL ? name : "");
}

GtkWidget *
unity_dash_app_tile_new (AstalAppsApplication *app)
{
  return g_object_new (UNITY_TYPE_DASH_APP_TILE, "application", app, NULL);
}

static void
unity_dash_app_tile_class_init (UnityDashAppTileClass *klass)
{
  gtk_widget_class_set_css_name (GTK_WIDGET_CLASS (klass), "unity-dash-tile");
}

static void
unity_dash_app_tile_init (UnityDashAppTile *self)
{
  self->label = GTK_LABEL (gtk_label_new (NULL));
  gtk_label_set_ellipsize (self->label, PANGO_ELLIPSIZE_END);
  gtk_label_set_wrap (self->label, FALSE);
  gtk_label_set_max_width_chars (self->label, 18);
  gtk_widget_set_halign (GTK_WIDGET (self->label), GTK_ALIGN_CENTER);
  gtk_widget_set_margin_top (GTK_WIDGET (self->label), 6);
  gtk_widget_add_css_class (GTK_WIDGET (self->label), "body");
  gtk_box_append (unity_app_tile_get_box (UNITY_APP_TILE (self)), GTK_WIDGET (self->label));

  unity_app_tile_set_menu_position (UNITY_APP_TILE (self), GTK_POS_RIGHT);

  g_signal_connect (self, "clicked", G_CALLBACK (on_clicked), NULL);
  g_signal_connect (self, "notify::application", G_CALLBACK (sync_label), NULL);
}
