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

$(eval $(call gb_CppunitTest_CppunitTest,sw_uibase_fldui))

$(eval $(call gb_CppunitTest_use_common_precompiled_header,sw_uibase_fldui))

$(eval $(call gb_CppunitTest_add_exception_objects,sw_uibase_fldui, \
    sw/qa/uibase/fldui/fldui \
))

$(eval $(call gb_CppunitTest_use_libraries,sw_uibase_fldui, \
    comphelper \
    cppu \
    cppuhelper \
    sal \
    sfx \
    subsequenttest \
    svxcore \
    sw \
    swqahelper \
    test \
    unotest \
    utl \
    vcl \
    svt \
    tl \
    svl \
))

$(eval $(call gb_CppunitTest_use_externals,sw_uibase_fldui,\
    boost_headers \
    libxml2 \
))

$(eval $(call gb_CppunitTest_set_include,sw_uibase_fldui,\
    -I$(SRCDIR)/sw/inc \
    -I$(SRCDIR)/sw/source/core/inc \
    -I$(SRCDIR)/sw/source/uibase/inc \
    -I$(SRCDIR)/sw/qa/inc \
    $$(INCLUDE) \
))

$(eval $(call gb_CppunitTest_use_api,sw_uibase_fldui,\
    udkapi \
    offapi \
    oovbaapi \
))

$(eval $(call gb_CppunitTest_use_ure,sw_uibase_fldui))
$(eval $(call gb_CppunitTest_use_vcl,sw_uibase_fldui))

$(eval $(call gb_CppunitTest_use_rdb,sw_uibase_fldui,services))

$(eval $(call gb_CppunitTest_use_custom_headers,sw_uibase_fldui,\
    officecfg/registry \
))

$(eval $(call gb_CppunitTest_use_configuration,sw_uibase_fldui))

$(eval $(call gb_CppunitTest_use_uiconfigs,sw_uibase_fldui, \
    modules/swriter \
))

$(eval $(call gb_CppunitTest_use_more_fonts,sw_uibase_fldui))

ifneq ($(filter MORE_FONTS,$(BUILD_TYPE)),)
$(eval $(call gb_CppunitTest_set_non_application_font_use,sw_uibase_fldui,abort))
endif

# vim: set noet sw=4 ts=4:
