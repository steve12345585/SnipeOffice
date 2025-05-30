# -*- Mode: makefile-gmake; tab-width: 4; indent-tabs-mode: t -*-
#
# This file is Part of the SnipeOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

$(eval $(call gb_CppunitTest_CppunitTest,sdext_pdfimport))

$(eval $(call gb_CppunitTest_use_sdk_api,sdext_pdfimport))

$(eval $(call gb_CppunitTest_set_include,sdext_pdfimport,\
    -I$(SRCDIR)/sdext/source/pdfimport/inc \
    -I$(SRCDIR)/sdext/source/pdfimport/test \
    $$(INCLUDE) \
))

$(eval $(call gb_CppunitTest_use_externals,sdext_pdfimport,\
    boost_headers \
    zlib \
    libxml2 \
))

$(eval $(call gb_CppunitTest_use_libraries,sdext_pdfimport,\
    basegfx \
    vcl \
    comphelper \
    cppu \
    cppuhelper \
    drawinglayercore \
    drawinglayer \
    sal \
    salhelper \
    test \
    unotest \
    tl \
    xo \
    i18nutil \
))

$(eval $(call gb_CppunitTest_use_library_objects,sdext_pdfimport,pdfimport))

$(eval $(call gb_CppunitTest_add_exception_objects,sdext_pdfimport,\
    sdext/qa/unit/pdfimport \
))

$(eval $(call gb_CppunitTest_use_executable,sdext_pdfimport,xpdfimport))

$(eval $(call gb_CppunitTest_use_ure,sdext_pdfimport))
$(eval $(call gb_CppunitTest_use_vcl,sdext_pdfimport))

$(eval $(call gb_CppunitTest_use_components,sdext_pdfimport,\
    comphelper/util/comphelp \
    configmgr/source/configmgr \
    i18npool/util/i18npool \
    ucb/source/core/ucb1 \
    ucb/source/ucp/file/ucpfile1 \
))

$(eval $(call gb_CppunitTest_use_configuration,sdext_pdfimport))

# vim:set noet sw=4 ts=4:
