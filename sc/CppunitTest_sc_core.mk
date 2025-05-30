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

$(eval $(call gb_CppunitTest_CppunitTest,sc_core))

$(eval $(call gb_CppunitTest_use_external,sc_core,boost_headers))

$(eval $(call gb_CppunitTest_use_common_precompiled_header,sc_core))

$(eval $(call gb_CppunitTest_add_exception_objects,sc_core, \
	sc/qa/unit/test_ScChartListenerCollection \
))

$(eval $(call gb_CppunitTest_use_libraries,sc_core, \
	cppu \
	cppuhelper \
	sal \
	salhelper \
	sc \
	test \
	unotest \
))

$(eval $(call gb_CppunitTest_set_include,sc_core,\
	-I$(SRCDIR)/sc/inc \
	$$(INCLUDE) \
))

$(eval $(call gb_CppunitTest_use_sdk_api,sc_core))

$(eval $(call gb_CppunitTest_use_ure,sc_core))
$(eval $(call gb_CppunitTest_use_vcl,sc_core))

$(eval $(call gb_CppunitTest_use_rdb,sc_core,services))

$(eval $(call gb_CppunitTest_use_configuration,sc_core))

# vim: set noet sw=4 ts=4:
