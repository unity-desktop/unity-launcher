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

#include "components/unity-launcher-app-strip.h"
#include "components/unity-launcher-app-tile.h"

#define PLACEHOLDER_MS 200

/* Drag gap size and axis, kept on the placeholder so it survives the fade-out. */
typedef struct
{
  gint            main_size;
  gint            cross_size;
  GtkOrientation  orientation;
} PlaceholderSlot;

struct _UnityPinnedAppsReorder
{
  GObject       parent_instance;

  GtkBox       *strip;        /* borrowed */
  GtkWidget    *placeholder;  /* child of the strip while a drag is live */
  GtkWidget    *fading;       /* the previous placeholder, fading out */
  AdwAnimation *anim;         /* animation on `placeholder`; the fade is separate */
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
  return widget != NULL && UNITY_IS_LAUNCHER_APP_TILE (widget);
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

/* The nth tile, skipping @skip. Every index here is measured in the order the
 * dragged tile has already left, so a slot needs no later correction. */
static GtkWidget *
nth_tile (UnityPinnedAppsReorder *self, gint n, GtkWidget *skip)
{
  for (GtkWidget *c = gtk_widget_get_first_child (GTK_WIDGET (self->strip));
       c != NULL; c = gtk_widget_get_next_sibling (c))
    if (is_tile (c) && c != skip && n-- == 0)
      return c;
  return NULL;
}

static GtkWidget *
tile_for_app_id (UnityPinnedAppsReorder *self, const gchar *app_id)
{
  if (app_id == NULL || *app_id == '\0')
    return NULL;

  for (GtkWidget *c = gtk_widget_get_first_child (GTK_WIDGET (self->strip));
       c != NULL; c = gtk_widget_get_next_sibling (c))
    if (is_tile (c) &&
        g_strcmp0 (unity_launcher_app_tile_get_app_id (UNITY_LAUNCHER_APP_TILE (c)), app_id) == 0)
      return c;
  return NULL;
}

static gint
slot_extent (UnityPinnedAppsReorder *self)
{
  GtkWidget *t = nth_tile (self, 0, NULL);
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
  /* A previous fade may still hold a child of the strip; cancelling the fade
   * loses its `done` callback, so unparent it here. */
  if (self->fading != NULL)
    {
      gtk_box_remove (self->strip, self->fading);
      self->fading = NULL;
    }
  self->index = -1;
}

static void
fade_out_done (AdwAnimation *anim, gpointer data)
{
  (void) anim;
  UnityPinnedAppsReorder *self = data;
  if (self->fading != NULL)
    {
      gtk_box_remove (self->strip, self->fading);
      self->fading = NULL;
    }
}

static void
fade_out_placeholder (UnityPinnedAppsReorder *self)
{
  GtkWidget *ph = self->placeholder;

  /* Cancel the show animation, if it is still running on the same widget. */
  if (self->anim != NULL)
    {
      adw_animation_reset (self->anim);
      g_clear_object (&self->anim);
    }
  self->placeholder = NULL;
  self->index       = -1;
  if (ph == NULL)
    return;

  /* A previous fade may still be running; drop its widget so the new fade owns
   * the only strip child in flight. */
  if (self->fading != NULL)
    {
      gtk_box_remove (self->strip, self->fading);
      self->fading = NULL;
    }
  self->fading = ph;

  AdwAnimationTarget *t = adw_callback_animation_target_new (anim_cb, g_object_ref (ph),
                                                             g_object_unref);
  AdwAnimation *a = adw_timed_animation_new (ph, gtk_widget_get_opacity (ph), 0.0,
                                             PLACEHOLDER_MS, t);
  adw_timed_animation_set_easing (ADW_TIMED_ANIMATION (a), ADW_EASE_OUT_QUAD);
  g_signal_connect (a, "done", G_CALLBACK (fade_out_done), self);
  /* The animation is not stored on self; it holds itself through play and drops
   * its reference in done, so remove_placeholder can never reset it. */
  adw_animation_play (a);
  g_object_unref (a);
}

static void
show_placeholder (UnityPinnedAppsReorder *self, gint idx, GtkWidget *source)
{
  if (idx == self->index)
    return;

  gboolean first = (self->placeholder == NULL);
  remove_placeholder (self);
  if (idx < 0)
    return;

  GtkWidget *t0 = nth_tile (self, 0, NULL);

  PlaceholderSlot *slot = g_new0 (PlaceholderSlot, 1);
  slot->main_size   = slot_extent (self);
  slot->cross_size  = t0 != NULL ? tile_cross_extent (self, t0) : -1;
  slot->orientation = orientation (self);

  GtkWidget *ph = gtk_box_new (slot->orientation, 0);
  gtk_widget_add_css_class (ph, "drag-placeholder");
  g_object_set_data_full (G_OBJECT (ph), "placeholder-slot", slot, g_free);

  GtkWidget *after = idx > 0 ? nth_tile (self, idx - 1, source) : NULL;
  gtk_box_insert_child_after (self->strip, ph, after);
  self->placeholder = ph;
  self->index       = idx;

  if (first)
    {
      anim_cb (0.0, ph);
      AdwAnimationTarget *t = adw_callback_animation_target_new (anim_cb, g_object_ref (ph),
                                                                 g_object_unref);
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

/* The slot the pointer names. A tile lands in the pinned run, so the count ends
 * where that run does: past it the slot is the end of it, and a drop there pins
 * the app last. The dragged tile is skipped before that test, so an unpinned one
 * does not end its own count. */
static gint
drop_slot (UnityPinnedAppsReorder *self, gdouble coord, GtkWidget *source)
{
  gint slot = 0;
  for (GtkWidget *c = gtk_widget_get_first_child (GTK_WIDGET (self->strip));
       c != NULL; c = gtk_widget_get_next_sibling (c))
    {
      graphene_rect_t b;
      if (!is_tile (c) || c == source ||
          !gtk_widget_compute_bounds (c, GTK_WIDGET (self->strip), &b))
        continue;
      if (!unity_launcher_app_tile_get_pinned (UNITY_LAUNCHER_APP_TILE (c)) ||
          coord <= rect_main_mid (self, &b))
        break;
      slot++;
    }
  return slot;
}

/* The slot the dragged tile already holds, or -1 when it holds none because it
 * is not pinned yet. A drop on its own slot changes nothing. */
static gint
source_slot (UnityPinnedAppsReorder *self, GtkWidget *source)
{
  gint slot = 0;
  for (GtkWidget *c = gtk_widget_get_first_child (GTK_WIDGET (self->strip));
       c != NULL; c = gtk_widget_get_next_sibling (c))
    {
      if (!is_tile (c))
        continue;
      if (!unity_launcher_app_tile_get_pinned (UNITY_LAUNCHER_APP_TILE (c)))
        break;
      if (c == source)
        return slot;
      slot++;
    }
  return -1;
}

static GdkDragAction
on_drag_motion (GtkDropTarget *target, gdouble x, gdouble y, gpointer data)
{
  UnityPinnedAppsReorder *self = data;

  if (self->anim != NULL &&
      adw_animation_get_state (self->anim) == ADW_ANIMATION_PLAYING)
    return GDK_ACTION_MOVE;

  const GValue *value  = gtk_drop_target_get_value (target);
  GtkWidget    *source = tile_for_app_id (self, (value != NULL && G_VALUE_HOLDS_STRING (value))
                                                ? g_value_get_string (value) : NULL);

  /* Only a tile of this strip can be placed in it. */
  if (source == NULL)
    {
      show_placeholder (self, -1, NULL);
      return 0;
    }

  /* Open no gap where the tile already sits: the drop would do nothing. */
  gint slot = drop_slot (self, is_horizontal (self) ? x : y, source);
  if (slot == source_slot (self, source))
    {
      show_placeholder (self, -1, NULL);
      return 0;
    }

  show_placeholder (self, slot, source);
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
  (void) target; (void) x; (void) y;
  UnityPinnedAppsReorder *self = data;

  const gchar *app_id = G_VALUE_HOLDS_STRING (value) ? g_value_get_string (value) : NULL;
  gint         dest   = self->index;

  remove_placeholder (self);
  if (app_id == NULL || dest < 0)
    return FALSE;

  gtk_widget_activate_action (GTK_WIDGET (self->strip), UNITY_LAUNCHER_ACTION_REORDER,
                              "(si)", app_id, dest);
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
