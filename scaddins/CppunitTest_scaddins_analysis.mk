# -*- Mode: makefile-gmake; tab-width: 4; indent-tabs-mode: t; fill-column: 100 -*-
#
# This file is Part of the SnipeOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

$(eval $(call gb_CppunitTest_CppunitTest,scaddins_analysis))

$(eval $(call gb_CppunitTest_add_exception_objects,scaddins_analysis, \
    scaddins/qa/analysis \
))

$(eval $(call gb_CppunitTest_use_libraries,scaddins_analysis, \
    comphelper \
    cppu \
    sal \
    test \
    unotest \
))


$(eval $(call gb_CppunitTest_use_ure,scaddins_analysis))
$(eval $(call gb_CppunitTest_use_vcl,scaddins_analysis))

$(eval $(call gb_CppunitTest_use_rdb,scaddins_analysis,services))

$(eval $(call gb_CppunitTest_use_configuration,scaddins_analysis))

$(eval $(call gb_CppunitTest_use_sdk_api,scaddins_analysis))

$(eval $(call gb_CppunitTest_use_internal_comprehensive_api,scaddins_analysis,\
    scaddins \
))

# vim: set noet sw=4 ts=4:
