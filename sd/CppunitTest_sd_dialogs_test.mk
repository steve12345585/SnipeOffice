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

$(eval $(call gb_CppunitTest_CppunitScreenShot,sd_dialogs_test))

$(eval $(call gb_CppunitTest_use_common_precompiled_header,sd_dialogs_test))

$(eval $(call gb_CppunitTest_add_exception_objects,sd_dialogs_test, \
    sd/qa/unit/dialogs-test \
))

$(eval $(call gb_CppunitTest_use_sdk_api,sd_dialogs_test))

$(eval $(call gb_CppunitTest_set_include,sd_dialogs_test,\
    -I$(SRCDIR)/sd/source/ui/inc \
    -I$(SRCDIR)/sd/inc \
    $$(INCLUDE) \
))

$(eval $(call gb_CppunitTest_use_libraries,sd_dialogs_test, \
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
    sd \
    sdui \
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

$(eval $(call gb_CppunitTest_use_external,sd_dialogs_test,boost_headers))

$(eval $(call gb_CppunitTest_use_sdk_api,sd_dialogs_test))

$(eval $(call gb_CppunitTest_use_ure,sd_dialogs_test))
$(eval $(call gb_CppunitTest_use_vcl_non_headless_with_windows,sd_dialogs_test))

$(eval $(call gb_CppunitTest_use_rdb,sd_dialogs_test,services))

$(eval $(call gb_CppunitTest_use_configuration,sd_dialogs_test))

$(eval $(call gb_CppunitTest_use_uiconfigs,sd_dialogs_test,\
	cui \
	modules/sdraw \
	modules/simpress \
))

# vim: set noet sw=4 ts=4:
