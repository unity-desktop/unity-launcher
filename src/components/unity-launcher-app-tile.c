/* unity-launcher-app-tile.c
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

#include "components/unity-launcher-app-tile.h"

#include <math.h>

#include <adwaita.h>
#include <astal-wayfire.h>
#include <astal-wlr.h>
#include <gdk/wayland/gdkwayland.h>
#include <graphene.h>

#include "components/unity-launcher-app-strip.h"

#define FOOTPRINT_PADDING     4

#define DND_OPACITY_ANIM_KEY  "dnd-opacity-anim"
#define DND_FADE_OUT_MS       150
#define DND_FADE_IN_MS        200

struct _UnityLauncherAppTile
{
  UnityAppTile      parent_instance;

  UnityAppEntry *entry;
  GtkWidget     *dot;
  gboolean       dragging;
};

typedef enum
{
  PROP_DRAGGING = 1,
} UnityLauncherAppTileProperty;
static GParamSpec *properties[PROP_DRAGGING + 1];

G_DEFINE_FINAL_TYPE (UnityLauncherAppTile, unity_launcher_app_tile, UNITY_TYPE_APP_TILE)

static void
sync_tooltip (UnityLauncherAppTile *self)
{
  AstalAppsApplication *app = self->entry ? unity_app_entry_get_application (self->entry)
                                        : NULL;

  gtk_widget_set_tooltip_text (GTK_WIDGET (self),
                               app ? astal_apps_application_get_name (app) : NULL);
}

static void
sync_footprint (UnityLauncherAppTile *self)
{
  gint side = unity_app_tile_get_icon_size (UNITY_APP_TILE (self)) + FOOTPRINT_PADDING;
  gtk_widget_set_size_request (
    GTK_WIDGET (unity_app_tile_get_box (UNITY_APP_TILE (self))), side, side);
}

static void
set_state_class (UnityLauncherAppTile *self, const gchar *name, gboolean on)
{
  if (on)
    gtk_widget_add_css_class (GTK_WIDGET (self), name);
  else
    gtk_widget_remove_css_class (GTK_WIDGET (self), name);
}

static void
sync_running (UnityLauncherAppTile *self)
{
  gboolean running = self->entry && unity_app_entry_get_running (self->entry);

  gtk_widget_set_opacity (self->dot, running ? 1.0 : 0.0);
  set_state_class (self, "running", running);
  gtk_widget_action_set_enabled (GTK_WIDGET (self), "tile.quit", running);
}

static void
sync_active (UnityLauncherAppTile *self)
{
  set_state_class (self, "active",
                   self->entry && unity_app_entry_get_activated (self->entry));
}

static void
push_rectangle_hints (UnityLauncherAppTile *self)
{
  if (self->entry == NULL || !gtk_widget_get_mapped (GTK_WIDGET (self)))
    return;

  GListModel *toplevels = unity_app_entry_get_toplevels (self->entry);
  guint n = toplevels ? g_list_model_get_n_items (toplevels) : 0;
  if (n == 0)
    return;

  GtkWidget  *widget = GTK_WIDGET (self);
  GtkRoot    *root   = gtk_widget_get_root (widget);
  if (root == NULL || !GTK_IS_NATIVE (root))
    return;
  GdkSurface *gsurface = gtk_native_get_surface (GTK_NATIVE (root));
  if (gsurface == NULL || !GDK_IS_WAYLAND_SURFACE (gsurface))
    return;
  struct wl_surface *wsurface = gdk_wayland_surface_get_wl_surface (gsurface);
  if (wsurface == NULL)
    return;

  graphene_rect_t bounds;
  if (!gtk_widget_compute_bounds (widget, GTK_WIDGET (root), &bounds) ||
      bounds.size.width <= 0 || bounds.size.height <= 0)
    return;

  gint x = (int) lroundf (bounds.origin.x);
  gint y = (int) lroundf (bounds.origin.y);
  gint w = (int) lroundf (bounds.size.width);
  gint h = (int) lroundf (bounds.size.height);

  for (guint i = 0; i < n; i++)
    {
      g_autoptr (AstalWlrToplevel) tl = g_list_model_get_item (toplevels, i);
      if (tl != NULL)
        astal_wlr_toplevel_set_rectangle (tl, wsurface, x, y, w, h);
    }
}

static void
clear_rectangle_hints (UnityLauncherAppTile *self)
{
  if (self->entry == NULL)
    return;

  GtkWidget  *widget   = GTK_WIDGET (self);
  GtkRoot    *root     = gtk_widget_get_root (widget);
  GdkSurface *gsurface = root ? gtk_native_get_surface (GTK_NATIVE (root)) : NULL;
  if (gsurface == NULL || !GDK_IS_WAYLAND_SURFACE (gsurface))
    return;
  struct wl_surface *wsurface = gdk_wayland_surface_get_wl_surface (gsurface);
  if (wsurface == NULL)
    return;

  GListModel *toplevels = unity_app_entry_get_toplevels (self->entry);
  guint n = toplevels ? g_list_model_get_n_items (toplevels) : 0;
  for (guint i = 0; i < n; i++)
    {
      g_autoptr (AstalWlrToplevel) tl = g_list_model_get_item (toplevels, i);
      if (tl != NULL)
        astal_wlr_toplevel_set_rectangle (tl, wsurface, 0, 0, 0, 0);
    }
}

static void
on_tile_map (GtkWidget *widget, gpointer data)
{
  (void) data;
  push_rectangle_hints (UNITY_LAUNCHER_APP_TILE (widget));
}

static void
on_tile_unmap (GtkWidget *widget, gpointer data)
{
  (void) data;
  clear_rectangle_hints (UNITY_LAUNCHER_APP_TILE (widget));
}

static void
on_toplevels_items_changed (GListModel *model, guint position, guint removed,
                            guint added, gpointer data)
{
  (void) model; (void) position; (void) removed;
  if (added > 0)
    push_rectangle_hints (UNITY_LAUNCHER_APP_TILE (data));
}

static void
spread_app_windows (UnityLauncherAppTile *self)
{
  GListModel *toplevels = unity_app_entry_get_toplevels (self->entry);
  guint       n         = toplevels ? g_list_model_get_n_items (toplevels) : 0;

  g_autoptr (GPtrArray) ids = g_ptr_array_new_with_free_func (g_free);
  g_autoptr (GHashTable) seen = g_hash_table_new (g_str_hash, g_str_equal);
  for (guint i = 0; i < n; i++)
    {
      g_autoptr (AstalWlrToplevel) tl = g_list_model_get_item (toplevels, i);
      const gchar *aid = tl ? astal_wlr_toplevel_get_app_id (tl) : NULL;
      if (aid == NULL || *aid == '\0' || g_hash_table_contains (seen, aid))
        continue;

      gchar *dup = g_strdup (aid);
      g_ptr_array_add (ids, dup);
      g_hash_table_add (seen, dup);
    }

  AstalWayfireSpatial *spatial = astal_wayfire_spatial_get_default ();
  if (spatial != NULL)
    astal_wayfire_spatial_spread_app (spatial, (gchar **) ids->pdata, ids->len);
}

static void
tile_quit (GtkWidget *widget, const gchar *action_name, GVariant *param)
{
  (void) action_name; (void) param;
  UnityLauncherAppTile *self = UNITY_LAUNCHER_APP_TILE (widget);
  if (self->entry != NULL)
    unity_app_entry_close_all (self->entry);
}

static void
on_self_clicked (GtkButton *button, gpointer user_data)
{
  (void) user_data;
  UnityLauncherAppTile *self = UNITY_LAUNCHER_APP_TILE (button);
  if (self->entry == NULL)
    return;

  push_rectangle_hints (self);

  GListModel *toplevels = unity_app_entry_get_toplevels (self->entry);
  if (toplevels != NULL && g_list_model_get_n_items (toplevels) >= 2)
    spread_app_windows (self);
  else
    unity_app_entry_activate_or_launch (self->entry);
}

static void
unity_launcher_app_tile_populate_menu (UnityAppTile *tile, GMenu *menu)
{
  UnityLauncherAppTile *self = UNITY_LAUNCHER_APP_TILE (tile);

  if (self->entry == NULL)
    return;
  const gchar *app_id = unity_app_entry_get_app_id (self->entry);
  if (app_id == NULL || *app_id == '\0')
    return;

  g_autoptr (GMenu) section = g_menu_new ();
  g_menu_append (section, "Quit", "tile.quit");
  g_menu_append_section (menu, NULL, G_MENU_MODEL (section));
}

static GdkContentProvider *
on_drag_prepare (GtkDragSource *source, gdouble x, gdouble y, gpointer user_data)
{
  (void) source; (void) x; (void) y;
  UnityLauncherAppTile *self = UNITY_LAUNCHER_APP_TILE (user_data);
  if (self->entry == NULL)
    return NULL;
  const gchar *app_id = unity_app_entry_get_app_id (self->entry);
  if (app_id == NULL || *app_id == '\0')
    return NULL;
  return gdk_content_provider_new_typed (G_TYPE_STRING, app_id);
}

static void
fade_opacity (GtkWidget *widget, gdouble from, gdouble to, guint ms)
{
  AdwAnimation *old = g_object_get_data (G_OBJECT (widget), DND_OPACITY_ANIM_KEY);
  if (old != NULL)
    adw_animation_reset (old);

  AdwAnimationTarget *tgt = adw_property_animation_target_new (G_OBJECT (widget), "opacity");
  AdwAnimation *anim = adw_timed_animation_new (widget, from, to, ms, tgt);
  adw_timed_animation_set_easing (ADW_TIMED_ANIMATION (anim), ADW_EASE_OUT_QUAD);
  g_object_set_data_full (G_OBJECT (widget), DND_OPACITY_ANIM_KEY, anim, g_object_unref);
  adw_animation_play (anim);
}

static GdkPaintable *
build_drag_paintable (UnityLauncherAppTile *self)
{
  if (self->entry == NULL)
    return NULL;
  GAppInfo *info = unity_app_entry_get_app_info (self->entry);
  GIcon    *icon = info ? g_app_info_get_icon (info) : NULL;
  if (icon == NULL)
    return NULL;

  gint          size  = unity_app_tile_get_icon_size (UNITY_APP_TILE (self));
  gint          scale = gtk_widget_get_scale_factor (GTK_WIDGET (self));
  GtkIconTheme *theme = gtk_icon_theme_get_for_display (gtk_widget_get_display (GTK_WIDGET (self)));

  GtkIconPaintable *paintable = gtk_icon_theme_lookup_by_gicon (
    theme, icon, size, scale, GTK_TEXT_DIR_NONE, GTK_ICON_LOOKUP_FORCE_REGULAR);
  return paintable ? GDK_PAINTABLE (paintable) : NULL;
}

static void
on_drag_begin (GtkDragSource *source, GdkDrag *drag, gpointer user_data)
{
  (void) drag;
  UnityLauncherAppTile *self = UNITY_LAUNCHER_APP_TILE (user_data);
  gint size = unity_app_tile_get_icon_size (UNITY_APP_TILE (self));

  g_autoptr (GdkPaintable) paintable = build_drag_paintable (self);
  if (paintable != NULL)
    gtk_drag_source_set_icon (source, paintable, size / 2, size / 2);

  self->dragging = TRUE;
  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_DRAGGING]);

  fade_opacity (GTK_WIDGET (self), 1.0, 0.0, DND_FADE_OUT_MS);
}

static void
on_drag_end (GtkDragSource *source, GdkDrag *drag, gboolean delete_data, gpointer user_data)
{
  (void) source; (void) drag;
  UnityLauncherAppTile *self = UNITY_LAUNCHER_APP_TILE (user_data);
  GtkWidget *widget = GTK_WIDGET (user_data);

  self->dragging = FALSE;
  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_DRAGGING]);

  if (delete_data)
    {
      g_object_set_data (G_OBJECT (widget), DND_OPACITY_ANIM_KEY, NULL);
      gtk_widget_set_opacity (widget, 1.0);
    }
  else
    {
      fade_opacity (widget, 0.0, 1.0, DND_FADE_IN_MS);
    }
}

static void
group_with_click_gesture (GtkWidget *widget, GtkGesture *drag_gesture)
{
  g_autoptr (GListModel) controllers = gtk_widget_observe_controllers (widget);
  guint n = g_list_model_get_n_items (controllers);
  for (guint i = 0; i < n; i++)
    {
      g_autoptr (GtkEventController) ctrl = g_list_model_get_item (controllers, i);
      if (GTK_IS_GESTURE_CLICK (ctrl))
        {
          gtk_gesture_group (drag_gesture, GTK_GESTURE (ctrl));
          break;
        }
    }
}

static void
install_dnd (UnityLauncherAppTile *self)
{
  GtkDragSource *drag_source = gtk_drag_source_new ();
  gtk_drag_source_set_actions (drag_source, GDK_ACTION_MOVE);
  g_signal_connect_object (drag_source, "prepare",    G_CALLBACK (on_drag_prepare), self, G_CONNECT_DEFAULT);
  g_signal_connect_object (drag_source, "drag-begin", G_CALLBACK (on_drag_begin),   self, G_CONNECT_DEFAULT);
  g_signal_connect_object (drag_source, "drag-end",   G_CALLBACK (on_drag_end),     self, G_CONNECT_DEFAULT);
  gtk_widget_add_controller (GTK_WIDGET (self), GTK_EVENT_CONTROLLER (drag_source));
  group_with_click_gesture (GTK_WIDGET (self), GTK_GESTURE (drag_source));
}

static void
on_entry_running_notify (UnityAppEntry *entry, GParamSpec *pspec, UnityLauncherAppTile *self)
{
  (void) entry; (void) pspec;
  sync_running (self);
}

static void
on_entry_activated_notify (UnityAppEntry *entry, GParamSpec *pspec, UnityLauncherAppTile *self)
{
  (void) entry; (void) pspec;
  sync_active (self);
}


static void
on_icon_size_notify (GObject *object, GParamSpec *pspec, gpointer user_data)
{
  (void) object; (void) pspec;
  sync_footprint (user_data);
}

static void
construct_entry_bindings (UnityLauncherAppTile *self, UnityAppEntry *entry)
{
  if (entry == NULL)
    return;

  self->entry = g_object_ref (entry);
  unity_app_tile_set_application (UNITY_APP_TILE (self),
                              unity_app_entry_get_application (entry));

  g_signal_connect_object (entry, "notify::running",
                           G_CALLBACK (on_entry_running_notify),   self, G_CONNECT_DEFAULT);
  g_signal_connect_object (entry, "notify::activated",
                           G_CALLBACK (on_entry_activated_notify), self, G_CONNECT_DEFAULT);

  GListModel *toplevels = unity_app_entry_get_toplevels (entry);
  g_signal_connect_object (toplevels, "items-changed",
                           G_CALLBACK (on_toplevels_items_changed), self, G_CONNECT_DEFAULT);

  sync_tooltip (self);
  sync_running (self);
  sync_active  (self);
}

GtkWidget *
unity_launcher_app_tile_new (UnityAppEntry *entry)
{
  GtkWidget *self = g_object_new (UNITY_TYPE_LAUNCHER_APP_TILE, NULL);
  construct_entry_bindings (UNITY_LAUNCHER_APP_TILE (self), entry);
  return self;
}

const gchar *
unity_launcher_app_tile_get_app_id (UnityLauncherAppTile *self)
{
  g_return_val_if_fail (UNITY_IS_LAUNCHER_APP_TILE (self), NULL);
  return self->entry ? unity_app_entry_get_app_id (self->entry) : NULL;
}

gboolean
unity_launcher_app_tile_get_pinned (UnityLauncherAppTile *self)
{
  g_return_val_if_fail (UNITY_IS_LAUNCHER_APP_TILE (self), FALSE);
  return self->entry ? unity_app_entry_get_pinned (self->entry) : FALSE;
}

gboolean
unity_launcher_app_tile_get_dragging (UnityLauncherAppTile *self)
{
  g_return_val_if_fail (UNITY_IS_LAUNCHER_APP_TILE (self), FALSE);
  return self->dragging;
}

static void
unity_launcher_app_tile_dispose (GObject *object)
{
  UnityLauncherAppTile *self = UNITY_LAUNCHER_APP_TILE (object);
  clear_rectangle_hints (self);
  g_clear_object (&self->entry);
  G_OBJECT_CLASS (unity_launcher_app_tile_parent_class)->dispose (object);
}

static void
unity_launcher_app_tile_get_property (GObject *object, guint id, GValue *value, GParamSpec *pspec)
{
  UnityLauncherAppTile *self = UNITY_LAUNCHER_APP_TILE (object);
  switch ((UnityLauncherAppTileProperty) id)
    {
    case PROP_DRAGGING: g_value_set_boolean (value, self->dragging); break;
    default: G_OBJECT_WARN_INVALID_PROPERTY_ID (object, id, pspec);
    }
}

static void
unity_launcher_app_tile_class_init (UnityLauncherAppTileClass *klass)
{
  GObjectClass    *object_class = G_OBJECT_CLASS (klass);
  UnityAppTileClass *tile_class = UNITY_APP_TILE_CLASS (klass);

  object_class->dispose      = unity_launcher_app_tile_dispose;
  object_class->get_property = unity_launcher_app_tile_get_property;

  tile_class->populate_menu = unity_launcher_app_tile_populate_menu;

  gtk_widget_class_install_action (GTK_WIDGET_CLASS (klass),
                                   "tile.quit", NULL, tile_quit);

  properties[PROP_DRAGGING] = g_param_spec_boolean (
    "dragging", NULL, NULL, FALSE,
    G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_properties (object_class, G_N_ELEMENTS (properties), properties);

  gtk_widget_class_set_css_name (GTK_WIDGET_CLASS (klass), "unity-launcher-tile");
}

static void
unity_launcher_app_tile_init (UnityLauncherAppTile *self)
{
  gtk_widget_action_set_enabled (GTK_WIDGET (self), "tile.quit", FALSE);
  self->dot = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_widget_set_halign (self->dot, GTK_ALIGN_CENTER);
  gtk_widget_add_css_class (self->dot, "running-dot");
  gtk_box_append (unity_app_tile_get_box (UNITY_APP_TILE (self)), self->dot);

  sync_footprint (self);
  install_dnd (self);

  g_signal_connect (self, "clicked", G_CALLBACK (on_self_clicked), NULL);
  g_signal_connect (self, "notify::icon-size", G_CALLBACK (on_icon_size_notify), self);
  g_signal_connect (self, "map",   G_CALLBACK (on_tile_map),   NULL);
  g_signal_connect (self, "unmap", G_CALLBACK (on_tile_unmap), NULL);
}
