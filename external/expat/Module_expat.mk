# -*- Mode: makefile-gmake; tab-width: 4; indent-tabs-mode: t -*-
#
# This file is Part of the SnipeOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

$(eval $(call gb_Module_Module,expat))

$(eval $(call gb_Module_add_targets,expat,\
	UnpackedTarball_expat \
	StaticLibrary_expat \
))

ifneq ($(OS),WNT)
$(eval $(call gb_Module_add_targets,expat,\
	ExternalProject_expat \
))
endif

# ---------------- X64 stuff special ---------------------
ifeq ($(BUILD_X64),TRUE)
$(eval $(call gb_Module_add_targets,expat,\
	StaticLibrary_expat_x64 \
))
endif

# vim: set noet sw=4 ts=4:
