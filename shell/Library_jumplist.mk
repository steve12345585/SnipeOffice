# -*- Mode: makefile-gmake; tab-width: 4; indent-tabs-mode: t; fill-column: 100 -*-
#
# This file is Part of the SnipeOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

$(eval $(call gb_Library_Library,jumplist))

$(eval $(call gb_Library_use_sdk_api,jumplist))

$(eval $(call gb_Library_use_libraries,jumplist,\
	comphelper \
	cppu \
	cppuhelper \
	sal \
))

ifeq ($(OS),WNT)

$(eval $(call gb_Library_use_system_win32_libs,jumplist,\
	ole32 \
	Propsys \
	shell32 \
))

$(eval $(call gb_Library_set_componentfile,jumplist,shell/source/win32/jumplist/jumplist,services))

$(eval $(call gb_Library_add_exception_objects,jumplist,\
	shell/source/win32/jumplist/JumpList \
))

endif # OS

# vim: set noet sw=4 ts=4:
