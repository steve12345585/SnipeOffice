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

$(eval $(call gb_CppunitTest_CppunitScreenShot,chart2_dialogs_test))

$(eval $(call gb_CppunitTest_add_exception_objects,chart2_dialogs_test, \
    chart2/qa/unit/chart2-dialogs-test \
))

$(eval $(call gb_CppunitTest_use_sdk_api,chart2_dialogs_test))

$(eval $(call gb_CppunitTest_set_include,chart2_dialogs_test,\
    -I$(SRCDIR)/chart2/source/inc \
    -I$(SRCDIR)/chart2/inc \
    $$(INCLUDE) \
))

$(eval $(call gb_CppunitTest_use_libraries,chart2_dialogs_test, \
    basegfx \
    comphelper \
    cppu \
    cppuhelper \
    drawinglayer \
    editeng \
    i18nlangtag \
    i18nutil \
    msfilter \
    oox \
    sal \
    salhelper \
    sax \
    sfx \
    sot \
    svl \
    svt \
    svx \
    svxcore \
    test \
    tl \
    tk \
    ucbhelper \
    unotest \
    utl \
    vcl \
    xo \
))

$(eval $(call gb_CppunitTest_use_external,chart2_dialogs_test,boost_headers))

$(eval $(call gb_CppunitTest_use_sdk_api,chart2_dialogs_test))

$(eval $(call gb_CppunitTest_use_ure,chart2_dialogs_test))
$(eval $(call gb_CppunitTest_use_vcl_non_headless_with_windows,chart2_dialogs_test))

$(eval $(call gb_CppunitTest_use_rdb,chart2_dialogs_test,services))

$(eval $(call gb_CppunitTest_use_configuration,chart2_dialogs_test))

$(eval $(call gb_CppunitTest_use_uiconfigs,chart2_dialogs_test,\
	modules/schart \
	svx \
))

$(eval $(call gb_CppunitTest_use_packages,chart2_dialogs_test,\
	extras_palettes \
))

# vim: set noet sw=4 ts=4:
