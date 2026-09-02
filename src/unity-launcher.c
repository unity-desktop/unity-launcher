/* unity-launcher.c
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

#include "unity-launcher.h"

#include "components/unity-hide.h"
#include "components/unity-launcher-app-strip.h"
#include "components/unity-launcher-dash-button.h"
#include "unity-position.h"
#include "unity-settings.h"

struct _UnityLauncher
{
  AstalWindow            parent_instance;
  GtkBox                *content;
  UnityLauncherAppStrip *app_strip;

  GSettings             *settings;

  UnityHide             *hide;
};

G_DEFINE_FINAL_TYPE (UnityLauncher, unity_launcher, UNITY_TYPE_STRIP)

static void
apply_position (UnityLauncher *self)
{
  UnityPosition  pos = g_settings_get_enum (self->settings, UNITY_LAUNCHER_KEY_POSITION);
  GtkOrientation orientation = unity_position_orientation (pos);

  astal_window_set_anchor (ASTAL_WINDOW (self), unity_position_anchor (pos));
  gtk_orientable_set_orientation (GTK_ORIENTABLE (self->content), orientation);
  gtk_orientable_set_orientation (GTK_ORIENTABLE (self->app_strip), orientation);
}

/* A tile being dragged or showing its menu holds the launcher revealed, so it
 * never hides out from under the interaction. */
static void
on_strip_dragging (GObject *strip, GParamSpec *pspec, gpointer user_data)
{
  (void) pspec;
  UnityLauncher *self = user_data;
  unity_hide_set_hold (self->hide, UNITY_HIDE_HOLD_DRAG,
                       unity_launcher_app_strip_get_dragging (
                         UNITY_LAUNCHER_APP_STRIP (strip)));
}

static void
on_strip_menu_shown (GObject *strip, GParamSpec *pspec, gpointer user_data)
{
  (void) pspec;
  UnityLauncher *self = user_data;
  unity_hide_set_hold (self->hide, UNITY_HIDE_HOLD_MENU,
                       unity_launcher_app_strip_get_menu_shown (
                         UNITY_LAUNCHER_APP_STRIP (strip)));
}

static void
unity_launcher_dispose (GObject *object)
{
  UnityLauncher *self = UNITY_LAUNCHER (object);

  g_clear_object (&self->hide);

  gtk_widget_dispose_template (GTK_WIDGET (object), UNITY_TYPE_LAUNCHER);
  G_OBJECT_CLASS (unity_launcher_parent_class)->dispose (object);
}

static void
unity_launcher_class_init (UnityLauncherClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose = unity_launcher_dispose;

  /* Ensure the custom children are registered before the template is parsed. */
  g_type_ensure (UNITY_TYPE_LAUNCHER_APP_STRIP);
  g_type_ensure (UNITY_TYPE_LAUNCHER_DASH_BUTTON);

  gtk_widget_class_set_template_from_resource (
    widget_class, "/org/unity/launcher/unity-launcher.ui");
  gtk_widget_class_bind_template_child (widget_class, UnityLauncher, content);
  gtk_widget_class_bind_template_child (widget_class, UnityLauncher, app_strip);

  gtk_widget_class_set_css_name (widget_class, "unity-launcher");
}

static void
unity_launcher_init (UnityLauncher *self)
{
  self->settings = unity_settings_get_default ();

  gtk_widget_init_template (GTK_WIDGET (self));

  apply_position (self);
  g_signal_connect_object (self->settings, "changed::" UNITY_LAUNCHER_KEY_POSITION,
                           G_CALLBACK (apply_position), self, G_CONNECT_SWAPPED);

  self->hide = unity_hide_new (ASTAL_WINDOW (self));

  g_signal_connect_object (self->app_strip, "notify::dragging",
                           G_CALLBACK (on_strip_dragging), self, G_CONNECT_DEFAULT);
  g_signal_connect_object (self->app_strip, "notify::menu-shown",
                           G_CALLBACK (on_strip_menu_shown), self, G_CONNECT_DEFAULT);
}

static void
ensure_style (void)
{
  static gsize done = 0;

  if (g_once_init_enter (&done))
    {
      g_autoptr (GtkCssProvider) provider = gtk_css_provider_new ();

      gtk_css_provider_load_from_resource (provider, "/org/unity/launcher/launcher.css");
      gtk_style_context_add_provider_for_display (
        gdk_display_get_default (), GTK_STYLE_PROVIDER (provider),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

      g_once_init_leave (&done, 1);
    }
}

UnityLauncher *
unity_launcher_new (GtkApplication *app)
{
  ensure_style ();
  return g_object_new (UNITY_TYPE_LAUNCHER, "application", app, NULL);
}
