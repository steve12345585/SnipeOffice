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

$(eval $(call gb_CppunitTest_CppunitTest,cppcanvas_emfplus))

$(eval $(call gb_CppunitTest_add_exception_objects,cppcanvas_emfplus, \
	cppcanvas/qa/extras/emfplus/emfplus \
))

$(eval $(call gb_CppunitTest_use_libraries,cppcanvas_emfplus, \
	comphelper \
	cppu \
	cppuhelper \
	sal \
	sfx \
	subsequenttest \
	test \
	tl \
	unotest \
    utl \
	vcl \
))

$(eval $(call gb_CppunitTest_use_external,cppcanvas_emfplus,boost_headers))

$(eval $(call gb_CppunitTest_use_sdk_api,cppcanvas_emfplus))

$(eval $(call gb_CppunitTest_use_ure,cppcanvas_emfplus))
$(eval $(call gb_CppunitTest_use_vcl_non_headless,cppcanvas_emfplus))

$(eval $(call gb_CppunitTest_use_components,cppcanvas_emfplus,\
	canvas/source/cairo/cairocanvas \
	canvas/source/factory/canvasfactory \
	cppcanvas/source/uno/mtfrenderer \
	configmgr/source/configmgr \
	emfio/emfio \
	extensions/source/scanner/scn \
	filter/source/config/cache/filterconfig1 \
	framework/util/fwk \
	i18npool/util/i18npool \
	package/util/package2 \
	sax/source/expatwrap/expwrap \
	sfx2/util/sfx \
	sd/util/sd \
	sd/util/sdd \
	svl/source/fsstor/fsstorage \
	toolkit/util/tk \
	vcl/vcl.common \
	ucb/source/core/ucb1 \
	ucb/source/ucp/file/ucpfile1 \
	unoxml/source/service/unoxml \
	uui/util/uui \
	svtools/util/svt \
))

$(eval $(call gb_CppunitTest_use_configuration,cppcanvas_emfplus))

# vim: set noet sw=4 ts=4:
