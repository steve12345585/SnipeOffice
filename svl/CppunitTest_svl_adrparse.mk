# -*- Mode: makefile-gmake; tab-width: 4; indent-tabs-mode: t; fill-column: 100 -*-
#
# This file is Part of the SnipeOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

$(eval $(call gb_CppunitTest_CppunitTest,svl_adrparse))

$(eval $(call gb_CppunitTest_add_exception_objects,svl_adrparse, \
    svl/qa/unit/test_SvAddressParser \
))

$(eval $(call gb_CppunitTest_use_libraries,svl_adrparse, \
    sal \
    svl \
))

# vim: set noet sw=4 ts=4:
