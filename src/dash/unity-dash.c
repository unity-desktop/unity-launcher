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

#include "components/unity-dismiss.h"
#include "dash/unity-dash-apps.h"
#include "dash/unity-dash-search-controller.h"
#include "dash/unity-dash-search.h"
#include "unity-position.h"
#include "unity-settings.h"

#define UNITY_LAUNCHER_KEY_DASH_MAXIMIZED "dash-maximized"

#define POPOVER_NUM 2
#define POPOVER_DEN 3

struct _UnityDash
{
  AstalWindow                  parent_instance;

  GSettings                   *settings;
  UnityDashSearchController *search;

  AdwBin          *area;
  AdwToolbarView  *panel;
  GtkSearchEntry  *entry;
  AdwViewStack    *stack;
  UnityDashApps   *apps_page;
  UnityDashSearch *search_page;

  gboolean         fullscreen;
  gboolean         suppress_dismiss;  /* guards the hide/re-present in the maximize toggle */
};

/* A layer-shell overlay is used instead of a GtkPopover, because a popover
 * cannot span the whole output. */
G_DEFINE_FINAL_TYPE (UnityDash, unity_dash, ASTAL_TYPE_WINDOW)

static void
apply_layout (UnityDash *self)
{
  GtkWidget *panel = GTK_WIDGET (self->panel);

  if (self->fullscreen)
    {
      gtk_widget_add_css_class (panel, "fullscreen");
      gtk_widget_set_halign  (panel, GTK_ALIGN_FILL);
      gtk_widget_set_valign  (panel, GTK_ALIGN_FILL);
      gtk_widget_set_hexpand (panel, TRUE);
      gtk_widget_set_vexpand (panel, TRUE);
      gtk_widget_set_size_request (panel, -1, -1);
      return;
    }

  gtk_widget_remove_css_class (panel, "fullscreen");

  /* Open in the corner by the dash button; the launcher's exclusive zone keeps it clear. */
  UnityPosition pos = g_settings_get_enum (self->settings, UNITY_LAUNCHER_KEY_POSITION);
  gtk_widget_set_halign (panel, unity_position_dash_halign (pos));
  gtk_widget_set_valign (panel, unity_position_dash_valign (pos));
  gtk_widget_set_hexpand (panel, FALSE);
  gtk_widget_set_vexpand (panel, FALSE);

  GdkRectangle geo = { 0, 0, 0, 0 };
  GdkMonitor  *monitor = astal_window_get_current_monitor (ASTAL_WINDOW (self));
  if (monitor != NULL)
    gdk_monitor_get_geometry (monitor, &geo);
  if (geo.width > 0 && geo.height > 0)
    gtk_widget_set_size_request (panel,
                                 geo.width  * POPOVER_NUM / POPOVER_DEN,
                                 geo.height * POPOVER_NUM / POPOVER_DEN);
}

/* Mirror the fullscreen state onto the window's maximized state so the header
 * bar's maximize button shows the right icon. This only works while unmapped. */
static void
sync_window_maximized (UnityDash *self)
{
  if (self->fullscreen)
    gtk_window_maximize (GTK_WINDOW (self));
  else
    gtk_window_unmaximize (GTK_WINDOW (self));
}

/* Show the window, syncing the maximized state first while it is still unmapped
 * so the native maximize button appears with the correct icon. */
static void
present_dash (UnityDash *self)
{
  sync_window_maximized (self);
  gtk_window_present (GTK_WINDOW (self));
}

/* Overrides of GtkWindow's built-in window.* actions that the header bar's
 * native controls target. */
static void
on_close_action (GtkWidget *widget, const gchar *name, GVariant *param)
{
  (void) name; (void) param;
  unity_dash_close (UNITY_DASH (widget));
}

static void
on_minimize_action (GtkWidget *widget, const gchar *name, GVariant *param)
{
  (void) name; (void) param;
  gtk_widget_set_visible (widget, FALSE);
}

static void
on_toggle_maximized_action (GtkWidget *widget, const gchar *name, GVariant *param)
{
  (void) name; (void) param;
  UnityDash *self = UNITY_DASH (widget);

  self->fullscreen = !self->fullscreen;
  apply_layout (self);
  g_settings_set_boolean (self->settings, UNITY_LAUNCHER_KEY_DASH_MAXIMIZED,
                          self->fullscreen);

  /* The maximize icon tracks a state settable only while unmapped. Hide and
   * re-present so the button rebuilds, and suppress the hide's dismiss. */
  self->suppress_dismiss = TRUE;
  gtk_widget_set_visible (widget, FALSE);
  present_dash (self);
  gtk_widget_grab_focus (GTK_WIDGET (self->entry));
  self->suppress_dismiss = FALSE;
}

/* A launching descendant activates dash.close to dismiss the dash. GTK walks up
 * the widget tree to find the action installed here. */
static void
on_dash_close_action (GtkWidget *widget, const gchar *name, GVariant *param)
{
  (void) name; (void) param;
  unity_dash_close (UNITY_DASH (widget));
}

static void
on_grid_map (GtkWidget *widget, gpointer user_data)
{
  (void) user_data;
  apply_layout (UNITY_DASH (widget));
}

/* Light dismissal (Escape, click-outside, focus-loss): hide but keep state so
 * the next open restores it. Guarded against the maximize toggle's re-present. */
static void
on_dismiss_minimize (gpointer user_data)
{
  UnityDash *self = user_data;
  if (self->suppress_dismiss)
    return;
  gtk_widget_set_visible (GTK_WIDGET (self), FALSE);
}

/* Explicit close (Ctrl+W, Alt+F4, window close-request): hide and reset. */
static void
on_dismiss_close (gpointer user_data)
{
  unity_dash_close (UNITY_DASH (user_data));
}

GtkWidget *
unity_dash_new (GtkApplication *app)
{
  return g_object_new (UNITY_TYPE_DASH, "application", app, NULL);
}

void
unity_dash_reset (UnityDash *self)
{
  g_return_if_fail (UNITY_IS_DASH (self));

  self->fullscreen = g_settings_get_boolean (
    self->settings, UNITY_LAUNCHER_KEY_DASH_MAXIMIZED);
  apply_layout (self);

  unity_dash_search_controller_reset (self->search);
  unity_dash_apps_reset (self->apps_page);
  gtk_widget_grab_focus (GTK_WIDGET (self->entry));
}

void
unity_dash_close (UnityDash *self)
{
  g_return_if_fail (UNITY_IS_DASH (self));
  if (self->suppress_dismiss || !gtk_widget_get_visible (GTK_WIDGET (self)))
    return;
  unity_dash_reset (self);
  gtk_widget_set_visible (GTK_WIDGET (self), FALSE);
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

  present_dash (self);

  /* Select the retained query, so an unminimize lets the user type over it. */
  gtk_widget_grab_focus (GTK_WIDGET (self->entry));
  gtk_editable_select_region (GTK_EDITABLE (self->entry), 0, -1);
}

static void
unity_dash_dispose (GObject *object)
{
  UnityDash *self = UNITY_DASH (object);
  g_clear_object (&self->search);
  G_OBJECT_CLASS (unity_dash_parent_class)->dispose (object);
  gtk_widget_dispose_template (GTK_WIDGET (object), UNITY_TYPE_DASH);
}

static void
unity_dash_class_init (UnityDashClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  G_OBJECT_CLASS (klass)->dispose = unity_dash_dispose;

  g_type_ensure (UNITY_TYPE_DASH_APPS);
  g_type_ensure (UNITY_TYPE_DASH_SEARCH);

  gtk_widget_class_set_template_from_resource (
    widget_class, "/org/unity/launcher/dash/unity-dash.ui");
  gtk_widget_class_bind_template_child (widget_class, UnityDash, area);
  gtk_widget_class_bind_template_child (widget_class, UnityDash, panel);
  gtk_widget_class_bind_template_child (widget_class, UnityDash, entry);
  gtk_widget_class_bind_template_child (widget_class, UnityDash, stack);
  gtk_widget_class_bind_template_child (widget_class, UnityDash, apps_page);
  gtk_widget_class_bind_template_child (widget_class, UnityDash, search_page);
  gtk_widget_class_set_css_name (widget_class, "unity-dash");

  /* Override GtkWindow's built-in window controls so the header bar's native
   * close, minimize and maximize buttons drive the dash instead. */
  gtk_widget_class_install_action (widget_class, "window.close",            NULL, on_close_action);
  gtk_widget_class_install_action (widget_class, "window.minimize",         NULL, on_minimize_action);
  gtk_widget_class_install_action (widget_class, "window.toggle-maximized", NULL, on_toggle_maximized_action);

  /* Descendants launch through this to dismiss the dash. */
  gtk_widget_class_install_action (widget_class, "dash.close", NULL, on_dash_close_action);
}

static void
unity_dash_init (UnityDash *self)
{
  self->settings = unity_settings_get_default ();

  gtk_widget_init_template (GTK_WIDGET (self));

  gtk_search_entry_set_key_capture_widget (self->entry, GTK_WIDGET (self));

  self->search = unity_dash_search_controller_new (
    self->entry, self->stack, self->search_page);

  g_signal_connect (self, "map", G_CALLBACK (on_grid_map), NULL);
  g_signal_connect_object (self->settings, "changed::" UNITY_LAUNCHER_KEY_POSITION,
                           G_CALLBACK (apply_layout), self, G_CONNECT_SWAPPED);

  unity_dismiss_attach (GTK_WIDGET (self), GTK_WIDGET (self->area),
                        GTK_WIDGET (self->panel),
                        on_dismiss_minimize, on_dismiss_close, self);

  unity_dash_reset (self);
}
