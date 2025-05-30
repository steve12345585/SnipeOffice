# -*- Mode: makefile-gmake; tab-width: 4; indent-tabs-mode: t -*-
#
# This file is Part of the SnipeOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

$(eval $(call gb_Library_Library,unopkgapp))

$(eval $(call gb_Library_set_include,unopkgapp,\
    $$(INCLUDE) \
    -I$(SRCDIR)/desktop/inc \
    -I$(SRCDIR)/desktop/source/deployment/inc \
    -I$(SRCDIR)/desktop/source/inc \
))

$(eval $(call gb_Library_use_external,unopkgapp,boost_headers))

$(eval $(call gb_Library_use_sdk_api,unopkgapp))

$(eval $(call gb_Library_add_defs,unopkgapp,\
    -DDESKTOP_DLLIMPLEMENTATION \
))

$(eval $(call gb_Library_use_libraries,unopkgapp,\
    comphelper \
    cppu \
    cppuhelper \
    deploymentmisc \
    sal \
    tl \
    ucbhelper \
    utl \
    vcl \
    i18nlangtag \
))

$(eval $(call gb_Library_add_exception_objects,unopkgapp,\
    desktop/source/pkgchk/unopkg/unopkg_app \
    desktop/source/pkgchk/unopkg/unopkg_cmdenv \
    desktop/source/pkgchk/unopkg/unopkg_misc \
))

# vim: set ts=4 sw=4 et:
