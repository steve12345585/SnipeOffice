# -*- Mode: makefile-gmake; tab-width: 4; indent-tabs-mode: t -*-
#
# This file is Part of the SnipeOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

$(eval $(call gb_CppunitTest_CppunitTest,vcl_textlayout))

$(eval $(call gb_CppunitTest_set_include,vcl_textlayout,\
    $$(INCLUDE) \
    -I$(SRCDIR)/vcl/inc \
))

$(eval $(call gb_CppunitTest_add_exception_objects,vcl_textlayout, \
	vcl/qa/cppunit/textlayout \
))

$(eval $(call gb_CppunitTest_use_externals,vcl_textlayout,\
	boost_headers \
	harfbuzz \
))

ifeq ($(SYSTEM_ICU),TRUE)
$(eval $(call gb_CppunitTest_use_externals,vcl_textlayout,\
	icuuc \
))
else
$(eval $(call gb_CppunitTest_use_externals,vcl_textlayout,\
        icu_headers \
))
endif

$(eval $(call gb_CppunitTest_use_libraries,vcl_textlayout, \
	comphelper \
	cppu \
	cppuhelper \
	i18nlangtag \
	sal \
	svt \
	test \
	tl \
	unotest \
	vcl \
))

$(eval $(call gb_CppunitTest_use_sdk_api,vcl_textlayout))

$(eval $(call gb_CppunitTest_use_ure,vcl_textlayout))
$(eval $(call gb_CppunitTest_use_vcl,vcl_textlayout))

$(eval $(call gb_CppunitTest_use_components,vcl_textlayout,\
	configmgr/source/configmgr \
	i18npool/util/i18npool \
	ucb/source/core/ucb1 \
	linguistic/source/lng \
))

$(eval $(call gb_CppunitTest_use_configuration,vcl_textlayout))

$(eval $(call gb_CppunitTest_use_more_fonts,vcl_textlayout))

# vim: set noet sw=4 ts=4:
