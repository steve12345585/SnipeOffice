# -*- Mode: makefile-gmake; tab-width: 4; indent-tabs-mode: t -*-
#
#
# This file is Part of the SnipeOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

$(eval $(call gb_Executable_Executable,minweld))

$(eval $(call gb_Executable_use_api,minweld,\
    offapi \
    udkapi \
))

$(eval $(call gb_Executable_set_include,minweld,\
    $$(INCLUDE) \
    -I$(SRCDIR)/vcl/inc \
))

$(eval $(call gb_Executable_use_libraries,minweld,\
    tl \
    sal \
    vcl \
    cppu \
    cppuhelper \
    comphelper \
    i18nlangtag \
    fwk \
))

$(eval $(call gb_Executable_add_exception_objects,minweld,\
    vcl/workben/minweld \
))

# vim: set noet sw=4 ts=4:
