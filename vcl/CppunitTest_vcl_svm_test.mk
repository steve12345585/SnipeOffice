# -*- Mode: makefile-gmake; tab-width: 4; indent-tabs-mode: t -*-
#
# This file is Part of the SnipeOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

$(eval $(call gb_CppunitTest_CppunitTest,vcl_svm_test))

$(eval $(call gb_CppunitTest_add_exception_objects,vcl_svm_test, \
    vcl/qa/cppunit/svm/svmtest \
))

$(eval $(call gb_CppunitTest_use_externals,vcl_svm_test,\
	boost_headers \
    libxml2 \
))

$(eval $(call gb_CppunitTest_set_include,vcl_svm_test,\
    $$(INCLUDE) \
    -I$(SRCDIR)/vcl/inc \
))

$(eval $(call gb_CppunitTest_use_libraries,vcl_svm_test, \
	basegfx \
	comphelper \
	cppu \
	cppuhelper \
	sal \
	salhelper \
    svt \
	test \
	tl \
	unotest \
	vcl \
	utl \
))

$(eval $(call gb_CppunitTest_use_sdk_api,vcl_svm_test))

$(eval $(call gb_CppunitTest_use_ure,vcl_svm_test))
$(eval $(call gb_CppunitTest_use_vcl,vcl_svm_test))

$(eval $(call gb_CppunitTest_use_components,vcl_svm_test,\
    configmgr/source/configmgr \
    i18npool/util/i18npool \
    ucb/source/core/ucb1 \
    unotools/util/utl \
))

$(eval $(call gb_CppunitTest_use_configuration,vcl_svm_test))

$(eval $(call gb_CppunitTest_use_more_fonts,vcl_svm_test))

# vim: set noet sw=4 ts=4:
