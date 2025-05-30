# -*- Mode: makefile-gmake; tab-width: 4; indent-tabs-mode: t -*-
#
# This file is Part of the SnipeOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

$(eval $(call gb_CppunitTest_CppunitTest,framework_dispatch))

$(eval $(call gb_CppunitTest_add_exception_objects,framework_dispatch, \
    framework/qa/cppunit/dispatchtest \
))

$(eval $(call gb_CppunitTest_use_sdk_api,framework_dispatch))

$(eval $(call gb_CppunitTest_use_libraries,framework_dispatch, \
	comphelper \
	cppu \
	cppuhelper \
	fwk \
	sal \
	subsequenttest \
	utl \
	tl \
	test \
	unotest \
))

$(eval $(call gb_CppunitTest_use_external,framework_dispatch,boost_headers))

$(eval $(call gb_CppunitTest_use_sdk_api,framework_dispatch))

$(eval $(call gb_CppunitTest_use_ure,framework_dispatch))
$(eval $(call gb_CppunitTest_use_vcl,framework_dispatch))

$(eval $(call gb_CppunitTest_use_rdb,framework_dispatch,services))

$(eval $(call gb_CppunitTest_use_configuration,framework_dispatch))

# vim: set noet sw=4 ts=4:
