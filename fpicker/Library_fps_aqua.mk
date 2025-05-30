# -*- Mode: makefile-gmake; tab-width: 4; indent-tabs-mode: t -*-
#
#
# This file is Part of the SnipeOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

$(eval $(call gb_Library_Library,fps_aqua))

$(eval $(call gb_Library_set_componentfile,fps_aqua,fpicker/source/aqua/fps_aqua,services))

$(eval $(call gb_Library_set_include,fps_aqua,\
    $$(INCLUDE) \
    -I$(SRCDIR)/fpicker/inc \
))

$(eval $(call gb_Library_use_external,fps_aqua,boost_headers))

$(eval $(call gb_Library_use_sdk_api,fps_aqua))

$(eval $(call gb_Library_use_system_darwin_frameworks,fps_aqua,\
    Cocoa \
    CoreFoundation \
))

$(eval $(call gb_Library_use_libraries,fps_aqua,\
	cppu \
	cppuhelper \
	i18nlangtag \
	sal \
	utl \
	vcl \
))

$(eval $(call gb_Library_add_objcxxobjects,fps_aqua,\
	fpicker/source/aqua/AquaFilePickerDelegate \
	fpicker/source/aqua/ControlHelper \
	fpicker/source/aqua/FilterHelper \
	fpicker/source/aqua/NSString_OOoAdditions \
	fpicker/source/aqua/NSURL_OOoAdditions \
	fpicker/source/aqua/resourceprovider \
	fpicker/source/aqua/SalAquaFilePicker \
	fpicker/source/aqua/SalAquaFolderPicker \
	fpicker/source/aqua/SalAquaPicker \
))

# vim: set noet sw=4 ts=4:
