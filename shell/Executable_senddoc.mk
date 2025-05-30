# -*- Mode: makefile-gmake; tab-width: 4; indent-tabs-mode: t -*-
#
# This file is Part of the SnipeOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

$(eval $(call gb_Executable_Executable,senddoc))

$(eval $(call gb_Executable_use_system_win32_libs,senddoc,\
	kernel32 \
))

$(eval $(call gb_Executable_use_sdk_api,senddoc))

$(eval $(call gb_Executable_use_libraries,senddoc,\
	i18nlangtag \
	sal \
	utl \
))

$(eval $(call gb_Executable_add_exception_objects,senddoc,\
    shell/source/win32/simplemail/senddoc \
))

$(eval $(call gb_Executable_add_default_nativeres,senddoc))

# vim: set shiftwidth=4 tabstop=4 noexpandtab:
