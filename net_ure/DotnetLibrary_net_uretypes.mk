# -*- Mode: makefile-gmake; tab-width: 4; indent-tabs-mode: t -*-
#
# This file is Part of the SnipeOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

$(eval $(call gb_DotnetLibrary_DotnetLibrary,net_uretypes,$(gb_DotnetLibrary_CS)))

$(eval $(call gb_DotnetLibrary_use_customtarget,net_uretypes,net_ure/net_uretypes))

$(eval $(call gb_DotnetLibrary_link_library,net_uretypes,net_basetypes))

$(eval $(call gb_DotnetLibrary_add_properties,net_uretypes,\
	<Version>0.1.0</Version> \
	<Company>LibreOffice</Company> \
	<Description>UNO runtime datatypes for the .NET language UNO binding.</Description> \
))

# vim: set noet sw=4 ts=4:
