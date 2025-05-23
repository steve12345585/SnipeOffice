# -*- Mode: makefile-gmake; tab-width: 4; indent-tabs-mode: t -*-
#
# This file is Part of the SnipeOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

$(eval $(call gb_Library_Library,vclbootstrapprotector))

$(eval $(call gb_Library_add_exception_objects,vclbootstrapprotector, \
    test/source/vclbootstrapprotector \
))

$(eval $(call gb_Library_use_externals,vclbootstrapprotector, \
    boost_headers \
    cppunit \
))

$(eval $(call gb_Library_use_libraries,vclbootstrapprotector, \
    comphelper \
    cppu \
    i18nlangtag \
    sal \
    test-setupvcl \
    tl \
    utl \
    vcl \
))

$(eval $(call gb_Library_use_sdk_api,vclbootstrapprotector))

# vim: set noet sw=4 ts=4:
