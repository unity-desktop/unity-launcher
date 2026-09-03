/* unity-hide.c
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

#include "components/unity-hide.h"

#include <adwaita.h>
#include <astal-wayfire.h>
#include <graphene.h>
#include <graphene.h>

#include "dash/unity-dash.h"
#include "unity-position.h"

static gboolean
window_is_dash (GtkWindow *window)
{
  return window != NULL && UNITY_IS_DASH (window);
}
#include "unity-settings.h"

#define HIDE_PEEK      2
#define HIDE_SLIDE_MS  200
#define HIDE_DELAY_MS  300

typedef enum
{
  HIDE_NONE,
  HIDE_AUTOHIDE,
  HIDE_INTELLIHIDE,
} HideMode;

struct _UnityHide
{
  GObject        parent_instance;

  AstalWindow   *window;
  GSettings     *settings;
  GtkWindow     *dash;

  HideMode       mode;
  UnityPosition  position;
  gboolean       hovered;
  guint          holds;
  gboolean       spread;
  gboolean       revealed;

  AdwAnimation  *slide;
  guint          hide_source;
  gboolean       app_watched;

  GSignalGroup  *focused_view_signals;  /* watches the activated view's geometry, for intellihide */
};

G_DEFINE_FINAL_TYPE (UnityHide, unity_hide, G_TYPE_OBJECT)

static void
set_edge_margin (UnityHide *self, gint value)
{
  g_object_set (self->window, unity_position_edge_margin (self->position), value, NULL);
}

static void
reset_margins (UnityHide *self)
{
  g_object_set (self->window, "margin-left", 0, "margin-right", 0,
                "margin-top", 0, "margin-bottom", 0, NULL);
}

static gint
launcher_thickness (UnityHide *self)
{
  GtkWidget     *widget = GTK_WIDGET (self->window);
  gboolean       horiz  = unity_position_is_horizontal (self->position);
  GtkOrientation axis   = horiz ? GTK_ORIENTATION_VERTICAL : GTK_ORIENTATION_HORIZONTAL;
  gint           size   = horiz ? gtk_widget_get_height (widget)
                                : gtk_widget_get_width (widget);
  if (size <= 0)
    gtk_widget_measure (widget, axis, -1, NULL, &size, NULL, NULL);
  return MAX (0, size);
}

static gint
slide_extent (UnityHide *self)
{
  return MAX (0, launcher_thickness (self) - HIDE_PEEK);
}

static void
slide_cb (gdouble value, gpointer data)
{
  set_edge_margin (data, (gint) value);
}

static void
hide_animate (UnityHide *self, gboolean reveal)
{
  if (self->revealed == reveal)
    return;
  self->revealed = reveal;

  gint from = 0;
  g_object_get (self->window, unity_position_edge_margin (self->position), &from, NULL);
  gint to = reveal ? 0 : -slide_extent (self);

  if (self->slide != NULL)
    {
      adw_animation_reset (self->slide);
      g_clear_object (&self->slide);
    }
  AdwAnimationTarget *target = adw_callback_animation_target_new (slide_cb, self, NULL);
  self->slide = adw_timed_animation_new (GTK_WIDGET (self->window), from, to, HIDE_SLIDE_MS, target);
  adw_timed_animation_set_easing (ADW_TIMED_ANIMATION (self->slide), ADW_EASE_OUT_QUAD);
  adw_animation_play (self->slide);
}

static gboolean
dash_open (UnityHide *self)
{
  return self->dash != NULL && gtk_widget_get_visible (GTK_WIDGET (self->dash));
}

static gint
launcher_footprint (UnityHide *self)
{
  GdkSurface *surface = gtk_native_get_surface (GTK_NATIVE (self->window));
  if (surface == NULL)
    return launcher_thickness (self);
  return unity_position_is_horizontal (self->position) ? gdk_surface_get_height (surface)
                              : gdk_surface_get_width (surface);
}

/* Whether the focused window covers the launcher's strip. Fullscreen and
 * maximized are asked of the compositor (fast, always covers); anything else
 * falls back to a geometry intersection so a floating window dragged into the
 * strip zone still triggers hide. */
static gboolean
occluded (UnityHide *self)
{
  AstalWayfireWayfire *wf = astal_wayfire_wayfire_get_default ();
  gint thickness = launcher_footprint (self);
  if (wf == NULL || thickness <= 0)
    return FALSE;

  AstalWayfireView *view = astal_wayfire_wayfire_get_focused_view (wf);
  /* Restrict to real application toplevels so our own layer-shell surfaces (the
   * dash) never count as occluding. Today UNITY_HIDE_HOLD_DASH also short-
   * circuits this path, but this makes the filter structural. */
  if (view == NULL || !astal_wayfire_view_get_mapped (view) ||
      astal_wayfire_view_get_minimized (view) ||
      !astal_wayfire_view_get_is_toplevel (view))
    return FALSE;

  /* Fast path: the compositor's own answer. */
  if (astal_wayfire_view_get_fullscreen (view) ||
      astal_wayfire_view_get_is_maximized (view))
    return TRUE;

  /* Geometry path: does the window's rect intersect the strip? focused_output
   * returns transfer-none and tracks focused_view.output_id, so no lookup and
   * no leak. */
  AstalWayfireOutput *output = astal_wayfire_wayfire_get_focused_output (wf);
  if (output == NULL)
    return FALSE;

  gint out_x = astal_wayfire_output_get_x (output);
  gint out_y = astal_wayfire_output_get_y (output);
  gint out_w = astal_wayfire_output_get_width (output);
  gint out_h = astal_wayfire_output_get_height (output);

  /* The strip the launcher occupies: full output on its edge, `thickness` deep. */
  gint strip_x = out_x, strip_y = out_y, strip_w = out_w, strip_h = out_h;
  switch (self->position)
    {
    case UNITY_POSITION_RIGHT:  strip_x = out_x + out_w - thickness; strip_w = thickness; break;
    case UNITY_POSITION_BOTTOM: strip_y = out_y + out_h - thickness; strip_h = thickness; break;
    case UNITY_POSITION_LEFT:
    default:                    strip_w = thickness;                                      break;
    }

  graphene_rect_t win = GRAPHENE_RECT_INIT (
    astal_wayfire_view_get_x (view), astal_wayfire_view_get_y (view),
    astal_wayfire_view_get_width (view), astal_wayfire_view_get_height (view));
  graphene_rect_t strip = GRAPHENE_RECT_INIT (strip_x, strip_y, strip_w, strip_h);

  return graphene_rect_intersection (&win, &strip, NULL);
}

/* The one thing that differs by mode: a spread or none always shows; both hiding
 * modes show during an interaction; intellihide also shows while nothing covers it. */
static gboolean
want_reveal (UnityHide *self)
{
  if (self->spread || self->mode == HIDE_NONE)
    return TRUE;
  if (self->hovered || self->holds != 0)
    return TRUE;
  return self->mode == HIDE_INTELLIHIDE && !occluded (self);
}

/* Only none reserves an exclusive zone; the hiding modes overlay. */
static gboolean
is_reserving (UnityHide *self)
{
  return self->mode == HIDE_NONE;
}

/* Tell the spatial plugin how far to inset a spread so it lays out beside the
 * launcher. Only while overlaying (autohide/intellihide); in none the launcher's
 * exclusive zone already reserves the room, so the inset would count twice. */
static void
sync_spread_inset (UnityHide *self)
{
  AstalWayfireSpatial *spatial = astal_wayfire_spatial_get_default ();
  if (spatial == NULL)
    return;

  gint fp = is_reserving (self) ? 0 : launcher_footprint (self);
  gint left = 0, right = 0, bottom = 0;
  switch (self->position)
    {
    case UNITY_POSITION_RIGHT:  right = fp;  break;
    case UNITY_POSITION_BOTTOM: bottom = fp; break;
    case UNITY_POSITION_LEFT:
    default:                    left = fp;   break;
    }
  astal_wayfire_spatial_set_inset (spatial, left, right, 0, bottom);
}

/* Tell the dash how much to inset while it overlays. When reserving, the
 * exclusive zone already keeps the strip clear, so no inset is needed. */
static void
sync_dash_inset (UnityHide *self)
{
  if (self->dash == NULL)
    return;
  gboolean inset_needed = dash_open (self) && !is_reserving (self);
  gint     inset        = inset_needed ? launcher_footprint (self) : 0;
  g_object_set (self->dash, "launcher-inset", inset, NULL);
}

/* Reserve an exclusive zone only in `none`; autohide and intellihide overlay. */
static void
sync_exclusivity (UnityHide *self)
{
  g_object_set (self->window, "exclusivity",
                is_reserving (self) ? ASTAL_EXCLUSIVITY_EXCLUSIVE : ASTAL_EXCLUSIVITY_NORMAL, NULL);
}

static void
update (UnityHide *self)
{
  sync_exclusivity (self);
  sync_dash_inset (self);
  hide_animate (self, want_reveal (self));
}

static void
on_focused_view_geometry (GObject *view, GParamSpec *pspec, gpointer data)
{
  (void) view; (void) pspec;
  update (data);
}

/* Watch the focused view's geometry only in intellihide; a NULL target silences it. */
static void
sync_occlusion (UnityHide *self)
{
  AstalWayfireWayfire *wf = astal_wayfire_wayfire_get_default ();
  AstalWayfireView *view = (self->mode == HIDE_INTELLIHIDE && wf != NULL)
    ? astal_wayfire_wayfire_get_focused_view (wf) : NULL;
  g_signal_group_set_target (self->focused_view_signals, G_OBJECT (view));
}

static void
on_focused_view_changed (GObject *wf, GParamSpec *pspec, gpointer data)
{
  (void) wf; (void) pspec;
  UnityHide *self = data;
  if (self->mode != HIDE_INTELLIHIDE)
    return;
  sync_occlusion (self);
  update (self);
}

void
unity_hide_set_hold (UnityHide *self, UnityHideHold hold, gboolean active)
{
  g_return_if_fail (UNITY_IS_HIDE (self));

  guint holds = active ? (self->holds | hold) : (self->holds & ~hold);
  if (holds == self->holds)
    return;

  self->holds = holds;
  if (holds != 0)
    g_clear_handle_id (&self->hide_source, g_source_remove);
  update (self);
}

static void
on_enter (GtkEventControllerMotion *controller, gdouble x, gdouble y, gpointer data)
{
  (void) controller; (void) x; (void) y;
  UnityHide *self = data;
  g_clear_handle_id (&self->hide_source, g_source_remove);
  self->hovered = TRUE;
  update (self);
}

static void
hide_now (gpointer data)
{
  UnityHide *self = data;
  self->hide_source = 0;
  self->hovered = FALSE;
  update (self);
}

static void
on_leave (GtkEventControllerMotion *controller, gpointer data)
{
  (void) controller;
  UnityHide *self = data;
  g_clear_handle_id (&self->hide_source, g_source_remove);
  self->hide_source = g_timeout_add_once (HIDE_DELAY_MS, hide_now, self);
}

/* Reconfigure for the current hide mode and edge. */
static void
apply (UnityHide *self)
{
  self->mode     = g_settings_get_enum (self->settings, UNITY_LAUNCHER_KEY_HIDE);
  self->position = g_settings_get_enum (self->settings, UNITY_LAUNCHER_KEY_POSITION);

  sync_occlusion (self);
  sync_spread_inset (self);

  g_clear_handle_id (&self->hide_source, g_source_remove);
  if (self->slide != NULL)
    {
      adw_animation_reset (self->slide);
      g_clear_object (&self->slide);
    }
  self->hovered = FALSE;
  reset_margins (self);

  /* None stays shown with its exclusive zone; the hiding modes start off the edge.
   * Either way update() reconciles exclusivity, the dash inset, and the slide. */
  self->revealed = (self->mode == HIDE_NONE);
  if (!self->revealed)
    set_edge_margin (self, -slide_extent (self));
  update (self);
}

static void
on_settings_changed (GSettings *settings, const gchar *key, gpointer data)
{
  (void) settings; (void) key;
  apply (data);
}

/* The real size is known only once the compositor sizes the surface, and it can
 * change later. Recompute the hidden margin from it, unless sliding or revealed. */
static void
on_surface_layout (GdkSurface *surface, gint width, gint height, gpointer data)
{
  (void) surface;
  UnityHide *self = data;

  /* The footprint is real only once sized, and changes with the icon size. */
  sync_spread_inset (self);

  if (self->mode == HIDE_NONE || self->revealed)
    return;

  gboolean sliding = self->slide != NULL &&
                     adw_animation_get_state (self->slide) == ADW_ANIMATION_PLAYING;
  if (sliding)
    return;

  gint thickness = unity_position_is_horizontal (self->position) ? height : width;
  if (thickness > 0)
    set_edge_margin (self, -(MAX (0, thickness - HIDE_PEEK)));
}

static void
on_dash_notify_visible (GObject *dash, GParamSpec *pspec, gpointer data)
{
  (void) dash; (void) pspec;
  UnityHide *self = data;
  unity_hide_set_hold (self, UNITY_HIDE_HOLD_DASH, dash_open (self));
}

/* Track whether an overview/expo spread is up; it shows the launcher in every mode. */
static void
on_spatial_notify_stage (GObject *spatial, GParamSpec *pspec, gpointer user_data)
{
  (void) pspec;
  UnityHide *self = user_data;
  gboolean spread =
    astal_wayfire_spatial_get_stage (ASTAL_WAYFIRE_SPATIAL (spatial)) != ASTAL_WAYFIRE_STAGE_NONE;
  if (spread == self->spread)
    return;
  self->spread = spread;
  update (self);
}

/* Follow the dash's visibility so the launcher reveals with it. */
static void
track_dash (UnityHide *self, GtkWindow *dash)
{
  if (dash == NULL || self->dash == dash)
    return;
  g_set_weak_pointer (&self->dash, dash);
  g_signal_connect_object (dash, "notify::visible",
                           G_CALLBACK (on_dash_notify_visible), self, G_CONNECT_DEFAULT);
  unity_hide_set_hold (self, UNITY_HIDE_HOLD_DASH, dash_open (self));
}

/* The dash is a sibling window that may be built after the launcher, so watch for
 * it being added as well as scanning the ones already there. */
static void
on_window_added (GtkApplication *app, GtkWindow *window, gpointer data)
{
  (void) app;
  if (window_is_dash (window))
    track_dash (data, window);
}

static void
on_realize (GtkWidget *widget, gpointer data)
{
  UnityHide *self = data;

  GdkSurface *surface = gtk_native_get_surface (GTK_NATIVE (widget));
  if (surface != NULL)
    g_signal_connect_object (surface, "layout", G_CALLBACK (on_surface_layout),
                             self, G_CONNECT_DEFAULT);

  GtkApplication *app = gtk_window_get_application (GTK_WINDOW (widget));
  if (app == NULL || self->app_watched)
    return;
  self->app_watched = TRUE;

  g_signal_connect_object (app, "window-added", G_CALLBACK (on_window_added),
                           self, G_CONNECT_DEFAULT);
  for (GList *l = gtk_application_get_windows (app); l != NULL; l = l->next)
    if (window_is_dash (l->data))
      {
        track_dash (self, GTK_WINDOW (l->data));
        break;
      }
}

static void
unity_hide_dispose (GObject *object)
{
  UnityHide *self = UNITY_HIDE (object);

  g_clear_handle_id (&self->hide_source, g_source_remove);
  g_clear_object (&self->focused_view_signals);
  if (self->slide != NULL)
    {
      adw_animation_reset (self->slide);
      g_clear_object (&self->slide);
    }
  if (self->dash != NULL)
    {
      g_object_set (self->dash, "launcher-inset", 0, NULL);
      g_clear_weak_pointer (&self->dash);
    }

  /* Drop the spread inset we may have set, so it does not linger for a gone launcher. */
  AstalWayfireSpatial *spatial = astal_wayfire_spatial_get_default ();
  if (spatial != NULL)
    astal_wayfire_spatial_set_inset (spatial, 0, 0, 0, 0);

  G_OBJECT_CLASS (unity_hide_parent_class)->dispose (object);
}

static void
unity_hide_class_init (UnityHideClass *klass)
{
  G_OBJECT_CLASS (klass)->dispose = unity_hide_dispose;
}

static void
unity_hide_init (UnityHide *self)
{
  self->settings = unity_settings_get_default ();
  self->revealed = TRUE;

  /* Re-evaluate intellihide when the focused window moves, resizes, or (un)maps. */
  self->focused_view_signals = g_signal_group_new (ASTAL_WAYFIRE_TYPE_VIEW);
  const gchar *watched[] = { "notify::x", "notify::y", "notify::width", "notify::height",
                             "notify::minimized", "notify::mapped" };
  for (gsize i = 0; i < G_N_ELEMENTS (watched); i++)
    g_signal_group_connect (self->focused_view_signals, watched[i],
                            G_CALLBACK (on_focused_view_geometry), self);
}

UnityHide *
unity_hide_new (AstalWindow *window)
{
  g_return_val_if_fail (ASTAL_IS_WINDOW (window), NULL);

  UnityHide *self = g_object_new (UNITY_TYPE_HIDE, NULL);
  self->window = window;

  GtkEventController *motion = gtk_event_controller_motion_new ();
  g_signal_connect_object (motion, "enter", G_CALLBACK (on_enter), self, G_CONNECT_DEFAULT);
  g_signal_connect_object (motion, "leave", G_CALLBACK (on_leave), self, G_CONNECT_DEFAULT);
  gtk_widget_add_controller (GTK_WIDGET (window), motion);

  g_signal_connect_object (self->settings, "changed::" UNITY_LAUNCHER_KEY_HIDE,
                           G_CALLBACK (on_settings_changed), self, G_CONNECT_DEFAULT);
  g_signal_connect_object (self->settings, "changed::" UNITY_LAUNCHER_KEY_POSITION,
                           G_CALLBACK (on_settings_changed), self, G_CONNECT_DEFAULT);

  AstalWayfireSpatial *spatial = astal_wayfire_spatial_get_default ();
  if (spatial != NULL)
    {
      g_signal_connect_object (spatial, "notify::stage",
                               G_CALLBACK (on_spatial_notify_stage), self, G_CONNECT_DEFAULT);
      on_spatial_notify_stage (G_OBJECT (spatial), NULL, self);
    }

  AstalWayfireWayfire *wf = astal_wayfire_wayfire_get_default ();
  if (wf != NULL)
    g_signal_connect_object (wf, "notify::focused-view",
                             G_CALLBACK (on_focused_view_changed), self, G_CONNECT_DEFAULT);

  g_signal_connect_object (window, "realize", G_CALLBACK (on_realize), self, G_CONNECT_DEFAULT);
  if (gtk_widget_get_realized (GTK_WIDGET (window)))
    on_realize (GTK_WIDGET (window), self);

  apply (self);
  return self;
}
