# -*- Mode: makefile-gmake; tab-width: 4; indent-tabs-mode: t -*-
#
# This file is Part of the SnipeOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

$(eval $(call gb_Module_Module,io))

$(eval $(call gb_Module_add_targets,io,\
    Library_io \
))

$(eval $(call gb_Module_add_subsequentcheck_targets,io,\
    CppunitTest_io_textinputstream \
))

ifneq (,$(filter Executable_io-testconnection,$(MAKECMDGOALS)))
$(eval $(call gb_Module_add_targets,io, \
    Executable_io-testconnection \
))
endif

# vim:set noet sw=4 ts=4:
