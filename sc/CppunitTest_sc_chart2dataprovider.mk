# -*- Mode: makefile-gmake; tab-width: 4; indent-tabs-mode: t -*-
#
# This file is Part of the SnipeOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

$(eval $(call gb_CppunitTest_CppunitTest,sc_chart2dataprovider))

$(eval $(call gb_CppunitTest_use_common_precompiled_header,sc_chart2dataprovider))

$(eval $(call gb_CppunitTest_add_exception_objects,sc_chart2dataprovider, \
    sc/qa/unit/chart2dataprovider \
))

$(eval $(call gb_CppunitTest_use_externals,sc_chart2dataprovider, \
	boost_headers \
	mdds_headers \
	libxml2 \
))

$(eval $(call gb_CppunitTest_use_libraries,sc_chart2dataprovider, \
    basegfx \
    comphelper \
    cppu \
    cppuhelper \
    drawinglayer \
    drawinglayercore \
    editeng \
    for \
    forui \
    i18nlangtag \
    msfilter \
    oox \
    sal \
    salhelper \
    sax \
    sb \
    sc \
    scqahelper \
    sfx \
    sot \
    subsequenttest \
    svl \
    svt \
    svx \
    svxcore \
	test \
    tk \
    tl \
    ucbhelper \
	unotest \
    utl \
    $(call gb_Helper_optional,SCRIPTING, \
        vbahelper) \
    vcl \
    xo \
	$(gb_UWINAPI) \
))

$(eval $(call gb_CppunitTest_set_include,sc_chart2dataprovider,\
    -I$(SRCDIR)/sc/source/ui/inc \
    -I$(SRCDIR)/sc/inc \
    $$(INCLUDE) \
))

$(eval $(call gb_CppunitTest_use_api,sc_chart2dataprovider,\
	udkapi \
	offapi \
	oovbaapi \
))

$(eval $(call gb_CppunitTest_use_ure,sc_chart2dataprovider))
$(eval $(call gb_CppunitTest_use_vcl,sc_chart2dataprovider))

$(eval $(call gb_CppunitTest_use_rdb,sc_chart2dataprovider,services))

$(eval $(call gb_CppunitTest_use_configuration,sc_chart2dataprovider))

# vim: set noet sw=4 ts=4:
