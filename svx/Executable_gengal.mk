# -*- Mode: makefile-gmake; tab-width: 4; indent-tabs-mode: t -*-
#
# This file is Part of the SnipeOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# This file incorporates work covered by the following license notice:
#
#   Licensed to the Apache Software Foundation (ASF) under one or more
#   contributor license agreements. See the NOTICE file distributed
#   with this work for additional information regarding copyright
#   ownership. The ASF licenses this file to you under the Apache
#   License, Version 2.0 (the "License"); you may not use this file
#   except in compliance with the License. You may obtain a copy of
#   the License at http://www.apache.org/licenses/LICENSE-2.0 .
#

$(eval $(call gb_Executable_Executable,gengal))

$(eval $(call gb_Executable_set_include,gengal,\
    $$(INCLUDE) \
    -I$(SRCDIR)/svx/inc/ \
    -I$(SRCDIR)/svx/inc/pch \
))

$(eval $(call gb_Executable_use_external,gengal,boost_headers))

$(eval $(call gb_Executable_use_sdk_api,gengal))

ifneq (,$(filter-out MACOSX WNT,$(OS)))
$(eval $(call gb_Executable_use_package,gengal,svx_gengal))
endif

$(eval $(call gb_Executable_use_libraries,gengal,\
    basegfx \
    sal \
    tl \
    sfx \
    svl \
    svt \
    comphelper \
    cppu \
    cppuhelper \
    utl \
    svxcore \
    vcl \
))

$(eval $(call gb_Executable_use_vclmain,gengal))

$(eval $(call gb_Executable_add_exception_objects,gengal,\
    svx/source/gengal/gengal \
))

ifeq ($(OS),WNT)
$(eval $(call gb_Executable_use_system_win32_libs,gengal,\
	kernel32 \
	msvcrt \
	oldnames \
	user32 \
))
endif

$(eval $(call gb_Executable_add_default_nativeres,gengal))

# vim: set noet sw=4 ts=4:
