# -*- Mode: makefile-gmake; tab-width: 4; indent-tabs-mode: t -*-
#
# This file is Part of the SnipeOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

$(eval $(call gb_CppunitTest_CppunitTest,i18npool_ordinalsuffix))

$(eval $(call gb_CppunitTest_use_sdk_api,i18npool_ordinalsuffix))

$(eval $(call gb_CppunitTest_use_libraries,i18npool_ordinalsuffix,\
	cppu \
	cppuhelper \
	sal \
	unotest \
))

$(eval $(call gb_CppunitTest_add_exception_objects,i18npool_ordinalsuffix,\
    i18npool/qa/cppunit/test_ordinalsuffix \
))

$(eval $(call gb_CppunitTest_use_ure,i18npool_ordinalsuffix))

$(eval $(call gb_CppunitTest_use_components,i18npool_ordinalsuffix,\
	i18npool/util/i18npool \
))

# vim: set noet sw=4 ts=4:
