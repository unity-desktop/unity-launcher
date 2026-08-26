/* unity-launcher-app-strip.c
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

#include "components/unity-launcher-app-strip.h"

#include <adwaita.h>

#include "components/unity-launcher-app-tile.h"
#include "components/unity-pinned-apps-reorder.h"
#include "components/unity-pinned-apps.h"
#include "unity-app-entry.h"
#include "unity-app-list.h"
#include "unity-position.h"
#include "unity-settings.h"

#define SCROLL_MS 200

struct _UnityLauncherAppStrip
{
  GtkWidget               parent_instance;

  GtkScrolledWindow      *scroller;
  GtkBox                 *strip;

  GSettings              *settings;  /* borrowed shared settings */
  UnityAppList           *apps;
  UnityPinnedAppsReorder *reorder;
  AdwAnimation           *scroll;

  GtkOrientation          orientation;
  gboolean                dragging;
  gboolean                menu_shown;
};

typedef enum
{
  PROP_ORIENTATION = 1,
  PROP_DRAGGING,
  PROP_MENU_SHOWN,
} UnityLauncherAppStripProperty;
static GParamSpec *properties[PROP_MENU_SHOWN + 1];

G_DEFINE_FINAL_TYPE_WITH_CODE (UnityLauncherAppStrip, unity_launcher_app_strip, GTK_TYPE_WIDGET,
                               G_IMPLEMENT_INTERFACE (GTK_TYPE_ORIENTABLE, NULL))

static inline gboolean
is_horizontal (UnityLauncherAppStrip *self)
{
  return self->orientation == GTK_ORIENTATION_HORIZONTAL;
}

static void
strip_reorder_pinned (GtkWidget *widget, const gchar *action_name, GVariant *param)
{
  (void) action_name;
  UnityLauncherAppStrip *self = UNITY_LAUNCHER_APP_STRIP (widget);
  if (param == NULL)
    return;

  const gchar *app_id = NULL;
  gint32       dest   = 0;
  g_variant_get (param, "(&si)", &app_id, &dest);
  unity_pinned_apps_insert (self->settings, app_id, dest);
}

static gboolean
is_tile (GtkWidget *w)
{
  return w != NULL && UNITY_IS_LAUNCHER_APP_TILE (w);
}

static GtkPositionType
current_menu_side (UnityLauncherAppStrip *self)
{
  UnityPosition pos = g_settings_get_enum (self->settings, UNITY_LAUNCHER_KEY_POSITION);
  return unity_position_menu_side (pos);
}

static void
apply_menu_side (UnityLauncherAppStrip *self)
{
  GtkPositionType side = current_menu_side (self);

  for (GtkWidget *c = gtk_widget_get_first_child (GTK_WIDGET (self->strip));
       c != NULL; c = gtk_widget_get_next_sibling (c))
    if (is_tile (c))
      unity_app_tile_set_menu_position (UNITY_APP_TILE (c), side);
}

static void
apply_alignment (UnityLauncherAppStrip *self)
{
  GtkAlign align = unity_tile_alignment_to_align (
    g_settings_get_enum (self->settings, UNITY_LAUNCHER_KEY_TILE_ALIGNMENT));
  if (is_horizontal (self))
    gtk_widget_set_halign (GTK_WIDGET (self->strip), align);
  else
    gtk_widget_set_valign (GTK_WIDGET (self->strip), align);
}

static void
apply_orientation (UnityLauncherAppStrip *self)
{
  gboolean horiz = is_horizontal (self);

  gtk_orientable_set_orientation (GTK_ORIENTABLE (self->strip), self->orientation);

  gtk_scrolled_window_set_policy (self->scroller,
                                  horiz ? GTK_POLICY_EXTERNAL : GTK_POLICY_NEVER,
                                  horiz ? GTK_POLICY_NEVER : GTK_POLICY_EXTERNAL);
  gtk_scrolled_window_set_propagate_natural_width (self->scroller, !horiz);
  gtk_scrolled_window_set_propagate_natural_height (self->scroller, horiz);
  gtk_widget_set_hexpand (GTK_WIDGET (self), horiz);
  gtk_widget_set_vexpand (GTK_WIDGET (self), !horiz);

  /* The alignment axis flipped, so clear both axes then set the new main one. */
  gtk_widget_set_halign (GTK_WIDGET (self->strip), GTK_ALIGN_FILL);
  gtk_widget_set_valign (GTK_WIDGET (self->strip), GTK_ALIGN_FILL);
  apply_alignment (self);
}

static GtkWidget *
nth_tile (UnityLauncherAppStrip *self, gint n)
{
  for (GtkWidget *c = gtk_widget_get_first_child (GTK_WIDGET (self->strip));
       c != NULL; c = gtk_widget_get_next_sibling (c))
    if (is_tile (c) && n-- == 0)
      return c;
  return NULL;
}

static void
set_dragging (UnityLauncherAppStrip *self, gboolean dragging)
{
  if (self->dragging == dragging)
    return;

  self->dragging = dragging;
  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_DRAGGING]);
}

static void
set_menu_shown (UnityLauncherAppStrip *self, gboolean menu_shown)
{
  if (self->menu_shown == menu_shown)
    return;

  self->menu_shown = menu_shown;
  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_MENU_SHOWN]);
}

static void
on_tile_dragging (GObject *tile, GParamSpec *pspec, gpointer user_data)
{
  (void) pspec;
  set_dragging (user_data, unity_launcher_app_tile_get_dragging (UNITY_LAUNCHER_APP_TILE (tile)));
}

static void
on_tile_menu_shown (GObject *tile, GParamSpec *pspec, gpointer user_data)
{
  (void) pspec;
  set_menu_shown (user_data, unity_app_tile_get_menu_shown (UNITY_APP_TILE (tile)));
}

static void
reveal_tile (UnityLauncherAppStrip *self, GtkWidget *tile)
{
  graphene_rect_t bounds;
  if (!gtk_widget_compute_bounds (tile, GTK_WIDGET (self->strip), &bounds))
    return;

  gboolean horiz = is_horizontal (self);
  gdouble  start = horiz ? bounds.origin.x   : bounds.origin.y;
  gdouble  size  = horiz ? bounds.size.width : bounds.size.height;
  if (size <= 0)
    return;

  /* Finish any scroll still running, so the next one starts where the strip is. */
  if (self->scroll != NULL)
    {
      adw_animation_skip (self->scroll);
      g_clear_object (&self->scroll);
    }

  GtkAdjustment *adj = horiz ? gtk_scrolled_window_get_hadjustment (self->scroller)
                             : gtk_scrolled_window_get_vadjustment (self->scroller);
  gdouble value  = gtk_adjustment_get_value (adj);
  gdouble page   = gtk_adjustment_get_page_size (adj);
  gdouble target = value;

  if (start < value)
    target = start;
  else if (start + size > value + page)
    target = start + size - page;

  if (target == value)
    return;

  AdwAnimationTarget *t = adw_property_animation_target_new (G_OBJECT (adj), "value");
  self->scroll = adw_timed_animation_new (GTK_WIDGET (self), value, target, SCROLL_MS, t);
  adw_timed_animation_set_easing (ADW_TIMED_ANIMATION (self->scroll), ADW_EASE_OUT_CUBIC);
  adw_animation_play (self->scroll);
}

static void
on_entry_activated (GtkWidget *tile, GParamSpec *pspec, UnityAppEntry *entry)
{
  (void) pspec;
  GtkWidget *strip = gtk_widget_get_ancestor (tile, UNITY_TYPE_LAUNCHER_APP_STRIP);

  if (strip != NULL && unity_app_entry_get_activated (entry))
    reveal_tile (UNITY_LAUNCHER_APP_STRIP (strip), tile);
}

static GtkWidget *
create_launcher_tile (UnityLauncherAppStrip *self, UnityAppEntry *entry)
{
  GtkWidget *tile = unity_launcher_app_tile_new (entry);
  unity_app_tile_set_menu_position (UNITY_APP_TILE (tile), current_menu_side (self));
  g_settings_bind (self->settings, UNITY_LAUNCHER_KEY_LAUNCHER_ICON_SIZE,
                   tile, "icon-size", G_SETTINGS_BIND_GET);
  g_signal_connect (tile, "notify::dragging",   G_CALLBACK (on_tile_dragging),   self);
  g_signal_connect (tile, "notify::menu-shown", G_CALLBACK (on_tile_menu_shown), self);
  g_signal_connect_object (entry, "notify::activated", G_CALLBACK (on_entry_activated),
                           tile, G_CONNECT_SWAPPED);
  return tile;
}

static void
on_model_items_changed (GListModel *model, guint position, guint removed,
                        guint added, gpointer user_data)
{
  UnityLauncherAppStrip *self = user_data;

  /* Step to the next tile, not the next sibling: the reorder placeholder is a
   * non-tile child of the strip, and would otherwise be removed by a batch of
   * two or more tile removals that span it. */
  GtkWidget *child = nth_tile (self, position);
  for (guint i = 0; i < removed && child != NULL; i++)
    {
      GtkWidget *next = gtk_widget_get_next_sibling (child);
      while (next != NULL && !is_tile (next))
        next = gtk_widget_get_next_sibling (next);
      gtk_box_remove (self->strip, child);
      child = next;
    }

  GtkWidget *after = position > 0 ? nth_tile (self, (gint) position - 1) : NULL;
  for (guint i = 0; i < added; i++)
    {
      g_autoptr (GObject) entry = g_list_model_get_item (model, position + i);
      GtkWidget *tile = create_launcher_tile (self, UNITY_APP_ENTRY (entry));
      gtk_box_insert_child_after (self->strip, tile, after);
      after = tile;
    }
}

static void
sync_pinned_apps (UnityLauncherAppStrip *self)
{
  g_auto (GStrv) ids = g_settings_get_strv (self->settings, UNITY_LAUNCHER_KEY_PINNED_APPS);

  unity_app_list_set_pinned_app_ids (self->apps, (const gchar *const *) ids);
}

gboolean
unity_launcher_app_strip_get_dragging (UnityLauncherAppStrip *self)
{
  g_return_val_if_fail (UNITY_IS_LAUNCHER_APP_STRIP (self), FALSE);
  return self->dragging;
}

gboolean
unity_launcher_app_strip_get_menu_shown (UnityLauncherAppStrip *self)
{
  g_return_val_if_fail (UNITY_IS_LAUNCHER_APP_STRIP (self), FALSE);
  return self->menu_shown;
}

static void
unity_launcher_app_strip_dispose (GObject *object)
{
  UnityLauncherAppStrip *self = UNITY_LAUNCHER_APP_STRIP (object);

  /* Before the template, so the reorder placeholder leaves the strip first. */
  g_clear_object (&self->scroll);
  g_clear_object (&self->reorder);
  g_clear_object (&self->apps);

  gtk_widget_dispose_template (GTK_WIDGET (object), UNITY_TYPE_LAUNCHER_APP_STRIP);

  G_OBJECT_CLASS (unity_launcher_app_strip_parent_class)->dispose (object);
}

static void
unity_launcher_app_strip_get_property (GObject *object, guint id, GValue *value,
                                       GParamSpec *pspec)
{
  UnityLauncherAppStrip *self = UNITY_LAUNCHER_APP_STRIP (object);
  switch ((UnityLauncherAppStripProperty) id)
    {
    case PROP_ORIENTATION: g_value_set_enum    (value, self->orientation); break;
    case PROP_DRAGGING:    g_value_set_boolean (value, self->dragging);    break;
    case PROP_MENU_SHOWN:  g_value_set_boolean (value, self->menu_shown);  break;
    default: G_OBJECT_WARN_INVALID_PROPERTY_ID (object, id, pspec);
    }
}

static void
unity_launcher_app_strip_set_property (GObject *object, guint id, const GValue *value,
                                       GParamSpec *pspec)
{
  UnityLauncherAppStrip *self = UNITY_LAUNCHER_APP_STRIP (object);
  switch ((UnityLauncherAppStripProperty) id)
    {
    case PROP_ORIENTATION:
      self->orientation = g_value_get_enum (value);
      apply_orientation (self);
      g_object_notify (object, "orientation");
      break;
    default: G_OBJECT_WARN_INVALID_PROPERTY_ID (object, id, pspec);
    }
}

static void
unity_launcher_app_strip_class_init (UnityLauncherAppStripClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose      = unity_launcher_app_strip_dispose;
  object_class->get_property = unity_launcher_app_strip_get_property;
  object_class->set_property = unity_launcher_app_strip_set_property;

  properties[PROP_DRAGGING] = g_param_spec_boolean (
    "dragging", NULL, NULL, FALSE, G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);
  properties[PROP_MENU_SHOWN] = g_param_spec_boolean (
    "menu-shown", NULL, NULL, FALSE, G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  g_object_class_install_property (object_class, PROP_DRAGGING, properties[PROP_DRAGGING]);
  g_object_class_install_property (object_class, PROP_MENU_SHOWN, properties[PROP_MENU_SHOWN]);
  g_object_class_override_property (object_class, PROP_ORIENTATION, "orientation");

  gtk_widget_class_install_action (widget_class,
                                   "launcher.reorder-pinned", "(si)", strip_reorder_pinned);

  gtk_widget_class_set_template_from_resource (
    widget_class, "/org/unity/launcher/components/unity-launcher-app-strip.ui");
  gtk_widget_class_bind_template_child (widget_class, UnityLauncherAppStrip, scroller);
  gtk_widget_class_bind_template_child (widget_class, UnityLauncherAppStrip, strip);

  gtk_widget_class_set_layout_manager_type (widget_class, GTK_TYPE_BIN_LAYOUT);
  gtk_widget_class_set_css_name (widget_class, "app-strip");
}

static void
unity_launcher_app_strip_init (UnityLauncherAppStrip *self)
{
  self->settings = unity_settings_get_default ();

  gtk_widget_init_template (GTK_WIDGET (self));

  apply_orientation (self);
  g_signal_connect_object (self->settings, "changed::" UNITY_LAUNCHER_KEY_TILE_ALIGNMENT,
                           G_CALLBACK (apply_alignment), self, G_CONNECT_SWAPPED);
  g_signal_connect_object (self->settings, "changed::" UNITY_LAUNCHER_KEY_POSITION,
                           G_CALLBACK (apply_menu_side), self, G_CONNECT_SWAPPED);

  self->reorder = unity_pinned_apps_reorder_new (self->strip);

  self->apps = unity_app_list_new ();
  g_signal_connect_object (self->apps, "items-changed",
                           G_CALLBACK (on_model_items_changed), self, G_CONNECT_DEFAULT);
  on_model_items_changed (G_LIST_MODEL (self->apps), 0, 0,
                          g_list_model_get_n_items (G_LIST_MODEL (self->apps)), self);

  g_signal_connect_object (self->settings, "changed::" UNITY_LAUNCHER_KEY_PINNED_APPS,
                           G_CALLBACK (sync_pinned_apps), self, G_CONNECT_SWAPPED);
  sync_pinned_apps (self);
}
