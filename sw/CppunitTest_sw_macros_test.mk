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

$(eval $(call gb_CppunitTest_CppunitTest,sw_macros_test))

$(eval $(call gb_CppunitTest_use_externals,sw_macros_test,\
	boost_headers \
	libxml2 \
))

$(eval $(call gb_CppunitTest_use_common_precompiled_header,sw_macros_test))

$(eval $(call gb_CppunitTest_add_exception_objects,sw_macros_test, \
    sw/qa/core/macros-test \
))

$(eval $(call gb_CppunitTest_use_libraries,sw_macros_test, \
    basegfx \
    comphelper \
    cppu \
    cppuhelper \
    drawinglayer \
    docmodel \
    editeng \
    for \
    forui \
    i18nlangtag \
    msfilter \
    oox \
    sal \
    salhelper \
    sax \
    sb \
    sw \
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
    $(call gb_Helper_optional,SCRIPTING, \
        vbahelper) \
    vcl \
    xo \
))

$(eval $(call gb_CppunitTest_set_include,sw_macros_test,\
    -I$(SRCDIR)/sw/inc \
    $$(INCLUDE) \
))

$(eval $(call gb_CppunitTest_use_api,sw_macros_test,\
    offapi \
    oovbaapi \
    udkapi \
))

$(eval $(call gb_CppunitTest_use_ure,sw_macros_test))
$(eval $(call gb_CppunitTest_use_vcl,sw_macros_test))

$(eval $(call gb_CppunitTest_use_rdb,sw_macros_test,services))

$(eval $(call gb_CppunitTest_use_configuration,sw_macros_test))

$(eval $(call gb_CppunitTest_add_arguments,sw_macros_test, \
    -env:arg-env=$(gb_Helper_LIBRARY_PATH_VAR)"$$$${$(gb_Helper_LIBRARY_PATH_VAR)+=$$$$$(gb_Helper_LIBRARY_PATH_VAR)}" \
))

# vim: set noet sw=4 ts=4:
