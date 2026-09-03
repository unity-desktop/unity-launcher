/* unity-app-tile.c
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

#include "components/unity-app-tile.h"

#include <gio/gdesktopappinfo.h>

#include "components/unity-desktop-actions.h"
#include "unity-app-catalog.h"
#include "components/unity-pinned-apps.h"
#include "unity-settings.h"

#define ICON_SIZE_DEFAULT 48

static void unity_app_tile_set_gicon (UnityAppTile *self, GIcon *icon);

typedef struct
{
  AstalAppsApplication *app;
  GtkBox          *box;
  GtkImage        *image;
  GtkPopoverMenu  *popover;
  gint             icon_size;
  GtkPositionType  menu_position;
  gboolean         menu_shown;
} UnityAppTilePrivate;

G_DEFINE_ABSTRACT_TYPE_WITH_PRIVATE (UnityAppTile, unity_app_tile, GTK_TYPE_BUTTON)

typedef enum
{
  PROP_APPLICATION = 1,
  PROP_ICON_SIZE,
  PROP_MENU_SHOWN,
} UnityAppTileProperty;
static GParamSpec *properties[PROP_MENU_SHOWN + 1];

static void
sync_icon (UnityAppTile *self)
{
  UnityAppTilePrivate *priv      = unity_app_tile_get_instance_private (self);
  const gchar      *icon_name = priv->app ? astal_apps_application_get_icon_name (priv->app)
                                          : NULL;

  if (icon_name == NULL || *icon_name == '\0')
    {
      unity_app_tile_set_gicon (self, NULL);
      return;
    }

  g_autoptr (GIcon) icon = g_icon_new_for_string (icon_name, NULL);
  unity_app_tile_set_gicon (self, icon);
}

static const gchar *
entry_id (UnityAppTile *self)
{
  UnityAppTilePrivate *priv = unity_app_tile_get_instance_private (self);
  return priv->app ? astal_apps_application_get_entry (priv->app) : NULL;
}

static void
tile_launch (GtkWidget *widget, const gchar *action_name, GVariant *param)
{
  (void) action_name; (void) param;
  unity_app_tile_launch (UNITY_APP_TILE (widget));
}

static void
tile_launch_action (GtkWidget *widget, const gchar *action_name, GVariant *param)
{
  (void) action_name;
  UnityAppTilePrivate *priv = unity_app_tile_get_instance_private (UNITY_APP_TILE (widget));
  GDesktopAppInfo     *info = priv->app ? astal_apps_application_get_app (priv->app) : NULL;

  if (info != NULL)
    {
      GdkDisplay *display = gdk_display_get_default ();
      g_autoptr (GdkAppLaunchContext) ctx = display
        ? gdk_display_get_app_launch_context (display) : NULL;
      if (ctx != NULL)
        gdk_app_launch_context_set_timestamp (ctx, GDK_CURRENT_TIME);

      g_desktop_app_info_launch_action (info, g_variant_get_string (param, NULL),
                                        ctx ? G_APP_LAUNCH_CONTEXT (ctx) : NULL);
      astal_apps_application_set_frequency (priv->app,
                                            astal_apps_application_get_frequency (priv->app) + 1);
    }
}

static void
tile_pin_toggle (GtkWidget *widget, const gchar *action_name, GVariant *param)
{
  (void) action_name; (void) param;
  unity_pinned_apps_toggle (unity_settings_get_default (),
                            entry_id (UNITY_APP_TILE (widget)));
}

static void
append_app_sections (UnityAppTile *self, GMenu *menu)
{
  UnityAppTilePrivate *priv = unity_app_tile_get_instance_private (self);
  if (priv->app == NULL)
    return;

  g_autoptr (GMenu) section = g_menu_new ();
  g_menu_append (section, "Open", "tile.launch");
  g_menu_append_section (menu, NULL, G_MENU_MODEL (section));

  unity_desktop_actions_append (menu, astal_apps_application_get_app (priv->app),
                                "tile.launch-action");

  gboolean pinned = unity_pinned_apps_contains (unity_settings_get_default (),
                                                entry_id (self));
  g_autoptr (GMenu) pin = g_menu_new ();
  g_menu_append (pin, pinned ? "Unpin from Launcher" : "Pin to Launcher",
                 "tile.pin-toggle");
  g_menu_append_section (menu, NULL, G_MENU_MODEL (pin));
}

static void
on_menu_closed (GtkPopover *popover, gpointer data)
{
  (void) popover;
  UnityAppTile        *self = data;
  UnityAppTilePrivate *priv = unity_app_tile_get_instance_private (self);

  if (!priv->menu_shown)
    return;
  priv->menu_shown = FALSE;
  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_MENU_SHOWN]);
}

static void
present_menu (UnityAppTile *self)
{
  UnityAppTilePrivate       *priv = unity_app_tile_get_instance_private (self);
  UnityAppTileClass         *klass = UNITY_APP_TILE_GET_CLASS (self);

  g_autoptr (GMenu) menu = g_menu_new ();
  append_app_sections (self, menu);
  if (klass->populate_menu != NULL)
    klass->populate_menu (self, menu);
  if (g_menu_model_get_n_items (G_MENU_MODEL (menu)) == 0)
    return;

  if (priv->popover == NULL)
    {
      priv->popover = GTK_POPOVER_MENU (gtk_popover_menu_new_from_model (NULL));
      gtk_widget_set_parent (GTK_WIDGET (priv->popover), GTK_WIDGET (self));
      gtk_popover_set_has_arrow (GTK_POPOVER (priv->popover), FALSE);
      gtk_popover_set_position (GTK_POPOVER (priv->popover), priv->menu_position);
      gtk_widget_add_css_class (GTK_WIDGET (priv->popover), "body");
      g_signal_connect_object (priv->popover, "closed", G_CALLBACK (on_menu_closed),
                               self, G_CONNECT_DEFAULT);
    }
  gtk_popover_menu_set_menu_model (priv->popover, G_MENU_MODEL (menu));

  GdkRectangle rect = {
    0, 0, gtk_widget_get_width (GTK_WIDGET (self)), gtk_widget_get_height (GTK_WIDGET (self)),
  };
  gtk_popover_set_pointing_to (GTK_POPOVER (priv->popover), &rect);
  gtk_popover_popup (GTK_POPOVER (priv->popover));

  if (!priv->menu_shown)
    {
      priv->menu_shown = TRUE;
      g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_MENU_SHOWN]);
    }
}

static void
on_secondary_pressed (GtkGestureClick *gesture, gint n_press, gdouble x, gdouble y,
                      gpointer user_data)
{
  (void) n_press; (void) x; (void) y;
  gtk_gesture_set_state (GTK_GESTURE (gesture), GTK_EVENT_SEQUENCE_CLAIMED);
  present_menu (UNITY_APP_TILE (user_data));
}

GtkBox *
unity_app_tile_get_box (UnityAppTile *self)
{
  UnityAppTilePrivate *priv;
  g_return_val_if_fail (UNITY_IS_APP_TILE (self), NULL);
  priv = unity_app_tile_get_instance_private (self);
  return priv->box;
}

gint
unity_app_tile_get_icon_size (UnityAppTile *self)
{
  UnityAppTilePrivate *priv;
  g_return_val_if_fail (UNITY_IS_APP_TILE (self), ICON_SIZE_DEFAULT);
  priv = unity_app_tile_get_instance_private (self);
  return priv->icon_size;
}

void
unity_app_tile_set_gicon (UnityAppTile *self, GIcon *icon)
{
  g_return_if_fail (UNITY_IS_APP_TILE (self));
  UnityAppTilePrivate *priv = unity_app_tile_get_instance_private (self);
  if (icon != NULL)
    gtk_image_set_from_gicon (priv->image, icon);
  else
    gtk_image_set_from_icon_name (priv->image, "application-x-executable-symbolic");
}

void
unity_app_tile_set_application (UnityAppTile *self, AstalAppsApplication *app)
{
  g_return_if_fail (UNITY_IS_APP_TILE (self));
  UnityAppTilePrivate *priv = unity_app_tile_get_instance_private (self);

  if (!g_set_object (&priv->app, app))
    return;

  sync_icon (self);
  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_APPLICATION]);
}

AstalAppsApplication *
unity_app_tile_get_application (UnityAppTile *self)
{
  UnityAppTilePrivate *priv;
  g_return_val_if_fail (UNITY_IS_APP_TILE (self), NULL);
  priv = unity_app_tile_get_instance_private (self);
  return priv->app;
}

void
unity_app_tile_launch (UnityAppTile *self)
{
  g_return_if_fail (UNITY_IS_APP_TILE (self));
  UnityAppTilePrivate *priv = unity_app_tile_get_instance_private (self);

  if (priv->app == NULL)
    return;

  unity_app_catalog_launch (priv->app);
}

void
unity_app_tile_set_menu_position (UnityAppTile *self, GtkPositionType position)
{
  g_return_if_fail (UNITY_IS_APP_TILE (self));
  UnityAppTilePrivate *priv = unity_app_tile_get_instance_private (self);
  priv->menu_position = position;
  if (priv->popover != NULL)
    gtk_popover_set_position (GTK_POPOVER (priv->popover), position);
}

gboolean
unity_app_tile_get_menu_shown (UnityAppTile *self)
{
  UnityAppTilePrivate *priv;
  g_return_val_if_fail (UNITY_IS_APP_TILE (self), FALSE);
  priv = unity_app_tile_get_instance_private (self);
  return priv->menu_shown;
}

static void
unity_app_tile_get_property (GObject *object, guint id, GValue *value, GParamSpec *pspec)
{
  UnityAppTilePrivate *priv = unity_app_tile_get_instance_private (UNITY_APP_TILE (object));
  switch ((UnityAppTileProperty) id)
    {
    case PROP_APPLICATION: g_value_set_object  (value, priv->app);        break;
    case PROP_ICON_SIZE:  g_value_set_int     (value, priv->icon_size);  break;
    case PROP_MENU_SHOWN: g_value_set_boolean (value, priv->menu_shown); break;
    default: G_OBJECT_WARN_INVALID_PROPERTY_ID (object, id, pspec);
    }
}

static void
unity_app_tile_set_property (GObject *object, guint id, const GValue *value, GParamSpec *pspec)
{
  UnityAppTilePrivate *priv = unity_app_tile_get_instance_private (UNITY_APP_TILE (object));
  switch ((UnityAppTileProperty) id)
    {
    case PROP_APPLICATION:
      unity_app_tile_set_application (UNITY_APP_TILE (object), g_value_get_object (value));
      break;
    case PROP_ICON_SIZE:
      {
        gint n = g_value_get_int (value);
        if (priv->icon_size != n)
          {
            priv->icon_size = n;
            gtk_image_set_pixel_size (priv->image, n);
            g_object_notify_by_pspec (object, pspec);
          }
        break;
      }
    default: G_OBJECT_WARN_INVALID_PROPERTY_ID (object, id, pspec);
    }
}

static void
unity_app_tile_dispose (GObject *object)
{
  UnityAppTilePrivate *priv = unity_app_tile_get_instance_private (UNITY_APP_TILE (object));

  g_clear_object (&priv->app);

  if (priv->popover != NULL)
    {
      GtkWidget *popover = GTK_WIDGET (priv->popover);
      priv->popover = NULL;
      gtk_widget_unparent (popover);
    }

  G_OBJECT_CLASS (unity_app_tile_parent_class)->dispose (object);
}

static void
unity_app_tile_class_init (UnityAppTileClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose      = unity_app_tile_dispose;
  object_class->get_property = unity_app_tile_get_property;
  object_class->set_property = unity_app_tile_set_property;

  gtk_widget_class_install_action (GTK_WIDGET_CLASS (klass),
                                   "tile.launch",        NULL, tile_launch);
  gtk_widget_class_install_action (GTK_WIDGET_CLASS (klass),
                                   "tile.launch-action", "s",  tile_launch_action);
  gtk_widget_class_install_action (GTK_WIDGET_CLASS (klass),
                                   "tile.pin-toggle",    NULL, tile_pin_toggle);

  properties[PROP_APPLICATION] = g_param_spec_object (
    "application", NULL, NULL, ASTAL_APPS_TYPE_APPLICATION,
    G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);
  properties[PROP_ICON_SIZE] = g_param_spec_int (
    "icon-size", NULL, NULL, 1, G_MAXINT, ICON_SIZE_DEFAULT,
    G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);
  properties[PROP_MENU_SHOWN] = g_param_spec_boolean (
    "menu-shown", NULL, NULL, FALSE,
    G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, G_N_ELEMENTS (properties), properties);
}

static void
unity_app_tile_init (UnityAppTile *self)
{
  UnityAppTilePrivate *priv = unity_app_tile_get_instance_private (self);

  priv->icon_size     = ICON_SIZE_DEFAULT;
  priv->menu_position = GTK_POS_BOTTOM;

  gtk_widget_add_css_class (GTK_WIDGET (self), "flat");

  priv->box = GTK_BOX (gtk_box_new (GTK_ORIENTATION_VERTICAL, 1));
  gtk_widget_set_halign (GTK_WIDGET (priv->box), GTK_ALIGN_CENTER);
  gtk_widget_set_valign (GTK_WIDGET (priv->box), GTK_ALIGN_CENTER);

  priv->image = GTK_IMAGE (gtk_image_new ());
  gtk_widget_set_halign (GTK_WIDGET (priv->image), GTK_ALIGN_CENTER);
  gtk_image_set_pixel_size (priv->image, priv->icon_size);
  gtk_widget_add_css_class (GTK_WIDGET (priv->image), "icon-dropshadow");
  gtk_widget_add_css_class (GTK_WIDGET (priv->image), "lowres-icon");
  gtk_box_append (priv->box, GTK_WIDGET (priv->image));

  gtk_button_set_child (GTK_BUTTON (self), GTK_WIDGET (priv->box));

  GtkGesture *gesture = gtk_gesture_click_new ();
  gtk_gesture_single_set_button (GTK_GESTURE_SINGLE (gesture), GDK_BUTTON_SECONDARY);
  g_signal_connect (gesture, "pressed", G_CALLBACK (on_secondary_pressed), self);
  gtk_widget_add_controller (GTK_WIDGET (self), GTK_EVENT_CONTROLLER (gesture));
}
