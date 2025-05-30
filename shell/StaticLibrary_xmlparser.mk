# -*- Mode: makefile-gmake; tab-width: 4; indent-tabs-mode: t -*-
#
# This file is Part of the SnipeOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

$(eval $(call gb_StaticLibrary_StaticLibrary,shell_xmlparser))

$(eval $(call gb_StaticLibrary_set_include,shell_xmlparser,\
	-I$(SRCDIR)/shell/inc \
	$$(INCLUDE) \
))

$(eval $(call gb_StaticLibrary_use_externals,shell_xmlparser,\
	expat \
))

$(eval $(call gb_StaticLibrary_add_exception_objects,shell_xmlparser,\
    shell/source/all/xml_parser \
))

# vim: set shiftwidth=4 tabstop=4 noexpandtab:
