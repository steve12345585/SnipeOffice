# -*- Mode: makefile-gmake; tab-width: 4; indent-tabs-mode: t -*-
#
#
# This file is Part of the SnipeOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

$(eval $(call gb_CppunitTest_CppunitTest,sal_osl_security))

$(eval $(call gb_CppunitTest_add_exception_objects,sal_osl_security,\
    sal/qa/osl/security/osl_Security \
))

$(eval $(call gb_CppunitTest_use_libraries,sal_osl_security,\
    sal \
))

# vim: set noet sw=4 ts=4:
