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

$(eval $(call gb_CppunitTest_CppunitTest,sc_shapetest))

$(eval $(call gb_CppunitTest_use_externals,sc_shapetest, \
	boost_headers \
	mdds_headers \
	libxml2 \
))

$(eval $(call gb_CppunitTest_use_common_precompiled_header,sc_shapetest))

$(eval $(call gb_CppunitTest_add_exception_objects,sc_shapetest, \
    sc/qa/unit/scshapetest \
))

$(eval $(call gb_CppunitTest_use_libraries,sc_shapetest, \
    cppu \
    cppuhelper \
    sal \
    sc \
    scqahelper \
    sfx \
    subsequenttest \
    svl \
    svx \
    svxcore \
    test \
    tl \
    unotest \
    utl \
    vcl \
))

$(eval $(call gb_CppunitTest_set_include,sc_shapetest,\
    -I$(SRCDIR)/sc/source/ui/inc \
    -I$(SRCDIR)/sc/inc \
    $$(INCLUDE) \
))

$(eval $(call gb_CppunitTest_use_sdk_api,sc_shapetest))

$(eval $(call gb_CppunitTest_use_ure,sc_shapetest))
$(eval $(call gb_CppunitTest_use_vcl,sc_shapetest))

$(eval $(call gb_CppunitTest_use_rdb,sc_shapetest,services))

$(eval $(call gb_CppunitTest_use_configuration,sc_shapetest))

$(eval $(call gb_CppunitTest_add_arguments,sc_shapetest, \
    -env:arg-env=$(gb_Helper_LIBRARY_PATH_VAR)"$$$${$(gb_Helper_LIBRARY_PATH_VAR)+=$$$$$(gb_Helper_LIBRARY_PATH_VAR)}" \
))

# vim: set noet sw=4 ts=4:
