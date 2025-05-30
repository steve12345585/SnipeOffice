# -*- Mode: makefile-gmake; tab-width: 4; indent-tabs-mode: t -*-
#
# This file is Part of the SnipeOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

$(eval $(call gb_CppunitTest_CppunitTest,svl_inetcontenttype))

$(eval $(call gb_CppunitTest_add_exception_objects,svl_inetcontenttype, \
    svl/qa/unit/test_INetContentType \
))

$(eval $(call gb_CppunitTest_use_api,svl_inetcontenttype, \
    udkapi \
))

$(eval $(call gb_CppunitTest_use_externals,svl_inetcontenttype, \
    boost_headers \
))

$(eval $(call gb_CppunitTest_use_libraries,svl_inetcontenttype, \
    sal \
    svl \
    tl \
))

# vim: set noet sw=4 ts=4:
