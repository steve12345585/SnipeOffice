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

$(eval $(call gb_CppunitTest_CppunitTest,sc_functionlistobj))

$(eval $(call gb_CppunitTest_use_external,sc_functionlistobj,boost_headers))

$(eval $(call gb_CppunitTest_use_common_precompiled_header,sc_functionlistobj))

$(eval $(call gb_CppunitTest_add_exception_objects,sc_functionlistobj, \
	sc/qa/extras/scfunctionlistobj \
))

$(eval $(call gb_CppunitTest_use_libraries,sc_functionlistobj, \
	cppu \
	cppuhelper \
	sal \
	subsequenttest \
	test \
	unotest \
	utl \
	tl \
))

$(eval $(call gb_CppunitTest_set_include,sc_functionlistobj,\
	$$(INCLUDE) \
))

$(eval $(call gb_CppunitTest_use_sdk_api,sc_functionlistobj))

$(eval $(call gb_CppunitTest_use_ure,sc_functionlistobj))
$(eval $(call gb_CppunitTest_use_vcl,sc_functionlistobj))

$(eval $(call gb_CppunitTest_use_components,sc_functionlistobj,\
    basic/util/sb \
    comphelper/util/comphelp \
    configmgr/source/configmgr \
    dbaccess/util/dba \
    filter/source/config/cache/filterconfig1 \
    filter/source/storagefilterdetect/storagefd \
    forms/util/frm \
    framework/util/fwk \
    i18npool/source/search/i18nsearch \
    i18npool/util/i18npool \
    linguistic/source/lng \
    oox/util/oox \
    package/source/xstor/xstor \
    package/util/package2 \
    sax/source/expatwrap/expwrap \
    scripting/source/basprov/basprov \
    scripting/util/scriptframe \
    sc/util/sc \
    sc/util/scd \
    sc/util/scfilt \
    $(call gb_Helper_optional,SCRIPTING, sc/util/vbaobj) \
    sfx2/util/sfx \
    sot/util/sot \
    svl/source/fsstor/fsstorage \
    toolkit/util/tk \
    ucb/source/core/ucb1 \
    ucb/source/ucp/file/ucpfile1 \
    ucb/source/ucp/tdoc/ucptdoc1 \
    unotools/util/utl \
    unoxml/source/rdf/unordf \
    unoxml/source/service/unoxml \
    uui/util/uui \
    vcl/vcl.common \
    xmloff/util/xo \
    svtools/util/svt \
))

$(eval $(call gb_CppunitTest_use_configuration,sc_functionlistobj))

# vim: set noet sw=4 ts=4:
