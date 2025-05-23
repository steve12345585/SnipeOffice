# -*- Mode: makefile-gmake; tab-width: 4; indent-tabs-mode: t -*-
#
# This file is Part of the SnipeOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

$(eval $(call gb_Executable_Executable,tilebench))

$(eval $(call gb_Executable_set_include,tilebench,\
    $$(INCLUDE) \
    -I$(SRCDIR)/desktop/inc \
))

$(eval $(call gb_Executable_use_externals,tilebench,\
    boost_headers \
))

$(eval $(call gb_Executable_use_libraries,tilebench,\
	sal \
))

$(eval $(call gb_Executable_add_libs,tilebench,\
    -lm $(UNIX_DLAPI_LIBS) \
))

$(eval $(call gb_Executable_add_exception_objects,tilebench,\
    libreofficekit/qa/tilebench/tilebench \
))

# vim: set noet sw=4 ts=4:
