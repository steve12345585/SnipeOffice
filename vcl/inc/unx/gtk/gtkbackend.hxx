/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <gtk/gtk.h>
#if defined(GDK_WINDOWING_X11)
#if GTK_CHECK_VERSION(4, 0, 0)
#include <gdk/x11/gdkx.h>
#else
#include <gdk/gdkx.h>
#endif
bool DLSYM_GDK_IS_X11_DISPLAY(GdkDisplay* pDisplay);
#endif
#if defined(GDK_WINDOWING_WAYLAND)
#if GTK_CHECK_VERSION(4, 0, 0)
#include <gdk/wayland/gdkwayland.h>
#else
#include <gdk/gdkwayland.h>
#endif
bool DLSYM_GDK_IS_WAYLAND_DISPLAY(GdkDisplay* pDisplay);
#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
