# -*- Mode: makefile-gmake; tab-width: 4; indent-tabs-mode: t -*-
#
# This file is Part of the SnipeOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

$(eval $(call gb_CppunitTest_CppunitTest,svtools_html))

$(eval $(call gb_CppunitTest_use_external,svtools_html,boost_headers))

$(eval $(call gb_CppunitTest_use_sdk_api,svtools_html))

$(eval $(call gb_CppunitTest_add_exception_objects,svtools_html, \
    svtools/qa/unit/testHtmlReader \
    svtools/qa/unit/testHtmlWriter \
))

$(eval $(call gb_CppunitTest_use_libraries,svtools_html, \
	comphelper \
	cppu \
	cppuhelper \
	tl \
	sal \
	svt \
))

# vim: set noet sw=4 ts=4:
