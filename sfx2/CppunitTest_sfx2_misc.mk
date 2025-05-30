# -*- Mode: makefile-gmake; tab-width: 4; indent-tabs-mode: t -*-
#
# This file is Part of the SnipeOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

$(eval $(call gb_CppunitTest_CppunitTest,sfx2_misc))

$(eval $(call gb_CppunitTest_add_exception_objects,sfx2_misc, \
    sfx2/qa/cppunit/test_misc \
))

$(eval $(call gb_CppunitTest_use_sdk_api,sfx2_misc))

$(eval $(call gb_CppunitTest_use_libraries,sfx2_misc, \
	comphelper \
	cppu \
	cppuhelper \
	test \
	unotest \
	vcl \
	sal \
    subsequenttest \
	sfx \
	utl \
	tl \
))

$(eval $(call gb_CppunitTest_use_externals,sfx2_misc,\
	libxml2 \
	boost_headers \
))

$(eval $(call gb_CppunitTest_use_sdk_api,sfx2_misc))

$(eval $(call gb_CppunitTest_use_ure,sfx2_misc))
$(eval $(call gb_CppunitTest_use_vcl,sfx2_misc))

$(eval $(call gb_CppunitTest_use_rdb,sfx2_misc,services))

$(eval $(call gb_CppunitTest_use_configuration,sfx2_misc))

# vim: set noet sw=4 ts=4:
