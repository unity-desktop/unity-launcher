/* unity-search-result-rows.c
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

#include "dash/search/unity-search-result-rows.h"

#include <adwaita.h>

#include "components/unity-search-result-row.h"

struct _UnitySearchResultRows
{
  GtkBox               parent_instance;

  AdwPreferencesGroup *group;     /* template: provider title + launch suffix */
  GtkButton           *launch;    /* template: the "Open in <app>" button */

  UnitySearchProvider *provider;  /* for the header's launch-in-app action */
  GStrv                terms;     /* the query terms handed to LaunchSearch */
};

G_DEFINE_FINAL_TYPE (UnitySearchResultRows, unity_search_result_rows, GTK_TYPE_BOX)

/* Append a slice of the results as UnitySearchResultRow widgets. Each row owns
 * its own display, copy and activation (see unity-search-result-row.c). */
static void
add_rows (GtkListBox *list, GPtrArray *results, guint from, guint to)
{
  for (guint i = from; i < to; i++)
    gtk_list_box_append (list, unity_search_result_row_new (results->pdata[i]));
}

/* A .boxed-list column that holds result rows. */
static GtkListBox *
create_column (void)
{
  GtkListBox *list = GTK_LIST_BOX (gtk_list_box_new ());
  gtk_list_box_set_selection_mode (list, GTK_SELECTION_NONE);
  gtk_widget_add_css_class (GTK_WIDGET (list), "boxed-list");
  gtk_widget_set_valign (GTK_WIDGET (list), GTK_ALIGN_START);
  gtk_widget_set_hexpand (GTK_WIDGET (list), TRUE);
  return list;
}

/* The header-suffix button is the provider's "open in <app>" action: it calls
 * LaunchSearch and closes the dash, mirroring GNOME Shell. */
static void
on_launch_clicked (GtkButton *button, gpointer user_data)
{
  (void) button;
  UnitySearchResultRows *self = user_data;
  if (self->provider != NULL)
    unity_search_provider_launch_search (
      self->provider, (const gchar *const *) self->terms, GDK_CURRENT_TIME);
  gtk_widget_activate_action (GTK_WIDGET (self), "dash.close", NULL);
}

/* The provider title and its launch suffix come from the template. This only
 * lays out the result rows below them. */
static void
populate (UnitySearchResultRows *self, GPtrArray *results)
{
  guint n = results != NULL ? results->len : 0;

  /* One result fills the width. Several split into two columns, the first taking
   * the extra row on odd counts. */
  if (n <= 1)
    {
      GtkListBox *list = create_column ();
      add_rows (list, results, 0, n);
      gtk_box_append (GTK_BOX (self), GTK_WIDGET (list));
      return;
    }

  guint left = (n + 1) / 2;

  GtkWidget *columns = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 12);
  gtk_box_set_homogeneous (GTK_BOX (columns), TRUE);
  gtk_widget_set_hexpand (columns, TRUE);

  GtkListBox *left_list = create_column ();
  add_rows (left_list, results, 0, left);
  gtk_box_append (GTK_BOX (columns), GTK_WIDGET (left_list));

  GtkListBox *right_list = create_column ();
  add_rows (right_list, results, left, n);
  gtk_box_append (GTK_BOX (columns), GTK_WIDGET (right_list));

  gtk_box_append (GTK_BOX (self), columns);
}

GtkWidget *
unity_search_result_rows_new (UnitySearchProvider *provider, GPtrArray *results)
{
  UnitySearchResultRows *self = g_object_new (UNITY_TYPE_SEARCH_RESULT_ROWS, NULL);
  self->provider = provider ? g_object_ref (provider) : NULL;
  if (results != NULL && results->len > 0)
    self->terms = g_strdupv (
      (gchar **) unity_search_result_get_terms (results->pdata[0]));

  const gchar *name = unity_search_provider_get_name (self->provider);
  adw_preferences_group_set_title (self->group, name);
  g_autofree gchar *label = g_strdup_printf ("Open in %s", name);
  gtk_button_set_label (self->launch, label);

  populate (self, results);
  return GTK_WIDGET (self);
}

static void
unity_search_result_rows_dispose (GObject *object)
{
  UnitySearchResultRows *self = UNITY_SEARCH_RESULT_ROWS (object);
  g_clear_object (&self->provider);
  gtk_widget_dispose_template (GTK_WIDGET (object), UNITY_TYPE_SEARCH_RESULT_ROWS);
  G_OBJECT_CLASS (unity_search_result_rows_parent_class)->dispose (object);
}

static void
unity_search_result_rows_finalize (GObject *object)
{
  UnitySearchResultRows *self = UNITY_SEARCH_RESULT_ROWS (object);
  g_strfreev (self->terms);
  G_OBJECT_CLASS (unity_search_result_rows_parent_class)->finalize (object);
}

static void
unity_search_result_rows_class_init (UnitySearchResultRowsClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  G_OBJECT_CLASS (klass)->dispose  = unity_search_result_rows_dispose;
  G_OBJECT_CLASS (klass)->finalize = unity_search_result_rows_finalize;

  gtk_widget_class_set_template_from_resource (
    widget_class, "/org/unity/launcher/dash/search/unity-search-result-rows.ui");
  gtk_widget_class_bind_template_child (widget_class, UnitySearchResultRows, group);
  gtk_widget_class_bind_template_child (widget_class, UnitySearchResultRows, launch);
  gtk_widget_class_bind_template_callback (widget_class, on_launch_clicked);
}

static void
unity_search_result_rows_init (UnitySearchResultRows *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}
