# -*- Mode: makefile-gmake; tab-width: 4; indent-tabs-mode: t -*-
#
# This file is Part of the SnipeOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

$(eval $(call gb_Executable_Executable,unopkg_com))

$(eval $(call gb_Executable_set_targettype_gui,unopkg_com,NO))

$(eval $(call gb_Executable_use_static_libraries,unopkg_com,\
    ooopathutils \
    winloader \
))

$(eval $(call gb_Executable_use_system_win32_libs,unopkg_com,\
    shell32 \
))

$(eval $(call gb_Executable_add_exception_objects,unopkg_com,\
    desktop/win32/source/officeloader/unopkg_com \
))

$(eval $(call gb_Executable_add_default_nativeres,unopkg_com))

# vim: set ts=4 sw=4 et:
