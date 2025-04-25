# -*- Mode: makefile-gmake; tab-width: 4; indent-tabs-mode: t -*-
#
# This file is Part of the SnipeOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

$(eval $(call gb_Executable_Executable,icontest))

$(eval $(call gb_Executable_add_defs,icontest,\
    -DVCL_INTERNALS \
))

$(eval $(call gb_Executable_use_api,icontest,\
    offapi \
    udkapi \
))

$(eval $(call gb_Executable_use_libraries,icontest,\
    comphelper \
    cppu \
    cppuhelper \
    sal \
    tl \
    ucbhelper \
    vcl \
))

$(eval $(call gb_Executable_use_vclmain,icontest))

$(eval $(call gb_Executable_add_exception_objects,icontest,\
    vcl/workben/icontest \
))

# vim: set noet sw=4 ts=4:
