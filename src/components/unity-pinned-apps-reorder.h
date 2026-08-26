/* unity-pinned-apps-reorder.h
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

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define UNITY_TYPE_PINNED_APPS_REORDER (unity_pinned_apps_reorder_get_type ())

/**
 * UnityPinnedAppsReorder:
 *
 * Handles dragging a pinned launcher tile to a new slot. It shows a make-room
 * placeholder as the pointer moves and, on drop, activates the launcher's
 * `reorder-pinned` action with the source id and the destination index.
 */
G_DECLARE_FINAL_TYPE (UnityPinnedAppsReorder, unity_pinned_apps_reorder,
                      UNITY, PINNED_APPS_REORDER, GObject)

/**
 * unity_pinned_apps_reorder_new:
 * @strip: the box that holds the launcher tiles. It is borrowed.
 *
 * Makes a reorder handler for @strip.
 *
 * Returns: (transfer full): a new #UnityPinnedAppsReorder.
 */
UnityPinnedAppsReorder *unity_pinned_apps_reorder_new (GtkBox *strip);

G_END_DECLS
