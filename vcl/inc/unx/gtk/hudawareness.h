/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <gio/gio.h>

G_BEGIN_DECLS

typedef void         (* HudAwarenessCallback)                           (gboolean               hud_active,
                                                                         gpointer               user_data);

guint                   hud_awareness_register                          (GDBusConnection       *connection,
                                                                         const gchar           *object_path,
                                                                         HudAwarenessCallback   callback,
                                                                         gpointer               user_data,
                                                                         GDestroyNotify         notify,
                                                                         GError               **error);

void                    hud_awareness_unregister                        (GDBusConnection       *connection,
                                                                         guint                  awareness_id);

G_END_DECLS
