/* unity-launcher-dash-button.c
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

#include "components/unity-launcher-dash-button.h"

#include "unity-settings.h"

typedef enum
{
  PROP_ICON_SIZE = 1,
} UnityLauncherDashButtonProperty;
static GParamSpec *properties[PROP_ICON_SIZE + 1];

struct _UnityLauncherDashButton
{
  GtkButton  parent_instance;

  GtkImage  *image;
  gint       icon_size;
};

G_DEFINE_FINAL_TYPE (UnityLauncherDashButton, unity_launcher_dash_button, GTK_TYPE_BUTTON)

/* A square the launcher icon size, with the glyph at half that. The button's own
 * padding lifts it to the tile footprint. */
static void
sync_size (UnityLauncherDashButton *self)
{
  gtk_widget_set_size_request (GTK_WIDGET (self->image), self->icon_size, self->icon_size);
  gtk_image_set_pixel_size (self->image, self->icon_size / 2);
}

static void
unity_launcher_dash_button_set_property (GObject *object, guint id, const GValue *value,
                                         GParamSpec *pspec)
{
  UnityLauncherDashButton *self = UNITY_LAUNCHER_DASH_BUTTON (object);
  switch ((UnityLauncherDashButtonProperty) id)
    {
    case PROP_ICON_SIZE:
      self->icon_size = g_value_get_int (value);
      sync_size (self);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, id, pspec);
    }
}

static void
unity_launcher_dash_button_get_property (GObject *object, guint id, GValue *value,
                                         GParamSpec *pspec)
{
  UnityLauncherDashButton *self = UNITY_LAUNCHER_DASH_BUTTON (object);
  switch ((UnityLauncherDashButtonProperty) id)
    {
    case PROP_ICON_SIZE: g_value_set_int (value, self->icon_size); break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, id, pspec);
    }
}

static void
unity_launcher_dash_button_dispose (GObject *object)
{
  gtk_widget_dispose_template (GTK_WIDGET (object), UNITY_TYPE_LAUNCHER_DASH_BUTTON);

  G_OBJECT_CLASS (unity_launcher_dash_button_parent_class)->dispose (object);
}

static void
unity_launcher_dash_button_class_init (UnityLauncherDashButtonClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose      = unity_launcher_dash_button_dispose;
  object_class->set_property = unity_launcher_dash_button_set_property;
  object_class->get_property = unity_launcher_dash_button_get_property;

  properties[PROP_ICON_SIZE] = g_param_spec_int (
    "icon-size", NULL, NULL, 1, G_MAXINT, 48,
    G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_properties (object_class, G_N_ELEMENTS (properties), properties);

  gtk_widget_class_set_template_from_resource (
    widget_class, "/org/unity/launcher/components/unity-launcher-dash-button.ui");
  gtk_widget_class_bind_template_child (widget_class, UnityLauncherDashButton, image);
}

static void
unity_launcher_dash_button_init (UnityLauncherDashButton *self)
{
  self->icon_size = 48;

  gtk_widget_init_template (GTK_WIDGET (self));

  /* The launcher binds "launcher-icon-size" to icon-size, so the button stays
   * in sync with the tiles beside it via one path. */
  g_settings_bind (unity_settings_get_default (), UNITY_LAUNCHER_KEY_LAUNCHER_ICON_SIZE,
                   self, "icon-size", G_SETTINGS_BIND_GET);
}
