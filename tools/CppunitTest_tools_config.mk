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

$(eval $(call gb_CppunitTest_CppunitTest,tools_config))

$(eval $(call gb_CppunitTest_use_external,tools_config,boost_headers))

$(eval $(call gb_CppunitTest_add_exception_objects,tools_config, \
    tools/qa/cppunit/test_config \
))

$(eval $(call gb_CppunitTest_use_sdk_api,tools_config))

$(eval $(call gb_CppunitTest_use_libraries,tools_config, \
    sal \
    tl \
    test \
    unotest \
))

$(eval $(call gb_CppunitTest_use_static_libraries,tools_config, \
    ooopathutils \
))

$(eval $(call gb_CppunitTest_set_include,tools_config,\
    $$(INCLUDE) \
    -I$(SRCDIR)/tools/inc \
))

# vim: set noet sw=4 ts=4:
