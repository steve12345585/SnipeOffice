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

$(eval $(call gb_CppunitTest_CppunitTest,sd_layout_tests))

$(eval $(call gb_CppunitTest_use_externals,sd_layout_tests,\
	boost_headers \
	libxml2 \
))

$(eval $(call gb_CppunitTest_use_common_precompiled_header,sd_layout_tests))

$(eval $(call gb_CppunitTest_add_exception_objects,sd_layout_tests, \
    sd/qa/unit/layout-tests \
))

$(eval $(call gb_CppunitTest_use_libraries,sd_layout_tests, \
	$(call gb_Helper_optional,AVMEDIA,avmedia) \
    basegfx \
    comphelper \
    cppu \
    cppuhelper \
    drawinglayer \
    editeng \
    for \
    forui \
    i18nlangtag \
    msfilter \
    oox \
    sal \
    salhelper \
    sax \
    sd \
    sfx \
    sot \
    subsequenttest \
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

$(eval $(call gb_CppunitTest_set_include,sd_layout_tests,\
    -I$(SRCDIR)/sd/source/ui/inc \
    -I$(SRCDIR)/sd/inc \
    $$(INCLUDE) \
))

$(eval $(call gb_CppunitTest_use_sdk_api,sd_layout_tests))

$(eval $(call gb_CppunitTest_use_ure,sd_layout_tests))
$(eval $(call gb_CppunitTest_use_vcl,sd_layout_tests))

$(eval $(call gb_CppunitTest_use_rdb,sd_layout_tests,services))

$(eval $(call gb_CppunitTest_use_custom_headers,sd_layout_tests,\
	officecfg/registry \
))

$(eval $(call gb_CppunitTest_use_configuration,sd_layout_tests))

# vim: set noet sw=4 ts=4:
