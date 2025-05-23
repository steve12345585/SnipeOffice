# -*- Mode: makefile-gmake; tab-width: 4; indent-tabs-mode: t -*-
#
# This file is Part of the SnipeOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

$(eval $(call gb_CppunitTest_CppunitTest,vcl_jpeg_read_write_test))

$(eval $(call gb_CppunitTest_add_exception_objects,vcl_jpeg_read_write_test, \
    vcl/qa/cppunit/jpeg/JpegReaderTest \
    vcl/qa/cppunit/jpeg/JpegWriterTest \
))

$(eval $(call gb_CppunitTest_use_external,vcl_jpeg_read_write_test,boost_headers))

$(eval $(call gb_CppunitTest_set_include,vcl_jpeg_read_write_test,\
    $$(INCLUDE) \
    -I$(SRCDIR)/vcl/inc \
))

$(eval $(call gb_CppunitTest_use_libraries,vcl_jpeg_read_write_test, \
	comphelper \
	cppu \
	cppuhelper \
	sal \
    svt \
	test \
	tl \
	unotest \
	vcl \
))

$(eval $(call gb_CppunitTest_use_sdk_api,vcl_jpeg_read_write_test))

$(eval $(call gb_CppunitTest_use_ure,vcl_jpeg_read_write_test))
$(eval $(call gb_CppunitTest_use_vcl,vcl_jpeg_read_write_test))

$(eval $(call gb_CppunitTest_use_components,vcl_jpeg_read_write_test,\
    configmgr/source/configmgr \
    i18npool/util/i18npool \
    ucb/source/core/ucb1 \
    ucb/source/ucp/file/ucpfile1 \
    uui/util/uui \
))

$(eval $(call gb_CppunitTest_use_configuration,vcl_jpeg_read_write_test))

# vim: set noet sw=4 ts=4:
