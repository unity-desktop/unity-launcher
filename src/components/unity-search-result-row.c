/* unity-search-result-row.c
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

#include "components/unity-search-result-row.h"

struct _UnitySearchResultRow
{
  AdwActionRow       parent_instance;

  UnitySearchResult *result;

  GtkImage          *icon;       /* template: prefix icon, hidden when absent */
  GtkButton         *copy;       /* template: copy button, shown when copyable */
  GtkSeparator      *separator;  /* template: divider ahead of the copy button */
};

G_DEFINE_FINAL_TYPE (UnitySearchResultRow, unity_search_result_row, ADW_TYPE_ACTION_ROW)

static void
on_copy_clicked (GtkButton *button, gpointer user_data)
{
  UnitySearchResultRow *self = user_data;
  const gchar *text = unity_search_result_get_clipboard_text (self->result);
  if (text != NULL && *text != '\0')
    gdk_clipboard_set_text (gtk_widget_get_clipboard (GTK_WIDGET (button)), text);
}

/* Launch the result. Overrides AdwActionRow's activate so a click or Enter on
 * the row triggers it. */
static void
unity_search_result_row_activate (AdwActionRow *row)
{
  UnitySearchResultRow *self = UNITY_SEARCH_RESULT_ROW (row);
  if (self->result != NULL)
    unity_search_result_activate (self->result, GDK_CURRENT_TIME);
}

GtkWidget *
unity_search_result_row_new (UnitySearchResult *result)
{
  UnitySearchResultRow *self = g_object_new (UNITY_TYPE_SEARCH_RESULT_ROW, NULL);
  self->result = result ? g_object_ref (result) : NULL;

  adw_preferences_row_set_title (ADW_PREFERENCES_ROW (self),
                                 unity_search_result_get_name (result));
  const gchar *desc = unity_search_result_get_description (result);
  if (desc != NULL && *desc != '\0')
    adw_action_row_set_subtitle (ADW_ACTION_ROW (self), desc);

  GIcon *gicon = unity_search_result_get_gicon (result);
  if (gicon != NULL)
    {
      gtk_image_set_from_gicon (self->icon, gicon);
      gtk_widget_set_visible (GTK_WIDGET (self->icon), TRUE);
    }

  const gchar *copy_text = unity_search_result_get_clipboard_text (result);
  if (copy_text != NULL && *copy_text != '\0')
    {
      gtk_widget_set_visible (GTK_WIDGET (self->copy), TRUE);
      gtk_widget_set_visible (GTK_WIDGET (self->separator), TRUE);
    }

  return GTK_WIDGET (self);
}

static void
unity_search_result_row_dispose (GObject *object)
{
  UnitySearchResultRow *self = UNITY_SEARCH_RESULT_ROW (object);
  g_clear_object (&self->result);
  gtk_widget_dispose_template (GTK_WIDGET (object), UNITY_TYPE_SEARCH_RESULT_ROW);
  G_OBJECT_CLASS (unity_search_result_row_parent_class)->dispose (object);
}

static void
unity_search_result_row_class_init (UnitySearchResultRowClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  G_OBJECT_CLASS (klass)->dispose = unity_search_result_row_dispose;
  ADW_ACTION_ROW_CLASS (klass)->activate = unity_search_result_row_activate;

  gtk_widget_class_set_template_from_resource (
    widget_class, "/org/unity/launcher/components/unity-search-result-row.ui");
  gtk_widget_class_bind_template_child (widget_class, UnitySearchResultRow, icon);
  gtk_widget_class_bind_template_child (widget_class, UnitySearchResultRow, copy);
  gtk_widget_class_bind_template_child (widget_class, UnitySearchResultRow, separator);
  gtk_widget_class_bind_template_callback (widget_class, on_copy_clicked);
}

static void
unity_search_result_row_init (UnitySearchResultRow *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}
