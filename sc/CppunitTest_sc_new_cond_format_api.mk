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

$(eval $(call gb_CppunitTest_CppunitTest,sc_new_cond_format_api))

$(eval $(call gb_CppunitTest_use_common_precompiled_header,sc_new_cond_format_api))

$(eval $(call gb_CppunitTest_add_exception_objects,sc_new_cond_format_api, \
	sc/qa/extras/new_cond_format \
))

$(eval $(call gb_CppunitTest_use_external,sc_new_cond_format_api,boost_headers))

$(eval $(call gb_CppunitTest_use_libraries,sc_new_cond_format_api, \
	basegfx \
	comphelper \
	cppu \
	cppuhelper \
	drawinglayer \
	drawinglayercore \
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
	sfx \
	sot \
	subsequenttest \
	svl \
	svt \
	svx \
	svxcore \
	test \
	tk \
	tl \
	ucbhelper \
	unotest \
	utl \
    $(call gb_Helper_optional,SCRIPTING, \
        vbahelper) \
	vcl \
	xo \
))

$(eval $(call gb_CppunitTest_set_include,sc_new_cond_format_api,\
	-I$(SRCDIR)/sc/source/ui/inc \
	-I$(SRCDIR)/sc/inc \
	$$(INCLUDE) \
))

$(eval $(call gb_CppunitTest_use_sdk_api,sc_new_cond_format_api))

$(eval $(call gb_CppunitTest_use_ure,sc_new_cond_format_api))
$(eval $(call gb_CppunitTest_use_vcl,sc_new_cond_format_api))

$(eval $(call gb_CppunitTest_use_rdb,sc_new_cond_format_api,services))

$(eval $(call gb_CppunitTest_use_configuration,sc_new_cond_format_api))

# vim: set noet sw=4 ts=4:
