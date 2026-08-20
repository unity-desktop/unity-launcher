/* unity-dismiss.h
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

/**
 * UnityDismissFunc:
 * @user_data: the data given to unity_dismiss_attach().
 *
 * A callback that the dismiss helper runs on minimize or on close.
 */
typedef void (*UnityDismissFunc) (gpointer user_data);

/**
 * unity_dismiss_attach:
 * @surface: the layer-shell surface widget.
 * @area: the full-surface widget that catches outside clicks.
 * @content: the content widget. A click inside it does not dismiss.
 * @on_minimize: runs on Escape, an outside click, or focus leaving the surface.
 * @on_close: runs on Ctrl+W, Alt+F4, or a window close-request.
 * @user_data: the data for the callbacks.
 *
 * Gives a layer-shell surface popover-like dismissal.
 */
void unity_dismiss_attach (GtkWidget        *surface,
                           GtkWidget        *area,
                           GtkWidget        *content,
                           UnityDismissFunc  on_minimize,
                           UnityDismissFunc  on_close,
                           gpointer          user_data);

G_END_DECLS
