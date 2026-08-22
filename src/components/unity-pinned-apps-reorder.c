/* unity-pinned-apps-reorder.c
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

#include "components/unity-pinned-apps-reorder.h"

#include <adwaita.h>

#include "components/unity-launcher-tile.h"
#include "unity-launcher.h"

#define PLACEHOLDER_MS 200

/* Drag gap size and axis, kept on the placeholder so it survives the fade-out. */
typedef struct
{
  gint           main_size;
  gint           cross_size;
  GtkOrientation orientation;
} PlaceholderSlot;

struct _UnityPinnedAppsReorder
{
  GObject       parent_instance;

  GtkBox       *strip;        /* borrowed */
  GtkWidget    *placeholder;  /* child of the strip while a drag is live */
  AdwAnimation *anim;
  gint          index;
};

G_DEFINE_FINAL_TYPE (UnityPinnedAppsReorder, unity_pinned_apps_reorder, G_TYPE_OBJECT)

static GtkOrientation
orientation (UnityPinnedAppsReorder *self)
{
  return gtk_orientable_get_orientation (GTK_ORIENTABLE (self->strip));
}

static gboolean
is_horizontal (UnityPinnedAppsReorder *self)
{
  return orientation (self) == GTK_ORIENTATION_HORIZONTAL;
}

static gboolean
is_tile (GtkWidget *widget)
{
  return widget != NULL && UNITY_IS_LAUNCHER_TILE (widget);
}

static gint
tile_main_extent (UnityPinnedAppsReorder *self, GtkWidget *tile)
{
  return is_horizontal (self) ? gtk_widget_get_width (tile)
                              : gtk_widget_get_height (tile);
}

static gint
tile_cross_extent (UnityPinnedAppsReorder *self, GtkWidget *tile)
{
  return is_horizontal (self) ? gtk_widget_get_height (tile)
                              : gtk_widget_get_width (tile);
}

static gdouble
rect_main_mid (UnityPinnedAppsReorder *self, const graphene_rect_t *b)
{
  return is_horizontal (self) ? b->origin.x + b->size.width / 2.0
                              : b->origin.y + b->size.height / 2.0;
}

static GtkWidget *
nth_tile (UnityPinnedAppsReorder *self, gint n)
{
  for (GtkWidget *c = gtk_widget_get_first_child (GTK_WIDGET (self->strip));
       c != NULL; c = gtk_widget_get_next_sibling (c))
    if (is_tile (c) && n-- == 0)
      return c;
  return NULL;
}

static gint
slot_extent (UnityPinnedAppsReorder *self)
{
  GtkWidget *t = nth_tile (self, 0);
  if (t == NULL)
    return 0;
  gint e = tile_main_extent (self, t);
  if (e <= 0)
    gtk_widget_measure (t, orientation (self), -1, NULL, &e, NULL, NULL);
  return MAX (0, e);
}

static void
anim_cb (gdouble value, gpointer data)
{
  GtkWidget       *ph   = data;
  PlaceholderSlot *slot = g_object_get_data (G_OBJECT (ph), "placeholder-slot");
  gint             along = (gint) (value * slot->main_size + 0.5);

  if (slot->orientation == GTK_ORIENTATION_HORIZONTAL)
    gtk_widget_set_size_request (ph, along, slot->cross_size);
  else
    gtk_widget_set_size_request (ph, slot->cross_size, along);
  gtk_widget_set_opacity (ph, value);
}

static void
remove_placeholder (UnityPinnedAppsReorder *self)
{
  if (self->anim != NULL)
    {
      adw_animation_reset (self->anim);
      g_clear_object (&self->anim);
    }
  if (self->placeholder != NULL)
    {
      gtk_box_remove (self->strip, self->placeholder);
      self->placeholder = NULL;
    }
  self->index = -1;
}

static void
fade_out_done (AdwAnimation *anim, gpointer data)
{
  (void) anim;
  GtkWidget *ph     = data;
  GtkWidget *parent = gtk_widget_get_parent (ph);
  if (parent != NULL)
    gtk_box_remove (GTK_BOX (parent), ph);
}

static void
fade_out_placeholder (UnityPinnedAppsReorder *self)
{
  GtkWidget *ph = self->placeholder;
  if (self->anim != NULL)
    {
      adw_animation_reset (self->anim);
      g_clear_object (&self->anim);
    }
  self->placeholder = NULL;
  self->index       = -1;
  if (ph == NULL)
    return;

  AdwAnimationTarget *t = adw_callback_animation_target_new (anim_cb, ph, NULL);
  AdwAnimation *a = adw_timed_animation_new (ph, gtk_widget_get_opacity (ph), 0.0,
                                             PLACEHOLDER_MS, t);
  adw_timed_animation_set_easing (ADW_TIMED_ANIMATION (a), ADW_EASE_OUT_QUAD);
  g_signal_connect (a, "done", G_CALLBACK (fade_out_done), ph);
  self->anim = a;
  adw_animation_play (a);
}

static void
show_placeholder (UnityPinnedAppsReorder *self, gint idx)
{
  if (idx == self->index)
    return;

  gboolean first = (self->placeholder == NULL);
  remove_placeholder (self);
  if (idx < 0)
    return;

  GtkWidget *t0 = nth_tile (self, 0);

  PlaceholderSlot *slot = g_new0 (PlaceholderSlot, 1);
  slot->main_size   = slot_extent (self);
  slot->cross_size  = t0 != NULL ? tile_cross_extent (self, t0) : -1;
  slot->orientation = orientation (self);

  GtkWidget *ph = gtk_box_new (slot->orientation, 0);
  gtk_widget_add_css_class (ph, "drag-placeholder");
  g_object_set_data_full (G_OBJECT (ph), "placeholder-slot", slot, g_free);

  GtkWidget *after = idx > 0 ? nth_tile (self, idx - 1) : NULL;
  gtk_box_insert_child_after (self->strip, ph, after);
  self->placeholder = ph;
  self->index       = idx;

  if (first)
    {
      anim_cb (0.0, ph);
      AdwAnimationTarget *t = adw_callback_animation_target_new (anim_cb, ph, NULL);
      AdwAnimation *a = adw_timed_animation_new (ph, 0.0, 1.0, PLACEHOLDER_MS, t);
      adw_timed_animation_set_easing (ADW_TIMED_ANIMATION (a), ADW_EASE_OUT_QUAD);
      self->anim = a;
      adw_animation_play (a);
    }
  else
    {
      anim_cb (1.0, ph);
    }
}

static gint
pinned_index_of (UnityPinnedAppsReorder *self, const gchar *app_id)
{
  gint i = 0;
  for (GtkWidget *c = gtk_widget_get_first_child (GTK_WIDGET (self->strip));
       c != NULL; c = gtk_widget_get_next_sibling (c))
    {
      if (!is_tile (c))
        continue;
      if (!unity_launcher_tile_get_pinned (UNITY_LAUNCHER_TILE (c)))
        break;
      if (g_strcmp0 (unity_launcher_tile_get_app_id (UNITY_LAUNCHER_TILE (c)), app_id) == 0)
        return i;
      i++;
    }
  return -1;
}

static gint
pinned_slot_for_coord (UnityPinnedAppsReorder *self, gdouble coord)
{
  gint slot = 0;
  for (GtkWidget *c = gtk_widget_get_first_child (GTK_WIDGET (self->strip));
       c != NULL; c = gtk_widget_get_next_sibling (c))
    {
      if (!is_tile (c))
        continue;
      if (!unity_launcher_tile_get_pinned (UNITY_LAUNCHER_TILE (c)))
        break;
      graphene_rect_t b;
      if (!gtk_widget_compute_bounds (c, GTK_WIDGET (self->strip), &b))
        continue;
      if (coord > rect_main_mid (self, &b))
        slot++;
      else
        break;
    }
  return slot;
}

static gint
slot_to_dest (UnityPinnedAppsReorder *self, gint slot, const gchar *source)
{
  gint src_idx = pinned_index_of (self, source);
  return (src_idx >= 0 && slot > src_idx) ? slot - 1 : slot;
}

static GdkDragAction
on_drag_motion (GtkDropTarget *target, gdouble x, gdouble y, gpointer data)
{
  UnityPinnedAppsReorder *self = data;

  if (self->anim != NULL &&
      adw_animation_get_state (self->anim) == ADW_ANIMATION_PLAYING)
    return GDK_ACTION_MOVE;

  const GValue *value  = gtk_drop_target_get_value (target);
  const gchar  *source = (value != NULL && G_VALUE_HOLDS_STRING (value))
                         ? g_value_get_string (value) : NULL;

  if (source == NULL || pinned_index_of (self, source) < 0)
    {
      show_placeholder (self, -1);
      return 0;
    }

  show_placeholder (self, pinned_slot_for_coord (self, is_horizontal (self) ? x : y));
  return GDK_ACTION_MOVE;
}

static void
on_drag_leave (GtkDropTarget *target, gpointer data)
{
  (void) target;
  fade_out_placeholder (data);
}

static gboolean
on_drop (GtkDropTarget *target, const GValue *value, gdouble x, gdouble y, gpointer data)
{
  (void) target;
  UnityPinnedAppsReorder *self = data;

  const gchar *source = G_VALUE_HOLDS_STRING (value) ? g_value_get_string (value) : NULL;
  if (source == NULL || pinned_index_of (self, source) < 0)
    {
      remove_placeholder (self);
      return FALSE;
    }

  gdouble coord = is_horizontal (self) ? x : y;
  gint slot = self->index >= 0 ? self->index : pinned_slot_for_coord (self, coord);
  gint dest = slot_to_dest (self, slot, source);

  remove_placeholder (self);
  gtk_widget_activate_action (GTK_WIDGET (self->strip), UNITY_LAUNCHER_ACTION_REORDER,
                              "(si)", source, dest);
  return TRUE;
}

static void
unity_pinned_apps_reorder_dispose (GObject *object)
{
  UnityPinnedAppsReorder *self = UNITY_PINNED_APPS_REORDER (object);

  remove_placeholder (self);

  G_OBJECT_CLASS (unity_pinned_apps_reorder_parent_class)->dispose (object);
}

static void
unity_pinned_apps_reorder_class_init (UnityPinnedAppsReorderClass *klass)
{
  G_OBJECT_CLASS (klass)->dispose = unity_pinned_apps_reorder_dispose;
}

static void
unity_pinned_apps_reorder_init (UnityPinnedAppsReorder *self)
{
  self->index = -1;
}

UnityPinnedAppsReorder *
unity_pinned_apps_reorder_new (GtkBox *strip)
{
  g_return_val_if_fail (GTK_IS_BOX (strip), NULL);

  UnityPinnedAppsReorder *self = g_object_new (UNITY_TYPE_PINNED_APPS_REORDER, NULL);
  self->strip = strip;

  GtkDropTarget *target = gtk_drop_target_new (G_TYPE_STRING, GDK_ACTION_MOVE);
  gtk_drop_target_set_preload (target, TRUE);
  g_signal_connect_object (target, "motion", G_CALLBACK (on_drag_motion), self, G_CONNECT_DEFAULT);
  g_signal_connect_object (target, "leave",  G_CALLBACK (on_drag_leave),  self, G_CONNECT_DEFAULT);
  g_signal_connect_object (target, "drop",   G_CALLBACK (on_drop),        self, G_CONNECT_DEFAULT);
  gtk_widget_add_controller (GTK_WIDGET (strip), GTK_EVENT_CONTROLLER (target));

  return self;
}
