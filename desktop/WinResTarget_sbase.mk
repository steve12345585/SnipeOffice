# -*- Mode: makefile-gmake; tab-width: 4; indent-tabs-mode: t -*-
#
# This file is Part of the SnipeOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

$(eval $(call gb_WinResTarget_WinResTarget,sbase/launcher))

$(eval $(call gb_WinResTarget_set_include,sbase/launcher,\
    $$(INCLUDE) \
    -I$(SRCDIR)/sysui/desktop \
))

$(eval $(call gb_WinResTarget_add_defs,sbase/launcher,\
    -DRES_APP_ICON=icons/base_app.ico \
))

$(eval $(call gb_WinResTarget_add_dependencies,sbase/launcher,\
    sysui/desktop/icons/base_app.ico \
))

$(eval $(call gb_WinResTarget_set_rcfile,sbase/launcher,desktop/win32/source/applauncher/launcher))

# vim: set ts=4 sw=4 et:
