# -*- Mode: makefile-gmake; tab-width: 4; indent-tabs-mode: t; fill-column: 100 -*-
#
# This file is Part of the SnipeOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

$(eval $(call gb_CppunitTest_CppunitTest,sd_activex_controls_tests))

$(eval $(call gb_CppunitTest_use_externals,sd_activex_controls_tests,\
    boost_headers \
    libxml2 \
))

$(eval $(call gb_CppunitTest_use_common_precompiled_header,sd_activex_controls_tests))

$(eval $(call gb_CppunitTest_add_exception_objects,sd_activex_controls_tests, \
    sd/qa/unit/activex-controls-tests \
))

$(eval $(call gb_CppunitTest_use_libraries,sd_activex_controls_tests, \
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

$(eval $(call gb_CppunitTest_set_include,sd_activex_controls_tests,\
    -I$(SRCDIR)/sd/source/ui/inc \
    -I$(SRCDIR)/sd/inc \
    $$(INCLUDE) \
))

$(eval $(call gb_CppunitTest_use_sdk_api,sd_activex_controls_tests))

$(eval $(call gb_CppunitTest_use_ure,sd_activex_controls_tests))
$(eval $(call gb_CppunitTest_use_vcl,sd_activex_controls_tests))

$(eval $(call gb_CppunitTest_use_rdb,sd_activex_controls_tests,services))

$(eval $(call gb_CppunitTest_use_configuration,sd_activex_controls_tests))

# vim: set noet sw=4 ts=4:
