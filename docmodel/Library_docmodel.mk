# -*- Mode: makefile-gmake; tab-width: 4; indent-tabs-mode: t; fill-column: 100 -*-
#
# This file is Part of the SnipeOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

$(eval $(call gb_Library_Library,docmodel))

$(eval $(call gb_Library_add_exception_objects,docmodel,\
    docmodel/source/uno/UnoComplexColor \
    docmodel/source/uno/UnoGradientTools \
    docmodel/source/uno/UnoTheme \
    docmodel/source/theme/ColorSet \
    docmodel/source/theme/Theme \
    docmodel/source/color/ComplexColorJSON \
))

$(eval $(call gb_Library_set_include,docmodel,\
    $$(INCLUDE) \
    -I$(SRCDIR)/docmodel/inc \
))

$(eval $(call gb_Library_use_externals,docmodel,\
    libxml2 \
    boost_headers \
))

$(eval $(call gb_Library_add_defs,docmodel,\
    -DDOCMODEL_DLLIMPLEMENTATION \
))

$(eval $(call gb_Library_set_precompiled_header,docmodel,docmodel/inc/pch/precompiled_docmodel))

$(eval $(call gb_Library_use_sdk_api,docmodel))

$(eval $(call gb_Library_use_libraries,docmodel,\
    basegfx \
    comphelper \
    cppuhelper \
    cppu \
    sal \
    tl \
))

# vim: set noet sw=4 ts=4:
