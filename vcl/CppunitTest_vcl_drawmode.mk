# -*- Mode: makefile-gmake; tab-width: 4; indent-tabs-mode: t -*-
#
# This file is Part of the SnipeOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

$(eval $(call gb_CppunitTest_CppunitTest,vcl_drawmode))

$(eval $(call gb_CppunitTest_set_include,vcl_drawmode,\
    $$(INCLUDE) \
    -I$(SRCDIR)/vcl/inc \
))

$(eval $(call gb_CppunitTest_add_exception_objects,vcl_drawmode, \
	vcl/qa/cppunit/drawmode \
))

$(eval $(call gb_CppunitTest_use_libraries,vcl_drawmode, \
	comphelper \
	cppu \
	cppuhelper \
	sal \
	svt \
	test \
	tl \
	tk \
	unotest \
	vcl \
))

$(eval $(call gb_CppunitTest_use_sdk_api,vcl_drawmode))

$(eval $(call gb_CppunitTest_use_ure,vcl_drawmode))
$(eval $(call gb_CppunitTest_use_vcl,vcl_drawmode))

$(eval $(call gb_CppunitTest_use_configuration,vcl_drawmode))

$(eval $(call gb_CppunitTest_use_components,vcl_drawmode,\
	configmgr/source/configmgr \
	i18npool/util/i18npool \
	ucb/source/core/ucb1 \
))

# vim: set noet sw=4 ts=4:
