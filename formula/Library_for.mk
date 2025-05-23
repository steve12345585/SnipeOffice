# -*- Mode: makefile-gmake; tab-width: 4; indent-tabs-mode: t -*-
#
# This file is Part of the SnipeOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

$(eval $(call gb_Library_Library,for))

$(eval $(call gb_Library_set_include,for,\
    $$(INCLUDE) \
    -I$(SRCDIR)/formula/inc \
))

$(eval $(call gb_Library_add_defs,for,\
    -DFORMULA_DLLIMPLEMENTATION \
))

$(eval $(call gb_Library_use_external,for,boost_headers))

$(eval $(call gb_Library_use_sdk_api,for))

$(eval $(call gb_Library_use_libraries,for,\
    comphelper \
    cppu \
    cppuhelper \
    sal \
	i18nlangtag \
    svl \
    svt \
    tl \
    utl \
    vcl \
))

$(eval $(call gb_Library_set_componentfile,for,formula/util/for,services))

$(eval $(call gb_Library_add_exception_objects,for,\
    formula/source/core/api/FormulaCompiler \
    formula/source/core/api/FormulaOpCodeMapperObj \
    formula/source/core/api/grammar \
    formula/source/core/api/token \
    formula/source/core/api/vectortoken \
    formula/source/core/resource/core_resource \
))

# vim: set noet sw=4 ts=4:
