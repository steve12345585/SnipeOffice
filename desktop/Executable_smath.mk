# -*- Mode: makefile-gmake; tab-width: 4; indent-tabs-mode: t -*-
#
# This file is Part of the SnipeOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

$(eval $(call gb_Executable_Executable,smath))

$(eval $(call gb_Executable_set_targettype_gui,smath,YES))

$(eval $(call gb_Executable_add_ldflags,smath,\
    /ENTRY:wWinMainCRTStartup \
))

$(eval $(call gb_Executable_use_static_libraries,smath,\
    winlauncher \
))

$(eval $(call gb_Executable_add_exception_objects,smath,\
    desktop/win32/source/applauncher/smath \
))

$(eval $(call gb_Executable_add_nativeres,smath,smath/launcher))

$(eval $(call gb_Executable_add_default_nativeres,smath,$(PRODUCTNAME) Math))

# vim: set ts=4 sw=4 et:
