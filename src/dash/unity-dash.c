/* unity-dash.c
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

#include "dash/unity-dash.h"

#include <gdk/gdkkeysyms.h>

#include "dash/unity-dash-apps.h"
#include "dash/unity-dash-search.h"
#include "unity-position.h"
#include "unity-settings.h"

#define PAGE_APPS   "apps"
#define PAGE_SEARCH "search"

struct _UnityDash
{
  UnityWindowPopup parent_instance;

  GSettings       *settings;

  AdwToolbarView  *panel;
  GtkSearchEntry  *entry;
  AdwViewStack    *stack;
  UnityDashApps *apps_page;
  UnityDashSearch *search_page;

  gint             launcher_inset;
};

G_DEFINE_FINAL_TYPE (UnityDash, unity_dash, UNITY_TYPE_WINDOW_POPUP)

typedef enum
{
  PROP_LAUNCHER_INSET = 1,
} UnityDashProperty;

static GParamSpec *properties[PROP_LAUNCHER_INSET + 1];

static void
on_closed (UnityDash *self)
{
  unity_dash_reset (self);
}

static void
apply_launcher_inset (UnityDash *self)
{
  UnityPosition pos = g_settings_get_enum (self->settings, UNITY_LAUNCHER_KEY_POSITION);

  g_object_set (self, "margin-left", 0, "margin-right", 0,
                "margin-top", 0, "margin-bottom", 0, NULL);
  if (self->launcher_inset > 0)
    g_object_set (self, unity_position_edge_margin (pos), self->launcher_inset, NULL);
}

static void
apply_panel_alignment (UnityDash *self)
{
  UnityPosition pos;

  if (unity_window_popup_get_maximized (UNITY_WINDOW_POPUP (self)))
    return;

  pos = g_settings_get_enum (self->settings, UNITY_LAUNCHER_KEY_POSITION);
  gtk_widget_set_halign (GTK_WIDGET (self->panel), unity_position_dash_halign (pos));
  gtk_widget_set_valign (GTK_WIDGET (self->panel), unity_position_dash_valign (pos));
}

static void
on_position_changed (UnityDash *self)
{
  apply_panel_alignment (self);
  apply_launcher_inset  (self);
}

GtkWidget *
unity_dash_new (GtkApplication *app)
{
  return g_object_new (UNITY_TYPE_DASH, "application", app, NULL);
}

static void
on_search_changed (GtkSearchEntry *entry, gpointer user_data)
{
  UnityDash   *self = user_data;
  const gchar *text = gtk_editable_get_text (GTK_EDITABLE (entry));
  gboolean     empty = (text == NULL || *text == '\0');

  if (empty)
    {
      unity_dash_search_reset (self->search_page);
      adw_view_stack_set_visible_child_name (self->stack, PAGE_APPS);
      return;
    }

  adw_view_stack_set_visible_child_name (self->stack, PAGE_SEARCH);
  unity_dash_search_run (self->search_page, text);
}

static void
on_entry_activate (GtkSearchEntry *entry, gpointer user_data)
{
  (void) entry;
  UnityDash *self = user_data;
  if (g_strcmp0 (adw_view_stack_get_visible_child_name (self->stack), PAGE_SEARCH) == 0)
    unity_dash_search_activate_selected (self->search_page);
}

static gboolean
on_entry_key (GtkEventControllerKey *key, guint keyval, guint keycode,
              GdkModifierType state, gpointer user_data)
{
  (void) key; (void) keycode; (void) state;
  UnityDash *self = user_data;

  if (keyval == GDK_KEY_Down &&
      g_strcmp0 (adw_view_stack_get_visible_child_name (self->stack), PAGE_SEARCH) == 0)
    {
      unity_dash_search_focus_results (self->search_page);
      return GDK_EVENT_STOP;
    }
  return GDK_EVENT_PROPAGATE;
}

void
unity_dash_reset (UnityDash *self)
{
  gboolean maximized;

  g_return_if_fail (UNITY_IS_DASH (self));

  maximized = g_settings_get_boolean (self->settings, UNITY_LAUNCHER_KEY_DASH_MAXIMIZED);
  unity_window_popup_set_maximized (UNITY_WINDOW_POPUP (self), maximized);

  unity_dash_search_reset (self->search_page);
  adw_view_stack_set_visible_child_name (self->stack, PAGE_APPS);
  gtk_editable_set_text (GTK_EDITABLE (self->entry), "");

  unity_dash_apps_reset (self->apps_page);
  gtk_widget_grab_focus (GTK_WIDGET (self->entry));
}

void
unity_dash_toggle (UnityDash *self)
{
  g_return_if_fail (UNITY_IS_DASH (self));

  if (gtk_widget_get_visible (GTK_WIDGET (self)))
    {
      gtk_widget_set_visible (GTK_WIDGET (self), FALSE);
      return;
    }

  gtk_window_present (GTK_WINDOW (self));

  /* Select the retained query, so an unminimize lets the user type over it. */
  gtk_widget_grab_focus (GTK_WIDGET (self->entry));
  gtk_editable_select_region (GTK_EDITABLE (self->entry), 0, -1);
}

static void
unity_dash_dispose (GObject *object)
{
  gtk_widget_dispose_template (GTK_WIDGET (object), UNITY_TYPE_DASH);
  G_OBJECT_CLASS (unity_dash_parent_class)->dispose (object);
}

static void
unity_dash_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
  UnityDash *self = UNITY_DASH (object);

  (void) pspec;

  switch ((UnityDashProperty) prop_id)
    {
    case PROP_LAUNCHER_INSET:
      g_value_set_int (value, self->launcher_inset);
      break;
    }
}

static void
unity_dash_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
  UnityDash *self = UNITY_DASH (object);

  (void) pspec;

  switch ((UnityDashProperty) prop_id)
    {
    case PROP_LAUNCHER_INSET:
      self->launcher_inset = g_value_get_int (value);
      apply_launcher_inset (self);
      break;
    }
}

static void
unity_dash_class_init (UnityDashClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose      = unity_dash_dispose;
  object_class->get_property = unity_dash_get_property;
  object_class->set_property = unity_dash_set_property;

  properties[PROP_LAUNCHER_INSET] = g_param_spec_int (
    "launcher-inset", NULL, NULL, 0, G_MAXINT, 0,
    G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_properties (object_class, G_N_ELEMENTS (properties), properties);

  g_type_ensure (UNITY_TYPE_DASH_APPS);
  g_type_ensure (UNITY_TYPE_DASH_SEARCH);

  gtk_widget_class_set_template_from_resource (
    widget_class, "/org/unity/launcher/dash/unity-dash.ui");
  gtk_widget_class_bind_template_child (widget_class, UnityDash, panel);
  gtk_widget_class_bind_template_child (widget_class, UnityDash, entry);
  gtk_widget_class_bind_template_child (widget_class, UnityDash, stack);
  gtk_widget_class_bind_template_child (widget_class, UnityDash, apps_page);
  gtk_widget_class_bind_template_child (widget_class, UnityDash, search_page);
  gtk_widget_class_set_css_name (widget_class, "unity-dash");
}

static void
unity_dash_init (UnityDash *self)
{
  self->settings = unity_settings_get_default ();

  gtk_widget_init_template (GTK_WIDGET (self));

  gtk_search_entry_set_key_capture_widget (self->entry, GTK_WIDGET (self));

  g_signal_connect_object (self->entry, "search-changed",
                           G_CALLBACK (on_search_changed), self, G_CONNECT_DEFAULT);
  g_signal_connect_object (self->entry, "activate",
                           G_CALLBACK (on_entry_activate),  self, G_CONNECT_DEFAULT);

  GtkEventController *entry_key = gtk_event_controller_key_new ();
  gtk_event_controller_set_propagation_phase (entry_key, GTK_PHASE_CAPTURE);
  g_signal_connect_object (entry_key, "key-pressed",
                           G_CALLBACK (on_entry_key), self, G_CONNECT_DEFAULT);
  gtk_widget_add_controller (GTK_WIDGET (self->entry), entry_key);

  g_signal_connect (self, "closed", G_CALLBACK (on_closed), NULL);

  apply_panel_alignment (self);
  g_signal_connect_object (self->settings, "changed::" UNITY_LAUNCHER_KEY_POSITION,
                           G_CALLBACK (on_position_changed), self, G_CONNECT_SWAPPED);
  g_signal_connect_object (self, "notify::maximized",
                           G_CALLBACK (apply_panel_alignment), self, G_CONNECT_SWAPPED);

  unity_dash_reset (self);
}
