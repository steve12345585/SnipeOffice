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

$(eval $(call gb_CppunitTest_CppunitTest,sw_ooxmlw14export))

$(eval $(call gb_CppunitTest_use_common_precompiled_header,sw_ooxmlw14export))

$(eval $(call gb_CppunitTest_add_exception_objects,sw_ooxmlw14export, \
    sw/qa/extras/ooxmlexport/ooxmlw14export \
))

$(eval $(call gb_CppunitTest_use_libraries,sw_ooxmlw14export, \
	$(sw_ooxmlexport_libraries) \
))

$(eval $(call gb_CppunitTest_use_externals,sw_ooxmlw14export,\
	boost_headers \
    libxml2 \
))

$(eval $(call gb_CppunitTest_set_include,sw_ooxmlw14export,\
    -I$(SRCDIR)/sw/inc \
    -I$(SRCDIR)/sw/source/core/inc \
	-I$(SRCDIR)/sw/qa/inc \
    $$(INCLUDE) \
))

$(eval $(call gb_CppunitTest_use_api,sw_ooxmlw14export,\
	udkapi \
	offapi \
	oovbaapi \
))

$(eval $(call gb_CppunitTest_use_ure,sw_ooxmlw14export))
$(eval $(call gb_CppunitTest_use_vcl,sw_ooxmlw14export))

$(eval $(call gb_CppunitTest_use_rdb,sw_ooxmlw14export,services))

$(eval $(call gb_CppunitTest_use_configuration,sw_ooxmlw14export))

ifneq ($(filter MORE_FONTS,$(BUILD_TYPE)),)
$(eval $(call gb_CppunitTest_set_non_application_font_use,sw_ooxmlw14export,abort))
endif

# vim: set noet sw=4 ts=4:
