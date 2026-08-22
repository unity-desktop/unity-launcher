/* unity-hide.h
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

#pragma once

#include <astal-4.h>

G_BEGIN_DECLS

#define UNITY_TYPE_HIDE (unity_hide_get_type ())

/**
 * UnityHide:
 *
 * Drives hiding on the launcher window from the "hide" setting (none, autohide,
 * intellihide), sliding it off the edge set by "position".
 */
G_DECLARE_FINAL_TYPE (UnityHide, unity_hide, UNITY, HIDE, GObject)

/**
 * UnityHideHold:
 * @UNITY_HIDE_HOLD_DASH: the dash is open.
 * @UNITY_HIDE_HOLD_DRAG: a launcher tile is being dragged.
 * @UNITY_HIDE_HOLD_MENU: a tile context menu is open.
 *
 * Reasons that keep the launcher revealed beyond a plain hover. Any active reason
 * holds it out; the launcher hides only when none are set and the pointer leaves.
 */
typedef enum
{
  UNITY_HIDE_HOLD_DASH = 1 << 0,
  UNITY_HIDE_HOLD_DRAG = 1 << 1,
  UNITY_HIDE_HOLD_MENU = 1 << 2,
} G_GNUC_FLAG_ENUM UnityHideHold;

/**
 * unity_hide_new:
 * @window: the launcher window to drive. It is borrowed.
 *
 * Makes a hide controller for @window.
 *
 * Returns: (transfer full): a new #UnityHide.
 */
UnityHide *unity_hide_new (AstalWindow *window);

/**
 * unity_hide_set_hold:
 * @self: a #UnityHide.
 * @hold: the reason to set or clear.
 * @active: whether the reason is active.
 *
 * Marks a keep-revealed reason active or not. While any reason is active the
 * launcher stays out regardless of the pointer.
 */
void unity_hide_set_hold (UnityHide *self, UnityHideHold hold, gboolean active);

G_END_DECLS
