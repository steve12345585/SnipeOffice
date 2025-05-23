# -*- Mode: makefile-gmake; tab-width: 4; indent-tabs-mode: t -*-
#*************************************************************************
#
# This file is Part of the SnipeOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
#*************************************************************************

$(eval $(call gb_CppunitTest_CppunitTest,sc_screenshots))

$(eval $(call gb_CppunitTest_use_common_precompiled_header,sc_screenshots))

$(eval $(call gb_CppunitTest_add_exception_objects,sc_screenshots, \
    sc/qa/unit/screenshots/screenshots \
))

$(eval $(call gb_CppunitTest_use_libraries,sc_screenshots, \
    comphelper \
    cppu \
    cppuhelper \
    editeng \
    sal \
    sfx \
    svl \
    svt \
    svx \
    svxcore \
    sc \
    scui \
    test \
    unotest \
    vcl \
    tl \
    utl \
))

$(eval $(call gb_CppunitTest_use_externals,sc_screenshots,\
    boost_headers \
))

$(eval $(call gb_CppunitTest_set_include,sc_screenshots,\
    -I$(SRCDIR)/sc/source/ui/inc \
    -I$(SRCDIR)/sc/inc \
    $$(INCLUDE) \
))

$(eval $(call gb_CppunitTest_use_api,sc_screenshots,\
    offapi \
    oovbaapi \
    udkapi \
))

$(eval $(call gb_CppunitTest_use_ure,sc_screenshots))
$(eval $(call gb_CppunitTest_use_vcl_non_headless_with_windows,sc_screenshots))

$(eval $(call gb_CppunitTest_use_rdb,sc_screenshots,services))

$(eval $(call gb_CppunitTest_use_configuration,sc_screenshots))

$(eval $(call gb_CppunitTest_use_uiconfigs,sc_screenshots,\
	cui \
	modules/scalc \
))

# vim: set noet sw=4 ts=4:
